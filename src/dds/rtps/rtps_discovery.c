/**
 * @file rtps_discovery.c
 * @brief RTPS发现协议实现 - 简单发现协议(SDP)
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现RTPS发现协议的核心逻辑：
 * - SPDP: 简单参与者发现协议（Simple Participant Discovery Protocol）
 * - SEDP: 简单端点发现协议（Simple Endpoint Discovery Protocol）
 */

#include "rtps_discovery.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */

#define RTPS_MAGIC_COOKIE     0x52545053  /* "RTPS" */
#define RTPS_VENDOR_ID        0x0100      /* Vendor ID */

/* Entity ID定义 */
#define ENTITYID_UNKNOWN                     0x00000000
#define ENTITYID_PARTICIPANT                 0x000001C1
#define ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER   0x000003C2
#define ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER   0x000003C7
#define ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER  0x000004C2
#define ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER  0x000004C7
#define ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER    0x000100C2
#define ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER    0x000100C7
#define ENTITYID_BUILTIN_PARTICIPANT_MESSAGE_WRITER 0x000200C2
#define ENTITYID_BUILTIN_PARTICIPANT_MESSAGE_READER 0x000200C7

/* 内置端点位图 */
#define BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER      0x00000001
#define BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR       0x00000002
#define BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER      0x00000004
#define BUILTIN_ENDPOINT_PUBLICATION_DETECTOR       0x00000008
#define BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER     0x00000010
#define BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR      0x00000020
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_WRITER 0x00000040
#define BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_READER 0x00000080

/* 编解码常量 */
#define RTPS_SUBMESSAGE_HEADER_SIZE     4
#define RTPS_PARAMETER_ID_PAD           0x0000
#define RTPS_PARAMETER_ID_SENTINEL      0x0001

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 清除过期的参与者
 */
static void rtps_discovery_purge_expired_participants(rtps_discovery_context_t *ctx,
                                                         uint64_t current_time_ms)
{
    rtps_participant_proxy_t **current = &ctx->participants;
    
    while (*current != NULL) {
        rtps_participant_proxy_t *participant = *current;
        uint64_t lease_expiry = participant->last_seen_timestamp + participant->lease_duration_ms;
        
        if (current_time_ms > lease_expiry) {
            /* 移除过期参与者 */
            *current = participant->next;
            ctx->participants_discovered--;
            free(participant);
        } else {
            current = &participant->next;
        }
    }
}

/**
 * @brief 创建新的参与者代理
 */
static rtps_participant_proxy_t* rtps_participant_create(
    const rtps_guid_prefix_t guid_prefix,
    const uint8_t *data,
    uint32_t len)
{
    rtps_participant_proxy_t *participant = (rtps_participant_proxy_t *)malloc(
        sizeof(rtps_participant_proxy_t));
    if (participant == NULL) {
        return NULL;
    }
    
    memset(participant, 0, sizeof(rtps_participant_proxy_t));
    memcpy(participant->guid_prefix, guid_prefix, RTPS_GUID_PREFIX_SIZE);
    memcpy(participant->guid.prefix, guid_prefix, RTPS_GUID_PREFIX_SIZE);
    participant->guid.entity_id[0] = 0x00;
    participant->guid.entity_id[1] = 0x00;
    participant->guid.entity_id[2] = 0x01;
    participant->guid.entity_id[3] = 0xC1;
    
    participant->protocol_version = (2 << 16) | 4;
    participant->vendor_id = RTPS_VENDOR_ID;
    participant->lease_duration_ms = RTPS_DISCOVERY_LEASE_DURATION_MS;
    participant->state = RTPS_DISCOVERY_STATE_DISCOVERING;
    
    /* 解析发现数据（简化版本） */
    participant->available_builtin_endpoints = 
        BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
        BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
        BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
        BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
        BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
        BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
    
    return participant;
}

/**
 * @brief 更新参与者活跃时间
 */
static void rtps_participant_update_liveliness(rtps_participant_proxy_t *participant,
                                                uint64_t current_time_ms)
{
    participant->last_seen_timestamp = current_time_ms;
    if (participant->state == RTPS_DISCOVERY_STATE_DISCOVERING) {
        participant->state = RTPS_DISCOVERY_STATE_MATCHED;
    }
}

/**
 * @brief 匹配端点
 */
static void rtps_discovery_match_endpoints(rtps_discovery_context_t *ctx,
                                            rtps_endpoint_proxy_t *endpoint)
{
    rtps_endpoint_proxy_t *iter = ctx->endpoints;
    
    while (iter != NULL) {
        if (iter != endpoint && 
            iter->state == RTPS_DISCOVERY_STATE_DISCOVERING) {
            /* 检查主题和类型是否匹配 */
            if (strcmp(iter->topic_name, endpoint->topic_name) == 0 &&
                strcmp(iter->type_name, endpoint->type_name) == 0) {
                /* 检查读写关系（Writer匹配Reader） */
                if ((iter->type == RTPS_ENDPOINT_TYPE_WRITER && 
                     endpoint->type == RTPS_ENDPOINT_TYPE_READER) ||
                    (iter->type == RTPS_ENDPOINT_TYPE_READER && 
                     endpoint->type == RTPS_ENDPOINT_TYPE_WRITER)) {
                    
                    iter->state = RTPS_DISCOVERY_STATE_MATCHED;
                    endpoint->state = RTPS_DISCOVERY_STATE_MATCHED;
                    ctx->endpoints_matched++;
                    
                    if (ctx->on_endpoint_matched) {
                        ctx->on_endpoint_matched(endpoint, true);
                    }
                }
            }
        }
        iter = iter->next;
    }
}

/**
 * @brief 清理无效的端点条目
 */
static void rtps_endpoint_cleanup_orphaned(rtps_discovery_context_t *ctx)
{
    rtps_endpoint_proxy_t **current = &ctx->endpoints;
    
    while (*current != NULL) {
        rtps_endpoint_proxy_t *endpoint = *current;
        
        /* 检查端点对应的参与者是否仍存在 */
        if (rtps_discovery_find_participant(ctx, endpoint->participant_guid.prefix) == NULL) {
            /* 移除孤立端点 */
            *current = endpoint->next;
            free(endpoint);
        } else {
            current = &endpoint->next;
        }
    }
}

/**
 * @brief 序列化参与者发现数据（CDR格式）
 */
static uint32_t serialize_participant_data(const rtps_participant_proxy_t *participant,
                                            uint8_t *buffer,
                                            uint32_t max_len)
{
    uint32_t pos = 0;
    
    if (max_len < 64) return 0;
    
    /* 协议版本 */
    buffer[pos++] = RTPS_PROTOCOL_VERSION_MAJOR;
    buffer[pos++] = RTPS_PROTOCOL_VERSION_MINOR;
    pos += 2; /* padding */
    
    /* Vendor ID */
    buffer[pos++] = (RTPS_VENDOR_ID >> 8) & 0xFF;
    buffer[pos++] = RTPS_VENDOR_ID & 0xFF;
    pos += 2; /* padding */
    
    /* GUID Prefix */
    memcpy(&buffer[pos], participant->guid_prefix, RTPS_GUID_PREFIX_SIZE);
    pos += RTPS_GUID_PREFIX_SIZE;
    
    /* Expects Inline QoS */
    buffer[pos++] = participant->expects_inline_qos ? 1 : 0;
    pos += 3; /* padding */
    
    /* Available Builtin Endpoints */
    buffer[pos++] = (participant->available_builtin_endpoints >> 24) & 0xFF;
    buffer[pos++] = (participant->available_builtin_endpoints >> 16) & 0xFF;
    buffer[pos++] = (participant->available_builtin_endpoints >> 8) & 0xFF;
    buffer[pos++] = participant->available_builtin_endpoints & 0xFF;
    
    /* Lease Duration */
    buffer[pos++] = (participant->lease_duration_ms >> 24) & 0xFF;
    buffer[pos++] = (participant->lease_duration_ms >> 16) & 0xFF;
    buffer[pos++] = (participant->lease_duration_ms >> 8) & 0xFF;
    buffer[pos++] = participant->lease_duration_ms & 0xFF;
    
    /* AUTOSAR扩展 */
    if (participant->autosar_deterministic) {
        buffer[pos++] = 0x01; /* 确定性标记 */
        buffer[pos++] = (participant->autosar_startup_time_ms >> 16) & 0xFF;
        buffer[pos++] = (participant->autosar_startup_time_ms >> 8) & 0xFF;
        buffer[pos++] = participant->autosar_startup_time_ms & 0xFF;
    }
    
    return pos;
}

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

eth_status_t rtps_discovery_init(rtps_discovery_context_t *ctx, 
                                  const rtps_discovery_config_t *config)
{
    if (ctx == NULL || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memset(ctx, 0, sizeof(rtps_discovery_context_t));
    memcpy(&ctx->config, config, sizeof(rtps_discovery_config_t));
    
    /* 设置默认值 */
    if (ctx->config.initial_announce_period_ms == 0) {
        ctx->config.initial_announce_period_ms = RTPS_DISCOVERY_INITIAL_DELAY_MS;
    }
    if (ctx->config.announce_period_ms == 0) {
        ctx->config.announce_period_ms = RTPS_DISCOVERY_ANNOUNCE_PERIOD_MS;
    }
    if (ctx->config.lease_duration_ms == 0) {
        ctx->config.lease_duration_ms = RTPS_DISCOVERY_LEASE_DURATION_MS;
    }
    if (ctx->config.match_timeout_ms == 0) {
        ctx->config.match_timeout_ms = RTPS_DISCOVERY_MATCH_TIMEOUT_MS;
    }
    
    /* 车载模式优化 */
    if (ctx->config.autosar_mode) {
        ctx->config.initial_announce_period_ms = 5;  /* 更快初始发现 */
        ctx->config.announce_period_ms = 10;
        ctx->config.match_timeout_ms = 50;  /* <100ms快速启动 */
    }
    
    ctx->initialized = true;
    return ETH_OK;
}

void rtps_discovery_deinit(rtps_discovery_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* 清理参与者链表 */
    while (ctx->participants != NULL) {
        rtps_participant_proxy_t *temp = ctx->participants;
        ctx->participants = temp->next;
        free(temp);
    }
    
    /* 清理端点链表 */
    while (ctx->endpoints != NULL) {
        rtps_endpoint_proxy_t *temp = ctx->endpoints;
        ctx->endpoints = temp->next;
        free(temp);
    }
    
    ctx->active = false;
    ctx->initialized = false;
}

eth_status_t rtps_discovery_start(rtps_discovery_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return ETH_ERROR;
    }
    
    ctx->active = true;
    ctx->start_timestamp = 0; /* 调用者需要提供实际时间 */
    ctx->announcement_count = 0;
    
    return ETH_OK;
}

void rtps_discovery_stop(rtps_discovery_context_t *ctx)
{
    if (ctx != NULL) {
        ctx->active = false;
    }
}

eth_status_t rtps_discovery_process(rtps_discovery_context_t *ctx, 
                                     uint64_t current_time_ms)
{
    if (ctx == NULL || !ctx->initialized) {
        return ETH_ERROR;
    }
    
    if (!ctx->active) {
        return ETH_OK;
    }
    
    /* 初始化时间戳 */
    if (ctx->start_timestamp == 0) {
        ctx->start_timestamp = current_time_ms;
    }
    
    /* 计算经过时间 */
    uint32_t elapsed_ms = (uint32_t)(current_time_ms - ctx->start_timestamp);
    
    /* 清理过期参与者 */
    rtps_discovery_purge_expired_participants(ctx, current_time_ms);
    
    /* 清理孤立端点 */
    rtps_endpoint_cleanup_orphaned(ctx);
    
    /* 检查发现超时 */
    if (elapsed_ms > ctx->config.match_timeout_ms) {
        /* 检查是否有未匹配的本地端点 */
        rtps_endpoint_proxy_t *iter = ctx->endpoints;
        bool all_matched = true;
        
        while (iter != NULL) {
            if (iter->state != RTPS_DISCOVERY_STATE_MATCHED) {
                all_matched = false;
                ctx->discovery_failures++;
                break;
            }
            iter = iter->next;
        }
        
        /* 如果所有端点已匹配，触发完成回调 */
        if (all_matched && ctx->on_discovery_complete) {
            ctx->on_discovery_complete();
        }
    }
    
    return ETH_OK;
}

eth_status_t rtps_discovery_handle_packet(rtps_discovery_context_t *ctx,
                                           const uint8_t *data,
                                           uint32_t len,
                                           const void *source_addr)
{
    if (ctx == NULL || data == NULL || len < 20) {
        return ETH_INVALID_PARAM;
    }
    
    uint32_t pos = 0;
    
    /* 验证RTPS协议头 */
    if (data[0] != 'R' || data[1] != 'T' || 
        data[2] != 'P' || data[3] != 'S') {
        return ETH_ERROR;
    }
    pos += 4;
    
    /* 协议版本 */
    uint8_t major = data[pos++];
    uint8_t minor = data[pos++];
    (void)major;
    (void)minor;
    
    /* GUID Prefix */
    rtps_guid_prefix_t src_prefix;
    memcpy(src_prefix, &data[pos], RTPS_GUID_PREFIX_SIZE);
    pos += RTPS_GUID_PREFIX_SIZE;
    
    /* 查找已存在的参与者 */
    rtps_participant_proxy_t *participant = rtps_discovery_find_participant(ctx, src_prefix);
    
    if (participant == NULL) {
        /* 新参与者，创建代理 */
        participant = rtps_participant_create(src_prefix, data, len);
        if (participant == NULL) {
            return ETH_NO_MEMORY;
        }
        
        /* 添加到列表 */
        participant->next = ctx->participants;
        ctx->participants = participant;
        ctx->participants_discovered++;
        
        if (ctx->on_participant_discovered) {
            ctx->on_participant_discovered(participant);
        }
    }
    
    /* 更新参与者活跃性 */
    rtps_participant_update_liveliness(participant, ctx->start_timestamp);
    
    return ETH_OK;
}

eth_status_t rtps_discovery_create_participant_data(rtps_discovery_context_t *ctx,
                                                      uint8_t *buffer,
                                                      uint32_t max_len,
                                                      uint32_t *actual_len)
{
    if (ctx == NULL || buffer == NULL || actual_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 创建临时参与者代理 */
    rtps_participant_proxy_t local_participant;
    memset(&local_participant, 0, sizeof(local_participant));
    memcpy(local_participant.guid_prefix, ctx->config.guid_prefix, RTPS_GUID_PREFIX_SIZE);
    local_participant.protocol_version = (2 << 16) | 4;
    local_participant.vendor_id = RTPS_VENDOR_ID;
    local_participant.available_builtin_endpoints = 
        BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
        BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
        BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
        BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
        BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
        BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
    local_participant.lease_duration_ms = ctx->config.lease_duration_ms;
    local_participant.autosar_deterministic = ctx->config.autosar_mode;
    local_participant.autosar_startup_time_ms = ctx->config.deterministic_window_ms;
    
    /* 序列化 */
    uint32_t len = serialize_participant_data(&local_participant, buffer, max_len);
    if (len == 0) {
        return ETH_ERROR;
    }
    
    *actual_len = len;
    return ETH_OK;
}

eth_status_t rtps_discovery_add_local_endpoint(rtps_discovery_context_t *ctx,
                                                 rtps_endpoint_proxy_t *endpoint)
{
    if (ctx == NULL || endpoint == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 检查端点数量限制 */
    uint32_t count = 0;
    rtps_endpoint_proxy_t *iter = ctx->endpoints;
    while (iter != NULL) {
        count++;
        iter = iter->next;
    }
    
    if (ctx->config.max_endpoints_per_participant > 0 &&
        count >= ctx->config.max_endpoints_per_participant) {
        return ETH_ERROR;
    }
    
    /* 设置初始序列号 */
    endpoint->next_seq_number.high = 0;
    endpoint->next_seq_number.low = 1;
    endpoint->max_seq_number.high = 0;
    endpoint->max_seq_number.low = 0;
    endpoint->state = RTPS_DISCOVERY_STATE_ANNOUNCING;
    
    /* 添加到链表头部 */
    endpoint->next = ctx->endpoints;
    ctx->endpoints = endpoint;
    
    /* 尝试匹配 */
    rtps_discovery_match_endpoints(ctx, endpoint);
    
    return ETH_OK;
}

eth_status_t rtps_discovery_remove_local_endpoint(rtps_discovery_context_t *ctx,
                                                    const rtps_guid_t *guid)
{
    if (ctx == NULL || guid == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    rtps_endpoint_proxy_t **current = &ctx->endpoints;
    
    while (*current != NULL) {
        if (rtps_guid_equal(&(*current)->guid, guid)) {
            rtps_endpoint_proxy_t *to_remove = *current;
            *current = to_remove->next;
            
            if (to_remove->state == RTPS_DISCOVERY_STATE_MATCHED) {
                ctx->endpoints_matched--;
            }
            
            free(to_remove);
            return ETH_OK;
        }
        current = &(*current)->next;
    }
    
    return ETH_ERROR;
}

rtps_participant_proxy_t* rtps_discovery_find_participant(
    rtps_discovery_context_t *ctx,
    const rtps_guid_prefix_t guid_prefix)
{
    if (ctx == NULL) {
        return NULL;
    }
    
    rtps_participant_proxy_t *iter = ctx->participants;
    while (iter != NULL) {
        if (memcmp(iter->guid_prefix, guid_prefix, RTPS_GUID_PREFIX_SIZE) == 0) {
            return iter;
        }
        iter = iter->next;
    }
    
    return NULL;
}

rtps_endpoint_proxy_t* rtps_discovery_find_endpoint(
    rtps_discovery_context_t *ctx,
    const rtps_guid_t *guid)
{
    if (ctx == NULL || guid == NULL) {
        return NULL;
    }
    
    rtps_endpoint_proxy_t *iter = ctx->endpoints;
    while (iter != NULL) {
        if (rtps_guid_equal(&iter->guid, guid)) {
            return iter;
        }
        iter = iter->next;
    }
    
    return NULL;
}

bool rtps_discovery_is_complete(rtps_discovery_context_t *ctx)
{
    if (ctx == NULL || !ctx->active) {
        return false;
    }
    
    /* 检查所有本地端点是否都已匹配 */
    rtps_endpoint_proxy_t *iter = ctx->endpoints;
    while (iter != NULL) {
        if (iter->state != RTPS_DISCOVERY_STATE_MATCHED) {
            return false;
        }
        iter = iter->next;
    }
    
    return true;
}

eth_status_t rtps_discovery_get_stats(rtps_discovery_context_t *ctx,
                                        uint32_t *participants_discovered,
                                        uint32_t *endpoints_matched,
                                        uint32_t *time_elapsed_ms)
{
    if (ctx == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (participants_discovered) {
        *participants_discovered = ctx->participants_discovered;
    }
    if (endpoints_matched) {
        *endpoints_matched = ctx->endpoints_matched;
    }
    if (time_elapsed_ms) {
        *time_elapsed_ms = (uint32_t)(ctx->start_timestamp > 0 ? 
            0 : 0); /* 需要实际时间计算 */
    }
    
    return ETH_OK;
}

void rtps_discovery_set_callbacks(rtps_discovery_context_t *ctx,
                                   void (*on_participant_discovered)(rtps_participant_proxy_t *),
                                   void (*on_endpoint_matched)(rtps_endpoint_proxy_t *, bool),
                                   void (*on_discovery_complete)(void))
{
    if (ctx != NULL) {
        ctx->on_participant_discovered = on_participant_discovered;
        ctx->on_endpoint_matched = on_endpoint_matched;
        ctx->on_discovery_complete = on_discovery_complete;
    }
}

uint16_t rtps_discovery_get_port(uint32_t domain_id, 
                                  uint32_t participant_id,
                                  bool is_metatraffic)
{
    uint16_t port = RTPS_DISCOVERY_PORT_BASE +
                    (domain_id * RTPS_DISCOVERY_PORT_DOMAIN_GAIN) +
                    (participant_id * RTPS_DISCOVERY_PORT_PARTICIPANT_GAIN);
    
    if (is_metatraffic) {
        port += 0; /* 元数据单播 */
    } else {
        port += 1; /* 用户数据单播 */
    }
    
    return port;
}

eth_status_t rtps_guid_prefix_generate(rtps_guid_prefix_t prefix,
                                        const eth_mac_addr_t mac_addr,
                                        uint32_t domain_id)
{
    if (prefix == NULL || mac_addr == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 使用MAC地址和域ID生成唯一的GUID前缀 */
    prefix[0] = (domain_id >> 24) & 0xFF;
    prefix[1] = (domain_id >> 16) & 0xFF;
    prefix[2] = (domain_id >> 8) & 0xFF;
    prefix[3] = domain_id & 0xFF;
    
    prefix[4] = mac_addr[0];
    prefix[5] = mac_addr[1];
    prefix[6] = mac_addr[2];
    prefix[7] = mac_addr[3];
    prefix[8] = mac_addr[4];
    prefix[9] = mac_addr[5];
    
    /* 添加版本和随机数据 */
    prefix[10] = RTPS_PROTOCOL_VERSION_MAJOR;
    prefix[11] = RTPS_PROTOCOL_VERSION_MINOR;
    
    return ETH_OK;
}

bool rtps_guid_equal(const rtps_guid_t *guid1, const rtps_guid_t *guid2)
{
    if (guid1 == NULL || guid2 == NULL) {
        return false;
    }
    
    return (memcmp(guid1->prefix, guid2->prefix, RTPS_GUID_PREFIX_SIZE) == 0) &&
           (memcmp(guid1->entity_id, guid2->entity_id, RTPS_ENTITY_ID_SIZE) == 0);
}

int rtps_seqnum_compare(const rtps_sequence_number_t *seq1,
                         const rtps_sequence_number_t *seq2)
{
    if (seq1->high != seq2->high) {
        return (seq1->high > seq2->high) ? 1 : -1;
    }
    
    if (seq1->low == seq2->low) {
        return 0;
    }
    
    return (seq1->low > seq2->low) ? 1 : -1;
}

void rtps_seqnum_increment(rtps_sequence_number_t *seq)
{
    if (seq == NULL) {
        return;
    }
    
    seq->low++;
    if (seq->low == 0) {
        seq->high++;
    }
}

eth_status_t rtps_discovery_create_endpoint_data(rtps_discovery_context_t *ctx,
                                                   rtps_endpoint_proxy_t *endpoint,
                                                   uint8_t *buffer,
                                                   uint32_t max_len,
                                                   uint32_t *actual_len)
{
    if (ctx == NULL || endpoint == NULL || buffer == NULL || actual_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    uint32_t pos = 0;
    
    if (max_len < 256) {
        return ETH_ERROR;
    }
    
    /* 端点GUID */
    memcpy(&buffer[pos], endpoint->guid.prefix, RTPS_GUID_PREFIX_SIZE);
    pos += RTPS_GUID_PREFIX_SIZE;
    memcpy(&buffer[pos], endpoint->guid.entity_id, RTPS_ENTITY_ID_SIZE);
    pos += RTPS_ENTITY_ID_SIZE;
    
    /* 参与者GUID */
    memcpy(&buffer[pos], endpoint->participant_guid.prefix, RTPS_GUID_PREFIX_SIZE);
    pos += RTPS_GUID_PREFIX_SIZE;
    memcpy(&buffer[pos], endpoint->participant_guid.entity_id, RTPS_ENTITY_ID_SIZE);
    pos += RTPS_ENTITY_ID_SIZE;
    
    /* 主题名称 */
    uint32_t topic_len = strlen(endpoint->topic_name);
    buffer[pos++] = (topic_len >> 24) & 0xFF;
    buffer[pos++] = (topic_len >> 16) & 0xFF;
    buffer[pos++] = (topic_len >> 8) & 0xFF;
    buffer[pos++] = topic_len & 0xFF;
    memcpy(&buffer[pos], endpoint->topic_name, topic_len);
    pos += topic_len;
    /* 4字节对齐 */
    while (pos % 4 != 0) buffer[pos++] = 0;
    
    /* 类型名称 */
    uint32_t type_len = strlen(endpoint->type_name);
    buffer[pos++] = (type_len >> 24) & 0xFF;
    buffer[pos++] = (type_len >> 16) & 0xFF;
    buffer[pos++] = (type_len >> 8) & 0xFF;
    buffer[pos++] = type_len & 0xFF;
    memcpy(&buffer[pos], endpoint->type_name, type_len);
    pos += type_len;
    while (pos % 4 != 0) buffer[pos++] = 0;
    
    /* 可靠性 */
    buffer[pos++] = (endpoint->reliability_kind >> 24) & 0xFF;
    buffer[pos++] = (endpoint->reliability_kind >> 16) & 0xFF;
    buffer[pos++] = (endpoint->reliability_kind >> 8) & 0xFF;
    buffer[pos++] = endpoint->reliability_kind & 0xFF;
    
    /* 持久性 */
    buffer[pos++] = (endpoint->durability_kind >> 24) & 0xFF;
    buffer[pos++] = (endpoint->durability_kind >> 16) & 0xFF;
    buffer[pos++] = (endpoint->durability_kind >> 8) & 0xFF;
    buffer[pos++] = endpoint->durability_kind & 0xFF;
    
    *actual_len = pos;
    return ETH_OK;
}

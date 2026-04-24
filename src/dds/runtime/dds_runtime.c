/**
 * @file dds_runtime.c
 * @brief DDS运行时实现
 * @version 1.0
 * @date 2026-04-24
 */

#include "dds_runtime.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * 内部状态和常量
 * ============================================================================ */

/** 全局运行时上下文 */
typedef struct {
    bool initialized;
    bool running;
    dds_runtime_config_t config;
    dds_runtime_stats_t stats;
    
    /* 资源池 */
    dds_domain_participant_t *participants;
    uint32_t participant_count;
    
    /* RTPS层 */
    rtps_discovery_context_t discovery_ctx;
    rtps_network_context_t network_ctx;
    
    /* 时序状态 */
    uint64_t last_discovery_time;
    uint64_t last_heartbeat_time;
    uint64_t last_cleanup_time;
    uint64_t start_time;
    
    /* 错误状态 */
    int last_error;
} dds_runtime_context_t;

static dds_runtime_context_t g_runtime = {0};

/* 默认配置 */
static const dds_runtime_config_t g_default_config = {
    .max_participants = DDS_DEFAULT_MAX_PARTICIPANTS,
    .max_publishers = DDS_DEFAULT_MAX_PUBLISHERS,
    .max_subscribers = DDS_DEFAULT_MAX_SUBSCRIBERS,
    .max_topics = DDS_DEFAULT_MAX_TOPICS,
    .max_endpoints = DDS_DEFAULT_MAX_ENDPOINTS,
    .spin_period_ms = DDS_SPIN_PERIOD_MS,
    .discovery_period_ms = DDS_DISCOVERY_SPIN_PERIOD_MS,
    .heartbeat_period_ms = DDS_HEARTBEAT_PERIOD_MS,
    .cleanup_period_ms = DDS_CLEANUP_PERIOD_MS,
    .use_multicast = false,
    .use_shm = true,
    .autosar_mode = false,
    .autosar_slot_time_us = DDS_AUTOSAR_SPIN_PERIOD_US,
    .enable_deterministic = false,
    .deterministic_window_ms = 100,
    .enable_fast_startup = false,
    .startup_timeout_ms = DDS_FAST_STARTUP_TIMEOUT_MS,
};

/* ============================================================================
 * 平台抽象层（简化版本）
 * ============================================================================ */

static uint64_t platform_get_time_ms(void)
{
    /* 实际实现应使用系统时钟 */
    static uint64_t simulated_time = 0;
    simulated_time += 10;
    return simulated_time;
}

static uint64_t platform_get_time_us(void)
{
    return platform_get_time_ms() * 1000;
}

static void platform_sleep_ms(uint32_t ms)
{
    (void)ms;
    /* 实际实现应使用系统延时 */
}

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 生成新的GUID
 */
static void generate_guid(rtps_guid_t *guid, const rtps_guid_prefix_t prefix, 
                          uint32_t entity_kind, uint32_t entity_id)
{
    memcpy(guid->prefix, prefix, RTPS_GUID_PREFIX_SIZE);
    guid->entity_id[0] = (entity_kind >> 24) & 0xFF;
    guid->entity_id[1] = (entity_kind >> 16) & 0xFF;
    guid->entity_id[2] = (entity_kind >> 8) & 0xFF;
    guid->entity_id[3] = entity_id & 0xFF;
}

/**
 * @brief 找找主题
 */
static dds_topic_t* find_topic(dds_domain_participant_t *participant, 
                                const char *topic_name)
{
    dds_topic_t *topic = participant->topics;
    while (topic != NULL) {
        if (strcmp(topic->name, topic_name) == 0) {
            return topic;
        }
        topic = topic->next;
    }
    return NULL;
}

/**
 * @brief 发现回调函数
 */
static void on_participant_discovered(rtps_participant_proxy_t *participant)
{
    (void)participant;
    g_runtime.stats.discovery_count++;
}

static void on_endpoint_matched(rtps_endpoint_proxy_t *endpoint, bool matched)
{
    (void)endpoint;
    if (matched) {
        g_runtime.stats.match_count++;
    }
}

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

eth_status_t dds_runtime_init(const dds_runtime_config_t *config)
{
    if (g_runtime.initialized) {
        return ETH_OK; /* 已初始化 */
    }
    
    memset(&g_runtime, 0, sizeof(g_runtime));
    
    /* 使用配置或默认配置 */
    if (config != NULL) {
        memcpy(&g_runtime.config, config, sizeof(dds_runtime_config_t));
    } else {
        memcpy(&g_runtime.config, &g_default_config, sizeof(dds_runtime_config_t));
    }
    
    /* 初始化RTPS发现协议 */
    rtps_discovery_config_t discovery_config = {
        .type = RTPS_DISCOVERY_TYPE_SIMPLE,
        .domain_id = DDS_DEFAULT_DOMAIN_ID,
        .max_participants = g_runtime.config.max_participants,
        .max_endpoints_per_participant = g_runtime.config.max_endpoints,
        .autosar_mode = g_runtime.config.autosar_mode,
        .deterministic_window_ms = g_runtime.config.deterministic_window_ms,
        .enable_fast_startup = g_runtime.config.enable_fast_startup,
    };
    
    eth_status_t status = rtps_discovery_init(&g_runtime.discovery_ctx, &discovery_config);
    if (status != ETH_OK) {
        g_runtime.last_error = (int)status;
        return status;
    }
    
    /* 设置发现回调 */
    rtps_discovery_set_callbacks(&g_runtime.discovery_ctx,
                                  on_participant_discovered,
                                  on_endpoint_matched,
                                  NULL);
    
    /* 初始化网络状态 */
    status = rtps_network_init(&g_runtime.network_ctx);
    if (status != ETH_OK) {
        rtps_discovery_deinit(&g_runtime.discovery_ctx);
        g_runtime.last_error = (int)status;
        return status;
    }
    
    g_runtime.start_time = platform_get_time_ms();
    g_runtime.initialized = true;
    
    return ETH_OK;
}

void dds_runtime_deinit(void)
{
    if (!g_runtime.initialized) {
        return;
    }
    
    /* 停止运行 */
    g_runtime.running = false;
    
    /* 清理所有参与者 */
    while (g_runtime.participants != NULL) {
        dds_runtime_delete_participant(g_runtime.participants);
    }
    
    /* 反初始化RTPS层 */
    rtps_discovery_deinit(&g_runtime.discovery_ctx);
    rtps_network_deinit(&g_runtime.network_ctx);
    
    memset(&g_runtime, 0, sizeof(g_runtime));
}

eth_status_t dds_spin_once(uint32_t timeout_ms)
{
    (void)timeout_ms;
    
    if (!g_runtime.initialized) {
        return ETH_NOT_INIT;
    }
    
    uint64_t current_time = platform_get_time_ms();
    
    /* 发现处理 */
    if (current_time - g_runtime.last_discovery_time >= g_runtime.config.discovery_period_ms) {
        dds_runtime_schedule_discovery(current_time);
        g_runtime.last_discovery_time = current_time;
    }
    
    /* 心跳处理 */
    if (current_time - g_runtime.last_heartbeat_time >= g_runtime.config.heartbeat_period_ms) {
        dds_runtime_schedule_heartbeat(current_time);
        g_runtime.last_heartbeat_time = current_time;
    }
    
    /* 资源清理 */
    if (current_time - g_runtime.last_cleanup_time >= g_runtime.config.cleanup_period_ms) {
        dds_runtime_cleanup();
        g_runtime.last_cleanup_time = current_time;
    }
    
    /* 更新运行时间 */
    g_runtime.stats.uptime_ms = current_time - g_runtime.start_time;
    
    return ETH_OK;
}

eth_status_t dds_run(void)
{
    if (!g_runtime.initialized) {
        return ETH_NOT_INIT;
    }
    
    if (g_runtime.running) {
        return ETH_BUSY; /* 已在运行 */
    }
    
    g_runtime.running = true;
    
    /* 启动发现协议 */
    rtps_discovery_start(&g_runtime.discovery_ctx);
    
    /* 主循环 */
    while (g_runtime.running) {
        dds_spin_once(g_runtime.config.spin_period_ms);
        platform_sleep_ms(g_runtime.config.spin_period_ms);
    }
    
    return ETH_OK;
}

void dds_stop(void)
{
    g_runtime.running = false;
    rtps_discovery_stop(&g_runtime.discovery_ctx);
}

bool dds_runtime_is_running(void)
{
    return g_runtime.running;
}

eth_status_t dds_wait_for_discovery(uint32_t timeout_ms)
{
    if (!g_runtime.initialized) {
        return ETH_NOT_INIT;
    }
    
    uint64_t start_time = platform_get_time_ms();
    
    while (!rtps_discovery_is_complete(&g_runtime.discovery_ctx)) {
        uint64_t elapsed = platform_get_time_ms() - start_time;
        if (elapsed >= timeout_ms) {
            return ETH_TIMEOUT;
        }
        
        dds_spin_once(10);
        platform_sleep_ms(10);
    }
    
    return ETH_OK;
}

/* ============================================================================
 * 参与者管理实现
 * ============================================================================ */

dds_domain_participant_t* dds_runtime_create_participant(
    dds_domain_id_t domain_id,
    const dds_runtime_config_t *config)
{
    (void)config;
    
    if (!g_runtime.initialized) {
        return NULL;
    }
    
    if (g_runtime.participant_count >= g_runtime.config.max_participants) {
        g_runtime.last_error = -1;
        return NULL;
    }
    
    dds_domain_participant_t *participant = (dds_domain_participant_t *)malloc(
        sizeof(dds_domain_participant_t));
    if (participant == NULL) {
        g_runtime.last_error = -2;
        return NULL;
    }
    
    memset(participant, 0, sizeof(dds_domain_participant_t));
    
    /* 生成GUID */
    rtps_guid_prefix_t prefix;
    rtps_guid_prefix_generate(prefix, (uint8_t *)"\x00\x01\x02\x03\x04\x05", domain_id);
    generate_guid(&participant->guid, prefix, 0x000001C1, g_runtime.participant_count);
    
    participant->domain_id = domain_id;
    participant->active = true;
    participant->create_time = platform_get_time_ms();
    
    /* 默认QoS */
    participant->qos.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    participant->qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    participant->qos.deadline_ms = 0;
    participant->qos.latency_budget_ms = 0;
    participant->qos.history_depth = 1;
    
    /* 添加到全局列表 */
    participant->next = g_runtime.participants;
    g_runtime.participants = participant;
    g_runtime.participant_count++;
    g_runtime.stats.participant_count++;
    
    return participant;
}

eth_status_t dds_runtime_delete_participant(dds_domain_participant_t *participant)
{
    if (participant == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 删除所有主题 */
    while (participant->topics != NULL) {
        dds_delete_topic(participant->topics);
    }
    
    /* 删除所有发布者 */
    while (participant->publishers != NULL) {
        dds_delete_publisher(participant->publishers);
    }
    
    /* 删除所有订阅者 */
    while (participant->subscribers != NULL) {
        dds_delete_subscriber(participant->subscribers);
    }
    
    /* 从全局列表移除 */
    dds_domain_participant_t **current = &g_runtime.participants;
    while (*current != NULL) {
        if (*current == participant) {
            *current = participant->next;
            break;
        }
        current = &(*current)->next;
    }
    
    free(participant);
    g_runtime.participant_count--;
    g_runtime.stats.participant_count--;
    
    return ETH_OK;
}

dds_domain_participant_t* dds_runtime_find_participant(
    dds_domain_id_t domain_id,
    const rtps_guid_prefix_t guid_prefix)
{
    dds_domain_participant_t *p = g_runtime.participants;
    while (p != NULL) {
        if (p->domain_id == domain_id) {
            if (guid_prefix == NULL || 
                memcmp(p->guid.prefix, guid_prefix, RTPS_GUID_PREFIX_SIZE) == 0) {
                return p;
            }
        }
        p = p->next;
    }
    return NULL;
}

uint32_t dds_runtime_get_participant_count(void)
{
    return g_runtime.participant_count;
}

/* ============================================================================
 * 发布/订阅管理实现
 * ============================================================================ */

eth_status_t dds_runtime_match_endpoints(dds_data_writer_t *writer,
                                          dds_data_reader_t *reader)
{
    if (writer == NULL || reader == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 检查主题是否匹配 */
    if (writer->topic != reader->topic) {
        return ETH_ERROR;
    }
    
    /* 更新RTPS状态机 */
    eth_status_t status = rtps_writer_sm_match_reader(&writer->state_machine, 
                                                       &reader->guid);
    if (status != ETH_OK) {
        return status;
    }
    
    status = rtps_reader_sm_match_writer(&reader->state_machine, 
                                          &writer->guid);
    
    return status;
}

eth_status_t dds_runtime_unmatch_endpoints(dds_data_writer_t *writer,
                                            dds_data_reader_t *reader)
{
    if (writer == NULL || reader == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    rtps_writer_sm_unmatch_reader(&writer->state_machine, &reader->guid);
    rtps_reader_sm_unmatch_writer(&reader->state_machine, &writer->guid);
    
    return ETH_OK;
}

eth_status_t dds_runtime_handle_discovery(const uint8_t *data,
                                           uint32_t len,
                                           const void *source_addr)
{
    (void)source_addr;
    
    if (!g_runtime.initialized) {
        return ETH_NOT_INIT;
    }
    
    return rtps_discovery_handle_packet(&g_runtime.discovery_ctx, data, len, source_addr);
}

eth_status_t dds_runtime_handle_rtps_data(const uint8_t *data,
                                           uint32_t len,
                                           const void *source_addr)
{
    (void)data;
    (void)len;
    (void)source_addr;
    
    if (!g_runtime.initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 解析RTPS报文 */
    rtps_message_parser_t parser;
    eth_status_t status = rtps_message_parser_init(&parser, data, len);
    if (status != ETH_OK) {
        return status;
    }
    
    /* 处理各个Submessage */
    rtps_parsed_submessage_t submsg;
    while (rtps_message_parser_next(&parser, &submsg) == ETH_OK) {
        switch (submsg.id) {
            case RTPS_SUBMESSAGE_ID_DATA:
                /* 处理DATA */
                break;
                
            case RTPS_SUBMESSAGE_ID_HEARTBEAT:
                /* 处理HEARTBEAT */
                break;
                
            case RTPS_SUBMESSAGE_ID_ACKNACK:
                /* 处理ACKNACK */
                break;
                
            default:
                break;
        }
    }
    
    return ETH_OK;
}

/* ============================================================================
 * 资源管理实现
 * ============================================================================ */

eth_status_t dds_runtime_allocate_pools(void)
{
    /* 资源池分配实现 */
    return ETH_OK;
}

void dds_runtime_free_pools(void)
{
    /* 资源池释放实现 */
}

eth_status_t dds_runtime_get_stats(dds_runtime_stats_t *stats)
{
    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(stats, &g_runtime.stats, sizeof(dds_runtime_stats_t));
    return ETH_OK;
}

eth_status_t dds_runtime_reset_stats(void)
{
    memset(&g_runtime.stats, 0, sizeof(g_runtime.stats));
    g_runtime.stats.uptime_ms = platform_get_time_ms() - g_runtime.start_time;
    return ETH_OK;
}

eth_status_t dds_runtime_cleanup(void)
{
    /* 清理过期资源 */
    return ETH_OK;
}

/* ============================================================================
 * 时间和时序API实现
 * ============================================================================ */

uint64_t dds_get_current_time_ms(void)
{
    return platform_get_time_ms();
}

uint64_t dds_get_current_time_us(void)
{
    return platform_get_time_us();
}

void dds_sleep_ms(uint32_t ms)
{
    platform_sleep_ms(ms);
}

/* ============================================================================
 * 错误处理API实现
 * ============================================================================ */

int dds_runtime_get_last_error(void)
{
    return g_runtime.last_error;
}

const char* dds_runtime_get_error_string(int error_code)
{
    switch (error_code) {
        case 0: return "OK";
        case -1: return "ERROR";
        case -2: return "TIMEOUT";
        case -3: return "INVALID_PARAM";
        case -4: return "NO_MEMORY";
        case -5: return "BUSY";
        case -6: return "NOT_INIT";
        default: return "UNKNOWN";
    }
}

/* ============================================================================
 * 内部调度API实现
 * ============================================================================ */

eth_status_t dds_runtime_schedule_discovery(uint64_t current_time_ms)
{
    return rtps_discovery_process(&g_runtime.discovery_ctx, current_time_ms);
}

eth_status_t dds_runtime_schedule_heartbeat(uint64_t current_time_ms)
{
    (void)current_time_ms;
    
    /* 遍历所有Writer处理心跳 */
    dds_domain_participant_t *p = g_runtime.participants;
    while (p != NULL) {
        dds_publisher_t *pub = p->publishers;
        while (pub != NULL) {
            dds_data_writer_t *writer = pub->writers;
            while (writer != NULL) {
                bool need_heartbeat = false;
                rtps_writer_sm_process(&writer->state_machine, 
                                        current_time_ms, 
                                        &need_heartbeat);
                writer = writer->next;
            }
            pub = pub->next;
        }
        p = p->next;
    }
    
    /* 遍历所有Reader处理ACKNACK */
    p = g_runtime.participants;
    while (p != NULL) {
        dds_subscriber_t *sub = p->subscribers;
        while (sub != NULL) {
            dds_data_reader_t *reader = sub->readers;
            while (reader != NULL) {
                bool need_acknack = false;
                rtps_reader_sm_process(&reader->state_machine, 
                                        current_time_ms, 
                                        &need_acknack);
                reader = reader->next;
            }
            sub = sub->next;
        }
        p = p->next;
    }
    
    return ETH_OK;
}

eth_status_t dds_runtime_schedule_send(uint64_t current_time_ms)
{
    (void)current_time_ms;
    /* 数据发送调度实现 */
    return ETH_OK;
}

eth_status_t dds_runtime_schedule_receive(uint64_t current_time_ms)
{
    (void)current_time_ms;
    /* 数据接收调度实现 */
    return ETH_OK;
}

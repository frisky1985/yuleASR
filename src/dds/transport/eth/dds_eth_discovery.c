/**
 * @file dds_eth_discovery.c
 * @brief DDS发现协议实现 - SPDP & SEDP
 * @version 1.0
 * @date 2026-04-26
 */

#include <string.h>
#include <stdio.h>
#include "dds_eth_discovery.h"

/* ============================================================================
 * 私有定义
 * ============================================================================ */

#define DDS_DISCOVERY_MAGIC_NUMBER      0x44445344U  /* "DDSD" */
#define DDS_MAX_DISCOVERED_PARTICIPANTS 16
#define DDS_MAX_DISCOVERED_PUBLICATIONS 32
#define DDS_MAX_DISCOVERED_SUBSCRIPTIONS 32

/* 内建Endpoints位图 */
#define DDS_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER  (1U << 0)
#define DDS_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR   (1U << 1)
#define DDS_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER  (1U << 2)
#define DDS_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR   (1U << 3)
#define DDS_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER (1U << 4)
#define DDS_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR  (1U << 5)

/* 发现数据缓存 */
typedef struct {
    uint32_t magic;
    bool initialized;
    bool running;
    dds_discovery_config_t config;
    
    /* 发现状态 */
    uint32_t last_spdp_announce_time;
    uint32_t last_sedp_announce_time;
    uint32_t spdp_announce_count;
    
    /* 发现回调 */
    dds_spdp_participant_discovered_cb_t spdp_callback;
    void *spdp_user_data;
    dds_sedp_publication_discovered_cb_t sedp_pub_callback;
    void *sedp_pub_user_data;
    dds_sedp_subscription_discovered_cb_t sedp_sub_callback;
    void *sedp_sub_user_data;
    
    /* 发现数据缓存 */
    dds_discovered_participant_t discovered_participants[DDS_MAX_DISCOVERED_PARTICIPANTS];
    dds_discovered_publication_t discovered_publications[DDS_MAX_DISCOVERED_PUBLICATIONS];
    dds_discovered_subscription_t discovered_subscriptions[DDS_MAX_DISCOVERED_SUBSCRIPTIONS];
} dds_discovery_ctx_t;

static dds_discovery_ctx_t g_discovery = {0};

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static int32_t dds_find_free_participant_slot(void);
static int32_t dds_find_participant_by_guid(const dds_entity_guid_t *guid);
static int32_t dds_find_free_publication_slot(void);
static int32_t dds_find_publication_by_guid(const dds_entity_guid_t *guid);
static int32_t dds_find_free_subscription_slot(void);
static int32_t dds_find_subscription_by_guid(const dds_entity_guid_t *guid);
static bool dds_guid_equal(const dds_entity_guid_t *a, const dds_entity_guid_t *b);
static void dds_build_spdp_data(const dds_participant_proxy_t *proxy,
                                 uint8_t *buffer, uint32_t *len);
static eth_status_t dds_parse_spdp_data(const uint8_t *data, uint32_t len,
                                         dds_participant_proxy_t *proxy);

/* ============================================================================
 * 初始化和反初始化API实现
 * ============================================================================ */

eth_status_t dds_discovery_init(const dds_discovery_config_t *config)
{
    eth_status_t status = ETH_OK;
    
    if (config == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (g_discovery.initialized) {
        status = ETH_BUSY;
    }
    else {
        /* 复制配置 */
        (void)memcpy(&g_discovery.config, config, sizeof(dds_discovery_config_t));
        
        /* 初始化状态 */
        g_discovery.magic = DDS_DISCOVERY_MAGIC_NUMBER;
        g_discovery.initialized = false;
        g_discovery.running = false;
        g_discovery.last_spdp_announce_time = 0;
        g_discovery.last_sedp_announce_time = 0;
        g_discovery.spdp_announce_count = 0;
        
        /* 清零回调 */
        g_discovery.spdp_callback = NULL;
        g_discovery.spdp_user_data = NULL;
        g_discovery.sedp_pub_callback = NULL;
        g_discovery.sedp_pub_user_data = NULL;
        g_discovery.sedp_sub_callback = NULL;
        g_discovery.sedp_sub_user_data = NULL;
        
        /* 清零缓存 */
        (void)memset(g_discovery.discovered_participants, 0, 
                    sizeof(g_discovery.discovered_participants));
        (void)memset(g_discovery.discovered_publications, 0,
                    sizeof(g_discovery.discovered_publications));
        (void)memset(g_discovery.discovered_subscriptions, 0,
                    sizeof(g_discovery.discovered_subscriptions));
        
        /* 验证配置 */
        if (config->spdp_period_ms < DDS_DISCOVERY_MIN_PERIOD_MS) {
            g_discovery.config.spdp_period_ms = DDS_DISCOVERY_DEFAULT_PERIOD_MS;
        }
        if (config->spdp_period_ms > DDS_DISCOVERY_MAX_PERIOD_MS) {
            g_discovery.config.spdp_period_ms = DDS_DISCOVERY_MAX_PERIOD_MS;
        }
        
        g_discovery.initialized = true;
    }
    
    return status;
}

void dds_discovery_deinit(void)
{
    if (g_discovery.initialized) {
        (void)dds_discovery_stop();
        (void)memset(&g_discovery, 0, sizeof(g_discovery));
    }
}

eth_status_t dds_discovery_start(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (g_discovery.running) {
        status = ETH_BUSY;
    }
    else {
        g_discovery.running = true;
        
        /* 立即广播一次SPDP */
        if (g_discovery.config.enable_spdp) {
            (void)dds_spdp_announce_participant();
        }
    }
    
    return status;
}

eth_status_t dds_discovery_stop(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_discovery.running = false;
    }
    
    return status;
}

eth_status_t dds_discovery_main_function(uint32_t elapsed_ms)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (!g_discovery.running) {
        status = ETH_ERROR;
    }
    else {
        /* 更新时间计数器 */
        g_discovery.last_spdp_announce_time += elapsed_ms;
        g_discovery.last_sedp_announce_time += elapsed_ms;
        
        /* 检查SPDP定时广播 */
        if (g_discovery.config.enable_spdp) {
            if (g_discovery.last_spdp_announce_time >= g_discovery.config.spdp_period_ms) {
                g_discovery.last_spdp_announce_time = 0;
                (void)dds_spdp_announce_participant();
            }
        }
        
        /* 检查SEDP定时重发 */
        if (g_discovery.config.enable_sedp) {
            if (g_discovery.last_sedp_announce_time >= g_discovery.config.sedp_resend_period_ms) {
                g_discovery.last_sedp_announce_time = 0;
                /* 重发所有本地Endpoint */
                /* 注: 实际实现需要遍历本地Endpoints并广播 */
            }
        }
        
        /* 清理超时Participant */
        /* 注: 需要传入当前绝对时间 */
    }
    
    return status;
}

/* ============================================================================
 * SPDP API实现
 * ============================================================================ */

eth_status_t dds_spdp_announce_participant(void)
{
    eth_status_t status = ETH_OK;
    dds_participant_proxy_t proxy;
    uint8_t buffer[512];
    uint32_t len = 0;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 构建Participant代理 */
        status = dds_spdp_get_local_participant(&proxy);
        
        if (status == ETH_OK) {
            /* 序列化SPDP数据 */
            dds_build_spdp_data(&proxy, buffer, &len);
            
            /* 通过多播发送SPDP数据 */
            /* 注: 需要通过dds_eth_transport发送 */
            
            g_discovery.spdp_announce_count++;
        }
    }
    
    return status;
}

eth_status_t dds_spdp_handle_data(const uint8_t *data, uint32_t len,
                                   const dds_locator_t *locator)
{
    eth_status_t status = ETH_OK;
    dds_participant_proxy_t proxy;
    int32_t idx;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((data == NULL) || (locator == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 解析SPDP数据 */
        status = dds_parse_spdp_data(data, len, &proxy);
        
        if (status == ETH_OK) {
            /* 检查是否为自己 */
            if (!dds_guid_equal(&proxy.participant_guid, 
                               &g_discovery.config.participant_guid)) {
                /* 查找是否已存在 */
                idx = dds_find_participant_by_guid(&proxy.participant_guid);
                
                if (idx >= 0) {
                    /* 更新现有Participant */
                    (void)memcpy(&g_discovery.discovered_participants[idx].participant,
                                &proxy, sizeof(dds_participant_proxy_t));
                    g_discovery.discovered_participants[idx].last_update_ms = 0;
                }
                else {
                    /* 添加新Participant */
                    idx = dds_find_free_participant_slot();
                    if (idx >= 0) {
                        (void)memcpy(&g_discovery.discovered_participants[idx].participant,
                                    &proxy, sizeof(dds_participant_proxy_t));
                        g_discovery.discovered_participants[idx].valid = true;
                        g_discovery.discovered_participants[idx].discovery_time_ms = 0;
                        g_discovery.discovered_participants[idx].last_update_ms = 0;
                        
                        /* 通知回调 */
                        if (g_discovery.spdp_callback != NULL) {
                            g_discovery.spdp_callback(&proxy, true, 
                                                     g_discovery.spdp_user_data);
                        }
                    }
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_spdp_get_local_participant(dds_participant_proxy_t *proxy)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (proxy == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 填充Participant代理数据 */
        (void)memset(proxy, 0, sizeof(dds_participant_proxy_t));
        
        /* Participant GUID */
        (void)memcpy(&proxy->participant_guid, &g_discovery.config.participant_guid,
                    sizeof(dds_entity_guid_t));
        
        /* 内建Endpoints */
        proxy->available_builtin_endpoints = 
            DDS_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
            DDS_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;
        
        if (g_discovery.config.enable_sedp) {
            proxy->available_builtin_endpoints |= 
                DDS_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
                DDS_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
                DDS_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
                DDS_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
        }
        
        /* 定位器 */
        (void)memcpy(&proxy->metatraffic_unicast_locator,
                    &g_discovery.config.metatraffic_unicast_locator,
                    sizeof(dds_locator_t));
        (void)memcpy(&proxy->metatraffic_multicast_locator,
                    &g_discovery.config.metatraffic_multicast_locator,
                    sizeof(dds_locator_t));
        (void)memcpy(&proxy->default_unicast_locator,
                    &g_discovery.config.default_unicast_locator,
                    sizeof(dds_locator_t));
        (void)memcpy(&proxy->default_multicast_locator,
                    &g_discovery.config.default_multicast_locator,
                    sizeof(dds_locator_t));
        
        /* 域标签 */
        (void)strncpy(proxy->domain_tag, g_discovery.config.domain_tag,
                     DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1);
        proxy->domain_tag[DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1] = '\0';
    }
    
    return status;
}

eth_status_t dds_spdp_register_callback(dds_spdp_participant_discovered_cb_t callback,
                                         void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_discovery.spdp_callback = callback;
        g_discovery.spdp_user_data = user_data;
    }
    
    return status;
}

/* ============================================================================
 * SEDP API实现
 * ============================================================================ */

eth_status_t dds_sedp_announce_publication(
    const dds_publication_builtin_topic_data_t *pub_data)
{
    eth_status_t status = ETH_OK;
    uint8_t buffer[512];
    uint32_t len = 0;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (pub_data == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_discovery.config.enable_sedp) {
        status = ETH_ERROR;
    }
    else {
        /* 序列化Publication数据 */
        status = dds_discovery_serialize_publication(pub_data, buffer, 512, &len);
        
        if (status == ETH_OK) {
            /* 通过SEDP广播 */
            /* 注: 需要通过dds_eth_transport发送到元数据多播组 */
        }
    }
    
    return status;
}

eth_status_t dds_sedp_announce_subscription(
    const dds_subscription_builtin_topic_data_t *sub_data)
{
    eth_status_t status = ETH_OK;
    uint8_t buffer[512];
    uint32_t len = 0;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (sub_data == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_discovery.config.enable_sedp) {
        status = ETH_ERROR;
    }
    else {
        /* 序列化Subscription数据 */
        status = dds_discovery_serialize_subscription(sub_data, buffer, 512, &len);
        
        if (status == ETH_OK) {
            /* 通过SEDP广播 */
        }
    }
    
    return status;
}

eth_status_t dds_sedp_handle_publication(const uint8_t *data, uint32_t len)
{
    eth_status_t status = ETH_OK;
    dds_publication_builtin_topic_data_t pub;
    int32_t idx;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (data == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 解析Publication数据 */
        status = dds_discovery_deserialize_publication(data, len, &pub);
        
        if (status == ETH_OK) {
            /* 查找是否已存在 */
            idx = dds_find_publication_by_guid(&pub.publication_guid);
            
            if (idx < 0) {
                /* 添加新Publication */
                idx = dds_find_free_publication_slot();
                if (idx >= 0) {
                    (void)memcpy(&g_discovery.discovered_publications[idx].publication,
                                &pub, sizeof(dds_publication_builtin_topic_data_t));
                    g_discovery.discovered_publications[idx].valid = true;
                    g_discovery.discovered_publications[idx].discovery_time_ms = 0;
                    
                    /* 通知回调 */
                    if (g_discovery.sedp_pub_callback != NULL) {
                        g_discovery.sedp_pub_callback(&pub, true,
                                                     g_discovery.sedp_pub_user_data);
                    }
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_sedp_handle_subscription(const uint8_t *data, uint32_t len)
{
    eth_status_t status = ETH_OK;
    dds_subscription_builtin_topic_data_t sub;
    int32_t idx;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (data == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 解析Subscription数据 */
        status = dds_discovery_deserialize_subscription(data, len, &sub);
        
        if (status == ETH_OK) {
            idx = dds_find_subscription_by_guid(&sub.subscription_guid);
            
            if (idx < 0) {
                idx = dds_find_free_subscription_slot();
                if (idx >= 0) {
                    (void)memcpy(&g_discovery.discovered_subscriptions[idx].subscription,
                                &sub, sizeof(dds_subscription_builtin_topic_data_t));
                    g_discovery.discovered_subscriptions[idx].valid = true;
                    g_discovery.discovered_subscriptions[idx].discovery_time_ms = 0;
                    
                    if (g_discovery.sedp_sub_callback != NULL) {
                        g_discovery.sedp_sub_callback(&sub, true,
                                                      g_discovery.sedp_sub_user_data);
                    }
                }
            }
        }
    }
    
    return status;
}

eth_status_t dds_sedp_register_publication_callback(
    dds_sedp_publication_discovered_cb_t callback, void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_discovery.sedp_pub_callback = callback;
        g_discovery.sedp_pub_user_data = user_data;
    }
    
    return status;
}

eth_status_t dds_sedp_register_subscription_callback(
    dds_sedp_subscription_discovered_cb_t callback, void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_discovery.sedp_sub_callback = callback;
        g_discovery.sedp_sub_user_data = user_data;
    }
    
    return status;
}

/* ============================================================================
 * 发现数据管理API实现
 * ============================================================================ */

eth_status_t dds_discovery_get_participant(const dds_entity_guid_t *guid,
                                            dds_participant_proxy_t *participant)
{
    eth_status_t status = ETH_ERROR;
    int32_t idx;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((guid == NULL) || (participant == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_find_participant_by_guid(guid);
        if (idx >= 0) {
            (void)memcpy(participant,
                        &g_discovery.discovered_participants[idx].participant,
                        sizeof(dds_participant_proxy_t));
            status = ETH_OK;
        }
    }
    
    return status;
}

eth_status_t dds_discovery_get_publication(const dds_entity_guid_t *guid,
                                            dds_publication_builtin_topic_data_t *publication)
{
    eth_status_t status = ETH_ERROR;
    int32_t idx;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((guid == NULL) || (publication == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_find_publication_by_guid(guid);
        if (idx >= 0) {
            (void)memcpy(publication,
                        &g_discovery.discovered_publications[idx].publication,
                        sizeof(dds_publication_builtin_topic_data_t));
            status = ETH_OK;
        }
    }
    
    return status;
}

eth_status_t dds_discovery_get_subscription(const dds_entity_guid_t *guid,
                                             dds_subscription_builtin_topic_data_t *subscription)
{
    eth_status_t status = ETH_ERROR;
    int32_t idx;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((guid == NULL) || (subscription == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        idx = dds_find_subscription_by_guid(guid);
        if (idx >= 0) {
            (void)memcpy(subscription,
                        &g_discovery.discovered_subscriptions[idx].subscription,
                        sizeof(dds_subscription_builtin_topic_data_t));
            status = ETH_OK;
        }
    }
    
    return status;
}

eth_status_t dds_discovery_get_all_participants(dds_participant_proxy_t *participants,
                                                 uint32_t max_count,
                                                 uint32_t *actual_count)
{
    eth_status_t status = ETH_OK;
    uint32_t count = 0;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((participants == NULL) || (actual_count == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        for (uint32_t i = 0; (i < DDS_MAX_DISCOVERED_PARTICIPANTS) && (count < max_count); i++) {
            if (g_discovery.discovered_participants[i].valid) {
                (void)memcpy(&participants[count],
                            &g_discovery.discovered_participants[i].participant,
                            sizeof(dds_participant_proxy_t));
                count++;
            }
        }
        *actual_count = count;
    }
    
    return status;
}

eth_status_t dds_discovery_cleanup_stale_participants(uint32_t current_time_ms)
{
    eth_status_t status = ETH_OK;
    
    if (!g_discovery.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_PARTICIPANTS; i++) {
            if (g_discovery.discovered_participants[i].valid) {
                if ((current_time_ms - g_discovery.discovered_participants[i].last_update_ms) >
                    DDS_DISCOVERY_PARTICIPANT_CLEANUP_MS) {
                    /* Participant超时 */
                    if (g_discovery.spdp_callback != NULL) {
                        g_discovery.spdp_callback(
                            &g_discovery.discovered_participants[i].participant,
                            false, g_discovery.spdp_user_data);
                    }
                    g_discovery.discovered_participants[i].valid = false;
                }
            }
        }
    }
    
    return status;
}

/* ============================================================================
 * 发现数据序列化API实现
 * ============================================================================ */

eth_status_t dds_discovery_serialize_participant(
    const dds_participant_proxy_t *proxy,
    uint8_t *buffer, uint32_t buf_size, uint32_t *actual_len)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((proxy == NULL) || (buffer == NULL) || (actual_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 序列化Participant GUID */
        if ((offset + sizeof(dds_entity_guid_t)) <= buf_size) {
            (void)memcpy(&buffer[offset], &proxy->participant_guid, sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        /* 序列化内建Endpoints */
        if ((offset + 4) <= buf_size) {
            buffer[offset++] = (uint8_t)((proxy->available_builtin_endpoints >> 24) & 0xFF);
            buffer[offset++] = (uint8_t)((proxy->available_builtin_endpoints >> 16) & 0xFF);
            buffer[offset++] = (uint8_t)((proxy->available_builtin_endpoints >> 8) & 0xFF);
            buffer[offset++] = (uint8_t)(proxy->available_builtin_endpoints & 0xFF);
        }
        
        /* 序列化定位器 */
        /* 注: 简化版本，实际需要完整的CDR编码 */
        
        /* 序列化域标签 */
        uint32_t tag_len = (uint32_t)strlen(proxy->domain_tag);
        if ((offset + tag_len + 1) <= buf_size) {
            (void)strcpy((char*)&buffer[offset], proxy->domain_tag);
            offset += tag_len + 1;
        }
        
        *actual_len = offset;
    }
    
    return status;
}

eth_status_t dds_discovery_deserialize_participant(
    const uint8_t *data, uint32_t len,
    dds_participant_proxy_t *proxy)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((data == NULL) || (proxy == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        (void)memset(proxy, 0, sizeof(dds_participant_proxy_t));
        
        /* 反序列化Participant GUID */
        if ((offset + sizeof(dds_entity_guid_t)) <= len) {
            (void)memcpy(&proxy->participant_guid, &data[offset], sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        /* 反序列化内建Endpoints */
        if ((offset + 4) <= len) {
            proxy->available_builtin_endpoints = 
                ((uint32_t)data[offset] << 24) |
                ((uint32_t)data[offset + 1] << 16) |
                ((uint32_t)data[offset + 2] << 8) |
                (uint32_t)data[offset + 3];
            offset += 4;
        }
        
        /* 反序列化域标签 */
        if (offset < len) {
            uint32_t remaining = len - offset;
            uint32_t copy_len = remaining < (DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1) ?
                                remaining : (DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1);
            (void)memcpy(proxy->domain_tag, &data[offset], copy_len);
            proxy->domain_tag[copy_len] = '\0';
        }
    }
    
    return status;
}

eth_status_t dds_discovery_serialize_publication(
    const dds_publication_builtin_topic_data_t *pub,
    uint8_t *buffer, uint32_t buf_size, uint32_t *actual_len)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((pub == NULL) || (buffer == NULL) || (actual_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 序列化Participant GUID */
        if ((offset + sizeof(dds_entity_guid_t)) <= buf_size) {
            (void)memcpy(&buffer[offset], &pub->participant_guid, sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        /* 序列化Publication GUID */
        if ((offset + sizeof(dds_entity_guid_t)) <= buf_size) {
            (void)memcpy(&buffer[offset], &pub->publication_guid, sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        /* 序列化主题名 */
        uint32_t topic_len = (uint32_t)strlen(pub->topic_name);
        if ((offset + topic_len + 1) <= buf_size) {
            (void)strcpy((char*)&buffer[offset], pub->topic_name);
            offset += topic_len + 1;
        }
        
        /* 序列化类型名 */
        uint32_t type_len = (uint32_t)strlen(pub->type_name);
        if ((offset + type_len + 1) <= buf_size) {
            (void)strcpy((char*)&buffer[offset], pub->type_name);
            offset += type_len + 1;
        }
        
        *actual_len = offset;
    }
    
    return status;
}

eth_status_t dds_discovery_deserialize_publication(
    const uint8_t *data, uint32_t len,
    dds_publication_builtin_topic_data_t *pub)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((data == NULL) || (pub == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        (void)memset(pub, 0, sizeof(dds_publication_builtin_topic_data_t));
        
        /* 反序列化Participant GUID */
        if ((offset + sizeof(dds_entity_guid_t)) <= len) {
            (void)memcpy(&pub->participant_guid, &data[offset], sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        /* 反序列化Publication GUID */
        if ((offset + sizeof(dds_entity_guid_t)) <= len) {
            (void)memcpy(&pub->publication_guid, &data[offset], sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        /* 反序列化主题名和类型名 */
        if (offset < len) {
            uint32_t remaining = len - offset;
            uint32_t copy_len = remaining < (DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1) ?
                                remaining : (DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1);
            (void)memcpy(pub->topic_name, &data[offset], copy_len);
            pub->topic_name[copy_len] = '\0';
            /* 注: 简化版本，实际需要解析长度字段 */
        }
    }
    
    return status;
}

eth_status_t dds_discovery_serialize_subscription(
    const dds_subscription_builtin_topic_data_t *sub,
    uint8_t *buffer, uint32_t buf_size, uint32_t *actual_len)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((sub == NULL) || (buffer == NULL) || (actual_len == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 类似Publication的序列化 */
        if ((offset + sizeof(dds_entity_guid_t)) <= buf_size) {
            (void)memcpy(&buffer[offset], &sub->participant_guid, sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        if ((offset + sizeof(dds_entity_guid_t)) <= buf_size) {
            (void)memcpy(&buffer[offset], &sub->subscription_guid, sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        uint32_t topic_len = (uint32_t)strlen(sub->topic_name);
        if ((offset + topic_len + 1) <= buf_size) {
            (void)strcpy((char*)&buffer[offset], sub->topic_name);
            offset += topic_len + 1;
        }
        
        *actual_len = offset;
    }
    
    return status;
}

eth_status_t dds_discovery_deserialize_subscription(
    const uint8_t *data, uint32_t len,
    dds_subscription_builtin_topic_data_t *sub)
{
    eth_status_t status = ETH_OK;
    uint32_t offset = 0;
    
    if ((data == NULL) || (sub == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        (void)memset(sub, 0, sizeof(dds_subscription_builtin_topic_data_t));
        
        if ((offset + sizeof(dds_entity_guid_t)) <= len) {
            (void)memcpy(&sub->participant_guid, &data[offset], sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        if ((offset + sizeof(dds_entity_guid_t)) <= len) {
            (void)memcpy(&sub->subscription_guid, &data[offset], sizeof(dds_entity_guid_t));
            offset += sizeof(dds_entity_guid_t);
        }
        
        if (offset < len) {
            uint32_t remaining = len - offset;
            uint32_t copy_len = remaining < (DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1) ?
                                remaining : (DDS_DISCOVERY_DOMAIN_TAG_MAX_LEN - 1);
            (void)memcpy(sub->topic_name, &data[offset], copy_len);
            sub->topic_name[copy_len] = '\0';
        }
    }
    
    return status;
}

/* ============================================================================
 * 内部函数实现
 * ============================================================================ */

static int32_t dds_find_free_participant_slot(void)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_PARTICIPANTS; i++) {
        if (!g_discovery.discovered_participants[i].valid) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_find_participant_by_guid(const dds_entity_guid_t *guid)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_PARTICIPANTS; i++) {
        if (g_discovery.discovered_participants[i].valid) {
            if (dds_guid_equal(&g_discovery.discovered_participants[i].participant.participant_guid, 
                              guid)) {
                idx = (int32_t)i;
                break;
            }
        }
    }
    
    return idx;
}

static int32_t dds_find_free_publication_slot(void)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_PUBLICATIONS; i++) {
        if (!g_discovery.discovered_publications[i].valid) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_find_publication_by_guid(const dds_entity_guid_t *guid)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_PUBLICATIONS; i++) {
        if (g_discovery.discovered_publications[i].valid) {
            if (dds_guid_equal(&g_discovery.discovered_publications[i].publication.publication_guid,
                              guid)) {
                idx = (int32_t)i;
                break;
            }
        }
    }
    
    return idx;
}

static int32_t dds_find_free_subscription_slot(void)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_SUBSCRIPTIONS; i++) {
        if (!g_discovery.discovered_subscriptions[i].valid) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_find_subscription_by_guid(const dds_entity_guid_t *guid)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_MAX_DISCOVERED_SUBSCRIPTIONS; i++) {
        if (g_discovery.discovered_subscriptions[i].valid) {
            if (dds_guid_equal(&g_discovery.discovered_subscriptions[i].subscription.subscription_guid,
                              guid)) {
                idx = (int32_t)i;
                break;
            }
        }
    }
    
    return idx;
}

static bool dds_guid_equal(const dds_entity_guid_t *a, const dds_entity_guid_t *b)
{
    return (memcmp(a->guid_prefix, b->guid_prefix, 12) == 0) &&
           (memcmp(&a->entity_id, &b->entity_id, sizeof(dds_entity_id_t)) == 0);
}

static void dds_build_spdp_data(const dds_participant_proxy_t *proxy,
                                 uint8_t *buffer, uint32_t *len)
{
    /* 实际实现需要使用CDR序列化 */
    (void)dds_discovery_serialize_participant(proxy, buffer, 512, len);
}

static eth_status_t dds_parse_spdp_data(const uint8_t *data, uint32_t len,
                                         dds_participant_proxy_t *proxy)
{
    return dds_discovery_deserialize_participant(data, len, proxy);
}

/**
 * @file dds_tsn_transport.c
 * @brief DDS TSN传输层实现
 * @version 1.0
 * @date 2026-04-26
 */

#include <string.h>
#include <stdio.h>
#include "dds_tsn_transport.h"

/* ============================================================================
 * 私有定义
 * ============================================================================ */

typedef struct {
    bool used;
    dds_tsn_stream_config_t config;
    dds_tsn_stream_state_t state;
    
    union {
        dds_tsn_talker_status_t talker;
        dds_tsn_listener_status_t listener;
    } status;
    
    /* 回调 */
    dds_tsn_data_rx_cb_t rx_callback;
    void *rx_user_data;
    
    /* 统计 */
    uint64_t frames_count;
    uint64_t bytes_count;
    uint32_t avg_latency_ns;
    uint32_t max_latency_ns;
    
    /* TSN特定 */
    uint32_t stream_reservation_handle;
    bool reserved;
} dds_tsn_stream_entry_t;

typedef struct {
    bool initialized;
    bool running;
    eth_mac_addr_t local_mac;
    uint16_t default_vlan_id;
    
    dds_tsn_stream_entry_t streams[DDS_TSN_MAX_STREAMS];
    uint32_t stream_count;
    
    /* 回调 */
    dds_tsn_stream_state_cb_t state_callback;
    void *state_user_data;
    dds_tsn_latency_violation_cb_t latency_callback;
    void *latency_user_data;
    
    /* 统计 */
    dds_tsn_transport_stats_t stats;
} dds_tsn_context_t;

static dds_tsn_context_t g_tsn_ctx = {0};

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static int32_t dds_tsn_find_free_stream_slot(void);
static int32_t dds_tsn_find_stream_by_id(const dds_tsn_stream_id_t *stream_id);
static void dds_tsn_update_stream_state(dds_tsn_stream_entry_t *stream,
                                         dds_tsn_stream_state_t new_state);
static eth_status_t dds_tsn_setup_tsn_features(dds_tsn_stream_entry_t *stream);
static eth_status_t dds_tsn_teardown_tsn_features(dds_tsn_stream_entry_t *stream);

/* ============================================================================
 * 初始化和反初始化API实现
 * ============================================================================ */

eth_status_t dds_tsn_transport_init(const eth_mac_addr_t local_mac, uint16_t vlan_id)
{
    eth_status_t status = ETH_OK;
    
    if (local_mac == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else if (g_tsn_ctx.initialized) {
        status = ETH_BUSY;
    }
    else {
        /* 保存本地MAC地址 */
        (void)memcpy(g_tsn_ctx.local_mac, local_mac, sizeof(eth_mac_addr_t));
        g_tsn_ctx.default_vlan_id = vlan_id;
        
        /* 清零Stream表 */
        (void)memset(g_tsn_ctx.streams, 0, sizeof(g_tsn_ctx.streams));
        g_tsn_ctx.stream_count = 0;
        
        /* 清零回调 */
        g_tsn_ctx.state_callback = NULL;
        g_tsn_ctx.state_user_data = NULL;
        g_tsn_ctx.latency_callback = NULL;
        g_tsn_ctx.latency_user_data = NULL;
        
        /* 清零统计 */
        (void)memset(&g_tsn_ctx.stats, 0, sizeof(g_tsn_ctx.stats));
        
        /* 初始化TSN协议栈 */
        /* 注: 调用现有的TSN初始化函数 */
        
        g_tsn_ctx.initialized = true;
        g_tsn_ctx.running = false;
    }
    
    return status;
}

void dds_tsn_transport_deinit(void)
{
    if (g_tsn_ctx.initialized) {
        /* 停止所有Stream */
        (void)dds_tsn_transport_stop();
        
        /* 删除所有Stream */
        for (uint32_t i = 0; i < DDS_TSN_MAX_STREAMS; i++) {
            if (g_tsn_ctx.streams[i].used) {
                (void)dds_tsn_delete_stream(i);
            }
        }
        
        /* 反初始化TSN协议栈 */
        /* 注: 调用现有的TSN反初始化函数 */
        
        /* 清零上下文 */
        (void)memset(&g_tsn_ctx, 0, sizeof(g_tsn_ctx));
    }
}

eth_status_t dds_tsn_transport_start(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (g_tsn_ctx.running) {
        status = ETH_BUSY;
    }
    else {
        /* 启动所有已配置的Stream */
        for (uint32_t i = 0; i < DDS_TSN_MAX_STREAMS; i++) {
            if (g_tsn_ctx.streams[i].used) {
                dds_tsn_update_stream_state(&g_tsn_ctx.streams[i],
                                             DDS_TSN_STREAM_STATE_RUNNING);
            }
        }
        
        g_tsn_ctx.running = true;
    }
    
    return status;
}

eth_status_t dds_tsn_transport_stop(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 停止所有Stream */
        for (uint32_t i = 0; i < DDS_TSN_MAX_STREAMS; i++) {
            if (g_tsn_ctx.streams[i].used) {
                dds_tsn_update_stream_state(&g_tsn_ctx.streams[i],
                                             DDS_TSN_STREAM_STATE_READY);
            }
        }
        
        g_tsn_ctx.running = false;
    }
    
    return status;
}

eth_status_t dds_tsn_transport_main_function(uint32_t elapsed_ms)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        /* 更新每个Stream的状态和统计 */
        for (uint32_t i = 0; i < DDS_TSN_MAX_STREAMS; i++) {
            if (g_tsn_ctx.streams[i].used) {
                /* 检查Stream状态 */
                if (g_tsn_ctx.streams[i].state == DDS_TSN_STREAM_STATE_RUNNING) {
                    /* 检查是否需要重新预留 */
                    if (!g_tsn_ctx.streams[i].reserved) {
                        /* 尝试重新预留 */
                        (void)dds_tsn_reserve_stream(i);
                    }
                }
            }
        }
        
        (void)elapsed_ms;
    }
    
    return status;
}

/* ============================================================================
 * Stream管理API实现
 * ============================================================================ */

eth_status_t dds_tsn_create_stream(const dds_tsn_stream_config_t *config,
                                    uint32_t *stream_handle)
{
    eth_status_t status = ETH_OK;
    int32_t idx;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((config == NULL) || (stream_handle == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 查找是否已存在相同Stream ID */
        idx = dds_tsn_find_stream_by_id(&config->id.stream_id);
        
        if (idx >= 0) {
            /* 返回现有Stream */
            *stream_handle = (uint32_t)idx;
        }
        else {
            /* 创建新Stream */
            idx = dds_tsn_find_free_stream_slot();
            if (idx < 0) {
                status = ETH_NO_MEMORY;
            }
            else {
                dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[idx];
                
                entry->used = true;
                (void)memcpy(&entry->config, config, sizeof(dds_tsn_stream_config_t));
                entry->state = DDS_TSN_STREAM_STATE_IDLE;
                entry->reserved = false;
                entry->frames_count = 0;
                entry->bytes_count = 0;
                entry->avg_latency_ns = 0;
                entry->max_latency_ns = 0;
                entry->rx_callback = NULL;
                entry->rx_user_data = NULL;
                
                /* 初始化状态 */
                if (config->type == DDS_TSN_STREAM_TYPE_TALKER) {
                    entry->status.talker.state = DDS_TSN_STREAM_STATE_IDLE;
                    entry->status.talker.frames_sent = 0;
                    entry->status.talker.bytes_sent = 0;
                    entry->status.talker.tx_errors = 0;
                    entry->status.talker.stream_active = false;
                }
                else {
                    entry->status.listener.state = DDS_TSN_STREAM_STATE_IDLE;
                    entry->status.listener.frames_received = 0;
                    entry->status.listener.bytes_received = 0;
                    entry->status.listener.rx_errors = 0;
                    entry->status.listener.late_frames = 0;
                    entry->status.listener.stream_active = false;
                    entry->status.listener.source_active = false;
                }
                
                *stream_handle = (uint32_t)idx;
                g_tsn_ctx.stream_count++;
                
                /* 通知状态变化 */
                dds_tsn_update_stream_state(entry, DDS_TSN_STREAM_STATE_CONFIGURING);
            }
        }
    }
    
    return status;
}

eth_status_t dds_tsn_delete_stream(uint32_t stream_handle)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        /* 释放TSN资源 */
        (void)dds_tsn_teardown_tsn_features(entry);
        
        /* 释放Stream预留 */
        if (entry->reserved) {
            (void)dds_tsn_release_stream(stream_handle);
        }
        
        /* 清理条目 */
        entry->used = false;
        g_tsn_ctx.stream_count--;
        
        /* 更新统计 */
        if (entry->config.type == DDS_TSN_STREAM_TYPE_TALKER) {
            g_tsn_ctx.stats.active_talkers--;
        }
        else if (entry->config.type == DDS_TSN_STREAM_TYPE_LISTENER) {
            g_tsn_ctx.stats.active_listeners--;
        }
    }
    
    return status;
}

eth_status_t dds_tsn_get_stream_config(uint32_t stream_handle,
                                        dds_tsn_stream_config_t *config)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (config == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        (void)memcpy(config, &g_tsn_ctx.streams[stream_handle].config,
                    sizeof(dds_tsn_stream_config_t));
    }
    
    return status;
}

eth_status_t dds_tsn_update_stream_config(uint32_t stream_handle,
                                           const dds_tsn_stream_config_t *config)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (config == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        /* 只能在非运行状态下更新配置 */
        if (entry->state == DDS_TSN_STREAM_STATE_RUNNING) {
            status = ETH_BUSY;
        }
        else {
            (void)memcpy(&entry->config, config, sizeof(dds_tsn_stream_config_t));
        }
    }
    
    return status;
}

eth_status_t dds_tsn_get_stream_state(uint32_t stream_handle,
                                       dds_tsn_stream_state_t *state)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (state == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        *state = g_tsn_ctx.streams[stream_handle].state;
    }
    
    return status;
}

/* ============================================================================
 * Talker API实现
 * ============================================================================ */

eth_status_t dds_tsn_configure_talker(uint32_t stream_handle,
                                       const dds_tsn_talker_endpoint_t *talker_config)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (talker_config == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (entry->config.type != DDS_TSN_STREAM_TYPE_TALKER) {
            status = ETH_ERROR;
        }
        else {
            (void)memcpy(&entry->config.endpoint.talker, talker_config,
                        sizeof(dds_tsn_talker_endpoint_t));
        }
    }
    
    return status;
}

eth_status_t dds_tsn_get_talker_status(uint32_t stream_handle,
                                        dds_tsn_talker_status_t *status)
{
    eth_status_t status_ret = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status_ret = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (status == NULL)) {
        status_ret = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status_ret = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (entry->config.type != DDS_TSN_STREAM_TYPE_TALKER) {
            status_ret = ETH_ERROR;
        }
        else {
            (void)memcpy(status, &entry->status.talker,
                        sizeof(dds_tsn_talker_status_t));
        }
    }
    
    return status_ret;
}

eth_status_t dds_tsn_send_stream_data(uint32_t stream_handle,
                                       const uint8_t *data,
                                       uint32_t len,
                                       uint64_t timestamp_ns)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (data == NULL) || (len == 0)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (entry->config.type != DDS_TSN_STREAM_TYPE_TALKER) {
            status = ETH_ERROR;
        }
        else if (entry->state != DDS_TSN_STREAM_STATE_RUNNING) {
            status = ETH_ERROR;
        }
        else {
            /* 通过TSN发送数据 */
            /* 注: 实际实现需要调用TSN协议栈的发送函数 */
            
            /* 更新统计 */
            entry->frames_count++;
            entry->bytes_count += len;
            entry->status.talker.frames_sent++;
            entry->status.talker.bytes_sent += len;
            
            g_tsn_ctx.stats.total_frames_sent++;
            g_tsn_ctx.stats.total_bytes_sent += len;
            
            (void)timestamp_ns;
        }
    }
    
    return status;
}

/* ============================================================================
 * Listener API实现
 * ============================================================================ */

eth_status_t dds_tsn_configure_listener(uint32_t stream_handle,
                                         const dds_tsn_listener_endpoint_t *listener_config)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (listener_config == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (entry->config.type != DDS_TSN_STREAM_TYPE_LISTENER) {
            status = ETH_ERROR;
        }
        else {
            (void)memcpy(&entry->config.endpoint.listener, listener_config,
                        sizeof(dds_tsn_listener_endpoint_t));
        }
    }
    
    return status;
}

eth_status_t dds_tsn_get_listener_status(uint32_t stream_handle,
                                          dds_tsn_listener_status_t *status)
{
    eth_status_t status_ret = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status_ret = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (status == NULL)) {
        status_ret = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status_ret = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (entry->config.type != DDS_TSN_STREAM_TYPE_LISTENER) {
            status_ret = ETH_ERROR;
        }
        else {
            (void)memcpy(status, &entry->status.listener,
                        sizeof(dds_tsn_listener_status_t));
        }
    }
    
    return status_ret;
}

eth_status_t dds_tsn_register_rx_callback(uint32_t stream_handle,
                                           dds_tsn_data_rx_cb_t callback,
                                           void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if ((stream_handle >= DDS_TSN_MAX_STREAMS) || (callback == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        g_tsn_ctx.streams[stream_handle].rx_callback = callback;
        g_tsn_ctx.streams[stream_handle].rx_user_data = user_data;
    }
    
    return status;
}

eth_status_t dds_tsn_unregister_rx_callback(uint32_t stream_handle)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        g_tsn_ctx.streams[stream_handle].rx_callback = NULL;
        g_tsn_ctx.streams[stream_handle].rx_user_data = NULL;
    }
    
    return status;
}

/* ============================================================================
 * Stream ID管理API实现
 * ============================================================================ */

eth_status_t dds_tsn_generate_stream_id(const char *topic_name,
                                         uint32_t domain_id,
                                         dds_tsn_stream_id_t *stream_id)
{
    eth_status_t status = ETH_OK;
    
    if ((topic_name == NULL) || (stream_id == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        /* 生成Stream ID:
         * 前4字节: Domain ID
         * 后4字节: 主题名哈希
         */
        stream_id->id[0] = (uint8_t)((domain_id >> 24) & 0xFF);
        stream_id->id[1] = (uint8_t)((domain_id >> 16) & 0xFF);
        stream_id->id[2] = (uint8_t)((domain_id >> 8) & 0xFF);
        stream_id->id[3] = (uint8_t)(domain_id & 0xFF);
        
        /* 简单哈希计算 */
        uint32_t hash = 0;
        for (size_t i = 0; i < strlen(topic_name); i++) {
            hash = hash * 31 + topic_name[i];
        }
        
        stream_id->id[4] = (uint8_t)((hash >> 24) & 0xFF);
        stream_id->id[5] = (uint8_t)((hash >> 16) & 0xFF);
        stream_id->id[6] = (uint8_t)((hash >> 8) & 0xFF);
        stream_id->id[7] = (uint8_t)(hash & 0xFF);
    }
    
    return status;
}

eth_status_t dds_tsn_parse_stream_id(const dds_tsn_stream_id_t *stream_id,
                                      char *topic_name,
                                      uint32_t topic_name_len,
                                      uint32_t *domain_id)
{
    eth_status_t status = ETH_OK;
    
    if ((stream_id == NULL) || (domain_id == NULL)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        *domain_id = ((uint32_t)stream_id->id[0] << 24) |
                     ((uint32_t)stream_id->id[1] << 16) |
                     ((uint32_t)stream_id->id[2] << 8) |
                     (uint32_t)stream_id->id[3];
        
        /* 主题名不能从Stream ID解析，返回空字符串 */
        if (topic_name != NULL && topic_name_len > 0) {
            topic_name[0] = '\0';
        }
    }
    
    return status;
}

bool dds_tsn_stream_id_equal(const dds_tsn_stream_id_t *a, const dds_tsn_stream_id_t *b)
{
    return (memcmp(a->id, b->id, DDS_TSN_STREAM_ID_LEN) == 0);
}

eth_status_t dds_tsn_stream_id_to_string(const dds_tsn_stream_id_t *stream_id,
                                          char *str, uint32_t str_len)
{
    eth_status_t status = ETH_OK;
    
    if ((stream_id == NULL) || (str == NULL) || (str_len < 17)) {
        status = ETH_INVALID_PARAM;
    }
    else {
        (void)snprintf(str, str_len,
                       "%02X%02X%02X%02X-%02X%02X%02X%02X",
                       stream_id->id[0], stream_id->id[1],
                       stream_id->id[2], stream_id->id[3],
                       stream_id->id[4], stream_id->id[5],
                       stream_id->id[6], stream_id->id[7]);
    }
    
    return status;
}

/* ============================================================================
 * Stream预留和调度API实现
 * ============================================================================ */

eth_status_t dds_tsn_reserve_stream(uint32_t stream_handle)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (entry->reserved) {
            status = ETH_OK; /* 已经预留 */
        }
        else {
            /* 调用TSN预留协议 */
            /* 注: 实际实现需要调用SRP (Stream Reservation Protocol) */
            
            entry->reserved = true;
            
            /* 配置TSN特性 */
            status = dds_tsn_setup_tsn_features(entry);
        }
    }
    
    return status;
}

eth_status_t dds_tsn_release_stream(uint32_t stream_handle)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (!entry->reserved) {
            status = ETH_OK; /* 未预留 */
        }
        else {
            /* 移除TSN特性 */
            (void)dds_tsn_teardown_tsn_features(entry);
            
            /* 调用TSN释放协议 */
            entry->reserved = false;
        }
    }
    
    return status;
}

eth_status_t dds_tsn_configure_tas(uint32_t stream_handle,
                                    const void *gate_control_list,
                                    uint32_t list_length)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        /* 配置TAS (Time-Aware Shaper) */
        /* 注: 实际实现需要调用现有的TAS配置API */
        (void)gate_control_list;
        (void)list_length;
    }
    
    return status;
}

eth_status_t dds_tsn_configure_cbs(uint32_t stream_handle,
                                    uint64_t idle_slope,
                                    uint64_t send_slope)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        /* 配置CBS (Credit-Based Shaper) */
        /* 注: 实际实现需要调用现有的CBS配置API */
        (void)idle_slope;
        (void)send_slope;
    }
    
    return status;
}

/* ============================================================================
 * 状态回调API实现
 * ============================================================================ */

eth_status_t dds_tsn_register_state_callback(dds_tsn_stream_state_cb_t callback,
                                              void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_tsn_ctx.state_callback = callback;
        g_tsn_ctx.state_user_data = user_data;
    }
    
    return status;
}

eth_status_t dds_tsn_register_latency_violation_callback(
    dds_tsn_latency_violation_cb_t callback, void *user_data)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        g_tsn_ctx.latency_callback = callback;
        g_tsn_ctx.latency_user_data = user_data;
    }
    
    return status;
}

/* ============================================================================
 * 统计API实现
 * ============================================================================ */

eth_status_t dds_tsn_get_stats(dds_tsn_transport_stats_t *stats)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stats == NULL) {
        status = ETH_INVALID_PARAM;
    }
    else {
        (void)memcpy(stats, &g_tsn_ctx.stats, sizeof(dds_tsn_transport_stats_t));
    }
    
    return status;
}

eth_status_t dds_tsn_clear_stats(void)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else {
        (void)memset(&g_tsn_ctx.stats, 0, sizeof(dds_tsn_transport_stats_t));
    }
    
    return status;
}

eth_status_t dds_tsn_get_stream_stats(uint32_t stream_handle,
                                       uint64_t *frames,
                                       uint64_t *bytes,
                                       uint32_t *latency_ns)
{
    eth_status_t status = ETH_OK;
    
    if (!g_tsn_ctx.initialized) {
        status = ETH_NOT_INIT;
    }
    else if (stream_handle >= DDS_TSN_MAX_STREAMS) {
        status = ETH_INVALID_PARAM;
    }
    else if (!g_tsn_ctx.streams[stream_handle].used) {
        status = ETH_ERROR;
    }
    else {
        dds_tsn_stream_entry_t *entry = &g_tsn_ctx.streams[stream_handle];
        
        if (frames != NULL) {
            *frames = entry->frames_count;
        }
        if (bytes != NULL) {
            *bytes = entry->bytes_count;
        }
        if (latency_ns != NULL) {
            *latency_ns = entry->avg_latency_ns;
        }
    }
    
    return status;
}

/* ============================================================================
 * 内部函数实现
 * ============================================================================ */

static int32_t dds_tsn_find_free_stream_slot(void)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_TSN_MAX_STREAMS; i++) {
        if (!g_tsn_ctx.streams[i].used) {
            idx = (int32_t)i;
            break;
        }
    }
    
    return idx;
}

static int32_t dds_tsn_find_stream_by_id(const dds_tsn_stream_id_t *stream_id)
{
    int32_t idx = -1;
    
    for (uint32_t i = 0; i < DDS_TSN_MAX_STREAMS; i++) {
        if (g_tsn_ctx.streams[i].used) {
            if (dds_tsn_stream_id_equal(&g_tsn_ctx.streams[i].config.id.stream_id,
                                         stream_id)) {
                idx = (int32_t)i;
                break;
            }
        }
    }
    
    return idx;
}

static void dds_tsn_update_stream_state(dds_tsn_stream_entry_t *stream,
                                         dds_tsn_stream_state_t new_state)
{
    dds_tsn_stream_state_t old_state = stream->state;
    stream->state = new_state;
    
    /* 更新内部状态 */
    if (stream->config.type == DDS_TSN_STREAM_TYPE_TALKER) {
        stream->status.talker.state = new_state;
    }
    else {
        stream->status.listener.state = new_state;
    }
    
    /* 通知回调 */
    if ((old_state != new_state) && (g_tsn_ctx.state_callback != NULL)) {
        g_tsn_ctx.state_callback(&stream->config.id.stream_id, new_state,
                                  g_tsn_ctx.state_user_data);
    }
}

static eth_status_t dds_tsn_setup_tsn_features(dds_tsn_stream_entry_t *stream)
{
    eth_status_t status = ETH_OK;
    
    /* 配置TSN特性 */
    if (stream->config.enable_cbs) {
        /* 配置CBS */
    }
    
    if (stream->config.enable_tas) {
        /* 配置TAS */
    }
    
    if (stream->config.enable_frame_preemption) {
        /* 配置帧预先取代 */
    }
    
    return status;
}

static eth_status_t dds_tsn_teardown_tsn_features(dds_tsn_stream_entry_t *stream)
{
    eth_status_t status = ETH_OK;
    
    /* 移除TSN特性配置 */
    (void)stream;
    
    return status;
}

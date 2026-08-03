/**
 * @file secoc_freshness.c
 * @brief SecOC Freshness Value Management Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR SecOC 4.4规范的Freshness值管理
 */

#include "secoc_freshness.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

static uint64_t default_get_timestamp_us(void) {
    /* 默认实现 - 生产环境应提供真实时间源 */
    static uint64_t counter = 0;
    return counter++ * 1000;  /* 模拟每毫秒增加 */
}

static uint64_t bytes_to_uint64(const uint8_t *data, uint8_t len) {
    uint64_t value = 0;
    for (int i = 0; i < len && i < 8; i++) {
        value |= ((uint64_t)data[i] << (i * 8));
    }
    return value;
}

static void uint64_to_bytes(uint64_t value, uint8_t *data, uint8_t len) {
    for (int i = 0; i < len && i < 8; i++) {
        data[i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
}

static const char* fv_state_strings[] = {
    "OK",
    "NOT_SYNC",
    "SYNC_IN_PROGRESS",
    "SYNC_FAILED",
    "REPLAY_DETECTED",
    "OVERFLOW"
};

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

secoc_freshness_manager_t* secoc_freshness_init(void) {
    secoc_freshness_manager_t *mgr = (secoc_freshness_manager_t*)calloc(1, sizeof(secoc_freshness_manager_t));
    if (!mgr) {
        return NULL;
    }
    
    mgr->get_timestamp_us = default_get_timestamp_us;
    mgr->initialized = true;
    mgr->clock_drift_us = 0;
    
    return mgr;
}

void secoc_freshness_deinit(secoc_freshness_manager_t *mgr) {
    if (!mgr) {
        return;
    }
    
    mgr->initialized = false;
    free(mgr);
}

void secoc_freshness_set_timer(secoc_freshness_manager_t *mgr,
                               uint64_t (*get_timestamp_us)(void)) {
    if (!mgr) {
        return;
    }
    
    mgr->get_timestamp_us = get_timestamp_us ? get_timestamp_us : default_get_timestamp_us;
}

/* ============================================================================
 * Freshness值配置管理
 * ============================================================================ */

secoc_fv_entry_t* secoc_freshness_register(secoc_freshness_manager_t *mgr,
                                           uint32_t pdu_id,
                                           secoc_freshness_type_t type,
                                           secoc_sync_mode_t sync_mode,
                                           uint64_t max_value,
                                           uint32_t sync_gap) {
    if (!mgr || !mgr->initialized) {
        return NULL;
    }
    
    if (mgr->num_entries >= SECOC_FV_MAX_ENTRIES) {
        return NULL;
    }
    
    /* 检查是否已存在 */
    for (uint32_t i = 0; i < mgr->num_entries; i++) {
        if (mgr->entries[i].pdu_id == pdu_id) {
            return NULL;
        }
    }
    
    secoc_fv_entry_t *entry = &mgr->entries[mgr->num_entries];
    memset(entry, 0, sizeof(secoc_fv_entry_t));
    
    entry->pdu_id = pdu_id;
    entry->type = type;
    entry->sync_mode = sync_mode;
    entry->state = SECOC_FV_STATE_NOT_SYNC;
    entry->max_counter = max_value;
    entry->sync_gap = sync_gap;
    entry->window_size = 64;  /* 默认接收窗口大小 */
    entry->tx_counter = 1;     /* 从1开始，0保留 */
    entry->trip_counter = 1;
    
    /* 初始化时间戳 */
    entry->base_timestamp = mgr->get_timestamp_us();
    entry->timestamp_resolution = SECOC_TS_RESOLUTION_US;
    
    mgr->num_entries++;
    
    return entry;
}

secoc_status_t secoc_freshness_unregister(secoc_freshness_manager_t *mgr, uint32_t pdu_id) {
    if (!mgr || !mgr->initialized) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < mgr->num_entries; i++) {
        if (mgr->entries[i].pdu_id == pdu_id) {
            memmove(&mgr->entries[i], &mgr->entries[i + 1],
                    (mgr->num_entries - i - 1) * sizeof(secoc_fv_entry_t));
            mgr->num_entries--;
            return SECOC_OK;
        }
    }
    
    return SECOC_ERROR_INVALID_PARAM;
}

secoc_fv_entry_t* secoc_freshness_find(secoc_freshness_manager_t *mgr, uint32_t pdu_id) {
    if (!mgr || !mgr->initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < mgr->num_entries; i++) {
        if (mgr->entries[i].pdu_id == pdu_id) {
            return &mgr->entries[i];
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 计数器模式操作
 * ============================================================================ */

secoc_status_t secoc_freshness_get_tx_counter(secoc_freshness_manager_t *mgr,
                                              uint32_t pdu_id,
                                              uint64_t *freshness,
                                              uint8_t *freshness_len) {
    if (!mgr || !freshness || !freshness_len) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->type != SECOC_FRESHNESS_COUNTER) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否需要同步 (Slave模式) */
    if (entry->sync_mode == SECOC_SYNC_SLAVE && !entry->synchronized) {
        entry->state = SECOC_FV_STATE_NOT_SYNC;
        return SECOC_ERROR_SYNC_FAILED;
    }
    
    /* 检查溢出 */
    if (entry->tx_counter >= entry->max_counter) {
        entry->state = SECOC_FV_STATE_OVERFLOW;
        return SECOC_ERROR_FRESHNESS_FAILED;
    }
    
    *freshness = entry->tx_counter;
    *freshness_len = (entry->max_counter > 0xFFFFFF) ? 4 :
                     (entry->max_counter > 0xFFFF) ? 3 :
                     (entry->max_counter > 0xFF) ? 2 : 1;
    
    entry->tx_count++;
    entry->last_tx_timestamp = mgr->get_timestamp_us();
    
    return SECOC_OK;
}

secoc_status_t secoc_freshness_verify_counter(secoc_freshness_manager_t *mgr,
                                              uint32_t pdu_id,
                                              uint64_t freshness,
                                              uint8_t freshness_len) {
    (void)freshness_len;
    
    if (!mgr) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->type != SECOC_FRESHNESS_COUNTER) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    entry->rx_count++;
    entry->last_rx_timestamp = mgr->get_timestamp_us();
    
    /* 检查重放 */
    if (secoc_freshness_check_replay(entry, freshness)) {
        entry->state = SECOC_FV_STATE_REPLAY_DETECTED;
        entry->replay_count++;
        return SECOC_ERROR_REPLAY_DETECTED;
    }
    
    /* 检查Freshness值是否足够新 */
    if (freshness <= entry->last_accepted_value) {
        /* 允许少量重叠 (用于处理消息乱序) */
        if (entry->last_accepted_value - freshness > SECOC_FRESHNESS_TOLERANCE) {
            entry->state = SECOC_FV_STATE_REPLAY_DETECTED;
            return SECOC_ERROR_REPLAY_DETECTED;
        }
    }
    
    /* 检查同步阈值 (Slave模式需要同步) */
    if (entry->sync_mode == SECOC_SYNC_SLAVE) {
        uint64_t gap = (freshness > entry->rx_counter) ? 
                       (freshness - entry->rx_counter) : 0;
        if (gap > entry->sync_gap) {
            entry->state = SECOC_FV_STATE_SYNC_IN_PROGRESS;
            return SECOC_ERROR_SYNC_FAILED;
        }
    }
    
    /* 更新状态 */
    entry->rx_counter = freshness;
    entry->last_accepted_value = freshness;
    entry->state = SECOC_FV_STATE_OK;
    
    secoc_freshness_update_window(entry, freshness);
    
    return SECOC_OK;
}

void secoc_freshness_increment_counter(secoc_fv_entry_t *entry) {
    if (!entry) {
        return;
    }
    
    if (entry->tx_counter < entry->max_counter) {
        entry->tx_counter++;
    }
}

/* ============================================================================
 * 时间戳模式操作
 * ============================================================================ */

secoc_status_t secoc_freshness_get_tx_timestamp(secoc_freshness_manager_t *mgr,
                                                uint32_t pdu_id,
                                                uint64_t *freshness,
                                                uint8_t *freshness_len) {
    if (!mgr || !freshness || !freshness_len) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->type != SECOC_FRESHNESS_TIMESTAMP) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    uint64_t current_time = mgr->get_timestamp_us();
    
    /* 计算相对时间戳 */
    *freshness = (current_time - entry->base_timestamp) / entry->timestamp_resolution;
    *freshness_len = 4;  /* 默认4字节 */
    
    entry->tx_count++;
    entry->last_tx_timestamp = current_time;
    
    return SECOC_OK;
}

secoc_status_t secoc_freshness_verify_timestamp(secoc_freshness_manager_t *mgr,
                                                uint32_t pdu_id,
                                                uint64_t freshness,
                                                uint8_t freshness_len) {
    (void)freshness_len;
    
    if (!mgr) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->type != SECOC_FRESHNESS_TIMESTAMP) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    entry->rx_count++;
    
    uint64_t current_time = mgr->get_timestamp_us();
    uint64_t received_timestamp = entry->base_timestamp + (freshness * entry->timestamp_resolution);
    
    /* 校准时钟偏移 */
    int64_t time_diff = (int64_t)received_timestamp - (int64_t)current_time + mgr->clock_drift_us;
    
    /* 检查时间戳是否合理 (允许一定的偏移) */
    if (time_diff < -(int64_t)(SECOC_TS_MAX_DRIFT_MS * 1000)) {
        /* 时间戳太老 */
        entry->state = SECOC_FV_STATE_REPLAY_DETECTED;
        return SECOC_ERROR_REPLAY_DETECTED;
    }
    
    if (time_diff > (int64_t)(SECOC_TS_MAX_DRIFT_MS * 1000)) {
        /* 时间戳太新 - 可能是时钟偏移，校准 */
        secoc_freshness_calibrate_clock(mgr, current_time, received_timestamp);
    }
    
    entry->last_rx_timestamp = current_time;
    entry->state = SECOC_FV_STATE_OK;
    
    return SECOC_OK;
}

void secoc_freshness_calibrate_clock(secoc_freshness_manager_t *mgr,
                                     uint64_t local_timestamp,
                                     uint64_t remote_timestamp) {
    if (!mgr) {
        return;
    }
    
    /* 简单的低通滤波 */
    int64_t new_drift = (int64_t)remote_timestamp - (int64_t)local_timestamp;
    mgr->clock_drift_us = (mgr->clock_drift_us * 7 + new_drift) / 8;
}

/* ============================================================================
 * 行程计数器模式操作
 * ============================================================================ */

secoc_status_t secoc_freshness_get_tx_trip(secoc_freshness_manager_t *mgr,
                                           uint32_t pdu_id,
                                           uint64_t *freshness,
                                           uint8_t *freshness_len) {
    if (!mgr || !freshness || !freshness_len) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->type != SECOC_FRESHNESS_TRIP_COUNTER) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 组合行程计数器、复位计数器和同步计数器 */
    *freshness = ((uint64_t)entry->trip_counter << 32) |
                 ((uint64_t)entry->reset_counter << 24) |
                 (entry->tx_counter & 0xFFFFFF);
    *freshness_len = 8;
    
    entry->tx_count++;
    
    return SECOC_OK;
}

secoc_status_t secoc_freshness_verify_trip(secoc_freshness_manager_t *mgr,
                                           uint32_t pdu_id,
                                           uint64_t freshness,
                                           uint8_t freshness_len) {
    (void)freshness_len;
    
    if (!mgr) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->type != SECOC_FRESHNESS_TRIP_COUNTER) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    entry->rx_count++;
    
    /* 解析收到的Freshness值 */
    uint32_t received_trip = (freshness >> 32) & 0xFFFFFFFF;
    uint32_t received_reset = (freshness >> 24) & 0xFF;
    uint64_t received_counter = freshness & 0xFFFFFF;
    
    /* 检查行程计数器 */
    if (received_trip < entry->trip_counter) {
        entry->state = SECOC_FV_STATE_REPLAY_DETECTED;
        return SECOC_ERROR_REPLAY_DETECTED;
    }
    
    /* 检查复位计数器 (同行程内) */
    if (received_trip == entry->trip_counter && received_reset < entry->reset_counter) {
        entry->state = SECOC_FV_STATE_REPLAY_DETECTED;
        return SECOC_ERROR_REPLAY_DETECTED;
    }
    
    /* 检查同步计数器 */
    if (received_trip == entry->trip_counter && 
        received_reset == entry->reset_counter) {
        if (received_counter <= entry->last_accepted_value) {
            entry->state = SECOC_FV_STATE_REPLAY_DETECTED;
            return SECOC_ERROR_REPLAY_DETECTED;
        }
    }
    
    entry->last_accepted_value = received_counter;
    entry->state = SECOC_FV_STATE_OK;
    
    return SECOC_OK;
}

void secoc_freshness_increment_trip(secoc_fv_entry_t *entry) {
    if (!entry) {
        return;
    }
    
    if (entry->trip_counter < SECOC_TRIP_MAX_VALUE) {
        entry->trip_counter++;
        entry->tx_counter = 1;  /* 重置同步计数器 */
    }
}

void secoc_freshness_increment_reset(secoc_fv_entry_t *entry) {
    if (!entry) {
        return;
    }
    
    if (entry->reset_counter < SECOC_RESET_MAX_VALUE) {
        entry->reset_counter++;
        entry->tx_counter = 1;  /* 重置同步计数器 */
    } else {
        /* 复位计数器溢出，增加行程计数器 */
        secoc_freshness_increment_trip(entry);
        entry->reset_counter = 0;
    }
}

/* ============================================================================
 * Master-Slave同步机制
 * ============================================================================ */

secoc_status_t secoc_freshness_create_sync_request(secoc_freshness_manager_t *mgr,
                                                   uint32_t pdu_id,
                                                   secoc_sync_request_t *request) {
    if (!mgr || !request) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->sync_mode != SECOC_SYNC_SLAVE) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    memset(request, 0, sizeof(secoc_sync_request_t));
    request->header.msg_type = SECOC_SYNC_REQ;
    request->header.pdu_id_high = (pdu_id >> 8) & 0xFF;
    request->header.pdu_id_low = pdu_id & 0xFF;
    request->slave_fv_low = entry->rx_counter & 0xFFFFFFFF;
    request->timestamp = (uint32_t)(mgr->get_timestamp_us() / 1000);
    
    entry->state = SECOC_FV_STATE_SYNC_IN_PROGRESS;
    entry->sync_retry_count++;
    
    return SECOC_OK;
}

secoc_status_t secoc_freshness_handle_sync_request(secoc_freshness_manager_t *mgr,
                                                   uint32_t pdu_id,
                                                   const secoc_sync_request_t *request,
                                                   secoc_sync_response_t *response) {
    if (!mgr || !request || !response) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->sync_mode != SECOC_SYNC_MASTER) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 检查请求类型 */
    if (request->header.msg_type != SECOC_SYNC_REQ) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 构建响应 */
    memset(response, 0, sizeof(secoc_sync_response_t));
    response->header.msg_type = SECOC_SYNC_RES;
    response->header.pdu_id_high = (pdu_id >> 8) & 0xFF;
    response->header.pdu_id_low = pdu_id & 0xFF;
    response->master_fv = entry->tx_counter;
    response->trip_counter = entry->trip_counter;
    response->reset_counter = entry->reset_counter;
    response->master_timestamp = (uint32_t)(mgr->get_timestamp_us() / 1000);
    
    /* 计算MAC (简化版本 - 生产环境应使用HMAC) */
    /* 这里只是为了演示，实际项目中需要完整的MAC计算 */
    for (int i = 0; i < 8; i++) {
        response->mac[i] = (uint8_t)((response->master_fv >> (i * 8)) & 0xFF);
    }
    
    entry->sync_count++;
    
    return SECOC_OK;
}

secoc_status_t secoc_freshness_create_sync_broadcast(secoc_freshness_manager_t *mgr,
                                                     uint32_t pdu_id,
                                                     secoc_sync_response_t *broadcast) {
    if (!mgr || !broadcast) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->sync_mode != SECOC_SYNC_MASTER) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    memset(broadcast, 0, sizeof(secoc_sync_response_t));
    broadcast->header.msg_type = SECOC_SYNC_BROADCAST;
    broadcast->header.pdu_id_high = (pdu_id >> 8) & 0xFF;
    broadcast->header.pdu_id_low = pdu_id & 0xFF;
    broadcast->master_fv = entry->tx_counter;
    broadcast->trip_counter = entry->trip_counter;
    broadcast->reset_counter = entry->reset_counter;
    broadcast->master_timestamp = (uint32_t)(mgr->get_timestamp_us() / 1000);
    
    /* 计算MAC */
    for (int i = 0; i < 8; i++) {
        broadcast->mac[i] = (uint8_t)((broadcast->master_fv >> (i * 8)) & 0xFF);
    }
    
    entry->sync_count++;
    entry->last_sync_time = mgr->get_timestamp_us();
    
    return SECOC_OK;
}

secoc_status_t secoc_freshness_handle_sync_response(secoc_freshness_manager_t *mgr,
                                                    uint32_t pdu_id,
                                                    const secoc_sync_response_t *response) {
    if (!mgr || !response) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->sync_mode != SECOC_SYNC_SLAVE) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 验证消息类型 */
    if (response->header.msg_type != SECOC_SYNC_RES && 
        response->header.msg_type != SECOC_SYNC_BROADCAST) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 验证MAC (简化版本) */
    uint8_t computed_mac[8];
    for (int i = 0; i < 8; i++) {
        computed_mac[i] = (uint8_t)((response->master_fv >> (i * 8)) & 0xFF);
    }
    
    if (memcmp(response->mac, computed_mac, 8) != 0) {
        entry->state = SECOC_FV_STATE_SYNC_FAILED;
        return SECOC_ERROR_SYNC_FAILED;
    }
    
    /* 更新Freshness值 */
    entry->rx_counter = response->master_fv;
    entry->trip_counter = response->trip_counter;
    entry->reset_counter = response->reset_counter;
    entry->last_accepted_value = response->master_fv;
    entry->synchronized = true;
    entry->state = SECOC_FV_STATE_OK;
    entry->sync_count++;
    entry->last_sync_time = mgr->get_timestamp_us();
    entry->sync_retry_count = 0;
    
    return SECOC_OK;
}

bool secoc_freshness_need_sync(secoc_freshness_manager_t *mgr,
                               uint32_t pdu_id,
                               uint64_t current_time) {
    if (!mgr) {
        return false;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return false;
    }
    
    if (entry->sync_mode != SECOC_SYNC_MASTER) {
        return false;
    }
    
    /* 检查是否到了发送同步消息的时间 */
    uint64_t elapsed = current_time - entry->last_sync_time;
    return (elapsed >= SECOC_SYNC_MASTER_INTERVAL_MS * 1000);
}

secoc_status_t secoc_freshness_force_sync(secoc_freshness_manager_t *mgr, uint32_t pdu_id) {
    if (!mgr) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    if (entry->sync_mode != SECOC_SYNC_SLAVE) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    entry->state = SECOC_FV_STATE_SYNC_IN_PROGRESS;
    entry->synchronized = false;
    
    /* 触发同步请求回调 */
    if (mgr->on_sync_request) {
        secoc_sync_request_t request;
        secoc_freshness_create_sync_request(mgr, pdu_id, &request);
        mgr->on_sync_request(pdu_id, &request);
    }
    
    return SECOC_OK;
}

/* ============================================================================
 * 通用接口
 * ============================================================================ */

secoc_status_t secoc_freshness_get_tx_value(secoc_freshness_manager_t *mgr,
                                            uint32_t pdu_id,
                                            uint64_t *freshness,
                                            uint8_t *freshness_len) {
    if (!mgr || !freshness || !freshness_len) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    switch (entry->type) {
        case SECOC_FRESHNESS_COUNTER:
            return secoc_freshness_get_tx_counter(mgr, pdu_id, freshness, freshness_len);
            
        case SECOC_FRESHNESS_TIMESTAMP:
            return secoc_freshness_get_tx_timestamp(mgr, pdu_id, freshness, freshness_len);
            
        case SECOC_FRESHNESS_TRIP_COUNTER:
            return secoc_freshness_get_tx_trip(mgr, pdu_id, freshness, freshness_len);
            
        default:
            return SECOC_ERROR_INVALID_PARAM;
    }
}

secoc_status_t secoc_freshness_verify(secoc_freshness_manager_t *mgr,
                                      uint32_t pdu_id,
                                      uint64_t freshness,
                                      uint8_t freshness_len,
                                      secoc_verify_result_t *result) {
    if (!mgr || !result) {
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_fv_entry_t *entry = secoc_freshness_find(mgr, pdu_id);
    if (!entry) {
        *result = SECOC_VERIFY_FAILURE;
        return SECOC_ERROR_INVALID_PARAM;
    }
    
    secoc_status_t status;
    
    switch (entry->type) {
        case SECOC_FRESHNESS_COUNTER:
            status = secoc_freshness_verify_counter(mgr, pdu_id, freshness, freshness_len);
            break;
            
        case SECOC_FRESHNESS_TIMESTAMP:
            status = secoc_freshness_verify_timestamp(mgr, pdu_id, freshness, freshness_len);
            break;
            
        case SECOC_FRESHNESS_TRIP_COUNTER:
            status = secoc_freshness_verify_trip(mgr, pdu_id, freshness, freshness_len);
            break;
            
        default:
            *result = SECOC_VERIFY_FAILURE;
            return SECOC_ERROR_INVALID_PARAM;
    }
    
    /* 映射状态到验证结果 */
    switch (status) {
        case SECOC_OK:
            *result = SECOC_VERIFY_SUCCESS;
            break;
        case SECOC_ERROR_REPLAY_DETECTED:
            *result = SECOC_VERIFY_REPLAY_DETECTED;
            break;
        case SECOC_ERROR_FRESHNESS_FAILED:
            *result = SECOC_VERIFY_FRESHNESS_FAILED;
            break;
        default:
            *result = SECOC_VERIFY_FAILURE;
            break;
    }
    
    return SECOC_OK;
}

/* ============================================================================
 * 重放保护
 * ============================================================================ */

bool secoc_freshness_check_replay(secoc_fv_entry_t *entry, uint64_t received_value) {
    if (!entry) {
        return true;  /* 安全假设: 检测到重放 */
    }
    
    /* 检查是否已在窗口中 */
    if (received_value >= entry->window_start &&
        received_value < entry->window_start + entry->window_size) {
        /* 在窗口内，检查是否已接收过 */
        /* 简化版本: 只检查是否小于等于上次接收的值 */
        if (received_value <= entry->last_accepted_value) {
            return true;  /* 检测到重放 */
        }
    }
    
    return false;
}

void secoc_freshness_update_window(secoc_fv_entry_t *entry, uint64_t accepted_value) {
    if (!entry) {
        return;
    }
    
    /* 更新窗口起点，确保接受的值在窗口内 */
    if (accepted_value > entry->window_size / 2) {
        entry->window_start = accepted_value - entry->window_size / 2;
    } else {
        entry->window_start = 0;
    }
}

/* ============================================================================
 * 统计和调试
 * ============================================================================ */

void secoc_freshness_get_stats(secoc_fv_entry_t *entry,
                               uint64_t *tx_count,
                               uint64_t *rx_count,
                               uint64_t *sync_count,
                               uint64_t *replay_count) {
    if (!entry) {
        return;
    }
    
    if (tx_count) *tx_count = entry->tx_count;
    if (rx_count) *rx_count = entry->rx_count;
    if (sync_count) *sync_count = entry->sync_count;
    if (replay_count) *replay_count = entry->replay_count;
}

const char* secoc_freshness_state_str(secoc_fv_state_t state) {
    if (state < sizeof(fv_state_strings) / sizeof(fv_state_strings[0])) {
        return fv_state_strings[state];
    }
    return "UNKNOWN";
}

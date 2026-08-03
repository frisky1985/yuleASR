/**
 * @file cbs.c
 * @brief 基于信用的调度器实现 (IEEE 802.1Qav-2009)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note SR Class A (2ms) / SR Class B (50ms)
 * @note 符合ASIL-D安全等级
 */

#include "cbs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* ============================================================================
 * 常量和宏定义
 * ============================================================================ */

#define CBS_MAX_PORTS                   8
#define CBS_MAX_QUEUES_PER_PORT         8
#define NS_PER_US                       1000ULL

/* ============================================================================
 * 内部数据结构
 * ============================================================================ */

typedef struct {
    cbs_port_config_t config;
    bool configured;
    
    cbs_queue_state_t queue_states[CBS_SR_CLASS_MAX];
    cbs_safety_monitor_t safety_monitor;
    
    uint64_t last_update_time_ns;
    uint64_t total_bytes_transmitted;
    uint64_t bandwidth_check_time_ns;
} cbs_port_runtime_t;

typedef struct {
    bool initialized;
    cbs_port_runtime_t ports[CBS_MAX_PORTS];
    
    cbs_credit_update_callback_t credit_cb;
    cbs_transmit_permission_callback_t tx_perm_cb;
    cbs_bandwidth_alert_callback_t bw_alert_cb;
    cbs_safety_alert_callback_t safety_cb;
    void *credit_user_data;
    void *tx_perm_user_data;
    void *bw_alert_user_data;
    void *safety_user_data;
} cbs_global_state_t;

/* ============================================================================
 * 全局状态实例
 * ============================================================================ */

static cbs_global_state_t g_cbs_state;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

static uint64_t get_current_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static cbs_port_runtime_t* get_port(uint16_t port_id) {
    if (port_id >= CBS_MAX_PORTS) {
        return NULL;
    }
    return &g_cbs_state.ports[port_id];
}

static eth_status_t check_safety(cbs_port_runtime_t *port) {
    cbs_safety_monitor_t *monitor = &port->safety_monitor;
    
    /* 检查信用限制 */
    for (int i = 0; i < CBS_SR_CLASS_MAX; i++) {
        cbs_queue_state_t *queue = &port->queue_states[i];
        
        if (queue->current_credit < CBS_MAX_NEGATIVE_CREDIT) {
            monitor->credit_limit_violated = true;
            queue->credit_violations++;
            
            if (g_cbs_state.safety_cb) {
                g_cbs_state.safety_cb(port->config.port_id, 0x01,
                                      "Credit limit violated",
                                      g_cbs_state.safety_user_data);
            }
        }
    }
    
    return ETH_OK;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

eth_status_t cbs_init(void) {
    if (g_cbs_state.initialized) {
        cbs_deinit();
    }
    
    memset(&g_cbs_state, 0, sizeof(g_cbs_state));
    
    for (int i = 0; i < CBS_MAX_PORTS; i++) {
        g_cbs_state.ports[i].config.port_id = i;
        for (int j = 0; j < CBS_SR_CLASS_MAX; j++) {
            g_cbs_state.ports[i].queue_states[j].current_credit = 0;
        }
    }
    
    g_cbs_state.initialized = true;
    return ETH_OK;
}

void cbs_deinit(void) {
    if (!g_cbs_state.initialized) {
        return;
    }
    
    memset(&g_cbs_state, 0, sizeof(g_cbs_state));
}

eth_status_t cbs_config_port(uint16_t port_id, const cbs_port_config_t *config) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(&g_cbs_state.ports[port_id].config, config, sizeof(cbs_port_config_t));
    g_cbs_state.ports[port_id].configured = true;
    g_cbs_state.ports[port_id].last_update_time_ns = get_current_time_ns();
    
    return ETH_OK;
}

eth_status_t cbs_get_port_config(uint16_t port_id, cbs_port_config_t *config) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(config, &g_cbs_state.ports[port_id].config, sizeof(cbs_port_config_t));
    return ETH_OK;
}

eth_status_t cbs_config_sr_class(uint16_t port_id, uint8_t class_index,
                                  const cbs_sr_class_config_t *sr_config) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        class_index >= CBS_SR_CLASS_MAX || sr_config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(&g_cbs_state.ports[port_id].config.sr_classes[class_index],
           sr_config, sizeof(cbs_sr_class_config_t));
    
    return ETH_OK;
}

eth_status_t cbs_config_queue(uint16_t port_id, uint8_t queue_index,
                               const cbs_queue_config_t *queue_config) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_index >= CBS_SR_CLASS_MAX || queue_config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(&g_cbs_state.ports[port_id].config.queue_configs[queue_index],
           queue_config, sizeof(cbs_queue_config_t));
    
    return ETH_OK;
}

eth_status_t cbs_update_credit(uint16_t port_id, uint8_t queue_id, 
                                uint64_t elapsed_time_ns) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    if (port == NULL || !port->configured) {
        return ETH_NOT_INIT;
    }
    
    cbs_queue_state_t *queue = &port->queue_states[queue_id];
    cbs_queue_config_t *config = &port->config.queue_configs[queue_id];
    
    /* 信用更新公式:
     * - 传输中: credit = credit + send_slope * time
     * - 空闲中: credit = credit + idle_slope * time
     */
    int64_t time_us = (int64_t)(elapsed_time_ns / NS_PER_US);
    int64_t credit_change;
    
    if (queue->transmission_in_progress) {
        /* 传输中 - 信用减少 */
        credit_change = (config->send_slope_bps * time_us) / 1000000;
    } else {
        /* 空闲中 - 信用增加 */
        credit_change = (config->idle_slope_bps * time_us) / 1000000;
    }
    
    queue->current_credit += credit_change;
    
    /* 限制信用在合理范围内 */
    if (queue->current_credit > config->high_credit) {
        queue->current_credit = config->high_credit;
    }
    if (queue->current_credit < config->low_credit) {
        queue->current_credit = config->low_credit;
    }
    
    /* 更新观察值 */
    if (queue->current_credit < queue->min_credit_seen) {
        queue->min_credit_seen = queue->current_credit;
    }
    if (queue->current_credit > queue->max_credit_seen) {
        queue->max_credit_seen = queue->current_credit;
    }
    
    /* 通知回调 */
    if (g_cbs_state.credit_cb) {
        g_cbs_state.credit_cb(port_id, queue_id, queue->current_credit, 
                              g_cbs_state.credit_user_data);
    }
    
    return ETH_OK;
}

eth_status_t cbs_get_credit(uint16_t port_id, uint8_t queue_id, int64_t *credit) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX || credit == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    if (port == NULL) {
        return ETH_NOT_INIT;
    }
    
    *credit = port->queue_states[queue_id].current_credit;
    return ETH_OK;
}

eth_status_t cbs_set_credit(uint16_t port_id, uint8_t queue_id, int64_t credit) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    if (port == NULL) {
        return ETH_NOT_INIT;
    }
    
    port->queue_states[queue_id].current_credit = credit;
    return ETH_OK;
}

eth_status_t cbs_decrement_credit(uint16_t port_id, uint8_t queue_id, 
                                   uint32_t frame_size) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    cbs_queue_state_t *queue = &port->queue_states[queue_id];
    cbs_queue_config_t *config = &port->config.queue_configs[queue_id];
    
    /* 传输完成后减少信用
     * 注: 在实际传输中，信用会逐渐减少
     * 这里只做最终确认
     */
    int64_t credit_cost = (int64_t)frame_size * 8;  /* 转换为bit */
    
    if (queue->current_credit >= 0) {
        /* 如果传输前信用>=0，传输后信用为负 */
        queue->current_credit = -credit_cost;
    }
    
    /* 限制最小信用 */
    if (queue->current_credit < config->low_credit) {
        queue->current_credit = config->low_credit;
    }
    
    return ETH_OK;
}

eth_status_t cbs_can_transmit(uint16_t port_id, uint8_t queue_id, 
                               uint32_t frame_size, bool *can_transmit) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX || can_transmit == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    if (port == NULL || !port->configured) {
        *can_transmit = false;
        return ETH_OK;
    }
    
    cbs_queue_state_t *queue = &port->queue_states[queue_id];
    cbs_queue_config_t *config = &port->config.queue_configs[queue_id];
    
    /* 先更新信用 */
    uint64_t current_time = get_current_time_ns();
    uint64_t elapsed = current_time - port->last_update_time_ns;
    cbs_update_credit(port_id, queue_id, elapsed);
    port->last_update_time_ns = current_time;
    
    /* CBS传输规则: 只有当信用 >= 所需信用时才能传输 */
    int64_t required_credit = (int64_t)frame_size * 8;
    
    *can_transmit = (queue->current_credit >= required_credit);
    
    /* 通知回调 */
    if (g_cbs_state.tx_perm_cb) {
        g_cbs_state.tx_perm_cb(port_id, queue_id, *can_transmit, required_credit,
                              g_cbs_state.tx_perm_user_data);
    }
    
    return ETH_OK;
}

eth_status_t cbs_start_transmission(uint16_t port_id, uint8_t queue_id, 
                                     uint32_t frame_size) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    cbs_queue_state_t *queue = &port->queue_states[queue_id];
    
    queue->transmission_in_progress = true;
    queue->transmission_start_time_ns = get_current_time_ns();
    queue->frames_transmitted++;
    queue->bytes_transmitted += frame_size;
    
    port->total_bytes_transmitted += frame_size;
    
    return ETH_OK;
}

eth_status_t cbs_complete_transmission(uint16_t port_id, uint8_t queue_id,
                                        uint32_t frame_size, uint64_t actual_time_ns) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    cbs_queue_state_t *queue = &port->queue_states[queue_id];
    cbs_queue_config_t *config = &port->config.queue_configs[queue_id];
    
    queue->transmission_in_progress = false;
    
    /* 更新信用 */
    cbs_update_credit(port_id, queue_id, actual_time_ns);
    cbs_decrement_credit(port_id, queue_id, frame_size);
    
    /* 检查传输超时 */
    uint32_t expected_time_us = (frame_size * 8) / 
                                (port->config.port_transmit_rate_bps / 1000000);
    uint32_t actual_time_us = (uint32_t)(actual_time_ns / NS_PER_US);
    
    if (actual_time_us > expected_time_us + config->transmission_overrun_us) {
        queue->transmission_overruns++;
    }
    
    return ETH_OK;
}

eth_status_t cbs_get_sr_class_priority(cbs_sr_class_type_t class_type, uint8_t *priority) {
    if (priority == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    switch (class_type) {
        case CBS_SR_CLASS_A_TYPE:
            *priority = 3;  /* 通常SR Class A使用优先级3 */
            break;
        case CBS_SR_CLASS_B_TYPE:
            *priority = 2;  /* 通常SR Class B使用优先级2 */
            break;
        default:
            return ETH_INVALID_PARAM;
    }
    
    return ETH_OK;
}

eth_status_t cbs_get_sr_class_latency(cbs_sr_class_type_t class_type, uint32_t *latency_us) {
    if (latency_us == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    switch (class_type) {
        case CBS_SR_CLASS_A_TYPE:
            *latency_us = CBS_CLASS_A_MAX_LATENCY_US;
            break;
        case CBS_SR_CLASS_B_TYPE:
            *latency_us = CBS_CLASS_B_MAX_LATENCY_US;
            break;
        default:
            return ETH_INVALID_PARAM;
    }
    
    return ETH_OK;
}

eth_status_t cbs_calc_idle_slope(uint32_t port_transmit_rate_bps,
                                  uint32_t bandwidth_percent,
                                  int64_t *idle_slope) {
    if (idle_slope == NULL || bandwidth_percent > 100) {
        return ETH_INVALID_PARAM;
    }
    
    /* 限制最大带宽 */
    if (bandwidth_percent > CBS_IDLE_SLOPE_MAX_PERCENT) {
        bandwidth_percent = CBS_IDLE_SLOPE_MAX_PERCENT;
    }
    
    *idle_slope = (int64_t)port_transmit_rate_bps * bandwidth_percent / 100;
    
    return ETH_OK;
}

eth_status_t cbs_calc_send_slope(uint32_t port_transmit_rate_bps,
                                  int64_t idle_slope,
                                  int64_t *send_slope) {
    if (send_slope == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* Send slope = Idle slope - Port transmit rate */
    *send_slope = idle_slope - (int64_t)port_transmit_rate_bps;
    
    return ETH_OK;
}

eth_status_t cbs_get_stats(uint16_t port_id, cbs_stats_t *stats) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    
    memset(stats, 0, sizeof(cbs_stats_t));
    
    for (int i = 0; i < CBS_SR_CLASS_MAX; i++) {
        memcpy(&stats->queue_states[i], &port->queue_states[i], sizeof(cbs_queue_state_t));
        stats->total_frames_transmitted += port->queue_states[i].frames_transmitted;
        stats->total_bytes_transmitted += port->queue_states[i].bytes_transmitted;
        stats->total_credit_violations += port->queue_states[i].credit_violations;
    }
    
    return ETH_OK;
}

eth_status_t cbs_clear_stats(uint16_t port_id) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    
    for (int i = 0; i < CBS_SR_CLASS_MAX; i++) {
        port->queue_states[i].frames_transmitted = 0;
        port->queue_states[i].bytes_transmitted = 0;
        port->queue_states[i].credit_violations = 0;
        port->queue_states[i].transmission_overruns = 0;
        port->queue_states[i].min_credit_seen = 0;
        port->queue_states[i].max_credit_seen = 0;
    }
    
    port->total_bytes_transmitted = 0;
    
    return ETH_OK;
}

eth_status_t cbs_get_queue_state(uint16_t port_id, uint8_t queue_id, 
                                  cbs_queue_state_t *state) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        queue_id >= CBS_SR_CLASS_MAX || state == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(state, &g_cbs_state.ports[port_id].queue_states[queue_id], 
           sizeof(cbs_queue_state_t));
    
    return ETH_OK;
}

eth_status_t cbs_register_credit_update_callback(cbs_credit_update_callback_t callback, 
                                                  void *user_data) {
    g_cbs_state.credit_cb = callback;
    g_cbs_state.credit_user_data = user_data;
    return ETH_OK;
}

eth_status_t cbs_register_transmit_permission_callback(cbs_transmit_permission_callback_t callback,
                                                        void *user_data) {
    g_cbs_state.tx_perm_cb = callback;
    g_cbs_state.tx_perm_user_data = user_data;
    return ETH_OK;
}

eth_status_t cbs_register_bandwidth_alert_callback(cbs_bandwidth_alert_callback_t callback,
                                                    void *user_data) {
    g_cbs_state.bw_alert_cb = callback;
    g_cbs_state.bw_alert_user_data = user_data;
    return ETH_OK;
}

eth_status_t cbs_register_safety_alert_callback(cbs_safety_alert_callback_t callback,
                                                 void *user_data) {
    g_cbs_state.safety_cb = callback;
    g_cbs_state.safety_user_data = user_data;
    return ETH_OK;
}

eth_status_t cbs_init_safety_monitor(uint16_t port_id) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_safety_monitor_t *monitor = &g_cbs_state.ports[port_id].safety_monitor;
    
    memset(monitor, 0, sizeof(cbs_safety_monitor_t));
    monitor->min_credit_limit = CBS_MAX_NEGATIVE_CREDIT;
    monitor->max_credit_limit = 0;
    
    return ETH_OK;
}

eth_status_t cbs_run_safety_checks(uint16_t port_id) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    return check_safety(&g_cbs_state.ports[port_id]);
}

eth_status_t cbs_get_safety_monitor(uint16_t port_id, cbs_safety_monitor_t *monitor) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || monitor == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(monitor, &g_cbs_state.ports[port_id].safety_monitor, 
           sizeof(cbs_safety_monitor_t));
    
    return ETH_OK;
}

eth_status_t cbs_check_bandwidth_utilization(uint16_t port_id, 
                                              uint32_t *utilization_percent) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS || 
        utilization_percent == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    
    uint64_t current_time = get_current_time_ns();
    uint64_t elapsed_ns = current_time - port->bandwidth_check_time_ns;
    
    if (elapsed_ns == 0) {
        *utilization_percent = 0;
        return ETH_OK;
    }
    
    /* 计算带宽利用率 */
    uint64_t bits_sent = port->total_bytes_transmitted * 8;
    uint64_t elapsed_us = elapsed_ns / NS_PER_US;
    uint64_t actual_rate = (bits_sent * 1000000) / elapsed_us;
    
    *utilization_percent = (uint32_t)((actual_rate * 100) / port->config.port_transmit_rate_bps);
    
    /* 检查阈值 */
    if (*utilization_percent > port->config.bandwidth_alarm_threshold) {
        port->safety_monitor.bandwidth_threshold_exceeded = true;
        
        if (g_cbs_state.bw_alert_cb) {
            g_cbs_state.bw_alert_cb(port_id, *utilization_percent, 
                                    g_cbs_state.bw_alert_user_data);
        }
    }
    
    /* 重置计数器 */
    port->total_bytes_transmitted = 0;
    port->bandwidth_check_time_ns = current_time;
    
    return ETH_OK;
}

eth_status_t cbs_create_automotive_config(uint16_t port_id, 
                                           uint32_t port_rate_mbps,
                                           cbs_port_config_t *config) {
    if (config == NULL || port_rate_mbps == 0) {
        return ETH_INVALID_PARAM;
    }
    
    memset(config, 0, sizeof(cbs_port_config_t));
    
    config->port_id = port_id;
    config->port_transmit_rate_bps = (uint32_t)port_rate_mbps * 1000000;
    config->sr_class_count = 2;
    config->queue_count = 2;
    config->enable_safety_checks = true;
    config->bandwidth_alarm_threshold = CBS_SAFETY_BANDWIDTH_THRESHOLD;
    
    /* SR Class A 配置 */
    config->sr_classes[CBS_SR_CLASS_A].class_type = CBS_SR_CLASS_A_TYPE;
    config->sr_classes[CBS_SR_CLASS_A].max_interval_frames = 10;
    config->sr_classes[CBS_SR_CLASS_A].max_frame_size = 300;
    config->sr_classes[CBS_SR_CLASS_A].max_latency_us = CBS_CLASS_A_MAX_LATENCY_US;
    config->sr_classes[CBS_SR_CLASS_A].interval_us = CBS_CLASS_A_INTERVAL_US;
    config->sr_classes[CBS_SR_CLASS_A].priority = 3;
    config->sr_classes[CBS_SR_CLASS_A].vid = 2;
    
    /* SR Class B 配置 */
    config->sr_classes[CBS_SR_CLASS_B].class_type = CBS_SR_CLASS_B_TYPE;
    config->sr_classes[CBS_SR_CLASS_B].max_interval_frames = 5;
    config->sr_classes[CBS_SR_CLASS_B].max_frame_size = 500;
    config->sr_classes[CBS_SR_CLASS_B].max_latency_us = CBS_CLASS_B_MAX_LATENCY_US;
    config->sr_classes[CBS_SR_CLASS_B].interval_us = CBS_CLASS_B_INTERVAL_US;
    config->sr_classes[CBS_SR_CLASS_B].priority = 2;
    config->sr_classes[CBS_SR_CLASS_B].vid = 2;
    
    uint32_t port_rate_bps = port_rate_mbps * 1000000;
    
    /* Class A 队列配置 - 25% 带宽 */
    config->queue_configs[CBS_SR_CLASS_A].queue_id = 0;
    config->queue_configs[CBS_SR_CLASS_A].sr_class = CBS_SR_CLASS_A_TYPE;
    cbs_calc_idle_slope(port_rate_bps, 25, &config->queue_configs[CBS_SR_CLASS_A].idle_slope_bps);
    cbs_calc_send_slope(port_rate_bps, config->queue_configs[CBS_SR_CLASS_A].idle_slope_bps,
                        &config->queue_configs[CBS_SR_CLASS_A].send_slope_bps);
    config->queue_configs[CBS_SR_CLASS_A].low_credit = CBS_MAX_NEGATIVE_CREDIT;
    config->queue_configs[CBS_SR_CLASS_A].high_credit = 0;
    config->queue_configs[CBS_SR_CLASS_A].transmission_overrun_us = CBS_TRANSMISSION_OVERRUN_US;
    
    /* Class B 队列配置 - 25% 带宽 */
    config->queue_configs[CBS_SR_CLASS_B].queue_id = 1;
    config->queue_configs[CBS_SR_CLASS_B].sr_class = CBS_SR_CLASS_B_TYPE;
    cbs_calc_idle_slope(port_rate_bps, 25, &config->queue_configs[CBS_SR_CLASS_B].idle_slope_bps);
    cbs_calc_send_slope(port_rate_bps, config->queue_configs[CBS_SR_CLASS_B].idle_slope_bps,
                        &config->queue_configs[CBS_SR_CLASS_B].send_slope_bps);
    config->queue_configs[CBS_SR_CLASS_B].low_credit = CBS_MAX_NEGATIVE_CREDIT;
    config->queue_configs[CBS_SR_CLASS_B].high_credit = 0;
    config->queue_configs[CBS_SR_CLASS_B].transmission_overrun_us = CBS_TRANSMISSION_OVERRUN_US;
    
    return ETH_OK;
}

eth_status_t cbs_calc_required_credit(uint32_t frame_size,
                                       int64_t send_slope,
                                       int64_t *required_credit) {
    if (required_credit == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 所需信用 = 帧大小(bits) */
    *required_credit = (int64_t)frame_size * 8;
    
    (void)send_slope; /* 在基础实现中未使用 */
    
    return ETH_OK;
}

eth_status_t cbs_print_status(uint16_t port_id) {
    if (!g_cbs_state.initialized || port_id >= CBS_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    cbs_port_runtime_t *port = get_port(port_id);
    
    printf("=== CBS Port %d Status ===\n", port_id);
    printf("Configured: %s\n", port->configured ? "Yes" : "No");
    printf("Port Transmit Rate: %u bps\n", port->config.port_transmit_rate_bps);
    printf("\nQueue States:\n");
    
    for (int i = 0; i < CBS_SR_CLASS_MAX; i++) {
        cbs_queue_state_t *queue = &port->queue_states[i];
        printf("  Queue %d (SR Class %c):\n", i, (i == 0) ? 'A' : 'B');
        printf("    Current Credit: %ld\n", (long)queue->current_credit);
        printf("    Min/Max Credit: %ld / %ld\n", 
               (long)queue->min_credit_seen, (long)queue->max_credit_seen);
        printf("    Frames TX: %lu\n", (unsigned long)queue->frames_transmitted);
        printf("    Bytes TX: %lu\n", (unsigned long)queue->bytes_transmitted);
        printf("    Credit Violations: %lu\n", (unsigned long)queue->credit_violations);
    }
    
    printf("========================\n");
    
    return ETH_OK;
}

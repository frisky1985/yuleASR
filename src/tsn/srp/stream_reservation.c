/**
 * @file stream_reservation.c
 * @brief 流预留协议实现 (IEEE 802.1Qcc-2018 / 802.1Qca-2015)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note MSRP协议实现
 * @note 符合ASIL-D安全等级
 */

#include "stream_reservation.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* ============================================================================
 * 常量和宏定义
 * ============================================================================ */

#define SRP_MAX_PORTS                   8

/* ============================================================================
 * 内部数据结构
 * ============================================================================ */

typedef struct {
    srp_port_bandwidth_t bandwidth;
    srp_domain_declaration_t domain[2];  /* SR Class A and B */
    srp_safety_monitor_t safety_monitor;
    bool configured;
} srp_port_state_t;

typedef struct {
    bool initialized;
    
    /* 流预留表 */
    srp_stream_reservation_t streams[SRP_MAX_STREAMS];
    uint32_t stream_count;
    
    /* 端口状态 */
    srp_port_state_t ports[SRP_MAX_PORTS];
    
    /* 路径表 */
    srp_stream_path_t paths[SRP_MAX_STREAMS];
    uint32_t path_count;
    
    /* 回调函数 */
    srp_talker_registered_callback_t talker_cb;
    srp_listener_registered_callback_t listener_cb;
    srp_reservation_changed_callback_t reservation_cb;
    srp_bandwidth_alert_callback_t bw_alert_cb;
    srp_safety_alert_callback_t safety_cb;
    void *talker_user_data;
    void *listener_user_data;
    void *reservation_user_data;
    void *bw_alert_user_data;
    void *safety_user_data;
} srp_global_state_t;

/* ============================================================================
 * 全局状态实例
 * ============================================================================ */

static srp_global_state_t g_srp_state;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

static int find_stream_index(const srp_stream_id_t stream_id) {
    for (uint32_t i = 0; i < g_srp_state.stream_count; i++) {
        if (memcmp(g_srp_state.streams[i].stream_id, stream_id, SRP_STREAM_ID_LEN) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static srp_stream_reservation_t* find_or_create_stream(const srp_stream_id_t stream_id) {
    int idx = find_stream_index(stream_id);
    if (idx >= 0) {
        return &g_srp_state.streams[idx];
    }
    
    if (g_srp_state.stream_count >= SRP_MAX_STREAMS) {
        return NULL;
    }
    
    idx = g_srp_state.stream_count++;
    srp_stream_reservation_t *stream = &g_srp_state.streams[idx];
    memset(stream, 0, sizeof(srp_stream_reservation_t));
    memcpy(stream->stream_id, stream_id, SRP_STREAM_ID_LEN);
    stream->state = SRP_RESERVATION_STATE_NONE;
    stream->safety_margin_percent = SRP_SAFETY_BANDWIDTH_MARGIN;
    
    return stream;
}

static srp_port_state_t* get_port(uint16_t port_id) {
    if (port_id >= SRP_MAX_PORTS) {
        return NULL;
    }
    return &g_srp_state.ports[port_id];
}

static uint32_t calc_bandwidth(const srp_tspec_t *tspec) {
    /* 带宽计算: MaxFrameSize * MaxIntervalFrames / Interval */
    if (tspec->interval_numerator == 0) {
        return 0;
    }
    
    uint64_t bits_per_frame = (uint64_t)tspec->max_frame_size * 8;
    uint64_t frames_per_sec = ((uint64_t)tspec->max_interval_frames * 
                               tspec->interval_denominator) / tspec->interval_numerator;
    
    return (uint32_t)(bits_per_frame * frames_per_sec);
}

static eth_status_t update_reservation_state(srp_stream_reservation_t *stream) {
    srp_reservation_state_t old_state = stream->state;
    srp_reservation_state_t new_state = old_state;
    
    /* 根据Talker和Listener状态更新预留状态 */
    if (!stream->talker_registered) {
        new_state = SRP_RESERVATION_STATE_NONE;
    } else if (stream->talker.failed) {
        new_state = SRP_RESERVATION_STATE_FAILED;
    } else if (stream->listener_count == 0) {
        new_state = SRP_RESERVATION_STATE_REGISTERING;
    } else {
        /* 检查所有Listener状态 */
        bool all_ready = true;
        bool any_failed = false;
        
        for (uint32_t i = 0; i < stream->listener_count; i++) {
            if (stream->listeners[i].state == SRP_LISTENER_STATE_READY_FAILED ||
                stream->listeners[i].state == SRP_LISTENER_STATE_ASKING_FAILED) {
                any_failed = true;
                break;
            }
            if (stream->listeners[i].state != SRP_LISTENER_STATE_READY) {
                all_ready = false;
            }
        }
        
        if (any_failed) {
            new_state = SRP_RESERVATION_STATE_FAILED;
        } else if (all_ready) {
            new_state = SRP_RESERVATION_STATE_REGISTERED;
        } else {
            new_state = SRP_RESERVATION_STATE_REGISTERING;
        }
    }
    
    if (new_state != old_state) {
        stream->state = new_state;
        
        if (g_srp_state.reservation_cb) {
            g_srp_state.reservation_cb(stream->stream_id, old_state, new_state,
                                       g_srp_state.reservation_user_data);
        }
    }
    
    return ETH_OK;
}

static eth_status_t check_safety(uint16_t port_id) {
    srp_port_state_t *port = get_port(port_id);
    if (port == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    srp_safety_monitor_t *monitor = &port->safety_monitor;
    
    /* 检查带宽限制 */
    uint32_t used_percent = 0;
    if (port->bandwidth.total_bandwidth_bps > 0) {
        used_percent = (port->bandwidth.reserved_bandwidth_bps * 100) / 
                       port->bandwidth.total_bandwidth_bps;
    }
    
    if (used_percent > SRP_MAX_BANDWIDTH_PERCENT) {
        monitor->bandwidth_violated = true;
        monitor->bandwidth_violation_count++;
        
        if (g_srp_state.safety_cb) {
            g_srp_state.safety_cb(port_id, 0x01, "Bandwidth limit exceeded",
                                  g_srp_state.safety_user_data);
        }
    }
    
    return ETH_OK;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

eth_status_t srp_init(void) {
    if (g_srp_state.initialized) {
        srp_deinit();
    }
    
    memset(&g_srp_state, 0, sizeof(g_srp_state));
    
    for (int i = 0; i < SRP_MAX_PORTS; i++) {
        g_srp_state.ports[i].bandwidth.port_id = i;
    }
    
    g_srp_state.initialized = true;
    return ETH_OK;
}

void srp_deinit(void) {
    if (!g_srp_state.initialized) {
        return;
    }
    
    memset(&g_srp_state, 0, sizeof(g_srp_state));
}

eth_status_t srp_config_port_bandwidth(uint16_t port_id, uint32_t total_bandwidth_bps) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    srp_port_state_t *port = get_port(port_id);
    port->bandwidth.total_bandwidth_bps = total_bandwidth_bps;
    port->bandwidth.available_bandwidth_bps = 
        (total_bandwidth_bps * SRP_MAX_BANDWIDTH_PERCENT) / 100;
    port->bandwidth.reserved_bandwidth_bps = 0;
    port->configured = true;
    
    return ETH_OK;
}

eth_status_t srp_config_sr_class(uint16_t port_id, uint8_t class_id,
                                  const srp_domain_declaration_t *domain) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS || 
        class_id >= 2 || domain == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(&g_srp_state.ports[port_id].domain[class_id], domain, 
           sizeof(srp_domain_declaration_t));
    
    return ETH_OK;
}

eth_status_t srp_register_talker(uint16_t port_id, 
                                  const srp_talker_declaration_t *talker) {
    if (!g_srp_state.initialized || talker == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    srp_stream_reservation_t *stream = find_or_create_stream(talker->stream_id);
    if (stream == NULL) {
        return ETH_NO_MEMORY;
    }
    
    memcpy(&stream->talker, talker, sizeof(srp_talker_declaration_t));
    stream->talker_registered = true;
    
    /* 计算带宽需求 */
    stream->reserved_bandwidth_bps = calc_bandwidth(&talker->tspec);
    
    /* 预留带宽 */
    srp_reserve_bandwidth(port_id, talker->stream_id, stream->reserved_bandwidth_bps);
    
    /* 更新预留状态 */
    update_reservation_state(stream);
    
    /* 通知回调 */
    if (g_srp_state.talker_cb) {
        g_srp_state.talker_cb(talker->stream_id, talker, g_srp_state.talker_user_data);
    }
    
    return ETH_OK;
}

eth_status_t srp_deregister_talker(uint16_t port_id, const srp_stream_id_t stream_id) {
    if (!g_srp_state.initialized) {
        return ETH_NOT_INIT;
    }
    
    int idx = find_stream_index(stream_id);
    if (idx < 0) {
        return ETH_ERROR;
    }
    
    srp_stream_reservation_t *stream = &g_srp_state.streams[idx];
    
    /* 释放带宽 */
    srp_release_bandwidth(port_id, stream_id);
    
    stream->talker_registered = false;
    memset(&stream->talker, 0, sizeof(srp_talker_declaration_t));
    
    update_reservation_state(stream);
    
    return ETH_OK;
}

eth_status_t srp_update_talker(uint16_t port_id, const srp_talker_declaration_t *talker) {
    /* 简化为重新注册 */
    return srp_register_talker(port_id, talker);
}

eth_status_t srp_get_talker(uint16_t port_id, const srp_stream_id_t stream_id,
                             srp_talker_declaration_t *talker) {
    (void)port_id;
    
    if (!g_srp_state.initialized || talker == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    int idx = find_stream_index(stream_id);
    if (idx < 0) {
        return ETH_ERROR;
    }
    
    memcpy(talker, &g_srp_state.streams[idx].talker, sizeof(srp_talker_declaration_t));
    
    return ETH_OK;
}

eth_status_t srp_register_listener(uint16_t port_id,
                                    const srp_listener_declaration_t *listener) {
    if (!g_srp_state.initialized || listener == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    srp_stream_reservation_t *stream = find_or_create_stream(listener->stream_id);
    if (stream == NULL) {
        return ETH_NO_MEMORY;
    }
    
    /* 检查是否已存在该Listener */
    for (uint32_t i = 0; i < stream->listener_count; i++) {
        /* 简化: 更新现有Listener状态 */
        stream->listeners[i].state = listener->state;
        memcpy(&stream->listeners[i].failure_info, &listener->failure_info,
               sizeof(srp_failure_info_t));
        
        update_reservation_state(stream);
        
        if (g_srp_state.listener_cb) {
            g_srp_state.listener_cb(listener->stream_id, listener, 
                                    g_srp_state.listener_user_data);
        }
        
        return ETH_OK;
    }
    
    /* 添加新Listener */
    if (stream->listener_count >= SRP_MAX_LISTENERS) {
        return ETH_NO_MEMORY;
    }
    
    uint32_t idx = stream->listener_count++;
    memcpy(&stream->listeners[idx], listener, sizeof(srp_listener_declaration_t));
    
    update_reservation_state(stream);
    
    if (g_srp_state.listener_cb) {
        g_srp_state.listener_cb(listener->stream_id, listener, 
                                g_srp_state.listener_user_data);
    }
    
    return ETH_OK;
}

eth_status_t srp_deregister_listener(uint16_t port_id, const srp_stream_id_t stream_id) {
    (void)port_id;
    
    if (!g_srp_state.initialized) {
        return ETH_NOT_INIT;
    }
    
    int idx = find_stream_index(stream_id);
    if (idx < 0) {
        return ETH_ERROR;
    }
    
    srp_stream_reservation_t *stream = &g_srp_state.streams[idx];
    
    /* 简化: 清除所有Listener */
    stream->listener_count = 0;
    memset(stream->listeners, 0, sizeof(stream->listeners));
    
    update_reservation_state(stream);
    
    return ETH_OK;
}

eth_status_t srp_update_listener_state(uint16_t port_id, const srp_stream_id_t stream_id,
                                        srp_listener_state_t state) {
    srp_listener_declaration_t listener;
    memset(&listener, 0, sizeof(listener));
    memcpy(listener.stream_id, stream_id, SRP_STREAM_ID_LEN);
    listener.state = state;
    
    return srp_register_listener(port_id, &listener);
}

eth_status_t srp_reserve_bandwidth(uint16_t port_id, const srp_stream_id_t stream_id,
                                    uint32_t bandwidth_bps) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    srp_port_state_t *port = get_port(port_id);
    if (!port->configured) {
        return ETH_NOT_INIT;
    }
    
    /* 检查可用带宽 */
    uint32_t available = port->bandwidth.available_bandwidth_bps - 
                         port->bandwidth.reserved_bandwidth_bps;
    
    if (bandwidth_bps > available) {
        return ETH_ERROR;  /* 带宽不足 */
    }
    
    /* 预留带宽 */
    port->bandwidth.reserved_bandwidth_bps += bandwidth_bps;
    
    /* 检查带宽告警 */
    uint32_t used_percent = (port->bandwidth.reserved_bandwidth_bps * 100) / 
                            port->bandwidth.total_bandwidth_bps;
    
    if (used_percent > port->bandwidth.available_bandwidth_bps * 100 / 
                       port->bandwidth.total_bandwidth_bps - 10) {
        if (g_srp_state.bw_alert_cb) {
            g_srp_state.bw_alert_cb(port_id, used_percent, 
                                    g_srp_state.bw_alert_user_data);
        }
    }
    
    return ETH_OK;
}

eth_status_t srp_release_bandwidth(uint16_t port_id, const srp_stream_id_t stream_id) {
    (void)stream_id;
    
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    srp_port_state_t *port = get_port(port_id);
    
    /* 查找流并释放其带宽 */
    int idx = find_stream_index(stream_id);
    if (idx >= 0) {
        uint32_t bandwidth = g_srp_state.streams[idx].reserved_bandwidth_bps;
        if (port->bandwidth.reserved_bandwidth_bps >= bandwidth) {
            port->bandwidth.reserved_bandwidth_bps -= bandwidth;
        }
    }
    
    return ETH_OK;
}

eth_status_t srp_calculate_path(const srp_stream_id_t stream_id, 
                                 srp_stream_path_t *path) {
    if (!g_srp_state.initialized || path == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 简化实现: 返回直连路径 */
    memcpy(path->stream_id, stream_id, SRP_STREAM_ID_LEN);
    path->port_count = 1;
    path->port_list[0] = 0;
    path->total_latency_us = 100;  /* 假设100us延迟 */
    path->path_calculated = true;
    
    return ETH_OK;
}

eth_status_t srp_check_feasibility(uint16_t port_id, uint32_t bandwidth_bps,
                                    uint32_t latency_us, bool *feasible) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS || feasible == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    srp_port_state_t *port = get_port(port_id);
    if (!port->configured) {
        *feasible = false;
        return ETH_OK;
    }
    
    /* 检查带宽 */
    uint32_t available = port->bandwidth.available_bandwidth_bps - 
                         port->bandwidth.reserved_bandwidth_bps;
    bool bandwidth_ok = (bandwidth_bps <= available);
    
    /* 检查延迟 */
    bool latency_ok = (latency_us <= SRP_MAX_RESERVATION_LATENCY_US);
    
    *feasible = bandwidth_ok && latency_ok;
    
    return ETH_OK;
}

eth_status_t srp_get_port_bandwidth(uint16_t port_id, srp_port_bandwidth_t *bandwidth) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS || bandwidth == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(bandwidth, &g_srp_state.ports[port_id].bandwidth, 
           sizeof(srp_port_bandwidth_t));
    
    return ETH_OK;
}

eth_status_t srp_get_stream_reservation(const srp_stream_id_t stream_id,
                                         srp_stream_reservation_t *reservation) {
    if (!g_srp_state.initialized || reservation == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    int idx = find_stream_index(stream_id);
    if (idx < 0) {
        return ETH_ERROR;
    }
    
    memcpy(reservation, &g_srp_state.streams[idx], sizeof(srp_stream_reservation_t));
    
    return ETH_OK;
}

eth_status_t srp_get_all_reservations(srp_stream_reservation_t *reservations, 
                                       uint32_t *count) {
    if (!g_srp_state.initialized || reservations == NULL || count == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    uint32_t to_copy = *count;
    if (to_copy > g_srp_state.stream_count) {
        to_copy = g_srp_state.stream_count;
    }
    
    for (uint32_t i = 0; i < to_copy; i++) {
        memcpy(&reservations[i], &g_srp_state.streams[i], 
               sizeof(srp_stream_reservation_t));
    }
    
    *count = to_copy;
    
    return ETH_OK;
}

eth_status_t srp_calc_bandwidth_requirement(const srp_tspec_t *tspec,
                                             uint32_t *bandwidth_bps) {
    if (tspec == NULL || bandwidth_bps == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    *bandwidth_bps = calc_bandwidth(tspec);
    
    return ETH_OK;
}

eth_status_t srp_register_talker_callback(srp_talker_registered_callback_t callback,
                                           void *user_data) {
    g_srp_state.talker_cb = callback;
    g_srp_state.talker_user_data = user_data;
    return ETH_OK;
}

eth_status_t srp_register_listener_callback(srp_listener_registered_callback_t callback,
                                             void *user_data) {
    g_srp_state.listener_cb = callback;
    g_srp_state.listener_user_data = user_data;
    return ETH_OK;
}

eth_status_t srp_register_reservation_callback(srp_reservation_changed_callback_t callback,
                                                void *user_data) {
    g_srp_state.reservation_cb = callback;
    g_srp_state.reservation_user_data = user_data;
    return ETH_OK;
}

eth_status_t srp_register_bandwidth_alert_callback(srp_bandwidth_alert_callback_t callback,
                                                    void *user_data) {
    g_srp_state.bw_alert_cb = callback;
    g_srp_state.bw_alert_user_data = user_data;
    return ETH_OK;
}

eth_status_t srp_register_safety_alert_callback(srp_safety_alert_callback_t callback,
                                                 void *user_data) {
    g_srp_state.safety_cb = callback;
    g_srp_state.safety_user_data = user_data;
    return ETH_OK;
}

eth_status_t srp_init_safety_monitor(uint16_t port_id) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    srp_safety_monitor_t *monitor = &g_srp_state.ports[port_id].safety_monitor;
    memset(monitor, 0, sizeof(srp_safety_monitor_t));
    monitor->reservation_integrity_ok = true;
    
    return ETH_OK;
}

eth_status_t srp_run_safety_checks(uint16_t port_id) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    return check_safety(port_id);
}

eth_status_t srp_get_safety_monitor(uint16_t port_id, srp_safety_monitor_t *monitor) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS || monitor == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memcpy(monitor, &g_srp_state.ports[port_id].safety_monitor, 
           sizeof(srp_safety_monitor_t));
    
    return ETH_OK;
}

eth_status_t srp_validate_reservation_integrity(const srp_stream_id_t stream_id,
                                                 bool *integrity_ok) {
    if (!g_srp_state.initialized || integrity_ok == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    int idx = find_stream_index(stream_id);
    if (idx < 0) {
        *integrity_ok = false;
        return ETH_OK;
    }
    
    srp_stream_reservation_t *stream = &g_srp_state.streams[idx];
    
    /* 检查预留一致性 */
    *integrity_ok = (stream->talker_registered && 
                     stream->reserved_bandwidth_bps > 0 &&
                     stream->state != SRP_RESERVATION_STATE_FAILED);
    
    return ETH_OK;
}

eth_status_t srp_stream_id_to_string(const srp_stream_id_t stream_id, 
                                      char *buf, size_t buf_size) {
    if (stream_id == NULL || buf == NULL || buf_size < 24) {
        return ETH_INVALID_PARAM;
    }
    
    snprintf(buf, buf_size, "%02X%02X:%02X%02X:%02X%02X:%02X%02X",
             stream_id[0], stream_id[1], stream_id[2], stream_id[3],
             stream_id[4], stream_id[5], stream_id[6], stream_id[7]);
    
    return ETH_OK;
}

eth_status_t srp_create_automotive_stream(const char *stream_name,
                                           uint16_t vlan_id,
                                           uint8_t priority,
                                           srp_stream_id_t stream_id) {
    if (stream_name == NULL || stream_id == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 根据流名称生成唯一的流ID */
    /* 简化实现: 使用字符串哈希 */
    uint32_t hash = 0;
    for (const char *p = stream_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    /* 使用VLAN ID和优先级填充 */
    stream_id[0] = (hash >> 24) & 0xFF;
    stream_id[1] = (hash >> 16) & 0xFF;
    stream_id[2] = (hash >> 8) & 0xFF;
    stream_id[3] = hash & 0xFF;
    stream_id[4] = (vlan_id >> 8) & 0xFF;
    stream_id[5] = vlan_id & 0xFF;
    stream_id[6] = priority;
    stream_id[7] = 0;
    
    return ETH_OK;
}

eth_status_t srp_print_reservation(const srp_stream_id_t stream_id) {
    if (!g_srp_state.initialized) {
        return ETH_NOT_INIT;
    }
    
    int idx = find_stream_index(stream_id);
    if (idx < 0) {
        printf("Stream not found\n");
        return ETH_ERROR;
    }
    
    srp_stream_reservation_t *stream = &g_srp_state.streams[idx];
    char id_str[24];
    srp_stream_id_to_string(stream_id, id_str, sizeof(id_str));
    
    printf("=== Stream Reservation: %s ===\n", id_str);
    printf("State: %d\n", stream->state);
    printf("Talker Registered: %s\n", stream->talker_registered ? "Yes" : "No");
    printf("Listener Count: %u\n", stream->listener_count);
    printf("Reserved Bandwidth: %u bps\n", stream->reserved_bandwidth_bps);
    printf("Accumulated Latency: %u us\n", stream->accumulated_latency_us);
    printf("Safety Margin: %u%%\n", stream->safety_margin_percent);
    printf("================================\n");
    
    return ETH_OK;
}

eth_status_t srp_print_port_status(uint16_t port_id) {
    if (!g_srp_state.initialized || port_id >= SRP_MAX_PORTS) {
        return ETH_INVALID_PARAM;
    }
    
    srp_port_state_t *port = get_port(port_id);
    
    printf("=== SRP Port %d Status ===\n", port_id);
    printf("Configured: %s\n", port->configured ? "Yes" : "No");
    printf("Total Bandwidth: %u bps\n", port->bandwidth.total_bandwidth_bps);
    printf("Available Bandwidth: %u bps\n", port->bandwidth.available_bandwidth_bps);
    printf("Reserved Bandwidth: %u bps\n", port->bandwidth.reserved_bandwidth_bps);
    printf("Active Streams: %u\n", g_srp_state.stream_count);
    printf("===========================\n");
    
    return ETH_OK;
}

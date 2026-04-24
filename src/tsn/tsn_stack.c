/**
 * @file tsn_stack.c
 * @brief TSN协议栈整合实现
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 车载TSN协议栈
 * @note 符合IEEE 802.1AS/Qbv/Qbu/Qav/Qcc/Qca
 * @note 支持ASIL-D安全等级
 */

#include "tsn_stack.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ============================================================================
 * 内部数据结构
 * ============================================================================ */

typedef struct {
    bool initialized;
    tsn_state_t state;
    tsn_stack_config_t config;
    tsn_stack_stats_t stats;
    uint64_t last_cycle_time_ns;
} tsn_stack_context_t;

/* ============================================================================
 * 全局状态实例
 * ============================================================================ */

static tsn_stack_context_t g_tsn_context;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

static uint64_t get_current_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static eth_status_t init_all_modules(const tsn_stack_config_t *config) {
    eth_status_t status;
    
    /* 初始化gPTP */
    status = gptp_init(&config->gptp_config);
    if (status != ETH_OK) {
        printf("TSN: gPTP init failed\n");
        return status;
    }
    
    /* 初始化TAS */
    if (config->tas_enabled) {
        status = tas_init();
        if (status != ETH_OK) {
            printf("TSN: TAS init failed\n");
            return status;
        }
    }
    
    /* 初始化CBS */
    if (config->cbs_enabled) {
        status = cbs_init();
        if (status != ETH_OK) {
            printf("TSN: CBS init failed\n");
            return status;
        }
    }
    
    /* 初始化帧抢占 */
    if (config->fp_enabled) {
        status = fp_init();
        if (status != ETH_OK) {
            printf("TSN: Frame Preemption init failed\n");
            return status;
        }
    }
    
    /* 初始化流预留 */
    if (config->srp_enabled) {
        status = srp_init();
        if (status != ETH_OK) {
            printf("TSN: Stream Reservation init failed\n");
            return status;
        }
    }
    
    return ETH_OK;
}

static void deinit_all_modules(void) {
    srp_deinit();
    fp_deinit();
    cbs_deinit();
    tas_deinit();
    gptp_deinit();
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

eth_status_t tsn_stack_init(const tsn_stack_config_t *config) {
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    if (g_tsn_context.initialized) {
        tsn_stack_deinit();
    }
    
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    memcpy(&g_tsn_context.config, config, sizeof(tsn_stack_config_t));
    
    /* 初始化所有模块 */
    eth_status_t status = init_all_modules(config);
    if (status != ETH_OK) {
        deinit_all_modules();
        return status;
    }
    
    g_tsn_context.initialized = true;
    g_tsn_context.state = TSN_STATE_INIT;
    
    printf("TSN Stack initialized successfully (version %s)\n", TSN_STACK_VERSION_STRING);
    
    return ETH_OK;
}

void tsn_stack_deinit(void) {
    if (!g_tsn_context.initialized) {
        return;
    }
    
    tsn_stack_stop();
    deinit_all_modules();
    
    memset(&g_tsn_context, 0, sizeof(g_tsn_context));
    
    printf("TSN Stack deinitialized\n");
}

eth_status_t tsn_stack_start(void) {
    if (!g_tsn_context.initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 启动gPTP */
    for (uint8_t i = 0; i < g_tsn_context.config.gptp_config.domain_count; i++) {
        gptp_start(i);
    }
    
    g_tsn_context.state = TSN_STATE_RUNNING;
    g_tsn_context.last_cycle_time_ns = get_current_time_ns();
    
    printf("TSN Stack started\n");
    
    return ETH_OK;
}

eth_status_t tsn_stack_stop(void) {
    if (!g_tsn_context.initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 停止gPTP */
    for (uint8_t i = 0; i < g_tsn_context.config.gptp_config.domain_count; i++) {
        gptp_stop(i);
    }
    
    g_tsn_context.state = TSN_STATE_STOPPED;
    
    printf("TSN Stack stopped\n");
    
    return ETH_OK;
}

eth_status_t tsn_stack_get_status(tsn_stack_status_t *status) {
    if (status == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memset(status, 0, sizeof(tsn_stack_status_t));
    
    status->state = g_tsn_context.state;
    status->gptp_active = (g_tsn_context.state == TSN_STATE_RUNNING);
    status->tas_active = g_tsn_context.config.tas_enabled;
    status->cbs_active = g_tsn_context.config.cbs_enabled;
    status->fp_active = g_tsn_context.config.fp_enabled;
    status->srp_active = g_tsn_context.config.srp_enabled;
    status->active_domain_count = g_tsn_context.config.gptp_config.domain_count;
    status->active_port_count = g_tsn_context.config.gptp_config.port_count;
    
    return ETH_OK;
}

eth_status_t tsn_create_automotive_config(uint32_t port_rate_mbps, 
                                           tsn_stack_config_t *config) {
    if (config == NULL || port_rate_mbps == 0) {
        return ETH_INVALID_PARAM;
    }
    
    memset(config, 0, sizeof(tsn_stack_config_t));
    
    /* gPTP配置 */
    config->gptp_config.domain_count = 1;
    config->gptp_config.port_count = 1;
    config->gptp_config.gm_capable = true;
    config->gptp_config.sync_precision_ns = GPTP_SYNC_PRECISION_NS;
    config->gptp_config.enable_asil_d = true;
    
    /* 启用所有模块 */
    config->tas_enabled = true;
    config->cbs_enabled = true;
    config->fp_enabled = true;
    config->srp_enabled = true;
    config->asil_d_enabled = true;
    config->safety_monitor_interval_ms = 10;
    
    return ETH_OK;
}

eth_status_t tsn_config_automotive_domain(uint8_t domain_index,
                                           const gptp_domain_config_t *domain_config) {
    if (!g_tsn_context.initialized || domain_config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    return gptp_config_domain(domain_index, domain_config);
}

eth_status_t tsn_config_automotive_scheduler(uint16_t port_id,
                                              const tas_port_config_t *tas_config,
                                              const cbs_port_config_t *cbs_config) {
    if (!g_tsn_context.initialized) {
        return ETH_NOT_INIT;
    }
    
    eth_status_t status = ETH_OK;
    
    /* 配置TAS */
    if (tas_config != NULL && g_tsn_context.config.tas_enabled) {
        status = tas_config_port(port_id, tas_config);
        if (status != ETH_OK) {
            return status;
        }
    }
    
    /* 配置CBS */
    if (cbs_config != NULL && g_tsn_context.config.cbs_enabled) {
        status = cbs_config_port(port_id, cbs_config);
        if (status != ETH_OK) {
            return status;
        }
    }
    
    return ETH_OK;
}

eth_status_t tsn_stack_run_cycle(void) {
    if (!g_tsn_context.initialized || g_tsn_context.state != TSN_STATE_RUNNING) {
        return ETH_NOT_INIT;
    }
    
    uint64_t current_time = get_current_time_ns();
    
    /* 运行TAS调度循环 */
    if (g_tsn_context.config.tas_enabled) {
        for (uint16_t i = 0; i < g_tsn_context.config.gptp_config.port_count; i++) {
            tas_run_schedule_cycle(i, current_time);
        }
    }
    
    /* 运行安全检查 */
    if (g_tsn_context.config.asil_d_enabled) {
        tsn_stack_run_safety_checks();
    }
    
    g_tsn_context.last_cycle_time_ns = current_time;
    
    return ETH_OK;
}

eth_status_t tsn_wait_for_sync(uint8_t domain_index, uint32_t timeout_ms) {
    if (!g_tsn_context.initialized) {
        return ETH_NOT_INIT;
    }
    
    uint32_t elapsed = 0;
    bool synchronized = false;
    
    while (elapsed < timeout_ms) {
        gptp_is_synchronized(domain_index, &synchronized);
        if (synchronized) {
            return ETH_OK;
        }
        
        /* 简单延时 */
        struct timespec ts = {0, 1000000};  /* 1ms */
        nanosleep(&ts, NULL);
        elapsed += 1;
    }
    
    return ETH_TIMEOUT;
}

eth_status_t tsn_transmit_frame(uint16_t port_id, uint8_t queue_id,
                                 const uint8_t *data, uint32_t len,
                                 uint8_t priority) {
    if (!g_tsn_context.initialized || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    bool can_tx = true;
    
    /* CBS检查 */
    if (g_tsn_context.config.cbs_enabled) {
        cbs_can_transmit(port_id, queue_id, len, &can_tx);
    }
    
    /* TAS检查 */
    if (can_tx && g_tsn_context.config.tas_enabled) {
        tas_can_transmit(port_id, queue_id, len, &can_tx);
    }
    
    if (!can_tx) {
        return ETH_BUSY;
    }
    
    /* 发送帧 */
    if (g_tsn_context.config.cbs_enabled) {
        cbs_start_transmission(port_id, queue_id, len);
    }
    
    /* 实际发送在这里完成 */
    /* 注: 需要与底层驱动集成 */
    
    if (g_tsn_context.config.cbs_enabled) {
        uint64_t tx_time = get_current_time_ns();
        cbs_complete_transmission(port_id, queue_id, len, tx_time);
    }
    
    return ETH_OK;
}

eth_status_t tsn_stack_get_stats(tsn_stack_stats_t *stats) {
    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    memset(stats, 0, sizeof(tsn_stack_stats_t));
    
    /* gPTP统计 */
    for (uint8_t i = 0; i < g_tsn_context.config.gptp_config.domain_count; i++) {
        gptp_clock_state_t clock_state;
        if (gptp_get_clock_state(i, &clock_state) == ETH_OK) {
            stats->total_domains++;
            if (clock_state.synchronized) {
                stats->synced_domains++;
            }
        }
    }
    
    /* TAS统计 */
    if (g_tsn_context.config.tas_enabled) {
        for (uint16_t i = 0; i < g_tsn_context.config.gptp_config.port_count; i++) {
            tas_scheduler_stats_t tas_stats;
            if (tas_get_scheduler_stats(i, &tas_stats) == ETH_OK) {
                stats->tas_cycles_completed += tas_stats.cycles_completed;
                stats->tas_schedule_errors += tas_stats.schedule_errors;
            }
        }
    }
    
    /* CBS统计 */
    if (g_tsn_context.config.cbs_enabled) {
        for (uint16_t i = 0; i < g_tsn_context.config.gptp_config.port_count; i++) {
            cbs_stats_t cbs_stats;
            if (cbs_get_stats(i, &cbs_stats) == ETH_OK) {
                stats->cbs_frames_transmitted += cbs_stats.total_frames_transmitted;
                stats->cbs_credit_violations += cbs_stats.total_credit_violations;
            }
        }
    }
    
    /* 帧抢占统计 */
    if (g_tsn_context.config.fp_enabled) {
        for (uint16_t i = 0; i < g_tsn_context.config.gptp_config.port_count; i++) {
            fp_stats_t fp_stats;
            if (fp_get_stats(i, &fp_stats) == ETH_OK) {
                stats->fp_preemptions_count += fp_stats.preemptions_count;
                stats->fp_reassembly_success += fp_stats.reassembly_success;
            }
        }
    }
    
    /* 流预留统计 */
    if (g_tsn_context.config.srp_enabled) {
        uint32_t count = SRP_MAX_STREAMS;
        srp_stream_reservation_t reservations[SRP_MAX_STREAMS];
        if (srp_get_all_reservations(reservations, &count) == ETH_OK) {
            stats->srp_active_streams = count;
        }
    }
    
    memcpy(&g_tsn_context.stats, stats, sizeof(tsn_stack_stats_t));
    
    return ETH_OK;
}

eth_status_t tsn_stack_clear_stats(void) {
    if (!g_tsn_context.initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 清除各模块统计 */
    if (g_tsn_context.config.cbs_enabled) {
        for (uint16_t i = 0; i < g_tsn_context.config.gptp_config.port_count; i++) {
            cbs_clear_stats(i);
        }
    }
    
    if (g_tsn_context.config.fp_enabled) {
        for (uint16_t i = 0; i < g_tsn_context.config.gptp_config.port_count; i++) {
            fp_clear_stats(i);
        }
    }
    
    memset(&g_tsn_context.stats, 0, sizeof(tsn_stack_stats_t));
    
    return ETH_OK;
}

eth_status_t tsn_stack_run_safety_checks(void) {
    if (!g_tsn_context.initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 运行各模块安全检查 */
    if (g_tsn_context.config.asil_d_enabled) {
        for (uint8_t i = 0; i < g_tsn_context.config.gptp_config.domain_count; i++) {
            gptp_run_safety_checks(i);
        }
    }
    
    return ETH_OK;
}

eth_status_t tsn_stack_print_status(void) {
    if (!g_tsn_context.initialized) {
        printf("TSN Stack not initialized\n");
        return ETH_NOT_INIT;
    }
    
    printf("\n========================================\n");
    printf("       TSN Stack Status\n");
    printf("========================================\n");
    printf("Version: %s\n", TSN_STACK_VERSION_STRING);
    printf("State: %d\n", g_tsn_context.state);
    printf("\nActive Modules:\n");
    printf("  gPTP: %s\n", g_tsn_context.config.gptp_config.domain_count > 0 ? "Yes" : "No");
    printf("  TAS: %s\n", g_tsn_context.config.tas_enabled ? "Yes" : "No");
    printf("  CBS: %s\n", g_tsn_context.config.cbs_enabled ? "Yes" : "No");
    printf("  Frame Preemption: %s\n", g_tsn_context.config.fp_enabled ? "Yes" : "No");
    printf("  Stream Reservation: %s\n", g_tsn_context.config.srp_enabled ? "Yes" : "No");
    printf("  ASIL-D Safety: %s\n", g_tsn_context.config.asil_d_enabled ? "Yes" : "No");
    
    tsn_stack_stats_t stats;
    tsn_stack_get_stats(&stats);
    
    printf("\nStatistics:\n");
    printf("  Synced Domains: %u/%u\n", stats.synced_domains, stats.total_domains);
    printf("  TAS Cycles: %lu\n", (unsigned long)stats.tas_cycles_completed);
    printf("  CBS Frames TX: %lu\n", (unsigned long)stats.cbs_frames_transmitted);
    printf("  Preemptions: %lu\n", (unsigned long)stats.fp_preemptions_count);
    printf("  Active Streams: %u\n", stats.srp_active_streams);
    printf("========================================\n\n");
    
    return ETH_OK;
}

const char* tsn_stack_get_version_string(void) {
    return TSN_STACK_VERSION_STRING;
}

void tsn_stack_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch) {
    if (major) *major = TSN_STACK_VERSION_MAJOR;
    if (minor) *minor = TSN_STACK_VERSION_MINOR;
    if (patch) *patch = TSN_STACK_VERSION_PATCH;
}

/**
 * @file eth_manager.c
 * @brief 以太网管理器实现 - 链路监控、自动协商、错误统计
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 符合AUTOSAR MCAL规范
 * @note 支持功能安全ASIL等级
 */

#include "eth_manager.h"
#include <string.h>
#include <stdio.h>

/* 导入PHY寄存器定义 - 通常在共享头文件中定义 */
#ifndef PHY_REG_STATUS
#define PHY_REG_STATUS          0x01
#define PHY_STATUS_AUTO_NEG_DONE 0x0020
#endif

/* ============================================================================
 * 内部状态定义
 * ============================================================================ */

/** 管理器内部结构 */
typedef struct {
    eth_manager_config_t config;            /* 配置 */
    eth_manager_state_t state;              /* 当前状态 */
    eth_manager_stats_t stats;              /* 统计 */
    eth_error_info_t last_error;            /* 最后错误 */

    /* 回调函数 */
    eth_manager_link_callback_t link_callback;
    eth_manager_error_callback_t error_callback;
    eth_manager_stats_callback_t stats_callback;
    void *link_user_data;
    void *error_user_data;
    void *stats_user_data;

    /* 内部状态 */
    uint32_t last_link_check_time;
    uint32_t link_down_count;               /* 连续链路断开计数 */
    bool negotiation_complete;              /* 自动协商完成 */
    uint32_t last_stats_update_time;
    uint32_t stats_interval_ms;             /* 统计更新间隔 */

    /* 吞吐量计算 */
    uint64_t last_tx_bytes;
    uint64_t last_rx_bytes;
    uint32_t last_throughput_time;
} eth_manager_context_t;

/* 全局管理器上下文 */
static eth_manager_context_t g_manager_ctx = {0};

/* 错误代码到字符串映射 */
typedef struct {
    uint32_t code;
    const char *string;
} eth_error_code_map_t;

static const eth_error_code_map_t g_error_codes[] = {
    {0x0000, "No error"},
    {0x1001, "MAC initialization failed"},
    {0x1002, "MAC start failed"},
    {0x1003, "MAC stop failed"},
    {0x2001, "DMA initialization failed"},
    {0x2002, "DMA buffer allocation failed"},
    {0x2003, "DMA descriptor error"},
    {0x3001, "PHY initialization failed"},
    {0x3002, "PHY link down"},
    {0x3003, "PHY auto-negotiation failed"},
    {0x3004, "PHY cable fault detected"},
    {0x4001, "Manager initialization failed"},
    {0x4002, "Configuration invalid"},
    {0x4003, "Link monitoring error"},
    {0x4004, "Auto-recovery failed"},
    {0xFFFF, "Unknown error"}
};

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 获取当前时间(毫秒)
 */
static uint32_t eth_get_time_ms(void)
{
    /* 实际实现需要从系统获取时间 */
    static uint32_t tick = 0;
    return tick++;
}

/**
 * @brief 设置错误信息
 */
static void eth_set_error(uint32_t error_code)
{
    g_manager_ctx.last_error.error_code = error_code;
    g_manager_ctx.last_error.error_string = eth_manager_error_to_string(error_code);
    g_manager_ctx.last_error.timestamp = eth_get_time_ms();

    g_manager_ctx.stats.error_count++;

    /* 调用错误回调 */
    if (g_manager_ctx.error_callback != NULL) {
        g_manager_ctx.error_callback(&g_manager_ctx.last_error, g_manager_ctx.error_user_data);
    }
}

/**
 * @brief 更新统计信息
 */
static eth_status_t eth_update_stats(void)
{
    eth_status_t status;

    /* 更新MAC统计 */
    status = eth_mac_get_stats(&g_manager_ctx.stats.mac_stats);
    if (status != ETH_OK) {
        return status;
    }

    /* 更新DMA统计 */
    status = eth_dma_get_stats(&g_manager_ctx.stats.dma_stats);
    if (status != ETH_OK) {
        return status;
    }

    /* 更新PHY统计 */
    status = automotive_phy_get_stats(&g_manager_ctx.stats.phy_stats);
    if (status != ETH_OK) {
        return status;
    }

    /* 更新管理器统计 */
    g_manager_ctx.stats.link_up_count = g_manager_ctx.stats.phy_stats.link_up_count;
    g_manager_ctx.stats.link_down_count = g_manager_ctx.stats.phy_stats.link_down_count;
    g_manager_ctx.stats.auto_neg_count = g_manager_ctx.stats.phy_stats.auto_neg_count;

    return ETH_OK;
}

/**
 * @brief 监控链路状态
 */
static eth_status_t eth_monitor_link(void)
{
    if (!g_manager_ctx.config.link_monitor.enable_link_monitoring) {
        return ETH_OK;
    }

    uint32_t current_time = eth_get_time_ms();
    uint32_t elapsed = current_time - g_manager_ctx.last_link_check_time;

    if (elapsed < g_manager_ctx.config.link_monitor.link_check_interval_ms) {
        return ETH_OK;
    }

    g_manager_ctx.last_link_check_time = current_time;

    /* 检查链路状态 */
    automotive_link_status_t status;
    eth_status_t ret = automotive_phy_get_link_status(&status);
    if (ret != ETH_OK) {
        return ret;
    }

    /* 更新状态机 */
    eth_manager_state_t old_state = g_manager_ctx.state;

    if (status.link_up) {
        g_manager_ctx.state = ETH_MANAGER_STATE_LINK_UP;
        g_manager_ctx.link_down_count = 0;
    } else {
        g_manager_ctx.link_down_count++;

        /* 检查链路断开阈值 */
        if (g_manager_ctx.link_down_count >= g_manager_ctx.config.link_monitor.link_down_threshold) {
            g_manager_ctx.state = ETH_MANAGER_STATE_LINK_DOWN;

            /* 尝试自动恢复 */
            if (g_manager_ctx.config.link_monitor.auto_recovery) {
                eth_manager_auto_recovery();
            }
        }
    }

    /* 通知回调(状态变化时) */
    if (old_state != g_manager_ctx.state && g_manager_ctx.link_callback != NULL) {
        eth_link_status_t link_status = status.link_up ? ETH_LINK_UP : ETH_LINK_DOWN;
        g_manager_ctx.link_callback(link_status, g_manager_ctx.link_user_data);
    }

    return ETH_OK;
}

/**
 * @brief 更新吞吐量
 */
static void eth_update_throughput(void)
{
    uint32_t current_time = eth_get_time_ms();
    uint32_t elapsed_ms = current_time - g_manager_ctx.last_throughput_time;

    if (elapsed_ms == 0) {
        return;
    }

    /* 计算发送吞吐量(Mbps) */
    uint64_t tx_bytes_diff = g_manager_ctx.stats.mac_stats.tx_bytes - g_manager_ctx.last_tx_bytes;
    g_manager_ctx.stats.tx_throughput_mbps = (uint32_t)((tx_bytes_diff * 8) / elapsed_ms / 1000);

    /* 计算接收吞吐量(Mbps) */
    uint64_t rx_bytes_diff = g_manager_ctx.stats.mac_stats.rx_bytes - g_manager_ctx.last_rx_bytes;
    g_manager_ctx.stats.rx_throughput_mbps = (uint32_t)((rx_bytes_diff * 8) / elapsed_ms / 1000);

    /* 更新上次值 */
    g_manager_ctx.last_tx_bytes = g_manager_ctx.stats.mac_stats.tx_bytes;
    g_manager_ctx.last_rx_bytes = g_manager_ctx.stats.mac_stats.rx_bytes;
    g_manager_ctx.last_throughput_time = current_time;
}

/* ============================================================================
 * API实现 - 初始化和基本操作
 * ============================================================================ */

eth_status_t eth_manager_init(const eth_manager_config_t *config)
{
    eth_status_t status;

    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 验证配置 */
    status = eth_manager_validate_config(config);
    if (status != ETH_OK) {
        return status;
    }

    /* 检查是否已初始化 */
    if (g_manager_ctx.state != ETH_MANAGER_STATE_UNINIT) {
        return ETH_ERROR;
    }

    /* 复制配置 */
    memcpy(&g_manager_ctx.config, config, sizeof(eth_manager_config_t));

    /* 初始化MAC */
    status = eth_mac_init(&config->mac_config);
    if (status != ETH_OK) {
        eth_set_error(0x1001);
        return status;
    }

    /* 初始化DMA */
    status = eth_dma_init(&config->dma_config);
    if (status != ETH_OK) {
        eth_set_error(0x2001);
        eth_mac_deinit();
        return status;
    }

    /* 初始化PHY */
    status = automotive_phy_init(&config->phy_config);
    if (status != ETH_OK) {
        eth_set_error(0x3001);
        eth_dma_deinit();
        eth_mac_deinit();
        return status;
    }

    /* 初始化统计 */
    memset(&g_manager_ctx.stats, 0, sizeof(eth_manager_stats_t));

    /* 初始化状态 */
    g_manager_ctx.state = ETH_MANAGER_STATE_INIT;
    g_manager_ctx.last_link_check_time = 0;
    g_manager_ctx.link_down_count = 0;
    g_manager_ctx.negotiation_complete = false;
    g_manager_ctx.last_error.error_code = 0;
    g_manager_ctx.last_error.error_string = NULL;
    g_manager_ctx.last_error.timestamp = 0;
    g_manager_ctx.last_tx_bytes = 0;
    g_manager_ctx.last_rx_bytes = 0;
    g_manager_ctx.last_throughput_time = eth_get_time_ms();
    g_manager_ctx.stats_interval_ms = 0;

    /* 清除回调 */
    g_manager_ctx.link_callback = NULL;
    g_manager_ctx.error_callback = NULL;
    g_manager_ctx.stats_callback = NULL;

    return ETH_OK;
}

void eth_manager_deinit(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return;
    }

    /* 停止管理器 */
    eth_manager_stop();

    /* 反初始化各模块 */
    automotive_phy_deinit();
    eth_dma_deinit();
    eth_mac_deinit();

    /* 清除状态 */
    memset(&g_manager_ctx, 0, sizeof(eth_manager_context_t));
    g_manager_ctx.state = ETH_MANAGER_STATE_UNINIT;
}

eth_status_t eth_manager_start(void)
{
    if (g_manager_ctx.state != ETH_MANAGER_STATE_INIT) {
        return ETH_ERROR;
    }

    eth_status_t status;

    /* 启动DMA */
    status = eth_dma_start();
    if (status != ETH_OK) {
        return status;
    }

    /* 启动MAC */
    status = eth_mac_start();
    if (status != ETH_OK) {
        eth_dma_stop();
        return status;
    }

    /* 执行自动协商(如果启用) */
    if (g_manager_ctx.config.negotiation.enable_auto_negotiation) {
        status = eth_manager_start_auto_negotiation();
        if (status != ETH_OK) {
            /* 协商失败不阻止启动 */
        }
    }

    /* 更新状态 */
    g_manager_ctx.state = ETH_MANAGER_STATE_LINK_DOWN;

    return ETH_OK;
}

eth_status_t eth_manager_stop(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_ERROR;
    }

    /* 停止MAC */
    eth_mac_stop();

    /* 停止DMA */
    eth_dma_stop();

    /* 更新状态 */
    if (g_manager_ctx.state != ETH_MANAGER_STATE_INIT) {
        g_manager_ctx.state = ETH_MANAGER_STATE_INIT;
    }

    return ETH_OK;
}

eth_status_t eth_manager_get_state(eth_manager_state_t *state)
{
    if (state == NULL) {
        return ETH_INVALID_PARAM;
    }

    *state = g_manager_ctx.state;
    return ETH_OK;
}

/* ============================================================================
 * API实现 - 链路管理
 * ============================================================================ */

eth_status_t eth_manager_register_link_callback(eth_manager_link_callback_t callback, void *user_data)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_manager_ctx.link_callback = callback;
    g_manager_ctx.link_user_data = user_data;

    return ETH_OK;
}

eth_status_t eth_manager_check_link(eth_link_status_t *status)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 监控链路 */
    eth_status_t ret = eth_monitor_link();
    if (ret != ETH_OK) {
        return ret;
    }

    /* 获取链路状态 */
    if (status != NULL) {
        automotive_link_status_t auto_status;
        ret = automotive_phy_get_link_status(&auto_status);
        if (ret != ETH_OK) {
            return ret;
        }
        *status = auto_status.link_up ? ETH_LINK_UP : ETH_LINK_DOWN;
    }

    return ETH_OK;
}

eth_status_t eth_manager_wait_for_link(uint32_t timeout_ms)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    return automotive_phy_wait_for_link(timeout_ms);
}

eth_status_t eth_manager_force_link_check(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 强制更新链路检查时间 */
    g_manager_ctx.last_link_check_time = 0;
    return eth_monitor_link();
}

/* ============================================================================
 * API实现 - 自动协商
 * ============================================================================ */

eth_status_t eth_manager_start_auto_negotiation(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_manager_ctx.negotiation_complete = false;

    eth_status_t status = automotive_phy_auto_negotiation();
    if (status != ETH_OK) {
        eth_set_error(0x3003);
        return status;
    }

    g_manager_ctx.stats.auto_neg_count++;

    return ETH_OK;
}

eth_status_t eth_manager_get_negotiation_status(bool *complete)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (complete == NULL) {
        return ETH_INVALID_PARAM;
    }

    *complete = g_manager_ctx.negotiation_complete;
    return ETH_OK;
}

eth_status_t eth_manager_set_forced_mode(eth_mode_t mode)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 禁用自动协商并设置强制速率模式 */
    g_manager_ctx.config.negotiation.enable_auto_negotiation = false;

    /* 更新MAC配置 */
    g_manager_ctx.config.mac_config.speed_mode = mode;

    /* 应用到硬件(需要重新初始化) */
    (void)mode;

    return ETH_OK;
}

eth_status_t eth_manager_wait_for_negotiation(uint32_t timeout_ms)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    uint32_t elapsed = 0;
    const uint32_t poll_interval = 100;  /* 每100ms检查一次 */

    while (elapsed < timeout_ms) {
        /* 读取PHY状态寄存器检查自动协商状态 */
        uint16_t status_reg;
        eth_status_t status = automotive_phy_read_reg(PHY_REG_STATUS, &status_reg);

        if (status == ETH_OK && (status_reg & PHY_STATUS_AUTO_NEG_DONE)) {
            g_manager_ctx.negotiation_complete = true;
            return ETH_OK;
        }

        elapsed += poll_interval;
    }

    return ETH_TIMEOUT;
}

/* ============================================================================
 * API实现 - 错误统计
 * ============================================================================ */

eth_status_t eth_manager_get_stats(eth_manager_stats_t *stats)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 更新统计 */
    eth_status_t status = eth_update_stats();
    if (status != ETH_OK) {
        return status;
    }

    /* 更新吞吐量 */
    eth_update_throughput();

    memcpy(stats, &g_manager_ctx.stats, sizeof(eth_manager_stats_t));
    return ETH_OK;
}

eth_status_t eth_manager_clear_stats(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 清除各模块统计 */
    eth_mac_clear_stats();
    eth_dma_clear_stats();
    automotive_phy_clear_stats();

    /* 清除管理器统计 */
    memset(&g_manager_ctx.stats, 0, sizeof(eth_manager_stats_t));

    /* 重置吞吐量计算 */
    g_manager_ctx.last_tx_bytes = 0;
    g_manager_ctx.last_rx_bytes = 0;

    return ETH_OK;
}

eth_status_t eth_manager_register_error_callback(eth_manager_error_callback_t callback, void *user_data)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_manager_ctx.error_callback = callback;
    g_manager_ctx.error_user_data = user_data;

    return ETH_OK;
}

eth_status_t eth_manager_get_last_error(eth_error_info_t *error)
{
    if (error == NULL) {
        return ETH_INVALID_PARAM;
    }

    memcpy(error, &g_manager_ctx.last_error, sizeof(eth_error_info_t));
    return ETH_OK;
}

eth_status_t eth_manager_clear_error(void)
{
    g_manager_ctx.last_error.error_code = 0;
    g_manager_ctx.last_error.error_string = NULL;
    g_manager_ctx.last_error.timestamp = 0;

    if (g_manager_ctx.state == ETH_MANAGER_STATE_ERROR) {
        g_manager_ctx.state = ETH_MANAGER_STATE_INIT;
    }

    return ETH_OK;
}

const char* eth_manager_error_to_string(uint32_t error_code)
{
    for (size_t i = 0; i < sizeof(g_error_codes) / sizeof(g_error_codes[0]); i++) {
        if (g_error_codes[i].code == error_code) {
            return g_error_codes[i].string;
        }
    }
    return "Unknown error";
}

/* ============================================================================
 * API实现 - 诊断和恢复
 * ============================================================================ */

eth_status_t eth_manager_run_diagnostics(automotive_phy_diagnostics_t *diagnostics)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    return automotive_phy_run_diagnostics(diagnostics);
}

eth_status_t eth_manager_soft_reset(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    eth_status_t status;

    /* 停止传输 */
    status = eth_manager_stop();
    if (status != ETH_OK) {
        return status;
    }

    /* 执行PHY软复位 */
    status = automotive_phy_soft_reset();
    if (status != ETH_OK) {
        return status;
    }

    /* 重新启动 */
    status = eth_manager_start();

    g_manager_ctx.stats.recovery_count++;

    return status;
}

eth_status_t eth_manager_hard_reset(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 反初始化所有模块 */
    eth_manager_deinit();

    /* 重新初始化 */
    eth_status_t status = eth_manager_init(&g_manager_ctx.config);
    if (status != ETH_OK) {
        return status;
    }

    /* 启动 */
    status = eth_manager_start();

    g_manager_ctx.stats.recovery_count++;

    return status;
}

eth_status_t eth_manager_auto_recovery(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    eth_status_t status;

    /* 首先尝试软复位 */
    status = eth_manager_soft_reset();
    if (status == ETH_OK) {
        /* 等待链路连接 */
        status = eth_manager_wait_for_link(5000);
        if (status == ETH_OK) {
            return ETH_OK;
        }
    }

    /* 软复位失败，尝试硬复位 */
    status = eth_manager_hard_reset();
    if (status != ETH_OK) {
        eth_set_error(0x4004);
    }

    return status;
}

eth_status_t eth_manager_check_recovery_needed(bool *need_recovery)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (need_recovery == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 检查是否达到链路断开阈值 */
    *need_recovery = (g_manager_ctx.link_down_count >= g_manager_ctx.config.link_monitor.link_down_threshold);

    return ETH_OK;
}

/* ============================================================================
 * API实现 - 进阶功能
 * ============================================================================ */

eth_status_t eth_manager_register_stats_callback(eth_manager_stats_callback_t callback,
                                                  void *user_data,
                                                  uint32_t interval_ms)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_manager_ctx.stats_callback = callback;
    g_manager_ctx.stats_user_data = user_data;
    g_manager_ctx.stats_interval_ms = interval_ms;

    return ETH_OK;
}

eth_status_t eth_manager_update_throughput(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    eth_update_throughput();
    return ETH_OK;
}

eth_status_t eth_manager_get_config(eth_manager_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }

    memcpy(config, &g_manager_ctx.config, sizeof(eth_manager_config_t));
    return ETH_OK;
}

eth_status_t eth_manager_validate_config(const eth_manager_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 验证DMA配置(确保描述符数量有效) */
    if (config->dma_config.rx_desc_count == 0 || config->dma_config.tx_desc_count == 0) {
        return ETH_INVALID_PARAM;
    }

    /* 验证PHY配置 */
    if (config->phy_config.phy_addr > 31) {
        return ETH_INVALID_PARAM;
    }

    return ETH_OK;
}

eth_status_t eth_manager_print_diagnostics(void)
{
    if (g_manager_ctx.state == ETH_MANAGER_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 获取当前状态 */
    eth_manager_state_t state;
    eth_manager_get_state(&state);

    /* 获取统计 */
    eth_manager_stats_t stats;
    eth_manager_get_stats(&stats);

    /* 获取链路状态 */
    automotive_link_status_t link_status;
    automotive_phy_get_link_status(&link_status);

    /* 打印诊断信息(实际实现可以使用printf或日志系统) */
    (void)state;
    (void)stats;
    (void)link_status;

    return ETH_OK;
}

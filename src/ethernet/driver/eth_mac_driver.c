/**
 * @file eth_mac_driver.c
 * @brief 车载以太网MAC驱动层实现
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 符合AUTOSAR MCAL规范
 * @note 支持功能安全ASIL等级
 */

#include "eth_mac_driver.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 内部状态和定义
 * ============================================================================ */

/** MAC驱动内部状态 */
typedef enum {
    ETH_MAC_STATE_UNINIT = 0,
    ETH_MAC_STATE_INIT,
    ETH_MAC_STATE_ACTIVE,
    ETH_MAC_STATE_LOW_POWER,
    ETH_MAC_STATE_ERROR
} eth_mac_state_t;

/** MAC驱动内部结构 */
typedef struct {
    eth_mac_config_t config;                /* 配置信息 */
    eth_mac_state_t state;                  /* 当前状态 */
    eth_mac_stats_t stats;                  /* 统计信息 */
    eth_mac_isr_callback_t isr_callback;    /* 中断回调 */
    eth_mac_rx_callback_t rx_callback;      /* 接收回调 */
    void *isr_user_data;                    /* 中断用户数据 */
    void *rx_user_data;                     /* 接收用户数据 */
    uint32_t error_code;                    /* 当前错误码 */
    bool interrupts_enabled;                /* 中断使能状态 */
    uint32_t enabled_interrupts;            /* 启用的中断位图 */
} eth_mac_context_t;

/* 全局MAC驱动上下文 */
static eth_mac_context_t g_mac_ctx = {0};

/* 驱动表 - 支持多MAC实例 */
#define ETH_MAC_MAX_INSTANCES   4
static eth_mac_context_t g_mac_instances[ETH_MAC_MAX_INSTANCES] = {0};
static uint8_t g_mac_instance_count = 0;

/* 驱动锁 */
static bool g_mac_initialized = false;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 验证MAC配置参数
 */
static eth_status_t eth_mac_validate_config(const eth_mac_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 验证MAC地址 */
    bool is_zero_addr = true;
    for (int i = 0; i < 6; i++) {
        if (config->mac_addr[i] != 0) {
            is_zero_addr = false;
            break;
        }
    }
    if (is_zero_addr) {
        return ETH_INVALID_PARAM;
    }

    /* 验证帧过滤配置 */
    if (config->filter.max_frame_size < 64 || config->filter.max_frame_size > 9000) {
        return ETH_INVALID_PARAM;
    }

    /* 验证时钟频率 */
    if (config->clock_freq_hz == 0) {
        return ETH_INVALID_PARAM;
    }

    return ETH_OK;
}

/**
 * @brief 转换MAC状态到字符串(用于调试)
 */
static const char* eth_mac_state_to_string(eth_mac_state_t state)
{
    switch (state) {
        case ETH_MAC_STATE_UNINIT:   return "UNINIT";
        case ETH_MAC_STATE_INIT:     return "INIT";
        case ETH_MAC_STATE_ACTIVE:   return "ACTIVE";
        case ETH_MAC_STATE_LOW_POWER:return "LOW_POWER";
        case ETH_MAC_STATE_ERROR:    return "ERROR";
        default:                     return "UNKNOWN";
    }
}

/**
 * @brief 模拟MAC硬件初始化 - 通用实现
 */
static eth_status_t eth_mac_hw_init_generic(const eth_mac_config_t *config)
{
    /* 模拟硬件初始化延迟 */
    (void)config;

    /* 模拟配置MAC寄存器 */
    /* 实际实现需要操作硬件寄存器 */

    return ETH_OK;
}

/**
 * @brief 模拟MAC硬件启动 - 通用实现
 */
static eth_status_t eth_mac_hw_start_generic(void)
{
    /* 模拟启动MAC控制器 */
    /* 实际实现需要置位MAC控制寄存器 */

    return ETH_OK;
}

/**
 * @brief 模拟MAC硬件停止 - 通用实现
 */
static eth_status_t eth_mac_hw_stop_generic(void)
{
    /* 模拟停止MAC控制器 */
    /* 实际实现需要清零MAC控制寄存器 */

    return ETH_OK;
}

/**
 * @brief 更新错误状态
 */
static void eth_mac_update_error_status(eth_status_t status, uint32_t error_code)
{
    if (status != ETH_OK) {
        g_mac_ctx.error_code = error_code;
        g_mac_ctx.state = ETH_MAC_STATE_ERROR;
    }
}

/* ============================================================================
 * API实现 - 基本操作
 * ============================================================================ */

eth_status_t eth_mac_init(const eth_mac_config_t *config)
{
    eth_status_t status;

    /* 验证参数 */
    status = eth_mac_validate_config(config);
    if (status != ETH_OK) {
        return status;
    }

    /* 检查是否已初始化 */
    if (g_mac_ctx.state != ETH_MAC_STATE_UNINIT) {
        return ETH_ERROR;
    }

    /* 复制配置 */
    memcpy(&g_mac_ctx.config, config, sizeof(eth_mac_config_t));

    /* 根据MAC类型选择初始化函数 */
    switch (config->mac_type) {
        case ETH_MAC_TYPE_GENERIC:
            status = eth_mac_hw_init_generic(config);
            break;
        case ETH_MAC_TYPE_DW_GMAC:
            status = eth_mac_dw_gmac_init(config);
            break;
        case ETH_MAC_TYPE_NXP_S32:
            status = eth_mac_nxp_s32_init(config);
            break;
        case ETH_MAC_TYPE_INFINEON_AURIX:
            status = eth_mac_infineon_aurix_init(config);
            break;
        case ETH_MAC_TYPE_TI_CPSW:
            status = eth_mac_ti_cpsw_init(config);
            break;
        default:
            status = eth_mac_hw_init_generic(config);
            break;
    }

    if (status != ETH_OK) {
        eth_mac_update_error_status(status, 0x1001); /* 初始化错误 */
        return status;
    }

    /* 初始化统计 */
    memset(&g_mac_ctx.stats, 0, sizeof(eth_mac_stats_t));

    /* 设置状态 */
    g_mac_ctx.state = ETH_MAC_STATE_INIT;
    g_mac_ctx.error_code = 0;
    g_mac_ctx.interrupts_enabled = false;
    g_mac_ctx.enabled_interrupts = 0;
    g_mac_ctx.isr_callback = NULL;
    g_mac_ctx.rx_callback = NULL;

    g_mac_initialized = true;

    return ETH_OK;
}

void eth_mac_deinit(void)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return;
    }

    /* 停止MAC */
    eth_mac_stop();

    /* 清除状态 */
    memset(&g_mac_ctx, 0, sizeof(eth_mac_context_t));
    g_mac_ctx.state = ETH_MAC_STATE_UNINIT;
    g_mac_initialized = false;
}

eth_status_t eth_mac_start(void)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_INIT) {
        return ETH_NOT_INIT;
    }

    eth_status_t status = eth_mac_hw_start_generic();
    if (status != ETH_OK) {
        eth_mac_update_error_status(status, 0x1002); /* 启动错误 */
        return status;
    }

    g_mac_ctx.state = ETH_MAC_STATE_ACTIVE;
    return ETH_OK;
}

eth_status_t eth_mac_stop(void)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_ACTIVE &&
        g_mac_ctx.state != ETH_MAC_STATE_LOW_POWER) {
        return ETH_ERROR;
    }

    eth_status_t status = eth_mac_hw_stop_generic();
    if (status != ETH_OK) {
        eth_mac_update_error_status(status, 0x1003); /* 停止错误 */
        return status;
    }

    g_mac_ctx.state = ETH_MAC_STATE_INIT;
    return ETH_OK;
}

eth_status_t eth_mac_set_address(const eth_mac_addr_t mac_addr)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 验证MAC地址 */
    bool is_zero_addr = true;
    for (int i = 0; i < 6; i++) {
        if (mac_addr[i] != 0) {
            is_zero_addr = false;
            break;
        }
    }
    if (is_zero_addr) {
        return ETH_INVALID_PARAM;
    }

    /* 更新配置 */
    memcpy(g_mac_ctx.config.mac_addr, mac_addr, 6);

    /* 应用到硬件(实际实现需要操作寄存器) */
    return ETH_OK;
}

eth_status_t eth_mac_get_address(eth_mac_addr_t mac_addr)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    memcpy(mac_addr, g_mac_ctx.config.mac_addr, 6);
    return ETH_OK;
}

eth_status_t eth_mac_set_mode(eth_mac_mode_t mode)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_INIT &&
        g_mac_ctx.state != ETH_MAC_STATE_ACTIVE) {
        return ETH_ERROR;
    }

    /* 检查模式有效性 */
    if (mode > ETH_MAC_MODE_LOW_POWER) {
        return ETH_INVALID_PARAM;
    }

    g_mac_ctx.config.mac_mode = mode;

    /* 应用到硬件(实际实现) */
    return ETH_OK;
}

eth_status_t eth_mac_set_filter(const eth_mac_filter_t *filter)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_INIT &&
        g_mac_ctx.state != ETH_MAC_STATE_ACTIVE) {
        return ETH_ERROR;
    }

    if (filter == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 验证帧大小 */
    if (filter->max_frame_size < 64 || filter->max_frame_size > 9000) {
        return ETH_INVALID_PARAM;
    }

    memcpy(&g_mac_ctx.config.filter, filter, sizeof(eth_mac_filter_t));

    /* 应用到硬件(实际实现) */
    return ETH_OK;
}

/* ============================================================================
 * API实现 - 中断管理
 * ============================================================================ */

eth_status_t eth_mac_configure_interrupt(eth_mac_interrupt_t type, bool enable)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (type >= ETH_MAC_INT_COUNT) {
        return ETH_INVALID_PARAM;
    }

    if (enable) {
        g_mac_ctx.enabled_interrupts |= (1U << type);
    } else {
        g_mac_ctx.enabled_interrupts &= ~(1U << type);
    }

    /* 应用到硬件(实际实现) */
    return ETH_OK;
}

eth_status_t eth_mac_register_isr_callback(eth_mac_isr_callback_t callback, void *user_data)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_mac_ctx.isr_callback = callback;
    g_mac_ctx.isr_user_data = user_data;

    return ETH_OK;
}

eth_status_t eth_mac_handle_interrupt(void)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_ACTIVE) {
        return ETH_ERROR;
    }

    /* 模拟处理中断(实际实现需要读取中断状态寄存器) */

    /* 调用用户回调 */
    if (g_mac_ctx.isr_callback != NULL) {
        for (int i = 0; i < ETH_MAC_INT_COUNT; i++) {
            if (g_mac_ctx.enabled_interrupts & (1U << i)) {
                g_mac_ctx.isr_callback((eth_mac_interrupt_t)i, g_mac_ctx.isr_user_data);
            }
        }
    }

    return ETH_OK;
}

/* ============================================================================
 * API实现 - 统计信息
 * ============================================================================ */

eth_status_t eth_mac_get_stats(eth_mac_stats_t *stats)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }

    memcpy(stats, &g_mac_ctx.stats, sizeof(eth_mac_stats_t));
    return ETH_OK;
}

eth_status_t eth_mac_clear_stats(void)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    memset(&g_mac_ctx.stats, 0, sizeof(eth_mac_stats_t));
    return ETH_OK;
}

/* ============================================================================
 * API实现 - 数据收发
 * ============================================================================ */

eth_status_t eth_mac_transmit(const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_ACTIVE) {
        return ETH_ERROR;
    }

    if (data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }

    /* 检查帧长度 */
    if (len < 14 || len > g_mac_ctx.config.filter.max_frame_size) {
        g_mac_ctx.stats.tx_errors++;
        return ETH_INVALID_PARAM;
    }

    /* 模拟发送操作(实际实现需要操作DMA或FIFO) */
    (void)timeout_ms;

    /* 更新统计 */
    g_mac_ctx.stats.tx_frames++;
    g_mac_ctx.stats.tx_bytes += len;

    return ETH_OK;
}

eth_status_t eth_mac_receive(uint8_t *buffer, uint32_t max_len,
                              uint32_t *received_len, uint32_t timeout_ms)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_ACTIVE) {
        return ETH_ERROR;
    }

    if (buffer == NULL || max_len == 0 || received_len == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 模拟接收操作(实际实现需要操作DMA或FIFO) */
    (void)timeout_ms;

    /* 模拟接收到数据 */
    *received_len = 0;

    return ETH_TIMEOUT; /* 模拟超时 */
}

eth_status_t eth_mac_register_rx_callback(eth_mac_rx_callback_t callback, void *user_data)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_mac_ctx.rx_callback = callback;
    g_mac_ctx.rx_user_data = user_data;

    return ETH_OK;
}

/* ============================================================================
 * API实现 - 特定芯片接口(存根)
 * ============================================================================ */

eth_status_t eth_mac_dw_gmac_init(const eth_mac_config_t *config)
{
    /* DesignWare GMAC特定初始化 - 存根 */
    (void)config;
    return ETH_OK;
}

eth_status_t eth_mac_nxp_s32_init(const eth_mac_config_t *config)
{
    /* NXP S32特定初始化 - 存根 */
    (void)config;
    return ETH_OK;
}

eth_status_t eth_mac_infineon_aurix_init(const eth_mac_config_t *config)
{
    /* Infineon AURIX特定初始化 - 存根 */
    (void)config;
    return ETH_OK;
}

eth_status_t eth_mac_ti_cpsw_init(const eth_mac_config_t *config)
{
    /* TI CPSW特定初始化 - 存根 */
    (void)config;
    return ETH_OK;
}

/* ============================================================================
 * API实现 - 高级功能
 * ============================================================================ */

eth_status_t eth_mac_set_clock_divider(uint32_t divider)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (divider == 0) {
        return ETH_INVALID_PARAM;
    }

    /* 应用到硬件(实际实现) */
    (void)divider;

    return ETH_OK;
}

eth_status_t eth_mac_enter_low_power(void)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_ACTIVE) {
        return ETH_ERROR;
    }

    /* 进入低功耗模式(实际实现) */
    g_mac_ctx.state = ETH_MAC_STATE_LOW_POWER;

    return ETH_OK;
}

eth_status_t eth_mac_exit_low_power(void)
{
    if (g_mac_ctx.state != ETH_MAC_STATE_LOW_POWER) {
        return ETH_ERROR;
    }

    /* 退出低功耗模式(实际实现) */
    g_mac_ctx.state = ETH_MAC_STATE_ACTIVE;

    return ETH_OK;
}

eth_status_t eth_mac_configure_emc(bool enable_filter, uint8_t slew_rate)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_mac_ctx.config.enable_emc_filter = enable_filter;
    g_mac_ctx.config.slew_rate = slew_rate;

    /* 应用到硬件(实际实现) */

    return ETH_OK;
}

eth_status_t eth_mac_self_test(void)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    /* 执行MAC自检(实际实现需要硬件支持) */

    return ETH_OK;
}

eth_status_t eth_mac_get_error_status(uint32_t *error_code)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    if (error_code == NULL) {
        return ETH_INVALID_PARAM;
    }

    *error_code = g_mac_ctx.error_code;
    return ETH_OK;
}

eth_status_t eth_mac_clear_error_status(void)
{
    if (g_mac_ctx.state == ETH_MAC_STATE_UNINIT) {
        return ETH_NOT_INIT;
    }

    g_mac_ctx.error_code = 0;

    if (g_mac_ctx.state == ETH_MAC_STATE_ERROR) {
        g_mac_ctx.state = ETH_MAC_STATE_INIT;
    }

    return ETH_OK;
}

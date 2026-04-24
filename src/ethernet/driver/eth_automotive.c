/**
 * @file eth_automotive.c
 * @brief 汽车以太网特定支持实现 - PHY驱动
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持100BASE-T1 (TJA1101/DP83TC811)
 * @note 支持1000BASE-T1 (TJA1103/DP83TG720/88Q2112)
 * @note 符合AUTOSAR MCAL规范
 */

#include "eth_automotive.h"
#include <string.h>

/* ============================================================================
 * PHY寄存器定义
 * ============================================================================ */

/** IEEE标准PHY寄存器 */
#define PHY_REG_CONTROL         0x00    /* 控制寄存器 */
#define PHY_REG_STATUS          0x01    /* 状态寄存器 */
#define PHY_REG_ID1             0x02    /* 标识寄存器1 */
#define PHY_REG_ID2             0x03    /* 标识寄存器2 */
#define PHY_REG_AUTO_NEG_ADVERT 0x04    /* 自动协商广告 */
#define PHY_REG_AUTO_NEG_LINK   0x05    /* 自动协商链路能力 */
#define PHY_REG_AUTO_NEG_EXP    0x06    /* 自动协商扩展 */

/** 控制寄存器位 */
#define PHY_CTRL_RESET          0x8000  /* 复位 */
#define PHY_CTRL_LOOPBACK       0x4000  /* 回环 */
#define PHY_CTRL_SPEED_SEL      0x2000  /* 速率选择 */
#define PHY_CTRL_AUTO_NEG       0x1000  /* 自动协商使能 */
#define PHY_CTRL_POWER_DOWN     0x0800  /* 低功耗 */
#define PHY_CTRL_ISOLATE        0x0400  /* 隔离 */
#define PHY_CTRL_RESTART_NEG    0x0200  /* 重新协商 */
#define PHY_CTRL_DUPLEX_MODE    0x0100  /* 双工模式 */

/** 状态寄存器位 */
#define PHY_STATUS_100BASE_T4   0x8000
#define PHY_STATUS_100BASE_TX_F 0x4000
#define PHY_STATUS_100BASE_TX_H 0x2000
#define PHY_STATUS_10BASE_T_F   0x1000
#define PHY_STATUS_10BASE_T_H   0x0800
#define PHY_STATUS_AUTO_NEG_DONE 0x0020
#define PHY_STATUS_REMOTE_FAULT 0x0010
#define PHY_STATUS_AUTO_NEG_CAP 0x0008
#define PHY_STATUS_LINK_UP      0x0004
#define PHY_STATUS_JABBER       0x0002
#define PHY_STATUS_EXT_CAP      0x0001

/** 特定PHY寄存器 - NXP TJA1101 */
#define TJA1101_EXTENDED_CONTROL    0x11    /* 扩展控制 */
#define TJA1101_CONFIG1             0x12    /* 配置寄存器1 */
#define TJA1101_CONFIG2             0x13    /* 配置寄存器2 */
#define TJA1101_SYM_ERR_COUNTER     0x14    /* 符号错误计数器 */
#define TJA1101_INT_STATUS          0x15    /* 中断状态 */
#define TJA1101_INT_ENABLE          0x16    /* 中断使能 */
#define TJA1101_COMM_STATUS         0x17    /* 通信状态 */
#define TJA1101_GENERAL_STATUS      0x18    /* 通用状态 */
#define TJA1101_EXTERNAL_STATUS     0x19    /* 外部状态 */

/** TJA1101控制位 */
#define TJA1101_CTRL_CONFIG_EN      0x8000  /* 配置使能 */
#define TJA1101_CTRL_CONFIG_INH     0x4000  /* 配置禁用 */
#define TJA1101_CTRL_WAKE_REQUEST   0x0080  /* 唤醒请求 */

/** TJA1101状态位 */
#define TJA1101_COMM_LINK_UP        0x8000  /* 链路上升 */
#define TJA1101_COMM_TX_MODE        0x3000  /* 传输模式 */
#define TJA1101_COMM_MASTER         0x0800  /* 主模式 */
#define TJA1101_COMM_SLAVE          0x0400  /* 从模式 */
#define TJA1101_COMM_CONFIG_OK      0x0080  /* 配置完成 */

/** 特定PHY寄存器 - TI DP83TC811 */
#define DP83TC811_PHY_STS           0x10    /* PHY状态 */
#define DP83TC811_PHY_CTRL          0x1F    /* PHY控制 */
#define DP83TC811_RMSRG1            0x18    /* 诊断寄存器1 */
#define DP83TC811_RMSRG2            0x19    /* 诊断寄存器2 */

/** DP83TC811状态位 */
#define DP83TC811_STS_LINK          0x0100  /* 链路状态 */
#define DP83TC811_STS_SPEED         0xC000  /* 速率状态 */
#define DP83TC811_STS_DUPLEX        0x0010  /* 双工状态 */
#define DP83TC811_STS_MASTER        0x0008  /* 主节点 */

/** 特定PHY寄存器 - TI DP83TG720 (1000BASE-T1) */
#define DP83TG720_PHY_STS           0x11    /* PHY状态 */
#define DP83TG720_RMSR              0x18    /* 链路测试状态 */
#define DP83TG720_CRSMR             0x19    /* 电缆诊断状态 */

/* ============================================================================
 * 内部状态结构
 * ============================================================================ */

typedef struct {
    automotive_phy_config_t config;         /* PHY配置 */
    automotive_link_status_t link_status;   /* 链路状态 */
    automotive_phy_stats_t stats;           /* 统计信息 */
    bool initialized;                       /* 初始化状态 */
    uint32_t last_diag_time;                /* 上次诊断时间 */
} automotive_phy_context_t;

/* 全局PHY上下文 */
static automotive_phy_context_t g_phy_ctx = {0};

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 验证PHY配置
 */
static eth_status_t automotive_phy_validate_config(const automotive_phy_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 验证MDIO地址 */
    if (config->phy_addr > 31) {
        return ETH_INVALID_PARAM;
    }

    /* 验证PHY类型 */
    if (config->phy_type > AUTOMOTIVE_PHY_RTL9010) {
        return ETH_INVALID_PARAM;
    }

    /* 验证标准 */
    if (config->standard > AUTOMOTIVE_ETH_STANDARD_10GBASE_T1) {
        return ETH_INVALID_PARAM;
    }

    return ETH_OK;
}

/**
 * @brief 通用MDIO读取(存根 - 需要硬件实现)
 */
static eth_status_t mdio_read(uint8_t phy_addr, uint8_t reg_addr, uint16_t *value)
{
    /* 存根: 实际实现需要操作MDIO总线 */
    (void)phy_addr;
    (void)reg_addr;
    *value = 0xFFFF;  /* 模拟读取 */
    return ETH_OK;
}

/**
 * @brief 通用MDIO写入(存根 - 需要硬件实现)
 */
static eth_status_t mdio_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t value)
{
    /* 存根: 实际实现需要操作MDIO总线 */
    (void)phy_addr;
    (void)reg_addr;
    (void)value;
    return ETH_OK;
}

/**
 * @brief 更新链路状态
 */
static eth_status_t automotive_phy_update_link_status(void)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    uint16_t status_reg;
    eth_status_t status;

    /* 读取状态寄存器 */
    status = mdio_read(g_phy_ctx.config.phy_addr, PHY_REG_STATUS, &status_reg);
    if (status != ETH_OK) {
        return status;
    }

    /* 检查链路状态变化 */
    bool link_up = (status_reg & PHY_STATUS_LINK_UP) != 0;
    if (link_up != g_phy_ctx.link_status.link_up) {
        if (link_up) {
            g_phy_ctx.stats.link_up_count++;
        } else {
            g_phy_ctx.stats.link_down_count++;
        }
        g_phy_ctx.link_status.link_up = link_up;
    }

    /* 更新速率和双工模式(特定于PHY型号) */
    g_phy_ctx.link_status.full_duplex = true;  /* 汽车以太网总是全双工 */

    switch (g_phy_ctx.config.standard) {
        case AUTOMOTIVE_ETH_STANDARD_100BASE_T1:
            g_phy_ctx.link_status.speed = AUTOMOTIVE_ETH_SPEED_100MBPS;
            break;
        case AUTOMOTIVE_ETH_STANDARD_1000BASE_T1:
            g_phy_ctx.link_status.speed = AUTOMOTIVE_ETH_SPEED_1000MBPS;
            break;
        default:
            break;
    }

    return ETH_OK;
}

/* ============================================================================
 * PHY驱动API实现
 * ============================================================================ */

eth_status_t automotive_phy_init(const automotive_phy_config_t *config)
{
    eth_status_t status;

    /* 验证参数 */
    status = automotive_phy_validate_config(config);
    if (status != ETH_OK) {
        return status;
    }

    /* 检查是否已初始化 */
    if (g_phy_ctx.initialized) {
        return ETH_ERROR;
    }

    /* 复制配置 */
    memcpy(&g_phy_ctx.config, config, sizeof(automotive_phy_config_t));

    /* 初始化统计 */
    memset(&g_phy_ctx.stats, 0, sizeof(automotive_phy_stats_t));

    /* 初始化链路状态 */
    memset(&g_phy_ctx.link_status, 0, sizeof(automotive_link_status_t));
    g_phy_ctx.link_status.signal_quality = -1;
    g_phy_ctx.link_status.cable_length = -1;

    /* 根据PHY类型执行特定初始化 */
    switch (config->phy_type) {
        case AUTOMOTIVE_PHY_TJA1101:
        case AUTOMOTIVE_PHY_TJA1102:
            status = phy_tja1101_init(config->phy_addr);
            break;
        case AUTOMOTIVE_PHY_DP83TC811:
            status = phy_dp83tc811_init(config->phy_addr);
            break;
        case AUTOMOTIVE_PHY_DP83TG720:
            status = phy_dp83tg720_init(config->phy_addr);
            break;
        case AUTOMOTIVE_PHY_MARVELL_88Q2112:
            status = phy_marvell_88q2112_init(config->phy_addr);
            break;
        default:
            /* 通用PHY初始化 */
            status = ETH_OK;
            break;
    }

    if (status != ETH_OK) {
        return status;
    }

    /* 配置主从模式 */
    if (config->enable_master_mode) {
        automotive_phy_set_master_mode(true);
    }

    /* 配置EMC参数 */
    automotive_phy_configure_emc(config->enable_emc_filter, config->slew_rate);

    /* 配置WOL */
    if (config->enable_wake_on_lan) {
        automotive_phy_configure_wol(true);
    }

    g_phy_ctx.initialized = true;
    g_phy_ctx.last_diag_time = 0;

    return ETH_OK;
}

void automotive_phy_deinit(void)
{
    if (!g_phy_ctx.initialized) {
        return;
    }

    /* 禁用PHY */
    uint16_t ctrl_reg;
    mdio_read(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, &ctrl_reg);
    ctrl_reg |= PHY_CTRL_POWER_DOWN;
    mdio_write(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, ctrl_reg);

    /* 清除状态 */
    memset(&g_phy_ctx, 0, sizeof(automotive_phy_context_t));
}

eth_status_t automotive_phy_auto_negotiation(void)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    /* 汽车以太网PHY通常不使用标准的自动协商
     * 而是使用主从协商(100BASE-T1)或速率协商(1000BASE-T1) */

    uint16_t ctrl_reg;
    eth_status_t status;

    /* 读取当前控制寄存器 */
    status = mdio_read(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, &ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    /* 启用自动协商(如果支持) */
    ctrl_reg |= PHY_CTRL_AUTO_NEG;
    status = mdio_write(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    /* 重新启动自动协商 */
    ctrl_reg |= PHY_CTRL_RESTART_NEG;
    status = mdio_write(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    g_phy_ctx.stats.auto_neg_count++;

    return ETH_OK;
}

eth_status_t automotive_phy_get_link_status(automotive_link_status_t *status)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    if (status == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 更新链路状态 */
    eth_status_t ret = automotive_phy_update_link_status();
    if (ret != ETH_OK) {
        return ret;
    }

    /* 复制状态 */
    memcpy(status, &g_phy_ctx.link_status, sizeof(automotive_link_status_t));

    return ETH_OK;
}

eth_status_t automotive_phy_wait_for_link(uint32_t timeout_ms)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;  /* 每10ms检查一次 */

    while (elapsed < timeout_ms) {
        automotive_link_status_t status;
        eth_status_t ret = automotive_phy_get_link_status(&status);

        if (ret == ETH_OK && status.link_up) {
            return ETH_OK;
        }

        /* 模拟延迟 */
        elapsed += poll_interval;
    }

    return ETH_TIMEOUT;
}

eth_status_t automotive_phy_run_diagnostics(automotive_phy_diagnostics_t *diagnostics)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    if (diagnostics == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 初始化诊断结果 */
    memset(diagnostics, 0, sizeof(automotive_phy_diagnostics_t));

    /* 检查链路状态 */
    automotive_link_status_t link_status;
    eth_status_t status = automotive_phy_get_link_status(&link_status);
    if (status != ETH_OK) {
        return status;
    }

    diagnostics->link_pass = link_status.link_up;
    diagnostics->cable_ok = link_status.link_up;
    diagnostics->cable_length_m = link_status.cable_length;
    diagnostics->signal_quality = link_status.signal_quality;

    /* 特定PHY诊断(如果启用) */
    if (g_phy_ctx.config.enable_cable_diag) {
        /* 特定PHY的电缆诊断(实际实现需要读取PHY特定寄存器) */
    }

    return ETH_OK;
}

eth_status_t automotive_phy_get_stats(automotive_phy_stats_t *stats)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }

    memcpy(stats, &g_phy_ctx.stats, sizeof(automotive_phy_stats_t));
    return ETH_OK;
}

eth_status_t automotive_phy_clear_stats(void)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    memset(&g_phy_ctx.stats, 0, sizeof(automotive_phy_stats_t));
    return ETH_OK;
}

eth_status_t automotive_phy_enter_low_power(bool wake_on_link)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    uint16_t ctrl_reg;
    eth_status_t status;

    /* 读取控制寄存器 */
    status = mdio_read(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, &ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    /* 进入低功耗模式 */
    if (wake_on_link) {
        /* 保留链路检测 */
        ctrl_reg |= PHY_CTRL_POWER_DOWN;
    } else {
        ctrl_reg |= PHY_CTRL_POWER_DOWN;
    }

    return mdio_write(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, ctrl_reg);
}

eth_status_t automotive_phy_exit_low_power(void)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    uint16_t ctrl_reg;
    eth_status_t status;

    /* 读取控制寄存器 */
    status = mdio_read(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, &ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    /* 退出低功耗模式 */
    ctrl_reg &= ~PHY_CTRL_POWER_DOWN;

    return mdio_write(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, ctrl_reg);
}

eth_status_t automotive_phy_set_master_mode(bool master)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    /* 特定PHY的主从配置(实际实现需要访问PHY特定寄存器) */
    (void)master;

    g_phy_ctx.link_status.is_master = master;
    g_phy_ctx.link_status.master_slave_resolved = true;

    return ETH_OK;
}

eth_status_t automotive_phy_soft_reset(void)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    uint16_t ctrl_reg;
    eth_status_t status;

    /* 读取当前控制寄存器 */
    status = mdio_read(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, &ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    /* 执行软复位 */
    ctrl_reg |= PHY_CTRL_RESET;
    status = mdio_write(g_phy_ctx.config.phy_addr, PHY_REG_CONTROL, ctrl_reg);
    if (status != ETH_OK) {
        return status;
    }

    g_phy_ctx.stats.reset_count++;

    return ETH_OK;
}

eth_status_t automotive_phy_configure_emc(bool enable_filter, uint8_t slew_rate)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    /* 更新配置 */
    g_phy_ctx.config.enable_emc_filter = enable_filter;
    g_phy_ctx.config.slew_rate = slew_rate;

    /* 特定PHY的EMC配置(实际实现) */

    return ETH_OK;
}

eth_status_t automotive_phy_configure_wol(bool enable)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    g_phy_ctx.config.enable_wake_on_lan = enable;

    /* 特定PHY的WOL配置(实际实现) */

    return ETH_OK;
}

/* ============================================================================
 * MDIO操作API实现
 * ============================================================================ */

eth_status_t automotive_phy_read_reg(uint8_t reg_addr, uint16_t *value)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    if (value == NULL) {
        return ETH_INVALID_PARAM;
    }

    return mdio_read(g_phy_ctx.config.phy_addr, reg_addr, value);
}

eth_status_t automotive_phy_write_reg(uint8_t reg_addr, uint16_t value)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    return mdio_write(g_phy_ctx.config.phy_addr, reg_addr, value);
}

eth_status_t automotive_phy_read_mmd(uint8_t dev_addr, uint16_t reg_addr, uint16_t *value)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    if (value == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 写入设备地址和寄存器地址 */
    mdio_write(g_phy_ctx.config.phy_addr, 0x0D, dev_addr);
    mdio_write(g_phy_ctx.config.phy_addr, 0x0E, reg_addr);

    /* 读取数据 */
    return mdio_read(g_phy_ctx.config.phy_addr, 0x0E, value);
}

eth_status_t automotive_phy_write_mmd(uint8_t dev_addr, uint16_t reg_addr, uint16_t value)
{
    if (!g_phy_ctx.initialized) {
        return ETH_NOT_INIT;
    }

    /* 写入设备地址和寄存器地址 */
    mdio_write(g_phy_ctx.config.phy_addr, 0x0D, dev_addr);
    mdio_write(g_phy_ctx.config.phy_addr, 0x0E, reg_addr);

    /* 写入数据 */
    return mdio_write(g_phy_ctx.config.phy_addr, 0x0E, value);
}

/* ============================================================================
 * 特定PHY驱动实现
 * ============================================================================ */

eth_status_t phy_tja1101_init(uint8_t phy_addr)
{
    /* TJA1101初始化序列 */
    (void)phy_addr;

    /* 执行软复位 */
    /* mdio_write(phy_addr, PHY_REG_CONTROL, PHY_CTRL_RESET); */

    /* 配置100BASE-T1模式 */
    /* 实际实现需要访问TJA1101特定寄存器 */

    return ETH_OK;
}

eth_status_t phy_dp83tc811_init(uint8_t phy_addr)
{
    /* DP83TC811初始化序列 */
    (void)phy_addr;

    /* 检查PHY ID */
    /* 实际实现需要读取PHY ID寄存器验证 */

    /* 配置100BASE-T1 */
    /* 实际实现需要访问DP83TC811特定寄存器 */

    return ETH_OK;
}

eth_status_t phy_dp83tg720_init(uint8_t phy_addr)
{
    /* DP83TG720 (1000BASE-T1) 初始化序列 */
    (void)phy_addr;

    /* 配置1000BASE-T1 */
    /* 实际实现需要访问DP83TG720特定寄存器 */

    return ETH_OK;
}

eth_status_t phy_marvell_88q2112_init(uint8_t phy_addr)
{
    /* Marvell 88Q2112 (1000BASE-T1) 初始化序列 */
    (void)phy_addr;

    /* 配置1000BASE-T1 */
    /* 实际实现需要访问88Q2112特定寄存器 */

    return ETH_OK;
}

/* ============================================================================
 * TSN API实现(预留)
 * ============================================================================ */

eth_status_t tsn_init(const tsn_config_t *config)
{
    /* TSN初始化预留接口 */
    (void)config;
    return ETH_OK;
}

eth_status_t tsn_enable_feature(tsn_features_t feature, bool enable)
{
    /* TSN功能使能预留接口 */
    (void)feature;
    (void)enable;
    return ETH_OK;
}

eth_status_t tsn_configure_gate_control(const tsn_gate_control_entry_t *entries, uint8_t count)
{
    /* TSN门控配置预留接口 */
    (void)entries;
    (void)count;
    return ETH_OK;
}

eth_status_t tsn_get_timestamp(uint64_t *timestamp)
{
    /* TSN时间戳获取预留接口 */
    if (timestamp == NULL) {
        return ETH_INVALID_PARAM;
    }
    *timestamp = 0;
    return ETH_OK;
}

eth_status_t tsn_sync_time(uint8_t domain_number)
{
    /* TSN时间同步预留接口 */
    (void)domain_number;
    return ETH_OK;
}

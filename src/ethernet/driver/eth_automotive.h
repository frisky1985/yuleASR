/**
 * @file eth_automotive.h
 * @brief 汽车以太网特定支持 - PHY驱动和TSN预留
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持100BASE-T1 (IEEE 802.3bw)
 * @note 支持1000BASE-T1 (IEEE 802.3bp)
 * @note TSN (IEEE 802.1AS/Qbv/Qbu)支持预留
 * @note 符合AUTOSAR MCAL规范
 */

#ifndef ETH_AUTOMOTIVE_H
#define ETH_AUTOMOTIVE_H

#include "../../common/types/eth_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 汽车以太网标准定义
 * ============================================================================ */

/** 汽车以太网速率类型 */
typedef enum {
    AUTOMOTIVE_ETH_SPEED_100MBPS = 0,   /* 100BASE-T1 - 100Mbps全双工 */
    AUTOMOTIVE_ETH_SPEED_1000MBPS,      /* 1000BASE-T1 - 1Gbps全双工 */
} automotive_eth_speed_t;

/** 汽车以太网标准 */
typedef enum {
    AUTOMOTIVE_ETH_STANDARD_100BASE_T1 = 0,     /* IEEE 802.3bw */
    AUTOMOTIVE_ETH_STANDARD_1000BASE_T1,        /* IEEE 802.3bp */
    AUTOMOTIVE_ETH_STANDARD_10BASE_T1S,         /* IEEE 802.3cg (Multi-drop) */
    AUTOMOTIVE_ETH_STANDARD_2P5GBASE_T1,        /* IEEE 802.3ch */
    AUTOMOTIVE_ETH_STANDARD_5GBASE_T1,          /* IEEE 802.3ch */
    AUTOMOTIVE_ETH_STANDARD_10GBASE_T1,         /* IEEE 802.3ch */
} automotive_eth_standard_t;

/** 汽车以太网链路测试模式 */
typedef enum {
    AUTOMOTIVE_LINK_TEST_NONE = 0,      /* 无测试 */
    AUTOMOTIVE_LINK_TEST_MASTER,        /* 主节点测试模式 */
    AUTOMOTIVE_LINK_TEST_SLAVE,         /* 从节点测试模式 */
} automotive_link_test_mode_t;

/** 汽车以太网连接状态 */
typedef struct {
    bool link_up;                       /* 链路连接状态 */
    automotive_eth_speed_t speed;       /* 当前速率 */
    bool full_duplex;                   /* 全双工模式 */
    int8_t signal_quality;              /* 信号质量 (0-100, -1表示不可用) */
    int8_t cable_length;                /* 电缆长度(米, -1表示不可用) */
    bool master_slave_resolved;         /* 主从模式已解析 */
    bool is_master;                     /* 当前为主节点 */
    uint32_t error_count;               /* 错误计数 */
} automotive_link_status_t;

/* ============================================================================
 * PHY芯片支持
 * ============================================================================ */

/** 支持的汽车以太网PHY */
typedef enum {
    AUTOMOTIVE_PHY_GENERIC = 0,         /* 通用PHY */
    AUTOMOTIVE_PHY_TJA1101,             /* NXP TJA1101 (100BASE-T1) */
    AUTOMOTIVE_PHY_TJA1102,             /* NXP TJA1102 (100BASE-T1 双端口) */
    AUTOMOTIVE_PHY_TJA1103,             /* NXP TJA1103 (1000BASE-T1) */
    AUTOMOTIVE_PHY_DP83TC811,           /* TI DP83TC811 (100BASE-T1) */
    AUTOMOTIVE_PHY_DP83TG720,           /* TI DP83TG720 (1000BASE-T1) */
    AUTOMOTIVE_PHY_VSC8541,             /* Microchip VSC8541 */
    AUTOMOTIVE_PHY_MARVELL_88Q2112,     /* Marvell 88Q2112 (1000BASE-T1) */
    AUTOMOTIVE_PHY_BCM89811,            /* Broadcom BCM89811 (1000BASE-T1) */
    AUTOMOTIVE_PHY_RTL9010,             /* Realtek RTL9010 (1000BASE-T1) */
} automotive_phy_type_t;

/* ============================================================================
 * PHY配置和状态
 * ============================================================================ */

/** PHY配置结构 */
typedef struct {
    automotive_phy_type_t phy_type;         /* PHY芯片类型 */
    automotive_eth_standard_t standard;     /* 以太网标准 */
    uint8_t phy_addr;                       /* MDIO地址 (0-31) */
    bool enable_master_mode;                /* 启用主模式(100BASE-T1) */
    bool enable_wake_on_lan;                /* 启用远程唤醒 */
    bool enable_loopback;                   /* 启用回环测试 */
    bool enable_jumbo;                      /* 启用巨型帧 */

    /* 电磁兼容配置 */
    uint8_t slew_rate;                      /* 信号斜率控制 */
    bool enable_emc_filter;                 /* 启用EMC滤波 */

    /* 诊断配置 */
    bool enable_link_diag;                  /* 启用链路诊断 */
    bool enable_cable_diag;                 /* 启用电缆诊断 */
    uint32_t diag_interval_ms;              /* 诊断间隔(毫秒) */
} automotive_phy_config_t;

/** PHY诊断结果 */
typedef struct {
    bool link_pass;                     /* 链路测试通过 */
    bool cable_ok;                      /* 电缆正常 */
    int8_t cable_length_m;              /* 电缆长度(米) */
    uint32_t pair_swaps;                /* 线对交换检测 */
    bool polarity_inverted;             /* 极性反转 */
    uint32_t signal_quality;            /* 信号质量指标 */
} automotive_phy_diagnostics_t;

/** PHY统计信息 */
typedef struct {
    uint32_t symbol_errors;             /* 符号错误数 */
    uint32_t crc_errors;                /* CRC错误数 */
    uint32_t frame_errors;              /* 帧错误数 */
    uint32_t link_up_count;             /* 链路连接次数 */
    uint32_t link_down_count;           /* 链路断开次数 */
    uint32_t auto_neg_count;            /* 自动协商次数 */
    uint32_t reset_count;               /* 复位次数 */
} automotive_phy_stats_t;

/* ============================================================================
 * TSN支持预留
 * ============================================================================ */

/** TSN功能标志 */
typedef enum {
    TSN_FEATURE_NONE = 0x00,
    TSN_FEATURE_AS = 0x01,              /* IEEE 802.1AS - gPTP */
    TSN_FEATURE_QBV = 0x02,             /* IEEE 802.1Qbv - 增量门控制列表 */
    TSN_FEATURE_QBU = 0x04,             /* IEEE 802.1Qbu - 帧预充 */
    TSN_FEATURE_QCI = 0x08,             /* IEEE 802.1Qci - 流过滤 */
    TSN_FEATURE_CBS = 0x10,             /* IEEE 802.1Qav - 信用型带宽 */
    TSN_FEATURE_QCA = 0x20,             /* IEEE 802.1Qca - 路径控制 */
    TSN_FEATURE_CB = 0x40,              /* IEEE 802.1CB - 帧复制和消除 */
} tsn_features_t;

/** TSN时间夹结构 */
typedef struct {
    uint64_t base_time_ns;              /* 基准时间(纳秒) */
    uint64_t cycle_time_ns;             /* 周期时间(纳秒) */
    uint64_t cycle_time_extension_ns;   /* 周期延伸时间 */
} tsn_schedule_t;

/** TSN门控列表条目 */
typedef struct {
    uint32_t time_interval_ns;          /* 时间间隔(纳秒) */
    uint8_t gate_states;                /* 门状态位图(8个门) */
} tsn_gate_control_entry_t;

/** TSN配置结构 */
typedef struct {
    uint32_t enabled_features;          /* 启用的TSN功能标志 */
    tsn_schedule_t schedule;            /* 时间调度配置 */
    uint8_t num_gate_entries;           /* 门控条目数量 */
    tsn_gate_control_entry_t *gate_entries; /* 门控条目数组 */
} tsn_config_t;

/* ============================================================================
 * PHY驱动API
 * ============================================================================ */

/**
 * @brief 初始化汽车以太网PHY
 * @param config PHY配置
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_init(const automotive_phy_config_t *config);

/**
 * @brief 反初始化汽车以太网PHY
 */
void automotive_phy_deinit(void);

/**
 * @brief 启动PHY自动协商
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_auto_negotiation(void);

/**
 * @brief 获取PHY链路状态
 * @param status 状态输出
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_get_link_status(automotive_link_status_t *status);

/**
 * @brief 等待链路连接
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功，ETH_TIMEOUT超时
 */
eth_status_t automotive_phy_wait_for_link(uint32_t timeout_ms);

/**
 * @brief 执行链路诊断
 * @param diagnostics 诊断结果输出
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_run_diagnostics(automotive_phy_diagnostics_t *diagnostics);

/**
 * @brief 获取PHY统计信息
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_get_stats(automotive_phy_stats_t *stats);

/**
 * @brief 清除PHY统计信息
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_clear_stats(void);

/**
 * @brief 进入低功耗模式
 * @param wake_on_link 允许链路变化唤醒
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_enter_low_power(bool wake_on_link);

/**
 * @brief 退出低功耗模式
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_exit_low_power(void);

/**
 * @brief 设置PHY主从模式
 * @param master 启用主模式
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_set_master_mode(bool master);

/**
 * @brief 执行PHY软复位
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_soft_reset(void);

/**
 * @brief 配置EMC/EMI参数
 * @param enable_filter 启用滤波
 * @param slew_rate 信号斜率控制值
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_configure_emc(bool enable_filter, uint8_t slew_rate);

/**
 * @brief 启用/禁用远程唤醒
 * @param enable 启用/禁用
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_configure_wol(bool enable);

/* ============================================================================
 * MDIO操作API
 * ============================================================================ */

/**
 * @brief 读取PHY寄存器
 * @param reg_addr 寄存器地址
 * @param value 输出值
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_read_reg(uint8_t reg_addr, uint16_t *value);

/**
 * @brief 写入PHY寄存器
 * @param reg_addr 寄存器地址
 * @param value 写入值
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_write_reg(uint8_t reg_addr, uint16_t value);

/**
 * @brief 读取PHY扩展寄存器(MMD)
 * @param dev_addr 设备地址
 * @param reg_addr 寄存器地址
 * @param value 输出值
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_read_mmd(uint8_t dev_addr, uint16_t reg_addr, uint16_t *value);

/**
 * @brief 写入PHY扩展寄存器(MMD)
 * @param dev_addr 设备地址
 * @param reg_addr 寄存器地址
 * @param value 写入值
 * @return ETH_OK成功
 */
eth_status_t automotive_phy_write_mmd(uint8_t dev_addr, uint16_t reg_addr, uint16_t value);

/* ============================================================================
 * 特定PHY驱动API
 * ============================================================================ */

/**
 * @brief NXP TJA1101 PHY初始化
 * @param phy_addr MDIO地址
 * @return ETH_OK成功
 */
eth_status_t phy_tja1101_init(uint8_t phy_addr);

/**
 * @brief TI DP83TC811 PHY初始化
 * @param phy_addr MDIO地址
 * @return ETH_OK成功
 */
eth_status_t phy_dp83tc811_init(uint8_t phy_addr);

/**
 * @brief TI DP83TG720 PHY初始化 (1000BASE-T1)
 * @param phy_addr MDIO地址
 * @return ETH_OK成功
 */
eth_status_t phy_dp83tg720_init(uint8_t phy_addr);

/**
 * @brief Marvell 88Q2112 PHY初始化 (1000BASE-T1)
 * @param phy_addr MDIO地址
 * @return ETH_OK成功
 */
eth_status_t phy_marvell_88q2112_init(uint8_t phy_addr);

/* ============================================================================
 * TSN API预留
 * ============================================================================ */

/**
 * @brief 初始化TSN支持(预留接口)
 * @param config TSN配置
 * @return ETH_OK成功
 */
eth_status_t tsn_init(const tsn_config_t *config);

/**
 * @brief 启用/TXSN特定功能(预留接口)
 * @param feature TSN功能标志
 * @param enable 启用/禁用
 * @return ETH_OK成功
 */
eth_status_t tsn_enable_feature(tsn_features_t feature, bool enable);

/**
 * @brief 配置TSN门控列表(预留接口)
 * @param entries 门控条目数组
 * @param count 条目数量
 * @return ETH_OK成功
 */
eth_status_t tsn_configure_gate_control(const tsn_gate_control_entry_t *entries, uint8_t count);

/**
 * @brief 获取TSN时间戳(预留接口)
 * @param timestamp 时间戳输出(纳秒)
 * @return ETH_OK成功
 */
eth_status_t tsn_get_timestamp(uint64_t *timestamp);

/**
 * @brief 同步时间(预留接口)
 * @param domain_number gPTP域号
 * @return ETH_OK成功
 */
eth_status_t tsn_sync_time(uint8_t domain_number);

#ifdef __cplusplus
}
#endif

#endif /* ETH_AUTOMOTIVE_H */

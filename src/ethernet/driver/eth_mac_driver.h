/**
 * @file eth_mac_driver.h
 * @brief 车载以太网MAC驱动层 - 抽象MAC层接口
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 符合AUTOSAR MCAL规范
 * @note 支持功能安全ASIL-B等级
 * @note 汽车温度范围: -40°C ~ 125°C
 */

#ifndef ETH_MAC_DRIVER_H
#define ETH_MAC_DRIVER_H

#include "../../common/types/eth_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * AUTOSAR MCAL兼容定义
 * ============================================================================ */

/** 功能安全等级 */
typedef enum {
    ETH_ASIL_QM = 0,    /* 质量管理 */
    ETH_ASIL_A,         /* ASIL A */
    ETH_ASIL_B,         /* ASIL B */
    ETH_ASIL_C,         /* ASIL C */
    ETH_ASIL_D,         /* ASIL D */
} eth_asil_level_t;

/** 服务保护机制 */
typedef enum {
    ETH_PROT_NONE = 0,
    ETH_PROT_ECC,       /* 错误校验和纠正 */
    ETH_PROT_CRC32,     /* CRC32保护 */
    ETH_PROT_SAFEGUARD, /* 安全监控 */
} eth_protection_t;

/* ============================================================================
 * MAC控制器类型定义
 * ============================================================================ */

/** 支持的MAC芯片类型 */
typedef enum {
    ETH_MAC_TYPE_GENERIC = 0,   /* 通用MAC控制器 */
    ETH_MAC_TYPE_DW_GMAC,       /* Synopsys DesignWare GMAC */
    ETH_MAC_TYPE_NXP_S32,       /* NXP S32系列车载以太网 */
    ETH_MAC_TYPE_INFINEON_AURIX,/* Infineon AURIX TC3xx */
    ETH_MAC_TYPE_TI_CPSW,       /* TI CPSW (Jacinto/TDA4) */
    ETH_MAC_TYPE_RENESAS_R_GBE, /* Renesas R-Car GbE */
    ETH_MAC_TYPE_BROADCOM_BCM,  /* Broadcom车载以太网 */
} eth_mac_type_t;

/** MAC工作模式 */
typedef enum {
    ETH_MAC_MODE_NORMAL = 0,    /* 正常模式 */
    ETH_MAC_MODE_LOOPBACK,      /* 回环测试模式 */
    ETH_MAC_MODE_PROMISCUOUS,   /* 混杂模式(接收所有帧) */
    ETH_MAC_MODE_MULTICAST_ONLY,/* 仅组播模式 */
    ETH_MAC_MODE_LOW_POWER,     /* 低功耗模式 */
} eth_mac_mode_t;

/** MAC帧过滤配置 */
typedef struct {
    bool enable_broadcast_filter;   /* 启用广播过滤 */
    bool enable_multicast_filter;   /* 启用组播过滤 */
    bool enable_unicast_filter;     /* 启用单播过滤 */
    bool enable_vlan_filter;        /* 启用VLAN过滤 */
    bool drop_crc_errors;           /* 丢弃CRC错误帧 */
    bool drop_runt_frames;          /* 丢弃短帧(<64字节) */
    bool drop_oversized;            /* 丢弃超长帧 */
    uint32_t max_frame_size;        /* 最大帧大小(默认1522) */
} eth_mac_filter_t;

/** MAC中断类型 */
typedef enum {
    ETH_MAC_INT_TX_COMPLETE = 0,    /* 发送完成 */
    ETH_MAC_INT_TX_ERROR,           /* 发送错误 */
    ETH_MAC_INT_RX_COMPLETE,        /* 接收完成 */
    ETH_MAC_INT_RX_ERROR,           /* 接收错误 */
    ETH_MAC_INT_RX_OVERRUN,         /* 接收溢出 */
    ETH_MAC_INT_LINK_CHANGE,        /* 链路状态变化 */
    ETH_MAC_INT_BUS_ERROR,          /* 总线错误 */
    ETH_MAC_INT_PMT,                /* 电源管理事件 */
    ETH_MAC_INT_TIMESTAMP,          /* 时间戳事件 */
    ETH_MAC_INT_COUNT
} eth_mac_interrupt_t;

/** MAC统计信息 */
typedef struct {
    uint64_t tx_frames;             /* 发送帧数 */
    uint64_t tx_bytes;              /* 发送字节数 */
    uint64_t tx_errors;             /* 发送错误数 */
    uint64_t tx_underrun;           /* 发送下溢错误 */
    uint64_t rx_frames;             /* 接收帧数 */
    uint64_t rx_bytes;              /* 接收字节数 */
    uint64_t rx_errors;             /* 接收错误数 */
    uint64_t rx_crc_errors;         /* CRC错误数 */
    uint64_t rx_align_errors;       /* 对齐错误数 */
    uint64_t rx_overrun;            /* 接收溢出数 */
    uint64_t rx_missed;             /* 丢包数 */
    uint64_t multicast_frames;      /* 组播帧数 */
    uint64_t broadcast_frames;      /* 广播帧数 */
    uint64_t pause_frames;          /* 暂停帧数 */
} eth_mac_stats_t;

/* ============================================================================
 * MAC控制器配置
 * ============================================================================ */

/** MAC控制器配置结构 */
typedef struct {
    eth_mac_type_t mac_type;        /* MAC控制器类型 */
    eth_mac_addr_t mac_addr;        /* MAC地址 */
    eth_mode_t speed_mode;          /* 速率模式 */
    eth_mac_mode_t mac_mode;        /* MAC工作模式 */
    eth_mac_filter_t filter;        /* 帧过滤配置 */
    eth_asil_level_t asil_level;    /* 功能安全等级 */
    eth_protection_t protection;    /* 保护机制 */

    /* 硬件参数 */
    uint32_t base_addr;             /* 寄存器基地址 */
    uint32_t irq_num;               /* 中断号 */
    uint32_t clock_freq_hz;         /* 时钟频率(Hz) */
    uint32_t mdio_clock_hz;         /* MDIO时钟频率(Hz) */

    /* 性能参数 */
    uint8_t tx_fifo_depth;          /* 发送FIFO深度 */
    uint8_t rx_fifo_depth;          /* 接收FIFO深度 */
    bool enable_jumbo_frames;       /* 启用巨型帧 */
    bool enable_timestamp;          /* 启用时间戳 */
    bool enable_checksum_offload;   /* 启用硬件校验和卸载 */

    /* EMC/EMI配置 */
    bool enable_emc_filter;         /* 启用EMC滤波 */
    uint8_t slew_rate;              /* 信号斜率控制 */
} eth_mac_config_t;

/* ============================================================================
 * MAC回调函数类型
 * ============================================================================ */

/** MAC中断回调函数 */
typedef void (*eth_mac_isr_callback_t)(eth_mac_interrupt_t type, void *user_data);

/** MAC帧接收回调 */
typedef void (*eth_mac_rx_callback_t)(uint8_t *data, uint32_t len, uint64_t timestamp, void *user_data);

/* ============================================================================
 * MAC驱动API - 通用接口
 * ============================================================================ */

/**
 * @brief 初始化MAC控制器
 * @param config MAC配置参数
 * @return ETH_OK成功，其他失败
 */
eth_status_t eth_mac_init(const eth_mac_config_t *config);

/**
 * @brief 反初始化MAC控制器
 */
void eth_mac_deinit(void);

/**
 * @brief 启动MAC控制器
 * @return ETH_OK成功
 */
eth_status_t eth_mac_start(void);

/**
 * @brief 停止MAC控制器
 * @return ETH_OK成功
 */
eth_status_t eth_mac_stop(void);

/**
 * @brief 设置MAC地址
 * @param mac_addr MAC地址
 * @return ETH_OK成功
 */
eth_status_t eth_mac_set_address(const eth_mac_addr_t mac_addr);

/**
 * @brief 获取MAC地址
 * @param mac_addr MAC地址输出缓冲区
 * @return ETH_OK成功
 */
eth_status_t eth_mac_get_address(eth_mac_addr_t mac_addr);

/**
 * @brief 配置MAC工作模式
 * @param mode 工作模式
 * @return ETH_OK成功
 */
eth_status_t eth_mac_set_mode(eth_mac_mode_t mode);

/**
 * @brief 配置帧过滤
 * @param filter 过滤配置
 * @return ETH_OK成功
 */
eth_status_t eth_mac_set_filter(const eth_mac_filter_t *filter);

/**
 * @brief 配置中断
 * @param type 中断类型
 * @param enable 启用/禁用
 * @return ETH_OK成功
 */
eth_status_t eth_mac_configure_interrupt(eth_mac_interrupt_t type, bool enable);

/**
 * @brief 注册中断回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_mac_register_isr_callback(eth_mac_isr_callback_t callback, void *user_data);

/**
 * @brief 处理MAC中断
 * @return ETH_OK成功
 */
eth_status_t eth_mac_handle_interrupt(void);

/**
 * @brief 获取MAC统计信息
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t eth_mac_get_stats(eth_mac_stats_t *stats);

/**
 * @brief 清除MAC统计信息
 * @return ETH_OK成功
 */
eth_status_t eth_mac_clear_stats(void);

/* ============================================================================
 * MAC驱动API - 数据收发
 * ============================================================================ */

/**
 * @brief 发送以太网帧
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功
 */
eth_status_t eth_mac_transmit(const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 接收以太网帧
 * @param buffer 接收缓冲区
 * @param max_len 缓冲区最大长度
 * @param received_len 实际接收长度输出
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功
 */
eth_status_t eth_mac_receive(uint8_t *buffer, uint32_t max_len,
                              uint32_t *received_len, uint32_t timeout_ms);

/**
 * @brief 注册接收回调(中断模式)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_mac_register_rx_callback(eth_mac_rx_callback_t callback, void *user_data);

/* ============================================================================
 * MAC驱动API - 硬件特定接口
 * ============================================================================ */

/**
 * @brief 特定芯片初始化 - Synopsys DW GMAC
 * @param config MAC配置
 * @return ETH_OK成功
 */
eth_status_t eth_mac_dw_gmac_init(const eth_mac_config_t *config);

/**
 * @brief 特定芯片初始化 - NXP S32
 * @param config MAC配置
 * @return ETH_OK成功
 */
eth_status_t eth_mac_nxp_s32_init(const eth_mac_config_t *config);

/**
 * @brief 特定芯片初始化 - Infineon AURIX
 * @param config MAC配置
 * @return ETH_OK成功
 */
eth_status_t eth_mac_infineon_aurix_init(const eth_mac_config_t *config);

/**
 * @brief 特定芯片初始化 - TI CPSW
 * @param config MAC配置
 * @return ETH_OK成功
 */
eth_status_t eth_mac_ti_cpsw_init(const eth_mac_config_t *config);

/**
 * @brief 设置MAC时钟分频
 * @param divider 分频值
 * @return ETH_OK成功
 */
eth_status_t eth_mac_set_clock_divider(uint32_t divider);

/**
 * @brief 进入低功耗模式
 * @return ETH_OK成功
 */
eth_status_t eth_mac_enter_low_power(void);

/**
 * @brief 退出低功耗模式
 * @return ETH_OK成功
 */
eth_status_t eth_mac_exit_low_power(void);

/**
 * @brief 配置EMC/EMI参数
 * @param enable_filter 启用滤波
 * @param slew_rate 信号斜率
 * @return ETH_OK成功
 */
eth_status_t eth_mac_configure_emc(bool enable_filter, uint8_t slew_rate);

/**
 * @brief 执行MAC自检
 * @return ETH_OK通过，其他失败
 */
eth_status_t eth_mac_self_test(void);

/**
 * @brief 获取MAC错误状态
 * @param error_code 错误代码输出
 * @return ETH_OK成功
 */
eth_status_t eth_mac_get_error_status(uint32_t *error_code);

/**
 * @brief 清除MAC错误状态
 * @return ETH_OK成功
 */
eth_status_t eth_mac_clear_error_status(void);

#ifdef __cplusplus
}
#endif

#endif /* ETH_MAC_DRIVER_H */

/**
 * @file eth_dma.h
 * @brief 车载以太网DMA管理 - 描述符环形缓冲区
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 收发分离的环形队列设计
 * @note 支持中断/轮询模式
 * @note 符合AUTOSAR MCAL规范
 */

#ifndef ETH_DMA_H
#define ETH_DMA_H

#include "../../common/types/eth_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DMA配置常量
 * ============================================================================ */

/** 默认DMA描述符数量 */
#define ETH_DMA_DEFAULT_RX_DESC_COUNT   16
#define ETH_DMA_DEFAULT_TX_DESC_COUNT   16

/** 最大DMA描述符数量 */
#define ETH_DMA_MAX_RX_DESC_COUNT       256
#define ETH_DMA_MAX_TX_DESC_COUNT       256

/** 默认缓冲区大小 */
#define ETH_DMA_DEFAULT_BUFFER_SIZE     1536    /* 包含VLAN标签的标准帧 */
#define ETH_DMA_JUMBO_BUFFER_SIZE       9018    /* 巨型帧支持 */

/** DMA对齐要求 */
#define ETH_DMA_ALIGN                   4
#define ETH_DMA_DESC_ALIGN              4

/* ============================================================================
 * DMA描述符状态位定义
 * ============================================================================ */

/** 接收描述符状态 */
#define ETH_RX_DESC_OWN         0x80000000U  /* DMA拥有描述符 */
#define ETH_RX_DESC_ES          0x00008000U  /* 错误汇总 */
#define ETH_RX_DESC_LS          0x00000100U  /* 最后描述符 */
#define ETH_RX_DESC_FS          0x00000200U  /* 第一个描述符 */
#define ETH_RX_DESC_VLAN        0x00000400U  /* VLAN标签 */
#define ETH_RX_DESC_FT          0x00002000U  /* 帧类型 */
#define ETH_RX_DESC_FL_MASK     0x3FFF0000U  /* 帧长度掩码 */
#define ETH_RX_DESC_FL_SHIFT    16

/** 发送描述符状态 */
#define ETH_TX_DESC_OWN         0x80000000U  /* DMA拥有描述符 */
#define ETH_TX_DESC_IC          0x40000000U  /* 中断使能 */
#define ETH_TX_DESC_LS          0x20000000U  /* 最后描述符 */
#define ETH_TX_DESC_FS          0x10000000U  /* 第一个描述符 */
#define ETH_TX_DESC_CIC_MASK    0x00C00000U  /* 校验和插入控制 */
#define ETH_TX_DESC_TER         0x00200000U  /* 环形终端 */
#define ETH_TX_DESC_TCH         0x00100000U  /* 二级描述符链接 */
#define ETH_TX_DESC_TBS_MASK    0x00001FFFU  /* 发送缓冲区大小掩码 */

/* ============================================================================
 * DMA描述符结构
 * ============================================================================ */

/** DMA接收描述符(4字长) */
typedef struct {
    volatile uint32_t rd0;  /* 状态和接收字节数 */
    volatile uint32_t rd1;  /* 缓冲区2长度和缓冲区1长度 */
    volatile uint32_t rd2;  /* 缓冲区1地址 */
    volatile uint32_t rd3;  /* 缓冲区2地址 */
} eth_dma_rx_desc_t;

/** DMA发送描述符(4字长) */
typedef struct {
    volatile uint32_t td0;  /* 状态和控制 */
    volatile uint32_t td1;  /* 缓冲区2长度和缓冲区1长度 */
    volatile uint32_t td2;  /* 缓冲区1地址 */
    volatile uint32_t td3;  /* 缓冲区2地址 */
} eth_dma_tx_desc_t;

/* ============================================================================
 * DMA环形缓冲区状态
 * ============================================================================ */

/** DMA环形缓冲区状态 */
typedef enum {
    ETH_DMA_STATE_UNINIT = 0,
    ETH_DMA_STATE_INIT,
    ETH_DMA_STATE_RUNNING,
    ETH_DMA_STATE_STOPPED,
    ETH_DMA_STATE_ERROR
} eth_dma_state_t;

/** DMA错误类型 */
typedef enum {
    ETH_DMA_ERR_NONE = 0,
    ETH_DMA_ERR_BUS,            /* 总线错误 */
    ETH_DMA_ERR_DESC,           /* 描述符错误 */
    ETH_DMA_ERR_BUFFER,         /* 缓冲区错误 */
    ETH_DMA_ERR_OVERRUN,        /* 接收溢出 */
    ETH_DMA_ERR_UNDERRUN,       /* 发送下溢 */
    ETH_DMA_ERR_TIMEOUT,        /* 超时 */
    ETH_DMA_ERR_OWNERSHIP,      /* 所有权错误 */
} eth_dma_error_t;

/** DMA操作模式 */
typedef enum {
    ETH_DMA_MODE_INTERRUPT = 0, /* 中断模式 */
    ETH_DMA_MODE_POLLING,       /* 轮询模式 */
    ETH_DMA_MODE_MIXED          /* 混合模式 */
} eth_dma_mode_t;

/* ============================================================================
 * DMA配置结构
 * ============================================================================ */

/** DMA配置结构 */
typedef struct {
    /* 描述符配置 */
    uint16_t rx_desc_count;         /* 接收描述符数量 */
    uint16_t tx_desc_count;         /* 发送描述符数量 */
    uint32_t buffer_size;           /* 缓冲区大小 */

    /* 操作模式 */
    eth_dma_mode_t mode;            /* DMA操作模式 */
    bool enable_jumbo;              /* 启用巨型帧 */

    /* 中断配置 */
    bool rx_interrupt;              /* 接收中断使能 */
    bool tx_interrupt;              /* 发送中断使能 */
    bool error_interrupt;           /* 错误中断使能 */
    uint8_t rx_threshold;           /* 接收阈值(描述符数) */

    /* 流控配置(防止总线占用) */
    bool enable_flow_control;       /* 启用流控 */
    uint8_t burst_length;           /* DMA突发传输长度 */
    bool fixed_burst;               /* 固定突发传输 */

    /* 数据对齐 */
    bool enable_aligned;            /* 启用数据对齐 */
} eth_dma_config_t;

/** DMA统计信息 */
typedef struct {
    uint32_t rx_packets;            /* 接收包数 */
    uint32_t tx_packets;            /* 发送包数 */
    uint32_t rx_bytes;              /* 接收字节数 */
    uint32_t tx_bytes;              /* 发送字节数 */
    uint32_t rx_dropped;            /* 丢弃的接收帧 */
    uint32_t tx_dropped;            /* 丢弃的发送帧 */
    uint32_t rx_errors;             /* 接收错误 */
    uint32_t tx_errors;             /* 发送错误 */
    uint32_t overrun_count;         /* 溢出计数 */
    uint32_t underrun_count;        /* 下溢计数 */
} eth_dma_stats_t;

/* ============================================================================
 * DMA数据包结构
 * ============================================================================ */

/** DMA接收数据包 */
typedef struct {
    uint8_t *buffer;                /* 缓冲区指针 */
    uint32_t length;                /* 数据长度 */
    uint64_t timestamp;             /* 接收时间戳(纳秒) */
    uint32_t status;                /* 描述符状态 */
    bool valid;                     /* 数据有效标志 */
} eth_dma_rx_packet_t;

/** DMA发送数据包 */
typedef struct {
    const uint8_t *buffer;          /* 缓冲区指针 */
    uint32_t length;                /* 数据长度 */
    uint32_t flags;                 /* 发送标志 */
    bool ready;                     /* 准备好标志 */
} eth_dma_tx_packet_t;

/* ============================================================================
 * DMA回调函数类型
 * ============================================================================ */

/** DMA接收完成回调 */
typedef void (*eth_dma_rx_complete_t)(eth_dma_rx_packet_t *packet, void *user_data);

/** DMA发送完成回调 */
typedef void (*eth_dma_tx_complete_t)(uint32_t count, void *user_data);

/** DMA错误回调 */
typedef void (*eth_dma_error_t_callback)(eth_dma_error_t error, void *user_data);

/* ============================================================================
 * DMA公共API
 * ============================================================================ */

/**
 * @brief 初始化DMA控制器和环形缓冲区
 * @param config DMA配置参数
 * @return ETH_OK成功
 */
eth_status_t eth_dma_init(const eth_dma_config_t *config);

/**
 * @brief 反初始化DMA控制器
 */
void eth_dma_deinit(void);

/**
 * @brief 启动DMA传输
 * @return ETH_OK成功
 */
eth_status_t eth_dma_start(void);

/**
 * @brief 停止DMA传输
 * @return ETH_OK成功
 */
eth_status_t eth_dma_stop(void);

/**
 * @brief 重置DMA控制器
 * @return ETH_OK成功
 */
eth_status_t eth_dma_reset(void);

/* ============================================================================
 * DMA接收API
 * ============================================================================ */

/**
 * @brief 获取接收描述符(轮询模式)
 * @param packet 输出接收数据包
 * @param timeout_ms 超时时间
 * @return ETH_OK成功，ETH_TIMEOUT超时
 */
eth_status_t eth_dma_rx_get_packet(eth_dma_rx_packet_t *packet, uint32_t timeout_ms);

/**
 * @brief 释放接收描述符给DMA
 * @param desc_index 描述符索引
 * @return ETH_OK成功
 */
eth_status_t eth_dma_rx_release_desc(uint16_t desc_index);

/**
 * @brief 注册接收完成回调(中断模式)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_dma_register_rx_callback(eth_dma_rx_complete_t callback, void *user_data);

/**
 * @brief 处理接收中断
 * @return ETH_OK成功
 */
eth_status_t eth_dma_handle_rx_interrupt(void);

/**
 * @brief 检查是否有可用的接收帧
 * @param available 输出可用帧数量
 * @return ETH_OK成功
 */
eth_status_t eth_dma_rx_check_available(uint16_t *available);

/* ============================================================================
 * DMA发送API
 * ============================================================================ */

/**
 * @brief 队列数据到发送环形缓冲区
 * @param packet 发送数据包
 * @param timeout_ms 超时时间
 * @return ETH_OK成功，ETH_TIMEOUT超时，ETH_BUSY缓冲区满
 */
eth_status_t eth_dma_tx_queue_packet(const eth_dma_tx_packet_t *packet, uint32_t timeout_ms);

/**
 * @brief 强制触发发送
 * @return ETH_OK成功
 */
eth_status_t eth_dma_tx_trigger(void);

/**
 * @brief 检查发送完成状态
 * @param completed 输出已完成帧数量
 * @return ETH_OK成功
 */
eth_status_t eth_dma_tx_check_complete(uint16_t *completed);

/**
 * @brief 注册发送完成回调(中断模式)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_dma_register_tx_callback(eth_dma_tx_complete_t callback, void *user_data);

/**
 * @brief 处理发送中断
 * @return ETH_OK成功
 */
eth_status_t eth_dma_handle_tx_interrupt(void);

/**
 * @brief 获取可用发送描述符数量
 * @param available 输出可用描述符数
 * @return ETH_OK成功
 */
eth_status_t eth_dma_tx_get_available(uint16_t *available);

/* ============================================================================
 * DMA管理API
 * ============================================================================ */

/**
 * @brief 获取DMA统计信息
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t eth_dma_get_stats(eth_dma_stats_t *stats);

/**
 * @brief 清除DMA统计信息
 * @return ETH_OK成功
 */
eth_status_t eth_dma_clear_stats(void);

/**
 * @brief 获取DMA当前状态
 * @param state 状态输出
 * @return ETH_OK成功
 */
eth_status_t eth_dma_get_state(eth_dma_state_t *state);

/**
 * @brief 获取DMA错误状态
 * @param error 错误类型输出
 * @return ETH_OK成功
 */
eth_status_t eth_dma_get_error(eth_dma_error_t *error);

/**
 * @brief 清除DMA错误状态
 * @return ETH_OK成功
 */
eth_status_t eth_dma_clear_error(void);

/**
 * @brief 配置DMA中断
 * @param enable_rx 启用接收中断
 * @param enable_tx 启用发送中断
 * @param enable_err 启用错误中断
 * @return ETH_OK成功
 */
eth_status_t eth_dma_configure_interrupt(bool enable_rx, bool enable_tx, bool enable_err);

/**
 * @brief 处理DMA错误中断
 * @return ETH_OK成功
 */
eth_status_t eth_dma_handle_error_interrupt(void);

/**
 * @brief 注册错误回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_dma_register_error_callback(eth_dma_error_t_callback callback, void *user_data);

/* ============================================================================
 * DMA内存管理API
 * ============================================================================ */

/**
 * @brief 分配DMA对齐的缓冲区
 * @param size 缓冲区大小
 * @return 缓冲区指针，NULL表示失败
 */
void* eth_dma_alloc_buffer(uint32_t size);

/**
 * @brief 释放DMA缓冲区
 * @param buffer 缓冲区指针
 */
void eth_dma_free_buffer(void *buffer);

/**
 * @brief 使DMA缓冲区无效(用于CPU访问后)
 * @param buffer 缓冲区指针
 * @param size 大小
 * @return ETH_OK成功
 */
eth_status_t eth_dma_invalidate_buffer(void *buffer, uint32_t size);

/**
 * @brief 使DMA缓冲区有效(用于DMA访问前)
 * @param buffer 缓冲区指针
 * @param size 大小
 * @return ETH_OK成功
 */
eth_status_t eth_dma_flush_buffer(void *buffer, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* ETH_DMA_H */

/**
 * @file shm_transport.h
 * @brief 共享内存传输层 - 同节点进程间通信
 * @version 1.0
 * @date 2026-04-24
 * 
 * @note 无拷贝数据传输
 * @note 环形缓冲区实现
 * @note 适配车载实时系统
 */

#ifndef SHM_TRANSPORT_H
#define SHM_TRANSPORT_H

#include "transport_interface.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 共享内存配置定义
 * ============================================================================ */

/** 共享内存魔法字 */
#define SHM_MAGIC_COOKIE        0x53484D44  /* "SHMD" */
#define SHM_VERSION             0x0100      /* 版本 1.0 */

/** 默认配置值 */
#define SHM_DEFAULT_RING_SIZE   (16 * 1024 * 1024)  /* 默认环形缓冲区16MB */
#define SHM_MAX_RING_SIZE       (256 * 1024 * 1024) /* 最大环形缓冲区256MB */
#define SHM_MIN_RING_SIZE       (64 * 1024)         /* 最小环形缓冲区64KB */
#define SHM_MAX_CHANNELS        32                  /* 最大通道数 */
#define SHM_MAX_WAITERS         8                   /* 最大等待者数 */
#define SHM_ALIGN_SIZE          64                  /* 内存对齐(缓存行大小) */

/** 无拷贝传输最大消息大小 */
#define SHM_MAX_MESSAGE_SIZE    (4 * 1024 * 1024)   /* 单消息最大 4MB */
#define SHM_MAX_MESSAGE_COUNT   1024                /* 最大消息数 */

/* ============================================================================
 * 车载优化配置
 * ============================================================================ */

/** 进程间优先级 */
typedef enum {
    SHM_PRIO_CRITICAL = 0,    /* 关键任务 - 制动/转向 */
    SHM_PRIO_HIGH,            /* 高优先级 - ADAS */
    SHM_PRIO_MEDIUM,          /* 中等优先级 - 信息娱乐 */
    SHM_PRIO_LOW,             /* 低优先级 - 诊断日志 */
    SHM_PRIO_COUNT
} shm_priority_t;

/** 确定性模式 */
typedef enum {
    SHM_MODE_STANDARD = 0,    /* 标准模式 */
    SHM_MODE_LOCK_FREE,       /* 无锁模式 */
    SHM_MODE_WAIT_FREE        /* 无等待模式 */
} shm_deterministic_mode_t;

/** 同步类型 */
typedef enum {
    SHM_SYNC_MUTEX = 0,       /* 互斥锁 */
    SHM_SYNC_SPINLOCK,        /* 自旋锁 */
    SHM_SYNC_FUTEX,           /* Futex(快速用户态互斥) */
    SHM_SYNC_NONE             /* 无同步(纯无锁) */
} shm_sync_type_t;

/* ============================================================================
 * 共享内存配置结构
 * ============================================================================ */

/** 共享内存传输配置 */
typedef struct {
    transport_config_t base;
    
    /* 基础配置 */
    char shm_name[64];                  /* 共享内存名称 */
    uint32_t ring_buffer_size;          /* 环形缓冲区大小 */
    uint32_t max_channels;              /* 最大通道数 */
    
    /* 性能优化 */
    shm_deterministic_mode_t det_mode;  /* 确定性模式 */
    shm_sync_type_t sync_type;          /* 同步类型 */
    uint8_t cache_line_size;            /* CPU缓存行大小 */
    
    /* 进程组配置 */
    pid_t producer_pid;                 /* 生产者进程ID */
    pid_t consumer_pid;                 /* 消费者进程ID */
    bool allow_multi_producer;          /* 允许多生产者 */
    bool allow_multi_consumer;          /* 允许多消费者 */
    
    /* 车载特定 */
    uint32_t deadline_us;               /* 截止时间限制(微秒) */
    bool enable_deadline;               /* 启用截止时间检查 */
    uint8_t cpu_affinity;               /* CPU亲和性掩码 */
    
    /* 预留字段 */
    uint32_t reserved[8];
    
} shm_transport_config_t;

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/** 共享内存消息头部 */
typedef struct __attribute__((packed, aligned(8))) {
    uint32_t magic;             /* 魔法字 */
    uint32_t version;           /* 版本 */
    uint64_t timestamp_ns;      /* 时间戳(纳秒) */
    uint32_t seq_num;           /* 序列号 */
    uint32_t payload_size;      /* 载荷大小 */
    uint32_t checksum;          /* 校验和 */
    uint8_t priority;           /* 优先级 */
    uint8_t flags;              /* 标志位 */
    uint16_t reserved;          /* 预留 */
} shm_message_header_t;

/** 消息标志位 */
#define SHM_FLAG_VALID          0x01    /* 消息有效 */
#define SHM_FLAG_END_OF_BURST   0x02    /* 突发结束 */
#define SHM_FLAG_HIGH_PRIORITY  0x04    /* 高优先级 */
#define SHM_FLAG_TIMESTAMPED    0x08    /* 带时间戳 */

/** 环形缓冲区状态 */
typedef struct {
    uint64_t write_pos;         /* 写入位置 */
    uint64_t read_pos;          /* 读取位置 */
    uint64_t watermark_high;    /* 高水位 */
    uint64_t watermark_low;     /* 低水位 */
    uint32_t message_count;     /* 当前消息数 */
    uint32_t reserved;          /* 内存对齐 */
} shm_ring_state_t;

/* ============================================================================
 * 统计信息
 * ============================================================================ */

/** 共享内存传输统计 */
typedef struct {
    uint64_t tx_messages;       /* 发送消息数 */
    uint64_t tx_bytes;          /* 发送字节数 */
    uint64_t rx_messages;       /* 接收消息数 */
    uint64_t rx_bytes;          /* 接收字节数 */
    uint64_t tx_drops;          /* 发送丢弃数 */
    uint64_t rx_drops;          /* 接收丢弃数 */
    uint64_t overruns;          /* 溢出数 */
    uint64_t underruns;         /* 下溢数 */
    uint64_t lock_contention;   /* 锁竞争次数 */
    uint32_t latency_min_us;    /* 最小延迟 */
    uint32_t latency_max_us;    /* 最大延迟 */
    uint32_t latency_avg_us;    /* 平均延迟 */
    uint32_t buffer_utilization; /* 缓冲区利用率(百分比) */
} shm_transport_stats_t;

/* ============================================================================
 * 共享内存API
 * ============================================================================ */

/**
 * @brief 创建共享内存传输实例
 * @param config 共享内存传输配置
 * @return 传输句柄，NULL表示失败
 */
transport_handle_t* shm_transport_create(const shm_transport_config_t *config);

/**
 * @brief 销毁共享内存传输实例
 * @param handle 传输句柄
 * @return ETH_OK成功
 */
eth_status_t shm_transport_destroy(transport_handle_t *handle);

/**
 * @brief 无拷贝发送数据
 * @param handle 传输句柄
 * @param data 数据指针
 * @param len 数据长度
 * @param priority 优先级
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 * @note 数据直接写入共享内存，无内存拷贝
 */
eth_status_t shm_transport_send_zero_copy(transport_handle_t *handle,
                                           void *data,
                                           uint32_t len,
                                           shm_priority_t priority,
                                           uint32_t timeout_ms);

/**
 * @brief 无拷贝接收数据
 * @param handle 传输句柄
 * @param data_out 输出数据指针
 * @param len_out 输出数据长度
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 * @note 返回的指针指向共享内存，无需拷贝
 */
eth_status_t shm_transport_receive_zero_copy(transport_handle_t *handle,
                                              void **data_out,
                                              uint32_t *len_out,
                                              uint32_t timeout_ms);

/**
 * @brief 释放接收的数据区域
 * @param handle 传输句柄
 * @param data 数据指针
 * @return ETH_OK成功
 * @note 必须在处理完数据后调用此函数
 */
eth_status_t shm_transport_release_buffer(transport_handle_t *handle,
                                           void *data);

/**
 * @brief 获取可写入的共享内存区域
 * @param handle 传输句柄
 * @param buffer 输出缓冲区指针
 * @param max_len 最大可写长度
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t shm_transport_get_write_buffer(transport_handle_t *handle,
                                             void **buffer,
                                             uint32_t max_len,
                                             uint32_t timeout_ms);

/**
 * @brief 提交写入的数据
 * @param handle 传输句柄
 * @param buffer 缓冲区指针(由get_write_buffer获取)
 * @param actual_len 实际写入长度
 * @param priority 优先级
 * @return ETH_OK成功
 */
eth_status_t shm_transport_commit_write(transport_handle_t *handle,
                                         void *buffer,
                                         uint32_t actual_len,
                                         shm_priority_t priority);

/**
 * @brief 强制刷新缓冲区
 * @param handle 传输句柄
 * @return ETH_OK成功
 */
eth_status_t shm_transport_flush(transport_handle_t *handle);

/**
 * @brief 获取缓冲区状态
 * @param handle 传输句柄
 * @param free_space 空闲空间大小输出
 * @param used_space 已用空间大小输出
 * @return ETH_OK成功
 */
eth_status_t shm_transport_get_buffer_status(transport_handle_t *handle,
                                              uint32_t *free_space,
                                              uint32_t *used_space);

/**
 * @brief 获取传输统计信息
 * @param handle 传输句柄
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t shm_transport_get_stats(transport_handle_t *handle,
                                      shm_transport_stats_t *stats);

/**
 * @brief 清除传输统计
 * @param handle 传输句柄
 * @return ETH_OK成功
 */
eth_status_t shm_transport_clear_stats(transport_handle_t *handle);

/**
 * @brief 设置确定性模式
 * @param handle 传输句柄
 * @param mode 确定性模式
 * @return ETH_OK成功
 */
eth_status_t shm_transport_set_deterministic_mode(transport_handle_t *handle,
                                                   shm_deterministic_mode_t mode);

/**
 * @brief 设置CPU亲和性
 * @param handle 传输句柄
 * @param cpu_mask CPU亲和性掩码
 * @return ETH_OK成功
 */
eth_status_t shm_transport_set_cpu_affinity(transport_handle_t *handle,
                                             uint8_t cpu_mask);

/**
 * @brief 预先分配缓冲区
 * @param handle 传输句柄
 * @param min_free_space 最小空闲空间要求
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t shm_transport_preallocate(transport_handle_t *handle,
                                        uint32_t min_free_space,
                                        uint32_t timeout_ms);

/**
 * @brief 使用poll等待接收数据
 * @param handle 传输句柄
 * @param timeout_ms 超时时间
 * @return ETH_OK表示有数据可读
 */
eth_status_t shm_transport_poll(transport_handle_t *handle,
                                 uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* SHM_TRANSPORT_H */

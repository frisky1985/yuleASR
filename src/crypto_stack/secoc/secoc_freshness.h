/**
 * @file secoc_freshness.h
 * @brief SecOC Freshness Value Management Module
 * @version 1.0
 * @date 2026-04-25
 *
 * 实现AUTOSAR SecOC 4.4规范的Freshness值管理
 * 支持计数器、时间戳和行程计数器模式
 * 支持Master-Slave同步机制
 */

#ifndef SECOC_FRESHNESS_H
#define SECOC_FRESHNESS_H

#include "secoc_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */

#define SECOC_FV_MAX_ENTRIES            64
#define SECOC_SYNC_MASTER_INTERVAL_MS   100     /* Master发送同步消息间隔 */
#define SECOC_SYNC_RETRY_COUNT          3       /* 同步重试次数 */
#define SECOC_FRESHNESS_TOLERANCE       5       /* 允许的Freshness值滞后 */

/* 时间戳模式配置 */
#define SECOC_TS_RESOLUTION_US          1000    /* 时间戳分辨率 (微秒) */
#define SECOC_TS_MAX_DRIFT_MS           100     /* 最大时钟偏移 (毫秒) */

/* 行程计数器配置 */
#define SECOC_TRIP_MAX_VALUE            0xFFFFFFFF
#define SECOC_RESET_MAX_VALUE           0xFF

/* ============================================================================
 * Freshness值状态
 * ============================================================================ */

typedef enum {
    SECOC_FV_STATE_OK = 0,
    SECOC_FV_STATE_NOT_SYNC,            /* 未同步 */
    SECOC_FV_STATE_SYNC_IN_PROGRESS,    /* 同步中 */
    SECOC_FV_STATE_SYNC_FAILED,         /* 同步失败 */
    SECOC_FV_STATE_REPLAY_DETECTED,     /* 检测到重放 */
    SECOC_FV_STATE_OVERFLOW             /* 计数器溢出 */
} secoc_fv_state_t;

/* ============================================================================
 * 同步消息类型
 * ============================================================================ */

typedef enum {
    SECOC_SYNC_REQ = 0,                 /* 同步请求 (Slave -> Master) */
    SECOC_SYNC_RES,                     /* 同步响应 (Master -> Slave) */
    SECOC_SYNC_BROADCAST                /* 同步广播 (Master定期发送) */
} secoc_sync_msg_type_t;

/* 同步消息头部 */
typedef struct __attribute__((packed)) {
    uint8_t msg_type;                   /* secoc_sync_msg_type_t */
    uint8_t pdu_id_high;
    uint8_t pdu_id_low;
    uint8_t reserved;
} secoc_sync_header_t;

/* 同步请求 */
typedef struct __attribute__((packed)) {
    secoc_sync_header_t header;
    uint32_t slave_fv_low;              /* Slave的低位FV */
    uint32_t timestamp;                 /* Slave时间戳 */
} secoc_sync_request_t;

/* 同步响应/广播 */
typedef struct __attribute__((packed)) {
    secoc_sync_header_t header;
    uint64_t master_fv;                 /* Master的完整FV */
    uint32_t trip_counter;              /* 行程计数器 */
    uint32_t reset_counter;             /* 复位计数器 */
    uint32_t master_timestamp;          /* Master时间戳 */
    uint8_t mac[8];                     /* 消息认证码 */
} secoc_sync_response_t;

/* ============================================================================
 * Freshness值管理器
 * ============================================================================ */

typedef struct {
    uint32_t pdu_id;
    secoc_freshness_type_t type;
    secoc_sync_mode_t sync_mode;
    secoc_fv_state_t state;
    
    /* 计数器模式 */
    uint64_t tx_counter;                /* 发送计数器 */
    uint64_t rx_counter;                /* 接收计数器 */
    uint64_t max_counter;               /* 计数器最大值 */
    uint32_t sync_gap;                  /* 同步阈值 */
    
    /* 时间戳模式 */
    uint64_t base_timestamp;            /* 基准时间戳 */
    uint64_t last_tx_timestamp;         /* 上次发送时间戳 */
    uint64_t last_rx_timestamp;         /* 上次接收时间戳 */
    uint32_t timestamp_resolution;      /* 时间戳分辨率 */
    
    /* 行程计数器模式 */
    uint32_t trip_counter;              /* 行程计数器 */
    uint32_t reset_counter;             /* 复位计数器 */
    uint32_t max_trip;                  /* 最大行程值 */
    uint32_t max_reset;                 /* 最大复位值 */
    
    /* 重放保护 */
    uint64_t last_accepted_value;       /* 上次接受的值 */
    uint64_t window_start;              /* 接收窗口起始 */
    uint32_t window_size;               /* 接收窗口大小 */
    
    /* 同步状态 */
    uint64_t last_sync_time;            /* 上次同步时间 */
    uint32_t sync_retry_count;          /* 同步重试计数 */
    bool synchronized;                  /* 是否已同步 */
    
    /* 统计 */
    uint64_t tx_count;
    uint64_t rx_count;
    uint64_t sync_count;
    uint64_t replay_count;
} secoc_fv_entry_t;

/* ============================================================================
 * Freshness管理器上下文
 * ============================================================================ */

typedef struct {
    secoc_fv_entry_t entries[SECOC_FV_MAX_ENTRIES];
    uint32_t num_entries;
    
    /* 全局时钟偏移估计 (用于时间戳模式) */
    int64_t clock_drift_us;
    
    /* Master同步回调 */
    void (*on_sync_request)(uint32_t pdu_id, const secoc_sync_request_t *req);
    void (*on_sync_response)(uint32_t pdu_id, const secoc_sync_response_t *res);
    
    /* 计时器函数 */
    uint64_t (*get_timestamp_us)(void);
    
    bool initialized;
} secoc_freshness_manager_t;

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

/**
 * @brief 初始化Freshness管理器
 * @return 初始化后的管理器指针
 */
secoc_freshness_manager_t* secoc_freshness_init(void);

/**
 * @brief 反初始化Freshness管理器
 * @param mgr Freshness管理器
 */
void secoc_freshness_deinit(secoc_freshness_manager_t *mgr);

/**
 * @brief 设置时间戳获取函数
 * @param mgr Freshness管理器
 * @param get_timestamp_us 获取微秒级时间戳的函数指针
 */
void secoc_freshness_set_timer(secoc_freshness_manager_t *mgr,
                               uint64_t (*get_timestamp_us)(void));

/* ============================================================================
 * Freshness值配置管理
 * ============================================================================ */

/**
 * @brief 注册Freshness值
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param type Freshness类型
 * @param sync_mode 同步模式
 * @param max_value 最大值 (计数器模式)
 * @param sync_gap 同步阈值
 * @return 注册的条目指针
 */
secoc_fv_entry_t* secoc_freshness_register(secoc_freshness_manager_t *mgr,
                                           uint32_t pdu_id,
                                           secoc_freshness_type_t type,
                                           secoc_sync_mode_t sync_mode,
                                           uint64_t max_value,
                                           uint32_t sync_gap);

/**
 * @brief 注销Freshness值
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_unregister(secoc_freshness_manager_t *mgr, uint32_t pdu_id);

/**
 * @brief 查找Freshness条目
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @return 条目指针，NULL表示未找到
 */
secoc_fv_entry_t* secoc_freshness_find(secoc_freshness_manager_t *mgr, uint32_t pdu_id);

/* ============================================================================
 * 计数器模式操作
 * ============================================================================ */

/**
 * @brief 获取发送用Freshness值 (计数器模式)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 输出Freshness值
 * @param freshness_len 输出长度 (字节)
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_get_tx_counter(secoc_freshness_manager_t *mgr,
                                              uint32_t pdu_id,
                                              uint64_t *freshness,
                                              uint8_t *freshness_len);

/**
 * @brief 验证接收的Freshness值 (计数器模式)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 接收到的Freshness值
 * @param freshness_len 长度 (字节)
 * @return SECOC_OK验证通过
 */
secoc_status_t secoc_freshness_verify_counter(secoc_freshness_manager_t *mgr,
                                              uint32_t pdu_id,
                                              uint64_t freshness,
                                              uint8_t freshness_len);

/**
 * @brief 增加发送计数器
 * @param entry Freshness条目
 */
void secoc_freshness_increment_counter(secoc_fv_entry_t *entry);

/* ============================================================================
 * 时间戳模式操作
 * ============================================================================ */

/**
 * @brief 获取发送用Freshness值 (时间戳模式)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 输出Freshness值
 * @param freshness_len 输出长度 (字节)
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_get_tx_timestamp(secoc_freshness_manager_t *mgr,
                                                uint32_t pdu_id,
                                                uint64_t *freshness,
                                                uint8_t *freshness_len);

/**
 * @brief 验证接收的Freshness值 (时间戳模式)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 接收到的时间戳
 * @param freshness_len 长度 (字节)
 * @return SECOC_OK验证通过
 */
secoc_status_t secoc_freshness_verify_timestamp(secoc_freshness_manager_t *mgr,
                                                uint32_t pdu_id,
                                                uint64_t freshness,
                                                uint8_t freshness_len);

/**
 * @brief 校准时钟偏移
 * @param mgr Freshness管理器
 * @param local_timestamp 本地时间戳
 * @param remote_timestamp 远程时间戳
 */
void secoc_freshness_calibrate_clock(secoc_freshness_manager_t *mgr,
                                     uint64_t local_timestamp,
                                     uint64_t remote_timestamp);

/* ============================================================================
 * 行程计数器模式操作
 * ============================================================================ */

/**
 * @brief 获取发送用Freshness值 (行程计数器模式)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 输出Freshness值
 * @param freshness_len 输出长度 (字节)
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_get_tx_trip(secoc_freshness_manager_t *mgr,
                                           uint32_t pdu_id,
                                           uint64_t *freshness,
                                           uint8_t *freshness_len);

/**
 * @brief 验证接收的Freshness值 (行程计数器模式)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 接收到的Freshness值
 * @param freshness_len 长度 (字节)
 * @return SECOC_OK验证通过
 */
secoc_status_t secoc_freshness_verify_trip(secoc_freshness_manager_t *mgr,
                                           uint32_t pdu_id,
                                           uint64_t freshness,
                                           uint8_t freshness_len);

/**
 * @brief 增加行程计数器
 * @param entry Freshness条目
 */
void secoc_freshness_increment_trip(secoc_fv_entry_t *entry);

/**
 * @brief 增加复位计数器
 * @param entry Freshness条目
 */
void secoc_freshness_increment_reset(secoc_fv_entry_t *entry);

/* ============================================================================
 * Master-Slave同步机制
 * ============================================================================ */

/**
 * @brief 创建同步请求消息 (Slave调用)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param request 输出请求消息
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_create_sync_request(secoc_freshness_manager_t *mgr,
                                                   uint32_t pdu_id,
                                                   secoc_sync_request_t *request);

/**
 * @brief 处理同步请求 (Master调用)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param request 请求消息
 * @param response 输出响应消息
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_handle_sync_request(secoc_freshness_manager_t *mgr,
                                                   uint32_t pdu_id,
                                                   const secoc_sync_request_t *request,
                                                   secoc_sync_response_t *response);

/**
 * @brief 创建同步广播消息 (Master定期调用)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param broadcast 输出广播消息
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_create_sync_broadcast(secoc_freshness_manager_t *mgr,
                                                     uint32_t pdu_id,
                                                     secoc_sync_response_t *broadcast);

/**
 * @brief 处理同步响应/广播 (Slave调用)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param response 响应/广播消息
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_handle_sync_response(secoc_freshness_manager_t *mgr,
                                                    uint32_t pdu_id,
                                                    const secoc_sync_response_t *response);

/**
 * @brief 检查是否需要同步
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param current_time 当前时间
 * @return true需要同步
 */
bool secoc_freshness_need_sync(secoc_freshness_manager_t *mgr,
                               uint32_t pdu_id,
                               uint64_t current_time);

/**
 * @brief 强制同步 (Slave在检测到差异时调用)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @return SECOC_OK同步成功或请求已发送
 */
secoc_status_t secoc_freshness_force_sync(secoc_freshness_manager_t *mgr, uint32_t pdu_id);

/* ============================================================================
 * 通用接口 (根据类型自动分发)
 * ============================================================================ */

/**
 * @brief 获取发送用Freshness值 (通用接口)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 输出Freshness值
 * @param freshness_len 输出长度 (字节)
 * @return SECOC_OK成功
 */
secoc_status_t secoc_freshness_get_tx_value(secoc_freshness_manager_t *mgr,
                                            uint32_t pdu_id,
                                            uint64_t *freshness,
                                            uint8_t *freshness_len);

/**
 * @brief 验证接收的Freshness值 (通用接口)
 * @param mgr Freshness管理器
 * @param pdu_id PDU标识符
 * @param freshness 接收到的Freshness值
 * @param freshness_len 长度 (字节)
 * @param result 输出验证结果
 * @return SECOC_OK处理成功
 */
secoc_status_t secoc_freshness_verify(secoc_freshness_manager_t *mgr,
                                      uint32_t pdu_id,
                                      uint64_t freshness,
                                      uint8_t freshness_len,
                                      secoc_verify_result_t *result);

/* ============================================================================
 * 重放保护
 * ============================================================================ */

/**
 * @brief 检测重放攻击
 * @param entry Freshness条目
 * @param received_value 接收到的值
 * @return true检测到重放
 */
bool secoc_freshness_check_replay(secoc_fv_entry_t *entry, uint64_t received_value);

/**
 * @brief 更新接收窗口
 * @param entry Freshness条目
 * @param accepted_value 已接受的值
 */
void secoc_freshness_update_window(secoc_fv_entry_t *entry, uint64_t accepted_value);

/* ============================================================================
 * 统计和调试
 * ============================================================================ */

/**
 * @brief 获取Freshness条目统计信息
 * @param entry Freshness条目
 * @param tx_count 输出发送次数
 * @param rx_count 输出接收次数
 * @param sync_count 输出同步次数
 * @param replay_count 输出重放检测次数
 */
void secoc_freshness_get_stats(secoc_fv_entry_t *entry,
                               uint64_t *tx_count,
                               uint64_t *rx_count,
                               uint64_t *sync_count,
                               uint64_t *replay_count);

/**
 * @brief 获取Freshness值状态字符串
 * @param state 状态
 * @return 状态字符串
 */
const char* secoc_freshness_state_str(secoc_fv_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* SECOC_FRESHNESS_H */

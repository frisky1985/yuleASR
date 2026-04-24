/**
 * @file rtps_state.h
 * @brief RTPS协议状态机 - 端点状态管理与心跳机制
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现RTPS端点的状态管理：
 * - Reader/Writer状态机
 * - 心跳机制（Heartbeat/ACKNACK）
 * - 移除通知
 * - 网络挂起/恢复处理
 * 支持车载场景的确定性状态转换
 */

#ifndef RTPS_STATE_H
#define RTPS_STATE_H

#include "rtps_discovery.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 状态机常量定义
 * ============================================================================ */

/** 心跳超时配置 */
#define RTPS_HEARTBEAT_PERIOD_MS            100     /* 心跳发送周期 */
#define RTPS_HEARTBEAT_TIMEOUT_MS           300     /* 心跳超时 */
#define RTPS_NACK_RESPONSE_DELAY_MS         10      /* NACK响应延迟 */
#define RTPS_NACK_SUPPRESSION_DURATION_MS   20      /* NACK抑制时间 */
#define RTPS_HEARTBEAT_RESPONSE_DELAY_MS    5       /* 心跳响应延迟 */
#define RTPS_HEARTBEAT_SUPPRESSION_MS       10      /* 心跳抑制时间 */

/** 接收器缓冲区配置 */
#define RTPS_READER_MAX_CACHED_CHANGES      32      /* 最大缓存变化 */
#define RTPS_WRITER_MAX_CACHED_CHANGES      64      /* Writer最大缓存 */
#define RTPS_MAX_MATCHED_READERS            8       /* 最大匹配Reader数量 */
#define RTPS_MAX_MATCHED_WRITERS            8       /* 最大匹配Writer数量 */

/* ============================================================================
 * 状态机状态定义
 * ============================================================================ */

/** Writer状态 */
typedef enum {
    RTPS_WRITER_STATE_IDLE = 0,         /* 空闲 */
    RTPS_WRITER_STATE_ANNOUNCING,       /* 正在公告 */
    RTPS_WRITER_STATE_PUSHING,          /* 推送数据 */
    RTPS_WRITER_STATE_WAITING_ACK,      /* 等待ACK */
    RTPS_WRITER_STATE_MUST_REFRESH,     /* 需要刷新 */
    RTPS_WRITER_STATE_ERROR,            /* 错误状态 */
} rtps_writer_state_t;

/** Reader状态 */
typedef enum {
    RTPS_READER_STATE_IDLE = 0,         /* 空闲 */
    RTPS_READER_STATE_WAITING_DATA,     /* 等待数据 */
    RTPS_READER_STATE_REQUESTING,       /* 请求数据 */
    RTPS_READER_STATE_PROCESSING,       /* 处理数据 */
    RTPS_READER_STATE_MUST_ACK,         /* 需要发送ACK */
    RTPS_READER_STATE_ERROR,            /* 错误状态 */
} rtps_reader_state_t;

/** 网络状态 */
typedef enum {
    RTPS_NETWORK_UP = 0,                /* 网络正常 */
    RTPS_NETWORK_SUSPENDED,             /* 网络挂起 */
    RTPS_NETWORK_RECOVERING,            /* 正在恢复 */
    RTPS_NETWORK_DOWN,                  /* 网络断开 */
} rtps_network_state_t;

/** 连接状态 */
typedef enum {
    RTPS_CONNECTION_MATCHED = 0,        /* 已匹配 */
    RTPS_CONNECTION_ACTIVE,             /* 活跃 */
    RTPS_CONNECTION_STALE,              /* 不活跃 */
    RTPS_CONNECTION_LOST,               /* 连接丢失 */
    RTPS_CONNECTION_REMOVED,            /* 已移除 */
} rtps_connection_state_t;

/* ============================================================================
 * 缓存变化数据结构
 * ============================================================================ */

/** 缓存的RTPS数据改变 */
typedef struct rtps_cached_change {
    rtps_sequence_number_t seq_number;  /* 序列号 */
    uint32_t data_len;                  /* 数据长度 */
    uint8_t *data;                      /* 数据指针 */
    bool is_inline_qos;                 /* 是否内联QoS */
    uint64_t timestamp;                 /* 时间戳 */
    uint32_t ref_count;                 /* 引用计数 */
    struct rtps_cached_change *next;    /* 链表指针 */
} rtps_cached_change_t;

/** 丢失报告范围 */
typedef struct rtps_sequence_number_set {
    rtps_sequence_number_t bitmap_base; /* 位图基准 */
    uint32_t num_bits;                  /* 位数 */
    uint32_t bitmap[8];                 /* 位图数组 */
} rtps_sequence_number_set_t;

/* ============================================================================
 * 端点状态机上下文
 * ============================================================================ */

/** 匹配的Reader信息 */
typedef struct rtps_matched_reader {
    rtps_guid_t remote_guid;            /* 远程Reader GUID */
    rtps_connection_state_t state;      /* 连接状态 */
    rtps_sequence_number_t last_sn;     /* 上次收到的序列号 */
    rtps_sequence_number_t max_sn;      /* 最大序列号 */
    uint64_t last_heartbeat_time;       /* 上次心跳时间 */
    uint64_t last_activity_time;        /* 上次活动时间 */
    bool expects_inline_qos;            /* 是否期望内联QoS */
    uint32_t ack_nack_count;            /* ACKNACK计数 */
    bool active;                        /* 是否活跃 */
} rtps_matched_reader_t;

/** 匹配的Writer信息 */
typedef struct rtps_matched_writer {
    rtps_guid_t remote_guid;            /* 远程Writer GUID */
    rtps_connection_state_t state;      /* 连接状态 */
    rtps_sequence_number_t next_expected_seq; /* 期望的下一序列号 */
    rtps_sequence_number_t last_available_seq; /* 最后可用序列号 */
    rtps_sequence_number_set_t missing_changes; /* 丢失变化集 */
    uint64_t last_heartbeat_time;       /* 上次心跳时间 */
    uint64_t last_activity_time;        /* 上次活动时间 */
    uint32_t heartbeat_count;           /* 心跳计数 */
    bool active;                        /* 是否活跃 */
} rtps_matched_writer_t;

/** Writer状态机 */
typedef struct rtps_writer_state_machine {
    rtps_guid_t guid;                   /* Writer GUID */
    rtps_writer_state_t state;          /* 当前状态 */
    
    /* 序列号管理 */
    rtps_sequence_number_t last_seq;    /* 上次发送序列号 */
    rtps_sequence_number_t max_seq;     /* 最大序列号 */
    
    /* 缓存变化 */
    rtps_cached_change_t *history_cache; /* 历史缓存 */
    uint32_t history_cache_size;        /* 缓存大小 */
    uint32_t history_cache_max;         /* 最大缓存 */
    
    /* 匹配的Readers */
    rtps_matched_reader_t matched_readers[RTPS_MAX_MATCHED_READERS];
    uint32_t matched_reader_count;
    
    /* 心跳状态 */
    uint64_t last_heartbeat_send_time;  /* 上次发送心跳时间 */
    uint32_t heartbeat_count;           /* 心跳计数 */
    bool must_send_ack;                 /* 是否必须发送ACK */
    
    /* AUTOSAR扩展 */
    bool is_autosar_deterministic;      /* 确定性模式 */
    uint32_t autosar_slot_time_us;      /* 时间槽 */
    
    /* 统计 */
    uint32_t data_sent_count;
    uint32_t heartbeat_sent_count;
    uint32_t gap_sent_count;
} rtps_writer_state_machine_t;

/** Reader状态机 */
typedef struct rtps_reader_state_machine {
    rtps_guid_t guid;                   /* Reader GUID */
    rtps_reader_state_t state;          /* 当前状态 */
    
    /* 序列号管理 */
    rtps_sequence_number_t next_expected_seq; /* 期望的下一序列号 */
    rtps_sequence_number_t highest_seq; /* 收到的最高序列号 */
    
    /* 缓存变化 */
    rtps_cached_change_t *receive_queue; /* 接收队列 */
    uint32_t receive_queue_size;        /* 队列大小 */
    uint32_t receive_queue_max;         /* 最大队列大小 */
    
    /* 匹配的Writers */
    rtps_matched_writer_t matched_writers[RTPS_MAX_MATCHED_WRITERS];
    uint32_t matched_writer_count;
    
    /* ACKNACK状态 */
    uint64_t last_acknack_send_time;    /* 上次发送ACKNACK时间 */
    uint32_t acknack_count;             /* ACKNACK计数 */
    rtps_sequence_number_set_t ack_bitmap; /* ACK位图 */
    bool must_send_acknack;             /* 是否必须发送ACKNACK */
    bool acknack_suppressed;            /* ACKNACK是否被抑制 */
    
    /* AUTOSAR扩展 */
    bool is_autosar_deterministic;
    uint32_t autosar_response_delay_us; /* 响应延迟 */
    
    /* 统计 */
    uint32_t data_received_count;
    uint32_t acknack_sent_count;
    uint32_t gap_received_count;
} rtps_reader_state_machine_t;

/* ============================================================================
 * 网络状态管理
 * ============================================================================ */

/** 网络状态上下文 */
typedef struct rtps_network_context {
    rtps_network_state_t state;         /* 当前网络状态 */
    uint64_t suspend_start_time;        /* 挂起开始时间 */
    uint64_t recovery_start_time;       /* 恢复开始时间 */
    
    /* 挂起/恢复回调 */
    void (*on_network_suspend)(void);
    void (*on_network_resume)(void);
    
    /* 统计 */
    uint32_t suspend_count;
    uint64_t total_suspend_time_ms;
} rtps_network_context_t;

/* ============================================================================
 * Writer状态机API
 * ============================================================================ */

/**
 * @brief 初始化Writer状态机
 * @param writer Writer状态机
 * @param guid Writer GUID
 * @param history_depth 历史深度
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_init(rtps_writer_state_machine_t *writer,
                                  const rtps_guid_t *guid,
                                  uint32_t history_depth);

/**
 * @brief 反初始化Writer状态机
 * @param writer Writer状态机
 */
void rtps_writer_sm_deinit(rtps_writer_state_machine_t *writer);

/**
 * @brief 匹配Reader
 * @param writer Writer状态机
 * @param reader_guid Reader GUID
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_match_reader(rtps_writer_state_machine_t *writer,
                                          const rtps_guid_t *reader_guid);

/**
 * @brief 取消匹配Reader
 * @param writer Writer状态机
 * @param reader_guid Reader GUID
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_unmatch_reader(rtps_writer_state_machine_t *writer,
                                            const rtps_guid_t *reader_guid);

/**
 * @brief 写入数据到Writer
 * @param writer Writer状态机
 * @param data 数据
 * @param len 长度
 * @param seq_number 输出分配的序列号
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_write(rtps_writer_state_machine_t *writer,
                                   const uint8_t *data,
                                   uint32_t len,
                                   rtps_sequence_number_t *seq_number);

/**
 * @brief 处理收到的ACKNACK
 * @param writer Writer状态机
 * @param reader_guid Reader GUID
 * @param ack_bitmap ACK位图
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_handle_acknack(rtps_writer_state_machine_t *writer,
                                            const rtps_guid_t *reader_guid,
                                            const rtps_sequence_number_set_t *ack_bitmap,
                                            uint64_t current_time_ms);

/**
 * @brief 处理周期性任务
 * @param writer Writer状态机
 * @param current_time_ms 当前时间
 * @param need_heartbeat 输出是否需要发送心跳
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_process(rtps_writer_state_machine_t *writer,
                                     uint64_t current_time_ms,
                                     bool *need_heartbeat);

/**
 * @brief 获取需要重发送的变化
 * @param writer Writer状态机
 * @param reader_guid 目标Reader
 * @param changes 输出变化数组
 * @param max_changes 最大数量
 * @param actual_changes 实际数量
 * @return ETH_OK成功
 */
eth_status_t rtps_writer_sm_get_requested_changes(rtps_writer_state_machine_t *writer,
                                                    const rtps_guid_t *reader_guid,
                                                    rtps_cached_change_t **changes,
                                                    uint32_t max_changes,
                                                    uint32_t *actual_changes);

/* ============================================================================
 * Reader状态机API
 * ============================================================================ */

/**
 * @brief 初始化Reader状态机
 * @param reader Reader状态机
 * @param guid Reader GUID
 * @param queue_size 队列大小
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_init(rtps_reader_state_machine_t *reader,
                                  const rtps_guid_t *guid,
                                  uint32_t queue_size);

/**
 * @brief 反初始化Reader状态机
 * @param reader Reader状态机
 */
void rtps_reader_sm_deinit(rtps_reader_state_machine_t *reader);

/**
 * @brief 匹配Writer
 * @param reader Reader状态机
 * @param writer_guid Writer GUID
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_match_writer(rtps_reader_state_machine_t *reader,
                                          const rtps_guid_t *writer_guid);

/**
 * @brief 取消匹配Writer
 * @param reader Reader状态机
 * @param writer_guid Writer GUID
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_unmatch_writer(rtps_reader_state_machine_t *reader,
                                            const rtps_guid_t *writer_guid);

/**
 * @brief 处理收到的DATA报文
 * @param reader Reader状态机
 * @param writer_guid Writer GUID
 * @param seq_number 序列号
 * @param data 数据
 * @param len 长度
 * @param current_time_ms 当前时间
 * @param need_acknack 输出是否需要发送ACKNACK
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_handle_data(rtps_reader_state_machine_t *reader,
                                         const rtps_guid_t *writer_guid,
                                         const rtps_sequence_number_t *seq_number,
                                         const uint8_t *data,
                                         uint32_t len,
                                         uint64_t current_time_ms,
                                         bool *need_acknack);

/**
 * @brief 处理收到的HEARTBEAT
 * @param reader Reader状态机
 * @param writer_guid Writer GUID
 * @param first_sn 第一个序列号
 * @param last_sn 最后序列号
 * @param current_time_ms 当前时间
 * @param need_acknack 输出是否需要发送ACKNACK
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_handle_heartbeat(rtps_reader_state_machine_t *reader,
                                              const rtps_guid_t *writer_guid,
                                              const rtps_sequence_number_t *first_sn,
                                              const rtps_sequence_number_t *last_sn,
                                              uint64_t current_time_ms,
                                              bool *need_acknack);

/**
 * @brief 处理周期性任务
 * @param reader Reader状态机
 * @param current_time_ms 当前时间
 * @param need_acknack 输出是否需要发送ACKNACK
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_process(rtps_reader_state_machine_t *reader,
                                     uint64_t current_time_ms,
                                     bool *need_acknack);

/**
 * @brief 读取接收队列中的数据
 * @param reader Reader状态机
 * @param data 输出缓冲区
 * @param max_len 最大长度
 * @param actual_len 实际长度
 * @param seq_number 输出序列号
 * @return ETH_OK成功，ETH_TIMEOUT无数据
 */
eth_status_t rtps_reader_sm_read(rtps_reader_state_machine_t *reader,
                                  uint8_t *data,
                                  uint32_t max_len,
                                  uint32_t *actual_len,
                                  rtps_sequence_number_t *seq_number);

/**
 * @brief 构建ACKNACK报文
 * @param reader Reader状态机
 * @param writer_guid 目标Writer
 * @param buffer 输出缓冲区
 * @param max_len 最大长度
 * @param actual_len 实际长度
 * @return ETH_OK成功
 */
eth_status_t rtps_reader_sm_build_acknack(rtps_reader_state_machine_t *reader,
                                           const rtps_guid_t *writer_guid,
                                           uint8_t *buffer,
                                           uint32_t max_len,
                                           uint32_t *actual_len);

/* ============================================================================
 * 网络状态管理API
 * ============================================================================ */

/**
 * @brief 初始化网络状态管理
 * @param net_ctx 网络上下文
 * @return ETH_OK成功
 */
eth_status_t rtps_network_init(rtps_network_context_t *net_ctx);

/**
 * @brief 反初始化网络状态管理
 * @param net_ctx 网络上下文
 */
void rtps_network_deinit(rtps_network_context_t *net_ctx);

/**
 * @brief 通知网络挂起
 * @param net_ctx 网络上下文
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t rtps_network_suspend(rtps_network_context_t *net_ctx,
                                   uint64_t current_time_ms);

/**
 * @brief 通知网络恢复
 * @param net_ctx 网络上下文
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t rtps_network_resume(rtps_network_context_t *net_ctx,
                                  uint64_t current_time_ms);

/**
 * @brief 获取当前网络状态
 * @param net_ctx 网络上下文
 * @return 网络状态
 */
rtps_network_state_t rtps_network_get_state(rtps_network_context_t *net_ctx);

/**
 * @brief 检查网络是否可用
 * @param net_ctx 网络上下文
 * @return true可用
 */
bool rtps_network_is_available(rtps_network_context_t *net_ctx);

/**
 * @brief 设置网络状态回调
 * @param net_ctx 网络上下文
 * @param on_suspend 挂起回调
 * @param on_resume 恢复回调
 */
void rtps_network_set_callbacks(rtps_network_context_t *net_ctx,
                                 void (*on_suspend)(void),
                                 void (*on_resume)(void));

#ifdef __cplusplus
}
#endif

#endif /* RTPS_STATE_H */

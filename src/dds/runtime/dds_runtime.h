/**
 * @file dds_runtime.h
 * @brief DDS运行时 - 参与者管理、发布/订阅管理、主循环调度
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现DDS运行时的核心管理：
 * - 参与者生命周期管理
 * - 发布/订阅管理
 * - 主循环调度
 * - 资源管理
 * 支持车载场景的确定性调度和快速启动
 */

#ifndef DDS_RUNTIME_H
#define DDS_RUNTIME_H

#include "../rtps/rtps_discovery.h"
#include "../rtps/rtps_state.h"
#include "../rtps/rtps_message.h"
#include "../core/dds_core.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DDS运行时常量定义
 * ============================================================================ */

/** 版本信息 */
#define DDS_RUNTIME_VERSION_MAJOR       1
#define DDS_RUNTIME_VERSION_MINOR       0
#define DDS_RUNTIME_VERSION_PATCH       0

/** 默认配置 */
#define DDS_DEFAULT_DOMAIN_ID           0
#define DDS_DEFAULT_MAX_PARTICIPANTS    8
#define DDS_DEFAULT_MAX_PUBLISHERS      16
#define DDS_DEFAULT_MAX_SUBSCRIBERS     16
#define DDS_DEFAULT_MAX_TOPICS          32
#define DDS_DEFAULT_MAX_ENDPOINTS       64

/** 运行时时序配置 */
#define DDS_SPIN_PERIOD_MS              10      /* 主循环周期 */
#define DDS_DISCOVERY_SPIN_PERIOD_MS    20      /* 发现处理周期 */
#define DDS_HEARTBEAT_PERIOD_MS         100     /* 心跳处理周期 */
#define DDS_CLEANUP_PERIOD_MS           1000    /* 资源清理周期 */

/** 车载优化配置 */
#define DDS_AUTOSAR_SPIN_PERIOD_US      1000    /* AUTOSAR模式周期（1ms） */
#define DDS_FAST_STARTUP_TIMEOUT_MS     100     /* 快速启动超时 */

/* ============================================================================
 * DDS核心实体结构
 * ============================================================================ */

/** 参与者内部结构 */
struct dds_domain_participant {
    rtps_guid_t guid;                       /* 参与者GUID */
    dds_domain_id_t domain_id;              /* 域ID */
    dds_qos_t qos;                          /* QoS配置 */
    
    /* 资源管理 */
    struct dds_publisher *publishers;       /* 发布者列表 */
    struct dds_subscriber *subscribers;     /* 订阅者列表 */
    struct dds_topic *topics;               /* 主题列表 */
    uint32_t publisher_count;
    uint32_t subscriber_count;
    uint32_t topic_count;
    
    /* 状态 */
    bool active;
    uint64_t create_time;
    
    /* 链表指针 */
    struct dds_domain_participant *next;
};

/** 发布者内部结构 */
struct dds_publisher {
    rtps_guid_t guid;                       /* 发布者GUID */
    dds_domain_participant_t *participant;  /* 所属参与者 */
    dds_qos_t qos;                          /* QoS配置 */
    
    /* 资源管理 */
    struct dds_data_writer *writers;        /* 写入者列表 */
    uint32_t writer_count;
    
    /* 状态 */
    bool active;
    
    /* 链表指针 */
    struct dds_publisher *next;
};

/** 订阅者内部结构 */
struct dds_subscriber {
    rtps_guid_t guid;                       /* 订阅者GUID */
    dds_domain_participant_t *participant;  /* 所属参与者 */
    dds_qos_t qos;                          /* QoS配置 */
    
    /* 资源管理 */
    struct dds_data_reader *readers;        /* 读取者列表 */
    uint32_t reader_count;
    
    /* 状态 */
    bool active;
    
    /* 链表指针 */
    struct dds_subscriber *next;
};

/** 主题内部结构 */
struct dds_topic {
    rtps_guid_t guid;                       /* 主题GUID */
    dds_domain_participant_t *participant;  /* 所属参与者 */
    dds_qos_t qos;                          /* QoS配置 */
    
    char name[64];                          /* 主题名称 */
    char type_name[64];                     /* 类型名称 */
    
    /* 端点引用 */
    uint32_t writer_ref_count;
    uint32_t reader_ref_count;
    
    /* 状态 */
    bool active;
    
    /* 链表指针 */
    struct dds_topic *next;
};

/** 数据写入者内部结构 */
struct dds_data_writer {
    rtps_guid_t guid;                       /* Writer GUID */
    dds_publisher_t *publisher;             /* 所属发布者 */
    dds_topic_t *topic;                     /* 关联主题 */
    dds_qos_t qos;                          /* QoS配置 */
    
    /* RTPS状态机 */
    rtps_writer_state_machine_t state_machine;
    
    /* 回调 */
    void (*write_callback)(void *user_data);
    void *write_callback_user_data;
    
    /* 统计 */
    uint32_t samples_written;
    uint64_t last_write_time;
    
    /* 状态 */
    bool active;
    
    /* 链表指针 */
    struct dds_data_writer *next;
};

/** 数据读取者内部结构 */
struct dds_data_reader {
    rtps_guid_t guid;                       /* Reader GUID */
    dds_subscriber_t *subscriber;           /* 所属订阅者 */
    dds_topic_t *topic;                     /* 关联主题 */
    dds_qos_t qos;                          /* QoS配置 */
    
    /* RTPS状态机 */
    rtps_reader_state_machine_t state_machine;
    
    /* 回调 */
    dds_data_callback_t data_callback;
    void *data_callback_user_data;
    
    /* 接收缓冲 */
    uint8_t *receive_buffer;
    uint32_t receive_buffer_size;
    
    /* 统计 */
    uint32_t samples_received;
    uint64_t last_read_time;
    
    /* 状态 */
    bool active;
    
    /* 链表指针 */
    struct dds_data_reader *next;
};

/* ============================================================================
 * DDS运行时配置
 * ============================================================================ */

/** 运行时配置 */
typedef struct dds_runtime_config {
    /* 资源限制 */
    uint32_t max_participants;
    uint32_t max_publishers;
    uint32_t max_subscribers;
    uint32_t max_topics;
    uint32_t max_endpoints;
    
    /* 时序配置 */
    uint32_t spin_period_ms;
    uint32_t discovery_period_ms;
    uint32_t heartbeat_period_ms;
    uint32_t cleanup_period_ms;
    
    /* 网络配置 */
    bool use_multicast;
    bool use_shm;                           /* 共享内存传输 */
    
    /* AUTOSAR扩展 */
    bool autosar_mode;
    uint32_t autosar_slot_time_us;
    bool enable_deterministic;
    uint32_t deterministic_window_ms;
    
    /* 启动配置 */
    bool enable_fast_startup;
    uint32_t startup_timeout_ms;
} dds_runtime_config_t;

/** 运行时统计信息 */
typedef struct dds_runtime_stats {
    uint32_t participant_count;
    uint32_t publisher_count;
    uint32_t subscriber_count;
    uint32_t topic_count;
    uint32_t writer_count;
    uint32_t reader_count;
    
    uint64_t total_samples_written;
    uint64_t total_samples_received;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    
    uint32_t discovery_count;
    uint32_t match_count;
    uint32_t error_count;
    
    uint64_t uptime_ms;
} dds_runtime_stats_t;

/* ============================================================================
 * DDS运行时API
 * ============================================================================ */

/**
 * @brief 初始化DDS运行时
 * @param config 运行时配置（可为NULL使用默认配置）
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_init(const dds_runtime_config_t *config);

/**
 * @brief 反初始化DDS运行时
 */
void dds_runtime_deinit(void);

/**
 * @brief 执行DDS事务处理（单次调用）
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t dds_spin_once(uint32_t timeout_ms);

/**
 * @brief 启动DDS主循环（阻塞模式）
 * @return ETH_OK成功
 */
eth_status_t dds_run(void);

/**
 * @brief 停止DDS主循环
 */
void dds_stop(void);

/**
 * @brief 检查运行时是否正在运行
 * @return true正在运行
 */
bool dds_runtime_is_running(void);

/**
 * @brief 等待发现完成
 * @param timeout_ms 超时时间
 * @return ETH_OK成功，ETH_TIMEOUT超时
 */
eth_status_t dds_wait_for_discovery(uint32_t timeout_ms);

/* ============================================================================
 * 参与者管理API
 * ============================================================================ */

/**
 * @brief 创建参与者（内部实现）
 * @param domain_id 域ID
 * @param config 配置（可为NULL）
 * @return 参与者句柄，NULL表示失败
 */
dds_domain_participant_t* dds_runtime_create_participant(
    dds_domain_id_t domain_id,
    const dds_runtime_config_t *config);

/**
 * @brief 删除参与者（内部实现）
 * @param participant 参与者句柄
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_delete_participant(dds_domain_participant_t *participant);

/**
 * @brief 查找参与者
 * @param domain_id 域ID
 * @param guid_prefix GUID前缀（可为NULL）
 * @return 参与者句柄，NULL表示未找到
 */
dds_domain_participant_t* dds_runtime_find_participant(
    dds_domain_id_t domain_id,
    const rtps_guid_prefix_t guid_prefix);

/**
 * @brief 获取当前参与者数量
 * @return 参与者数量
 */
uint32_t dds_runtime_get_participant_count(void);

/* ============================================================================
 * 发布/订阅管理API
 * ============================================================================ */

/**
 * @brief 匹配Writer和Reader
 * @param writer 写入者
 * @param reader 读取者
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_match_endpoints(dds_data_writer_t *writer,
                                          dds_data_reader_t *reader);

/**
 * @brief 取消匹配Writer和Reader
 * @param writer 写入者
 * @param reader 读取者
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_unmatch_endpoints(dds_data_writer_t *writer,
                                            dds_data_reader_t *reader);

/**
 * @brief 处理发现消息
 * @param data 发现数据
 * @param len 长度
 * @param source_addr 源地址
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_handle_discovery(const uint8_t *data,
                                           uint32_t len,
                                           const void *source_addr);

/**
 * @brief 处理RTPS数据报文
 * @param data 报文数据
 * @param len 长度
 * @param source_addr 源地址
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_handle_rtps_data(const uint8_t *data,
                                           uint32_t len,
                                           const void *source_addr);

/* ============================================================================
 * 资源管理API
 * ============================================================================ */

/**
 * @brief 分配资源池
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_allocate_pools(void);

/**
 * @brief 释放资源池
 */
void dds_runtime_free_pools(void);

/**
 * @brief 获取运行时统计信息
 * @param stats 输出统计信息
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_get_stats(dds_runtime_stats_t *stats);

/**
 * @brief 重置运行时统计
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_reset_stats(void);

/**
 * @brief 执行资源清理
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_cleanup(void);

/* ============================================================================
 * 时间和时序API
 * ============================================================================ */

/**
 * @brief 获取当前时间戳（毫秒）
 * @return 时间戳
 */
uint64_t dds_get_current_time_ms(void);

/**
 * @brief 获取当前时间戳（微秒）
 * @return 时间戳
 */
uint64_t dds_get_current_time_us(void);

/**
 * @brief 延时（毫秒）
 * @param ms 延迟毫秒数
 */
void dds_sleep_ms(uint32_t ms);

/* ============================================================================
 * 错误处理API
 * ============================================================================ */

/**
 * @brief 获取最后错误码
 * @return 错误码
 */
int dds_runtime_get_last_error(void);

/**
 * @brief 获取错误信息字符串
 * @param error_code 错误码
 * @return 错误信息字符串
 */
const char* dds_runtime_get_error_string(int error_code);

/* ============================================================================
 * 内部调度API
 * ============================================================================ */

/**
 * @brief 调度发现处理
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_schedule_discovery(uint64_t current_time_ms);

/**
 * @brief 调度心跳处理
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_schedule_heartbeat(uint64_t current_time_ms);

/**
 * @brief 调度数据发送
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_schedule_send(uint64_t current_time_ms);

/**
 * @brief 调度数据接收处理
 * @param current_time_ms 当前时间
 * @return ETH_OK成功
 */
eth_status_t dds_runtime_schedule_receive(uint64_t current_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* DDS_RUNTIME_H */

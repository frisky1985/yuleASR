/**
 * @file gptp.h
 * @brief gPTP时间同步 (IEEE 802.1AS-2020)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note 时钟同步精度<100ns
 * @note 符合ASIL-D安全等级
 */

#ifndef TSN_GPTP_H
#define TSN_GPTP_H

#include "../../common/types/eth_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * IEEE 802.1AS 常量定义
 * ============================================================================ */

/** gPTP EtherType */
#define GPTP_ETHERTYPE                  0x88F7

/** gPTP协议版本 */
#define GPTP_VERSION_MAJOR              2
#define GPTP_VERSION_MINOR              1

/** 默认时间域 */
#define GPTP_DEFAULT_DOMAIN             0
#define GPTP_ALTERNATE_DOMAIN_1         1
#define GPTP_ALTERNATE_DOMAIN_2         2

/** 时钟精度要求 */
#define GPTP_SYNC_PRECISION_NS          100     /* 同步精度 < 100ns */
#define GPTP_MAX_PATH_DELAY_NS          1000    /* 最大路径延迟 */

/** Pdelay阈值 */
#define GPTP_PDELAY_THRESHOLD_NS        10000   /* Pdelay阈值 10us */
#define GPTP_NEIGHBOR_PROP_DELAY_NS     800     /* 邻居传播延迟 */

/** 消息间隔 */
#define GPTP_SYNC_INTERVAL_MS           125     /* Sync消息间隔 125ms */
#define GPTP_PDELAY_REQ_INTERVAL_MS     1000    /* Pdelay_Req间隔 1s */
#define GPTP_ANNOUNCE_INTERVAL_MS       1000    /* Announce间隔 1s */

/** ASIL-D安全相关 */
#define GPTP_SAFETY_MONITOR_INTERVAL_MS 10      /* 安全监控间隔 */
#define GPTP_MAX_CLOCK_DRIFT_PPB        100     /* 最大时钟漂移 ppb */
#define GPTP_FAULT_INJECTION_TEST       0       /* 故障注入测试 */

/* ============================================================================
 * gPTP消息类型 (IEEE 802.1AS Table 10-1)
 * ============================================================================ */

typedef enum {
    GPTP_MSG_TYPE_SYNC              = 0x00,     /* Sync消息 */
    GPTP_MSG_TYPE_DELAY_REQ         = 0x01,     /* Delay_Req消息 */
    GPTP_MSG_TYPE_PDELAY_REQ        = 0x02,     /* Pdelay_Req消息 */
    GPTP_MSG_TYPE_PDELAY_RESP       = 0x03,     /* Pdelay_Resp消息 */
    GPTP_MSG_TYPE_FOLLOW_UP         = 0x08,     /* Follow_Up消息 */
    GPTP_MSG_TYPE_DELAY_RESP        = 0x09,     /* Delay_Resp消息 */
    GPTP_MSG_TYPE_PDELAY_RESP_FUP   = 0x0A,     /* Pdelay_Resp_Follow_Up */
    GPTP_MSG_TYPE_ANNOUNCE          = 0x0B,     /* Announce消息 */
    GPTP_MSG_TYPE_SIGNALING         = 0x0C,     /* Signaling消息 */
    GPTP_MSG_TYPE_MANAGEMENT        = 0x0D,     /* Management消息 */
} gptp_msg_type_t;

/* ============================================================================
 * gPTP时间戳和时钟定义
 * ============================================================================ */

/** 64位纳秒时间戳 */
typedef struct {
    uint64_t seconds;               /* 秒 (Unix时间或PTP纪元) */
    uint32_t nanoseconds;           /* 纳秒 [0, 999999999] */
} gptp_timestamp_t;

/** 时间间隔 (有符号64位纳秒) */
typedef int64_t gptp_time_interval_t;

/** 时钟标识符 (8字节) */
typedef uint8_t gptp_clock_identity_t[8];

/** 端口标识符 */
typedef struct {
    gptp_clock_identity_t clock_identity;
    uint16_t port_number;
} gptp_port_identity_t;

/** 时钟质量结构 */
typedef struct {
    uint8_t clock_class;            /* 时钟等级 */
    uint8_t clock_accuracy;         /* 时钟精度 */
    uint16_t offset_scaled_log_variance;  /* 偏移对数方差 */
} gptp_clock_quality_t;

/* ============================================================================
 * BMC (Best Master Clock) 数据结构
 * ============================================================================ */

/** BMC优先级向量 */
typedef struct {
    gptp_clock_identity_t root_system_identity;
    uint16_t steps_removed;
    gptp_port_identity_t source_port_identity;
    uint16_t port_number;
} gptp_priority_vector_t;

/** 时钟优先级比较结果 */
typedef enum {
    GPTP_BMC_SUPERIOR = 0,          /* 更优时钟 */
    GPTP_BMC_INFERIOR,              /* 较差时钟 */
    GPTP_BMC_EQUAL,                 /* 相同时钟 */
} gptp_bmc_result_t;

/** BMC状态 */
typedef enum {
    GPTP_BMC_STATE_INIT = 0,
    GPTP_BMC_STATE_RECEIVING,
    GPTP_BMC_STATE_SUPERIOR_MASTER,
    GPTP_BMC_STATE_INFERIOR_MASTER,
    GPTP_BMC_STATE_PASSIVE,
} gptp_bmc_state_t;

/* ============================================================================
 * 端口和域状态定义
 * ============================================================================ */

/** gPTP端口状态 */
typedef enum {
    GPTP_PORT_STATE_INITIALIZING = 0,
    GPTP_PORT_STATE_FAULTY,
    GPTP_PORT_STATE_DISABLED,
    GPTP_PORT_STATE_LISTENING,
    GPTP_PORT_STATE_PRE_MASTER,
    GPTP_PORT_STATE_MASTER,
    GPTP_PORT_STATE_PASSIVE,
    GPTP_PORT_STATE_UNCALIBRATED,
    GPTP_PORT_STATE_SLAVE,
} gptp_port_state_t;

/** gPTP时间域状态 */
typedef enum {
    GPTP_DOMAIN_STATE_INACTIVE = 0,
    GPTP_DOMAIN_STATE_ACTIVE,
    GPTP_DOMAIN_STATE_SYNCING,
    GPTP_DOMAIN_STATE_SYNCHRONIZED,
    GPTP_DOMAIN_STATE_FAULT,
} gptp_domain_state_t;

/* ============================================================================
 * gPTP配置结构
 * ============================================================================ */

/** gPTP端口配置 */
typedef struct {
    uint16_t port_id;                       /* 端口ID */
    eth_mac_addr_t mac_addr;                /* MAC地址 */
    bool is_edge_port;                      /* 是否为边界端口 */
    
    /* 消息间隔配置 */
    int8_t log_sync_interval;               /* Sync间隔对数 */
    int8_t log_pdelay_req_interval;         /* Pdelay_Req间隔对数 */
    int8_t log_announce_interval;           /* Announce间隔对数 */
    
    /* 超时配置 */
    uint8_t sync_receipt_timeout;           /* Sync接收超时 */
    uint8_t announce_receipt_timeout;       /* Announce接收超时 */
    
    /* 延迟不对称性补偿 */
    int32_t delay_asymmetry_ns;             /* 延迟不对称性 (ns) */
    
    /* 安全相关 */
    bool enable_safety_monitoring;          /* 启用安全监控 */
    uint32_t safety_monitor_interval_ms;    /* 安全监控间隔 */
} gptp_port_config_t;

/** gPTP时间域配置 */
typedef struct {
    uint8_t domain_number;                  /* 域编号 */
    uint8_t priority1;                      /* 优先级1 */
    uint8_t priority2;                      /* 优先级2 */
    gptp_clock_quality_t clock_quality;     /* 时钟质量 */
    gptp_clock_identity_t clock_identity;   /* 时钟标识 */
} gptp_domain_config_t;

/** gPTP全局配置 */
typedef struct {
    uint8_t domain_count;                   /* 时间域数量 */
    uint8_t port_count;                     /* 端口数量 */
    bool gm_capable;                        /* 是否可作为Grandmaster */
    uint32_t sync_precision_ns;             /* 同步精度要求 */
    bool enable_asil_d;                     /* ASIL-D安全等级 */
} gptp_config_t;

/* ============================================================================
 * 运行时数据结构
 * ============================================================================ */

/** Pdelay测量结果 */
typedef struct {
    gptp_timestamp_t t1;                /* Pdelay_Req发送时间 */
    gptp_timestamp_t t2;                /* Pdelay_Req接收时间 */
    gptp_timestamp_t t3;                /* Pdelay_Resp发送时间 */
    gptp_timestamp_t t4;                /* Pdelay_Resp接收时间 */
    int64_t neighbor_prop_delay_ns;     /* 邻居传播延迟 */
    int64_t neighbor_rate_ratio;        /* 邻居速率比 */
    bool valid;                         /* 测量是否有效 */
} gptp_pdelay_result_t;

/** Sync时间戳 */
typedef struct {
    gptp_timestamp_t origin_timestamp;      /* 源时间戳 */
    gptp_timestamp_t precise_origin_timestamp;  /* 精确源时间戳 */
    gptp_timestamp_t ingress_timestamp;     /* 接收时间戳 */
    int64_t correction_field_ns;            /* 校正域 */
    bool follow_up_received;                /* 是否收到Follow_Up */
} gptp_sync_data_t;

/** 时钟状态 */
typedef struct {
    /* 当前时间 */
    gptp_timestamp_t current_time;
    
    /* 主从状态 */
    gptp_port_identity_t master_port_id;    /* 主端口ID */
    gptp_clock_identity_t gm_identity;      /* Grandmaster标识 */
    
    /* 时间偏差 */
    int64_t master_offset_ns;               /* 主时钟偏移 */
    int64_t slave_offset_ns;                /* 从时钟偏移 */
    
    /* 速率比 */
    int64_t rate_ratio;                     /* 速率比 */
    int64_t rate_ratio_ppb;                 /* 速率比 ppb */
    
    /* 同步状态 */
    bool synchronized;                      /* 是否已同步 */
    uint32_t sync_count;                    /* 同步计数 */
    uint32_t consecutive_sync_errors;       /* 连续同步错误 */
} gptp_clock_state_t;

/* ============================================================================
 * 安全监控结构
 * ============================================================================ */

/** ASIL-D安全监控 */
typedef struct {
    /* 时钟漂移监控 */
    int64_t max_clock_drift_ppb;
    int64_t current_drift_ppb;
    uint32_t drift_violations;
    
    /* 同步监控 */
    uint32_t sync_timeouts;
    uint32_t max_consecutive_timeouts;
    
    /* 完整性检查 */
    bool timestamp_integrity_ok;
    bool bmc_integrity_ok;
    
    /* 故障注入测试 */
    bool fault_injection_active;
    uint32_t fault_scenario;
} gptp_safety_monitor_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 时间戳回调 */
typedef void (*gptp_timestamp_callback_t)(
    uint16_t port_id,
    gptp_msg_type_t msg_type,
    const gptp_timestamp_t *timestamp,
    void *user_data
);

/** 同步状态回调 */
typedef void (*gptp_sync_callback_t)(
    bool synchronized,
    int64_t offset_ns,
    int64_t rate_ratio_ppb,
    void *user_data
);

/** 安全告警回调 */
typedef void (*gptp_safety_alert_callback_t)(
    uint32_t alert_type,
    const char *alert_msg,
    void *user_data
);

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化gPTP协议栈
 * @param config 全局配置
 * @return ETH_OK成功
 */
eth_status_t gptp_init(const gptp_config_t *config);

/**
 * @brief 反初始化gPTP协议栈
 */
void gptp_deinit(void);

/**
 * @brief 启动gPTP时间同步
 * @param domain_index 域索引
 * @return ETH_OK成功
 */
eth_status_t gptp_start(uint8_t domain_index);

/**
 * @brief 停止gPTP时间同步
 * @param domain_index 域索引
 * @return ETH_OK成功
 */
eth_status_t gptp_stop(uint8_t domain_index);

/**
 * @brief 配置gPTP端口
 * @param port_index 端口索引
 * @param port_config 端口配置
 * @return ETH_OK成功
 */
eth_status_t gptp_config_port(uint8_t port_index, const gptp_port_config_t *port_config);

/**
 * @brief 配置gPTP时间域
 * @param domain_index 域索引
 * @param domain_config 域配置
 * @return ETH_OK成功
 */
eth_status_t gptp_config_domain(uint8_t domain_index, const gptp_domain_config_t *domain_config);

/* ============================================================================
 * 时间戳和时钟API
 * ============================================================================ */

/**
 * @brief 获取当前gPTP时间
 * @param domain_index 域索引
 * @param timestamp 时间戳输出
 * @return ETH_OK成功
 */
eth_status_t gptp_get_time(uint8_t domain_index, gptp_timestamp_t *timestamp);

/**
 * @brief 设置gPTP时间 (仅限Grandmaster)
 * @param domain_index 域索引
 * @param timestamp 时间戳输入
 * @return ETH_OK成功
 */
eth_status_t gptp_set_time(uint8_t domain_index, const gptp_timestamp_t *timestamp);

/**
 * @brief 调整时钟频率 (速率比)
 * @param domain_index 域索引
 * @param rate_ratio_ppb 速率比 (ppb)
 * @return ETH_OK成功
 */
eth_status_t gptp_adjust_rate(uint8_t domain_index, int64_t rate_ratio_ppb);

/**
 * @brief 调整时钟相位 (时间跳跃)
 * @param domain_index 域索引
 * @param offset_ns 偏移量 (ns)
 * @return ETH_OK成功
 */
eth_status_t gptp_adjust_phase(uint8_t domain_index, int64_t offset_ns);

/**
 * @brief 捕获硬件时间戳
 * @param port_id 端口ID
 * @param timestamp 时间戳输出
 * @return ETH_OK成功
 */
eth_status_t gptp_capture_timestamp(uint16_t port_id, gptp_timestamp_t *timestamp);

/* ============================================================================
 * BMC (Best Master Clock) API
 * ============================================================================ */

/**
 * @brief 执行BMC算法
 * @param domain_index 域索引
 * @return ETH_OK成功
 */
eth_status_t gptp_run_bmc(uint8_t domain_index);

/**
 * @brief 获取当前Grandmaster标识
 * @param domain_index 域索引
 * @param gm_identity Grandmaster标识输出
 * @return ETH_OK成功
 */
eth_status_t gptp_get_gm_identity(uint8_t domain_index, gptp_clock_identity_t *gm_identity);

/**
 * @brief 获取当前端口状态
 * @param port_index 端口索引
 * @param state 状态输出
 * @return ETH_OK成功
 */
eth_status_t gptp_get_port_state(uint8_t port_index, gptp_port_state_t *state);

/* ============================================================================
 * Pdelay测量API
 * ============================================================================ */

/**
 * @brief 启动Pdelay测量
 * @param port_index 端口索引
 * @return ETH_OK成功
 */
eth_status_t gptp_start_pdelay_measurement(uint8_t port_index);

/**
 * @brief 获取Pdelay测量结果
 * @param port_index 端口索引
 * @param result 测量结果输出
 * @return ETH_OK成功
 */
eth_status_t gptp_get_pdelay_result(uint8_t port_index, gptp_pdelay_result_t *result);

/**
 * @brief 处理Pdelay_Req消息
 * @param port_index 端口索引
 * @param rx_timestamp 接收时间戳
 * @return ETH_OK成功
 */
eth_status_t gptp_handle_pdelay_req(uint8_t port_index, const gptp_timestamp_t *rx_timestamp);

/**
 * @brief 处理Pdelay_Resp消息
 * @param port_index 端口索引
 * @param resp_data 响应数据
 * @return ETH_OK成功
 */
eth_status_t gptp_handle_pdelay_resp(uint8_t port_index, const void *resp_data);

/* ============================================================================
 * Sync消息处理API
 * ============================================================================ */

/**
 * @brief 发送Sync消息
 * @param port_index 端口索引
 * @param sync_interval_ms 同步间隔
 * @return ETH_OK成功
 */
eth_status_t gptp_send_sync(uint8_t port_index, uint32_t sync_interval_ms);

/**
 * @brief 处理接收到的Sync消息
 * @param port_index 端口索引
 * @param sync_data Sync数据
 * @return ETH_OK成功
 */
eth_status_t gptp_handle_sync(uint8_t port_index, const gptp_sync_data_t *sync_data);

/**
 * @brief 处理Follow_Up消息
 * @param port_index 端口索引
 * @param precise_origin_timestamp 精确源时间戳
 * @param correction_field_ns 校正域
 * @return ETH_OK成功
 */
eth_status_t gptp_handle_follow_up(uint8_t port_index, 
                                    const gptp_timestamp_t *precise_origin_timestamp,
                                    int64_t correction_field_ns);

/* ============================================================================
 * 状态查询API
 * ============================================================================ */

/**
 * @brief 获取时钟状态
 * @param domain_index 域索引
 * @param state 状态输出
 * @return ETH_OK成功
 */
eth_status_t gptp_get_clock_state(uint8_t domain_index, gptp_clock_state_t *state);

/**
 * @brief 获取同步精度
 * @param domain_index 域索引
 * @param precision_ns 精度输出 (ns)
 * @return ETH_OK成功
 */
eth_status_t gptp_get_sync_precision(uint8_t domain_index, uint32_t *precision_ns);

/**
 * @brief 检查是否已同步
 * @param domain_index 域索引
 * @param synchronized 同步状态输出
 * @return ETH_OK成功
 */
eth_status_t gptp_is_synchronized(uint8_t domain_index, bool *synchronized);

/* ============================================================================
 * 回调注册API
 * ============================================================================ */

/**
 * @brief 注册时间戳回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t gptp_register_timestamp_callback(gptp_timestamp_callback_t callback, void *user_data);

/**
 * @brief 注册同步状态回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t gptp_register_sync_callback(gptp_sync_callback_t callback, void *user_data);

/**
 * @brief 注册安全告警回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t gptp_register_safety_alert_callback(gptp_safety_alert_callback_t callback, void *user_data);

/* ============================================================================
 * ASIL-D安全API
 * ============================================================================ */

/**
 * @brief 初始化安全监控
 * @param domain_index 域索引
 * @return ETH_OK成功
 */
eth_status_t gptp_init_safety_monitor(uint8_t domain_index);

/**
 * @brief 执行安全监控检查
 * @param domain_index 域索引
 * @return ETH_OK通过检查
 */
eth_status_t gptp_run_safety_checks(uint8_t domain_index);

/**
 * @brief 获取安全监控状态
 * @param domain_index 域索引
 * @param monitor 监控状态输出
 * @return ETH_OK成功
 */
eth_status_t gptp_get_safety_monitor(uint8_t domain_index, gptp_safety_monitor_t *monitor);

/**
 * @brief 启用故障注入测试
 * @param domain_index 域索引
 * @param scenario 故障场景
 * @return ETH_OK成功
 */
eth_status_t gptp_enable_fault_injection(uint8_t domain_index, uint32_t scenario);

/**
 * @brief 执行时钟完整性检查
 * @param domain_index 域索引
 * @param integrity_ok 完整性状态输出
 * @return ETH_OK成功
 */
eth_status_t gptp_check_clock_integrity(uint8_t domain_index, bool *integrity_ok);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 时间戳加法
 * @param a 时间戳A
 * @param b_ns 纳秒偏移
 * @param result 结果输出
 */
void gptp_timestamp_add(const gptp_timestamp_t *a, int64_t b_ns, gptp_timestamp_t *result);

/**
 * @brief 时间戳减法
 * @param a 时间戳A
 * @param b 时间戳B
 * @return 纳秒差值 (a - b)
 */
int64_t gptp_timestamp_subtract(const gptp_timestamp_t *a, const gptp_timestamp_t *b);

/**
 * @brief 时间戳转纳秒
 * @param ts 时间戳
 * @return 纳秒值
 */
uint64_t gptp_timestamp_to_ns(const gptp_timestamp_t *ts);

/**
 * @brief 纳秒转时间戳
 * @param ns 纳秒值
 * @param ts 时间戳输出
 */
void gptp_ns_to_timestamp(uint64_t ns, gptp_timestamp_t *ts);

/**
 * @brief 格式化时间戳为字符串
 * @param ts 时间戳
 * @param buf 缓冲区
 * @param buf_size 缓冲区大小
 * @return 格式化后的字符串
 */
const char* gptp_timestamp_to_string(const gptp_timestamp_t *ts, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* TSN_GPTP_H */

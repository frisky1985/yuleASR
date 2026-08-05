/**
 * @file stream_reservation.h
 * @brief 流预留协议 (IEEE 802.1Qcc-2018 / 802.1Qca-2015)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note MSRP协议实现
 * @note 符合ASIL-D安全等级
 */

#ifndef TSN_STREAM_RESERVATION_H
#define TSN_STREAM_RESERVATION_H

#include "../../common/types/eth_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * IEEE 802.1Q 常量定义
 * ============================================================================ */

/** 最大流数量 */
#define SRP_MAX_STREAMS                 256
#define SRP_MAX_LISTENERS               256
#define SRP_MAX_TALKERS                 128

/** MSRP EtherType */
#define MSRP_ETHERTYPE                  0x22EA

/** 流标识符长度 */
#define SRP_STREAM_ID_LEN               8

/** 最大跳数 */
#define SRP_MAX_HOPS                    32

/** SR Class */
#define SRP_SR_CLASS_A                  0
#define SRP_SR_CLASS_B                  1

/** 带宽限制 */
#define SRP_MAX_BANDWIDTH_PERCENT       75
#define SRP_MIN_BANDWIDTH_RESERVATION   100       /* 100 bps */

/** ASIL-D安全相关 */
#define SRP_SAFETY_BANDWIDTH_MARGIN     10        /* 10% 安全余量 */
#define SRP_MAX_RESERVATION_LATENCY_US  1000      /* 1ms */

/* ============================================================================
 * MSRP 消息类型
 * ============================================================================ */

typedef enum {
    SRP_MSG_TYPE_TALKER_ADVERTISE = 0,      /* Talker广告 */
    SRP_MSG_TYPE_TALKER_FAILED,             /* Talker失败 */
    SRP_MSG_TYPE_LISTENER_READY,            /* Listener就绪 */
    SRP_MSG_TYPE_LISTENER_READY_FAILED,     /* Listener就绪失败 */
    SRP_MSG_TYPE_LISTENER_ASKING_FAILED,    /* Listener请求失败 */
    SRP_MSG_TYPE_DOMAIN,                    /* 域消息 */
} srp_msg_type_t;

/* ============================================================================
 * 流标识和属性
 * ============================================================================ */

/** 流标识符 (64-bit) */
typedef uint8_t srp_stream_id_t[SRP_STREAM_ID_LEN];

/** 数据帧流规范 */
typedef struct {
    uint16_t vlan_id;                       /* VLAN ID */
    uint8_t priority;                       /* 优先级 */
    uint16_t rank;                          /* 流等级 (0=Emergency, 1=Non-Emergency) */
    bool pcp_encoding;                      /* PCP编码 */
} srp_data_frame_spec_t;

/** TSpec带宽规范 */
typedef struct {
    uint32_t interval_numerator;            /* 间隔分子 */
    uint32_t interval_denominator;          /* 间隔分母 */
    uint16_t max_frame_size;                /* 最大帧大小 */
    uint16_t max_interval_frames;           /* 每间隔最大帧数 */
    bool preemption_enabled;                /* 抢占使能 */
} srp_tspec_t;

/** 累积延迟 */
typedef struct {
    uint32_t bridge_delay;                  /* 桥接延迟 */
    bool independent_delay;                 /* 是否为独立延迟 */
} srp_accumulated_latency_t;

/** 失败信息 */
typedef struct {
    uint8_t system_id[8];                   /* 失败系统ID */
    uint8_t failure_code;                   /* 失败代码 */
} srp_failure_info_t;

/* ============================================================================
 * Talker 声明
 * ============================================================================ */

typedef struct {
    srp_stream_id_t stream_id;              /* 流标识符 */
    eth_mac_addr_t dest_address;            /* 目标MAC地址 */
    srp_data_frame_spec_t data_frame_spec;  /* 数据帧规范 */
    srp_tspec_t tspec;                      /* TSpec */
    srp_accumulated_latency_t accumulated_latency;  /* 累积延迟 */
    srp_failure_info_t failure_info;        /* 失败信息 */
    bool failed;                            /* 是否失败 */
} srp_talker_declaration_t;

/* ============================================================================
 * Listener 声明
 * ============================================================================ */

typedef enum {
    SRP_LISTENER_STATE_NONE = 0,
    SRP_LISTENER_STATE_ASKING_FAILED,
    SRP_LISTENER_STATE_READY,
    SRP_LISTENER_STATE_READY_FAILED,
} srp_listener_state_t;

typedef struct {
    srp_stream_id_t stream_id;              /* 流标识符 */
    srp_listener_state_t state;             /* Listener状态 */
    srp_failure_info_t failure_info;        /* 失败信息 */
} srp_listener_declaration_t;

/* ============================================================================
 * 域声明
 * ============================================================================ */

typedef struct {
    uint8_t sr_class_id;                    /* SR类ID */
    uint8_t sr_class_priority;              /* SR类优先级 */
    uint16_t sr_class_vid;                  /* SR类VLAN ID */
} srp_domain_declaration_t;

/* ============================================================================
 * 流预留状态
 * ============================================================================ */

typedef enum {
    SRP_RESERVATION_STATE_NONE = 0,
    SRP_RESERVATION_STATE_REGISTERING,
    SRP_RESERVATION_STATE_REGISTERED,
    SRP_RESERVATION_STATE_DEREGISTERING,
    SRP_RESERVATION_STATE_FAILED,
} srp_reservation_state_t;

/** 流预留记录 */
typedef struct {
    srp_stream_id_t stream_id;              /* 流标识符 */
    srp_reservation_state_t state;          /* 预留状态 */
    
    /* Talker信息 */
    srp_talker_declaration_t talker;        /* Talker声明 */
    bool talker_registered;                 /* Talker是否注册 */
    
    /* Listener信息 */
    uint32_t listener_count;                /* Listener数量 */
    srp_listener_declaration_t listeners[SRP_MAX_LISTENERS];  /* Listener列表 */
    
    /* 带宽信息 */
    uint32_t reserved_bandwidth_bps;        /* 预留带宽 */
    uint32_t actual_bandwidth_bps;          /* 实际带宽 */
    
    /* 路径信息 */
    uint8_t hop_count;                      /* 跳数 */
    uint32_t accumulated_latency_us;        /* 累积延迟 */
    
    /* 安全相关 */
    uint32_t safety_margin_percent;         /* 安全余量百分比 */
} srp_stream_reservation_t;

/* ============================================================================
 * 带宽管理
 * ============================================================================ */

/** 端口带宽状态 */
typedef struct {
    uint16_t port_id;                       /* 端口ID */
    uint32_t total_bandwidth_bps;           /* 总带宽 */
    uint32_t available_bandwidth_bps;       /* 可用带宽 */
    uint32_t reserved_bandwidth_bps;        /* 已预留带宽 */
    uint32_t actual_usage_bps;              /* 实际使用带宽 */
} srp_port_bandwidth_t;

/** 流路径 */
typedef struct {
    srp_stream_id_t stream_id;              /* 流标识符 */
    uint16_t port_list[SRP_MAX_HOPS];       /* 端口列表 */
    uint8_t port_count;                     /* 端口数量 */
    uint32_t total_latency_us;              /* 总延迟 */
    bool path_calculated;                   /* 路径是否已计算 */
} srp_stream_path_t;

/* ============================================================================
 * 安全监控
 * ============================================================================ */

typedef struct {
    /* 带宽监控 */
    bool bandwidth_violated;
    uint32_t bandwidth_violation_count;
    
    /* 延迟监控 */
    bool latency_violated;
    uint32_t latency_violation_count;
    
    /* 预留完整性 */
    bool reservation_integrity_ok;
    uint32_t integrity_violations;
    
    /* 故障检测 */
    bool fault_detected;
    uint32_t fault_code;
} srp_safety_monitor_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

typedef void (*srp_talker_registered_callback_t)(
    const srp_stream_id_t stream_id,
    const srp_talker_declaration_t *talker,
    void *user_data
);

typedef void (*srp_listener_registered_callback_t)(
    const srp_stream_id_t stream_id,
    const srp_listener_declaration_t *listener,
    void *user_data
);

typedef void (*srp_reservation_changed_callback_t)(
    const srp_stream_id_t stream_id,
    srp_reservation_state_t old_state,
    srp_reservation_state_t new_state,
    void *user_data
);

typedef void (*srp_bandwidth_alert_callback_t)(
    uint16_t port_id,
    uint32_t bandwidth_percent,
    void *user_data
);

typedef void (*srp_safety_alert_callback_t)(
    uint16_t port_id,
    uint32_t alert_type,
    const char *alert_msg,
    void *user_data
);

/* ============================================================================
 * 初始化和配置API
 * ============================================================================ */

/**
 * @brief 初始化流预留协议
 * @return ETH_OK成功
 */
eth_status_t srp_init(void);

/**
 * @brief 反初始化流预留协议
 */
void srp_deinit(void);

/**
 * @brief 配置端口带宽
 * @param port_id 端口ID
 * @param total_bandwidth_bps 总带宽
 * @return ETH_OK成功
 */
eth_status_t srp_config_port_bandwidth(uint16_t port_id, uint32_t total_bandwidth_bps);

/**
 * @brief 配置SR Class
 * @param port_id 端口ID
 * @param class_id Class ID
 * @param domain 域声明
 * @return ETH_OK成功
 */
eth_status_t srp_config_sr_class(uint16_t port_id, uint8_t class_id,
                                  const srp_domain_declaration_t *domain);

/* ============================================================================
 * Talker API
 * ============================================================================ */

/**
 * @brief 注册Talker
 * @param port_id 端口ID
 * @param talker Talker声明
 * @return ETH_OK成功
 */
eth_status_t srp_register_talker(uint16_t port_id, 
                                  const srp_talker_declaration_t *talker);

/**
 * @brief 注销Talker
 * @param port_id 端口ID
 * @param stream_id 流标识符
 * @return ETH_OK成功
 */
eth_status_t srp_deregister_talker(uint16_t port_id, const srp_stream_id_t stream_id);

/**
 * @brief 更新Talker声明
 * @param port_id 端口ID
 * @param talker Talker声明
 * @return ETH_OK成功
 */
eth_status_t srp_update_talker(uint16_t port_id, const srp_talker_declaration_t *talker);

/**
 * @brief 获取Talker声明
 * @param port_id 端口ID
 * @param stream_id 流标识符
 * @param talker Talker声明输出
 * @return ETH_OK成功
 */
eth_status_t srp_get_talker(uint16_t port_id, const srp_stream_id_t stream_id,
                             srp_talker_declaration_t *talker);

/* ============================================================================
 * Listener API
 * ============================================================================ */

/**
 * @brief 注册Listener
 * @param port_id 端口ID
 * @param listener Listener声明
 * @return ETH_OK成功
 */
eth_status_t srp_register_listener(uint16_t port_id,
                                    const srp_listener_declaration_t *listener);

/**
 * @brief 注销Listener
 * @param port_id 端口ID
 * @param stream_id 流标识符
 * @return ETH_OK成功
 */
eth_status_t srp_deregister_listener(uint16_t port_id, const srp_stream_id_t stream_id);

/**
 * @brief 更新Listener状态
 * @param port_id 端口ID
 * @param stream_id 流标识符
 * @param state 新状态
 * @return ETH_OK成功
 */
eth_status_t srp_update_listener_state(uint16_t port_id, const srp_stream_id_t stream_id,
                                        srp_listener_state_t state);

/* ============================================================================
 * 流预留API
 * ============================================================================ */

/**
 * @brief 预留流带宽
 * @param port_id 端口ID
 * @param stream_id 流标识符
 * @param bandwidth_bps 带宽需求 (bps)
 * @return ETH_OK成功
 */
eth_status_t srp_reserve_bandwidth(uint16_t port_id, const srp_stream_id_t stream_id,
                                    uint32_t bandwidth_bps);

/**
 * @brief 释放预留带宽
 * @param port_id 端口ID
 * @param stream_id 流标识符
 * @return ETH_OK成功
 */
eth_status_t srp_release_bandwidth(uint16_t port_id, const srp_stream_id_t stream_id);

/**
 * @brief 计算流路径
 * @param stream_id 流标识符
 * @param path 路径信息输出
 * @return ETH_OK成功
 */
eth_status_t srp_calculate_path(const srp_stream_id_t stream_id, 
                                 srp_stream_path_t *path);

/**
 * @brief 验证流预留可行性
 * @param port_id 端口ID
 * @param bandwidth_bps 带宽需求
 * @param latency_us 延迟需求
 * @param feasible 是否可行输出
 * @return ETH_OK成功
 */
eth_status_t srp_check_feasibility(uint16_t port_id, uint32_t bandwidth_bps,
                                    uint32_t latency_us, bool *feasible);

/* ============================================================================
 * 带宽查询API
 * ============================================================================ */

/**
 * @brief 获取端口带宽状态
 * @param port_id 端口ID
 * @param bandwidth 带宽状态输出
 * @return ETH_OK成功
 */
eth_status_t srp_get_port_bandwidth(uint16_t port_id, srp_port_bandwidth_t *bandwidth);

/**
 * @brief 获取流预留状态
 * @param stream_id 流标识符
 * @param reservation 预留状态输出
 * @return ETH_OK成功
 */
eth_status_t srp_get_stream_reservation(const srp_stream_id_t stream_id,
                                         srp_stream_reservation_t *reservation);

/**
 * @brief 获取所有流预留
 * @param reservations 预留数组
 * @param count 数组大小/返回数量
 * @return ETH_OK成功
 */
eth_status_t srp_get_all_reservations(srp_stream_reservation_t *reservations, 
                                       uint32_t *count);

/**
 * @brief 计算流带宽需求
 * @param tspec TSpec规范
 * @param bandwidth_bps 带宽输出 (bps)
 * @return ETH_OK成功
 */
eth_status_t srp_calc_bandwidth_requirement(const srp_tspec_t *tspec,
                                             uint32_t *bandwidth_bps);

/* ============================================================================
 * 回调注册API
 * ============================================================================ */

eth_status_t srp_register_talker_callback(srp_talker_registered_callback_t callback,
                                           void *user_data);

eth_status_t srp_register_listener_callback(srp_listener_registered_callback_t callback,
                                             void *user_data);

eth_status_t srp_register_reservation_callback(srp_reservation_changed_callback_t callback,
                                                void *user_data);

eth_status_t srp_register_bandwidth_alert_callback(srp_bandwidth_alert_callback_t callback,
                                                    void *user_data);

eth_status_t srp_register_safety_alert_callback(srp_safety_alert_callback_t callback,
                                                 void *user_data);

/* ============================================================================
 * 安全监控API
 * ============================================================================ */

eth_status_t srp_init_safety_monitor(uint16_t port_id);

eth_status_t srp_run_safety_checks(uint16_t port_id);

eth_status_t srp_get_safety_monitor(uint16_t port_id, srp_safety_monitor_t *monitor);

eth_status_t srp_validate_reservation_integrity(const srp_stream_id_t stream_id,
                                                 bool *integrity_ok);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

eth_status_t srp_stream_id_to_string(const srp_stream_id_t stream_id, 
                                      char *buf, size_t buf_size);

eth_status_t srp_create_automotive_stream(const char *stream_name,
                                           uint16_t vlan_id,
                                           uint8_t priority,
                                           srp_stream_id_t stream_id);

eth_status_t srp_print_reservation(const srp_stream_id_t stream_id);

eth_status_t srp_print_port_status(uint16_t port_id);

#ifdef __cplusplus
}
#endif

#endif /* TSN_STREAM_RESERVATION_H */

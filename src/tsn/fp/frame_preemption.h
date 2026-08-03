/**
 * @file frame_preemption.h
 * @brief 帧抢占 (IEEE 802.1Qbu-2016)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note 快速帧切换 < 100ns
 * @note 符合ASIL-D安全等级
 */

#ifndef TSN_FRAME_PREEMPTION_H
#define TSN_FRAME_PREEMPTION_H

#include "../../common/types/eth_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * IEEE 802.1Qbu 常量定义
 * ============================================================================ */

/** mPacket相关常量 */
#define FP_MIN_MPACKET_SIZE             64      /* mPacket最小大小 */
#define FP_MAX_MPACKET_SIZE             1522    /* mPacket最大大小 */
#define FP_MIN_FRAG_SIZE                64      /* 最小分片大小 */
#define FP_MAX_FRAG_SIZE                512     /* 最大分片大小 (512B对齐) */

/** SMD (Start mDelimiter) 值 */
#define FP_SMD_S0                       0xE6    /* 开始帧分片 - 第一个 */
#define FP_SMD_S1                       0x4C    /* 开始帧分片 - 第二个 */
#define FP_SMD_S2                       0x7F    /* 开始帧分片 - 第三个 */
#define FP_SMD_S3                       0xB3    /* 开始帧分片 - 第四个 */
#define FP_SMD_C0                       0x61    /* 继续帧分片 - 第一个 */
#define FP_SMD_C1                       0x83    /* 继续帧分片 - 第二个 */
#define FP_SMD_C2                       0x9E    /* 继续帧分片 - 第三个 */
#define FP_SMD_C3                       0x2A    /* 继续帧分片 - 第四个 */
#define FP_SMD_E0                       0x87    /* 结束帧分片 - 第一个 */
#define FP_SMD_E1                       0xE4    /* 结束帧分片 - 第二个 */
#define FP_SMD_E2                       0xD2    /* 结束帧分片 - 第三个 */
#define FP_SMD_E3                       0x39    /* 结束帧分片 - 第四个 */

/** 响应 mDelimiter */
#define FP_SMD_R0                       0x78    /* 响应 - 第一个 */
#define FP_SMD_R1                       0x9A    /* 响应 - 第二个 */
#define FP_SMD_R2                       0xCD    /* 响应 - 第三个 */
#define FP_SMD_R3                       0x45    /* 响应 - 第四个 */

/** 校验和掩码 */
#define FP_CRC_MASK                     0x01

/** 抢占相关常量 */
#define FP_PREEMPTION_OVERHEAD          8       /* 抢占开销 (SMD + CRC) */
#define FP_HOLD_ADVANCE_NS              100     /* 保持提前量 (ns) */
#define FP_RELEASE_ADVANCE_NS           100     /* 释放提前量 (ns) */

/** 帧类型 */
#define FP_FRAME_TYPE_EXPRESS           0x01    /* 快速帧 */
#define FP_FRAME_TYPE_PREEMPTABLE       0x02    /* 可抢占帧 */

/** ASIL-D安全相关 */
#define FP_SAFETY_MAX_PREEMPTIONS       16      /* 最大抢占次数 */
#define FP_SAFETY_REASSEMBLY_TIMEOUT_US 1000    /* 重组超时 (1ms) */

/* ============================================================================
 * 帧抢占状态
 * ============================================================================ */

typedef enum {
    FP_STATE_INACTIVE = 0,          /* 未激活 */
    FP_STATE_IDLE,                  /* 空闲 */
    FP_STATE_TRANSMITTING_EXPRESS,  /* 传输快速帧 */
    FP_STATE_TRANSMITTING_PREEMPTABLE,  /* 传输可抢占帧 */
    FP_STATE_PREEMPTING,            /* 正在抢占 */
    FP_STATE_REASSEMBLING,          /* 重组中 */
    FP_STATE_ERROR,                 /* 错误状态 */
} fp_state_t;

/** 帧类型 */
typedef enum {
    FP_FRAME_UNKNOWN = 0,           /* 未知类型 */
    FP_FRAME_COMPLETE,              /* 完整帧 */
    FP_FRAME_FIRST_FRAGMENT,        /* 第一分片 */
    FP_FRAME_CONTINUE_FRAGMENT,     /* 中间分片 */
    FP_FRAME_LAST_FRAGMENT,         /* 最后分片 */
} fp_fragment_type_t;

/* ============================================================================
 * mPacket结构
 * ============================================================================ */

/** mPacket头部 */
typedef struct {
    uint8_t smd;                    /* Start mDelimiter */
    uint8_t frag_count;             /* 分片计数 */
    uint16_t frag_length;           /* 分片长度 */
} fp_mpacket_header_t;

/** mPacket尾部 */
typedef struct {
    uint32_t crc;                   /* CRC校验 */
} fp_mpacket_trailer_t;

/** mPacket描述符 */
typedef struct {
    uint8_t *data;                  /* 数据指针 */
    uint32_t len;                   /* 数据长度 */
    uint8_t smd;                    /* SMD值 */
    fp_fragment_type_t frag_type;   /* 分片类型 */
    uint8_t frag_sequence;          /* 分片序列号 */
    bool is_valid;                  /* 是否有效 */
} fp_mpacket_t;

/* ============================================================================
 * 可抢占帧状态
 * ============================================================================ */

/** 帧分片状态 */
typedef struct {
    uint8_t *frame_data;            /* 原始帧数据 */
    uint32_t frame_len;             /* 原始帧长度 */
    uint32_t bytes_transmitted;     /* 已传输字节数 */
    uint32_t frag_count;            /* 分片计数 */
    uint8_t frag_sequence;          /* 当前分片序列号 */
    bool in_progress;               /* 是否正在进行 */
    uint64_t start_time_ns;         /* 开始时间 */
} fp_preemptable_frame_state_t;

/** 帧重组上下文 */
typedef struct {
    uint8_t reassembly_buffer[FP_MAX_MPACKET_SIZE];  /* 重组缓冲区 */
    uint32_t reassembly_len;        /* 重组长度 */
    uint8_t expected_sequence;      /* 期望的序列号 */
    bool reassembly_active;         /* 重组是否激活 */
    uint64_t last_frag_time_ns;     /* 最后分片时间 */
} fp_reassembly_context_t;

/* ============================================================================
 * 抢占器配置
 * ============================================================================ */

/** 抢占器配置 */
typedef struct {
    uint16_t port_id;                       /* 端口ID */
    bool preemption_enabled;                /* 抢占使能 */
    bool express_preemption_enabled;        /* 快速帧抢占使能 */
    
    /* 分片参数 */
    uint32_t min_frag_size;                 /* 最小分片大小 */
    uint32_t max_frag_size;                 /* 最大分片大小 */
    
    /* 时间参数 */
    uint32_t hold_advance_ns;               /* 保持提前量 (ns) */
    uint32_t release_advance_ns;            /* 释放提前量 (ns) */
    
    /* 帧类型映射 */
    uint8_t express_priority_mask;          /* 快速帧优先级掩码 */
    uint8_t preemptable_priority_mask;      /* 可抢占帧优先级掩码 */
    
    /* 安全相关 */
    bool enable_safety_checks;              /* 启用安全检查 */
    uint32_t max_preemptions_per_frame;     /* 每帧最大抢占次数 */
    uint32_t reassembly_timeout_us;         /* 重组超时时间 */
} fp_config_t;

/* ============================================================================
 * 统计信息
 * ============================================================================ */

/** 抢占统计 */
typedef struct {
    uint64_t express_frames_tx;             /* 快速帧发送数 */
    uint64_t express_frames_rx;             /* 快速帧接收数 */
    uint64_t preemptable_frames_tx;         /* 可抢占帧发送数 */
    uint64_t preemptable_frames_rx;         /* 可抢占帧接收数 */
    uint64_t mpackets_tx;                   /* mPacket发送数 */
    uint64_t mpackets_rx;                   /* mPacket接收数 */
    uint64_t fragments_tx;                  /* 分片发送数 */
    uint64_t fragments_rx;                  /* 分片接收数 */
    uint64_t preemptions_count;             /* 抢占次数 */
    uint64_t reassembly_success;            /* 重组成功数 */
    uint64_t reassembly_failures;           /* 重组失败数 */
    uint64_t crc_errors;                    /* CRC错误数 */
    uint64_t timeout_errors;                /* 超时错误数 */
    uint32_t max_reassembly_time_us;        /* 最大重组时间 */
} fp_stats_t;

/* ============================================================================
 * 安全监控
 * ============================================================================ */

typedef struct {
    /* 完整性检查 */
    bool frame_integrity_ok;
    uint32_t integrity_violations;
    
    /* 重组监控 */
    uint32_t reassembly_timeouts;
    uint32_t sequence_errors;
    
    /* 抢占监控 */
    uint32_t max_preemptions_exceeded;
    
    /* 错误检测 */
    bool fault_detected;
    uint32_t fault_code;
} fp_safety_monitor_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 帧发送回调 */
typedef void (*fp_tx_callback_t)(
    uint16_t port_id,
    const uint8_t *data,
    uint32_t len,
    bool is_express,
    void *user_data
);

/** 帧接收回调 */
typedef void (*fp_rx_callback_t)(
    uint16_t port_id,
    const uint8_t *data,
    uint32_t len,
    bool is_complete,
    void *user_data
);

/** 抢占事件回调 */
typedef void (*fp_preemption_event_callback_t)(
    uint16_t port_id,
    uint32_t event_type,
    void *user_data
);

/** 安全告警回调 */
typedef void (*fp_safety_alert_callback_t)(
    uint16_t port_id,
    uint32_t alert_type,
    const char *alert_msg,
    void *user_data
);

/* ============================================================================
 * 初始化和配置API
 * ============================================================================ */

/**
 * @brief 初始化帧抢占模块
 * @return ETH_OK成功
 */
eth_status_t fp_init(void);

/**
 * @brief 反初始化帧抢占模块
 */
void fp_deinit(void);

/**
 * @brief 配置端口抢占参数
 * @param port_id 端口ID
 * @param config 配置参数
 * @return ETH_OK成功
 */
eth_status_t fp_config_port(uint16_t port_id, const fp_config_t *config);

/**
 * @brief 获取端口配置
 * @param port_id 端口ID
 * @param config 配置输出
 * @return ETH_OK成功
 */
eth_status_t fp_get_port_config(uint16_t port_id, fp_config_t *config);

/**
 * @brief 启用/禁用抢占
 * @param port_id 端口ID
 * @param enable 使能状态
 * @return ETH_OK成功
 */
eth_status_t fp_enable_preemption(uint16_t port_id, bool enable);

/**
 * @brief 配置帧类型映射
 * @param port_id 端口ID
 * @param express_mask 快速帧优先级掩码
 * @param preemptable_mask 可抢占帧优先级掩码
 * @return ETH_OK成功
 */
eth_status_t fp_set_frame_type_mapping(uint16_t port_id, 
                                        uint8_t express_mask,
                                        uint8_t preemptable_mask);

/* ============================================================================
 * 发送API
 * ============================================================================ */

/**
 * @brief 发送快速帧 (不可抢占)
 * @param port_id 端口ID
 * @param data 帧数据
 * @param len 帧长度
 * @return ETH_OK成功
 */
eth_status_t fp_send_express_frame(uint16_t port_id, const uint8_t *data, uint32_t len);

/**
 * @brief 发送可抢占帧
 * @param port_id 端口ID
 * @param data 帧数据
 * @param len 帧长度
 * @return ETH_OK成功
 */
eth_status_t fp_send_preemptable_frame(uint16_t port_id, const uint8_t *data, uint32_t len);

/**
 * @brief 抢占当前传输
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t fp_preempt_transmission(uint16_t port_id);

/**
 * @brief 恢复被抢占的传输
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t fp_resume_transmission(uint16_t port_id);

/**
 * @brief 检查是否可以抢占
 * @param port_id 端口ID
 * @param can_preempt 是否可抢占输出
 * @return ETH_OK成功
 */
eth_status_t fp_can_preempt(uint16_t port_id, bool *can_preempt);

/* ============================================================================
 * 分片和重组API
 * ============================================================================ */

/**
 * @brief 分片帧为mPacket
 * @param frame_data 帧数据
 * @param frame_len 帧长度
 * @param frag_size 分片大小
 * @param mpackets mPacket数组输出
 * @param mpacket_count mPacket数量输出
 * @return ETH_OK成功
 */
eth_status_t fp_fragment_frame(const uint8_t *frame_data, uint32_t frame_len,
                                uint32_t frag_size,
                                fp_mpacket_t *mpackets, uint32_t *mpacket_count);

/**
 * @brief 重组mPacket为完整帧
 * @param mpacket mPacket
 * @param frame_data 帧数据输出
 * @param frame_len 帧长度输出
 * @param is_complete 是否完整输出
 * @return ETH_OK成功
 */
eth_status_t fp_reassemble_mpacket(const fp_mpacket_t *mpacket,
                                    uint8_t *frame_data, uint32_t *frame_len,
                                    bool *is_complete);

/**
 * @brief 处理接收到的mPacket
 * @param port_id 端口ID
 * @param mpacket mPacket数据
 * @return ETH_OK成功
 */
eth_status_t fp_rx_mpacket(uint16_t port_id, const fp_mpacket_t *mpacket);

/* ============================================================================
 * 状态查询API
 * ============================================================================ */

/**
 * @brief 获取当前状态
 * @param port_id 端口ID
 * @param state 状态输出
 * @return ETH_OK成功
 */
eth_status_t fp_get_state(uint16_t port_id, fp_state_t *state);

/**
 * @brief 获取统计信息
 * @param port_id 端口ID
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t fp_get_stats(uint16_t port_id, fp_stats_t *stats);

/**
 * @brief 清除统计信息
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t fp_clear_stats(uint16_t port_id);

/**
 * @brief 获取当前传输状态
 * @param port_id 端口ID
 * @param preemptable_state 可抢占帧状态输出
 * @return ETH_OK成功
 */
eth_status_t fp_get_preemptable_state(uint16_t port_id, 
                                       fp_preemptable_frame_state_t *preemptable_state);

/* ============================================================================
 * 回调注册API
 * ============================================================================ */

/**
 * @brief 注册帧发送回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t fp_register_tx_callback(fp_tx_callback_t callback, void *user_data);

/**
 * @brief 注册帧接收回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t fp_register_rx_callback(fp_rx_callback_t callback, void *user_data);

/**
 * @brief 注册抢占事件回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t fp_register_preemption_event_callback(fp_preemption_event_callback_t callback, 
                                                    void *user_data);

/**
 * @brief 注册安全告警回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t fp_register_safety_alert_callback(fp_safety_alert_callback_t callback,
                                                void *user_data);

/* ============================================================================
 * 安全监控API
 * ============================================================================ */

/**
 * @brief 初始化安全监控
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t fp_init_safety_monitor(uint16_t port_id);

/**
 * @brief 执行安全检查
 * @param port_id 端口ID
 * @return ETH_OK通过检查
 */
eth_status_t fp_run_safety_checks(uint16_t port_id);

/**
 * @brief 获取安全监控状态
 * @param port_id 端口ID
 * @param monitor 监控状态输出
 * @return ETH_OK成功
 */
eth_status_t fp_get_safety_monitor(uint16_t port_id, fp_safety_monitor_t *monitor);

/**
 * @brief 检查帧完整性
 * @param port_id 端口ID
 * @param integrity_ok 完整性状态输出
 * @return ETH_OK成功
 */
eth_status_t fp_check_frame_integrity(uint16_t port_id, bool *integrity_ok);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 判断是否为快速帧
 * @param priority 帧优先级
 * @param config 抢占配置
 * @return true是快速帧
 */
bool fp_is_express_frame(uint8_t priority, const fp_config_t *config);

/**
 * @brief 判断是否为可抢占帧
 * @param priority 帧优先级
 * @param config 抢占配置
 * @return true是可抢占帧
 */
bool fp_is_preemptable_frame(uint8_t priority, const fp_config_t *config);

/**
 * @brief 计算mPacket CRC
 * @param data 数据
 * @param len 长度
 * @return CRC值
 */
uint32_t fp_calc_mpacket_crc(const uint8_t *data, uint32_t len);

/**
 * @brief 解析mPacket头部
 * @param data 数据
 * @param len 长度
 * @param mpacket mPacket输出
 * @return ETH_OK成功
 */
eth_status_t fp_parse_mpacket_header(const uint8_t *data, uint32_t len, 
                                      fp_mpacket_t *mpacket);

/**
 * @brief 打印抢占器状态
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t fp_print_status(uint16_t port_id);

#ifdef __cplusplus
}
#endif

#endif /* TSN_FRAME_PREEMPTION_H */

/**
 * @file ota_dem_integration.h
 * @brief OTA DEM Integration Module
 * @version 1.0
 * @date 2026-04-26
 *
 * Integrates OTA with DEM (Diagnostic Event Manager) for DTC recording
 * Records OTA-related diagnostic trouble codes
 *
 * ISO 14229-1 UDS DTC compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_DEM_INTEGRATION_H
#define OTA_DEM_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define OTA_DEM_MAJOR_VERSION           1
#define OTA_DEM_MINOR_VERSION           0
#define OTA_DEM_PATCH_VERSION           0

/* ============================================================================
 * OTA相关DTC定义 (SAE J2012格式: 0xPPCCCC)
 * ============================================================================ */

/* OTA系统错误 (P0 = Powertrain类别) */
#define OTA_DTC_UPDATE_FAILED           0xF0A001  /* OTA更新失败 */
#define OTA_DTC_DOWNLOAD_FAILED         0xF0A002  /* 下载失败 */
#define OTA_DTC_VERIFICATION_FAILED     0xF0A003  /* 验证失败 */
#define OTA_DTC_INSTALLATION_FAILED     0xF0A004  /* 安装失败 */
#define OTA_DTC_ACTIVATION_FAILED       0xF0A005  /* 激活失败 */

/* 安全错误 */
#define OTA_DTC_SIGNATURE_INVALID       0xF0B001  /* 签名无效 */
#define OTA_DTC_HASH_MISMATCH           0xF0B002  /* 哈希不匹配 */
#define OTA_DTC_VERSION_ROLLBACK        0xF0B003  /* 版本回滚 */
#define OTA_DTC_CERT_INVALID            0xF0B004  /* 证书无效 */
#define OTA_DTC_SECURITY_TIMEOUT        0xF0B005  /* 安全验证超时 */

/* 通信错误 */
#define OTA_DTC_COMM_TIMEOUT            0xF0C001  /* 通信超时 */
#define OTA_DTC_COMM_ERROR              0xF0C002  /* 通信错误 */
#define OTA_DTC_DDS_PUBLISH_FAILED      0xF0C003  /* DDS发布失败 */

/* ECU特定错误 */
#define OTA_DTC_ECU_NOT_RESPONDING      0xF0D001  /* ECU无响应 */
#define OTA_DTC_ECU_REJECTED            0xF0D002  /* ECU拒绝更新 */
#define OTA_DTC_ECU_INCOMPATIBLE        0xF0D003  /* ECU不兼容 */

/* ============================================================================
 * DTC状态定义 (ISO 14229-1 DTCStatusMask)
 * ============================================================================ */
#define OTA_DTC_STATUS_TEST_FAILED              0x01
#define OTA_DTC_STATUS_TEST_FAILED_THIS_CYCLE   0x02
#define OTA_DTC_STATUS_PENDING                  0x04
#define OTA_DTC_STATUS_CONFIRMED                0x08
#define OTA_DTC_STATUS_TEST_NOT_COMPLETED       0x10
#define OTA_DTC_STATUS_TEST_FAILED_SINCE_CLEAR  0x20
#define OTA_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR 0x40
#define OTA_DTC_STATUS_WARNING_INDICATOR        0x80

/* ============================================================================
 * DTC严重性级别
 * ============================================================================ */
typedef enum {
    OTA_DTC_SEVERITY_LOW = 0,           /* 低 - 不影响行驶安全 */
    OTA_DTC_SEVERITY_MEDIUM,             /* 中 - 需要注意 */
    OTA_DTC_SEVERITY_HIGH,               /* 高 - 影响功能 */
    OTA_DTC_SEVERITY_CRITICAL            /* 严重 - 影响安全 */
} ota_dtc_severity_t;

/* ============================================================================
 * DTC故障类型
 * ============================================================================ */
typedef enum {
    OTA_DTC_FAULT_TYPE_SINGLE = 0,      /* 单次故障 */
    OTA_DTC_FAULT_TYPE_INTERMITTENT,     /* 间歇性故障 */
    OTA_DTC_FAULT_TYPE_PERMANENT,        /* 永久性故障 */
    OTA_DTC_FAULT_TYPE_AGEING            /* 老化故障 */
} ota_dtc_fault_type_t;

/* ============================================================================
 * DEM事件数据结构
 * ============================================================================ */
typedef struct {
    uint32_t dtc_code;                  /* DTC代码 */
    uint8_t status;                     /* DTC状态 */
    ota_dtc_severity_t severity;        /* 严重性 */
    ota_dtc_fault_type_t fault_type;    /* 故障类型 */
    uint32_t occurrence_counter;        /* 发生计数器 */
    uint64_t timestamp;                 /* 时间戳 */
    uint16_t ecu_id;                    /* 相关ECU ID (0 = 全局) */
    uint32_t additional_data;           /* 附加数据 */
} ota_dem_event_t;

/* ============================================================================
 * 快照数据结构 (Freeze Frame)
 * ============================================================================ */
#define OTA_DEM_MAX_FREEZE_FRAME_SIZE   64

typedef struct {
    uint32_t dtc_code;                  /* 关联DTC */
    uint64_t timestamp;                 /* 快照时间戳 */
    uint16_t ecu_id;                    /* ECU ID */
    uint8_t data[OTA_DEM_MAX_FREEZE_FRAME_SIZE]; /* 快照数据 */
    uint8_t data_len;                   /* 数据长度 */
} ota_dem_freeze_frame_t;

/* ============================================================================
 * DEM操作回调类型
 * ============================================================================ */
typedef struct {
    /* 设置DTC状态 */
    int32_t (*set_dtc_status)(uint32_t dtc_code, uint8_t status);
    
    /* 获取DTC状态 */
    int32_t (*get_dtc_status)(uint32_t dtc_code, uint8_t *status);
    
    /* 清除DTC */
    int32_t (*clear_dtc)(uint32_t dtc_code);
    
    /* 记录快照数据 */
    int32_t (*record_freeze_frame)(const ota_dem_freeze_frame_t *freeze_frame);
    
    /* 记录扩展数据 */
    int32_t (*record_extended_data)(uint32_t dtc_code, const uint8_t *data, uint8_t len);
} ota_dem_interface_t;

/* ============================================================================
 * DEM集成配置
 * ============================================================================ */
typedef struct {
    bool enable_dtc_recording;          /* 启用DTC记录 */
    bool enable_freeze_frames;          /* 启用快照数据 */
    uint32_t max_stored_dtcs;           /* 最大存储DTC数量 */
    uint32_t ageing_threshold;          /* 老化阈值 */
    
    /* DEM接口 */
    ota_dem_interface_t *dem_interface;
} ota_dem_config_t;

/* ============================================================================
 * DEM集成上下文
 * ============================================================================ */
typedef struct {
    ota_dem_config_t config;
    
    /* DTC记录缓冲区 */
    ota_dem_event_t dtc_records[16];
    uint8_t num_dtc_records;
    
    /* 统计 */
    uint32_t total_dtcs_reported;
    uint32_t total_dtcs_cleared;
    uint32_t total_freeze_frames;
    
    bool initialized;
} ota_dem_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化OTA DEM集成
 * @param ctx DEM集成上下文
 * @param config 配置参数
 * @return ETH_OK成功
 */
eth_status_t ota_dem_integration_init(ota_dem_context_t *ctx,
                                       const ota_dem_config_t *config);

/**
 * @brief 反初始化OTA DEM集成
 * @param ctx DEM集成上下文
 */
void ota_dem_integration_deinit(ota_dem_context_t *ctx);

/**
 * @brief 报告OTA DTC
 * @param ctx DEM集成上下文
 * @param dtc_code DTC代码
 * @param severity 严重性级别
 * @param ecu_id 相关ECU ID (0 = 全局)
 * @return ETH_OK成功
 */
eth_status_t ota_dem_report_dtc(ota_dem_context_t *ctx,
                                 uint32_t dtc_code,
                                 ota_dtc_severity_t severity,
                                 uint16_t ecu_id);

/**
 * @brief 报告OTA DTC并记录快照
 * @param ctx DEM集成上下文
 * @param dtc_code DTC代码
 * @param severity 严重性级别
 * @param ecu_id 相关ECU ID
 * @param freeze_data 快照数据
 * @param freeze_data_len 快照数据长度
 * @return ETH_OK成功
 */
eth_status_t ota_dem_report_dtc_with_freeze_frame(
    ota_dem_context_t *ctx,
    uint32_t dtc_code,
    ota_dtc_severity_t severity,
    uint16_t ecu_id,
    const uint8_t *freeze_data,
    uint8_t freeze_data_len);

/**
 * @brief 清除OTA DTC
 * @param ctx DEM集成上下文
 * @param dtc_code DTC代码 (0 = 清除所有OTA DTC)
 * @return ETH_OK成功
 */
eth_status_t ota_dem_clear_dtc(ota_dem_context_t *ctx, uint32_t dtc_code);

/**
 * @brief 报告更新失败DTC
 * @param ctx DEM集成上下文
 * @param error_code 错误码
 * @param ecu_id 相关ECU ID
 * @return ETH_OK成功
 */
eth_status_t ota_dem_report_update_failure(ota_dem_context_t *ctx,
                                            uint32_t error_code,
                                            uint16_t ecu_id);

/**
 * @brief 报告安全验证失败DTC
 * @param ctx DEM集成上下文
 * @param security_error 安全验证错误码
 * @param ecu_id 相关ECU ID
 * @return ETH_OK成功
 */
eth_status_t ota_dem_report_security_failure(ota_dem_context_t *ctx,
                                              int32_t security_error,
                                              uint16_t ecu_id);

/**
 * @brief 报告通信失败DTC
 * @param ctx DEM集成上下文
 * @param comm_error 通信错误码
 * @param ecu_id 相关ECU ID
 * @return ETH_OK成功
 */
eth_status_t ota_dem_report_comm_failure(ota_dem_context_t *ctx,
                                          int32_t comm_error,
                                          uint16_t ecu_id);

/**
 * @brief 获取OTA DTC状态
 * @param ctx DEM集成上下文
 * @param dtc_code DTC代码
 * @param status 输出状态
 * @return ETH_OK成功
 */
eth_status_t ota_dem_get_dtc_status(ota_dem_context_t *ctx,
                                     uint32_t dtc_code,
                                     uint8_t *status);

/**
 * @brief 获取OTA DTC描述
 * @param dtc_code DTC代码
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 实际写入长度
 */
int ota_dem_get_dtc_description(uint32_t dtc_code, char *buf, size_t buf_size);

/**
 * @brief 创建快照数据
 * @param ecu_id ECU ID
 * @param status 更新状态
 * @param progress_percent 进度百分比
 * @param freeze_frame 输出快照结构
 * @return ETH_OK成功
 */
eth_status_t ota_dem_create_freeze_frame(uint16_t ecu_id,
                                          uint8_t status,
                                          uint8_t progress_percent,
                                          ota_dem_freeze_frame_t *freeze_frame);

/**
 * @brief 获取DTC回调函数 (用于OTA其他模块)
 * @param ctx DEM集成上下文
 * @return 回调函数指针
 */
void (*ota_dem_get_report_callback(ota_dem_context_t *ctx))(uint32_t, uint8_t);

/**
 * @brief 周期处理函数
 * @param ctx DEM集成上下文
 * @param current_time_ms 当前时间(毫秒)
 */
void ota_dem_integration_cyclic(ota_dem_context_t *ctx,
                                 uint64_t current_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* OTA_DEM_INTEGRATION_H */

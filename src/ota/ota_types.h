/**
 * @file ota_types.h
 * @brief OTA Common Types and Definitions
 * @version 1.0
 * @date 2026-04-26
 *
 * Common types used across OTA modules
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_TYPES_H
#define OTA_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define OTA_CORE_MAJOR_VERSION          1
#define OTA_CORE_MINOR_VERSION          0
#define OTA_CORE_PATCH_VERSION          0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_MAX_PACKAGE_SIZE            (512 * 1024 * 1024)  /* 512MB max package */
#define OTA_MAX_FIRMWARE_SIZE           (256 * 1024 * 1024)  /* 256MB max firmware */
#define OTA_MAX_CAMPAIGN_ID_LEN         64
#define OTA_MAX_PACKAGE_ID_LEN          64
#define OTA_MAX_ECUS_PER_CAMPAIGN       32
#define OTA_MAX_URL_LEN                 512
#define OTA_MAX_VERSION_LEN             32
#define OTA_MAX_RETRY_COUNT             3
#define OTA_DOWNLOAD_CHUNK_SIZE         (64 * 1024)          /* 64KB chunks */
#define OTA_DOWNLOAD_CACHE_SIZE         (16 * 1024 * 1024)   /* 16MB cache */
#define OTA_MAX_CONCURRENT_DOWNLOADS    4

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    OTA_ERR_OK = 0,
    OTA_ERR_INVALID_PARAM = -1,
    OTA_ERR_NOT_INITIALIZED = -2,
    OTA_ERR_BUSY = -3,
    OTA_ERR_TIMEOUT = -4,
    OTA_ERR_NETWORK = -5,
    OTA_ERR_DOWNLOAD_FAILED = -6,
    OTA_ERR_VERIFY_FAILED = -7,
    OTA_ERR_SIGNATURE_INVALID = -8,
    OTA_ERR_HASH_MISMATCH = -9,
    OTA_ERR_VERSION_ROLLBACK = -10,
    OTA_ERR_PARTITION_ERROR = -11,
    OTA_ERR_FLASH_ERROR = -12,
    OTA_ERR_INSTALL_FAILED = -13,
    OTA_ERR_ROLLBACK_FAILED = -14,
    OTA_ERR_PACKAGE_CORRUPTED = -15,
    OTA_ERR_UNSUPPORTED_FORMAT = -16,
    OTA_ERR_DECOMPRESSION_FAILED = -17,
    OTA_ERR_DECRYPTION_FAILED = -18,
    OTA_ERR_CAMPAIGN_INVALID = -19,
    OTA_ERR_PRECONDITION_NOT_MET = -20,
    OTA_ERR_OUT_OF_MEMORY = -21,
    OTA_ERR_CANCELLED = -22,
    OTA_ERR_RESUME_NOT_SUPPORTED = -23,
    OTA_ERR_NO_SPACE = -24
} ota_error_t;

/* ============================================================================
 * OTA状态机状态 (根据规范)
 * ============================================================================ */
typedef enum {
    OTA_STATE_IDLE = 0,                 /* 空闲状态 */
    OTA_STATE_CAMPAIGN_RECEIVED,        /* 收到更新活动 */
    OTA_STATE_DOWNLOADING,              /* 正在下载 */
    OTA_STATE_VERIFYING,                /* 正在验证 */
    OTA_STATE_READY_TO_INSTALL,         /* 准备安装 */
    OTA_STATE_INSTALLING,               /* 正在安装 */
    OTA_STATE_ACTIVATING,               /* 正在激活 */
    OTA_STATE_VERIFYING_BOOT,           /* 验证启动 */
    OTA_STATE_SUCCESS,                  /* 更新成功 */
    OTA_STATE_FAILED,                   /* 更新失败 */
    OTA_STATE_ROLLING_BACK              /* 正在回滚 */
} ota_state_t;

/* ============================================================================
 * 下载状态
 * ============================================================================ */
typedef enum {
    OTA_DL_STATE_IDLE = 0,
    OTA_DL_STATE_CONNECTING,
    OTA_DL_STATE_DOWNLOADING,
    OTA_DL_STATE_PAUSED,
    OTA_DL_STATE_COMPLETED,
    OTA_DL_STATE_FAILED,
    OTA_DL_STATE_CANCELLED
} ota_download_state_t;

/* ============================================================================
 * 压缩类型
 * ============================================================================ */
typedef enum {
    OTA_COMPRESS_NONE = 0,
    OTA_COMPRESS_ZSTD = 1,
    OTA_COMPRESS_LZ4 = 2,
    OTA_COMPRESS_GZIP = 3
} ota_compression_type_t;

/* ============================================================================
 * 加密类型
 * ============================================================================ */
typedef enum {
    OTA_ENCRYPT_NONE = 0,
    OTA_ENCRYPT_AES_128_GCM = 1,
    OTA_ENCRYPT_AES_256_GCM = 2
} ota_encryption_type_t;

/* ============================================================================
 * 哈希算法
 * ============================================================================ */
typedef enum {
    OTA_HASH_SHA_256 = 0,
    OTA_HASH_SHA_384 = 1,
    OTA_HASH_SHA_512 = 2
} ota_hash_type_t;

/* ============================================================================
 * ECU更新信息
 * ============================================================================ */
typedef struct {
    uint16_t ecu_id;                                /* ECU ID */
    char name[32];                                  /* ECU名称 */
    char hardware_version[OTA_MAX_VERSION_LEN];     /* 硬件版本 */
    char firmware_from[OTA_MAX_VERSION_LEN];        /* 当前固件版本 */
    char firmware_to[OTA_MAX_VERSION_LEN];          /* 目标固件版本 */
    char package_file[64];                          /* 包文件名 */
    uint64_t package_size;                          /* 包大小 */
    uint8_t hash[64];                               /* 固件哈希 */
    uint8_t hash_len;                               /* 哈希长度 */
    ota_hash_type_t hash_type;                      /* 哈希类型 */
    bool signature_required;                        /* 是否需要签名 */
    
    /* 安装参数 */
    uint32_t estimated_time_seconds;                /* 估计安装时间 */
    bool reboot_required;                           /* 是否需要重启 */
    bool user_confirmation;                         /* 是否需要用户确认 */
    
    /* 状态 */
    ota_state_t state;                              /* 更新状态 */
    uint8_t progress_percent;                       /* 进度百分比 */
} ota_ecu_update_info_t;

/* ============================================================================
 * 活动(Campaign)信息
 * ============================================================================ */
typedef struct {
    char campaign_id[OTA_MAX_CAMPAIGN_ID_LEN];      /* 活动ID */
    char name[64];                                  /* 活动名称 */
    char description[256];                          /* 活动描述 */
    uint8_t priority;                               /* 优先级 (0-255) */
    uint64_t scheduled_start;                       /* 计划开始时间 */
    uint64_t scheduled_end;                         /* 计划结束时间 */
    
    /* ECU更新列表 */
    ota_ecu_update_info_t ecu_updates[OTA_MAX_ECUS_PER_CAMPAIGN];
    uint8_t num_ecu_updates;                        /* ECU更新数量 */
    
    /* 依赖关系 */
    uint8_t dependencies[OTA_MAX_ECUS_PER_CAMPAIGN]; /* 依赖ECU索引 */
    uint8_t num_dependencies;
    
    /* 元数据 */
    char vin[18];                                   /* 车辆VIN */
    char hw_platform[32];                           /* 硬件平台 */
} ota_campaign_info_t;

/* ============================================================================
 * 下载进度回调
 * ============================================================================ */
typedef void (*ota_download_progress_cb_t)(
    uint32_t bytes_downloaded,
    uint32_t total_bytes,
    uint8_t percentage,
    void *user_data
);

/* ============================================================================
 * 状态变更回调
 * ============================================================================ */
typedef void (*ota_state_change_cb_t)(
    ota_state_t old_state,
    ota_state_t new_state,
    ota_error_t error_code,
    void *user_data
);

/* ============================================================================
 * ECU状态变更回调
 * ============================================================================ */
typedef void (*ota_ecu_status_cb_t)(
    uint16_t ecu_id,
    ota_state_t state,
    uint8_t progress_percent,
    ota_error_t error,
    void *user_data
);

#ifdef __cplusplus
}
#endif

#endif /* OTA_TYPES_H */

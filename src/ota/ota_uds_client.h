/**
 * @file ota_uds_client.h
 * @brief OTA UDS Client - UDS 0x34/0x36/0x37/0x31 Service Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * UNECE R156 compliant OTA update via UDS protocol
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_UDS_CLIENT_H
#define OTA_UDS_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define OTA_UDS_CLIENT_MAJOR_VERSION    1
#define OTA_UDS_CLIENT_MINOR_VERSION    0
#define OTA_UDS_CLIENT_PATCH_VERSION    0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_MAX_FIRMWARE_SIZE           (256 * 1024 * 1024)  /* 256MB max */
#define OTA_MAX_BLOCK_SIZE              4095                 /* ISO-TP max */
#define OTA_MAX_RETRIES                 3
#define OTA_TRANSFER_TIMEOUT_MS         5000
#define OTA_SESSION_TIMEOUT_MS          30000

/* UDS Service IDs */
#define UDS_SID_DIAGNOSTIC_SESSION_CONTROL  0x10
#define UDS_SID_ECU_RESET                   0x11
#define UDS_SID_SECURITY_ACCESS             0x27
#define UDS_SID_ROUTINE_CONTROL             0x31
#define UDS_SID_REQUEST_DOWNLOAD            0x34
#define UDS_SID_REQUEST_UPLOAD              0x35
#define UDS_SID_TRANSFER_DATA               0x36
#define UDS_SID_REQUEST_TRANSFER_EXIT       0x37
#define UDS_SID_TESTER_PRESENT              0x3E

/* UDS Response SID */
#define UDS_SID_POSITIVE_RESPONSE_OFFSET    0x40
#define UDS_SID_NEGATIVE_RESPONSE           0x7F

/* Routine Control Types */
#define ROUTINE_CONTROL_START               0x01
#define ROUTINE_CONTROL_STOP                0x02
#define ROUTINE_CONTROL_REQUEST_RESULTS     0x03

/* Routine IDs for OTA */
#define ROUTINE_ID_ERASE_MEMORY             0xFF00
#define ROUTINE_ID_CHECK_PROGRAMMING_DEPENDENCIES 0xFF01
#define ROUTINE_ID_VERIFY_SIGNATURE         0xFF02
#define ROUTINE_ID_ACTIVATE_FIRMWARE        0xFF03

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    OTA_OK = 0,
    OTA_ERROR_INVALID_PARAM = -1,
    OTA_ERROR_NOT_INITIALIZED = -2,
    OTA_ERROR_BUSY = -3,
    OTA_ERROR_TIMEOUT = -4,
    OTA_ERROR_COMMUNICATION = -5,
    OTA_ERROR_SECURITY_ACCESS = -6,
    OTA_ERROR_TRANSFER = -7,
    OTA_ERROR_VERIFICATION = -8,
    OTA_ERROR_FLASH_PROGRAMMING = -9,
    OTA_ERROR_INVALID_FIRMWARE = -10,
    OTA_ERROR_VERSION_ROLLBACK = -11,
    OTA_ERROR_SIGNATURE_INVALID = -12,
    OTA_ERROR_HASH_MISMATCH = -13,
    OTA_ERROR_MEMORY_ERROR = -14,
    OTA_ERROR_ROUTINE_FAILED = -15,
    OTA_ERROR_SEQUENCE = -16
} ota_uds_error_t;

/* ============================================================================
 * OTA状态机状态
 * ============================================================================ */
typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_SESSION_ESTABLISHED,
    OTA_STATE_SECURITY_UNLOCKED,
    OTA_STATE_DOWNLOAD_REQUESTED,
    OTA_STATE_TRANSFERRING,
    OTA_STATE_TRANSFER_COMPLETED,
    OTA_STATE_ROUTINE_EXECUTING,
    OTA_STATE_VERIFICATION_PENDING,
    OTA_STATE_ACTIVATION_PENDING,
    OTA_STATE_RESET_PENDING,
    OTA_STATE_COMPLETED,
    OTA_STATE_FAILED,
    OTA_STATE_ROLLBACK
} ota_uds_state_t;

/* ============================================================================
 * 数据传输格式
 * ============================================================================ */
typedef enum {
    OTA_FORMAT_RAW = 0x00,
    OTA_FORMAT_ZLIB_COMPRESSED = 0x01,
    OTA_FORMAT_LZ4_COMPRESSED = 0x02,
    OTA_FORMAT_ENCRYPTED_AES128 = 0x10,
    OTA_FORMAT_ENCRYPTED_AES256 = 0x11
} ota_data_format_t;

/* ============================================================================
 * 固件信息
 * ============================================================================ */
typedef struct {
    uint32_t firmware_version;          /* Semantic version: major.minor.patch (8.8.16) */
    uint32_t hardware_version;          /* Target hardware version */
    uint8_t  ecu_id[4];                 /* ECU identifier */
    uint8_t  firmware_hash[32];         /* SHA-256 hash of firmware */
    uint8_t  signature[64];             /* ECDSA P-256 signature */
    uint32_t firmware_size;             /* Total firmware size */
    uint32_t download_address;          /* Target memory address */
    ota_data_format_t data_format;      /* Data format identifier */
} ota_firmware_info_t;

/* ============================================================================
 * UDS 0x34 Request Download参数
 * ============================================================================ */
typedef struct {
    uint8_t  data_format_id;            /* Data format identifier */
    uint8_t  addr_len_format;           /* Address and length format identifier */
    uint32_t memory_address;            /* Start address for download */
    uint32_t memory_size;               /* Total size to download */
} ota_download_request_t;

/* ============================================================================
 * UDS 0x36 Transfer Data参数
 * ============================================================================ */
typedef struct {
    uint8_t  block_sequence;            /* Block sequence counter */
    uint8_t  *data;                     /* Data buffer */
    uint16_t data_length;               /* Data length */
} ota_transfer_data_t;

/* ============================================================================
 * 传输进度回调
 * ============================================================================ */
typedef void (*ota_progress_callback_t)(
    uint32_t bytes_transferred,
    uint32_t total_bytes,
    uint8_t  percentage,
    void     *user_data
);

/* ============================================================================
 * 状态变更回调
 * ============================================================================ */
typedef void (*ota_state_callback_t)(
    ota_uds_state_t old_state,
    ota_uds_state_t new_state,
    ota_uds_error_t error_code,
    void *user_data
);

/* ============================================================================
 * UDS传输接口
 * ============================================================================ */
typedef struct {
    /* 发送UDS请求 */
    int32_t (*send_request)(
        uint8_t  *request_data,
        uint16_t request_len,
        uint8_t  *response_data,
        uint16_t *response_len,
        uint32_t timeout_ms
    );
    
    /* 进入扩展会话 */
    int32_t (*enter_extended_session)(uint32_t timeout_ms);
    
    /* 进入编程会话 */
    int32_t (*enter_programming_session)(uint32_t timeout_ms);
    
    /* 解锁安全访问 */
    int32_t (*unlock_security)(uint8_t level, uint32_t timeout_ms);
    
    /* 复位ECU */
    int32_t (*ecu_reset)(uint8_t reset_type, uint32_t timeout_ms);
} ota_uds_transport_if_t;

/* ============================================================================
 * OTA UDS客户端配置
 * ============================================================================ */
typedef struct {
    uint16_t target_ecu_id;             /* Target ECU logical address */
    uint32_t transfer_block_size;       /* Max transfer block size */
    uint32_t transfer_timeout_ms;       /* Transfer timeout */
    uint32_t session_timeout_ms;        /* Session timeout */
    uint8_t  security_level;            /* Required security level */
    bool     use_compression;           /* Enable compression */
    bool     use_encryption;            /* Enable encryption */
    
    /* Callbacks */
    ota_progress_callback_t on_progress;
    ota_state_callback_t    on_state_change;
    void *user_data;
    
    /* Transport interface */
    ota_uds_transport_if_t *transport;
} ota_uds_config_t;

/* ============================================================================
 * OTA UDS客户端上下文
 * ============================================================================ */
typedef struct {
    ota_uds_config_t config;
    ota_uds_state_t  state;
    ota_uds_error_t  last_error;
    
    /* Transfer state */
    uint32_t bytes_transferred;
    uint32_t total_bytes;
    uint8_t  block_sequence;
    uint32_t max_block_size;
    
    /* Firmware info */
    ota_firmware_info_t firmware_info;
    
    /* Session state */
    bool session_active;
    bool security_unlocked;
    uint32_t session_timer;
    
    /* Statistics */
    uint32_t retry_count;
    uint32_t total_time_ms;
    uint32_t transfer_speed_bps;
    
    /* DEM error reporting callback */
    void (*dem_report_dtc)(uint32_t dtc_code, uint8_t status);
    
    /* Initialized flag */
    bool initialized;
} ota_uds_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化OTA UDS客户端
 * @param ctx OTA UDS上下文
 * @param config 配置参数
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_init(ota_uds_context_t *ctx, const ota_uds_config_t *config);

/**
 * @brief 反初始化OTA UDS客户端
 * @param ctx OTA UDS上下文
 */
void ota_uds_deinit(ota_uds_context_t *ctx);

/**
 * @brief 启动固件下载会话 (UDS 0x10, 0x27)
 * @param ctx OTA UDS上下文
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_start_session(ota_uds_context_t *ctx);

/**
 * @brief 请求下载 (UDS 0x34)
 * @param ctx OTA UDS上下文
 * @param firmware_info 固件信息
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_request_download(
    ota_uds_context_t *ctx,
    const ota_firmware_info_t *firmware_info
);

/**
 * @brief 传输数据块 (UDS 0x36)
 * @param ctx OTA UDS上下文
 * @param block_data 数据块
 * @param block_len 数据长度
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_transfer_data(
    ota_uds_context_t *ctx,
    const uint8_t *block_data,
    uint16_t block_len
);

/**
 * @brief 完整固件传输
 * @param ctx OTA UDS上下文
 * @param firmware_data 固件数据
 * @param firmware_size 固件大小
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_transfer_firmware(
    ota_uds_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size
);

/**
 * @brief 请求退出传输 (UDS 0x37)
 * @param ctx OTA UDS上下文
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_request_transfer_exit(ota_uds_context_t *ctx);

/**
 * @brief 执行例程控制 (UDS 0x31)
 * @param ctx OTA UDS上下文
 * @param routine_type 例程控制类型 (START/STOP/RESULTS)
 * @param routine_id 例程ID
 * @param params 例程参数 (可为NULL)
 * @param params_len 参数长度
 * @param result 结果输出 (可为NULL)
 * @param result_len 结果长度 (输入/输出)
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_routine_control(
    ota_uds_context_t *ctx,
    uint8_t routine_type,
    uint16_t routine_id,
    const uint8_t *params,
    uint8_t params_len,
    uint8_t *result,
    uint8_t *result_len
);

/**
 * @brief 验证固件签名 (UDS 0x31 Routine)
 * @param ctx OTA UDS上下文
 * @param signature 签名数据
 * @param hash 固件哈希
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_verify_signature(
    ota_uds_context_t *ctx,
    const uint8_t *signature,
    const uint8_t *hash
);

/**
 * @brief 激活固件 (UDS 0x31 Routine)
 * @param ctx OTA UDS上下文
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_activate_firmware(ota_uds_context_t *ctx);

/**
 * @brief 执行ECU复位完成更新
 * @param ctx OTA UDS上下文
 * @param reset_type 复位类型
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_ecu_reset(
    ota_uds_context_t *ctx,
    uint8_t reset_type
);

/**
 * @brief 完成OTA更新流程
 * @param ctx OTA UDS上下文
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_complete_update(ota_uds_context_t *ctx);

/**
 * @brief 取消OTA更新
 * @param ctx OTA UDS上下文
 */
void ota_uds_cancel_update(ota_uds_context_t *ctx);

/**
 * @brief 获取当前状态
 * @param ctx OTA UDS上下文
 * @return 当前OTA状态
 */
ota_uds_state_t ota_uds_get_state(const ota_uds_context_t *ctx);

/**
 * @brief 获取最后错误码
 * @param ctx OTA UDS上下文
 * @return 最后错误码
 */
ota_uds_error_t ota_uds_get_last_error(const ota_uds_context_t *ctx);

/**
 * @brief 获取传输进度
 * @param ctx OTA UDS上下文
 * @param bytes_transferred 已传输字节
 * @param total_bytes 总字节
 * @param percentage 百分比
 * @return OTA_OK成功
 */
ota_uds_error_t ota_uds_get_progress(
    const ota_uds_context_t *ctx,
    uint32_t *bytes_transferred,
    uint32_t *total_bytes,
    uint8_t *percentage
);

/**
 * @brief 周期处理函数 (需在主循环中调用)
 * @param ctx OTA UDS上下文
 */
void ota_uds_cyclic_process(ota_uds_context_t *ctx);

/**
 * @brief 设置DEM错误报告回调
 * @param ctx OTA UDS上下文
 * @param report_func DTC报告函数
 */
void ota_uds_set_dem_callback(
    ota_uds_context_t *ctx,
    void (*report_func)(uint32_t dtc_code, uint8_t status)
);

#ifdef __cplusplus
}
#endif

#endif /* OTA_UDS_CLIENT_H */

/**
 * @file ota_downloader.h
 * @brief OTA Downloader - HTTP Downloader with Resume Support
 * @version 1.0
 * @date 2026-04-26
 *
 * HTTP downloader with Range request support for OTA updates
 * Supports resumable downloads and download cache management
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_DOWNLOADER_H
#define OTA_DOWNLOADER_H

#include "ota_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_DL_MAX_URL_LEN              512
#define OTA_DL_MAX_HEADER_LEN           2048
#define OTA_DL_MAX_REDIRECTS            5
#define OTA_DL_CONNECT_TIMEOUT_MS       10000
#define OTA_DL_READ_TIMEOUT_MS          30000
#define OTA_DL_MAX_CHUNK_SIZE           (128 * 1024)  /* 128KB chunks */
#define OTA_DL_MIN_CHUNK_SIZE           1024

/* HTTP Range 请求配置 */
#define OTA_DL_RANGE_REQUEST_ENABLED    1
#define OTA_DL_RANGE_SIZE               (64 * 1024)   /* 64KB per range request */

/* ============================================================================
 * 下载方式
 * ============================================================================ */
typedef enum {
    OTA_DL_MODE_FULL = 0,               /* 完整下载 */
    OTA_DL_MODE_RESUME,                 /* 断点续传 */
    OTA_DL_MODE_VERIFY_ONLY             /* 只验证不下载 */
} ota_download_mode_t;

/* ============================================================================
 * HTTP请求头
 * ============================================================================ */
typedef struct {
    char key[64];
    char value[256];
} ota_http_header_t;

/* ============================================================================
 * 下载请求配置
 * ============================================================================ */
typedef struct {
    char url[OTA_DL_MAX_URL_LEN];       /* 下载URL */
    char package_id[OTA_MAX_PACKAGE_ID_LEN]; /* 包ID (用于缓存) */
    uint64_t offset;                    /* 开始偏移量 (断点续传) */
    uint64_t expected_size;             /* 预期文件大小 */
    uint8_t expected_hash[64];          /* 预期文件哈希 */
    ota_hash_type_t hash_type;          /* 哈希类型 */
    ota_download_mode_t mode;           /* 下载模式 */
    
    /* 认证信息 (可选) */
    char username[64];
    char password[64];
    char bearer_token[512];
    
    /* 自定义HTTP头 */
    ota_http_header_t headers[8];
    uint8_t num_headers;
} ota_download_request_t;

/* ============================================================================
 * 下载统计信息
 * ============================================================================ */
typedef struct {
    uint64_t bytes_downloaded;          /* 已下载字节数 */
    uint64_t total_bytes;               /* 总字节数 */
    uint32_t speed_bps;                 /* 当前速度 (bytes/sec) */
    uint32_t average_speed_bps;         /* 平均速度 */
    uint32_t elapsed_time_ms;           /* 已用时间 */
    uint32_t estimated_time_ms;         /* 预估剩余时间 */
    uint8_t progress_percent;           /* 进度百分比 */
    uint32_t retry_count;               /* 重试次数 */
} ota_download_stats_t;

/* ============================================================================
 * 下载文件信息
 * ============================================================================ */
typedef struct {
    char url[OTA_DL_MAX_URL_LEN];
    char package_id[OTA_MAX_PACKAGE_ID_LEN];
    uint64_t file_size;
    uint64_t downloaded_size;
    uint64_t offset;                    /* 当前下载偏移 */
    uint8_t hash[64];
    ota_hash_type_t hash_type;
    ota_download_state_t state;
    bool supports_range;                /* 服务器是否支持Range请求 */
    time_t last_modified;               /* 文件修改时间 */
} ota_download_file_info_t;

/* ============================================================================
 * HTTP传输接口 (需要应用层实现)
 * ============================================================================ */
typedef struct {
    /**
     * @brief 初始化HTTP连接
     * @param url 服务器URL
     * @return 0成功, 非0失败
     */
    int32_t (*connect)(const char *url);
    
    /**
     * @brief 断开HTTP连接
     * @return 0成功
     */
    int32_t (*disconnect)(void);
    
    /**
     * @brief 发送HTTP请求
     * @param method HTTP方法 (GET, HEAD, etc.)
     * @param path 请求路径
     * @param headers 请求头
     * @param num_headers 头数量
     * @param body 请求体 (可为NULL)
     * @param body_len 请求体长度
     * @param response 响应缓冲区
     * @param response_len 响应长度 (输入/输出)
     * @param timeout_ms 超时时间
     * @return 0成功, 非0HTTP状态码或错误
     */
    int32_t (*send_request)(
        const char *method,
        const char *path,
        const ota_http_header_t *headers,
        uint8_t num_headers,
        const uint8_t *body,
        uint32_t body_len,
        uint8_t *response,
        uint32_t *response_len,
        uint32_t timeout_ms
    );
    
    /**
     * @brief 接收数据 (流式下载)
     * @param buffer 缓冲区
     * @param buffer_size 缓冲区大小
     * @param received_len 实际接收长度
     * @param timeout_ms 超时时间
     * @return 0成功
     */
    int32_t (*receive_data)(
        uint8_t *buffer,
        uint32_t buffer_size,
        uint32_t *received_len,
        uint32_t timeout_ms
    );
    
    /**
     * @brief 检查是否有可用数据
     * @return 可用字节数
     */
    int32_t (*data_available)(void);
} ota_http_transport_t;

/* ============================================================================
 * 数据写入回调 (用于保存下载数据)
 * ============================================================================ */
typedef int32_t (*ota_data_write_cb_t)(
    uint64_t offset,                    /* 写入偏移量 */
    const uint8_t *data,                /* 数据缓冲区 */
    uint32_t data_len,                  /* 数据长度 */
    void *user_data                     /* 用户数据 */
);

/* ============================================================================
 * 下载器配置
 * ============================================================================ */
typedef struct {
    uint32_t connect_timeout_ms;
    uint32_t read_timeout_ms;
    uint32_t max_retries;
    uint32_t chunk_size;                /* 每次下载块大小 */
    bool enable_range_requests;         /* 启用Range请求 */
    bool verify_hash;                   /* 下载完成后验证哈希 */
    
    /* 回调函数 */
    ota_data_write_cb_t on_data_write;  /* 数据写入回调 */
    ota_download_progress_cb_t on_progress; /* 进度回调 */
    void *user_data;
} ota_downloader_config_t;

/* ============================================================================
 * 下载器上下文
 * ============================================================================ */
typedef struct {
    ota_downloader_config_t config;
    ota_http_transport_t *transport;
    
    /* 下载状态 */
    ota_download_state_t state;
    ota_download_request_t current_request;
    ota_download_stats_t stats;
    ota_download_file_info_t file_info;
    
    /* HTTP状态 */
    int32_t http_status;
    uint32_t redirect_count;
    bool server_supports_range;
    uint64_t resume_offset;             /* 断点续传偏移量 */
    
    /* 计算哈希 */
    uint8_t hash_context[256];          /* 哈希上下文 (对应CSM) */
    bool hash_initialized;
    
    /* 初始化标志 */
    bool initialized;
    bool download_active;
    bool cancel_requested;
} ota_downloader_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化下载器
 * @param ctx 下载器上下文
 * @param config 配置参数
 * @param transport HTTP传输接口 (可为NULL, 使用内置实现)
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_init(
    ota_downloader_context_t *ctx,
    const ota_downloader_config_t *config,
    ota_http_transport_t *transport
);

/**
 * @brief 反初始化下载器
 * @param ctx 下载器上下文
 */
void ota_downloader_deinit(ota_downloader_context_t *ctx);

/**
 * @brief 开始下载
 * @param ctx 下载器上下文
 * @param request 下载请求配置
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_start(
    ota_downloader_context_t *ctx,
    const ota_download_request_t *request
);

/**
 * @brief 暂停下载
 * @param ctx 下载器上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_pause(ota_downloader_context_t *ctx);

/**
 * @brief 恢复下载 (断点续传)
 * @param ctx 下载器上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_resume(ota_downloader_context_t *ctx);

/**
 * @brief 取消下载
 * @param ctx 下载器上下文
 */
void ota_downloader_cancel(ota_downloader_context_t *ctx);

/**
 * @brief 检查服务器是否支持断点续传
 * @param ctx 下载器上下文
 * @param url 文件URL
 * @param supports_range 输出是否支持Range
 * @param file_size 输出文件大小 (0表示未知)
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_check_resume_support(
    ota_downloader_context_t *ctx,
    const char *url,
    bool *supports_range,
    uint64_t *file_size
);

/**
 * @brief 获取下载状态
 * @param ctx 下载器上下文
 * @return 当前下载状态
 */
ota_download_state_t ota_downloader_get_state(const ota_downloader_context_t *ctx);

/**
 * @brief 获取下载统计信息
 * @param ctx 下载器上下文
 * @param stats 输出统计信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_get_stats(
    const ota_downloader_context_t *ctx,
    ota_download_stats_t *stats
);

/**
 * @brief 获取文件信息
 * @param ctx 下载器上下文
 * @param info 输出文件信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_get_file_info(
    const ota_downloader_context_t *ctx,
    ota_download_file_info_t *info
);

/**
 * @brief 获取断点续传偏移量
 * @param ctx 下载器上下文
 * @return 当前偏移量
 */
uint64_t ota_downloader_get_resume_offset(const ota_downloader_context_t *ctx);

/**
 * @brief 设置下载偏移量 (用于断点续传)
 * @param ctx 下载器上下文
 * @param offset 开始偏移量
 */
void ota_downloader_set_offset(ota_downloader_context_t *ctx, uint64_t offset);

/**
 * @brief 周期处理函数 (需在主循环中调用)
 * @param ctx 下载器上下文
 * @return 是否还在下载中
 */
bool ota_downloader_process(ota_downloader_context_t *ctx);

/**
 * @brief 同步下载 (阻塞直到完成)
 * @param ctx 下载器上下文
 * @param request 下载请求配置
 * @param timeout_ms 超时时间
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_download_sync(
    ota_downloader_context_t *ctx,
    const ota_download_request_t *request,
    uint32_t timeout_ms
);

/**
 * @brief 验证下载文件哈希
 * @param ctx 下载器上下文
 * @param expected_hash 预期哈希
 * @param hash_type 哈希类型
 * @param valid 输出验证结果
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_verify_hash(
    ota_downloader_context_t *ctx,
    const uint8_t *expected_hash,
    ota_hash_type_t hash_type,
    bool *valid
);

/**
 * @brief 创建HTTP Range请求头
 * @param offset 开始偏移量
 * @param length 长度 (0表示到文件末尾)
 * @param header 输出头字符串
 * @param header_len 头长度
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_downloader_create_range_header(
    uint64_t offset,
    uint64_t length,
    char *header,
    uint32_t header_len
);

#ifdef __cplusplus
}
#endif

#endif /* OTA_DOWNLOADER_H */

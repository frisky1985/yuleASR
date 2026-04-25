/**
 * @file ota_package.h
 * @brief OTA Package Parser - .vpkg and .epkg Format Support
 * @version 1.0
 * @date 2026-04-26
 *
 * Package parser for Vehicle Package (.vpkg) and ECU Package (.epkg)
 * Supports compression (Zstd/LZ4) and signature verification
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_PACKAGE_H
#define OTA_PACKAGE_H

#include "ota_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_PKG_MAGIC_VPCK              0x5645434B  /* "VECK" */
#define OTA_PKG_MAGIC_EPCK              0x4550434B  /* "EPCK" */
#define OTA_PKG_MAX_MANIFEST_SIZE       (128 * 1024)   /* 128KB manifest */
#define OTA_PKG_MAX_SIGNATURE_SIZE      4096
#define OTA_PKG_MAX_CERT_CHAIN_SIZE     8192
#define OTA_PKG_MAX_ECU_PACKAGES        64
#define OTA_PKG_MAX_PAYLOAD_SIZE        (512 * 1024 * 1024)  /* 512MB */

/* ============================================================================
 * 包版本
 * ============================================================================ */
#define OTA_PKG_VERSION_1               1
#define OTA_PKG_CURRENT_VERSION         OTA_PKG_VERSION_1

/* ============================================================================
 * 签名类型
 * ============================================================================ */
typedef enum {
    OTA_SIG_RAW_ECDSA_P256 = 0,         /* 原始ECDSA P-256签名 */
    OTA_SIG_RAW_ECDSA_P384,             /* 原始ECDSA P-384签名 */
    OTA_SIG_CMS_PKCS7,                  /* CMS/PKCS#7签名 */
    OTA_SIG_COSE                        /* COSE签名 (RFC 8152) */
} ota_signature_type_t;

/* ============================================================================
 * Vehicle Package (.vpkg) 头部
 * ============================================================================ */
typedef struct __attribute__((packed)) {
    uint32_t magic;                     /* 魔法数: "VECK" */
    uint8_t version;                    /* 包版本 */
    uint8_t reserved[3];                /* 保留 */
    
    /* 包标识 */
    uint8_t package_id[16];             /* UUID */
    uint64_t timestamp;                 /* 构建时间戳 */
    char vin[17];                       /* 车辆VIN */
    char hw_version[16];                /* 目标硬件版本 */
    
    /* 偏移量 */
    uint32_t header_size;               /* 头部大小 */
    uint32_t manifest_offset;           /* Manifest偏移量 */
    uint32_t manifest_size;             /* Manifest大小 */
    uint32_t payload_offset;            /* 载荷偏移量 */
    uint64_t payload_size;              /* 载荷大小 */
    uint32_t signature_offset;          /* 签名偏移量 */
    uint32_t signature_size;            /* 签名大小 */
    
    /* 压缩和加密 */
    ota_compression_type_t compression; /* 压缩类型 */
    ota_encryption_type_t encryption;   /* 加密类型 */
    uint8_t reserved2[6];               /* 保留 */
    
    /* 验证 */
    uint8_t header_hash[32];            /* 头部SHA-256哈希 */
} ota_vpkg_header_t;

/* ============================================================================
 * ECU Package (.epkg) 头部
 * ============================================================================ */
typedef struct __attribute__((packed)) {
    uint32_t magic;                     /* 魔法数: "EPCK" */
    uint8_t version;                    /* 包版本 */
    uint8_t reserved[3];                /* 保留 */
    
    /* ECU标识 */
    uint16_t ecu_id;                    /* ECU ID */
    char hw_version[16];                /* 目标硬件版本 */
    char fw_version_from[16];           /* 当前固件版本 */
    char fw_version_to[16];             /* 目标固件版本 */
    
    /* 偏移量 */
    uint32_t header_size;               /* 头部大小 */
    uint32_t firmware_offset;           /* 固件偏移量 */
    uint32_t firmware_size;             /* 固件大小 (压缩前) */
    uint32_t firmware_compressed_size;  /* 固件压缩后大小 */
    uint32_t delta_offset;              /* 增量补丁偏移量 (可选) */
    uint32_t delta_size;                /* 增量补丁大小 */
    uint32_t layout_offset;             /* 内存布局偏移量 */
    uint32_t layout_size;               /* 内存布局大小 */
    uint32_t hash_offset;               /* 哈希偏移量 */
    uint32_t hash_size;                 /* 哈希大小 */
    
    /* 压缩和加密 */
    ota_compression_type_t compression; /* 压缩类型 */
    ota_encryption_type_t encryption;   /* 加密类型 */
    
    /* 下载地址 */
    uint32_t download_address;          /* 目标Flash地址 */
    
    /* 验证 */
    uint8_t firmware_hash[32];          /* 固件SHA-256哈希 */
} ota_epkg_header_t;

/* ============================================================================
 * Manifest结构 (JSON解析后)
 * ============================================================================ */
typedef struct {
    char manifest_version[16];          /* Manifest版本 */
    
    /* 包信息 */
    char package_id[64];
    uint64_t created_at;
    char created_by[64];
    
    /* 目标车辆 */
    char vin[18];
    char hw_platform[32];
    char compatibility[8][16];          /* 兼容硬件版本列表 */
    uint8_t num_compatible;
    
    /* 活动信息 */
    char campaign_id[OTA_MAX_CAMPAIGN_ID_LEN];
    char campaign_name[64];
    char description[256];
    uint8_t priority;
    uint64_t scheduled_start;
    uint64_t scheduled_end;
    
    /* 推广策略 */
    char rollout_strategy[16];          /* "wave", "canary", "all" */
    uint8_t batch_size_percent;
    uint8_t success_threshold_percent;
} ota_manifest_header_t;

/* ============================================================================
 * 签名信息
 * ============================================================================ */
typedef struct {
    ota_signature_type_t type;          /* 签名类型 */
    uint8_t *signature_data;            /* 签名数据 */
    uint32_t signature_len;             /* 签名长度 */
    uint8_t *cert_chain;                /* 证书链 */
    uint32_t cert_chain_len;            /* 证书链长度 */
    
    /* 签名覆盖范围 */
    uint64_t signed_data_start;         /* 开始偏移 */
    uint64_t signed_data_len;           /* 签名数据长度 */
} ota_signature_info_t;

/* ============================================================================
 * 解析后的包信息
 * ============================================================================ */
typedef struct {
    /* 包类型 */
    bool is_vpkg;                       /* true=.vpkg, false=.epkg */
    
    /* 头部 */
    union {
        ota_vpkg_header_t vpkg;
        ota_epkg_header_t epkg;
    } header;
    
    /* Manifest (仅.vpkg) */
    ota_manifest_header_t manifest;
    char *manifest_json;                /* 原始JSON字符串 */
    uint32_t manifest_json_len;
    
    /* ECU更新列表 (仅.vpkg) */
    ota_ecu_update_info_t *ecu_updates;
    uint8_t num_ecu_updates;
    
    /* 签名 */
    ota_signature_info_t signature;
    
    /* 载荷/固件 */
    uint64_t payload_offset;            /* 在文件中的偏移量 */
    uint64_t payload_size;              /* 压缩/加密后的大小 */
    uint64_t payload_uncompressed_size; /* 解压后的大小 */
    
    /* 压缩缓冲区 (用于解压) */
    uint8_t *compress_buffer;
    uint32_t compress_buffer_size;
} ota_package_info_t;

/* ============================================================================
 * 解压缓冲区
 * ============================================================================ */
typedef struct {
    uint8_t *input;                     /* 输入缓冲区 */
    uint32_t input_size;                /* 输入缓冲区大小 */
    uint32_t input_len;                 /* 实际输入数据长度 */
    
    uint8_t *output;                    /* 输出缓冲区 */
    uint32_t output_size;               /* 输出缓冲区大小 */
    uint32_t output_len;                /* 实际输出数据长度 */
    
    /* 压缩器上下文 */
    void *compress_ctx;                 /* Zstd/LZ4上下文 */
    ota_compression_type_t type;
    bool initialized;
} ota_compress_context_t;

/* ============================================================================
 * 包解析器配置
 * ============================================================================ */
typedef struct {
    uint32_t max_manifest_size;         /* 最大Manifest大小 */
    uint32_t max_payload_size;          /* 最大载荷大小 */
    bool verify_signature;              /* 是否验证签名 */
    bool verify_hash;                   /* 是否验证哈希 */
    bool decompress;                    /* 是否自动解压 */
    
    /* 内存分配函数 */
    void* (*alloc)(uint32_t size);
    void (*free)(void *ptr);
} ota_parser_config_t;

/* ============================================================================
 * 包解析器上下文
 * ============================================================================ */
typedef struct {
    ota_parser_config_t config;
    ota_package_info_t *current_package;
    
    /* 解压上下文 */
    ota_compress_context_t compress_ctx;
    
    /* 解析状态 */
    bool header_parsed;
    bool manifest_parsed;
    bool signature_parsed;
    
    /* 验证状态 */
    bool signature_valid;
    bool hash_valid;
    
    /* 初始化标志 */
    bool initialized;
} ota_parser_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化包解析器
 * @param ctx 解析器上下文
 * @param config 配置参数
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_parser_init(
    ota_parser_context_t *ctx,
    const ota_parser_config_t *config
);

/**
 * @brief 反初始化包解析器
 * @param ctx 解析器上下文
 */
void ota_package_parser_deinit(ota_parser_context_t *ctx);

/**
 * @brief 解析包头部
 * @param ctx 解析器上下文
 * @param data 包数据 (至少包含头部)
 * @param data_len 数据长度
 * @param info 输出包信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_parse_header(
    ota_parser_context_t *ctx,
    const uint8_t *data,
    uint32_t data_len,
    ota_package_info_t *info
);

/**
 * @brief 解析Manifest (JSON)
 * @param ctx 解析器上下文
 * @param json_data JSON数据
 * @param json_len JSON长度
 * @param manifest 输出Manifest结构
 * @param ecu_updates 输出ECU更新数组 (需预分配)
 * @param max_ecus 最大ECU数量
 * @param num_ecus 实际ECU数量输出
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_parse_manifest(
    ota_parser_context_t *ctx,
    const char *json_data,
    uint32_t json_len,
    ota_manifest_header_t *manifest,
    ota_ecu_update_info_t *ecu_updates,
    uint8_t max_ecus,
    uint8_t *num_ecus
);

/**
 * @brief 解析签名
 * @param ctx 解析器上下文
 * @param data 签名数据
 * @param data_len 数据长度
 * @param sig_type 签名类型
 * @param info 输出签名信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_parse_signature(
    ota_parser_context_t *ctx,
    const uint8_t *data,
    uint32_t data_len,
    ota_signature_type_t sig_type,
    ota_signature_info_t *info
);

/**
 * @brief 验证包签名
 * @param ctx 解析器上下文
 * @param package_data 完整包数据
 * @param package_len 包长度
 * @param info 包信息 (已解析)
 * @param key_id 验证密钥ID (CSM引用)
 * @param valid 输出验证结果
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_verify_signature(
    ota_parser_context_t *ctx,
    const uint8_t *package_data,
    uint64_t package_len,
    const ota_package_info_t *info,
    uint8_t key_id,
    bool *valid
);

/**
 * @brief 验证包哈希
 * @param ctx 解析器上下文
 * @param data 数据
 * @param data_len 数据长度
 * @param expected_hash 预期哈希
 * @param hash_type 哈希类型
 * @param valid 输出验证结果
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_verify_hash(
    ota_parser_context_t *ctx,
    const uint8_t *data,
    uint64_t data_len,
    const uint8_t *expected_hash,
    ota_hash_type_t hash_type,
    bool *valid
);

/**
 * @brief 初始化解压器
 * @param ctx 压缩上下文
 * @param type 压缩类型
 * @param output_buffer 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_decompress_init(
    ota_compress_context_t *ctx,
    ota_compression_type_t type,
    uint8_t *output_buffer,
    uint32_t output_size
);

/**
 * @brief 解压数据块
 * @param ctx 压缩上下文
 * @param input 输入数据
 * @param input_len 输入长度
 * @param output 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @param output_len 实际输出长度
 * @param finished 是否解压完成
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_decompress_block(
    ota_compress_context_t *ctx,
    const uint8_t *input,
    uint32_t input_len,
    uint8_t *output,
    uint32_t output_size,
    uint32_t *output_len,
    bool *finished
);

/**
 * @brief 解压完整数据
 * @param ctx 压缩上下文
 * @param compressed_data 压缩数据
 * @param compressed_len 压缩数据长度
 * @param decompressed_data 解压后数据输出
 * @param decompressed_size 解压后缓冲区大小
 * @param decompressed_len 实际解压后长度
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_decompress_full(
    ota_compress_context_t *ctx,
    const uint8_t *compressed_data,
    uint32_t compressed_len,
    uint8_t *decompressed_data,
    uint32_t decompressed_size,
    uint32_t *decompressed_len
);

/**
 * @brief 释放解压器资源
 * @param ctx 压缩上下文
 */
void ota_decompress_deinit(ota_compress_context_t *ctx);

/**
 * @brief 创建vehicle package头部
 * @param header 头部结构
 * @param package_id UUID
 * @param vin 车辆VIN
 * @param hw_version 硬件版本
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_create_vpkg_header(
    ota_vpkg_header_t *header,
    const uint8_t package_id[16],
    const char *vin,
    const char *hw_version
);

/**
 * @brief 创建ECU package头部
 * @param header 头部结构
 * @param ecu_id ECU ID
 * @param hw_version 硬件版本
 * @param fw_from 当前固件版本
 * @param fw_to 目标固件版本
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_package_create_epkg_header(
    ota_epkg_header_t *header,
    uint16_t ecu_id,
    const char *hw_version,
    const char *fw_from,
    const char *fw_to
);

/**
 * @brief 获取压缩类型名称
 * @param type 压缩类型
 * @return 类型名称字符串
 */
const char* ota_compression_type_name(ota_compression_type_t type);

/**
 * @brief 获取魔法数字节表示
 * @param magic 魔法数
 * @return 魔法数字符串
 */
const char* ota_package_magic_to_string(uint32_t magic);

#ifdef __cplusplus
}
#endif

#endif /* OTA_PACKAGE_H */

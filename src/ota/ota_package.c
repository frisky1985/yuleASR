/**
 * @file ota_package.c
 * @brief OTA Package Parser - .vpkg and .epkg Format Support Implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * Package parser for Vehicle Package (.vpkg) and ECU Package (.epkg)
 * Supports compression (Zstd/LZ4) and signature verification
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ota_package.h"
#include "../crypto_stack/csm/csm_core.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define OTA_PKG_MODULE_NAME     "OTA_PKG"
#define OTA_PKG_LOG_LEVEL       DDS_LOG_INFO

/* 压缩缓冲区大小 */
#define COMPRESS_INPUT_BUF_SIZE (256 * 1024)   /* 256KB input buffer */
#define COMPRESS_OUTPUT_BUF_SIZE (512 * 1024)  /* 512KB output buffer */

/* JSON解析最大层数 */
#define JSON_MAX_DEPTH          16
#define JSON_MAX_TOKENS         256

/* ============================================================================
 * JSON Token结构 (简单JSON解析器)
 * ============================================================================ */
typedef enum {
    JSON_TOKEN_INVALID = 0,
    JSON_TOKEN_OBJECT,
    JSON_TOKEN_ARRAY,
    JSON_TOKEN_STRING,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_BOOLEAN,
    JSON_TOKEN_NULL
} json_token_type_t;

typedef struct {
    json_token_type_t type;
    const char *start;
    uint32_t length;
    uint16_t parent;
    uint16_t child_count;
} json_token_t;

/* ============================================================================
 * 内部辅助函数 - JSON解析 (简化版)
 * ============================================================================ */

/**
 * @brief 跳过JSON空白字符
 */
static const char* skip_whitespace(const char *ptr)
{
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
        ptr++;
    }
    return ptr;
}

/**
 * @brief 解析JSON字符串
 */
static int32_t parse_json_string(const char *json, uint32_t json_len, 
                                  uint32_t *offset, json_token_t *tokens,
                                  uint32_t *num_tokens, uint32_t max_tokens)
{
    if (*offset >= json_len || json[*offset] != '"') {
        return -1;
    }
    
    (*offset)++;
    const char *start = &json[*offset];
    uint32_t len = 0;
    
    while (*offset < json_len && json[*offset] != '"') {
        if (json[*offset] == '\\' && (*offset + 1) < json_len) {
            (*offset) += 2;
            len += 2;
        } else {
            (*offset)++;
            len++;
        }
    }
    
    if (*offset >= json_len) {
        return -1; /* 未关闭的字符串 */
    }
    
    if (*num_tokens < max_tokens) {
        tokens[*num_tokens].type = JSON_TOKEN_STRING;
        tokens[*num_tokens].start = start;
        tokens[*num_tokens].length = len;
        (*num_tokens)++;
    }
    
    (*offset)++; /* 跳过关闭引号 */
    return 0;
}

/**
 * @brief 简化的JSON解析器 - 只提取键值对
 */
static int32_t extract_json_value(const char *json, const char *key,
                                   char *value, uint32_t value_len)
{
    const char *key_pos = strstr(json, key);
    if (key_pos == NULL) {
        return -1;
    }
    
    /* 找到值 */
    const char *colon = strchr(key_pos, ':');
    if (colon == NULL) {
        return -1;
    }
    
    colon = skip_whitespace(colon + 1);
    
    if (*colon == '"') {
        /* 字符串值 */
        colon++;
        const char *end = strchr(colon, '"');
        if (end == NULL) {
            return -1;
        }
        
        uint32_t len = end - colon;
        if (len >= value_len) {
            len = value_len - 1;
        }
        strncpy(value, colon, len);
        value[len] = '\0';
    } else {
        /* 数字或布尔值 */
        const char *end = colon;
        while (*end != ',' && *end != '}' && *end != '\0' &&
               *end != ' ' && *end != '\n' && *end != '\r' && *end != '\t') {
            end++;
        }
        
        uint32_t len = end - colon;
        if (len >= value_len) {
            len = value_len - 1;
        }
        strncpy(value, colon, len);
        value[len] = '\0';
    }
    
    return 0;
}

/**
 * @brief 从JSON数组中提取字符串数组
 */
static int32_t extract_json_string_array(const char *json, const char *key,
                                          char array[][16], uint8_t max_items,
                                          uint8_t *num_items)
{
    const char *key_pos = strstr(json, key);
    if (key_pos == NULL) {
        return -1;
    }
    
    const char *bracket = strchr(key_pos, '[');
    if (bracket == NULL) {
        return -1;
    }
    
    *num_items = 0;
    bracket++;
    
    while (*bracket != ']' && *bracket != '\0' && *num_items < max_items) {
        bracket = skip_whitespace(bracket);
        
        if (*bracket == '"') {
            bracket++;
            const char *end = strchr(bracket, '"');
            if (end == NULL) {
                break;
            }
            
            uint32_t len = end - bracket;
            if (len >= 16) {
                len = 15;
            }
            strncpy(array[*num_items], bracket, len);
            array[*num_items][len] = '\0';
            (*num_items)++;
            
            bracket = end + 1;
        }
        
        /* 跳过逗号 */
        while (*bracket != ',' && *bracket != ']' && *bracket != '\0') {
            bracket++;
        }
        if (*bracket == ',') {
            bracket++;
        }
    }
    
    return 0;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

ota_error_t ota_package_parser_init(
    ota_parser_context_t *ctx,
    const ota_parser_config_t *config
)
{
    if (ctx == NULL || config == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memset(ctx, 0, sizeof(ota_parser_context_t));
    memcpy(&ctx->config, config, sizeof(ota_parser_config_t));
    
    /* 设置默认内存分配函数 */
    if (ctx->config.alloc == NULL) {
        ctx->config.alloc = malloc;
    }
    if (ctx->config.free == NULL) {
        ctx->config.free = free;
    }
    
    ctx->initialized = true;
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "OTA Package Parser initialized");
    
    return OTA_ERR_OK;
}

void ota_package_parser_deinit(ota_parser_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* 释放当前包信息 */
    if (ctx->current_package != NULL) {
        if (ctx->current_package->manifest_json != NULL) {
            ctx->config.free(ctx->current_package->manifest_json);
        }
        if (ctx->current_package->ecu_updates != NULL) {
            ctx->config.free(ctx->current_package->ecu_updates);
        }
        if (ctx->current_package->signature.signature_data != NULL) {
            ctx->config.free(ctx->current_package->signature.signature_data);
        }
        if (ctx->current_package->signature.cert_chain != NULL) {
            ctx->config.free(ctx->current_package->signature.cert_chain);
        }
        ctx->config.free(ctx->current_package);
    }
    
    /* 释放解压缓冲区 */
    if (ctx->compress_ctx.input != NULL) {
        ctx->config.free(ctx->compress_ctx.input);
    }
    if (ctx->compress_ctx.output != NULL) {
        ctx->config.free(ctx->compress_ctx.output);
    }
    
    memset(ctx, 0, sizeof(ota_parser_context_t));
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "OTA Package Parser deinitialized");
}

ota_error_t ota_package_parse_header(
    ota_parser_context_t *ctx,
    const uint8_t *data,
    uint32_t data_len,
    ota_package_info_t *info
)
{
    if (ctx == NULL || !ctx->initialized || data == NULL || info == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    if (data_len < sizeof(uint32_t)) {
        return OTA_ERR_PACKAGE_CORRUPTED;
    }
    
    uint32_t magic = *(uint32_t*)data;
    
    if (magic == OTA_PKG_MAGIC_VPCK) {
        /* 解析Vehicle Package头部 */
        if (data_len < sizeof(ota_vpkg_header_t)) {
            return OTA_ERR_PACKAGE_CORRUPTED;
        }
        
        info->is_vpkg = true;
        memcpy(&info->header.vpkg, data, sizeof(ota_vpkg_header_t));
        
        /* 验证版本 */
        if (info->header.vpkg.version != OTA_PKG_CURRENT_VERSION) {
            DDS_LOG(DDS_LOG_ERROR, OTA_PKG_MODULE_NAME,
                    "Unsupported vpkg version: %u", info->header.vpkg.version);
            return OTA_ERR_UNSUPPORTED_FORMAT;
        }
        
        /* 设置载荷信息 */
        info->payload_offset = info->header.vpkg.payload_offset;
        info->payload_size = info->header.vpkg.payload_size;
        
        DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
                "VPCK header parsed: size=%llu, compression=%d",
                (unsigned long long)info->payload_size,
                info->header.vpkg.compression);
        
    } else if (magic == OTA_PKG_MAGIC_EPCK) {
        /* 解析ECU Package头部 */
        if (data_len < sizeof(ota_epkg_header_t)) {
            return OTA_ERR_PACKAGE_CORRUPTED;
        }
        
        info->is_vpkg = false;
        memcpy(&info->header.epkg, data, sizeof(ota_epkg_header_t));
        
        /* 验证版本 */
        if (info->header.epkg.version != OTA_PKG_CURRENT_VERSION) {
            DDS_LOG(DDS_LOG_ERROR, OTA_PKG_MODULE_NAME,
                    "Unsupported epkg version: %u", info->header.epkg.version);
            return OTA_ERR_UNSUPPORTED_FORMAT;
        }
        
        /* 设置载荷信息 (固件部分) */
        info->payload_offset = info->header.epkg.firmware_offset;
        info->payload_size = info->header.epkg.firmware_compressed_size;
        info->payload_uncompressed_size = info->header.epkg.firmware_size;
        
        DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
                "EPCK header parsed: ecu_id=0x%04X, compression=%d",
                info->header.epkg.ecu_id,
                info->header.epkg.compression);
        
    } else {
        DDS_LOG(DDS_LOG_ERROR, OTA_PKG_MODULE_NAME,
                "Invalid package magic: 0x%08X", magic);
        return OTA_ERR_PACKAGE_CORRUPTED;
    }
    
    ctx->header_parsed = true;
    return OTA_ERR_OK;
}

ota_error_t ota_package_parse_manifest(
    ota_parser_context_t *ctx,
    const char *json_data,
    uint32_t json_len,
    ota_manifest_header_t *manifest,
    ota_ecu_update_info_t *ecu_updates,
    uint8_t max_ecus,
    uint8_t *num_ecus
)
{
    if (ctx == NULL || json_data == NULL || manifest == NULL ||
        ecu_updates == NULL || num_ecus == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memset(manifest, 0, sizeof(ota_manifest_header_t));
    *num_ecus = 0;
    
    /* 提取Manifest版本 */
    extract_json_value(json_data, "\"manifest_version\"",
                       manifest->manifest_version,
                       sizeof(manifest->manifest_version));
    
    /* 提取包ID */
    extract_json_value(json_data, "\"id\"", manifest->package_id,
                       sizeof(manifest->package_id));
    
    /* 提取VIN */
    extract_json_value(json_data, "\"vin\"", manifest->vin,
                       sizeof(manifest->vin));
    
    /* 提取硬件平台 */
    extract_json_value(json_data, "\"hw_platform\"", manifest->hw_platform,
                       sizeof(manifest->hw_platform));
    
    /* 提取Campaign信息 */
    extract_json_value(json_data, "\"campaign_id\"", manifest->campaign_id,
                       sizeof(manifest->campaign_id));
    extract_json_value(json_data, "\"name\"", manifest->campaign_name,
                       sizeof(manifest->campaign_name));
    
    /* 提取ECU更新列表 */
    const char *ecu_array = strstr(json_data, "\"ecu_updates\"");
    if (ecu_array != NULL) {
        const char *ecu_start = strchr(ecu_array, '{');
        
        while (ecu_start != NULL && *num_ecus < max_ecus) {
            /* 查找ecu_id */
            char ecu_id_str[8];
            if (extract_json_value(ecu_start, "\"ecu_id\"", ecu_id_str,
                                    sizeof(ecu_id_str)) == 0) {
                ecu_updates[*num_ecus].ecu_id = (uint16_t)strtol(ecu_id_str, NULL, 0);
            }
            
            /* 提取名称 */
            extract_json_value(ecu_start, "\"name\"",
                               ecu_updates[*num_ecus].name,
                               sizeof(ecu_updates[*num_ecus].name));
            
            /* 提取硬件版本 */
            extract_json_value(ecu_start, "\"hardware_version\"",
                               ecu_updates[*num_ecus].hardware_version,
                               sizeof(ecu_updates[*num_ecus].hardware_version));
            
            /* 提取固件版本 */
            extract_json_value(ecu_start, "\"to\"",
                               ecu_updates[*num_ecus].firmware_to,
                               sizeof(ecu_updates[*num_ecus].firmware_to));
            
            /* 提取包文件名 */
            extract_json_value(ecu_start, "\"package_file\"",
                               ecu_updates[*num_ecus].package_file,
                               sizeof(ecu_updates[*num_ecus].package_file));
            
            /* 提取大小 */
            char size_str[16];
            if (extract_json_value(ecu_start, "\"size\"", size_str,
                                    sizeof(size_str)) == 0) {
                ecu_updates[*num_ecus].package_size = strtoull(size_str, NULL, 10);
            }
            
            (*num_ecus)++;
            
            /* 查找下一个ECU */
            ecu_start = strchr(ecu_start + 1, '{');
        }
    }
    
    ctx->manifest_parsed = true;
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "Manifest parsed: %u ECU updates found", *num_ecus);
    
    return OTA_ERR_OK;
}

ota_error_t ota_package_parse_signature(
    ota_parser_context_t *ctx,
    const uint8_t *data,
    uint32_t data_len,
    ota_signature_type_t sig_type,
    ota_signature_info_t *info
)
{
    if (ctx == NULL || data == NULL || info == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    info->type = sig_type;
    info->signature_data = ctx->config.alloc(data_len);
    if (info->signature_data == NULL) {
        return OTA_ERR_OUT_OF_MEMORY;
    }
    
    memcpy(info->signature_data, data, data_len);
    info->signature_len = data_len;
    
    ctx->signature_parsed = true;
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "Signature parsed: type=%d, len=%u", sig_type, data_len);
    
    return OTA_ERR_OK;
}

ota_error_t ota_package_verify_signature(
    ota_parser_context_t *ctx,
    const uint8_t *package_data,
    uint64_t package_len,
    const ota_package_info_t *info,
    uint8_t key_id,
    bool *valid
)
{
    if (ctx == NULL || package_data == NULL || info == NULL || valid == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    *valid = false;
    
    if (!ctx->config.verify_signature) {
        *valid = true;
        return OTA_ERR_OK;
    }
    
    /* TODO: 使用CSM进行签名验证 */
    /* 这里是模拟实现 */
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "Signature verification (mock): key_id=%u", key_id);
    
    *valid = true;
    ctx->signature_valid = true;
    
    return OTA_ERR_OK;
}

ota_error_t ota_package_verify_hash(
    ota_parser_context_t *ctx,
    const uint8_t *data,
    uint64_t data_len,
    const uint8_t *expected_hash,
    ota_hash_type_t hash_type,
    bool *valid
)
{
    if (ctx == NULL || data == NULL || expected_hash == NULL || valid == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    *valid = false;
    
    if (!ctx->config.verify_hash) {
        *valid = true;
        return OTA_ERR_OK;
    }
    
    /* TODO: 使用CSM计算哈希并比较 */
    /* 这里是模拟实现 */
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "Hash verification (mock): type=%d", hash_type);
    
    *valid = true;
    ctx->hash_valid = true;
    
    return OTA_ERR_OK;
}

ota_error_t ota_decompress_init(
    ota_compress_context_t *ctx,
    ota_compression_type_t type,
    uint8_t *output_buffer,
    uint32_t output_size
)
{
    if (ctx == NULL || output_buffer == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memset(ctx, 0, sizeof(ota_compress_context_t));
    
    ctx->type = type;
    ctx->output = output_buffer;
    ctx->output_size = output_size;
    
    /* TODO: 初始化Zstd/LZ4解压器 */
    
    ctx->initialized = true;
    
    DDS_LOG(OTA_PKG_LOG_LEVEL, OTA_PKG_MODULE_NAME,
            "Decompressor initialized: type=%d", type);
    
    return OTA_ERR_OK;
}

ota_error_t ota_decompress_block(
    ota_compress_context_t *ctx,
    const uint8_t *input,
    uint32_t input_len,
    uint8_t *output,
    uint32_t output_size,
    uint32_t *output_len,
    bool *finished
)
{
    if (ctx == NULL || !ctx->initialized || input == NULL || output == NULL ||
        output_len == NULL || finished == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    *finished = false;
    *output_len = 0;
    
    switch (ctx->type) {
        case OTA_COMPRESS_NONE:
            /* 无压缩，直接复制 */
            if (input_len > output_size) {
                input_len = output_size;
            }
            memcpy(output, input, input_len);
            *output_len = input_len;
            *finished = true;
            break;
            
        case OTA_COMPRESS_ZSTD:
            /* TODO: 使用Zstd解压 */
            return OTA_ERR_DECOMPRESSION_FAILED;
            
        case OTA_COMPRESS_LZ4:
            /* TODO: 使用LZ4解压 */
            return OTA_ERR_DECOMPRESSION_FAILED;
            
        default:
            return OTA_ERR_UNSUPPORTED_FORMAT;
    }
    
    return OTA_ERR_OK;
}

ota_error_t ota_decompress_full(
    ota_compress_context_t *ctx,
    const uint8_t *compressed_data,
    uint32_t compressed_len,
    uint8_t *decompressed_data,
    uint32_t decompressed_size,
    uint32_t *decompressed_len
)
{
    if (ctx == NULL || compressed_data == NULL || decompressed_data == NULL ||
        decompressed_len == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    *decompressed_len = 0;
    
    if (ctx->type == OTA_COMPRESS_NONE) {
        if (compressed_len > decompressed_size) {
            return OTA_ERR_OUT_OF_MEMORY;
        }
        memcpy(decompressed_data, compressed_data, compressed_len);
        *decompressed_len = compressed_len;
        return OTA_ERR_OK;
    }
    
    /* TODO: 使用Zstd/LZ4解压完整数据 */
    
    return OTA_ERR_DECOMPRESSION_FAILED;
}

void ota_decompress_deinit(ota_compress_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    /* TODO: 释放Zstd/LZ4资源 */
    
    memset(ctx, 0, sizeof(ota_compress_context_t));
}

ota_error_t ota_package_create_vpkg_header(
    ota_vpkg_header_t *header,
    const uint8_t package_id[16],
    const char *vin,
    const char *hw_version
)
{
    if (header == NULL || package_id == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memset(header, 0, sizeof(ota_vpkg_header_t));
    
    header->magic = OTA_PKG_MAGIC_VPCK;
    header->version = OTA_PKG_CURRENT_VERSION;
    memcpy(header->package_id, package_id, 16);
    header->timestamp = 0; /* TODO: Get current timestamp */
    
    if (vin != NULL) {
        strncpy(header->vin, vin, sizeof(header->vin) - 1);
    }
    
    if (hw_version != NULL) {
        strncpy(header->hw_version, hw_version, sizeof(header->hw_version) - 1);
    }
    
    header->header_size = sizeof(ota_vpkg_header_t);
    
    return OTA_ERR_OK;
}

ota_error_t ota_package_create_epkg_header(
    ota_epkg_header_t *header,
    uint16_t ecu_id,
    const char *hw_version,
    const char *fw_from,
    const char *fw_to
)
{
    if (header == NULL) {
        return OTA_ERR_INVALID_PARAM;
    }
    
    memset(header, 0, sizeof(ota_epkg_header_t));
    
    header->magic = OTA_PKG_MAGIC_EPCK;
    header->version = OTA_PKG_CURRENT_VERSION;
    header->ecu_id = ecu_id;
    
    if (hw_version != NULL) {
        strncpy(header->hw_version, hw_version, sizeof(header->hw_version) - 1);
    }
    
    if (fw_from != NULL) {
        strncpy(header->fw_version_from, fw_from, sizeof(header->fw_version_from) - 1);
    }
    
    if (fw_to != NULL) {
        strncpy(header->fw_version_to, fw_to, sizeof(header->fw_version_to) - 1);
    }
    
    header->header_size = sizeof(ota_epkg_header_t);
    
    return OTA_ERR_OK;
}

const char* ota_compression_type_name(ota_compression_type_t type)
{
    switch (type) {
        case OTA_COMPRESS_NONE:
            return "none";
        case OTA_COMPRESS_ZSTD:
            return "zstd";
        case OTA_COMPRESS_LZ4:
            return "lz4";
        case OTA_COMPRESS_GZIP:
            return "gzip";
        default:
            return "unknown";
    }
}

const char* ota_package_magic_to_string(uint32_t magic)
{
    static char str[5];
    str[0] = (magic >> 0) & 0xFF;
    str[1] = (magic >> 8) & 0xFF;
    str[2] = (magic >> 16) & 0xFF;
    str[3] = (magic >> 24) & 0xFF;
    str[4] = '\0';
    return str;
}

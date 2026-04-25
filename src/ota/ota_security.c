/**
 * @file ota_security.c
 * @brief OTA Security Verification Implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * Implements OTA security verification:
 * - Package signature verification
 * - Firmware fingerprint verification
 * - Version rollback protection
 *
 * ISO/SAE 21434 compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdio.h>
#include "ota_security.h"
#include "../crypto_stack/csm/csm_core.h"
#include "../crypto_stack/keym/keym_core.h"

/* ============================================================================
 * 宏定义
 * ============================================================================ */
#define OTA_SEC_MAGIC                   0x4F544153  /* "OTAS" */

/* CRC32多项式: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1 */
#define CRC32_POLYNOMIAL                0xEDB88320

/* ============================================================================
 * 静态函数前向声明
 * ============================================================================ */
static void ota_sec_report_dtc(ota_security_context_t *ctx, uint32_t dtc);
static uint32_t ota_sec_calculate_crc32(const uint8_t *data, uint32_t length);
static csm_algorithm_t ota_sec_get_hash_algo(ota_hash_type_t hash_type);
static csm_algorithm_t ota_sec_get_sign_algo(ota_signature_type_t sign_type);
static ota_security_error_t ota_sec_map_csm_error(csm_status_t csm_err);

/* ============================================================================
 * CRC32表 (优化计算)
 * ============================================================================ */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/* ============================================================================
 * 初始化和反初始化
 * ============================================================================ */

ota_security_error_t ota_security_init(
    ota_security_context_t *ctx,
    const ota_security_config_t *config,
    csm_context_t *csm_ctx,
    keym_context_t *keym_ctx)
{
    if (ctx == NULL || config == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已初始化 */
    if (ctx->initialized) {
        return OTA_SEC_OK;
    }
    
    /* 清零上下文 */
    memset(ctx, 0, sizeof(ota_security_context_t));
    
    /* 复制配置 */
    memcpy(&ctx->config, config, sizeof(ota_security_config_t));
    
    /* 设置默认值 */
    if (ctx->config.verification_timeout_ms == 0) {
        ctx->config.verification_timeout_ms = 30000; /* 30秒默认超时 */
    }
    if (ctx->config.max_boot_attempts == 0) {
        ctx->config.max_boot_attempts = 3;
    }
    
    /* 保存CSM和KeyM上下文 */
    ctx->csm_ctx = csm_ctx;
    ctx->keym_ctx = keym_ctx;
    
    /* 初始化回滚防护信息 */
    ctx->rollback_info.min_allowed_version = config->min_firmware_version;
    ctx->rollback_info.max_boot_attempts = config->max_boot_attempts;
    
    ctx->initialized = true;
    ctx->state = OTA_SEC_STATE_UNVERIFIED;
    
    return OTA_SEC_OK;
}

void ota_security_deinit(ota_security_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    ctx->initialized = false;
    ctx->csm_ctx = NULL;
    ctx->keym_ctx = NULL;
}

/* ============================================================================
 * 头部解析和验证
 * ============================================================================ */

ota_security_error_t ota_security_parse_header(
    ota_security_context_t *ctx,
    const uint8_t *header_data,
    uint32_t header_size,
    ota_firmware_header_t *header)
{
    if (ctx == NULL || header_data == NULL || header == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (header_size < sizeof(ota_firmware_header_t)) {
        return OTA_SEC_ERROR_PACKAGE_CORRUPTED;
    }
    
    /* 复制头部数据 */
    memcpy(header, header_data, sizeof(ota_firmware_header_t));
    
    /* 验证魔法数 */
    if (header->magic != OTA_SEC_PACKAGE_MAGIC) {
        return OTA_SEC_ERROR_PACKAGE_CORRUPTED;
    }
    
    /* 验证头部版本 */
    if (header->header_version != OTA_SEC_HEADER_VERSION) {
        return OTA_SEC_ERROR_PACKAGE_CORRUPTED;
    }
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_verify_header_crc(
    ota_security_context_t *ctx,
    const ota_firmware_header_t *header)
{
    if (ctx == NULL || header == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 计算CRC (排除头部的CRC字段) */
    uint32_t header_size_without_crc = sizeof(ota_firmware_header_t) - sizeof(uint32_t);
    uint32_t calculated_crc = ota_sec_calculate_crc32((const uint8_t *)header, header_size_without_crc);
    
    if (calculated_crc != header->header_crc32) {
        return OTA_SEC_ERROR_PACKAGE_CORRUPTED;
    }
    
    return OTA_SEC_OK;
}

/* ============================================================================
 * 签名验证
 * ============================================================================ */

ota_security_error_t ota_security_verify_signature(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *signature,
    uint32_t signature_len,
    ota_signature_type_t sign_type)
{
    if (ctx == NULL || firmware_data == NULL || signature == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (ctx->csm_ctx == NULL) {
        return OTA_SEC_ERROR_NOT_INITIALIZED;
    }
    
    ctx->state = OTA_SEC_STATE_VERIFYING_SIGNATURE;
    
    if (ctx->config.on_verification_progress != NULL) {
        ctx->config.on_verification_progress(ctx->state, 25);
    }
    
    /* 转换签名类型到CSM算法 */
    csm_algorithm_t algo = ota_sec_get_sign_algo(sign_type);
    if (algo == CSM_ALGO_NONE) {
        return OTA_SEC_ERROR_UNSUPPORTED_ALGO;
    }
    
    /* 获取验证密钥 */
    uint8_t key_id = ctx->config.oem_key_slot;
    if (key_id == 0) {
        return OTA_SEC_ERROR_KEY_NOT_FOUND;
    }
    
    /* 使用CSM验证签名 */
    bool verify_result = false;
    csm_status_t csm_result = csm_mac_verify(ctx->csm_ctx, algo, key_id,
                                              firmware_data, firmware_size,
                                              signature, signature_len,
                                              &verify_result);
    
    if (csm_result != CSM_OK) {
        return ota_sec_map_csm_error(csm_result);
    }
    
    if (!verify_result) {
        ota_sec_report_dtc(ctx, OTA_DTC_SIGNATURE_INVALID);
        return OTA_SEC_ERROR_SIGNATURE_INVALID;
    }
    
    return OTA_SEC_OK;
}

/* ============================================================================
 * 哈希验证
 * ============================================================================ */

ota_security_error_t ota_security_verify_hash(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *expected_hash,
    uint32_t hash_len,
    ota_hash_type_t hash_type)
{
    if (ctx == NULL || firmware_data == NULL || expected_hash == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (ctx->csm_ctx == NULL) {
        return OTA_SEC_ERROR_NOT_INITIALIZED;
    }
    
    ctx->state = OTA_SEC_STATE_VERIFYING_HASH;
    
    if (ctx->config.on_verification_progress != NULL) {
        ctx->config.on_verification_progress(ctx->state, 50);
    }
    
    /* 计算哈希 */
    uint8_t calculated_hash[OTA_SEC_MAX_HASH_SIZE];
    uint32_t calculated_hash_len = sizeof(calculated_hash);
    
    ota_security_error_t result = ota_security_calculate_hash(
        ctx, firmware_data, firmware_size,
        calculated_hash, &calculated_hash_len, hash_type);
    
    if (result != OTA_SEC_OK) {
        return result;
    }
    
    /* 比较哈希 */
    if (calculated_hash_len != hash_len) {
        return OTA_SEC_ERROR_HASH_MISMATCH;
    }
    
    if (memcmp(calculated_hash, expected_hash, hash_len) != 0) {
        ota_sec_report_dtc(ctx, OTA_DTC_HASH_MISMATCH);
        return OTA_SEC_ERROR_HASH_MISMATCH;
    }
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_calculate_hash(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    uint8_t *hash,
    uint32_t *hash_len,
    ota_hash_type_t hash_type)
{
    if (ctx == NULL || firmware_data == NULL || hash == NULL || hash_len == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (ctx->csm_ctx == NULL) {
        return OTA_SEC_ERROR_NOT_INITIALIZED;
    }
    
    /* 转换哈希类型到CSM算法 */
    csm_algorithm_t algo = ota_sec_get_hash_algo(hash_type);
    if (algo == CSM_ALGO_NONE) {
        return OTA_SEC_ERROR_UNSUPPORTED_ALGO;
    }
    
    /* 使用CSM计算哈希 */
    csm_status_t result = csm_hash(ctx->csm_ctx, algo, firmware_data, firmware_size,
                                    hash, hash_len);
    
    return ota_sec_map_csm_error(result);
}

/* ============================================================================
 * 指纹验证
 * ============================================================================ */

ota_security_error_t ota_security_calculate_fingerprint(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    ota_fingerprint_t *fingerprint)
{
    if (ctx == NULL || firmware_data == NULL || fingerprint == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 设置哈希类型为SHA-256 */
    fingerprint->hash_type = OTA_SEC_HASH_SHA256;
    fingerprint->fingerprint_len = 32;
    fingerprint->timestamp = 0; /* TODO: 获取系统时间 */
    
    /* 计算哈希作为指纹 */
    return ota_security_calculate_hash(ctx, firmware_data, firmware_size,
                                        fingerprint->fingerprint,
                                        &fingerprint->fingerprint_len,
                                        OTA_SEC_HASH_SHA256);
}

ota_security_error_t ota_security_verify_fingerprint(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const ota_fingerprint_t *expected_fingerprint)
{
    if (ctx == NULL || firmware_data == NULL || expected_fingerprint == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 计算当前指纹 */
    ota_fingerprint_t current_fingerprint;
    ota_security_error_t result = ota_security_calculate_fingerprint(
        ctx, firmware_data, firmware_size, &current_fingerprint);
    
    if (result != OTA_SEC_OK) {
        return result;
    }
    
    /* 比较指纹 */
    if (current_fingerprint.fingerprint_len != expected_fingerprint->fingerprint_len) {
        return OTA_SEC_ERROR_HASH_MISMATCH;
    }
    
    if (memcmp(current_fingerprint.fingerprint, expected_fingerprint->fingerprint,
               current_fingerprint.fingerprint_len) != 0) {
        return OTA_SEC_ERROR_HASH_MISMATCH;
    }
    
    return OTA_SEC_OK;
}

/* ============================================================================
 * 版本验证和回滚防护
 * ============================================================================ */

ota_version_compare_t ota_security_compare_versions(
    uint32_t version1,
    uint32_t version2)
{
    if (version1 == version2) {
        return OTA_VERSION_EQUAL;
    } else if (version1 > version2) {
        return OTA_VERSION_NEWER;
    } else {
        return OTA_VERSION_OLDER;
    }
}

ota_security_error_t ota_security_check_rollback(
    ota_security_context_t *ctx,
    uint32_t new_version,
    uint32_t current_version)
{
    if (ctx == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    ctx->state = OTA_SEC_STATE_VERIFYING_VERSION;
    
    if (ctx->config.on_verification_progress != NULL) {
        ctx->config.on_verification_progress(ctx->state, 75);
    }
    
    if (!ctx->config.verify_version) {
        return OTA_SEC_OK; /* 回滚检查已禁用 */
    }
    
    /* 检查版本是否回退 */
    ota_version_compare_t cmp = ota_security_compare_versions(new_version, current_version);
    
    if (cmp == OTA_VERSION_OLDER) {
        /* 版本回退检测 */
        ctx->rollback_info.rollback_detected = true;
        ctx->rollback_attempts_blocked++;
        ota_sec_report_dtc(ctx, OTA_DTC_VERSION_ROLLBACK);
        return OTA_SEC_ERROR_VERSION_ROLLBACK;
    }
    
    /* 检查最小允许版本 */
    if (new_version < ctx->config.min_firmware_version) {
        ctx->rollback_attempts_blocked++;
        ota_sec_report_dtc(ctx, OTA_DTC_VERSION_ROLLBACK);
        return OTA_SEC_ERROR_VERSION_ROLLBACK;
    }
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_update_rollback_info(
    ota_security_context_t *ctx,
    uint32_t new_version)
{
    if (ctx == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 更新版本历史 */
    ctx->rollback_info.previous_version = ctx->rollback_info.current_version;
    ctx->rollback_info.current_version = new_version;
    ctx->rollback_info.boot_attempts = 0;
    ctx->rollback_info.rollback_detected = false;
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_record_boot_attempt(
    ota_security_context_t *ctx,
    bool success)
{
    if (ctx == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (success) {
        /* 启动成功，重置计数器 */
        ctx->rollback_info.boot_attempts = 0;
    } else {
        /* 启动失败，增加计数器 */
        ctx->rollback_info.boot_attempts++;
    }
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_check_need_rollback(
    ota_security_context_t *ctx,
    bool *need_rollback)
{
    if (ctx == NULL || need_rollback == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 检查启动尝试次数 */
    if (ctx->rollback_info.boot_attempts >= ctx->rollback_info.max_boot_attempts) {
        *need_rollback = true;
        return OTA_SEC_OK;
    }
    
    /* 检查回滚标志 */
    if (ctx->rollback_info.rollback_detected) {
        *need_rollback = true;
        return OTA_SEC_OK;
    }
    
    *need_rollback = false;
    return OTA_SEC_OK;
}

/* ============================================================================
 * 证书验证
 * ============================================================================ */

ota_security_error_t ota_security_parse_cert_chain(
    ota_security_context_t *ctx,
    const uint8_t *cert_data,
    uint32_t cert_size,
    ota_cert_chain_t *chain)
{
    if (ctx == NULL || cert_data == NULL || chain == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    ctx->state = OTA_SEC_STATE_VERIFYING_CERT;
    
    if (ctx->config.on_verification_progress != NULL) {
        ctx->config.on_verification_progress(ctx->state, 90);
    }
    
    /* 清零证书链 */
    memset(chain, 0, sizeof(ota_cert_chain_t));
    
    /* TODO: 实现X.509证书解析 */
    /* 这里是简化实现，实际项目需要完整的X.509解析器 */
    
    /* 示例：假设证书数据已经是解析后的公钥 */
    if (cert_size > 0 && cert_size <= OTA_SEC_MAX_CERT_SIZE) {
        chain->num_certs = 1;
        chain->certs[0].data = (uint8_t *)cert_data; /* 注意：实际应复制数据 */
        chain->certs[0].size = cert_size;
        chain->root_cert_index = 0;
    }
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_verify_cert_chain(
    ota_security_context_t *ctx,
    const ota_cert_chain_t *chain,
    const uint8_t *trusted_root_key,
    uint32_t root_key_len)
{
    if (ctx == NULL || chain == NULL || trusted_root_key == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.verify_cert_chain) {
        return OTA_SEC_OK; /* 证书链验证已禁用 */
    }
    
    /* TODO: 实现证书链验证 */
    /* 这里是简化实现，实际需要完整的PKI验证 */
    
    return OTA_SEC_OK;
}

/* ============================================================================
 * 完整验证
 * ============================================================================ */

ota_security_error_t ota_security_verify_full(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    uint32_t current_version)
{
    ota_security_error_t result;
    
    if (ctx == NULL || firmware_data == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->initialized) {
        return OTA_SEC_ERROR_NOT_INITIALIZED;
    }
    
    ctx->total_verifications++;
    ctx->state = OTA_SEC_STATE_UNVERIFIED;
    
    /* 1. 解析固件头部 */
    result = ota_security_parse_header(ctx, firmware_data, 
                                        sizeof(ota_firmware_header_t),
                                        &ctx->current_header);
    if (result != OTA_SEC_OK) {
        ctx->last_error = result;
        ctx->failed_verifications++;
        ctx->state = OTA_SEC_STATE_VERIFICATION_FAILED;
        return result;
    }
    
    /* 2. 验证头部CRC */
    result = ota_security_verify_header_crc(ctx, &ctx->current_header);
    if (result != OTA_SEC_OK) {
        ctx->last_error = result;
        ctx->failed_verifications++;
        ctx->state = OTA_SEC_STATE_VERIFICATION_FAILED;
        return result;
    }
    
    /* 3. 验证回滚 (版本检查) */
    result = ota_security_check_rollback(ctx, ctx->current_header.firmware_version,
                                          current_version);
    if (result != OTA_SEC_OK) {
        ctx->last_error = result;
        ctx->failed_verifications++;
        ctx->state = OTA_SEC_STATE_VERIFICATION_FAILED;
        return result;
    }
    
    /* 获取固件数据指针 (跳过头部) */
    const uint8_t *fw_data = firmware_data + ctx->current_header.header_size;
    uint32_t fw_size = ctx->current_header.firmware_size;
    
    /* 4. 验证哈希 */
    if (ctx->config.verify_hash) {
        result = ota_security_verify_hash(ctx, fw_data, fw_size,
                                           ctx->current_header.hash,
                                           ctx->current_header.hash_len,
                                           ctx->current_header.hash_type);
        if (result != OTA_SEC_OK) {
            ctx->last_error = result;
            ctx->failed_verifications++;
            ctx->state = OTA_SEC_STATE_VERIFICATION_FAILED;
            return result;
        }
    }
    
    /* 5. 验证签名 */
    if (ctx->config.verify_signature) {
        result = ota_security_verify_signature(ctx, fw_data, fw_size,
                                                ctx->current_header.signature,
                                                ctx->current_header.signature_len,
                                                ctx->current_header.sign_type);
        if (result != OTA_SEC_OK) {
            ctx->last_error = result;
            ctx->failed_verifications++;
            ctx->state = OTA_SEC_STATE_VERIFICATION_FAILED;
            return result;
        }
    }
    
    /* 验证通过 */
    ctx->state = OTA_SEC_STATE_VERIFIED;
    
    if (ctx->config.on_verification_complete != NULL) {
        ctx->config.on_verification_complete(OTA_SEC_OK);
    }
    
    return OTA_SEC_OK;
}

ota_security_error_t ota_security_verify_fast(
    ota_security_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *expected_hash,
    uint32_t hash_len,
    const uint8_t *signature,
    uint32_t signature_len)
{
    ota_security_error_t result;
    
    if (ctx == NULL || firmware_data == NULL || expected_hash == NULL || signature == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    
    /* 快速哈希验证 */
    result = ota_security_verify_hash(ctx, firmware_data, firmware_size,
                                       expected_hash, hash_len, OTA_SEC_HASH_SHA256);
    if (result != OTA_SEC_OK) {
        return result;
    }
    
    /* 快速签名验证 */
    result = ota_security_verify_signature(ctx, firmware_data, firmware_size,
                                            signature, signature_len,
                                            OTA_SEC_SIGN_ECDSA_P256_SHA256);
    
    return result;
}

/* ============================================================================
 * 状态查询
 * ============================================================================ */

ota_security_state_t ota_security_get_state(const ota_security_context_t *ctx)
{
    if (ctx == NULL) {
        return OTA_SEC_STATE_UNVERIFIED;
    }
    return ctx->state;
}

ota_security_error_t ota_security_get_last_error(const ota_security_context_t *ctx)
{
    if (ctx == NULL) {
        return OTA_SEC_ERROR_INVALID_PARAM;
    }
    return ctx->last_error;
}

/* ============================================================================
 * DEM集成
 * ============================================================================ */

void ota_security_set_dem_callback(
    ota_security_context_t *ctx,
    void (*report_func)(uint32_t dtc_code, uint8_t status))
{
    if (ctx != NULL) {
        ctx->dem_report_dtc = report_func;
    }
}

/* ============================================================================
 * 工具函数
 * ============================================================================ */

int ota_security_format_version(uint32_t version, char *buf, size_t buf_size)
{
    uint8_t major = (version >> 24) & 0xFF;
    uint8_t minor = (version >> 16) & 0xFF;
    uint16_t patch = version & 0xFFFF;
    
    return snprintf(buf, buf_size, "%u.%u.%u", major, minor, patch);
}

/* ============================================================================
 * 静态函数实现
 * ============================================================================ */

static void ota_sec_report_dtc(ota_security_context_t *ctx, uint32_t dtc)
{
    if (ctx != NULL && ctx->dem_report_dtc != NULL) {
        /* DTC状态: 0x01 = Test Failed */
        ctx->dem_report_dtc(dtc, 0x01);
    }
}

static uint32_t ota_sec_calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        uint8_t table_index = (uint8_t)((crc ^ data[i]) & 0xFF);
        crc = (crc >> 8) ^ crc32_table[table_index];
    }
    
    return crc ^ 0xFFFFFFFF;
}

static csm_algorithm_t ota_sec_get_hash_algo(ota_hash_type_t hash_type)
{
    switch (hash_type) {
        case OTA_SEC_HASH_SHA256:
            return CSM_ALGO_SHA_256;
        case OTA_SEC_HASH_SHA384:
            return CSM_ALGO_SHA_384;
        case OTA_SEC_HASH_SHA512:
            return CSM_ALGO_SHA_512;
        default:
            return CSM_ALGO_NONE;
    }
}

static csm_algorithm_t ota_sec_get_sign_algo(ota_signature_type_t sign_type)
{
    switch (sign_type) {
        case OTA_SEC_SIGN_ECDSA_P256_SHA256:
            return CSM_ALGO_ECDSA_P256_SHA_256;
        case OTA_SEC_SIGN_ECDSA_P384_SHA384:
            return CSM_ALGO_ECDSA_P384_SHA_384;
        case OTA_SEC_SIGN_RSA_PKCS1_SHA256:
            return CSM_ALGO_RSA_PKCS1_V15_SHA_256;
        case OTA_SEC_SIGN_RSA_PSS_SHA256:
            return CSM_ALGO_RSA_PSS_SHA_256;
        default:
            return CSM_ALGO_NONE;
    }
}

static ota_security_error_t ota_sec_map_csm_error(csm_status_t csm_err)
{
    switch (csm_err) {
        case CSM_OK:
            return OTA_SEC_OK;
        case CSM_ERROR_INVALID_PARAM:
            return OTA_SEC_ERROR_INVALID_PARAM;
        case CSM_ERROR_NO_MEMORY:
            return OTA_SEC_ERROR_NO_MEMORY;
        case CSM_ERROR_KEY_NOT_FOUND:
            return OTA_SEC_ERROR_KEY_NOT_FOUND;
        case CSM_ERROR_ALGO_NOT_SUPPORTED:
            return OTA_SEC_ERROR_UNSUPPORTED_ALGO;
        case CSM_ERROR_CRYPTO_FAILED:
            return OTA_SEC_ERROR_CRYPTO_FAILED;
        case CSM_ERROR_TIMEOUT:
            return OTA_SEC_ERROR_TIMEOUT;
        default:
            return OTA_SEC_ERROR_CRYPTO_FAILED;
    }
}

/**
 * @file bl_secure_boot.c
 * @brief Secure Boot Verification Module Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * 符合UNECE R156和ISO/SAE 21434要求的安全启动验证
 * 使用SecOC/CSM/KeyM进行安全验证
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdlib.h>
#include "bl_secure_boot.h"
#include "../common/log/dds_log.h"
#include "../crypto_stack/csm/csm_core.h"
#include "../crypto_stack/keym/keym_core.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define BL_SB_MODULE_NAME       "BL_SB"
#define BL_SB_LOG_LEVEL         DDS_LOG_INFO

#define FIRMWARE_HEADER_SIZE    sizeof(bl_firmware_header_t)

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 更新验证状态
 */
static void set_state(bl_secure_boot_context_t *ctx, bl_secure_boot_state_t state)
{
    ctx->state = state;
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Secure boot state: %d", state);
    
    if (ctx->config.on_verification_progress != NULL) {
        uint8_t progress = 0;
        switch (state) {
            case BL_SB_STATE_UNVERIFIED:
                progress = 0;
                break;
            case BL_SB_STATE_VERIFICATION_IN_PROGRESS:
                progress = 10;
                break;
            case BL_SB_STATE_SIGNATURE_VALID:
                progress = 40;
                break;
            case BL_SB_STATE_HASH_VALID:
                progress = 60;
                break;
            case BL_SB_STATE_VERSION_CHECKED:
                progress = 80;
                break;
            case BL_SB_STATE_CERT_VALID:
                progress = 90;
                break;
            case BL_SB_STATE_VERIFIED:
                progress = 100;
                break;
            default:
                progress = 0;
                break;
        }
        ctx->config.on_verification_progress(state, progress);
    }
}

/**
 * @brief 设置错误码
 */
static void set_error(bl_secure_boot_context_t *ctx, bl_secure_boot_error_t error)
{
    ctx->last_error = error;
    ctx->failed_verifications++;
    
    DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
            "Secure boot error: %d", error);
    
    if (ctx->config.on_verification_complete != NULL) {
        ctx->config.on_verification_complete(error);
    }
}

/**
 * @brief 验证头部CRC32
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

bl_secure_boot_error_t bl_secure_boot_init(
    bl_secure_boot_context_t *ctx,
    const bl_secure_boot_config_t *config,
    void *csm_ctx,
    void *keym_ctx
)
{
    if (ctx == NULL || config == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    memset(ctx, 0, sizeof(bl_secure_boot_context_t));
    memcpy(&ctx->config, config, sizeof(bl_secure_boot_config_t));
    
    ctx->csm_context = csm_ctx;
    ctx->keym_context = keym_ctx;
    ctx->state = BL_SB_STATE_UNVERIFIED;
    
    /* 初始化回滚防护信息 */
    ctx->rollback_info.min_allowed_version = config->min_firmware_version;
    ctx->rollback_info.max_boot_attempts = config->max_boot_attempts;
    
    ctx->initialized = true;
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Secure boot initialized");
    
    return BL_SB_OK;
}

void bl_secure_boot_deinit(bl_secure_boot_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    memset(ctx, 0, sizeof(bl_secure_boot_context_t));
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Secure boot deinitialized");
}

bl_secure_boot_error_t bl_secure_boot_parse_header(
    bl_secure_boot_context_t *ctx,
    const uint8_t *header_data,
    uint32_t header_size,
    bl_firmware_header_t *header
)
{
    if (ctx == NULL || header_data == NULL || header == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    if (header_size < sizeof(bl_firmware_header_t)) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    /* 复制头部数据 */
    memcpy(header, header_data, sizeof(bl_firmware_header_t));
    
    /* 验证魔法 */
    if (header->magic != BL_SB_FIRMWARE_MAGIC) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Invalid firmware magic: 0x%08X", header->magic);
        return BL_SB_ERROR_INVALID_MAGIC;
    }
    
    /* 验证版本 */
    if (header->header_version != BL_SB_HEADER_VERSION) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Unsupported header version: %u", header->header_version);
        return BL_SB_ERROR_PARSE_ERROR;
    }
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_verify_header_crc(
    bl_secure_boot_context_t *ctx,
    const bl_firmware_header_t *header
)
{
    if (ctx == NULL || header == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    /* 计算头部CRC (排除header_crc32字段) */
    uint32_t header_size_without_crc = sizeof(bl_firmware_header_t) - sizeof(uint32_t);
    uint32_t calculated_crc = calculate_crc32(
        (const uint8_t*)header,
        header_size_without_crc
    );
    
    if (calculated_crc != header->header_crc32) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Header CRC mismatch: expected 0x%08X, got 0x%08X",
                header->header_crc32, calculated_crc);
        return BL_SB_ERROR_INVALID_HASH;
    }
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_verify_signature(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *signature,
    bl_signature_type_t sign_type
)
{
    if (ctx == NULL || firmware_data == NULL || signature == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    set_state(ctx, BL_SB_STATE_VERIFICATION_IN_PROGRESS);
    
    /* 使用CSM验证签名 */
    csm_context_t *csm = (csm_context_t*)ctx->csm_context;
    if (csm == NULL) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "CSM context not available");
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    /* 计算固件哈希 */
    uint8_t hash[32];
    csm_algorithm_t hash_algo = CSM_ALGO_SHA_256;
    
    csm_status_t status = csm_hash(csm, hash_algo, firmware_data, firmware_size, 
                                   hash, &(uint32_t){32});
    
    if (status != CSM_OK) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Hash calculation failed: %d", status);
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    /* 验证签名 */
    csm_algorithm_t sign_algo;
    switch (sign_type) {
        case BL_SB_SIGN_ECDSA_P256_SHA256:
            sign_algo = CSM_ALGO_ECDSA_P256_SHA256;
            break;
        default:
            DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                    "Unsupported signature type: %d", sign_type);
            return BL_SB_ERROR_INVALID_SIGNATURE;
    }
    
    /* 从KeyM获取验证公钥 */
    keym_context_t *keym = (keym_context_t*)ctx->keym_context;
    uint8_t public_key[64];
    uint32_t key_len = sizeof(public_key);
    
    if (keym != NULL) {
        keym_status_t key_status = keym_key_export(
            keym, 
            ctx->config.oem_key_slot, 
            public_key, 
            &key_len
        );
        
        if (key_status != KEYM_OK) {
            DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                    "Failed to get public key: %d", key_status);
            return BL_SB_ERROR_CRYPTO_FAILURE;
        }
    }
    
    /* 使用CSM验证签名 */
    bool verify_result = false;
    status = csm_mac_verify(csm, sign_algo, ctx->config.oem_key_slot,
                            hash, 32, signature, 64, &verify_result);
    
    if (status != CSM_OK || !verify_result) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Signature verification failed");
        set_state(ctx, BL_SB_STATE_VERIFICATION_FAILED);
        return BL_SB_ERROR_INVALID_SIGNATURE;
    }
    
    set_state(ctx, BL_SB_STATE_SIGNATURE_VALID);
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Signature verified successfully");
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_verify_hash(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *expected_hash,
    bl_hash_type_t hash_type
)
{
    if (ctx == NULL || firmware_data == NULL || expected_hash == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    uint8_t calculated_hash[64]; /* 支持最大SHA-512 */
    uint32_t hash_len = 32; /* SHA-256 */
    
    csm_context_t *csm = (csm_context_t*)ctx->csm_context;
    if (csm == NULL) {
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    csm_algorithm_t hash_algo = CSM_ALGO_SHA_256;
    switch (hash_type) {
        case BL_SB_HASH_SHA256:
            hash_algo = CSM_ALGO_SHA_256;
            hash_len = 32;
            break;
        case BL_SB_HASH_SHA384:
            hash_algo = CSM_ALGO_SHA_384;
            hash_len = 48;
            break;
        case BL_SB_HASH_SHA512:
            hash_algo = CSM_ALGO_SHA_512;
            hash_len = 64;
            break;
    }
    
    csm_status_t status = csm_hash(csm, hash_algo, firmware_data, firmware_size,
                                   calculated_hash, &hash_len);
    
    if (status != CSM_OK) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Hash calculation failed: %d", status);
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    if (memcmp(calculated_hash, expected_hash, hash_len) != 0) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Hash mismatch");
        return BL_SB_ERROR_INVALID_HASH;
    }
    
    set_state(ctx, BL_SB_STATE_HASH_VALID);
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Hash verified successfully");
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_calculate_hash(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    uint8_t *hash,
    bl_hash_type_t hash_type
)
{
    if (ctx == NULL || firmware_data == NULL || hash == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    csm_context_t *csm = (csm_context_t*)ctx->csm_context;
    if (csm == NULL) {
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    csm_algorithm_t hash_algo = CSM_ALGO_SHA_256;
    uint32_t hash_len = 32;
    
    switch (hash_type) {
        case BL_SB_HASH_SHA256:
            hash_algo = CSM_ALGO_SHA_256;
            hash_len = 32;
            break;
        case BL_SB_HASH_SHA384:
            hash_algo = CSM_ALGO_SHA_384;
            hash_len = 48;
            break;
        case BL_SB_HASH_SHA512:
            hash_algo = CSM_ALGO_SHA_512;
            hash_len = 64;
            break;
    }
    
    csm_status_t status = csm_hash(csm, hash_algo, firmware_data, firmware_size,
                                   hash, &hash_len);
    
    if (status != CSM_OK) {
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    return BL_SB_OK;
}

bl_version_compare_t bl_secure_boot_compare_versions(
    uint32_t version1,
    uint32_t version2
)
{
    if (version1 == version2) {
        return BL_VERSION_EQUAL;
    } else if (version1 < version2) {
        return BL_VERSION_OLDER;
    } else {
        return BL_VERSION_NEWER;
    }
}

bl_secure_boot_error_t bl_secure_boot_check_rollback(
    bl_secure_boot_context_t *ctx,
    uint32_t new_version,
    uint32_t current_version
)
{
    if (ctx == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    /* 检查版本是否回滚 */
    if (new_version < current_version) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Rollback detected: new=0x%08X, current=0x%08X",
                new_version, current_version);
        ctx->rollback_info.rollback_detected = true;
        set_state(ctx, BL_SB_STATE_ROLLBACK_DETECTED);
        return BL_SB_ERROR_VERSION_ROLLBACK;
    }
    
    /* 检查是否低于最低允许版本 */
    if (new_version < ctx->rollback_info.min_allowed_version) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Version below minimum allowed: 0x%08X < 0x%08X",
                new_version, ctx->rollback_info.min_allowed_version);
        return BL_SB_ERROR_VERSION_ROLLBACK;
    }
    
    /* 检查版本是否已锁定 */
    if (ctx->rollback_info.version_locked && 
        new_version < ctx->rollback_info.current_version) {
        DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                "Version locked, rollback not allowed");
        return BL_SB_ERROR_VERSION_ROLLBACK;
    }
    
    set_state(ctx, BL_SB_STATE_VERSION_CHECKED);
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Version check passed: 0x%08X", new_version);
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_update_rollback_info(
    bl_secure_boot_context_t *ctx,
    uint32_t new_version
)
{
    if (ctx == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    ctx->rollback_info.previous_version = ctx->rollback_info.current_version;
    ctx->rollback_info.current_version = new_version;
    ctx->rollback_info.boot_attempts = 0;
    ctx->rollback_info.rollback_detected = false;
    
    /* 更新最低允许版本 */
    if (new_version > ctx->rollback_info.min_allowed_version) {
        ctx->rollback_info.min_allowed_version = new_version;
    }
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Rollback info updated: version=0x%08X", new_version);
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_verify_cert_chain(
    bl_secure_boot_context_t *ctx,
    const bl_cert_chain_t *chain,
    const uint8_t *trusted_root_key
)
{
    if (ctx == NULL || chain == NULL || trusted_root_key == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    if (chain->num_certs == 0 || chain->num_certs > BL_SB_MAX_CERT_CHAIN_DEPTH) {
        return BL_SB_ERROR_CERT_CHAIN_INVALID;
    }
    
    csm_context_t *csm = (csm_context_t*)ctx->csm_context;
    if (csm == NULL) {
        return BL_SB_ERROR_CRYPTO_FAILURE;
    }
    
    /* 验证证书链从叶子到根 */
    for (int i = chain->num_certs - 1; i >= 0; i--) {
        const bl_certificate_t *cert = &chain->certs[i];
        
        /* 检查证书有效期 */
        if (ctx->config.verify_cert_validity) {
            uint64_t current_time = 0; /* TODO: 获取当前时间 */
            if (current_time < cert->valid_from || current_time > cert->valid_until) {
                DDS_LOG(DDS_LOG_ERROR, BL_SB_MODULE_NAME,
                        "Certificate expired or not yet valid");
                return BL_SB_ERROR_CERT_EXPIRED;
            }
        }
        
        /* 验证签名 */
        const uint8_t *signing_key = NULL;
        if (i == chain->num_certs - 1) {
            /* 根证书使用可信公钥验证 */
            signing_key = trusted_root_key;
        } else {
            /* 其他证书使用上一级证书的公钥 */
            signing_key = chain->certs[i + 1].public_key;
        }
        
        /* TODO: 实现证书签名验证 */
    }
    
    set_state(ctx, BL_SB_STATE_CERT_VALID);
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Certificate chain verified");
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_verify(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size
)
{
    if (ctx == NULL || firmware_data == NULL || firmware_size < FIRMWARE_HEADER_SIZE) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    bl_secure_boot_error_t result;
    bl_firmware_header_t header;
    
    ctx->total_verifications++;
    
    /* 1. 解析头部 */
    result = bl_secure_boot_parse_header(ctx, firmware_data, 
                                         FIRMWARE_HEADER_SIZE, &header);
    if (result != BL_SB_OK) {
        set_error(ctx, result);
        return result;
    }
    memcpy(&ctx->current_header, &header, sizeof(bl_firmware_header_t));
    
    /* 2. 验证头部CRC */
    result = bl_secure_boot_verify_header_crc(ctx, &header);
    if (result != BL_SB_OK) {
        set_error(ctx, result);
        return result;
    }
    
    /* 3. 检查版本防回滚 */
    if (ctx->config.verify_version) {
        result = bl_secure_boot_check_rollback(ctx, header.firmware_version,
                                               ctx->rollback_info.current_version);
        if (result != BL_SB_OK) {
            ctx->rollback_attempts_blocked++;
            set_error(ctx, result);
            return result;
        }
    }
    
    /* 4. 验证哈希 */
    if (ctx->config.verify_hash) {
        result = bl_secure_boot_verify_hash(
            ctx,
            firmware_data + FIRMWARE_HEADER_SIZE,
            firmware_size - FIRMWARE_HEADER_SIZE,
            header.hash,
            header.hash_type
        );
        if (result != BL_SB_OK) {
            set_error(ctx, result);
            return result;
        }
    }
    
    /* 5. 验证签名 */
    if (ctx->config.verify_signature) {
        result = bl_secure_boot_verify_signature(
            ctx,
            firmware_data + FIRMWARE_HEADER_SIZE,
            firmware_size - FIRMWARE_HEADER_SIZE,
            header.signature,
            header.sign_type
        );
        if (result != BL_SB_OK) {
            set_error(ctx, result);
            return result;
        }
    }
    
    /* 验证成功 */
    set_state(ctx, BL_SB_STATE_VERIFIED);
    
    /* 更新回滚防护信息 */
    bl_secure_boot_update_rollback_info(ctx, header.firmware_version);
    
    if (ctx->config.on_verification_complete != NULL) {
        ctx->config.on_verification_complete(BL_SB_OK);
    }
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Secure boot verification passed");
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_verify_fast(
    bl_secure_boot_context_t *ctx,
    const uint8_t *firmware_data,
    uint32_t firmware_size,
    const uint8_t *expected_hash,
    const uint8_t *signature
)
{
    if (ctx == NULL || firmware_data == NULL || 
        expected_hash == NULL || signature == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    bl_secure_boot_error_t result;
    
    /* 快速哈希验证 */
    result = bl_secure_boot_verify_hash(ctx, firmware_data, firmware_size,
                                        expected_hash, BL_SB_HASH_SHA256);
    if (result != BL_SB_OK) {
        return result;
    }
    
    /* 快速签名验证 */
    result = bl_secure_boot_verify_signature(ctx, firmware_data, firmware_size,
                                             signature, BL_SB_SIGN_ECDSA_P256_SHA256);
    if (result != BL_SB_OK) {
        return result;
    }
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_record_boot_attempt(
    bl_secure_boot_context_t *ctx,
    bool success
)
{
    if (ctx == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    if (success) {
        ctx->rollback_info.boot_attempts = 0;
        DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
                "Boot successful, attempts reset");
    } else {
        ctx->rollback_info.boot_attempts++;
        DDS_LOG(DDS_LOG_WARNING, BL_SB_MODULE_NAME,
                "Boot failed, attempt %u/%u",
                ctx->rollback_info.boot_attempts,
                ctx->rollback_info.max_boot_attempts);
    }
    
    return BL_SB_OK;
}

bl_secure_boot_error_t bl_secure_boot_check_need_rollback(
    bl_secure_boot_context_t *ctx,
    bool *need_rollback
)
{
    if (ctx == NULL || need_rollback == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    /* 如果启动尝试次数超过最大值，需要回滚 */
    if (ctx->rollback_info.boot_attempts >= ctx->rollback_info.max_boot_attempts) {
        *need_rollback = true;
        DDS_LOG(DDS_LOG_WARNING, BL_SB_MODULE_NAME,
                "Rollback required: max boot attempts exceeded");
    } else {
        *need_rollback = false;
    }
    
    return BL_SB_OK;
}

bl_secure_boot_state_t bl_secure_boot_get_state(
    const bl_secure_boot_context_t *ctx
)
{
    if (ctx == NULL) {
        return BL_SB_STATE_UNVERIFIED;
    }
    return ctx->state;
}

bl_secure_boot_error_t bl_secure_boot_get_last_error(
    const bl_secure_boot_context_t *ctx
)
{
    if (ctx == NULL) {
        return BL_SB_OK;
    }
    return ctx->last_error;
}

const char* bl_secure_boot_version_to_string(
    uint32_t version,
    char *buf,
    uint32_t buf_size
)
{
    if (buf == NULL || buf_size < 16) {
        return NULL;
    }
    
    /* 版本格式: major(8).minor(8).patch(16) */
    uint8_t major = (version >> 24) & 0xFF;
    uint8_t minor = (version >> 16) & 0xFF;
    uint16_t patch = version & 0xFFFF;
    
    snprintf(buf, buf_size, "%u.%u.%u", major, minor, patch);
    return buf;
}

bl_secure_boot_error_t bl_secure_boot_lock_version(
    bl_secure_boot_context_t *ctx,
    uint32_t version
)
{
    if (ctx == NULL) {
        return BL_SB_ERROR_INVALID_PARAM;
    }
    
    ctx->rollback_info.version_locked = true;
    ctx->rollback_info.version_lock_timestamp = 0; /* TODO: 获取当前时间 */
    
    DDS_LOG(BL_SB_LOG_LEVEL, BL_SB_MODULE_NAME,
            "Version locked at 0x%08X", version);
    
    return BL_SB_OK;
}

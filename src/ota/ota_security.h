/**
 * @file ota_security.h
 * @brief OTA Security Verification Module
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

#ifndef OTA_SECURITY_H
#define OTA_SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/types/eth_types.h"
#include "../crypto_stack/csm/csm_core.h"
#include "../crypto_stack/keym/keym_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define OTA_SECURITY_MAJOR_VERSION      1
#define OTA_SECURITY_MINOR_VERSION      0
#define OTA_SECURITY_PATCH_VERSION      0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_SEC_MAX_SIGNATURE_SIZE      64
#define OTA_SEC_MAX_HASH_SIZE           64
#define OTA_SEC_MAX_CERT_CHAIN_DEPTH    4
#define OTA_SEC_MAX_CERT_SIZE           2048
#define OTA_SEC_MAX_FIRMWARE_SIZE       (256 * 1024 * 1024)
#define OTA_SEC_PACKAGE_MAGIC           0x53504B47
#define OTA_SEC_HEADER_VERSION          1

/* DTC错误码 */
#define OTA_DTC_SIGNATURE_INVALID       0xF0B001
#define OTA_DTC_HASH_MISMATCH           0xF0B002
#define OTA_DTC_VERSION_ROLLBACK        0xF0B003
#define OTA_DTC_CERT_INVALID            0xF0B004
#define OTA_DTC_SECURITY_TIMEOUT        0xF0B005

/* ============================================================================
 * 安全验证状态
 * ============================================================================ */
typedef enum {
    OTA_SEC_STATE_UNVERIFIED = 0,
    OTA_SEC_STATE_VERIFYING_SIGNATURE,
    OTA_SEC_STATE_VERIFYING_HASH,
    OTA_SEC_STATE_VERIFYING_VERSION,
    OTA_SEC_STATE_VERIFYING_CERT,
    OTA_SEC_STATE_VERIFIED,
    OTA_SEC_STATE_VERIFICATION_FAILED
} ota_security_state_t;

/* ============================================================================
 * 安全错误码
 * ============================================================================ */
typedef enum {
    OTA_SEC_OK = 0,
    OTA_SEC_ERROR_INVALID_PARAM = -1,
    OTA_SEC_ERROR_NOT_INITIALIZED = -2,
    OTA_SEC_ERROR_NO_MEMORY = -3,
    OTA_SEC_ERROR_SIGNATURE_INVALID = -4,
    OTA_SEC_ERROR_HASH_MISMATCH = -5,
    OTA_SEC_ERROR_VERSION_ROLLBACK = -6,
    OTA_SEC_ERROR_CERT_INVALID = -7,
    OTA_SEC_ERROR_CERT_EXPIRED = -8,
    OTA_SEC_ERROR_CERT_CHAIN_INVALID = -9,
    OTA_SEC_ERROR_CRYPTO_FAILED = -10,
    OTA_SEC_ERROR_PACKAGE_CORRUPTED = -11,
    OTA_SEC_ERROR_UNSUPPORTED_ALGO = -12,
    OTA_SEC_ERROR_TIMEOUT = -13,
    OTA_SEC_ERROR_KEY_NOT_FOUND = -14
} ota_security_error_t;

/* ============================================================================
 * 签名/哈希类型
 * ============================================================================ */
typedef enum {
    OTA_SEC_SIGN_NONE = 0,
    OTA_SEC_SIGN_ECDSA_P256_SHA256,
    OTA_SEC_SIGN_ECDSA_P384_SHA384,
    OTA_SEC_SIGN_RSA_PKCS1_SHA256,
    OTA_SEC_SIGN_RSA_PSS_SHA256,
    OTA_SEC_SIGN_ED25519
} ota_signature_type_t;

typedef enum {
    OTA_SEC_HASH_SHA256 = 0,
    OTA_SEC_HASH_SHA384,
    OTA_SEC_HASH_SHA512
} ota_hash_type_t;

/* ============================================================================
 * 固件头部结构
 * ============================================================================ */
typedef struct {
    uint32_t magic;
    uint32_t header_version;
    uint32_t header_size;
    uint32_t firmware_version;
    uint32_t hardware_version;
    uint32_t firmware_size;
    uint32_t ecu_id;
    ota_signature_type_t sign_type;
    ota_hash_type_t hash_type;
    uint8_t  hash[OTA_SEC_MAX_HASH_SIZE];
    uint32_t hash_len;
    uint8_t  signature[OTA_SEC_MAX_SIGNATURE_SIZE];
    uint32_t signature_len;
    uint32_t cert_chain_offset;
    uint32_t cert_chain_size;
    uint32_t security_flags;
    uint32_t min_bootloader_version;
    uint64_t build_timestamp;
    uint8_t  build_id[16];
    uint32_t header_crc32;
} ota_firmware_header_t;

/* ============================================================================
 * 版本比较
 * ============================================================================ */
typedef enum {
    OTA_VERSION_EQUAL = 0,
    OTA_VERSION_OLDER = -1,
    OTA_VERSION_NEWER = 1
} ota_version_compare_t;

/* ============================================================================
 * 回滚防护
 * ============================================================================ */
typedef struct {
    uint32_t min_allowed_version;
    uint32_t current_version;
    uint32_t previous_version;
    uint32_t boot_attempts;
    uint32_t max_boot_attempts;
    bool     rollback_detected;
    bool     version_locked;
    uint64_t version_lock_timestamp;
} ota_rollback_protection_t;

/* ============================================================================
 * 指纹结构
 * ============================================================================ */
typedef struct {
    ota_hash_type_t hash_type;
    uint8_t fingerprint[OTA_SEC_MAX_HASH_SIZE];
    uint32_t fingerprint_len;
    uint64_t timestamp;
} ota_fingerprint_t;

/* ============================================================================
 * 证书结构
 * ============================================================================ */
typedef struct {
    uint8_t  *data;
    uint32_t size;
    uint8_t  issuer[128];
    uint8_t  subject[128];
    uint64_t valid_from;
    uint64_t valid_until;
    uint8_t  public_key[64];
    uint8_t  public_key_len;
    ota_signature_type_t sign_type;
    uint8_t  signature[OTA_SEC_MAX_SIGNATURE_SIZE];
    int8_t   parent_index;
    bool     is_self_signed;
    bool     is_ca;
} ota_certificate_t;

typedef struct {
    ota_certificate_t certs[OTA_SEC_MAX_CERT_CHAIN_DEPTH];
    uint8_t num_certs;
    uint8_t root_cert_index;
} ota_cert_chain_t;

/* ============================================================================
 * 安全配置
 * ============================================================================ */
typedef struct {
    bool verify_signature;
    bool verify_hash;
    bool verify_version;
    bool verify_cert_chain;
    bool verify_cert_validity;
    uint32_t max_boot_attempts;
    uint32_t min_firmware_version;
    uint8_t root_ca_key_slot;
    uint8_t oem_key_slot;
    uint32_t verification_timeout_ms;
    void (*on_verification_progress)(ota_security_state_t state, uint8_t progress);
    void (*on_verification_complete)(ota_security_error_t result);
} ota_security_config_t;

/* ============================================================================
 * 验证上下文
 * ============================================================================ */
typedef struct {
    ota_security_config_t config;
    ota_security_state_t  state;
    ota_security_error_t  last_error;
    csm_context_t *csm_ctx;
    keym_context_t *keym_ctx;
    ota_firmware_header_t current_header;
    ota_rollback_protection_t rollback_info;
    uint32_t total_verifications;
    uint32_t failed_verifications;
    uint32_t rollback_attempts_blocked;
    void (*dem_report_dtc)(uint32_t dtc_code, uint8_t status);
    bool initialized;
} ota_security_context_t;

/* ============================================================================
 * API声明
 * ============================================================================ */
ota_security_error_t ota_security_init(ota_security_context_t *ctx, const ota_security_config_t *config, csm_context_t *csm_ctx, keym_context_t *keym_ctx);
void ota_security_deinit(ota_security_context_t *ctx);

ota_security_error_t ota_security_parse_header(ota_security_context_t *ctx, const uint8_t *header_data, uint32_t header_size, ota_firmware_header_t *header);
ota_security_error_t ota_security_verify_header_crc(ota_security_context_t *ctx, const ota_firmware_header_t *header);
ota_security_error_t ota_security_verify_signature(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, const uint8_t *signature, uint32_t signature_len, ota_signature_type_t sign_type);
ota_security_error_t ota_security_verify_hash(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, const uint8_t *expected_hash, uint32_t hash_len, ota_hash_type_t hash_type);
ota_security_error_t ota_security_calculate_hash(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, uint8_t *hash, uint32_t *hash_len, ota_hash_type_t hash_type);

ota_security_error_t ota_security_calculate_fingerprint(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, ota_fingerprint_t *fingerprint);
ota_security_error_t ota_security_verify_fingerprint(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, const ota_fingerprint_t *expected_fingerprint);

ota_version_compare_t ota_security_compare_versions(uint32_t version1, uint32_t version2);
ota_security_error_t ota_security_check_rollback(ota_security_context_t *ctx, uint32_t new_version, uint32_t current_version);
ota_security_error_t ota_security_update_rollback_info(ota_security_context_t *ctx, uint32_t new_version);

ota_security_error_t ota_security_parse_cert_chain(ota_security_context_t *ctx, const uint8_t *cert_data, uint32_t cert_size, ota_cert_chain_t *chain);
ota_security_error_t ota_security_verify_cert_chain(ota_security_context_t *ctx, const ota_cert_chain_t *chain, const uint8_t *trusted_root_key, uint32_t root_key_len);

ota_security_error_t ota_security_verify_full(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, uint32_t current_version);
ota_security_error_t ota_security_verify_fast(ota_security_context_t *ctx, const uint8_t *firmware_data, uint32_t firmware_size, const uint8_t *expected_hash, uint32_t hash_len, const uint8_t *signature, uint32_t signature_len);

ota_security_error_t ota_security_record_boot_attempt(ota_security_context_t *ctx, bool success);
ota_security_error_t ota_security_check_need_rollback(ota_security_context_t *ctx, bool *need_rollback);

ota_security_state_t ota_security_get_state(const ota_security_context_t *ctx);
ota_security_error_t ota_security_get_last_error(const ota_security_context_t *ctx);
void ota_security_set_dem_callback(ota_security_context_t *ctx, void (*report_func)(uint32_t dtc_code, uint8_t status));
int ota_security_format_version(uint32_t version, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* OTA_SECURITY_H */

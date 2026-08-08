/**
 * @file keym_core.c
 * @brief KeyM (Key Manager) Core Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Implementation of AUTOSAR KeyM 4.4 specification
 * Key slot management, key derivation, and key rotation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "keym_core.h"
#include <mbedtls/sha256.h>

#define KEYM_VERSION "4.4.0-AUTOSAR"

/* ============================================================================
 * 静态存储 (ISO 26262 / AUTOSAR R21-11 BSW 禁止动态内存)
 * ============================================================================ */

/* 单例上下文: 编译期静态分配, 替代 calloc */
static keym_context_t s_keym_ctx;

/* 密钥材料存储: slots x MAX_KEY_MATERIAL_SIZE */
static uint8_t s_keym_key_storage[KEYM_MAX_KEY_SLOTS][KEYM_MAX_KEY_MATERIAL_SIZE];

/* 证书存储: certificates x MAX_CERT_SIZE (固定上限, 越界检查) */
#define KEYM_MAX_CERT_SIZE 4096U
static uint8_t s_keym_cert_storage[KEYM_MAX_CERTIFICATES][KEYM_MAX_CERT_SIZE];

/* ============================================================================
 * 密码原语常量 (mbedTLS 软件后端, 无动态分配)
 * ============================================================================ */

#define KEYM_SHA256_DIGEST_LEN   32U
#define KEYM_HMAC_BLOCK_LEN      64U
#define KEYM_CRC32_POLY          0xEDB88320UL   /* IEEE 802.3 反射多项式 */
#define KEYM_CRC32_INIT          0xFFFFFFFFUL

/* KDF 拼接暂存区 (counter||label||0x00||context||L 或 T(i-1)||info||[i]) */
#define KEYM_KDF_SCRATCH_SIZE    512U
static uint8_t s_keym_kdf_scratch[KEYM_KDF_SCRATCH_SIZE];

/* CRC-32 查找表 (惰性初始化一次, 单线程确定性) */
static uint32_t s_keym_crc32_table[256];
static bool      s_keym_crc32_table_ready = false;

/* ============================================================================
 * Internal Function Declarations
 * ============================================================================ */

static keym_status_t keym_validate_slot(keym_context_t *ctx, uint8_t slot_id);
static void keym_update_version_history(keym_context_t *ctx, uint8_t slot_id);
static const char* keym_get_key_type_name_internal(keym_key_type_t type);
static const char* keym_get_key_state_name_internal(keym_key_state_t state);
static uint32_t keym_get_key_type_size(keym_key_type_t type);
static bool keym_is_key_usage_allowed(keym_slot_info_t *slot, keym_key_usage_t usage);

/* ============================================================================
 * 密码原语实现 (真实计算, 替代原 TODO 假实现)
 * ============================================================================ */

/**
 * @brief HMAC-SHA256 (RFC 2104) — mbedtls_sha256 原语, 无动态分配
 *
 * 与 Csm_Cfg.c 软件后端同风格: 密钥 >64B 先 SHA-256 规整, <=64B 补零;
 * inner = SHA256(ipad || msg), out = SHA256(opad || inner)
 */
void keym_hmac_sha256(const uint8_t *key, uint32_t key_len,
                      const uint8_t *msg, uint32_t msg_len,
                      uint8_t out[KEYM_SHA256_DIGEST_LEN])
{
    uint8_t k[KEYM_HMAC_BLOCK_LEN];
    uint8_t ipad[KEYM_HMAC_BLOCK_LEN];
    uint8_t opad[KEYM_HMAC_BLOCK_LEN];
    uint8_t inner[KEYM_SHA256_DIGEST_LEN];
    uint32_t i;
    mbedtls_sha256_context ctx;

    /* 密钥规整: >64B 先哈希, <=64B 补零 */
    if (key_len > KEYM_HMAC_BLOCK_LEN) {
        mbedtls_sha256(key, key_len, k, 0);
        key_len = KEYM_SHA256_DIGEST_LEN;
    } else if ((key != NULL) && (key_len > 0U)) {
        (void)memcpy(k, key, key_len);
    } else {
        key_len = 0U;
    }
    for (i = key_len; i < KEYM_HMAC_BLOCK_LEN; i++) {
        k[i] = 0U;
    }
    for (i = 0U; i < KEYM_HMAC_BLOCK_LEN; i++) {
        ipad[i] = (uint8_t)(k[i] ^ 0x36U);
        opad[i] = (uint8_t)(k[i] ^ 0x5CU);
    }

    /* inner = SHA256(ipad || msg) */
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, ipad, KEYM_HMAC_BLOCK_LEN);
    if ((msg != NULL) && (msg_len > 0U)) {
        mbedtls_sha256_update(&ctx, msg, msg_len);
    }
    mbedtls_sha256_finish(&ctx, inner);
    mbedtls_sha256_free(&ctx);

    /* out = SHA256(opad || inner) */
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, opad, KEYM_HMAC_BLOCK_LEN);
    mbedtls_sha256_update(&ctx, inner, KEYM_SHA256_DIGEST_LEN);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
}

/**
 * @brief NIST SP 800-108 KDF (counter mode, PRF=HMAC-SHA256)
 *
 * K(i) = HMAC-SHA256(KI, [i]_32 || Label || 0x00 || Context || [L]_32)
 * OKM = K(1) || K(2) || ... (32 位大端 counter/length, L 单位 bit)
 */
keym_status_t keym_sp800_108_counter(const uint8_t *ki, uint32_t ki_len,
                                     const uint8_t *label, uint32_t label_len,
                                     const uint8_t *context, uint32_t context_len,
                                     uint8_t *okm, uint32_t okm_len)
{
    uint32_t fixed_len;
    uint32_t produced;
    uint32_t counter;
    uint8_t block[KEYM_SHA256_DIGEST_LEN];

    if ((ki == NULL) || (ki_len == 0U)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if (((label == NULL) && (label_len != 0U)) ||
        ((context == NULL) && (context_len != 0U))) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if ((okm == NULL) && (okm_len != 0U)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if ((okm_len == 0U) || (okm_len > KEYM_MAX_KEY_MATERIAL_SIZE)) {
        return KEYM_ERROR_INVALID_PARAM;
    }

    /* 单块消息 = 4B counter + label + 0x00 + context + 4B L */
    fixed_len = label_len + 1U + context_len + 4U;
    if ((fixed_len + 4U) > KEYM_KDF_SCRATCH_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }

    produced = 0U;
    counter = 1U;
    while (produced < okm_len) {
        uint32_t off = 0U;
        uint32_t copy_len;
        uint32_t l_bits;

        /* [i]_32 大端 */
        s_keym_kdf_scratch[off]     = (uint8_t)(counter >> 24U);
        s_keym_kdf_scratch[off + 1U] = (uint8_t)(counter >> 16U);
        s_keym_kdf_scratch[off + 2U] = (uint8_t)(counter >> 8U);
        s_keym_kdf_scratch[off + 3U] = (uint8_t)counter;
        off += 4U;

        if (label_len > 0U) {
            (void)memcpy(&s_keym_kdf_scratch[off], label, label_len);
            off += label_len;
        }
        s_keym_kdf_scratch[off] = 0x00U;   /* 固定分隔符 */
        off += 1U;
        if (context_len > 0U) {
            (void)memcpy(&s_keym_kdf_scratch[off], context, context_len);
            off += context_len;
        }

        /* [L]_32 大端, 单位 bit (okm_len <= 512 -> 无溢出) */
        l_bits = okm_len * 8U;
        s_keym_kdf_scratch[off]     = (uint8_t)(l_bits >> 24U);
        s_keym_kdf_scratch[off + 1U] = (uint8_t)(l_bits >> 16U);
        s_keym_kdf_scratch[off + 2U] = (uint8_t)(l_bits >> 8U);
        s_keym_kdf_scratch[off + 3U] = (uint8_t)l_bits;
        off += 4U;

        keym_hmac_sha256(ki, ki_len, s_keym_kdf_scratch, off, block);

        copy_len = ((okm_len - produced) < KEYM_SHA256_DIGEST_LEN) ?
                   (okm_len - produced) : KEYM_SHA256_DIGEST_LEN;
        (void)memcpy(&okm[produced], block, copy_len);
        produced += copy_len;
        counter++;
    }

    return KEYM_OK;
}

/**
 * @brief HKDF-SHA256 (RFC 5869 Extract+Expand)
 *
 * Extract: PRK = HMAC-SHA256(salt, IKM)  (salt 缺省为 32 字节零)
 * Expand : T(i) = HMAC-SHA256(PRK, T(i-1) || info || [i]_8), OKM = T(1)||T(2)||...
 */
keym_status_t keym_hkdf_sha256(const uint8_t *ikm, uint32_t ikm_len,
                               const uint8_t *salt, uint32_t salt_len,
                               const uint8_t *info, uint32_t info_len,
                               uint8_t *okm, uint32_t okm_len)
{
    static const uint8_t zero_salt[KEYM_SHA256_DIGEST_LEN] = {0};
    uint8_t prk[KEYM_SHA256_DIGEST_LEN];
    uint8_t prev[KEYM_SHA256_DIGEST_LEN];
    uint8_t block[KEYM_SHA256_DIGEST_LEN];
    uint8_t counter;
    uint32_t produced;

    if ((ikm == NULL) || (ikm_len == 0U)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if (((salt == NULL) && (salt_len != 0U)) ||
        ((info == NULL) && (info_len != 0U))) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if ((okm == NULL) && (okm_len != 0U)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if ((okm_len == 0U) || (okm_len > KEYM_MAX_KEY_MATERIAL_SIZE)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    if ((info_len + KEYM_SHA256_DIGEST_LEN + 1U) > KEYM_KDF_SCRATCH_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }

    /* Extract */
    keym_hmac_sha256((salt != NULL) ? salt : zero_salt,
                     (salt != NULL) ? salt_len : KEYM_SHA256_DIGEST_LEN,
                     ikm, ikm_len, prk);

    /* Expand: T(0) = 空串, T(i) = HMAC(PRK, T(i-1) || info || [i]_8) */
    (void)memset(prev, 0, sizeof(prev));
    produced = 0U;
    counter = 1U;
    while (produced < okm_len) {
        uint32_t off = 0U;
        uint32_t copy_len;

        if (produced > 0U) {
            (void)memcpy(&s_keym_kdf_scratch[off], prev, KEYM_SHA256_DIGEST_LEN);
            off += KEYM_SHA256_DIGEST_LEN;
        }
        if (info_len > 0U) {
            (void)memcpy(&s_keym_kdf_scratch[off], info, info_len);
            off += info_len;
        }
        s_keym_kdf_scratch[off] = counter;   /* 单字节 counter */
        off += 1U;

        keym_hmac_sha256(prk, KEYM_SHA256_DIGEST_LEN,
                         s_keym_kdf_scratch, off, block);

        copy_len = ((okm_len - produced) < KEYM_SHA256_DIGEST_LEN) ?
                   (okm_len - produced) : KEYM_SHA256_DIGEST_LEN;
        (void)memcpy(&okm[produced], block, copy_len);
        (void)memcpy(prev, block, KEYM_SHA256_DIGEST_LEN);
        produced += copy_len;
        counter++;
        if (counter == 0U) {
            return KEYM_ERROR_INVALID_PARAM;   /* >255 块 (超出 okm_len 上限) */
        }
    }

    return KEYM_OK;
}

/**
 * @brief CRC-32 (IEEE 802.3, 多项式 0xEDB88320 反射式, init/xorout 0xFFFFFFFF)
 */
uint32_t keym_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc;
    uint32_t i;

    if (!s_keym_crc32_table_ready) {
        for (i = 0U; i < 256U; i++) {
            uint32_t c = i;
            uint32_t j;
            for (j = 0U; j < 8U; j++) {
                c = ((c & 1U) != 0U) ? ((c >> 1U) ^ KEYM_CRC32_POLY) : (c >> 1U);
            }
            s_keym_crc32_table[i] = c;
        }
        s_keym_crc32_table_ready = true;
    }

    crc = KEYM_CRC32_INIT;
    if (data != NULL) {
        for (i = 0U; i < len; i++) {
            crc = s_keym_crc32_table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
        }
    }

    return crc ^ KEYM_CRC32_INIT;
}

/* ============================================================================
 * Initialization/Deinitialization
 * ============================================================================ */

keym_context_t* keym_init(void *cryif, void *csm)
{
    keym_context_t *ctx = &s_keym_ctx;
    
    /* 静态上下文: 清零后重新初始化 (替代 calloc + 失败返回 NULL) */
    (void)memset(ctx, 0, sizeof(keym_context_t));
    
    /* 将密钥/证书存储指针指向静态池 (固定大小, 编译期确定) */
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_KEY_SLOTS; i++) {
        ctx->materials[i].key_data = s_keym_key_storage[i];
    }
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_CERTIFICATES; i++) {
        ctx->certificates[i].cert_data = s_keym_cert_storage[i];
    }
    
    /* Initialize key slots */
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_KEY_SLOTS; i++) {
        ctx->slots[i].slot_id = i;
        ctx->slots[i].state = KEYM_STATE_EMPTY;
        ctx->slots[i].key_version = 0;
        ctx->slots[i].key_generation = 0;
        ctx->slots[i].parent_slot_id = KEYM_SLOT_ID_INVALID;
        ctx->slots[i].cryif_slot_id = CRYIF_KEY_SLOT_INVALID;
    }
    
    /* Initialize certificates */
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_CERTIFICATES; i++) {
        ctx->certificates[i].cert_id = i;
        ctx->certificates[i].is_revoked = false;
    }
    
    /* Set default rotation policy */
    ctx->rotation_policy.rotation_interval_ms = KEYM_ROTATION_INTERVAL_MS;
    ctx->rotation_policy.max_key_age_ms = KEYM_MAX_KEY_AGE_MS;
    ctx->rotation_policy.overlap_period_ms = 3600000; /* 1 hour */
    ctx->rotation_policy.auto_rotation = false;
    ctx->rotation_policy.keep_old_versions = true;
    ctx->rotation_policy.max_old_versions = 3;
    
    /* Set interfaces */
    ctx->cryif = cryif;
    ctx->csm = csm;
    ctx->dds_sec = NULL;
    
    /* Initialize statistics */
    ctx->total_key_operations = 0;
    ctx->total_derivations = 0;
    ctx->total_rotations = 0;
    
    ctx->initialized = true;
    
    return ctx;
}

void keym_deinit(keym_context_t *ctx)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return;
    }
    
    /* Clear all key slots */
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_KEY_SLOTS; i++) {
        if (ctx->slots[i].state != KEYM_STATE_EMPTY) {
            keym_slot_free(ctx, i);
        }
    }
    
    /* 静态证书存储无需释放, 只擦除内容 (替代 free) */
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_CERTIFICATES; i++) {
        if (ctx->certificates[i].cert_data != NULL) {
            (void)memset(ctx->certificates[i].cert_data, 0, KEYM_MAX_CERT_SIZE);
        }
    }
    
    ctx->initialized = false;
    /* 静态上下文: 不再 free(ctx) */
}

/* ============================================================================
 * Key Slot Management
 * ============================================================================ */

keym_status_t keym_slot_allocate(keym_context_t *ctx, uint8_t *slot_id,
                                 const char *name, keym_key_type_t key_type)
{
    if ((ctx == NULL) || !ctx->initialized || (slot_id == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* If specific slot requested */
    if (*slot_id != KEYM_SLOT_ID_INVALID) {
        if (*slot_id >= KEYM_MAX_KEY_SLOTS) {
            return KEYM_ERROR_INVALID_PARAM;
        }
        if (ctx->slots[*slot_id].state != KEYM_STATE_EMPTY) {
            return KEYM_ERROR_SLOT_OCCUPIED;
        }
    } else {
        /* Find free slot */
        for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_KEY_SLOTS; i++) {
            if (ctx->slots[i].state == KEYM_STATE_EMPTY) {
                *slot_id = i;
                break;
            }
        }
        if (*slot_id == KEYM_SLOT_ID_INVALID) {
            return KEYM_ERROR_NO_MEMORY;
        }
    }
    
    /* Initialize slot */
    keym_slot_info_t *slot = &ctx->slots[*slot_id];
    slot->state = KEYM_STATE_INACTIVE;
    slot->key_type = key_type;
    slot->key_len = keym_get_key_type_size(key_type);
    
    if (name != NULL) {
        strncpy(slot->name, name, KEYM_MAX_KEY_NAME_LEN - 1);
        slot->name[KEYM_MAX_KEY_NAME_LEN - 1] = '\0';
    }
    
    /* Allocate CryIf key slot if available */
    if (ctx->cryif != NULL) {
        /* This would call cryif_key_slot_allocate */
        slot->cryif_slot_id = *slot_id;  /* Simplified mapping */
    }
    
    return KEYM_OK;
}

keym_status_t keym_slot_free(keym_context_t *ctx, uint8_t slot_id)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Erase key material */
    memset(&ctx->materials[slot_id], 0, sizeof(keym_key_material_t));
    
    /* Free CryIf slot if allocated */
    if ((ctx->slots[slot_id].cryif_slot_id != CRYIF_KEY_SLOT_INVALID) &&
        (ctx->cryif != NULL)) {
        /* cryif_key_slot_free(ctx->cryif, ctx->slots[slot_id].cryif_slot_id); */
    }
    
    /* Clear slot info */
    memset(&ctx->slots[slot_id], 0, sizeof(keym_slot_info_t));
    ctx->slots[slot_id].slot_id = slot_id;
    ctx->slots[slot_id].state = KEYM_STATE_EMPTY;
    ctx->slots[slot_id].parent_slot_id = KEYM_SLOT_ID_INVALID;
    ctx->slots[slot_id].cryif_slot_id = CRYIF_KEY_SLOT_INVALID;
    
    return KEYM_OK;
}

keym_status_t keym_slot_get_info(keym_context_t *ctx, uint8_t slot_id,
                                 keym_slot_info_t *info)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (info == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    memcpy(info, &ctx->slots[slot_id], sizeof(keym_slot_info_t));
    return KEYM_OK;
}

uint8_t keym_slot_find_by_name(keym_context_t *ctx, const char *name)
{
    if ((ctx == NULL) || !ctx->initialized || (name == NULL)) {
        return KEYM_SLOT_ID_INVALID;
    }
    
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_KEY_SLOTS; i++) {
        if ((ctx->slots[i].state != KEYM_STATE_EMPTY) &&
            (strcmp(ctx->slots[i].name, name) == 0)) {
            return i;
        }
    }
    
    return KEYM_SLOT_ID_INVALID;
}

keym_status_t keym_slot_set_attributes(keym_context_t *ctx, uint8_t slot_id,
                                       keym_key_usage_t usage_flags,
                                       bool persistent, bool exportable)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    ctx->slots[slot_id].usage_flags = usage_flags;
    ctx->slots[slot_id].is_persistent = persistent;
    ctx->slots[slot_id].is_exportable = exportable;
    
    return KEYM_OK;
}

/* ============================================================================
 * Key Import/Export
 * ============================================================================ */

keym_status_t keym_key_import(keym_context_t *ctx, uint8_t slot_id,
                              const uint8_t *key_data, uint32_t key_len,
                              uint32_t key_version)
{
    keym_status_t status;
    keym_key_material_t *material;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if ((key_data == NULL) || (key_len == 0U)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Check key length */
    if (key_len > KEYM_MAX_KEY_MATERIAL_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Update version history */
    if (ctx->slots[slot_id].key_version > 0U) {
        keym_update_version_history(ctx, slot_id);
    }
    
    /* Store key material — 静态池已预指向, 直接使用 */
    material = &ctx->materials[slot_id];
    if (material->key_data == NULL) {
        material->key_data = s_keym_key_storage[slot_id];
    }
    
    memcpy(material->key_data, key_data, key_len);
    material->key_data_len = key_len;
    material->crc32 = keym_crc32(key_data, key_len);  /* IEEE 802.3 真实校验 */
    
    /* Update slot info */
    ctx->slots[slot_id].key_version = key_version;
    ctx->slots[slot_id].key_len = key_len;
    ctx->slots[slot_id].is_imported = true;
    ctx->slots[slot_id].created_time = 0;  /* TODO: Get current time */
    
    ctx->total_key_operations++;
    
    return KEYM_OK;
}

keym_status_t keym_key_export(keym_context_t *ctx, uint8_t slot_id,
                              uint8_t *key_data, uint32_t *key_len)
{
    keym_status_t status;
    keym_key_material_t *material;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if ((key_data == NULL) || (key_len == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Check if key is exportable */
    if (!ctx->slots[slot_id].is_exportable) {
        return KEYM_ERROR_PERMISSION_DENIED;
    }
    
    material = &ctx->materials[slot_id];
    if ((material->key_data == NULL) || (material->key_data_len == 0U)) {
        return KEYM_ERROR_KEY_NOT_FOUND;
    }
    
    /* Check buffer size */
    if (*key_len < material->key_data_len) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    memcpy(key_data, material->key_data, material->key_data_len);
    *key_len = material->key_data_len;
    
    return KEYM_OK;
}

keym_status_t keym_key_generate(keym_context_t *ctx, uint8_t slot_id,
                                keym_key_type_t key_type)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Update slot type if different */
    if (ctx->slots[slot_id].key_type != key_type) {
        ctx->slots[slot_id].key_type = key_type;
        ctx->slots[slot_id].key_len = keym_get_key_type_size(key_type);
    }
    
    /* Allocate key material buffer — 静态池已预指向 */
    if (ctx->materials[slot_id].key_data == NULL) {
        ctx->materials[slot_id].key_data = s_keym_key_storage[slot_id];
    }
    
    /* Generate random key */
    if (ctx->cryif != NULL) {
        /* Use CryIf for hardware random generation */
        /* cryif_random_generate(ctx->cryif, ...); */
    }
    
    /* For now, fill with pseudo-random pattern */
    for (uint32_t i = 0; i < ctx->slots[slot_id].key_len; i++) {
        ctx->materials[slot_id].key_data[i] = (uint8_t)((i * 7U) + (slot_id * 13U));
    }
    ctx->materials[slot_id].key_data_len = ctx->slots[slot_id].key_len;
    
    /* Update slot info */
    ctx->slots[slot_id].key_version = 1;
    ctx->slots[slot_id].key_generation++;
    ctx->slots[slot_id].created_time = 0;  /* TODO: Get current time */
    ctx->slots[slot_id].next_rotation_time = ctx->slots[slot_id].created_time +
                                              ctx->rotation_policy.rotation_interval_ms;
    
    ctx->total_key_operations++;
    
    return KEYM_OK;
}

keym_status_t keym_key_activate(keym_context_t *ctx, uint8_t slot_id)
{
    keym_status_t status;
    keym_key_state_t old_state;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (ctx->slots[slot_id].state != KEYM_STATE_INACTIVE) {
        return KEYM_ERROR_KEY_INVALID;
    }
    
    old_state = ctx->slots[slot_id].state;
    ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
    ctx->slots[slot_id].activated_time = 0;  /* TODO: Get current time */
    
    /* Trigger state change callback */
    if (ctx->on_state_change != NULL) {
        ctx->on_state_change(slot_id, old_state, KEYM_STATE_ACTIVE, ctx->callback_user_data);
    }
    
    return KEYM_OK;
}

keym_status_t keym_key_revoke(keym_context_t *ctx, uint8_t slot_id)
{
    keym_status_t status;
    keym_key_state_t old_state;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    old_state = ctx->slots[slot_id].state;
    ctx->slots[slot_id].state = KEYM_STATE_REVOKED;
    
    /* Trigger state change callback */
    if (ctx->on_state_change != NULL) {
        ctx->on_state_change(slot_id, old_state, KEYM_STATE_REVOKED, ctx->callback_user_data);
    }
    
    return KEYM_OK;
}

/* ============================================================================
 * Key Derivation (KDF)
 * ============================================================================ */

keym_status_t keym_key_derive(keym_context_t *ctx,
                              const keym_derivation_params_t *params,
                              uint8_t *derived_slot_id)
{
    keym_status_t status;
    
    if ((ctx == NULL) || !ctx->initialized || (params == NULL) || (derived_slot_id == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Validate parent slot */
    status = keym_validate_slot(ctx, params->parent_slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Perform key derivation based on KDF type
     * (目标槽分配在各自 KDF 路径内完成, 避免重复分配) */
    switch (params->kdf_type) {
        case KEYM_KDF_HKDF_SHA256: {
            uint8_t target_slot;
            
            /* Allocate target slot if needed */
            if (params->target_slot_id == KEYM_SLOT_ID_INVALID) {
                target_slot = KEYM_SLOT_ID_INVALID;
                status = keym_slot_allocate(ctx, &target_slot, "derived_key",
                                            params->derived_key_type);
                if (status != KEYM_OK) {
                    return status;
                }
                *derived_slot_id = target_slot;
            } else {
                target_slot = params->target_slot_id;
                status = keym_validate_slot(ctx, target_slot);
                if (status != KEYM_OK) {
                    return status;
                }
                *derived_slot_id = target_slot;
            }
            
            status = keym_hkdf_derive(ctx, params->parent_slot_id, target_slot,
                                      NULL, 0,
                                      params->context, params->context_len,
                                      params->derived_key_len);
            if (status == KEYM_OK) {
                /* Update derived key info */
                ctx->slots[target_slot].parent_slot_id = params->parent_slot_id;
                ctx->slots[target_slot].kdf_type = params->kdf_type;
                ctx->total_derivations++;
            }
            break;
        }

        case KEYM_KDF_NIST_SP800_108:
            /* NIST SP 800-108 counter-mode KDF (真实实现, PRF=HMAC-SHA256)
             * 含目标槽分配/簿记 */
            status = keym_sp800_108_derive(ctx, params, derived_slot_id);
            break;

        default:
            status = KEYM_ERROR_INVALID_PARAM;
            break;
    }

    return status;
}

/**
 * @brief NIST SP 800-108 派生 (public): 校验父槽 -> 分配/校验目标槽 -> 执行 KDF -> 簿记
 */
keym_status_t keym_sp800_108_derive(keym_context_t *ctx,
                                    const keym_derivation_params_t *params,
                                    uint8_t *derived_slot_id)
{
    keym_status_t status;
    uint8_t target_slot;
    keym_key_material_t *parent_mat;
    keym_key_material_t *target_mat;

    if ((ctx == NULL) || !ctx->initialized || (params == NULL) ||
        (derived_slot_id == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }

    /* Validate parent slot and key material */
    status = keym_validate_slot(ctx, params->parent_slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    parent_mat = &ctx->materials[params->parent_slot_id];
    if ((parent_mat->key_data == NULL) || (parent_mat->key_data_len == 0U)) {
        return KEYM_ERROR_KEY_NOT_FOUND;
    }

    /* Allocate target slot if needed */
    if (params->target_slot_id == KEYM_SLOT_ID_INVALID) {
        target_slot = KEYM_SLOT_ID_INVALID;
        status = keym_slot_allocate(ctx, &target_slot, "derived_key",
                                    params->derived_key_type);
        if (status != KEYM_OK) {
            return status;
        }
        *derived_slot_id = target_slot;
    } else {
        target_slot = params->target_slot_id;
        status = keym_validate_slot(ctx, target_slot);
        if (status != KEYM_OK) {
            return status;
        }
        *derived_slot_id = target_slot;
    }

    /* Target key material -> 静态池 */
    target_mat = &ctx->materials[target_slot];
    if (target_mat->key_data == NULL) {
        target_mat->key_data = s_keym_key_storage[target_slot];
    }
    if (target_mat->key_data == NULL) {
        return KEYM_ERROR_NO_MEMORY;
    }

    /* NIST SP 800-108 counter mode */
    status = keym_sp800_108_counter(parent_mat->key_data, parent_mat->key_data_len,
                                    params->label, params->label_len,
                                    params->context, params->context_len,
                                    target_mat->key_data, params->derived_key_len);
    if (status != KEYM_OK) {
        return status;
    }
    target_mat->key_data_len = params->derived_key_len;
    target_mat->crc32 = keym_crc32(target_mat->key_data, target_mat->key_data_len);

    /* Update derived key info */
    ctx->slots[target_slot].parent_slot_id = params->parent_slot_id;
    ctx->slots[target_slot].kdf_type = params->kdf_type;
    ctx->slots[target_slot].key_len = params->derived_key_len;
    ctx->total_derivations++;

    return KEYM_OK;
}

keym_status_t keym_hkdf_derive(keym_context_t *ctx, uint8_t parent_slot,
                               uint8_t target_slot,
                               const uint8_t *salt, uint32_t salt_len,
                               const uint8_t *info, uint32_t info_len,
                               uint32_t key_len)
{
    keym_status_t status;
    keym_key_material_t *parent_mat;
    keym_key_material_t *target_mat;

    /* RFC 5869 HKDF-SHA256 真实实现 (替代原假实现) */
    status = keym_validate_slot(ctx, parent_slot);
    if (status != KEYM_OK) {
        return status;
    }
    status = keym_validate_slot(ctx, target_slot);
    if (status != KEYM_OK) {
        return status;
    }
    if (key_len > KEYM_MAX_KEY_MATERIAL_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }

    parent_mat = &ctx->materials[parent_slot];
    if ((parent_mat->key_data == NULL) || (parent_mat->key_data_len == 0U)) {
        return KEYM_ERROR_KEY_NOT_FOUND;
    }

    /* Allocate key material for derived key — 静态池已预指向 */
    target_mat = &ctx->materials[target_slot];
    if (target_mat->key_data == NULL) {
        target_mat->key_data = s_keym_key_storage[target_slot];
    }
    if (target_mat->key_data == NULL) {
        return KEYM_ERROR_NO_MEMORY;
    }

    status = keym_hkdf_sha256(parent_mat->key_data, parent_mat->key_data_len,
                              salt, salt_len, info, info_len,
                              target_mat->key_data, key_len);
    if (status != KEYM_OK) {
        return status;
    }
    target_mat->key_data_len = key_len;
    target_mat->crc32 = keym_crc32(target_mat->key_data, key_len);

    return KEYM_OK;
}

/* ============================================================================
 * Key Rotation
 * ============================================================================ */

keym_status_t keym_set_rotation_policy(keym_context_t *ctx,
                                       const keym_rotation_policy_t *policy)
{
    if ((ctx == NULL) || !ctx->initialized || (policy == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    memcpy(&ctx->rotation_policy, policy, sizeof(keym_rotation_policy_t));
    return KEYM_OK;
}

keym_status_t keym_rotate_key(keym_context_t *ctx, uint8_t slot_id,
                              uint8_t *new_slot_id)
{
    keym_status_t status;
    uint8_t new_slot;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Mark current key for rotation */
    ctx->slots[slot_id].state = KEYM_STATE_PENDING_ROTATION;
    
    /* Allocate new slot if needed */
    if ((new_slot_id != NULL) && (*new_slot_id == KEYM_SLOT_ID_INVALID)) {
        new_slot = KEYM_SLOT_ID_INVALID;
        status = keym_slot_allocate(ctx, &new_slot,
                                    ctx->slots[slot_id].name,
                                    ctx->slots[slot_id].key_type);
        if (status != KEYM_OK) {
            ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
            return status;
        }
        *new_slot_id = new_slot;
    } else if (new_slot_id != NULL) {
        new_slot = *new_slot_id;
    } else {
        new_slot = slot_id;  /* In-place rotation */
    }
    
    /* Generate new key */
    status = keym_key_generate(ctx, new_slot, ctx->slots[slot_id].key_type);
    if (status != KEYM_OK) {
        if ((new_slot_id != NULL) && (*new_slot_id != slot_id)) {
            keym_slot_free(ctx, new_slot);
        }
        ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
        return status;
    }
    
    /* Activate new key */
    status = keym_key_activate(ctx, new_slot);
    if (status != KEYM_OK) {
        if ((new_slot_id != NULL) && (*new_slot_id != slot_id)) {
            keym_slot_free(ctx, new_slot);
        }
        ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
        return status;
    }
    
    /* Mark old key as rotated */
    ctx->slots[slot_id].state = KEYM_STATE_ROTATED;
    ctx->slots[slot_id].expired_time = 0;  /* TODO: Get current time */
    
    /* Update version history */
    keym_update_version_history(ctx, slot_id);
    
    ctx->total_rotations++;
    
    /* Trigger rotation callback */
    if (ctx->on_rotation != NULL) {
        ctx->on_rotation(new_slot, ctx->slots[new_slot].key_version, ctx->callback_user_data);
    }
    
    return KEYM_OK;
}

uint32_t keym_check_and_rotate(keym_context_t *ctx, uint64_t current_time)
{
    uint32_t rotations = 0;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return 0;
    }
    
    if (!ctx->rotation_policy.auto_rotation) {
        return 0;
    }
    
    for (int i = 0; (unsigned int)((unsigned int)(i)) < KEYM_MAX_KEY_SLOTS; i++) {
        if (ctx->slots[i].state == KEYM_STATE_ACTIVE) {
            /* Check if rotation is needed */
            if ((ctx->slots[i].next_rotation_time <= current_time) ||
                ((current_time - ctx->slots[i].activated_time) >= ctx->rotation_policy.max_key_age_ms)) {
                
                uint8_t new_slot = KEYM_SLOT_ID_INVALID;
                if (keym_rotate_key(ctx, i, &new_slot) == KEYM_OK) {
                    rotations++;
                }
            }
        }
    }
    
    return rotations;
}

keym_status_t keym_get_version_history(keym_context_t *ctx, uint8_t slot_id,
                                       keym_version_history_t *history,
                                       uint32_t max_entries,
                                       uint32_t *num_entries)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if ((history == NULL) || (num_entries == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    uint32_t count = 0;
    for (int i = 0; (i < KEYM_MAX_KEY_VERSIONS) && (count < max_entries); i++) {
        if (ctx->version_history[slot_id][i].version > 0U) {
            memcpy(&history[count], &ctx->version_history[slot_id][i],
                   sizeof(keym_version_history_t));
            count++;
        }
    }
    
    *num_entries = count;
    return KEYM_OK;
}

/* ============================================================================
 * DDS Security Integration
 * ============================================================================ */

keym_status_t keym_integrate_dds_security(keym_context_t *ctx, void *dds_sec)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->dds_sec = dds_sec;
    return KEYM_OK;
}

keym_status_t keym_import_from_dds_cert(keym_context_t *ctx, const char *cert_name,
                                        uint8_t *key_slot)
{
    (void)ctx;
    (void)cert_name;
    (void)key_slot;
    /* DDS 证书密钥导入未实现: 显式 fail-closed (原 TODO 恒 OK 假实现) */
    return KEYM_ERROR_NOT_IMPLEMENTED;
}

keym_status_t keym_export_to_dds_cert(keym_context_t *ctx, uint8_t key_slot,
                                      const char *cert_name)
{
    (void)ctx;
    (void)key_slot;
    (void)cert_name;
    /* DDS 证书密钥导出未实现: 显式 fail-closed (原 TODO 恒 OK 假实现) */
    return KEYM_ERROR_NOT_IMPLEMENTED;
}

keym_status_t keym_register_certificate(keym_context_t *ctx, uint8_t cert_id,
                                        const char *name,
                                        const uint8_t *cert_data, uint32_t cert_len)
{
    if ((ctx == NULL) || !ctx->initialized || (cert_data == NULL) || (cert_len == 0U)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    if (cert_id >= KEYM_MAX_CERTIFICATES) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Free existing certificate data — 静态池: 仅重置内容与长度 */
    if (ctx->certificates[cert_id].cert_data != NULL) {
        (void)memset(ctx->certificates[cert_id].cert_data, 0, KEYM_MAX_CERT_SIZE);
        ctx->certificates[cert_id].cert_data_len = 0U;
    }
    
    /* 越界检查: 证书大小固定上限 */
    if (cert_len > KEYM_MAX_CERT_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* 复制到静态证书存储 (预指向, 无需分配) */
    ctx->certificates[cert_id].cert_data = s_keym_cert_storage[cert_id];
    if (ctx->certificates[cert_id].cert_data == NULL) {
        return KEYM_ERROR_NO_MEMORY;
    }
    
    memcpy(ctx->certificates[cert_id].cert_data, cert_data, cert_len);
    ctx->certificates[cert_id].cert_data_len = cert_len;
    ctx->certificates[cert_id].is_revoked = false;
    
    if (name != NULL) {
        strncpy(ctx->certificates[cert_id].name, name, KEYM_MAX_KEY_NAME_LEN - 1);
        ctx->certificates[cert_id].name[KEYM_MAX_KEY_NAME_LEN - 1] = '\0';
    }
    
    return KEYM_OK;
}

keym_status_t keym_update_certificate(keym_context_t *ctx, uint8_t cert_id,
                                      const uint8_t *cert_data, uint32_t cert_len)
{
    /* Update is same as register */
    return keym_register_certificate(ctx, cert_id, NULL, cert_data, cert_len);
}

keym_status_t keym_revoke_certificate(keym_context_t *ctx, uint8_t cert_id)
{
    if ((ctx == NULL) || !ctx->initialized || (cert_id >= KEYM_MAX_CERTIFICATES)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->certificates[cert_id].is_revoked = true;
    return KEYM_OK;
}

/* ============================================================================
 * SecOC Integration
 * ============================================================================ */

keym_status_t keym_configure_secoc_key(keym_context_t *ctx, uint32_t secoc_pdu_id,
                                       uint8_t *key_slot)
{
    char slot_name[KEYM_MAX_KEY_NAME_LEN];
    keym_status_t status;
    uint8_t slot;
    
    if ((ctx == NULL) || !ctx->initialized || (key_slot == NULL)) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Create slot name based on PDU ID */
    snprintf(slot_name, sizeof(slot_name), "SecOC_PDU_%lu", (unsigned long)secoc_pdu_id);
    
    /* Check if already exists */
    slot = keym_slot_find_by_name(ctx, slot_name);
    if (slot != KEYM_SLOT_ID_INVALID) {
        *key_slot = slot;
        return KEYM_OK;
    }
    
    /* Allocate new slot */
    slot = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &slot, slot_name, KEYM_TYPE_AES_128);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Set SecOC usage */
    keym_slot_set_attributes(ctx, slot, KEYM_USAGE_SECOC, true, false);
    
    *key_slot = slot;
    return KEYM_OK;
}

uint8_t keym_get_secoc_key_slot(keym_context_t *ctx, uint32_t pdu_id)
{
    char slot_name[KEYM_MAX_KEY_NAME_LEN];
    
    if ((ctx == NULL) || !ctx->initialized) {
        return KEYM_SLOT_ID_INVALID;
    }
    
    snprintf(slot_name, sizeof(slot_name), "SecOC_PDU_%lu", (unsigned long)pdu_id);
    return keym_slot_find_by_name(ctx, slot_name);
}

/* ============================================================================
 * Callback Management
 * ============================================================================ */

keym_status_t keym_register_rotation_callback(keym_context_t *ctx,
                                               keym_rotation_callback_t callback,
                                               void *user_data)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->on_rotation = callback;
    ctx->callback_user_data = user_data;
    
    return KEYM_OK;
}

keym_status_t keym_register_state_callback(keym_context_t *ctx,
                                            keym_state_callback_t callback,
                                            void *user_data)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->on_state_change = callback;
    ctx->callback_user_data = user_data;
    
    return KEYM_OK;
}

/* ============================================================================
 * Secure Storage
 * ============================================================================ */

keym_status_t keym_load_persistent_keys(keym_context_t *ctx)
{
    (void)ctx;
    /* 持久化密钥加载未实现 (需 NvM/安全存储集成): 显式 fail-closed */
    return KEYM_ERROR_NOT_IMPLEMENTED;
}

keym_status_t keym_save_persistent_keys(keym_context_t *ctx)
{
    (void)ctx;
    /* 持久化密钥保存未实现 (需 NvM/安全存储集成): 显式 fail-closed */
    return KEYM_ERROR_NOT_IMPLEMENTED;
}

/* ============================================================================
 * Debug and Diagnostics
 * ============================================================================ */

const char* keym_get_key_type_name(keym_key_type_t type)
{
    return keym_get_key_type_name_internal(type);
}

const char* keym_get_key_state_name(keym_key_state_t state)
{
    return keym_get_key_state_name_internal(state);
}

const char* keym_get_version(void)
{
    return KEYM_VERSION;
}

void keym_debug_print_slot(keym_context_t *ctx, uint8_t slot_id)
{
    keym_slot_info_t *slot;
    
    if ((ctx == NULL) || (slot_id >= KEYM_MAX_KEY_SLOTS)) {
        return;
    }
    
    slot = &ctx->slots[slot_id];
    
    printf("Key Slot %d:\n", slot_id);
    printf("  Name: %s\n", slot->name);
    printf("  Type: %s\n", keym_get_key_type_name_internal(slot->key_type));
    printf("  State: %s\n", keym_get_key_state_name_internal(slot->state));
    printf("  Version: %u\n", slot->key_version);
    printf("  Key Length: %u\n", slot->key_len);
    printf("  Persistent: %s\n", slot->is_persistent ? "Yes" : "No");
    printf("  Exportable: %s\n", slot->is_exportable ? "Yes" : "No");
    printf("  Usage: 0x%02X\n", slot->usage_flags);
}

/* ============================================================================
 * Internal Functions
 * ============================================================================ */

static keym_status_t keym_validate_slot(keym_context_t *ctx, uint8_t slot_id)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    if (slot_id >= KEYM_MAX_KEY_SLOTS) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    if (ctx->slots[slot_id].state == KEYM_STATE_EMPTY) {
        return KEYM_ERROR_SLOT_NOT_FOUND;
    }
    
    return KEYM_OK;
}

static void keym_update_version_history(keym_context_t *ctx, uint8_t slot_id)
{
    /* Shift history entries */
    for (int i = KEYM_MAX_KEY_VERSIONS - 1; i > 0; i--) {
        memcpy(&ctx->version_history[slot_id][i],
               &ctx->version_history[slot_id][i - 1],
               sizeof(keym_version_history_t));
    }
    
    /* Add current version to history */
    ctx->version_history[slot_id][0].version = ctx->slots[slot_id].key_version;
    ctx->version_history[slot_id][0].created_time = ctx->slots[slot_id].created_time;
    ctx->version_history[slot_id][0].expired_time = ctx->slots[slot_id].expired_time;
    ctx->version_history[slot_id][0].state = ctx->slots[slot_id].state;
}

static const char* keym_get_key_type_name_internal(keym_key_type_t type)
{
    switch (type) {
        case KEYM_TYPE_AES_128:         return "AES-128";
        case KEYM_TYPE_AES_192:         return "AES-192";
        case KEYM_TYPE_AES_256:         return "AES-256";
        case KEYM_TYPE_HMAC_SHA1:       return "HMAC-SHA1";
        case KEYM_TYPE_HMAC_SHA256:     return "HMAC-SHA256";
        case KEYM_TYPE_HMAC_SHA384:     return "HMAC-SHA384";
        case KEYM_TYPE_HMAC_SHA512:     return "HMAC-SHA512";
        case KEYM_TYPE_RSA_1024:        return "RSA-1024";
        case KEYM_TYPE_RSA_2048:        return "RSA-2048";
        case KEYM_TYPE_RSA_4096:        return "RSA-4096";
        case KEYM_TYPE_ECC_P192:        return "ECC-P192";
        case KEYM_TYPE_ECC_P224:        return "ECC-P224";
        case KEYM_TYPE_ECC_P256:        return "ECC-P256";
        case KEYM_TYPE_ECC_P384:        return "ECC-P384";
        case KEYM_TYPE_ECC_P521:        return "ECC-P521";
        case KEYM_TYPE_DERIVED:         return "DERIVED";
        case KEYM_TYPE_KEY_MATERIAL:    return "KEY_MATERIAL";
        default:                        return "UNKNOWN";
    }
}

static const char* keym_get_key_state_name_internal(keym_key_state_t state)
{
    switch (state) {
        case KEYM_STATE_EMPTY:          return "EMPTY";
        case KEYM_STATE_ACTIVE:         return "ACTIVE";
        case KEYM_STATE_INACTIVE:       return "INACTIVE";
        case KEYM_STATE_EXPIRED:        return "EXPIRED";
        case KEYM_STATE_REVOKED:        return "REVOKED";
        case KEYM_STATE_PENDING_ROTATION: return "PENDING_ROTATION";
        case KEYM_STATE_ROTATED:        return "ROTATED";
        default:                        return "UNKNOWN";
    }
}

static uint32_t keym_get_key_type_size(keym_key_type_t type)
{
    switch (type) {
        case KEYM_TYPE_AES_128:         return 16;
        case KEYM_TYPE_AES_192:         return 24;
        case KEYM_TYPE_AES_256:         return 32;
        case KEYM_TYPE_HMAC_SHA1:       return 20;
        case KEYM_TYPE_HMAC_SHA256:     return 32;
        case KEYM_TYPE_HMAC_SHA384:     return 48;
        case KEYM_TYPE_HMAC_SHA512:     return 64;
        case KEYM_TYPE_RSA_1024:        return 128;
        case KEYM_TYPE_RSA_2048:        return 256;
        case KEYM_TYPE_RSA_4096:        return 512;
        case KEYM_TYPE_ECC_P192:        return 24;
        case KEYM_TYPE_ECC_P224:        return 28;
        case KEYM_TYPE_ECC_P256:        return 32;
        case KEYM_TYPE_ECC_P384:        return 48;
        case KEYM_TYPE_ECC_P521:        return 66;
        default:                        return 32;
    }
}

static bool keym_is_key_usage_allowed(keym_slot_info_t *slot, keym_key_usage_t usage)
{
    if (slot == NULL) { return false; }
    return ((uint32_t)slot->usage_flags & (uint32_t)usage) != 0U;
}

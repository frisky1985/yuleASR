/**
 * @file test_keym.c
 * @brief KeyM (Key Manager) Unit Tests
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../keym/keym_core.h"

/* Test macros */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))
#define TEST_ASSERT_NE(a, b) TEST_ASSERT((a) != (b))

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Test function prototypes */
static int test_keym_init_deinit(void);
static int test_keym_slot_management(void);
static int test_keym_key_import_export(void);
static int test_keym_key_generation(void);
static int test_keym_key_derivation(void);
static int test_keym_key_rotation(void);
static int test_keym_certificate_management(void);
static int test_keym_secoc_integration(void);
static int test_keym_hmac_rfc2104(void);
static int test_keym_sp800_108_vector(void);
static int test_keym_sp800_108_multiblock(void);
static int test_keym_sp800_108_derive_api(void);
static int test_keym_hkdf_rfc5869_tc1(void);
static int test_keym_crc32(void);
static int test_keym_not_implemented(void);

/* ============================================================================
 * Test Implementations
 * ============================================================================ */

static int test_keym_init_deinit(void)
{
    keym_context_t *ctx;
    
    printf("  Testing KeyM init/deinit...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    TEST_ASSERT(ctx->initialized == true);
    TEST_ASSERT(ctx->cryif == NULL);
    TEST_ASSERT(ctx->csm == NULL);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_slot_management(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t slot_id;
    keym_slot_info_t info;
    
    printf("  Testing KeyM slot management...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Allocate slot with auto ID */
    slot_id = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &slot_id, "test_key", KEYM_TYPE_AES_128);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_NE(slot_id, KEYM_SLOT_ID_INVALID);
    
    /* Get slot info */
    status = keym_slot_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.slot_id, slot_id);
    TEST_ASSERT(strcmp(info.name, "test_key") == 0);
    TEST_ASSERT_EQ(info.key_type, KEYM_TYPE_AES_128);
    TEST_ASSERT_EQ(info.state, KEYM_STATE_INACTIVE);
    
    /* Find by name */
    uint8_t found_slot = keym_slot_find_by_name(ctx, "test_key");
    TEST_ASSERT_EQ(found_slot, slot_id);
    
    /* Set attributes */
    status = keym_slot_set_attributes(ctx, slot_id, KEYM_USAGE_SECOC, true, false);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_slot_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.usage_flags, KEYM_USAGE_SECOC);
    TEST_ASSERT(info.is_persistent == true);
    TEST_ASSERT(info.is_exportable == false);
    
    /* Free slot */
    status = keym_slot_free(ctx, slot_id);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_key_import_export(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t slot_id;
    uint8_t key_data[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8_t exported_key[16];
    uint32_t exported_len = sizeof(exported_key);
    
    printf("  Testing KeyM key import/export...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Allocate slot */
    slot_id = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &slot_id, "import_key", KEYM_TYPE_AES_128);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Make exportable */
    status = keym_slot_set_attributes(ctx, slot_id, KEYM_USAGE_STORAGE, false, true);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Import key */
    status = keym_key_import(ctx, slot_id, key_data, sizeof(key_data), 1);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Export key */
    status = keym_key_export(ctx, slot_id, exported_key, &exported_len);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(exported_len, 16);
    TEST_ASSERT(memcmp(key_data, exported_key, 16) == 0);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_key_generation(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t slot_id;
    keym_slot_info_t info;
    
    printf("  Testing KeyM key generation...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Allocate slot */
    slot_id = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &slot_id, "gen_key", KEYM_TYPE_AES_256);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Generate key */
    status = keym_key_generate(ctx, slot_id, KEYM_TYPE_AES_256);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_slot_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.key_version, 1);
    TEST_ASSERT_EQ(info.key_generation, 1);
    
    /* Activate key */
    status = keym_key_activate(ctx, slot_id);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_slot_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.state, KEYM_STATE_ACTIVE);
    
    /* Revoke key */
    status = keym_key_revoke(ctx, slot_id);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_slot_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.state, KEYM_STATE_REVOKED);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_key_derivation(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t parent_slot;
    uint8_t derived_slot;
    keym_derivation_params_t params;
    keym_slot_info_t info;
    
    printf("  Testing KeyM key derivation...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Setup parent key */
    parent_slot = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &parent_slot, "parent_key", KEYM_TYPE_AES_256);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_key_generate(ctx, parent_slot, KEYM_TYPE_AES_256);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Setup derivation parameters */
    uint8_t context[] = "derived_key_context";
    uint8_t label[] = "derived_key_label";
    
    memset(&params, 0, sizeof(params));
    params.parent_slot_id = parent_slot;
    params.target_slot_id = KEYM_SLOT_ID_INVALID;  /* Auto allocate */
    params.kdf_type = KEYM_KDF_HKDF_SHA256;
    params.context = context;
    params.context_len = sizeof(context);
    params.label = label;
    params.label_len = sizeof(label);
    params.derived_key_type = KEYM_TYPE_AES_128;
    params.derived_key_len = 16;
    
    /* Derive key */
    status = keym_key_derive(ctx, &params, &derived_slot);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_NE(derived_slot, KEYM_SLOT_ID_INVALID);
    
    /* Check derived key info */
    status = keym_slot_get_info(ctx, derived_slot, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.parent_slot_id, parent_slot);
    TEST_ASSERT_EQ(info.kdf_type, KEYM_KDF_HKDF_SHA256);
    TEST_ASSERT_EQ(info.key_type, KEYM_TYPE_AES_128);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static void test_rotation_callback(uint8_t slot_id, uint32_t new_version, void *user_data)
{
    (void)slot_id;
    (void)new_version;
    int *count = (int*)user_data;
    (*count)++;
}

static int test_keym_key_rotation(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t slot_id;
    uint8_t new_slot;
    keym_slot_info_t info;
    int rotation_count = 0;
    keym_rotation_policy_t policy = {
        .rotation_interval_ms = 86400000,
        .max_key_age_ms = 604800000,
        .overlap_period_ms = 3600000,
        .auto_rotation = false,
        .keep_old_versions = true,
        .max_old_versions = 3
    };
    
    printf("  Testing KeyM key rotation...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Set rotation policy */
    status = keym_set_rotation_policy(ctx, &policy);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Register callback */
    status = keym_register_rotation_callback(ctx, test_rotation_callback, &rotation_count);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Setup key */
    slot_id = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &slot_id, "rotate_key", KEYM_TYPE_AES_128);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_key_generate(ctx, slot_id, KEYM_TYPE_AES_128);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    status = keym_key_activate(ctx, slot_id);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* Rotate key */
    new_slot = KEYM_SLOT_ID_INVALID;
    status = keym_rotate_key(ctx, slot_id, &new_slot);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_NE(new_slot, KEYM_SLOT_ID_INVALID);
    
    /* Check callback was called */
    TEST_ASSERT_EQ(rotation_count, 1);
    
    /* Check old slot state */
    status = keym_slot_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.state, KEYM_STATE_ROTATED);
    
    /* Check new slot state */
    status = keym_slot_get_info(ctx, new_slot, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.state, KEYM_STATE_ACTIVE);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_certificate_management(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t cert_data[] = "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----";
    
    printf("  Testing KeyM certificate management...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Register certificate */
    status = keym_register_certificate(ctx, 0, "test_cert", cert_data, sizeof(cert_data));
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT(ctx->certificates[0].cert_data != NULL);
    TEST_ASSERT_EQ(ctx->certificates[0].cert_data_len, sizeof(cert_data));
    
    /* Update certificate */
    uint8_t new_cert[] = "-----BEGIN CERTIFICATE-----\n...new...\n-----END CERTIFICATE-----";
    status = keym_update_certificate(ctx, 0, new_cert, sizeof(new_cert));
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(ctx->certificates[0].cert_data_len, sizeof(new_cert));
    
    /* Revoke certificate */
    status = keym_revoke_certificate(ctx, 0);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT(ctx->certificates[0].is_revoked == true);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_hmac_rfc2104(void)
{
    /* RFC 2104 Test Case 1: key=0x0b*20, data="Hi There" */
    uint8_t key[20];
    uint8_t out[32];
    static const uint8_t expected[32] = {
        0xb0,0x34,0x4c,0x61,0xd8,0xdb,0x38,0x53,0x5c,0xa8,0xaf,0xce,0xaf,0x0b,0xf1,0x2b,
        0x88,0x1d,0xc2,0x00,0xc9,0x83,0x3d,0xa7,0x26,0xe9,0x37,0x6c,0x2e,0x32,0xcf,0xf7};
    
    printf("  Testing KeyM HMAC-SHA256 (RFC 2104 TC1)...\n");
    
    memset(key, 0x0b, sizeof(key));
    keym_hmac_sha256(key, sizeof(key), (const uint8_t*)"Hi There", 8U, out);
    TEST_ASSERT(memcmp(out, expected, 32) == 0);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_sp800_108_vector(void)
{
    /* NIST SP 800-108 counter mode 已知向量 (Python hashlib/hmac 对拍):
     * KI=0123456789abcdef0123456789abcdef, label="SP800-108-test",
     * context="counter-mode", L=256bit -> OKM=cfe012ff... */
    static const uint8_t ki[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                                   0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    static const uint8_t label[] = "SP800-108-test";
    static const uint8_t context[] = "counter-mode";
    uint8_t okm[32];
    static const uint8_t expected[32] = {
        0xcf,0xe0,0x12,0xff,0xbe,0x94,0x44,0x3b,0x8c,0x81,0xea,0x2c,0xa7,0x77,0x1e,0xa9,
        0x8c,0xe6,0x3e,0xb8,0x23,0x6a,0xee,0xc8,0x50,0xec,0x19,0x9d,0x15,0x6c,0x81,0x40};
    keym_status_t status;
    
    printf("  Testing KeyM SP 800-108 KDF known vector...\n");
    
    status = keym_sp800_108_counter(ki, sizeof(ki),
                                    label, sizeof(label) - 1U,
                                    context, sizeof(context) - 1U,
                                    okm, sizeof(okm));
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT(memcmp(okm, expected, 32) == 0);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_sp800_108_multiblock(void)
{
    /* L=384bit -> 2 个 PRF 块, 验证 counter 递增与拼接 */
    static const uint8_t ki[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                                   0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    static const uint8_t label[] = "SP800-108-test";
    static const uint8_t context[] = "counter-mode";
    uint8_t okm[48];
    static const uint8_t expected[48] = {
        0xa4,0x4a,0xbe,0x06,0x9a,0x9c,0x19,0x1c,0x38,0xbe,0xc8,0x27,0xb8,0x49,0x5d,0x6a,
        0xb8,0x3e,0x31,0x75,0x12,0x4e,0xa1,0xb2,0xb6,0xc9,0x7a,0xa6,0x6e,0xb6,0x16,0xb2,
        0xe5,0xd0,0x6c,0x84,0x71,0x50,0x75,0xa3,0xbb,0xb8,0xfe,0x25,0xa0,0xad,0xa3,0x43};
    keym_status_t status;
    
    printf("  Testing KeyM SP 800-108 KDF multiblock...\n");
    
    status = keym_sp800_108_counter(ki, sizeof(ki),
                                    label, sizeof(label) - 1U,
                                    context, sizeof(context) - 1U,
                                    okm, sizeof(okm));
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT(memcmp(okm, expected, 48) == 0);
    
    /* 参数校验 fail-closed */
    TEST_ASSERT_EQ(keym_sp800_108_counter(NULL, 0U, label, 0U, context, 0U, okm, 32U),
                   KEYM_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQ(keym_sp800_108_counter(ki, sizeof(ki), NULL, 1U, context, 0U, okm, 32U),
                   KEYM_ERROR_INVALID_PARAM);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_sp800_108_derive_api(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t parent_slot;
    uint8_t derived_slot;
    uint8_t derived_key[32];
    uint32_t derived_len;
    keym_derivation_params_t params;
    static const uint8_t ki[16] = {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                                   0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    static const uint8_t label[] = "SP800-108-test";
    static const uint8_t context[] = "counter-mode";
    static const uint8_t expected[32] = {
        0xcf,0xe0,0x12,0xff,0xbe,0x94,0x44,0x3b,0x8c,0x81,0xea,0x2c,0xa7,0x77,0x1e,0xa9,
        0x8c,0xe6,0x3e,0xb8,0x23,0x6a,0xee,0xc8,0x50,0xec,0x19,0x9d,0x15,0x6c,0x81,0x40};
    keym_slot_info_t info;
    
    printf("  Testing KeyM SP 800-108 derive via keym_key_derive...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* 父槽导入 KI */
    parent_slot = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &parent_slot, "parent_ki", KEYM_TYPE_AES_128);
    TEST_ASSERT_EQ(status, KEYM_OK);
    status = keym_key_import(ctx, parent_slot, ki, sizeof(ki), 1U);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* SP 800-108 派生 */
    memset(&params, 0, sizeof(params));
    params.parent_slot_id = parent_slot;
    params.target_slot_id = KEYM_SLOT_ID_INVALID;
    params.kdf_type = KEYM_KDF_NIST_SP800_108;
    params.label = label;
    params.label_len = sizeof(label) - 1U;
    params.context = context;
    params.context_len = sizeof(context) - 1U;
    params.derived_key_type = KEYM_TYPE_AES_256;
    params.derived_key_len = 32U;
    
    status = keym_key_derive(ctx, &params, &derived_slot);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_NE(derived_slot, KEYM_SLOT_ID_INVALID);
    
    /* 派生槽默认可导出性关闭 -> 先置可导出再比对 */
    status = keym_slot_set_attributes(ctx, derived_slot, KEYM_USAGE_KEY_DERIVATION,
                                      false, true);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    /* 导出并与已知向量比对 */
    derived_len = sizeof(derived_key);
    status = keym_key_export(ctx, derived_slot, derived_key, &derived_len);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(derived_len, 32U);
    TEST_ASSERT(memcmp(derived_key, expected, 32) == 0);
    
    /* 簿记 */
    status = keym_slot_get_info(ctx, derived_slot, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.parent_slot_id, parent_slot);
    TEST_ASSERT_EQ(info.kdf_type, KEYM_KDF_NIST_SP800_108);
    TEST_ASSERT_EQ(info.key_type, KEYM_TYPE_AES_256);
    
    /* 导入密钥 CRC 应为真实值 (IEEE 802.3) */
    TEST_ASSERT_EQ(ctx->materials[parent_slot].crc32, keym_crc32(ki, sizeof(ki)));
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_hkdf_rfc5869_tc1(void)
{
    /* RFC 5869 Appendix A.1 TC1 */
    uint8_t ikm[22];
    uint8_t salt[13];
    uint8_t info[10];
    uint8_t okm[42];
    static const uint8_t expected[42] = {
        0x3c,0xb2,0x5f,0x25,0xfa,0xac,0xd5,0x7a,0x90,0x43,0x4f,0x64,0xd0,0x36,0x2f,0x2a,
        0x2d,0x2d,0x0a,0x90,0xcf,0x1a,0x5a,0x4c,0x5d,0xb0,0x2d,0x56,0xec,0xc4,0xc5,0xbf,
        0x34,0x00,0x72,0x08,0xd5,0xb8,0x87,0x18,0x58,0x65};
    keym_status_t status;
    uint32_t i;
    
    printf("  Testing KeyM HKDF-SHA256 (RFC 5869 TC1)...\n");
    
    memset(ikm, 0x0b, sizeof(ikm));
    for (i = 0U; i < 13U; i++) { salt[i] = (uint8_t)i; }
    for (i = 0U; i < 10U; i++) { info[i] = (uint8_t)(0xF0U + i); }
    
    status = keym_hkdf_sha256(ikm, sizeof(ikm), salt, sizeof(salt),
                              info, sizeof(info), okm, sizeof(okm));
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT(memcmp(okm, expected, 42) == 0);
    
    /* salt 缺省 (NULL, 0) 应可用 (RFC 5869: 32 字节零盐) */
    status = keym_hkdf_sha256(ikm, sizeof(ikm), NULL, 0U, info, sizeof(info),
                              okm, 16U);
    TEST_ASSERT_EQ(status, KEYM_OK);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_crc32(void)
{
    uint32_t crc;
    
    printf("  Testing KeyM CRC-32 (IEEE 802.3)...\n");
    
    crc = keym_crc32((const uint8_t*)"123456789", 9U);
    TEST_ASSERT_EQ(crc, 0xCBF43926UL);
    
    crc = keym_crc32(NULL, 0U);
    TEST_ASSERT_EQ(crc, 0x00000000UL);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_not_implemented(void)
{
    keym_context_t *ctx;
    uint8_t slot;
    
    printf("  Testing KeyM NOT_IMPLEMENTED fail-closed...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* DDS 证书导入导出 / 持久化存储 -> 显式 NOT_IMPLEMENTED (原 TODO 恒 OK 假实现) */
    TEST_ASSERT_EQ(keym_import_from_dds_cert(ctx, "cert", &slot),
                   KEYM_ERROR_NOT_IMPLEMENTED);
    TEST_ASSERT_EQ(keym_export_to_dds_cert(ctx, 0U, "cert"),
                   KEYM_ERROR_NOT_IMPLEMENTED);
    TEST_ASSERT_EQ(keym_load_persistent_keys(ctx), KEYM_ERROR_NOT_IMPLEMENTED);
    TEST_ASSERT_EQ(keym_save_persistent_keys(ctx), KEYM_ERROR_NOT_IMPLEMENTED);
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_keym_secoc_integration(void)
{
    keym_context_t *ctx;
    keym_status_t status;
    uint8_t key_slot;
    uint32_t pdu_id = 0x1234;
    
    printf("  Testing KeyM SecOC integration...\n");
    
    ctx = keym_init(NULL, NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Configure SecOC key */
    status = keym_configure_secoc_key(ctx, pdu_id, &key_slot);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_NE(key_slot, KEYM_SLOT_ID_INVALID);
    
    /* Get SecOC key slot */
    {
        uint8_t found_slot = keym_get_secoc_key_slot(ctx, pdu_id);
        TEST_ASSERT_EQ(found_slot, key_slot);
    }
    
    /* Check slot info */
    {
        keym_slot_info_t info;
        status = keym_slot_get_info(ctx, key_slot, &info);
        TEST_ASSERT_EQ(status, KEYM_OK);
        TEST_ASSERT_EQ(info.usage_flags, KEYM_USAGE_SECOC);
    }
    
    keym_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

static int run_test(int (*test_func)(void), const char *name)
{
    printf("Running %s...\n", name);
    tests_run++;
    
    if (test_func() == 0) {
        tests_passed++;
        return 0;
    }
    return -1;
}

int main(void)
{
    printf("============================================\n");
    printf("KeyM (Key Manager) Unit Tests\n");
    printf("============================================\n\n");
    
    run_test(test_keym_init_deinit, "KeyM Init/Deinit");
    run_test(test_keym_slot_management, "KeyM Slot Management");
    run_test(test_keym_key_import_export, "KeyM Key Import/Export");
    run_test(test_keym_key_generation, "KeyM Key Generation");
    run_test(test_keym_key_derivation, "KeyM Key Derivation");
    run_test(test_keym_key_rotation, "KeyM Key Rotation");
    run_test(test_keym_certificate_management, "KeyM Certificate Management");
    run_test(test_keym_secoc_integration, "KeyM SecOC Integration");
    run_test(test_keym_hmac_rfc2104, "KeyM HMAC-SHA256 RFC 2104 TC1");
    run_test(test_keym_sp800_108_vector, "KeyM SP 800-108 Known Vector");
    run_test(test_keym_sp800_108_multiblock, "KeyM SP 800-108 Multiblock");
    run_test(test_keym_sp800_108_derive_api, "KeyM SP 800-108 Derive API");
    run_test(test_keym_hkdf_rfc5869_tc1, "KeyM HKDF-SHA256 RFC 5869 TC1");
    run_test(test_keym_crc32, "KeyM CRC-32 IEEE 802.3");
    run_test(test_keym_not_implemented, "KeyM NOT_IMPLEMENTED fail-closed");
    
    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}

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
    uint8_t found_slot = keym_get_secoc_key_slot(ctx, pdu_id);
    TEST_ASSERT_EQ(found_slot, key_slot);
    
    /* Check slot info */
    keym_slot_info_t info;
    status = keym_slot_get_info(ctx, key_slot, &info);
    TEST_ASSERT_EQ(status, KEYM_OK);
    TEST_ASSERT_EQ(info.usage_flags, KEYM_USAGE_SECOC);
    
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
    
    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}

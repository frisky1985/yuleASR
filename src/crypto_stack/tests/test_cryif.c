/**
 * @file test_cryif.c
 * @brief CryIf (Crypto Interface) Unit Tests
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../cryif/cryif_core.h"

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
static int test_cryif_init_deinit(void);
static int test_cryif_driver_registration(void);
static int test_cryif_key_slot_management(void);
static int test_cryif_channel_management(void);
static int test_cryif_crypto_operations(void);

/* ============================================================================
 * Mock Driver Implementation
 * ============================================================================ */

typedef struct {
    int init_count;
    int deinit_count;
    int encrypt_count;
    int decrypt_count;
    int mac_count;
    int key_ops_count;
} mock_driver_stats_t;

static mock_driver_stats_t g_mock_stats = {0};

static cryif_status_t mock_init(cryif_driver_t *driver)
{
    mock_driver_stats_t *stats = (mock_driver_stats_t*)driver->driver_data;
    stats->init_count++;
    return CRYIF_OK;
}

static cryif_status_t mock_deinit(cryif_driver_t *driver)
{
    mock_driver_stats_t *stats = (mock_driver_stats_t*)driver->driver_data;
    stats->deinit_count++;
    return CRYIF_OK;
}

static bool mock_supports_algo(cryif_driver_t *driver, uint32_t algorithm)
{
    (void)driver;
    /* Support AES and SHA algorithms */
    return (algorithm < 0x40 || (algorithm >= 0x20 && algorithm < 0x40));
}

static cryif_status_t mock_encrypt(cryif_driver_t *driver,
                                   uint8_t key_slot,
                                   const uint8_t *plaintext, uint32_t pt_len,
                                   const uint8_t *iv, uint32_t iv_len,
                                   const uint8_t *aad, uint32_t aad_len,
                                   uint8_t *ciphertext, uint32_t *ct_len,
                                   uint8_t *tag, uint32_t *tag_len)
{
    mock_driver_stats_t *stats = (mock_driver_stats_t*)driver->driver_data;
    (void)key_slot;
    (void)iv;
    (void)iv_len;
    (void)aad;
    (void)aad_len;
    (void)tag;
    (void)tag_len;
    
    stats->encrypt_count++;
    memcpy(ciphertext, plaintext, pt_len);
    *ct_len = pt_len;
    return CRYIF_OK;
}

static cryif_status_t mock_decrypt(cryif_driver_t *driver,
                                   uint8_t key_slot,
                                   const uint8_t *ciphertext, uint32_t ct_len,
                                   const uint8_t *iv, uint32_t iv_len,
                                   const uint8_t *aad, uint32_t aad_len,
                                   const uint8_t *tag, uint32_t tag_len,
                                   uint8_t *plaintext, uint32_t *pt_len)
{
    mock_driver_stats_t *stats = (mock_driver_stats_t*)driver->driver_data;
    (void)key_slot;
    (void)iv;
    (void)iv_len;
    (void)aad;
    (void)aad_len;
    (void)tag;
    (void)tag_len;
    
    stats->decrypt_count++;
    memcpy(plaintext, ciphertext, ct_len);
    *pt_len = ct_len;
    return CRYIF_OK;
}

static cryif_status_t mock_mac_generate(cryif_driver_t *driver,
                                        uint8_t key_slot,
                                        const uint8_t *data, uint32_t data_len,
                                        const uint8_t *iv, uint32_t iv_len,
                                        uint8_t *mac, uint32_t *mac_len)
{
    mock_driver_stats_t *stats = (mock_driver_stats_t*)driver->driver_data;
    (void)key_slot;
    (void)data;
    (void)data_len;
    (void)iv;
    (void)iv_len;
    
    stats->mac_count++;
    memset(mac, 0xAB, 16);
    *mac_len = 16;
    return CRYIF_OK;
}

static cryif_status_t mock_key_import(cryif_driver_t *driver,
                                      uint8_t slot_id,
                                      cryif_key_type_t key_type,
                                      const uint8_t *key, uint32_t key_len)
{
    mock_driver_stats_t *stats = (mock_driver_stats_t*)driver->driver_data;
    (void)slot_id;
    (void)key_type;
    (void)key;
    (void)key_len;
    
    stats->key_ops_count++;
    return CRYIF_OK;
}

static cryif_driver_t g_mock_driver = {
    .hw_type = CRYIF_HW_HSM,
    .driver_name = "MockHSM",
    .driver_version = 0x01000000,
    .init = mock_init,
    .deinit = mock_deinit,
    .supports_algorithm = mock_supports_algo,
    .encrypt = mock_encrypt,
    .decrypt = mock_decrypt,
    .mac_generate = mock_mac_generate,
    .key_import = mock_key_import,
    .driver_data = &g_mock_stats
};

/* ============================================================================
 * Test Implementations
 * ============================================================================ */

static int test_cryif_init_deinit(void)
{
    cryif_context_t *ctx;
    
    printf("  Testing CryIf init/deinit...\n");
    
    ctx = cryif_init();
    TEST_ASSERT(ctx != NULL);
    TEST_ASSERT(ctx->initialized == true);
    TEST_ASSERT(ctx->num_drivers == 0);
    TEST_ASSERT(ctx->default_driver == NULL);
    
    cryif_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_cryif_driver_registration(void)
{
    cryif_context_t *ctx;
    cryif_status_t status;
    cryif_driver_t *found_driver;
    
    printf("  Testing CryIf driver registration...\n");
    
    ctx = cryif_init();
    TEST_ASSERT(ctx != NULL);
    
    /* Register driver */
    memset(&g_mock_stats, 0, sizeof(g_mock_stats));
    g_mock_driver.driver_data = &g_mock_stats;
    
    status = cryif_register_driver(ctx, &g_mock_driver);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(ctx->num_drivers, 1);
    TEST_ASSERT_EQ(ctx->default_driver, &g_mock_driver);
    TEST_ASSERT_EQ(g_mock_stats.init_count, 1);
    
    /* Find driver for algorithm */
    found_driver = cryif_find_driver_for_algo(ctx, 0x00);  /* AES-128-CBC */
    TEST_ASSERT_EQ(found_driver, &g_mock_driver);
    
    /* Unregister driver */
    status = cryif_unregister_driver(ctx, &g_mock_driver);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(ctx->num_drivers, 0);
    TEST_ASSERT_EQ(g_mock_stats.deinit_count, 1);
    
    cryif_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_cryif_key_slot_management(void)
{
    cryif_context_t *ctx;
    cryif_status_t status;
    uint8_t slot_id;
    cryif_key_slot_info_t info;
    uint8_t key_data[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    
    printf("  Testing CryIf key slot management...\n");
    
    ctx = cryif_init();
    TEST_ASSERT(ctx != NULL);
    
    status = cryif_register_driver(ctx, &g_mock_driver);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    
    /* Allocate slot */
    slot_id = CRYIF_KEY_SLOT_INVALID;
    status = cryif_key_slot_allocate(ctx, &slot_id, NULL);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_NE(slot_id, CRYIF_KEY_SLOT_INVALID);
    
    /* Import key */
    memset(&g_mock_stats, 0, sizeof(g_mock_stats));
    status = cryif_key_import(ctx, slot_id, CRYIF_KEY_TYPE_AES_128, key_data, sizeof(key_data));
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(g_mock_stats.key_ops_count, 1);
    
    /* Get slot info */
    status = cryif_key_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(info.slot_id, slot_id);
    TEST_ASSERT_EQ(info.state, CRYIF_KEY_STATE_VALID);
    
    /* Free slot */
    status = cryif_key_slot_free(ctx, slot_id);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    
    /* Try to get info of freed slot */
    status = cryif_key_get_info(ctx, slot_id, &info);
    TEST_ASSERT_EQ(status, CRYIF_ERROR_KEY_SLOT_NOT_FOUND);
    
    cryif_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_cryif_channel_management(void)
{
    cryif_context_t *ctx;
    cryif_status_t status;
    cryif_channel_t channel;
    
    printf("  Testing CryIf channel management...\n");
    
    ctx = cryif_init();
    TEST_ASSERT(ctx != NULL);
    
    status = cryif_register_driver(ctx, &g_mock_driver);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    
    /* Configure channel */
    status = cryif_channel_configure(ctx, 0, &g_mock_driver, 10);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    
    /* Get channel info */
    status = cryif_channel_get_info(ctx, 0, &channel);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(channel.channel_id, 0);
    TEST_ASSERT_EQ(channel.driver, &g_mock_driver);
    TEST_ASSERT_EQ(channel.priority, 10);
    
    cryif_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_cryif_crypto_operations(void)
{
    cryif_context_t *ctx;
    cryif_status_t status;
    uint8_t slot_id;
    uint8_t plaintext[] = "Test message!";
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    uint32_t ct_len = sizeof(ciphertext);
    uint32_t pt_len = sizeof(decrypted);
    uint8_t mac[16];
    uint32_t mac_len = sizeof(mac);
    
    printf("  Testing CryIf crypto operations...\n");
    
    ctx = cryif_init();
    TEST_ASSERT(ctx != NULL);
    
    status = cryif_register_driver(ctx, &g_mock_driver);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    
    /* Allocate and setup key slot */
    slot_id = CRYIF_KEY_SLOT_INVALID;
    status = cryif_key_slot_allocate(ctx, &slot_id, NULL);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    
    /* Test encrypt */
    memset(&g_mock_stats, 0, sizeof(g_mock_stats));
    status = cryif_encrypt(ctx, slot_id, 0, plaintext, sizeof(plaintext),
                           NULL, 0, NULL, 0,
                           ciphertext, &ct_len, NULL, NULL);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(g_mock_stats.encrypt_count, 1);
    
    /* Test decrypt */
    status = cryif_decrypt(ctx, slot_id, 0, ciphertext, ct_len,
                           NULL, 0, NULL, 0, NULL, 0,
                           decrypted, &pt_len);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(g_mock_stats.decrypt_count, 1);
    TEST_ASSERT_EQ(pt_len, sizeof(plaintext));
    
    /* Test MAC generation */
    status = cryif_mac_generate(ctx, slot_id, 0, plaintext, sizeof(plaintext), mac, &mac_len);
    TEST_ASSERT_EQ(status, CRYIF_OK);
    TEST_ASSERT_EQ(g_mock_stats.mac_count, 1);
    TEST_ASSERT_EQ(mac_len, 16);
    
    /* Free slot */
    cryif_key_slot_free(ctx, slot_id);
    
    cryif_deinit(ctx);
    
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
    printf("CryIf (Crypto Interface) Unit Tests\n");
    printf("============================================\n\n");
    
    run_test(test_cryif_init_deinit, "CryIf Init/Deinit");
    run_test(test_cryif_driver_registration, "CryIf Driver Registration");
    run_test(test_cryif_key_slot_management, "CryIf Key Slot Management");
    run_test(test_cryif_channel_management, "CryIf Channel Management");
    run_test(test_cryif_crypto_operations, "CryIf Crypto Operations");
    
    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}

/**
 * @file test_csm.c
 * @brief CSM (Crypto Services Manager) Unit Tests
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../csm/csm_core.h"

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
static int test_csm_init_deinit(void);
static int test_csm_job_create_release(void);
static int test_csm_job_submit_process(void);
static int test_csm_encrypt_decrypt(void);
static int test_csm_mac_operations(void);
static int test_csm_hash(void);
static int test_csm_random(void);
static int test_csm_queue_management(void);
static int test_csm_callbacks(void);

/* ============================================================================
 * Test Implementations
 * ============================================================================ */

static int test_csm_init_deinit(void)
{
    csm_context_t *ctx;
    csm_config_t config = {
        .max_jobs = 16,
        .default_timeout_ms = 1000,
        .enable_async_processing = true,
        .num_worker_threads = 1,
        .use_hw_acceleration = false,
        .queue_high_watermark = 12,
        .queue_low_watermark = 4
    };
    
    printf("  Testing CSM init/deinit...\n");
    
    /* Test with config */
    ctx = csm_init(&config);
    TEST_ASSERT(ctx != NULL);
    TEST_ASSERT(ctx->initialized == true);
    TEST_ASSERT(ctx->config.max_jobs == 16);
    csm_deinit(ctx);
    
    /* Test with NULL config (default) */
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_job_create_release(void)
{
    csm_context_t *ctx;
    uint32_t job_id1, job_id2;
    csm_status_t status;
    
    printf("  Testing CSM job create/release...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Create job */
    job_id1 = csm_job_create(ctx, CSM_JOB_ENCRYPT, CSM_ALGO_AES_128_GCM, 1);
    TEST_ASSERT_NE(job_id1, CSM_JOB_ID_INVALID);
    
    /* Create another job */
    job_id2 = csm_job_create(ctx, CSM_JOB_DECRYPT, CSM_ALGO_AES_128_GCM, 1);
    TEST_ASSERT_NE(job_id2, CSM_JOB_ID_INVALID);
    TEST_ASSERT_NE(job_id1, job_id2);  /* Job IDs should be unique */
    
    /* Release jobs */
    status = csm_job_release(ctx, job_id1);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    status = csm_job_release(ctx, job_id2);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_job_submit_process(void)
{
    csm_context_t *ctx;
    uint32_t job_id;
    csm_status_t status;
    csm_job_state_t state;
    
    printf("  Testing CSM job submit/process...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Create and setup job */
    job_id = csm_job_create(ctx, CSM_JOB_HASH, CSM_ALGO_SHA_256, 0);
    TEST_ASSERT_NE(job_id, CSM_JOB_ID_INVALID);
    
    /* Set input data */
    uint8_t input_data[] = "Hello, World!";
    uint8_t output_data[32];
    uint32_t output_len = sizeof(output_data);
    
    status = csm_job_set_input(ctx, job_id, input_data, sizeof(input_data));
    TEST_ASSERT_EQ(status, CSM_OK);
    
    status = csm_job_set_output(ctx, job_id, output_data, output_len, &output_len);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    /* Submit job */
    status = csm_job_submit(ctx, job_id, CSM_JOB_PRIO_NORMAL);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    /* Check state */
    status = csm_job_get_state(ctx, job_id, &state);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(state, CSM_JOB_STATE_QUEUED);
    
    /* Process job synchronously */
    status = csm_job_process_sync(ctx, job_id, 5000);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    /* Check final state */
    status = csm_job_get_state(ctx, job_id, &state);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(state, CSM_JOB_STATE_COMPLETED);
    
    /* Release job */
    status = csm_job_release(ctx, job_id);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_encrypt_decrypt(void)
{
    csm_context_t *ctx;
    csm_status_t status;
    
    printf("  Testing CSM encrypt/decrypt...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    uint8_t plaintext[] = "Test message for encryption!";
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    uint32_t ct_len = sizeof(ciphertext);
    uint32_t pt_len = sizeof(decrypted);
    
    /* Encrypt */
    status = csm_encrypt(ctx, CSM_ALGO_AES_128_CBC, 1,
                         plaintext, sizeof(plaintext),
                         ciphertext, &ct_len);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT(ct_len > 0);
    
    /* Decrypt */
    status = csm_decrypt(ctx, CSM_ALGO_AES_128_CBC, 1,
                         ciphertext, ct_len,
                         decrypted, &pt_len);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(pt_len, sizeof(plaintext));
    TEST_ASSERT(memcmp(plaintext, decrypted, sizeof(plaintext)) == 0);
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_mac_operations(void)
{
    csm_context_t *ctx;
    csm_status_t status;
    
    printf("  Testing CSM MAC operations (real HMAC-SHA256)...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    uint8_t data[] = "Message to authenticate";
    uint8_t mac[32];
    uint32_t mac_len = sizeof(mac);
    bool verify_result = false;
    
    /* 生成 MAC (真实 HMAC-SHA256, 32B) */
    status = csm_mac_generate(ctx, CSM_ALGO_HMAC_SHA_256, 1,
                              data, sizeof(data),
                              mac, &mac_len);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(mac_len, 32);
    
    /* 正确签名验证通过 (真实计算+比较) */
    status = csm_mac_verify(ctx, CSM_ALGO_HMAC_SHA_256, 1,
                            data, sizeof(data),
                            mac, mac_len, &verify_result);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(verify_result, true);
    
    /* 错误签名必须 FAIL (不再恒 true) */
    {
        uint8_t bad_mac[32];
        memcpy(bad_mac, mac, sizeof(bad_mac));
        bad_mac[0] ^= 0xFFU;   /* 翻转签名首字节 */
        verify_result = true;  /* 预置为 true, 验证后端会将其改为 false */
        status = csm_mac_verify(ctx, CSM_ALGO_HMAC_SHA_256, 1,
                                data, sizeof(data),
                                bad_mac, sizeof(bad_mac), &verify_result);
        TEST_ASSERT_EQ(status, CSM_OK);
        TEST_ASSERT_EQ(verify_result, false);
    }
    
    /* 错误长度签名 (64B, 如 ECDSA 风格) 必须 FAIL */
    {
        uint8_t long_mac[64];
        memset(long_mac, 0x11, sizeof(long_mac));
        verify_result = true;
        status = csm_mac_verify(ctx, CSM_ALGO_HMAC_SHA_256, 1,
                                data, sizeof(data),
                                long_mac, sizeof(long_mac), &verify_result);
        TEST_ASSERT_EQ(status, CSM_OK);
        TEST_ASSERT_EQ(verify_result, false);
    }
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_hash(void)
{
    csm_context_t *ctx;
    csm_status_t status;
    
    printf("  Testing CSM hash (real SHA-256)...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    uint8_t data[] = "Hash this message";
    uint8_t hash[32];
    uint32_t hash_len = sizeof(hash);
    
    /* SHA-256 */
    status = csm_hash(ctx, CSM_ALGO_SHA_256, data, sizeof(data), hash, &hash_len);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(hash_len, 32);
    
    /* SHA-256 已知向量: SHA256("abc") */
    {
        uint8_t hash_abc[32];
        uint32_t hash_abc_len = sizeof(hash_abc);
        static const uint8_t expected_abc[32] = {
            0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
            0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
        status = csm_hash(ctx, CSM_ALGO_SHA_256, (const uint8_t*)"abc", 3U,
                          hash_abc, &hash_abc_len);
        TEST_ASSERT_EQ(status, CSM_OK);
        TEST_ASSERT_EQ(hash_abc_len, 32);
        TEST_ASSERT(memcmp(hash_abc, expected_abc, 32) == 0);
    }
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_random(void)
{
    csm_context_t *ctx;
    csm_status_t status;
    
    printf("  Testing CSM random generation...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    uint8_t random1[32];
    uint8_t random2[32];
    
    /* Generate two random blocks */
    status = csm_random_generate(ctx, random1, sizeof(random1));
    TEST_ASSERT_EQ(status, CSM_OK);
    
    status = csm_random_generate(ctx, random2, sizeof(random2));
    TEST_ASSERT_EQ(status, CSM_OK);
    
    /* Should be different (with very high probability) */
    TEST_ASSERT(memcmp(random1, random2, sizeof(random1)) != 0);
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static int test_csm_queue_management(void)
{
    csm_context_t *ctx;
    csm_status_t status;
    csm_queue_stats_t stats;
    uint32_t job_ids[5];
    
    printf("  Testing CSM queue management...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Create and submit multiple jobs */
    for (int i = 0; i < 5; i++) {
        job_ids[i] = csm_job_create(ctx, CSM_JOB_HASH, CSM_ALGO_SHA_256, 0);
        TEST_ASSERT_NE(job_ids[i], CSM_JOB_ID_INVALID);
        
        uint8_t data[32] = {0};
        uint8_t hash[32];
        uint32_t hash_len = sizeof(hash);
        
        status = csm_job_set_input(ctx, job_ids[i], data, sizeof(data));
        TEST_ASSERT_EQ(status, CSM_OK);
        
        status = csm_job_set_output(ctx, job_ids[i], hash, hash_len, &hash_len);
        TEST_ASSERT_EQ(status, CSM_OK);
        
        status = csm_job_submit(ctx, job_ids[i], (i % 2 == 0) ? CSM_JOB_PRIO_HIGH : CSM_JOB_PRIO_NORMAL);
        TEST_ASSERT_EQ(status, CSM_OK);
    }
    
    /* Check stats */
    status = csm_get_queue_stats(ctx, &stats);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(stats.total_jobs_submitted, 5);
    
    /* Process queue */
    uint32_t processed = csm_process_queue(ctx);
    TEST_ASSERT_EQ(processed, 5);
    
    /* Check updated stats */
    status = csm_get_queue_stats(ctx, &stats);
    TEST_ASSERT_EQ(status, CSM_OK);
    TEST_ASSERT_EQ(stats.total_jobs_completed, 5);
    
    /* Release jobs */
    for (int i = 0; i < 5; i++) {
        csm_job_release(ctx, job_ids[i]);
    }
    
    csm_deinit(ctx);
    
    printf("  PASSED\n");
    return 0;
}

static void test_callback(uint32_t job_id, csm_status_t result, void *user_data)
{
    (void)job_id;
    int *callback_count = (int*)user_data;
    if (result == CSM_OK) {
        (*callback_count)++;
    }
}

static int test_csm_callbacks(void)
{
    csm_context_t *ctx;
    uint32_t job_id;
    csm_status_t status;
    int callback_count = 0;
    
    printf("  Testing CSM callbacks...\n");
    
    ctx = csm_init(NULL);
    TEST_ASSERT(ctx != NULL);
    
    /* Register global callback */
    int cb_id = csm_register_callback(ctx, test_callback, &callback_count);
    TEST_ASSERT(cb_id >= 0);
    
    /* Create and process job */
    job_id = csm_job_create(ctx, CSM_JOB_HASH, CSM_ALGO_SHA_256, 0);
    TEST_ASSERT_NE(job_id, CSM_JOB_ID_INVALID);
    
    uint8_t data[32] = {0};
    uint8_t hash[32];
    uint32_t hash_len = sizeof(hash);
    
    csm_job_set_input(ctx, job_id, data, sizeof(data));
    csm_job_set_output(ctx, job_id, hash, hash_len, &hash_len);
    csm_job_process_sync(ctx, job_id, 5000);
    
    TEST_ASSERT_EQ(callback_count, 1);
    
    /* Unregister callback */
    status = csm_unregister_callback(ctx, cb_id);
    TEST_ASSERT_EQ(status, CSM_OK);
    
    csm_job_release(ctx, job_id);
    csm_deinit(ctx);
    
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
    printf("CSM (Crypto Services Manager) Unit Tests\n");
    printf("============================================\n\n");
    
    run_test(test_csm_init_deinit, "CSM Init/Deinit");
    run_test(test_csm_job_create_release, "CSM Job Create/Release");
    run_test(test_csm_job_submit_process, "CSM Job Submit/Process");
    run_test(test_csm_encrypt_decrypt, "CSM Encrypt/Decrypt");
    run_test(test_csm_mac_operations, "CSM MAC Operations");
    run_test(test_csm_hash, "CSM Hash");
    run_test(test_csm_random, "CSM Random");
    run_test(test_csm_queue_management, "CSM Queue Management");
    run_test(test_csm_callbacks, "CSM Callbacks");
    
    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}

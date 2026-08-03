/**
 * @file test_secoc_core.c
 * @brief SecOC Core Module Unit Tests
 * @version 1.0
 * @date 2026-04-25
 */

#include "unity.h"
#include "secoc_core.h"
#include <string.h>
#include <stdio.h>

/* 测试用模拟数据 */
static secoc_context_t *test_ctx = NULL;
static const uint8_t test_key[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
};

/* 测试前置处理 */
void setUp(void) {
    test_ctx = secoc_init(NULL);
    TEST_ASSERT_NOT_NULL(test_ctx);
}

/* 测试后清理 */
void tearDown(void) {
    if (test_ctx) {
        secoc_deinit(test_ctx);
        test_ctx = NULL;
    }
}

/* 测试初始化 */
void test_secoc_init_should_return_valid_context(void) {
    TEST_ASSERT_NOT_NULL(test_ctx);
    TEST_ASSERT_TRUE(test_ctx->initialized);
}

/* 测试密钥导入 */
void test_secoc_import_key_should_succeed(void) {
    secoc_status_t status = secoc_import_key(test_ctx, 0, test_key, 16);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    /* 验证密钥已导入 */
    uint8_t retrieved_key[32];
    uint8_t key_len;
    status = secoc_get_key(test_ctx, 0, retrieved_key, &key_len);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(16, key_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(test_key, retrieved_key, 16);
}

/* 测试密钥导入无效参数 */
void test_secoc_import_key_invalid_params_should_fail(void) {
    secoc_status_t status = secoc_import_key(NULL, 0, test_key, 16);
    TEST_ASSERT_EQUAL(SECOC_ERROR_INVALID_PARAM, status);
    
    status = secoc_import_key(test_ctx, SECOC_MAX_KEY_SLOTS, test_key, 16);
    TEST_ASSERT_EQUAL(SECOC_ERROR_INVALID_PARAM, status);
    
    status = secoc_import_key(test_ctx, 0, NULL, 16);
    TEST_ASSERT_EQUAL(SECOC_ERROR_INVALID_PARAM, status);
    
    status = secoc_import_key(test_ctx, 0, test_key, 33);  /* 超过最大长度 */
    TEST_ASSERT_EQUAL(SECOC_ERROR_INVALID_PARAM, status);
}

/* 测试获取无效密钥槽 */
void test_secoc_get_key_invalid_slot_should_fail(void) {
    uint8_t key[32];
    uint8_t key_len;
    secoc_status_t status = secoc_get_key(test_ctx, 0, key, &key_len);
    TEST_ASSERT_EQUAL(SECOC_ERROR_KEY_NOT_FOUND, status);
}

/* 测试清除密钥 */
void test_secoc_clear_key_should_succeed(void) {
    /* 先导入密钥 */
    secoc_import_key(test_ctx, 0, test_key, 16);
    
    /* 清除密钥 */
    secoc_status_t status = secoc_clear_key(test_ctx, 0);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    /* 验证密钥已清除 */
    uint8_t key[32];
    uint8_t key_len;
    status = secoc_get_key(test_ctx, 0, key, &key_len);
    TEST_ASSERT_EQUAL(SECOC_ERROR_KEY_NOT_FOUND, status);
}

/* 测试添加PDU配置 */
void test_secoc_add_pdu_config_should_succeed(void) {
    secoc_pdu_config_t config = {
        .pdu_id = 0x100,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,  /* 4 bytes */
        .mac_len = 64,              /* 8 bytes */
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100,
        .use_tx_confirmation = true,
        .enable_replay_protection = true
    };
    
    secoc_status_t status = secoc_add_pdu_config(test_ctx, &config);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    /* 验证配置已添加 */
    const secoc_pdu_config_t *retrieved = secoc_get_pdu_config(test_ctx, 0x100);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(0x100, retrieved->pdu_id);
    TEST_ASSERT_EQUAL(SECOC_FRESHNESS_COUNTER, retrieved->freshness_type);
    TEST_ASSERT_EQUAL(SECOC_ALGO_AES_CMAC_128, retrieved->auth_algo);
}

/* 测试添加重复PDU ID */
void test_secoc_add_duplicate_pdu_config_should_fail(void) {
    secoc_pdu_config_t config = {
        .pdu_id = 0x100,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,
        .mac_len = 64,
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100
    };
    
    secoc_add_pdu_config(test_ctx, &config);
    
    /* 添加重复的PDU ID */
    secoc_status_t status = secoc_add_pdu_config(test_ctx, &config);
    TEST_ASSERT_EQUAL(SECOC_ERROR_INVALID_PARAM, status);
}

/* 测试查找不存在的PDU */
void test_secoc_get_nonexistent_pdu_config_should_return_null(void) {
    const secoc_pdu_config_t *config = secoc_get_pdu_config(test_ctx, 0x999);
    TEST_ASSERT_NULL(config);
}

/* 测试移除PDU配置 */
void test_secoc_remove_pdu_config_should_succeed(void) {
    secoc_pdu_config_t config = {
        .pdu_id = 0x200,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,
        .mac_len = 64,
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100
    };
    
    secoc_add_pdu_config(test_ctx, &config);
    
    secoc_status_t status = secoc_remove_pdu_config(test_ctx, 0x200);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    /* 验证配置已移除 */
    const secoc_pdu_config_t *retrieved = secoc_get_pdu_config(test_ctx, 0x200);
    TEST_ASSERT_NULL(retrieved);
}

/* 测试PDU认证和验证 */
void test_secoc_authenticate_and_verify_pdu_should_succeed(void) {
    /* 设置配置 */
    secoc_pdu_config_t config = {
        .pdu_id = 0x300,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,
        .mac_len = 64,
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100,
        .use_tx_confirmation = false,
        .enable_replay_protection = false
    };
    
    secoc_add_pdu_config(test_ctx, &config);
    secoc_import_key(test_ctx, 0, test_key, 16);
    
    /* 测试数据 */
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    /* 认证PDU */
    secoc_authenticated_pdu_t auth_pdu;
    secoc_status_t status = secoc_authenticate_tx_pdu(test_ctx, 0x300, test_data, sizeof(test_data), &auth_pdu);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(0x300, auth_pdu.pdu_id);
    TEST_ASSERT_EQUAL(sizeof(test_data), auth_pdu.data_len);
    TEST_ASSERT_EQUAL(4, auth_pdu.freshness_len);  /* 32 bits */
    TEST_ASSERT_EQUAL(8, auth_pdu.auth_len);       /* 64 bits */
    
    /* 构建Secured PDU */
    uint8_t secured_pdu[64];
    uint32_t secured_len;
    status = secoc_build_secured_pdu(&auth_pdu, secured_pdu, &secured_len, sizeof(secured_pdu));
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(sizeof(test_data) + 4 + 8, secured_len);  /* data + FV + MAC */
    
    /* 验证PDU */
    uint8_t original_data[64];
    uint32_t original_len;
    secoc_verify_result_t verify_result;
    status = secoc_verify_rx_pdu(test_ctx, 0x300, secured_pdu, secured_len,
                                  original_data, &original_len, &verify_result);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(SECOC_VERIFY_SUCCESS, verify_result);
    TEST_ASSERT_EQUAL(sizeof(test_data), original_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(test_data, original_data, sizeof(test_data));
    
    /* 清理 */
    if (auth_pdu.data) {
        free(auth_pdu.data);
    }
}

/* 测试验证失败情况 (篡改数据) */
void test_secoc_verify_modified_pdu_should_fail(void) {
    /* 设置配置 */
    secoc_pdu_config_t config = {
        .pdu_id = 0x400,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,
        .mac_len = 64,
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100,
        .use_tx_confirmation = false,
        .enable_replay_protection = false
    };
    
    secoc_add_pdu_config(test_ctx, &config);
    secoc_import_key(test_ctx, 0, test_key, 16);
    
    /* 测试数据 */
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    
    /* 认证PDU */
    secoc_authenticated_pdu_t auth_pdu;
    secoc_authenticate_tx_pdu(test_ctx, 0x400, test_data, sizeof(test_data), &auth_pdu);
    
    /* 构建Secured PDU */
    uint8_t secured_pdu[64];
    uint32_t secured_len;
    secoc_build_secured_pdu(&auth_pdu, secured_pdu, &secured_len, sizeof(secured_pdu));
    
    /* 篡改数据 */
    secured_pdu[0] ^= 0xFF;
    
    /* 验证PDU - 应该失败 */
    uint8_t original_data[64];
    uint32_t original_len;
    secoc_verify_result_t verify_result;
    secoc_status_t status = secoc_verify_rx_pdu(test_ctx, 0x400, secured_pdu, secured_len,
                                                 original_data, &original_len, &verify_result);
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);  /* 操作完成 */
    TEST_ASSERT_EQUAL(SECOC_VERIFY_MAC_FAILED, verify_result);  /* 但验证失败 */
    
    /* 清理 */
    if (auth_pdu.data) {
        free(auth_pdu.data);
    }
}

/* 测试统计功能 */
void test_secoc_stats_should_track_operations(void) {
    uint64_t tx_count, rx_count, verify_success, verify_failure;
    
    /* 初始状态 */
    secoc_get_stats(test_ctx, &tx_count, &rx_count, &verify_success, &verify_failure);
    TEST_ASSERT_EQUAL(0, tx_count);
    TEST_ASSERT_EQUAL(0, rx_count);
    
    /* 设置配置并执行操作 */
    secoc_pdu_config_t config = {
        .pdu_id = 0x500,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,
        .mac_len = 64,
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100
    };
    
    secoc_add_pdu_config(test_ctx, &config);
    secoc_import_key(test_ctx, 0, test_key, 16);
    
    uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    secoc_authenticated_pdu_t auth_pdu;
    secoc_authenticate_tx_pdu(test_ctx, 0x500, test_data, sizeof(test_data), &auth_pdu);
    
    /* 检查统计 */
    secoc_get_stats(test_ctx, &tx_count, &rx_count, &verify_success, &verify_failure);
    TEST_ASSERT_EQUAL(1, tx_count);
    
    if (auth_pdu.data) {
        free(auth_pdu.data);
    }
}

/* 测试重置统计 */
void test_secoc_reset_stats_should_clear_counters(void) {
    /* 设置配置并执行操作 */
    secoc_pdu_config_t config = {
        .pdu_id = 0x600,
        .freshness_type = SECOC_FRESHNESS_COUNTER,
        .auth_algo = SECOC_ALGO_AES_CMAC_128,
        .auth_key_slot = 0,
        .freshness_value_len = 32,
        .mac_len = 64,
        .freshness_counter_max = 0xFFFFFFFF,
        .freshness_sync_gap = 100
    };
    
    secoc_add_pdu_config(test_ctx, &config);
    secoc_import_key(test_ctx, 0, test_key, 16);
    
    uint8_t test_data[] = {0x11, 0x22, 0x33, 0x44};
    secoc_authenticated_pdu_t auth_pdu;
    secoc_authenticate_tx_pdu(test_ctx, 0x600, test_data, sizeof(test_data), &auth_pdu);
    
    /* 重置统计 */
    secoc_reset_stats(test_ctx);
    
    uint64_t tx_count, rx_count, verify_success, verify_failure;
    secoc_get_stats(test_ctx, &tx_count, &rx_count, &verify_success, &verify_failure);
    TEST_ASSERT_EQUAL(0, tx_count);
    TEST_ASSERT_EQUAL(0, rx_count);
    TEST_ASSERT_EQUAL(0, verify_success);
    TEST_ASSERT_EQUAL(0, verify_failure);
    
    if (auth_pdu.data) {
        free(auth_pdu.data);
    }
}

/* 主函数 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_secoc_init_should_return_valid_context);
    RUN_TEST(test_secoc_import_key_should_succeed);
    RUN_TEST(test_secoc_import_key_invalid_params_should_fail);
    RUN_TEST(test_secoc_get_key_invalid_slot_should_fail);
    RUN_TEST(test_secoc_clear_key_should_succeed);
    RUN_TEST(test_secoc_add_pdu_config_should_succeed);
    RUN_TEST(test_secoc_add_duplicate_pdu_config_should_fail);
    RUN_TEST(test_secoc_get_nonexistent_pdu_config_should_return_null);
    RUN_TEST(test_secoc_remove_pdu_config_should_succeed);
    RUN_TEST(test_secoc_authenticate_and_verify_pdu_should_succeed);
    RUN_TEST(test_secoc_verify_modified_pdu_should_fail);
    RUN_TEST(test_secoc_stats_should_track_operations);
    RUN_TEST(test_secoc_reset_stats_should_clear_counters);
    
    return UNITY_END();
}

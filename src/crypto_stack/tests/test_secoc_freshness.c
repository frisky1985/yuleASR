/**
 * @file test_secoc_freshness.c
 * @brief SecOC Freshness Module Unit Tests
 * @version 1.0
 * @date 2026-04-25
 */

#include "unity.h"
#include "secoc_freshness.h"
#include <string.h>

static secoc_freshness_manager_t *test_mgr = NULL;

void setUp(void) {
    test_mgr = secoc_freshness_init();
    TEST_ASSERT_NOT_NULL(test_mgr);
}

void tearDown(void) {
    if (test_mgr) {
        secoc_freshness_deinit(test_mgr);
        test_mgr = NULL;
    }
}

/* 测试初始化 */
void test_freshness_init_should_return_valid_manager(void) {
    TEST_ASSERT_NOT_NULL(test_mgr);
    TEST_ASSERT_TRUE(test_mgr->initialized);
    TEST_ASSERT_EQUAL(0, test_mgr->num_entries);
}

/* 测试注册计数器Freshness值 */
void test_freshness_register_counter_should_succeed(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0x100, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_MASTER, 0xFFFFFFFF, 100
    );
    
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(0x100, entry->pdu_id);
    TEST_ASSERT_EQUAL(SECOC_FRESHNESS_COUNTER, entry->type);
    TEST_ASSERT_EQUAL(SECOC_SYNC_MASTER, entry->sync_mode);
    TEST_ASSERT_EQUAL(SECOC_FV_STATE_NOT_SYNC, entry->state);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, entry->max_counter);
    TEST_ASSERT_EQUAL(100, entry->sync_gap);
}

/* 测试重复注册 */
void test_freshness_register_duplicate_should_fail(void) {
    secoc_freshness_register(test_mgr, 0x100, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0x100, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_SLAVE, 0xFFFFFFFF, 100
    );
    
    TEST_ASSERT_NULL(entry);
}

/* 测试查找Freshness条目 */
void test_freshness_find_should_return_entry(void) {
    secoc_freshness_register(test_mgr, 0x200, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    secoc_fv_entry_t *found = secoc_freshness_find(test_mgr, 0x200);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(0x200, found->pdu_id);
    
    secoc_fv_entry_t *not_found = secoc_freshness_find(test_mgr, 0x999);
    TEST_ASSERT_NULL(not_found);
}

/* 测试注销Freshness值 */
void test_freshness_unregister_should_succeed(void) {
    secoc_freshness_register(test_mgr, 0x300, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    secoc_status_t status = secoc_freshness_unregister(test_mgr, 0x300);
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    secoc_fv_entry_t *found = secoc_freshness_find(test_mgr, 0x300);
    TEST_ASSERT_NULL(found);
}

/* 测试Master模式的计数器获取 */
void test_freshness_get_tx_counter_master_should_succeed(void) {
    secoc_freshness_register(test_mgr, 0x400, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    uint64_t freshness;
    uint8_t freshness_len;
    secoc_status_t status = secoc_freshness_get_tx_counter(
        test_mgr, 0x400, &freshness, &freshness_len
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(1, freshness);  /* 从1开始 */
    TEST_ASSERT_EQUAL(4, freshness_len);
}

/* 测试Slave模式未同步时应失败 */
void test_freshness_get_tx_counter_slave_not_synced_should_fail(void) {
    secoc_freshness_register(test_mgr, 0x500, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_SLAVE, 0xFFFFFFFF, 100);
    
    uint64_t freshness;
    uint8_t freshness_len;
    secoc_status_t status = secoc_freshness_get_tx_counter(
        test_mgr, 0x500, &freshness, &freshness_len
    );
    
    TEST_ASSERT_EQUAL(SECOC_ERROR_SYNC_FAILED, status);
}

/* 测试计数器递增 */
void test_freshness_increment_counter_should_work(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0x600, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_MASTER, 0xFFFFFFFF, 100
    );
    
    uint64_t freshness1, freshness2;
    uint8_t len1, len2;
    
    secoc_freshness_get_tx_counter(test_mgr, 0x600, &freshness1, &len1);
    secoc_freshness_increment_counter(entry);
    secoc_freshness_get_tx_counter(test_mgr, 0x600, &freshness2, &len2);
    
    TEST_ASSERT_EQUAL(freshness1 + 1, freshness2);
}

/* 测试计数器溢出检测 */
void test_freshness_counter_overflow_should_fail(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0x700, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_MASTER, 10, 5
    );
    
    /* 设置计数器接近最大值 */
    entry->tx_counter = 10;
    
    uint64_t freshness;
    uint8_t freshness_len;
    secoc_status_t status = secoc_freshness_get_tx_counter(
        test_mgr, 0x700, &freshness, &freshness_len
    );
    
    TEST_ASSERT_EQUAL(SECOC_ERROR_FRESHNESS_FAILED, status);
    TEST_ASSERT_EQUAL(SECOC_FV_STATE_OVERFLOW, entry->state);
}

/* 测试验证计数器Freshness值 */
void test_freshness_verify_counter_should_succeed(void) {
    secoc_freshness_register(test_mgr, 0x800, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    secoc_status_t status = secoc_freshness_verify_counter(
        test_mgr, 0x800, 10, 4
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    secoc_fv_entry_t *entry = secoc_freshness_find(test_mgr, 0x800);
    TEST_ASSERT_EQUAL(10, entry->rx_counter);
    TEST_ASSERT_EQUAL(10, entry->last_accepted_value);
}

/* 测试重放攻击检测 */
void test_freshness_verify_replay_should_detect(void) {
    secoc_freshness_register(test_mgr, 0x900, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    /* 先验证一个值 */
    secoc_freshness_verify_counter(test_mgr, 0x900, 100, 4);
    
    /* 尝试验证一个更小的值（重放攻击） */
    secoc_status_t status = secoc_freshness_verify_counter(
        test_mgr, 0x900, 50, 4
    );
    
    TEST_ASSERT_EQUAL(SECOC_ERROR_REPLAY_DETECTED, status);
}

/* 测试时间戳模式 */
void test_freshness_timestamp_mode_should_work(void) {
    secoc_freshness_register(test_mgr, 0xA00, SECOC_FRESHNESS_TIMESTAMP, 
                            SECOC_SYNC_MASTER, 0, 0);
    
    uint64_t freshness;
    uint8_t freshness_len;
    secoc_status_t status = secoc_freshness_get_tx_timestamp(
        test_mgr, 0xA00, &freshness, &freshness_len
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(4, freshness_len);
    TEST_ASSERT(freshness > 0);
}

/* 测试行程计数器模式 */
void test_freshness_trip_mode_should_work(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0xB00, SECOC_FRESHNESS_TRIP_COUNTER, 
        SECOC_SYNC_MASTER, 0, 0
    );
    
    /* 设置行程计数器 */
    entry->trip_counter = 5;
    entry->reset_counter = 3;
    entry->tx_counter = 100;
    
    uint64_t freshness;
    uint8_t freshness_len;
    secoc_status_t status = secoc_freshness_get_tx_trip(
        test_mgr, 0xB00, &freshness, &freshness_len
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(8, freshness_len);
    
    /* 验证组合值 */
    uint32_t trip = (freshness >> 32) & 0xFFFFFFFF;
    uint32_t reset = (freshness >> 24) & 0xFF;
    uint64_t counter = freshness & 0xFFFFFF;
    
    TEST_ASSERT_EQUAL(5, trip);
    TEST_ASSERT_EQUAL(3, reset);
    TEST_ASSERT_EQUAL(100, counter);
}

/* 测试同步请求创建 */
void test_freshness_create_sync_request_should_succeed(void) {
    secoc_freshness_register(test_mgr, 0xC00, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_SLAVE, 0xFFFFFFFF, 100);
    
    secoc_sync_request_t request;
    secoc_status_t status = secoc_freshness_create_sync_request(
        test_mgr, 0xC00, &request
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(SECOC_SYNC_REQ, request.header.msg_type);
    TEST_ASSERT_EQUAL(0x0C, request.header.pdu_id_high);
    TEST_ASSERT_EQUAL(0x00, request.header.pdu_id_low);
}

/* 测试同步响应处理 */
void test_freshness_handle_sync_response_should_succeed(void) {
    secoc_freshness_register(test_mgr, 0xD00, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_SLAVE, 0xFFFFFFFF, 100);
    
    /* 创建模拟响应 */
    secoc_sync_response_t response = {
        .header = {
            .msg_type = SECOC_SYNC_RES,
            .pdu_id_high = 0x0D,
            .pdu_id_low = 0x00
        },
        .master_fv = 1000,
        .trip_counter = 5,
        .reset_counter = 2,
        .master_timestamp = 12345
    };
    /* 设置MAC */
    for (int i = 0; i < 8; i++) {
        response.mac[i] = (uint8_t)((1000 >> (i * 8)) & 0xFF);
    }
    
    secoc_status_t status = secoc_freshness_handle_sync_response(
        test_mgr, 0xD00, &response
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    secoc_fv_entry_t *entry = secoc_freshness_find(test_mgr, 0xD00);
    TEST_ASSERT_TRUE(entry->synchronized);
    TEST_ASSERT_EQUAL(1000, entry->rx_counter);
    TEST_ASSERT_EQUAL(5, entry->trip_counter);
    TEST_ASSERT_EQUAL(2, entry->reset_counter);
}

/* 测试Master处理同步请求 */
void test_freshness_handle_sync_request_should_succeed(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0xE00, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_MASTER, 0xFFFFFFFF, 100
    );
    entry->tx_counter = 500;
    entry->trip_counter = 10;
    entry->reset_counter = 3;
    
    secoc_sync_request_t request = {
        .header = {
            .msg_type = SECOC_SYNC_REQ,
            .pdu_id_high = 0x0E,
            .pdu_id_low = 0x00
        },
        .slave_fv_low = 100,
        .timestamp = 1000
    };
    
    secoc_sync_response_t response;
    secoc_status_t status = secoc_freshness_handle_sync_request(
        test_mgr, 0xE00, &request, &response
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(SECOC_SYNC_RES, response.header.msg_type);
    TEST_ASSERT_EQUAL(500, response.master_fv);
    TEST_ASSERT_EQUAL(10, response.trip_counter);
    TEST_ASSERT_EQUAL(3, response.reset_counter);
}

/* 测试同步广播 */
void test_freshness_create_sync_broadcast_should_succeed(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0xF00, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_MASTER, 0xFFFFFFFF, 100
    );
    entry->tx_counter = 999;
    
    secoc_sync_response_t broadcast;
    secoc_status_t status = secoc_freshness_create_sync_broadcast(
        test_mgr, 0xF00, &broadcast
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(SECOC_SYNC_BROADCAST, broadcast.header.msg_type);
    TEST_ASSERT_EQUAL(999, broadcast.master_fv);
}

/* 测试通用接口 */
void test_freshness_get_tx_value_should_dispatch_correctly(void) {
    /* 计数器模式 */
    secoc_freshness_register(test_mgr, 0x1000, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    uint64_t freshness;
    uint8_t freshness_len;
    secoc_status_t status = secoc_freshness_get_tx_value(
        test_mgr, 0x1000, &freshness, &freshness_len
    );
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    
    /* 时间戳模式 */
    secoc_freshness_register(test_mgr, 0x1001, SECOC_FRESHNESS_TIMESTAMP, 
                            SECOC_SYNC_MASTER, 0, 0);
    
    status = secoc_freshness_get_tx_value(
        test_mgr, 0x1001, &freshness, &freshness_len
    );
    TEST_ASSERT_EQUAL(SECOC_OK, status);
}

/* 测试通用验证接口 */
void test_freshness_verify_should_dispatch_correctly(void) {
    secoc_freshness_register(test_mgr, 0x1100, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_MASTER, 0xFFFFFFFF, 100);
    
    secoc_verify_result_t result;
    secoc_status_t status = secoc_freshness_verify(
        test_mgr, 0x1100, 100, 4, &result
    );
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_EQUAL(SECOC_VERIFY_SUCCESS, result);
}

/* 测试需要同步检查 */
void test_freshness_need_sync_should_return_true_for_master(void) {
    secoc_fv_entry_t *entry = secoc_freshness_register(
        test_mgr, 0x1200, SECOC_FRESHNESS_COUNTER, 
        SECOC_SYNC_MASTER, 0xFFFFFFFF, 100
    );
    entry->last_sync_time = 0;
    
    /* 使用大的时间值模拟超时 */
    bool need_sync = secoc_freshness_need_sync(test_mgr, 0x1200, 
                                                SECOC_SYNC_MASTER_INTERVAL_MS * 1000 + 1);
    TEST_ASSERT_TRUE(need_sync);
}

/* 测试强制同步 */
void test_freshness_force_sync_should_succeed(void) {
    secoc_freshness_register(test_mgr, 0x1300, SECOC_FRESHNESS_COUNTER, 
                            SECOC_SYNC_SLAVE, 0xFFFFFFFF, 100);
    
    secoc_fv_entry_t *entry = secoc_freshness_find(test_mgr, 0x1300);
    entry->synchronized = true;
    
    secoc_status_t status = secoc_freshness_force_sync(test_mgr, 0x1300);
    
    TEST_ASSERT_EQUAL(SECOC_OK, status);
    TEST_ASSERT_FALSE(entry->synchronized);
    TEST_ASSERT_EQUAL(SECOC_FV_STATE_SYNC_IN_PROGRESS, entry->state);
}

/* 测试状态字符串转换 */
void test_freshness_state_str_should_return_valid_strings(void) {
    TEST_ASSERT_EQUAL_STRING("OK", 
                             secoc_freshness_state_str(SECOC_FV_STATE_OK));
    TEST_ASSERT_EQUAL_STRING("NOT_SYNC", 
                             secoc_freshness_state_str(SECOC_FV_STATE_NOT_SYNC));
    TEST_ASSERT_EQUAL_STRING("REPLAY_DETECTED", 
                             secoc_freshness_state_str(SECOC_FV_STATE_REPLAY_DETECTED));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", 
                             secoc_freshness_state_str(999));  /* 无效状态 */
}

/* 主函数 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_freshness_init_should_return_valid_manager);
    RUN_TEST(test_freshness_register_counter_should_succeed);
    RUN_TEST(test_freshness_register_duplicate_should_fail);
    RUN_TEST(test_freshness_find_should_return_entry);
    RUN_TEST(test_freshness_unregister_should_succeed);
    RUN_TEST(test_freshness_get_tx_counter_master_should_succeed);
    RUN_TEST(test_freshness_get_tx_counter_slave_not_synced_should_fail);
    RUN_TEST(test_freshness_increment_counter_should_work);
    RUN_TEST(test_freshness_counter_overflow_should_fail);
    RUN_TEST(test_freshness_verify_counter_should_succeed);
    RUN_TEST(test_freshness_verify_replay_should_detect);
    RUN_TEST(test_freshness_timestamp_mode_should_work);
    RUN_TEST(test_freshness_trip_mode_should_work);
    RUN_TEST(test_freshness_create_sync_request_should_succeed);
    RUN_TEST(test_freshness_handle_sync_response_should_succeed);
    RUN_TEST(test_freshness_handle_sync_request_should_succeed);
    RUN_TEST(test_freshness_create_sync_broadcast_should_succeed);
    RUN_TEST(test_freshness_get_tx_value_should_dispatch_correctly);
    RUN_TEST(test_freshness_verify_should_dispatch_correctly);
    RUN_TEST(test_freshness_need_sync_should_return_true_for_master);
    RUN_TEST(test_freshness_force_sync_should_succeed);
    RUN_TEST(test_freshness_state_str_should_return_valid_strings);
    
    return UNITY_END();
}

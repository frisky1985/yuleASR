/**
 * @file test_safety_nvm.c
 * @brief NvM安全机制单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "../../../src/nvm/nvm.h"
#include "../../../src/nvm/nvm_types.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static uint8_t g_test_buffer[256];

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    memset(g_test_buffer, 0, sizeof(g_test_buffer));
    Nvm_Init();
    
    /* 注册模拟存储设备 */
    Nvm_RegisterStorage(0, NVM_STORAGE_FLASH, 0x00000000, 1024 * 1024);
}

void tearDown(void) {
    Nvm_Deinit();
}

/*==============================================================================
 * 测试用例 - NvM初始化
 *==============================================================================*/

/* Test 1: 模块初始化 */
void test_nvm_init(void) {
    Nvm_ErrorCode_t result = Nvm_Init();
    TEST_ASSERT_EQUAL(NVM_OK, result);
    TEST_ASSERT_TRUE(Nvm_IsInitialized());
}

/* Test 2: 模块反初始化 */
void test_nvm_deinit(void) {
    Nvm_ErrorCode_t result = Nvm_Deinit();
    TEST_ASSERT_EQUAL(NVM_OK, result);
    TEST_ASSERT_FALSE(Nvm_IsInitialized());
    
    /* 重新初始化以便后续测试 */
    Nvm_Init();
}

/* Test 3: 配置块 */
void test_nvm_configure_block(void) {
    Nvm_ErrorCode_t result = Nvm_ConfigureBlock(
        1,                          /* blockId */
        "TestBlock",                /* name */
        NVM_BLOCK_DATA,             /* type */
        0x00010000,                 /* address */
        256,                        /* length */
        true                        /* redundant */
    );
    TEST_ASSERT_EQUAL(NVM_OK, result);
}

/* Test 4: 写入和读取块 */
void test_nvm_write_read_block(void) {
    /* 配置块 */
    Nvm_ConfigureBlock(1, "ReadWriteBlock", NVM_BLOCK_DATA, 0x00010000, 128, false);
    
    /* 准备测试数据 */
    uint8_t write_data[128];
    for (int i = 0; i < 128; i++) {
        write_data[i] = (uint8_t)(i * 2);
    }
    
    /* 写入 */
    Nvm_ErrorCode_t result = Nvm_WriteBlock(1, write_data, 128);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    
    /* 读取 */
    uint8_t read_data[128];
    result = Nvm_ReadBlock(1, read_data, 128);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    
    /* 验证 */
    TEST_ASSERT_EQUAL_MEMORY(write_data, read_data, 128);
}

/* Test 5: 操作未配置块 */
void test_nvm_unconfigured_block(void) {
    /* 尝试读取未配置的块 */
    uint8_t buffer[64];
    Nvm_ErrorCode_t result = Nvm_ReadBlock(999, buffer, 64);
    TEST_ASSERT_NOT_EQUAL(NVM_OK, result);
}

/* Test 6: 块校验 */
void test_nvm_validate_block(void) {
    Nvm_ConfigureBlock(2, "ValidateBlock", NVM_BLOCK_DATA, 0x00020000, 64, false);
    
    uint8_t data[64] = "Test data for validation";
    Nvm_WriteBlock(2, data, 64);
    
    /* 验证块 */
    Nvm_ErrorCode_t result = Nvm_ValidateBlock(2);
    TEST_ASSERT_EQUAL(NVM_OK, result);
}

/* Test 7: 操作红外数据块 */
void test_nvm_redundant_block(void) {
    /* 配置带冗余的块 */
    Nvm_ConfigureBlock(3, "RedundantBlock", NVM_BLOCK_CONFIG, 0x00030000, 256, true);
    
    uint8_t data[256];
    memset(data, 0xAB, 256);
    
    Nvm_ErrorCode_t result = Nvm_WriteBlock(3, data, 256);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    
    uint8_t read_data[256];
    result = Nvm_ReadBlock(3, read_data, 256);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    TEST_ASSERT_EQUAL_MEMORY(data, read_data, 256);
}

/* Test 8: 块操作统计 */
void test_nvm_statistics(void) {
    /* 配置几个块 */
    Nvm_ConfigureBlock(10, "Block10", NVM_BLOCK_DATA, 0x00100000, 64, false);
    Nvm_ConfigureBlock(11, "Block11", NVM_BLOCK_DATA, 0x00100100, 64, false);
    Nvm_ConfigureBlock(12, "Block12", NVM_BLOCK_DATA, 0x00100200, 64, false);
    
    uint32_t blocks_configured, jobs_pending, wear_level;
    Nvm_ErrorCode_t result = Nvm_GetStatistics(&blocks_configured, &jobs_pending, &wear_level);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    
    /* 至少有3个已配置的块（加上setUp中配置的） */
    TEST_ASSERT_GREATER_OR_EQUAL(3, blocks_configured);
}

/* Test 9: 版本信息 */
void test_nvm_version(void) {
    uint8_t major, minor, patch;
    Nvm_ErrorCode_t result = Nvm_GetVersion(&major, &minor, &patch);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    
    /* 版本号应该是有效的 */
    TEST_ASSERT_GREATER_OR_EQUAL(1, major);
}

/* Test 10: 异步操作回调 */
static volatile int g_async_complete = 0;
static Nvm_ErrorCode_t g_async_result;

static void async_callback(Nvm_ErrorCode_t result, void* user_data) {
    (void)user_data;
    g_async_result = result;
    g_async_complete = 1;
}

void test_nvm_async_operations(void) {
    Nvm_ConfigureBlock(20, "AsyncBlock", NVM_BLOCK_DATA, 0x00200000, 128, false);
    
    g_async_complete = 0;
    
    uint8_t data[128] = "Async test data";
    Nvm_ErrorCode_t result = Nvm_WriteBlockAsync(20, data, 128, async_callback, NULL);
    TEST_ASSERT_EQUAL(NVM_OK, result);
    
    /* 模拟MainFunction调用 */
    int timeout = 1000;
    while (!g_async_complete && timeout > 0) {
        Nvm_MainFunction();
        usleep(1000);
        timeout--;
    }
    
    TEST_ASSERT_TRUE(g_async_complete);
    TEST_ASSERT_EQUAL(NVM_OK, g_async_result);
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_nvm_init);
    RUN_TEST(test_nvm_deinit);
    RUN_TEST(test_nvm_configure_block);
    RUN_TEST(test_nvm_write_read_block);
    RUN_TEST(test_nvm_unconfigured_block);
    RUN_TEST(test_nvm_validate_block);
    RUN_TEST(test_nvm_redundant_block);
    RUN_TEST(test_nvm_statistics);
    RUN_TEST(test_nvm_version);
    RUN_TEST(test_nvm_async_operations);
    
    return UNITY_END();
}

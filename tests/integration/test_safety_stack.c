/*
 * @file test_safety_stack.c
 * @brief 安全栈集成测试
 * 
 * 测试E2E-CRC-OS Timing安全机制
 *
 * Test Levels: Integration
 * ASIL Level: D
 */

#include <unity.h>
#include "E2E.h"
#include "Crc.h"
#include "Os.h"
#include "Os_TimingProtection.h"
#include <string.h>

void setUp(void) {
    E2E_Init();
    Os_Init();
    Os_TimingProtection_Init();
}

void tearDown(void) {
    Os_TimingProtection_DeInit();
    Os_DeInit();
}

/**
 * @brief 测试E2E保护的通信
 * @test SAFETY_E2E_001
 */
void test_SafetyStack_E2E_Protected_Communication(void) {
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    E2E_P01ConfigType config;
    E2E_P01ProtectStateType state;
    
    /* 配置E2E */
    config.CRCOffset = 0;
    config.CounterOffset = 8;
    config.DataID = 0x1234;
    config.DataLength = 16;
    
    /* 保护数据 */
    E2E_P01Protect(&config, &state, data);
    
    /* 模拟数据传输 */
    /* 验证CRC已计算 */
    TEST_ASSERT_NOT_EQUAL(0, data[0]);
    
    /* 验证计数器已更新 */
    uint8 counter = (data[1] >> 4) & 0x0F;
    TEST_ASSERT_EQUAL(0, counter);  /* 第一次 */
}

/**
 * @brief 测试安全通信与时间保护集成
 * @test SAFETY_TIMING_001
 */
void test_SafetyStack_Timing_E2E_Integration(void) {
    /* 配置时间预算 */
    Os_TimingProtection_SetExecutionBudget(0, 1000);
    
    /* 启动任务 */
    Os_TimingProtection_TaskStart(0);
    
    /* 执行E2E保护的通信 */
    uint8 data[16];
    E2E_P01Protect(NULL, NULL, data);
    
    /* 结束任务 */
    Os_TimingProtection_TaskEnd(0);
    
    /* 验证任务完成时间在预算内 */
    TEST_ASSERT_FALSE(Os_TimingProtection_IsBudgetExceeded(0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_SafetyStack_E2E_Protected_Communication);
    RUN_TEST(test_SafetyStack_Timing_E2E_Integration);
    return UNITY_END();
}

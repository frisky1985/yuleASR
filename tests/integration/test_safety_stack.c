/*
 * @file test_safety_stack.c
 * @brief 安全栈集成测试
 *
 * 测试 E2E-CRC-OS Timing 安全机制。
 *
 * 说明:
 *  - E2E (Profile 1) 保护/校验直接链接生产实现 src/bsw/services/e2e/src/E2E_P01.c
 *    (自包含、无硬件依赖), 对真实生产代码做保护/校验/篡改检测验证。
 *  - OS Timing Protection: 生产 Os_TimingProtection.c 依赖 FreeRTOS 无法在宿主机
 *    链接; 此处按生产实现 (Os_TimingProtection.c) 的函数签名以“测试替身”实现
 *    执行时间预算语义, 验证任务执行时间保护契约。
 *  - Os_Init/Os_DeInit 在生产 Os.h 中不存在 (OS 初始化 API 为 StartOS), 已移除。
 *    main() 更名为 test_safety_stack_main() 由集成测试 runner 统一调度。
 *
 * Test Levels: Integration
 * ASIL Level: D
 */

#include <unity.h>
#include "E2E.h"
#include "E2E_P01.h"
#include "Os.h"
#include <string.h>
#include <time.h>

/*==================================================================================================
 * 测试替身: OS Timing Protection (签名与 src/bsw/os/src/Os_TimingProtection.c 一致)
 *================================================================================================*/
#define TIMING_MAX_TASKS   4u

static uint32  g_taskStartUs[TIMING_MAX_TASKS];
static uint32  g_taskBudgetUs[TIMING_MAX_TASKS];
static boolean g_taskExceeded[TIMING_MAX_TASKS];

static uint32 timing_get_us(void)
{
    return (uint32)(clock() * (1000000U / CLOCKS_PER_SEC));
}

void Os_StartTaskExecutionTiming(TaskType TaskID)
{
    if (TaskID < TIMING_MAX_TASKS)
    {
        g_taskStartUs[TaskID]  = timing_get_us();
        g_taskExceeded[TaskID] = FALSE;
    }
}

void Os_StopTaskExecutionTiming(TaskType TaskID)
{
    (void)TaskID; /* 生产实现仅停止计时 */
}

void Os_CheckTaskExecutionBudget(TaskType TaskID)
{
    if (TaskID < TIMING_MAX_TASKS)
    {
        uint32 elapsed = timing_get_us() - g_taskStartUs[TaskID];
        if (elapsed > g_taskBudgetUs[TaskID])
        {
            g_taskExceeded[TaskID] = TRUE;
        }
    }
}

void Os_TimingProtectionMainFunction(void)
{
    TaskType i;
    for (i = 0u; i < TIMING_MAX_TASKS; i++)
    {
        Os_CheckTaskExecutionBudget(i);
    }
}

/*==================================================================================================
 * 测试用例
 *================================================================================================*/
static void safety_stack_init(void)
{
    TaskType i;
    for (i = 0u; i < TIMING_MAX_TASKS; i++)
    {
        g_taskStartUs[i]  = 0u;
        g_taskBudgetUs[i] = 1000000u; /* 默认 1s 预算 */
        g_taskExceeded[i] = FALSE;
    }
}

/**
 * @brief 测试E2E保护的通信 (真实生产 E2E_P01 实现)
 * @test SAFETY_E2E_001
 */
void test_SafetyStack_E2E_Protected_Communication(void)
{
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8 rxData[16];
    E2E_P01ConfigType config;
    E2E_P01ProtectStateType state;
    E2E_P01CheckStateType checkState;

    /* 配置E2E (字段与生产 E2E_P01ConfigType 一致) */
    memset(&config, 0, sizeof(config));
    memset(&state, 0, sizeof(state));
    config.CRCOffset      = 0;
    config.CounterOffset  = 8;
    config.DataID         = 0x1234;
    config.DataLength     = 16;
    config.DataIDMode     = E2E_P01_DATAID_BOTH;

    /* 第一次保护: 计数器为 0 */
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&config, &state, data));
    TEST_ASSERT_EQUAL(0, (data[8] & 0x0Fu));

    /* 第二次保护: 计数器递增为 1 */
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&config, &state, data));
    TEST_ASSERT_EQUAL(1, (data[8] & 0x0Fu));

    /* 模拟数据传输后校验: 第一次校验为初始状态, 第二次计数器连续 -> OK */
    memcpy(rxData, data, sizeof(rxData));
    memset(&checkState, 0, sizeof(checkState));
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&config, &checkState, rxData));
    TEST_ASSERT_EQUAL(E2E_P_INITIAL, checkState.Status);

    /* 模拟下一次传输 (counter 递增到 2) 后校验: 计数器连续 -> OK */
    E2E_P01Protect(&config, &state, data);
    memcpy(rxData, data, sizeof(rxData));
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&config, &checkState, rxData));
    TEST_ASSERT_EQUAL(E2E_P_OK, checkState.Status);

    /* 篡改检测: 翻转数据字节, CRC 校验必须失败 */
    rxData[4] ^= 0xFFu;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&config, &checkState, rxData));
    TEST_ASSERT_EQUAL(E2E_P_WRONGCRC, checkState.Status);
}

/**
 * @brief 测试安全通信与时间保护集成
 * @test SAFETY_TIMING_001
 */
void test_SafetyStack_Timing_E2E_Integration(void)
{
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    E2E_P01ConfigType config;
    E2E_P01ProtectStateType state;

    safety_stack_init();

    /* 配置时间预算 (1s, 远大于实际执行时间) */
    g_taskBudgetUs[0] = 1000000u;

    /* 启动任务计时 */
    Os_StartTaskExecutionTiming(0);

    /* 执行E2E保护的通信 */
    memset(&config, 0, sizeof(config));
    memset(&state, 0, sizeof(state));
    config.CRCOffset     = 0;
    config.CounterOffset = 8;
    config.DataID        = 0x1234;
    config.DataLength    = 16;
    config.DataIDMode    = E2E_P01_DATAID_BOTH;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&config, &state, data));

    /* 结束任务并检查预算 */
    Os_StopTaskExecutionTiming(0);
    Os_CheckTaskExecutionBudget(0);
    TEST_ASSERT_FALSE(g_taskExceeded[0]);

    /* 超预算场景: 预算 1us, 忙等 5ms 后必须判定超时 */
    g_taskBudgetUs[0] = 1u;
    Os_StartTaskExecutionTiming(0);
    {
        clock_t start = clock();
        while ((clock() - start) * 1000 / CLOCKS_PER_SEC < 5)
        {
            /* busy wait */
        }
    }
    Os_TimingProtectionMainFunction();
    TEST_ASSERT_TRUE(g_taskExceeded[0]);
}

int test_safety_stack_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_SafetyStack_E2E_Protected_Communication);
    RUN_TEST(test_SafetyStack_Timing_E2E_Integration);
    return UNITY_END();
}

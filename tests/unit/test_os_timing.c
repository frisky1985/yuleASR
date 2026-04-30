/*
 * @file test_os_timing.c
 * @brief OS Timing Protection 单元测试
 * 
 * 测试范围:
 * - 执行时间监控 (Execution Time Budget)
 * - 锁定时间监控 (Lock Time Budget)
 * - 断开时间监控 (Inter-arrival Time)
 * - 中止处理器测试
 * - ASIL-D级安全机制
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: D
 */

#include <unity.h>
#include "Os.h"
#include "Os_TimingProtection.h"
#include <string.h>

/* ================================ 测试数据 ================================ */

static boolean budgetExceeded;
static boolean handlerCalled;
static Os_BudgetType exceededBudget;

void setUp(void) {
    budgetExceeded = FALSE;
    handlerCalled = FALSE;
    exceededBudget = 0;
    
    Os_Init();
    Os_TimingProtection_Init();
}

void tearDown(void) {
    Os_TimingProtection_DeInit();
    Os_DeInit();
}

/* ================================ Callback ================================ */

void Os_TimingProtection_BudgetExceeded(Os_BudgetType Budget) {
    budgetExceeded = TRUE;
    handlerCalled = TRUE;
    exceededBudget = Budget;
}

/* ================================ 执行时间测试 ================================ */

/**
 * @brief 测试任务执行时间监控
 * @test OS_TIMING_EXEC_001
 */
void test_Os_Timing_Execution_Budget(void) {
    Os_TaskType task = 0;
    Os_TickType budget = 1000;  /* 10ms budget @100us tick */
    
    /* 配置执行时间预算 */
    Std_ReturnType result = Os_TimingProtection_SetExecutionBudget(task, budget);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 验证预算已设置 */
    Os_TickType currentBudget = Os_TimingProtection_GetExecutionBudget(task);
    TEST_ASSERT_EQUAL(budget, currentBudget);
}

/**
 * @brief 测试执行时间超时
 * @test OS_TIMING_EXEC_002
 */
void test_Os_Timing_Execution_Exceeded(void) {
    Os_TaskType task = 0;
    Os_TickType budget = 100;  /* 很短的预算 */
    
    Os_TimingProtection_SetExecutionBudget(task, budget);
    
    /* 模拟任务开始执行 */
    Os_TimingProtection_TaskStart(task);
    
    /* 模拟任务执行超过预算 */
    for (uint16 i = 0; i < 200; i++) {
        Os_TimingProtection_MainFunction();
    }
    
    /* 模拟任务结束执行 */
    Os_TimingProtection_TaskEnd(task);
    
    /* 验证预算超出事件已触发 */
    TEST_ASSERT_TRUE(budgetExceeded);
    TEST_ASSERT_TRUE(handlerCalled);
}

/* ================================ 锁定时间测试 ================================ */

/**
 * @brief 测试资源锁定时间监控
 * @test OS_TIMING_LOCK_001
 */
void test_Os_Timing_Lock_Budget(void) {
    Os_ResourceType resource = 0;
    Os_TickType budget = 500;  /* 5ms lock budget */
    
    /* 配置锁定时间预算 */
    Std_ReturnType result = Os_TimingProtection_SetLockBudget(resource, budget);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试资源锁定超时
 * @test OS_TIMING_LOCK_002
 */
void test_Os_Timing_Lock_Exceeded(void) {
    Os_ResourceType resource = 0;
    Os_TickType budget = 50;  /* 很短的锁定预算 */
    
    Os_TimingProtection_SetLockBudget(resource, budget);
    
    /* 获取资源 */
    Os_GetResource(resource);
    Os_TimingProtection_LockStart(resource);
    
    /* 模拟长时间锁定 */
    for (uint16 i = 0; i < 100; i++) {
        Os_TimingProtection_MainFunction();
    }
    
    /* 释放资源 */
    Os_TimingProtection_LockEnd(resource);
    Os_ReleaseResource(resource);
    
    /* 验证锁定超时检测 */
    TEST_ASSERT_TRUE(budgetExceeded);
}

/* ================================ 间隔时间测试 ================================ */

/**
 * @brief 测试任务间隔时间监控
 * @test OS_TIMING_ARRIVAL_001
 */
void test_Os_Timing_InterArrival_Time(void) {
    Os_TaskType task = 0;
    Os_TickType minArrival = 1000;  /* 最小间隔时间 */
    
    /* 配置间隔时间 */
    Std_ReturnType result = Os_TimingProtection_SetInterArrivalTime(task, minArrival);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试任务激活过快
 * @test OS_TIMING_ARRIVAL_002
 */
void test_Os_Timing_Arrival_TooFast(void) {
    Os_TaskType task = 0;
    Os_TickType minArrival = 1000;
    
    Os_TimingProtection_SetInterArrivalTime(task, minArrival);
    
    /* 第一次激活 */
    Os_TimingProtection_TaskActivated(task);
    
    /* 快速再次激活 (小于最小间隔时间) */
    for (uint16 i = 0; i < 100; i++) {
        Os_TimingProtection_MainFunction();
    }
    Os_TimingProtection_TaskActivated(task);
    
    /* 验证激活过快检测 */
    TEST_ASSERT_TRUE(budgetExceeded);
}

/* ================================ 中断禁止测试 ================================ */

/**
 * @brief 测试时间保护中断禁止
 * @test OS_TIMING_INT_001
 */
void test_Os_Timing_Interrupt_Disabled(void) {
    /* 开始一个时间监控区域 */
    Os_TimingProtection_StartMonitor();
    
    /* 禁止中断 */
    Os_DisableAllInterrupts();
    
    /* 验证中断被禁止 */
    TEST_ASSERT_FALSE(Os_IsInterruptEnabled());
    
    /* 恢复中断 */
    Os_EnableAllInterrupts();
    
    Os_TimingProtection_EndMonitor();
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* 执行时间测试 */
    RUN_TEST(test_Os_Timing_Execution_Budget);
    RUN_TEST(test_Os_Timing_Execution_Exceeded);
    
    /* 锁定时间测试 */
    RUN_TEST(test_Os_Timing_Lock_Budget);
    RUN_TEST(test_Os_Timing_Lock_Exceeded);
    
    /* 间隔时间测试 */
    RUN_TEST(test_Os_Timing_InterArrival_Time);
    RUN_TEST(test_Os_Timing_Arrival_TooFast);
    
    /* 中断禁止测试 */
    RUN_TEST(test_Os_Timing_Interrupt_Disabled);
    
    return UNITY_END();
}

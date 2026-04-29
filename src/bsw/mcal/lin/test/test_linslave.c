/**
 * @file test_linslave.c
 * @brief LinSlave 模块单元测试
 * @version 1.0.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "LinSlave.h"
#include "LinSlave_Pid.h"
#include "LinSlave_Checksum.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 模拟回调标志 */
static volatile int rx_callback_called = 0;
static volatile int error_callback_called = 0;
static volatile LinSlave_ErrorType last_error = LINSLAVE_ERROR_NONE;
static volatile uint8 callback_checksum_type = 0;

/* 测试宏 */
#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, expected, actual); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, expected, actual, __FILE__, __LINE__); \
        } \
    } while(0)

/* 测试回调函数 */
void TestRxCallback(uint8 Pid, uint8* ResponseDataPtr, uint8* ResponseLengthPtr, uint8* ChecksumTypePtr)
{
    (void)Pid;  /* 未使用参数 */
    rx_callback_called = 1;
    
    /* 设置响应数据 */
    ResponseDataPtr[0] = 0x01;
    ResponseDataPtr[1] = 0x02;
    ResponseDataPtr[2] = 0x03;
    *ResponseLengthPtr = 3;
    *ChecksumTypePtr = LINSLAVE_CHECKSUM_TYPE;  /* 设置校验和类型 */
}

void TestErrorCallback(LinSlave_ErrorType ErrorCode, uint8 Pid)
{
    (void)Pid;  /* 未使用参数 */
    error_callback_called = 1;
    last_error = ErrorCode;
}

/* PID测试 */
void test_pid_calculation(void)
{
    uint8 pid;
    
    printf("\n=== PID Calculation Tests ===\n");
    
    /* 测试ID 0 */
    pid = LinSlave_CalculatePid(0);
    TEST_ASSERT_EQ(0x80, pid);  /* 0x00 | 0x80 = 0x80 */
    
    /* 测试ID 1 */
    pid = LinSlave_CalculatePid(1);
    TEST_ASSERT_EQ(0xC1, pid);  /* 预计结果 */
    
    /* 测试ID 5 */
    pid = LinSlave_CalculatePid(5);
    TEST_ASSERT(LinSlave_ValidatePid(pid));
    
    /* 测试ID 59 (最大有效ID) */
    pid = LinSlave_CalculatePid(59);
    TEST_ASSERT(LinSlave_ValidatePid(pid));
    
    /* 测试ID 60 (超出范围) */
    pid = LinSlave_CalculatePid(60);
    TEST_ASSERT(!LinSlave_ValidatePid(pid));
}

/* 校验和测试 */
void test_checksum(void)
{
    uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    uint8 csum_classic, csum_enhanced;
    boolean valid;
    
    printf("\n=== Checksum Tests ===\n");
    
    /* 经典校验和 (仅数据) */
    csum_classic = LinSlave_CalculateChecksum(data, 4, 0, LINSLAVE_CHECKSUM_CLASSIC);
    printf("  Classic checksum: 0x%02X\n", csum_classic);
    TEST_ASSERT(csum_classic != 0xFF);  /* 应该计算出有效值 */
    
    valid = LinSlave_ValidateChecksum(data, 4, 0, LINSLAVE_CHECKSUM_CLASSIC, csum_classic);
    TEST_ASSERT(valid);
    
    /* 增强校验和 (包含PID) */
    csum_enhanced = LinSlave_CalculateChecksum(data, 4, 0xC1, LINSLAVE_CHECKSUM_ENHANCED);
    printf("  Enhanced checksum: 0x%02X\n", csum_enhanced);
    TEST_ASSERT(csum_enhanced != 0xFF);
    
    valid = LinSlave_ValidateChecksum(data, 4, 0xC1, LINSLAVE_CHECKSUM_ENHANCED, csum_enhanced);
    TEST_ASSERT(valid);
    
    /* 错误校验和验证 */
    valid = LinSlave_ValidateChecksum(data, 4, 0xC1, LINSLAVE_CHECKSUM_ENHANCED, csum_enhanced + 1);
    TEST_ASSERT(!valid);
}

/* 初始化测试 */
void test_init(void)
{
    LinSlave_StatusType status;
    
    printf("\n=== Initialization Tests ===\n");
    
    /* 有效初始化 */
    status = LinSlave_Init(&LinSlave_DefaultConfig);
    TEST_ASSERT_EQ(LINSLAVE_OK, status);
    TEST_ASSERT_EQ(LINSLAVE_STATE_IDLE, LinSlave_GetState());
    
    /* 反初始化 */
    LinSlave_DeInit();
    TEST_ASSERT_EQ(LINSLAVE_STATE_UNINIT, LinSlave_GetState());
    
    /* NULL指针测试 */
    status = LinSlave_Init(NULL);
    TEST_ASSERT_EQ(LINSLAVE_NOT_OK, status);
}

/* 回调测试 */
void test_callbacks(void)
{
    printf("\n=== Callback Tests ===\n");
    
    /* 重新初始化 */
    LinSlave_Init(&LinSlave_DefaultConfig);
    
    /* 注册回调 */
    rx_callback_called = 0;
    error_callback_called = 0;
    
    LinSlave_RegisterRxCallback(TestRxCallback);
    LinSlave_RegisterErrorCallback(TestErrorCallback);
    
    /* 模拟完整的报文流程 */
    LinSlave_BreakDetected();
    TEST_ASSERT_EQ(LINSLAVE_STATE_RX_BREAK, LinSlave_GetState());
    
    /* 发送 Sync */
    LinSlave_RxInterruptHandler(0x55);
    TEST_ASSERT_EQ(LINSLAVE_STATE_RX_SYNC, LinSlave_GetState());
    
    /* 发送 PID (NodeId = 5) */
    uint8 pid = LinSlave_CalculatePid(5);
    LinSlave_RxInterruptHandler(pid);
    /* 状态可能变为 RX_DATA 或 IDLE，取决于PID是否匹配 */
    
    LinSlave_DeInit();
}

/* 状态机测试 */
void test_state_machine(void)
{
    printf("\n=== State Machine Tests ===\n");
    
    LinSlave_Init(&LinSlave_DefaultConfig);
    
    /* 测试 Break 检测 */
    LinSlave_BreakDetected();
    TEST_ASSERT_EQ(LINSLAVE_STATE_RX_BREAK, LinSlave_GetState());
    
    /* 测试 Sync 接收 */
    LinSlave_RxInterruptHandler(0x55);
    TEST_ASSERT_EQ(LINSLAVE_STATE_RX_SYNC, LinSlave_GetState());
    
    /* 测试错误的 Sync */
    LinSlave_BreakDetected();
    LinSlave_RxInterruptHandler(0x56);  /* 错误的 Sync */
    TEST_ASSERT_EQ(LINSLAVE_ERROR_SYNC, LinSlave_GetLastError());
    TEST_ASSERT_EQ(LINSLAVE_STATE_IDLE, LinSlave_GetState());
    
    LinSlave_DeInit();
}

/* 响应数据测试 */
void test_response_data(void)
{
    LinSlave_StatusType status;
    uint8 data[] = {0x11, 0x22, 0x33, 0x44};
    
    printf("\n=== Response Data Tests ===\n");
    
    LinSlave_Init(&LinSlave_DefaultConfig);
    
    /* 有效设置 */
    status = LinSlave_SetResponseData(data, 4);
    TEST_ASSERT_EQ(LINSLAVE_OK, status);
    
    /* 无效长度 */
    status = LinSlave_SetResponseData(data, 0);
    TEST_ASSERT_EQ(LINSLAVE_NOT_OK, status);
    
    status = LinSlave_SetResponseData(data, 9);  /* 超出范围 */
    TEST_ASSERT_EQ(LINSLAVE_NOT_OK, status);
    
    /* NULL指针 */
    status = LinSlave_SetResponseData(NULL, 4);
    TEST_ASSERT_EQ(LINSLAVE_NOT_OK, status);
    
    LinSlave_DeInit();
}

/* 主函数 */
int main(void)
{
    printf("========================================\n");
    printf("   LinSlave Module Unit Tests\n");
    printf("========================================\n");
    
    test_pid_calculation();
    test_checksum();
    test_init();
    test_callbacks();
    test_state_machine();
    test_response_data();
    
    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED!\n");
        return 1;
    }
}

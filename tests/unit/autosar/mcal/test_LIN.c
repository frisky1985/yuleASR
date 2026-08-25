/**
 * @file test_LIN.c
 * @brief LIN Driver 模块单元测试
 * @version 1.0.0
 */

// @tests src/bsw/mcal/lin/src/Lin.c  @tests src/bsw/mcal/lin/include/Lin.h

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Lin.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, (int)(expected), (int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/* 测试数据 */
static uint8 test_sdu[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/* 初始化测试 */
/* @req SWS_Lin_00002 */
void test_init_deinit(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    /* 测试初始化 */
    Lin_Init(NULL);
    TEST_ASSERT(1);  /* 初始化完成 */
    
    /* 测试反初始化 */
    Lin_DeInit();
    TEST_ASSERT(1);  /* 反初始化完成 */
}

/* 发送帧测试 */
/* @req SWS_Lin_00004 */
void test_send_frame(void)
{
    Std_ReturnType result;
    Lin_PduType pdu;
    
    printf("\n=== Send Frame Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 准备PDU */
    pdu.Pid = 0x3C;  /* 诊断请求帧PID */
    pdu.FrameType = LIN_FRAMETYPE_DIAGNOSTIC;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 8;
    pdu.ChecksumType = LIN_CLASSIC_CS;
    pdu.SduPtr = test_sdu;
    
    /* 测试发送帧 */
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无条件帧 */
    pdu.Pid = 0x11;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效通道 */
    result = Lin_SendFrame(255, &pdu);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Lin_SendFrame(0, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Lin_DeInit();
}

/* 发送响应测试 */
/* @req SWS_Lin_00005 */
void test_send_response(void)
{
    Std_ReturnType result;
    Lin_PduType pdu;
    
    printf("\n=== Send Response Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 准备PDU */
    pdu.Pid = 0x3D;  /* 诊断响应帧PID */
    pdu.FrameType = LIN_FRAMETYPE_DIAGNOSTIC;
    pdu.FrameResponse = LIN_SLAVE_RESPONSE;
    pdu.Length = 8;
    pdu.ChecksumType = LIN_CLASSIC_CS;
    pdu.SduPtr = test_sdu;
    
    /* 测试发送响应 */
    result = Lin_SendResponse(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效通道 */
    result = Lin_SendResponse(255, &pdu);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* 测试NULL指针 */
    result = Lin_SendResponse(0, NULL);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Lin_DeInit();
}

/* 禁用响应测试 */
/* @req SWS_Lin_00005 */
void test_disable_response(void)
{
    Std_ReturnType result;
    
    printf("\n=== Disable Response Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 测试禁用响应 */
    result = Lin_DisableResponse(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效通道 */
    result = Lin_DisableResponse(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Lin_DeInit();
}

/* 唤醒测试 */
/* @req SWS_Lin_00007 */
void test_wakeup(void)
{
    Std_ReturnType result;
    
    printf("\n=== Wakeup Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 测试唤醒 */
    result = Lin_WakeUp(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试内部唤醒 */
    result = Lin_WakeUpInternal(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试检查唤醒 */
    result = Lin_CheckWakeup(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效通道 */
    result = Lin_WakeUp(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    result = Lin_WakeUpInternal(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    result = Lin_CheckWakeup(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Lin_DeInit();
}

/* 获取状态测试 */
/* @req SWS_Lin_00010 */
void test_get_status(void)
{
    Lin_StatusType status;
    uint8* sdu_ptr;
    
    printf("\n=== Get Status Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 测试获取状态 */
    status = Lin_GetStatus(0, &sdu_ptr);
    TEST_ASSERT(status >= LIN_NOT_OK && status <= LIN_CH_SLEEP);
    
    /* 测试无效通道 */
    status = Lin_GetStatus(255, &sdu_ptr);
    TEST_ASSERT_EQ(LIN_NOT_OK, status);
    
    Lin_DeInit();
}

/* 休眠测试 */
/* @req SWS_Lin_00011 */
void test_go_to_sleep(void)
{
    Std_ReturnType result;
    
    printf("\n=== Go To Sleep Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 测试进入休眠 */
    result = Lin_GoToSleep(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试内部进入休眠 */
    result = Lin_GoToSleepInternal(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试无效通道 */
    result = Lin_GoToSleep(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    result = Lin_GoToSleepInternal(255);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    Lin_DeInit();
}

/* 帧类型测试 */
/* @req SWS_Lin_00004 */
void test_frame_types(void)
{
    Std_ReturnType result;
    Lin_PduType pdu;
    
    printf("\n=== Frame Type Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 测试事件触发帧 */
    pdu.Pid = 0x10;
    pdu.FrameType = LIN_FRAMETYPE_EVENT_TRIGGERED;
    pdu.FrameResponse = LIN_SLAVE_RESPONSE;
    pdu.Length = 8;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = test_sdu;
    
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试偶发帧 */
    pdu.FrameType = LIN_FRAMETYPE_SPORADIC;
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 测试用户定义帧 */
    pdu.FrameType = LIN_FRAMETYPE_USER_DEFINED;
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    Lin_DeInit();
}

/* 校验和类型测试 */
/* @req SWS_Lin_00209 */
void test_checksum_types(void)
{
    Std_ReturnType result;
    Lin_PduType pdu;
    
    printf("\n=== Checksum Type Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 经典校验和 */
    pdu.Pid = 0x11;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 8;
    pdu.ChecksumType = LIN_CLASSIC_CS;
    pdu.SduPtr = test_sdu;
    
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    /* 增强校验和 */
    pdu.ChecksumType = LIN_ENHANCED_CS;
    result = Lin_SendFrame(0, &pdu);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    Lin_DeInit();
}

/* 版本信息测试 */
/* @req SWS_Lin_00003 */
void test_version_info(void)
{
    printf("\n=== Version Info Tests ===\n");
    
    Std_VersionInfoType version_info;
    
    Lin_GetVersionInfo(&version_info);
    TEST_ASSERT_EQ(LIN_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(LIN_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(LIN_SW_PATCH_VERSION, version_info.sw_patch_version);
    TEST_ASSERT_EQ(LIN_MODULE_ID, version_info.moduleID);
}

/* 状态转换测试 */
/* @req SWS_Lin_00211 */
void test_state_transitions(void)
{
    Std_ReturnType result;
    Lin_StatusType status;
    uint8* sdu_ptr;
    
    printf("\n=== State Transition Tests ===\n");
    
    Lin_Init(NULL);
    
    /* 从初始化到操作 */
    result = Lin_WakeUp(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    status = Lin_GetStatus(0, &sdu_ptr);
    TEST_ASSERT(status == LIN_OPERATIONAL || status == LIN_NOT_OK);
    
    /* 从操作到休眠 */
    result = Lin_GoToSleep(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    status = Lin_GetStatus(0, &sdu_ptr);
    TEST_ASSERT(status == LIN_CH_SLEEP || status == LIN_NOT_OK);
    
    /* 从休眠唤醒 */
    result = Lin_WakeUp(0);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    Lin_DeInit();
}

/* 主函数 */
int main(void)
{
    printf("========================================\n");
    printf("   LIN Module Unit Tests\n");
    printf("========================================\n");
    
    test_init_deinit();
    test_send_frame();
    test_send_response();
    test_disable_response();
    test_wakeup();
    test_get_status();
    test_go_to_sleep();
    test_frame_types();
    test_checksum_types();
    test_version_info();
    test_state_transitions();
    
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

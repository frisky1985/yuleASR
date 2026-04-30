/*
 * @file test_diagnostic_stack.c
 * @brief 诊断栈集成测试
 * 
 * 测试DCM-DoIP-CanTSyn诊断流程
 *
 * Test Levels: Integration
 * ASIL Level: B
 */

#include <unity.h>
#include "Dcm.h"
#include "DoIP.h"
#include "CanTSyn.h"
#include "PduR.h"
#include <string.h>

void setUp(void) {
    Dcm_Init(NULL_PTR);
    DoIP_Init(NULL_PTR);
    CanTSyn_Init(NULL_PTR);
}

void tearDown(void) {
    CanTSyn_DeInit();
    DoIP_DeInit();
    Dcm_DeInit();
}

/**
 * @brief 测试完整的DoIP诊断流程
 * @test DIAG_STACK_DOIP_001
 */
void test_DiagStack_DoIP_Full_Diagnostic(void) {
    /* 建立DoIP连接 */
    DoIP_RoutingActivationRequest(0x0E00, 0x00);
    
    /* 发送诊断请求 */
    uint8 diagRequest[] = {0x10, 0x01};  /* Session Control */
    DoIP_DiagnosticMessage_Transmit(0x0E00, 0xE000, diagRequest, sizeof(diagRequest));
    
    /* 模拟接收响应 */
    uint8 diagResponse[] = {0x50, 0x01};
    DoIP_RxIndication(0, diagResponse, sizeof(diagResponse));
    
    /* 验证响应已传递到DCM */
    TEST_ASSERT_TRUE(Dcm_IsResponseReceived());
}

/**
 * @brief 测试带时间同步的诊断
 * @test DIAG_STACK_TSYN_001
 */
void test_DiagStack_With_TimeSync(void) {
    /* 执行时间同步 */
    CanTSyn_TransmitSync(0);
    CanTSyn_MainFunction();
    
    /* 发送时间标记的诊断请求 */
    uint8 diagRequest[] = {0x22, 0xF1, 0x90};  /* Read VIN */
    Dcm_ProcessRequest(diagRequest, sizeof(diagRequest));
    
    /* 验证请求包含时间戳 */
    TEST_ASSERT_TRUE(Dcm_HasTimestamp());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_DiagStack_DoIP_Full_Diagnostic);
    RUN_TEST(test_DiagStack_With_TimeSync);
    return UNITY_END();
}

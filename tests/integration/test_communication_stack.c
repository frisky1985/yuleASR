/*
 * @file test_communication_stack.c
 * @brief 通信栈集成测试
 * 
 * 测试COM-PDUR-CANIF-CAN数据流
 * 验证整个通信栈的数据传输
 *
 * Test Levels: Integration
 * ASIL Level: QM
 */

#include <unity.h>
#include "Com.h"
#include "PduR.h"
#include "CanIf.h"
#include "Can.h"
#include <string.h>

static uint8 txBuffer[256];
static uint8 rxBuffer[256];
static boolean dataReceived;

void setUp(void) {
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
    dataReceived = FALSE;
    
    Com_Init(NULL_PTR);
    PduR_Init(NULL_PTR);
    CanIf_Init(NULL_PTR);
}

void tearDown(void) {
    CanIf_DeInit();
    PduR_DeInit();
    Com_DeInit();
}

/**
 * @brief 测试完整的COM发送流程
 * @test COM_STACK_TX_001
 */
void test_ComStack_Full_Tx_Path(void) {
    /* COM层发送 */
    uint8 data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    Com_SendSignal(0, data);
    
    /* 触发COM MainFunction */
    Com_MainFunctionTx();
    
    /* 验证数据已传递到PDUR */
    PduR_MainFunction();
    
    /* 验证数据已传递到CANIF */
    CanIf_MainFunction();
    
    /* 验证数据已传递到CAN */
    TEST_ASSERT_TRUE(Can_HasPendingTransmission());
}

/**
 * @brief 测试完整的COM接收流程
 * @test COM_STACK_RX_001
 */
void test_ComStack_Full_Rx_Path(void) {
    /* 模拟CAN接收 */
    uint8 canData[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    Can_Receive(0, canData, sizeof(canData));
    
    /* CANIF处理 */
    CanIf_RxIndication(0, canData, sizeof(canData));
    
    /* PDUR路由 */
    PduR_MainFunction();
    
    /* COM接收 */
    Com_MainFunctionRx();
    
    /* 验证数据已传递到COM */
    uint8 receivedData[8];
    Com_ReceiveSignal(0, receivedData);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(canData, receivedData, sizeof(canData));
}

/**
 * @brief 测试多路COM信号发送
 * @test COM_STACK_MULTI_001
 */
void test_ComStack_Multiple_Signals(void) {
    /* 发送多个信号 */
    uint8 signal1 = 0x11;
    uint16 signal2 = 0x2233;
    uint32 signal3 = 0x44556677;
    
    Com_SendSignal(0, &signal1);
    Com_SendSignal(1, &signal2);
    Com_SendSignal(2, &signal3);
    
    /* 触发发送 */
    Com_MainFunctionTx();
    
    /* 验证所有信号都已打包发送 */
    TEST_ASSERT_TRUE(Com_IsSignalSent(0));
    TEST_ASSERT_TRUE(Com_IsSignalSent(1));
    TEST_ASSERT_TRUE(Com_IsSignalSent(2));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ComStack_Full_Tx_Path);
    RUN_TEST(test_ComStack_Full_Rx_Path);
    RUN_TEST(test_ComStack_Multiple_Signals);
    return UNITY_END();
}

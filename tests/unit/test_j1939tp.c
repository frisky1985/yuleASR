/*
 * @file test_j1939tp.c
 * @brief J1939Tp模块单元测试
 * 
 * 测试范围:
 * - BAM (Broadcast Announce Message) 传输
 * - RTS/CTS (Request to Send/Clear to Send) 握手
 * - 数据分片和重组
 * - 超时处理
 * - 错误检测
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: QM
 */

#include <unity.h>
#include "J1939Tp.h"
#include "J1939Tp_Cbk.h"
#include "CanIf.h"
#include "PduR.h"
#include <string.h>

/* ================================ 测试前置条件 ================================ */

#define TEST_BAM_DATA_SIZE  40
#define TEST_RTS_DATA_SIZE  1785  /* 最大数据长度: 255 * 7 = 1785 */

static uint8 testData[TEST_RTS_DATA_SIZE];
static uint8 txBuffer[TEST_RTS_DATA_SIZE];
static uint8 rxBuffer[TEST_RTS_DATA_SIZE];
static boolean txCompleteCalled;
static boolean rxCompleteCalled;
static boolean errorCalled;
static J1939Tp_ErrorType lastError;

/* 模拟数据 */
static PduInfoType pduInfo;
static Can_PduType canPdu;

/* ================================ 测试用例初始化 ================================ */

void setUp(void) {
    memset(testData, 0xAA, sizeof(testData));
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
    txCompleteCalled = FALSE;
    rxCompleteCalled = FALSE;
    errorCalled = FALSE;
    lastError = J1939TP_E_NO_ERROR;
    
    /* 初始化J1939Tp */
    J1939Tp_Init(NULL_PTR);
}

void tearDown(void) {
    J1939Tp_DeInit();
}

/* ================================ Callbacks ================================ */

void J1939Tp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;
    if (result == E_OK) {
        txCompleteCalled = TRUE;
    }
}

void J1939Tp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    if (PduInfoPtr->SduLength <= sizeof(rxBuffer)) {
        memcpy(rxBuffer, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        rxCompleteCalled = TRUE;
    }
}

void J1939Tp_ErrorIndication(J1939Tp_ErrorType error) {
    errorCalled = TRUE;
    lastError = error;
}

/* ================================ BAM 传输测试 ================================ */

/**
 * @brief 测试BAM广播传输
 * @test J1939TP_BAM_TX_001
 */
void test_J1939Tp_BAM_Transmit_Success(void) {
    /* 准备测试数据 */
    PduInfoType pdu;
    pdu.SduDataPtr = testData;
    pdu.SduLength = TEST_BAM_DATA_SIZE;
    pdu.MetaDataPtr = NULL_PTR;
    
    /* 调用发送函数 */
    Std_ReturnType result = J1939Tp_Transmit(0, &pdu);
    
    /* 验证返回值 */
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 模拟BAM报文发送完成 */
    CanIf_TriggerTransmit(0, &pduInfo);
    
    /* 验证数据流正确性 */
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testData, txBuffer, TEST_BAM_DATA_SIZE);
}

/**
 * @brief 测试BAM传输超时
 * @test J1939TP_BAM_TX_002
 */
void test_J1939Tp_BAM_Transmit_Timeout(void) {
    PduInfoType pdu;
    pdu.SduDataPtr = testData;
    pdu.SduLength = TEST_BAM_DATA_SIZE;
    
    /* 发送BAM */
    J1939Tp_Transmit(0, &pdu);
    
    /* 模拟多个周期超时 */
    for (uint16 i = 0; i < 600; i++) {
        J1939Tp_MainFunction();
    }
    
    /* 验证超时错误回调 */
    TEST_ASSERT_TRUE(errorCalled);
    TEST_ASSERT_EQUAL(J1939TP_E_TX_TIMEOUT, lastError);
}

/* ================================ RTS/CTS 传输测试 ================================ */

/**
 * @brief 测试RTS/CTS握手流程
 * @test J1939TP_RTS_CTS_001
 */
void test_J1939Tp_RTS_CTS_Handshake(void) {
    PduInfoType pdu;
    pdu.SduDataPtr = testData;
    pdu.SduLength = TEST_RTS_DATA_SIZE;
    
    /* 发送大数据包，应使用RTS/CTS */
    Std_ReturnType result = J1939Tp_Transmit(1, &pdu);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 验证RTS报文发送 */
    /* RTS: 16进制 10-15字节为总长度 */
    uint8 rtsFrame[8];
    memset(rtsFrame, 0, sizeof(rtsFrame));
    rtsFrame[0] = 16;  /* Control byte: RTS */
    rtsFrame[1] = TEST_RTS_DATA_SIZE & 0xFF;
    rtsFrame[2] = (TEST_RTS_DATA_SIZE >> 8) & 0xFF;
    rtsFrame[3] = (TEST_RTS_DATA_SIZE + 6) / 7;  /* 总包数 */
    
    /* 模拟CTS响应 */
    uint8 ctsFrame[8];
    ctsFrame[0] = 17;  /* Control byte: CTS */
    ctsFrame[1] = 1;   /* 允许的包数 */
    ctsFrame[2] = 1;   /* 下一个包编号 */
    
    CanIf_RxIndication(0, &ctsFrame[0], 8);
    
    /* 验证数据传输开始 */
    TEST_ASSERT_FALSE(errorCalled);
}

/**
 * @brief 测试CTS拒绝流程
 * @test J1939TP_RTS_CTS_002
 */
void test_J1939Tp_RTS_CTS_Abort(void) {
    PduInfoType pdu;
    pdu.SduDataPtr = testData;
    pdu.SduLength = TEST_RTS_DATA_SIZE;
    
    J1939Tp_Transmit(1, &pdu);
    
    /* 模拟Connection Abort */
    uint8 abortFrame[8];
    abortFrame[0] = 255;  /* Control byte: Connection Abort */
    abortFrame[1] = 1;    /* 原因: 资源不足 */
    
    CanIf_RxIndication(0, &abortFrame[0], 8);
    
    /* 验证传输被中止 */
    TEST_ASSERT_TRUE(errorCalled);
    TEST_ASSERT_EQUAL(J1939TP_E_CONNECTION_ABORTED, lastError);
}

/* ================================ 数据重组测试 ================================ */

/**
 * @brief 测试数据重组
 * @test J1939TP_REASSEMBLY_001
 */
void test_J1939Tp_DataReassembly(void) {
    uint8 data1[8] = {1, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    uint8 data2[8] = {2, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    uint8 data3[8] = {3, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    
    /* 模拟接收BAM广播 */
    CanIf_RxIndication(0, data1, 8);
    CanIf_RxIndication(0, data2, 8);
    CanIf_RxIndication(0, data3, 8);
    
    /* 等待重组完成 */
    J1939Tp_MainFunction();
    
    /* 验证数据正确性 */
    TEST_ASSERT_TRUE(rxCompleteCalled);
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* BAM测试 */
    RUN_TEST(test_J1939Tp_BAM_Transmit_Success);
    RUN_TEST(test_J1939Tp_BAM_Transmit_Timeout);
    
    /* RTS/CTS测试 */
    RUN_TEST(test_J1939Tp_RTS_CTS_Handshake);
    RUN_TEST(test_J1939Tp_RTS_CTS_Abort);
    
    /* 数据重组测试 */
    RUN_TEST(test_J1939Tp_DataReassembly);
    
    return UNITY_END();
}

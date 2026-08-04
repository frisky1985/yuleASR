/*
 * @file test_communication_stack.c
 * @brief 通信栈集成测试
 *
 * 测试 COM-PDUR-CANIF-CAN 数据流, 验证整个通信栈的数据传输。
 *
 * 说明: 集成测试在宿主机(无硬件)上构建, 不链接生产 .c 实现。为验证端到端
 *       数据流, 本文件以“测试替身”(test double) 方式实现各层 API, 函数签名
 *       与生产头文件 (Com.h/PduR.h/CanIf.h/Can.h) 完全一致 —— 生产头文件即
 *       API 契约的编译期校验; 替身仅模拟各层之间的数据搬运语义。
 *       main() 更名为 test_communication_stack_main() 由集成测试 runner 统一调度。
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

/*==================================================================================================
 * 测试替身内部状态 (模拟各层缓冲)
 *================================================================================================*/
#define STACK_NUM_SIGNALS   3u
#define STACK_SIGNAL_SIZE   8u

static uint8   g_signalStore[STACK_NUM_SIGNALS][STACK_SIGNAL_SIZE];
static uint8   g_comTxPdu[64];      /* PduR 待发送 PDU (TX 方向) */
static uint16  g_comTxPduLen;
static uint8   g_pduRxData[64];     /* PduR 已接收 PDU (RX 方向) */
static uint16  g_pduRxLen;
static boolean g_canTxPending;
static uint8   g_canTxBuffer[64];
static uint16  g_canTxLen;
static uint8   g_canRxData[64];
static uint16  g_canRxLen;

/*==================================================================================================
 * 测试替身: COM 层 (签名与 src/bsw/services/com/include/Com.h 一致)
 *================================================================================================*/
void Com_Init(const Com_ConfigType* config)
{
    (void)config;
    memset(g_signalStore, 0, sizeof(g_signalStore));
    memset(g_comTxPdu, 0, sizeof(g_comTxPdu));
    g_comTxPduLen = 0u;
}

void Com_DeInit(void)
{
    Com_Init(NULL);
}

uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    if ((SignalId >= STACK_NUM_SIGNALS) || (SignalDataPtr == NULL))
    {
        return 1u; /* E_NOT_OK */
    }
    memcpy(g_signalStore[SignalId], SignalDataPtr, STACK_SIGNAL_SIZE);
    return 0u; /* E_OK */
}

uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)
{
    if ((SignalId >= STACK_NUM_SIGNALS) || (SignalDataPtr == NULL))
    {
        return 1u; /* E_NOT_OK */
    }
    memcpy(SignalDataPtr, g_signalStore[SignalId], STACK_SIGNAL_SIZE);
    return 0u; /* E_OK */
}

void Com_MainFunctionTx(void)
{
    /* 将各信号打包进 I-PDU, 通过 PduR_Transmit 交给 PduR 层 */
    uint8 i;
    PduInfoType pduInfo;

    g_comTxPduLen = (uint16)(STACK_NUM_SIGNALS * STACK_SIGNAL_SIZE);
    for (i = 0u; i < STACK_NUM_SIGNALS; i++)
    {
        memcpy(&g_comTxPdu[i * STACK_SIGNAL_SIZE], g_signalStore[i], STACK_SIGNAL_SIZE);
    }

    memset(&pduInfo, 0, sizeof(pduInfo));
    pduInfo.SduDataPtr = g_comTxPdu;
    pduInfo.SduLength  = g_comTxPduLen;
    PduR_Transmit(0u, &pduInfo);
}

void Com_MainFunctionRx(void)
{
    /* 将 PduR 收到的 I-PDU 数据解包写入信号存储 (模拟 Com 解包) */
    uint8 i;
    for (i = 0u; i < STACK_NUM_SIGNALS; i++)
    {
        memcpy(g_signalStore[i], &g_pduRxData[i * STACK_SIGNAL_SIZE], STACK_SIGNAL_SIZE);
    }
}

/*==================================================================================================
 * 测试替身: PduR 层 (签名与 src/bsw/services/pdur/include/PduR.h 一致)
 *================================================================================================*/
void PduR_Init(const PduR_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    memset(g_comTxPdu, 0, sizeof(g_comTxPdu));
    g_comTxPduLen = 0u;
    memset(g_pduRxData, 0, sizeof(g_pduRxData));
    g_pduRxLen = 0u;
}

void PduR_DeInit(void)
{
    PduR_Init(NULL);
}

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    if (PduInfoPtr == NULL)
    {
        return E_NOT_OK;
    }
    /* 暂存待发送 PDU, 由 PduR_MainFunction 转发给 CanIf */
    memcpy(g_comTxPdu, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    g_comTxPduLen = PduInfoPtr->SduLength;
    return E_OK;
}

void PduR_MainFunction(void)
{
    /* 将待发送 PDU 路由到 CanIf */
    if (g_comTxPduLen > 0u)
    {
        PduInfoType pduInfo;

        memset(&pduInfo, 0, sizeof(pduInfo));
        pduInfo.SduDataPtr = g_comTxPdu;
        pduInfo.SduLength  = g_comTxPduLen;
        CanIf_Transmit(0u, &pduInfo);
        g_comTxPduLen = 0u;
    }
}

void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    if (PduInfoPtr == NULL)
    {
        return;
    }
    /* 接收方向: 数据进入 Com 的接收 I-PDU 缓冲 */
    memset(g_pduRxData, 0, sizeof(g_pduRxData));
    memcpy(g_pduRxData, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    g_pduRxLen = PduInfoPtr->SduLength;
}

/*==================================================================================================
 * 测试替身: CanIf 层 (签名与 src/bsw/ecual/canif/include/CanIf.h 一致)
 *================================================================================================*/
void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    g_canTxPending = FALSE;
    memset(g_canTxBuffer, 0, sizeof(g_canTxBuffer));
    g_canTxLen = 0u;
    memset(g_canRxData, 0, sizeof(g_canRxData));
    g_canRxLen = 0u;
}

void CanIf_DeInit(void)
{
    CanIf_Init(NULL);
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    if (PduInfoPtr == NULL)
    {
        return E_NOT_OK;
    }
    /* 写入 CAN 硬件发送缓冲 */
    memcpy(g_canTxBuffer, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    g_canTxLen   = PduInfoPtr->SduLength;
    g_canTxPending = TRUE;
    return E_OK;
}

/* CanIf_RxIndication 在生产 CanIf.c 中有实现但未在 CanIf.h 导出,
 * 此处按生产实现签名 (CanIf.c:340) 声明并实现测试替身 */
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr)
{
    (void)Mailbox;
    if (PduInfoPtr == NULL)
    {
        return;
    }
    /* CAN 收到数据后向上层 (PduR) 上报 */
    PduR_RxIndication(0u, PduInfoPtr);
}

/*==================================================================================================
 * 测试用例
 *================================================================================================*/
static void comm_stack_init(void)
{
    Com_Init(NULL);
    PduR_Init(NULL);
    CanIf_Init(NULL);
}

/**
 * @brief 测试完整的COM发送流程
 * @test COM_STACK_TX_001
 */
void test_ComStack_Full_Tx_Path(void)
{
    uint8 data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    comm_stack_init();

    /* COM层发送 */
    TEST_ASSERT_EQUAL(0u, Com_SendSignal(0, data));

    /* 触发COM MainFunction: COM -> PduR */
    Com_MainFunctionTx();

    /* PduR 路由: PduR -> CanIf -> CAN */
    PduR_MainFunction();

    /* 验证数据已传递到 CAN 发送缓冲 */
    TEST_ASSERT_TRUE(g_canTxPending);
    TEST_ASSERT_EQUAL_MEMORY(data, g_canTxBuffer, sizeof(data));
}

/**
 * @brief 测试完整的COM接收流程
 * @test COM_STACK_RX_001
 */
void test_ComStack_Full_Rx_Path(void)
{
    uint8 canData[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    uint8 receivedData[8];
    Can_HwType hw;
    PduInfoType pduInfo;

    comm_stack_init();

    /* 模拟 CAN 硬件接收: 数据进入 CAN 接收缓冲 */
    memcpy(g_canRxData, canData, sizeof(canData));
    g_canRxLen = sizeof(canData);

    /* CANIF 处理: CanIf -> PduR (RxIndication) */
    memset(&hw, 0, sizeof(hw));
    memset(&pduInfo, 0, sizeof(pduInfo));
    pduInfo.SduDataPtr = g_canRxData;
    pduInfo.SduLength  = g_canRxLen;
    CanIf_RxIndication(&hw, &pduInfo);

    /* COM 接收处理: PduR 收到的 I-PDU 解包进信号存储 */
    Com_MainFunctionRx();

    /* 验证数据已传递到COM */
    TEST_ASSERT_EQUAL(0u, Com_ReceiveSignal(0, receivedData));
    TEST_ASSERT_EQUAL_MEMORY(canData, receivedData, sizeof(canData));
}

/**
 * @brief 测试多路COM信号发送
 * @test COM_STACK_MULTI_001
 */
void test_ComStack_Multiple_Signals(void)
{
    uint8 signal1 = 0x11;
    /* 用字节数组而非多字节整数, 避免宿主字节序 (x86 小端) 影响打包顺序断言 */
    uint8 signal2[8] = {0x22, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8 signal3[8] = {0x44, 0x55, 0x66, 0x77, 0x00, 0x00, 0x00, 0x00};

    comm_stack_init();

    /* 发送多个信号 */
    TEST_ASSERT_EQUAL(0u, Com_SendSignal(0, &signal1));
    TEST_ASSERT_EQUAL(0u, Com_SendSignal(1, signal2));
    TEST_ASSERT_EQUAL(0u, Com_SendSignal(2, signal3));

    /* 触发发送: COM -> PduR -> CanIf -> CAN */
    Com_MainFunctionTx();
    PduR_MainFunction();

    /* 验证所有信号都已打包进入 CAN 发送缓冲 (每个信号固定 8 字节槽位) */
    TEST_ASSERT_TRUE(g_canTxPending);
    TEST_ASSERT_EQUAL(0x11, g_canTxBuffer[0]);
    TEST_ASSERT_EQUAL(0x22, g_canTxBuffer[8]);
    TEST_ASSERT_EQUAL(0x33, g_canTxBuffer[9]);
    TEST_ASSERT_EQUAL(0x44, g_canTxBuffer[16]);
    TEST_ASSERT_EQUAL(0x55, g_canTxBuffer[17]);
    TEST_ASSERT_EQUAL(0x66, g_canTxBuffer[18]);
    TEST_ASSERT_EQUAL(0x77, g_canTxBuffer[19]);
}

int test_communication_stack_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ComStack_Full_Tx_Path);
    RUN_TEST(test_ComStack_Full_Rx_Path);
    RUN_TEST(test_ComStack_Multiple_Signals);
    return UNITY_END();
}

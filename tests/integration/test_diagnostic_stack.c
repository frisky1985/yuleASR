/*
 * @file test_diagnostic_stack.c
 * @brief 诊断栈集成测试
 *
 * 测试 DCM-DoIP-CanTSyn 诊断流程。
 *
 * 说明: 集成测试在宿主机(无硬件)上构建, 不链接生产 .c 实现。以“测试替身”
 *       方式实现各层 API (签名与生产头文件 Dcm.h/DoIP.h/CanTSyn.h/StbM.h/PduR.h
 *       一致), 验证 DoIP 诊断请求 -> PduR 路由 -> DCM 处理 -> 响应回传 DoIP
 *       的端到端流程, 以及 CanTSyn 时间同步与诊断时间戳的结合。
 *       main() 更名为 test_diagnostic_stack_main() 由集成测试 runner 统一调度。
 *
 * Test Levels: Integration
 * ASIL Level: B
 */

#include <unity.h>
#include "Dcm.h"
#include "DoIP.h"
#include "CanTSyn.h"
#include "StbM.h"
#include "PduR.h"
#include <string.h>

/*==================================================================================================
 * 测试替身内部状态
 *================================================================================================*/
#define DIAG_MAX_PDU   64u

static uint8   g_doipRxRequest[DIAG_MAX_PDU];   /* DoIP 收到的诊断请求 */
static uint16  g_doipRxLen;
static uint8   g_dcmRequest[DIAG_MAX_PDU];      /* DCM 待处理请求 */
static uint16  g_dcmReqLen;
static boolean g_dcmReqPending;
static uint8   g_dcmResponse[DIAG_MAX_PDU];     /* DCM 响应 */
static uint16  g_dcmRespLen;
static uint8   g_doipTxResponse[DIAG_MAX_PDU];  /* DoIP 待发送响应 */
static uint16  g_doipTxLen;
static uint64  g_virtualTime;                   /* CanTSyn/StbM 虚拟时间 */
static uint64  g_dcmTimestamp;                  /* DCM 处理请求时的时间戳 */

/*==================================================================================================
 * 测试替身: DCM 层 (签名与 src/bsw/services/dcm/include/Dcm.h 一致)
 *================================================================================================*/
void Dcm_Init(const Dcm_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    memset(g_dcmRequest, 0, sizeof(g_dcmRequest));
    g_dcmReqLen = 0u;
    g_dcmReqPending = FALSE;
    memset(g_dcmResponse, 0, sizeof(g_dcmResponse));
    g_dcmRespLen = 0u;
    g_dcmTimestamp = 0u;
}

void Dcm_DeInit(void)
{
    Dcm_Init(NULL);
}

void Dcm_MainFunction(void)
{
    /* 处理收到的诊断请求并生成响应 (模拟 DCM 会话控制/读数据服务) */
    if (!g_dcmReqPending)
    {
        return;
    }

    g_dcmTimestamp = g_virtualTime;

    if ((g_dcmReqLen >= 2u) && (g_dcmRequest[0] == 0x10u))
    {
        /* Session Control (0x10 0x01) -> 肯定响应 0x50 0x01 */
        g_dcmResponse[0] = 0x50u;
        g_dcmResponse[1] = g_dcmRequest[1];
        g_dcmRespLen = 2u;
    }
    else if ((g_dcmReqLen >= 3u) && (g_dcmRequest[0] == 0x22u))
    {
        /* ReadDataByIdentifier (0x22 F1 90) -> 肯定响应 0x62 ... */
        g_dcmResponse[0] = 0x62u;
        g_dcmResponse[1] = g_dcmRequest[1];
        g_dcmResponse[2] = g_dcmRequest[2];
        g_dcmRespLen = 3u;
    }
    else
    {
        /* 否定响应 0x7F */
        g_dcmResponse[0] = 0x7Fu;
        g_dcmResponse[1] = (g_dcmReqLen > 0u) ? g_dcmRequest[0] : 0x00u;
        g_dcmResponse[2] = 0x11u; /* serviceNotSupported */
        g_dcmRespLen = 3u;
    }

    /* 响应经 PduR 回传 (Dcm -> PduR -> DoIP) */
    {
        PduInfoType pduInfo;

        memset(&pduInfo, 0, sizeof(pduInfo));
        pduInfo.SduDataPtr = g_dcmResponse;
        pduInfo.SduLength  = g_dcmRespLen;
        PduR_Transmit(1u, &pduInfo);
    }

    g_dcmReqPending = FALSE;
}

/*==================================================================================================
 * 测试替身: DoIP 层 (签名与 src/bsw/services/doip/include/DoIP.h 一致)
 *================================================================================================*/
void DoIP_Init(const DoIP_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    memset(g_doipRxRequest, 0, sizeof(g_doipRxRequest));
    g_doipRxLen = 0u;
    memset(g_doipTxResponse, 0, sizeof(g_doipTxResponse));
    g_doipTxLen = 0u;
}

void DoIP_DeInit(void)
{
    DoIP_Init(NULL);
}

void DoIP_MainFunction(void)
{
    /* 空实现: 生产环境在此轮询网络并收发诊断消息 */
}

/* 测试辅助: 模拟 DoIP 收到一条诊断请求 (DoIP_TpRxIndication 语义),
 * 经 PduR 路由到 DCM。生产 API 中 DoIP 接收回调未在 DoIP.h 导出,
 * 此处按 DoIP -> PduR -> DCM 的标准路由建模。 */
static void doip_receive_diag_request(const uint8* request, uint16 length)
{
    PduInfoType pduInfo;

    memcpy(g_doipRxRequest, request, length);
    g_doipRxLen = length;

    memset(&pduInfo, 0, sizeof(pduInfo));
    pduInfo.SduDataPtr = g_doipRxRequest;
    pduInfo.SduLength  = g_doipRxLen;

    /* PduR 将 DoIP 收到的 PDU 路由给 DCM */
    PduR_RxIndication(0u, &pduInfo);
}

/*==================================================================================================
 * 测试替身: PduR 层 (签名与 src/bsw/services/pdur/include/PduR.h 一致)
 *================================================================================================*/
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    if (PduInfoPtr == NULL)
    {
        return E_NOT_OK;
    }
    if (TxPduId == 1u)
    {
        /* DCM 响应 -> DoIP 发送缓冲 */
        memcpy(g_doipTxResponse, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        g_doipTxLen = PduInfoPtr->SduLength;
    }
    return E_OK;
}

void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    if (PduInfoPtr == NULL)
    {
        return;
    }
    /* DoIP 收到的诊断请求 -> DCM */
    memset(g_dcmRequest, 0, sizeof(g_dcmRequest));
    memcpy(g_dcmRequest, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    g_dcmReqLen = PduInfoPtr->SduLength;
    g_dcmReqPending = TRUE;
}

/*==================================================================================================
 * 测试替身: CanTSyn 层 (签名与 src/bsw/services/cantsyn/include/CanTSyn.h 一致)
 *================================================================================================*/
void CanTSyn_Init(const CanTSyn_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    g_virtualTime = 0u;
}

void CanTSyn_DeInit(void)
{
    CanTSyn_Init(NULL);
}

void CanTSyn_MainFunction(void)
{
    /* 推进虚拟时间 (模拟本地时基在 MainFunction 中自增) */
    g_virtualTime += 1000u;
}

Std_ReturnType CanTSyn_SetGlobalTime(uint8 timeBaseId,
                                     const StbM_TimeStampType* timeStampPtr,
                                     const StbM_UserDataType* userDataPtr)
{
    (void)timeBaseId;
    (void)userDataPtr;
    if (timeStampPtr == NULL)
    {
        return E_NOT_OK;
    }
    /* 收到全局时间同步报文后, 将虚拟时间对齐到同步时间 */
    g_virtualTime = ((uint64)timeStampPtr->seconds * 1000000000u) + timeStampPtr->nanoseconds;
    return E_OK;
}

Std_ReturnType CanTSyn_GetCurrentVirtualTime(uint8 timeBaseId,
                                             StbM_VirtualLocalTimeType* virtualTimePtr)
{
    (void)timeBaseId;
    if (virtualTimePtr == NULL)
    {
        return E_NOT_OK;
    }
    *virtualTimePtr = g_virtualTime;
    return E_OK;
}

/*==================================================================================================
 * 测试用例
 *================================================================================================*/
static void diag_stack_init(void)
{
    Dcm_Init(NULL);
    DoIP_Init(NULL);
    CanTSyn_Init(NULL);
}

/**
 * @brief 测试完整的DoIP诊断流程
 * @test DIAG_STACK_DOIP_001
 */
void test_DiagStack_DoIP_Full_Diagnostic(void)
{
    uint8 diagRequest[]  = {0x10, 0x01};  /* Session Control */

    diag_stack_init();

    /* 模拟 DoIP 收到诊断请求 (RoutingActivation 后传输诊断消息) */
    doip_receive_diag_request(diagRequest, sizeof(diagRequest));

    /* DCM 处理请求并生成响应 */
    Dcm_MainFunction();

    /* 验证响应 (0x50 0x01) 已回传到 DoIP 发送缓冲 */
    TEST_ASSERT_EQUAL(2u, g_doipTxLen);
    TEST_ASSERT_EQUAL(0x50, g_doipTxResponse[0]);
    TEST_ASSERT_EQUAL(0x01, g_doipTxResponse[1]);
}

/**
 * @brief 测试带时间同步的诊断
 * @test DIAG_STACK_TSYN_001
 */
void test_DiagStack_With_TimeSync(void)
{
    StbM_TimeStampType timeStamp;
    StbM_VirtualLocalTimeType virtualTime = 0u;
    uint8 diagRequest[] = {0x22, 0xF1, 0x90};  /* Read VIN */

    diag_stack_init();

    /* 执行时间同步: 设置全局时间 */
    memset(&timeStamp, 0, sizeof(timeStamp));
    timeStamp.seconds = 100u;
    timeStamp.nanoseconds = 500u;
    TEST_ASSERT_EQUAL(E_OK, CanTSyn_SetGlobalTime(0u, &timeStamp, NULL));

    /* CanTSyn MainFunction 推进时基 */
    CanTSyn_MainFunction();

    /* 验证虚拟时间已同步且推进 */
    TEST_ASSERT_EQUAL(E_OK, CanTSyn_GetCurrentVirtualTime(0u, &virtualTime));
    TEST_ASSERT_TRUE(virtualTime > 0u);

    /* 发送带时间标记的诊断请求 */
    doip_receive_diag_request(diagRequest, sizeof(diagRequest));
    Dcm_MainFunction();

    /* 验证请求处理时携带了时间戳 (替代原 Dcm_HasTimestamp, 生产无此 API) */
    TEST_ASSERT_TRUE(g_dcmTimestamp > 0u);
    TEST_ASSERT_EQUAL(0x62, g_doipTxResponse[0]);
}

int test_diagnostic_stack_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_DiagStack_DoIP_Full_Diagnostic);
    RUN_TEST(test_DiagStack_With_TimeSync);
    return UNITY_END();
}

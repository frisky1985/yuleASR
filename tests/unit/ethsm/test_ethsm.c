/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : EthSM (Ethernet State Manager) Unit Tests — 双网络回归测试
*
* SW Version           : 1.0.0
* Build Date           : 2026-08-08
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: 回归测试 — 08-07 MISRA 机械修复将 trcvIdx/ctrlIdx 赋值语句损坏为裸表达式
*              (trcvIdx ; / ctrlIdx ;), 导致网络 1+ 的收发器/控制器被当作网络 0。
*              本测试编译真实生产源码 src/bsw/ecual/ethSm/src/EthSM.c,
*              通过 mock EthIf 记录实际传入的 trcvIdx/ctrlIdx, 验证:
*                - EthSM_CheckLinkState:    网络 0 → ETHSM_TRCV_IDX_NETWORK_0,
*                                            网络 1 → ETHSM_TRCV_IDX_NETWORK_1
*                - EthSM_ProcessState_NO_COM/WAIT_TRCVLINK/WAIT_ONLINE/COM_READY:
*                    SetControllerMode 使用正确 ctrlIdx
*                - EthSM_DeInit: 每个网络使用正确 ctrlIdx 关闭
*=================================================================================================*/

// @tests src/bsw/services/ethsm/src/EthSM.c  @tests src/bsw/services/ethsm/include/EthSM.h

#include "../test_framework.h"

#include "EthSM.h"
#include "EthSM_Cfg.h"
#include "EthIf.h"
#include "ComM.h"
#include "Det.h"

/*==================================================================================================
*                                      MOCK STATE
*==================================================================================================*/
#define MOCK_MAX_CALLS  16U

static uint8  mock_setMode_ctrl[MOCK_MAX_CALLS];
static int    mock_setMode_mode[MOCK_MAX_CALLS];
static uint8  mock_setMode_count;

static uint8  mock_link_trcv[MOCK_MAX_CALLS];
static uint8  mock_link_count;

static EthIf_LinkStateType mock_linkState = ETHIF_LINK_STATE_ACTIVE;

static void mock_reset(void)
{
    mock_setMode_count = 0U;
    mock_link_count = 0U;
    mock_linkState = ETHIF_LINK_STATE_ACTIVE;
}

/*==================================================================================================
*                                      MOCK IMPLEMENTATIONS
*   (EthIf / ComM / Det — 生产 EthSM.c 的外部依赖)
*==================================================================================================*/
Std_ReturnType EthIf_SetControllerMode(uint8 ControllerId, EthIf_ControllerMode Mode)
{
    if (mock_setMode_count < MOCK_MAX_CALLS)
    {
        mock_setMode_ctrl[mock_setMode_count] = ControllerId;
        mock_setMode_mode[mock_setMode_count] = (int)Mode;
        mock_setMode_count++;
    }
    return E_OK;
}

Std_ReturnType EthIf_GetTransceiverLinkState(uint8 TrcvIdx, EthIf_LinkStateType* LinkStatePtr)
{
    if (mock_link_count < MOCK_MAX_CALLS)
    {
        mock_link_trcv[mock_link_count] = TrcvIdx;
        mock_link_count++;
    }
    if (LinkStatePtr != NULL_PTR)
    {
        *LinkStatePtr = mock_linkState;
    }
    return E_OK;
}

void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode)
{
    (void)Channel;
    (void)Mode;
}

Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/*==================================================================================================
*                                      HELPER
*==================================================================================================*/
static EthSM_ConfigType g_test_config;

/* 重置模块状态: EthSM_Init 非幂等 (已初始化时 early-return),
 * 同一进程内多个 RUN_TEST 必须先用 DeInit 回退到未初始化态,
 * 再清空 mock 记录 (DeInit 本身会调 SetControllerMode(DOWN))。 */
static void ethsm_setup(void)
{
    EthSM_DeInit();
    mock_reset();
    EthSM_Init(&g_test_config);
}

/* 双网络同时请求 FULL_COMMUNICATION 并跑一次 MainFunction:
 * NO_COM → ProcessState_NO_COM (SetControllerMode(ctrlIdx, ACTIVE)) → WAIT_TRCVLINK
 * 返回后两网络均处于 WAIT_TRCVLINK, mock_setMode_ctrl = [0, 1] */
static void ethsm_bring_both_networks_to_wait_trcvlink(void)
{
    TEST_ASSERT_EQUAL_INT(E_OK, EthSM_RequestComMode(ETHSM_NETWORK_0, COMM_FULL_COMMUNICATION));
    TEST_ASSERT_EQUAL_INT(E_OK, EthSM_RequestComMode(ETHSM_NETWORK_1, COMM_FULL_COMMUNICATION));
    EthSM_MainFunction();
}

/*==================================================================================================
*                                      TEST CASES
*==================================================================================================*/

/* P0-A 回归: 双网络 FullComm 请求后, ProcessState_NO_COM 必须用各网络的 ctrlIdx
 * 调 EthIf_SetControllerMode (网络 0 → 0, 网络 1 → 1)。
 * 修复前: 两次调用都传 0 (赋值丢失, ctrlIdx 恒为初值) → 本用例失败。 */
/** @req SWS_EthSM_00004 */
void test_EthSM_DualNetwork_ProcessNOCOM_UsesNetworkCtrlIdx(void)
{
    ethsm_setup();

    ethsm_bring_both_networks_to_wait_trcvlink();

    TEST_ASSERT_EQUAL_UINT8(2U, mock_setMode_count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_0, mock_setMode_ctrl[0]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_ACTIVE, mock_setMode_mode[0]);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_1, mock_setMode_ctrl[1]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_ACTIVE, mock_setMode_mode[1]);

    /* 两网络都应已进入 WAIT_TRCVLINK */
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(ETHSM_NETWORK_0));
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(ETHSM_NETWORK_1));
}

/* P0-A 回归: WAIT_TRCVLINK 中 CheckLinkState 必须用各网络的 trcvIdx
 * 调 EthIf_GetTransceiverLinkState (网络 0 → 0, 网络 1 → 1)。
 * 修复前: 两次调用都传 0 → 本用例失败。 */
/** @req SWS_EthSM_00007 */
void test_EthSM_DualNetwork_CheckLinkState_UsesNetworkTrcvIdx(void)
{
    ethsm_setup();

    ethsm_bring_both_networks_to_wait_trcvlink();

    /* WAIT_TRCVLINK → MainFunction: CheckLinkState (link up → WAIT_ONLINE) */
    EthSM_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(2U, mock_link_count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_TRCV_IDX_NETWORK_0, mock_link_trcv[0]);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_TRCV_IDX_NETWORK_1, mock_link_trcv[1]);

    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_WAIT_ONLINE, EthSM_GetInternalState(ETHSM_NETWORK_0));
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_WAIT_ONLINE, EthSM_GetInternalState(ETHSM_NETWORK_1));
}

/* P0-A 回归: WAIT_TRCVLINK 中收到 NO_COMMUNICATION → 用各网络 ctrlIdx 关控制器 */
/** @req SWS_EthSM_00004 */
void test_EthSM_DualNetwork_NoComShutdown_UsesNetworkCtrlIdx(void)
{
    ethsm_setup();

    ethsm_bring_both_networks_to_wait_trcvlink();

    /* 对网络 1 请求 NO_COMMUNICATION → WAIT_TRCVLINK 应 SetControllerMode(1, DOWN) */
    TEST_ASSERT_EQUAL_INT(E_OK, EthSM_RequestComMode(ETHSM_NETWORK_1, COMM_NO_COMMUNICATION));
    EthSM_MainFunction();

    /* 调用序列: [0,ACTIVE], [1,ACTIVE], [1,DOWN] */
    TEST_ASSERT_EQUAL_UINT8(3U, mock_setMode_count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_1, mock_setMode_ctrl[2]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_DOWN, mock_setMode_mode[2]);

    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_NO_COM, EthSM_GetInternalState(ETHSM_NETWORK_1));
}

/* P0-A 回归: WAIT_ONLINE 超时 → 用各网络 ctrlIdx 关控制器 */
/** @req SWS_EthSM_00007 */
void test_EthSM_DualNetwork_WaitOnlineTimeout_UsesNetworkCtrlIdx(void)
{
    ethsm_setup();

    ethsm_bring_both_networks_to_wait_trcvlink();

    /* 进入 WAIT_ONLINE (link up) */
    EthSM_MainFunction();

    /* TcpIp 保持 OFFLINE → 累计超时 (5000ms / 10ms = 500 周期) */
    uint16 cycles;
    for (cycles = 0U; cycles < 510U; cycles++)
    {
        EthSM_MainFunction();
    }

    /* 超时后网络 1 应 SetControllerMode(1, DOWN):
     * 调用序列: [0,ACTIVE], [1,ACTIVE], [0,DOWN](net0 超时), [1,DOWN](net1 超时) */
    TEST_ASSERT_EQUAL_UINT8(4U, mock_setMode_count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_1, mock_setMode_ctrl[3]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_DOWN, mock_setMode_mode[3]);

    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_NO_COM, EthSM_GetInternalState(ETHSM_NETWORK_1));
}

/* P0-A 回归: COM_READY 中收到 NO_COMMUNICATION → 用各网络 ctrlIdx 关控制器 */
void test_EthSM_DualNetwork_ComReadyShutdown_UsesNetworkCtrlIdx(void)
{
    ethsm_setup();

    ethsm_bring_both_networks_to_wait_trcvlink();

    /* WAIT_TRCVLINK → WAIT_ONLINE (link up) */
    EthSM_MainFunction();
    /* TcpIp online → COM_READY */
    EthSM_TcpIpModeIndication(ETHSM_NETWORK_0, TCPIP_STATE_ONLINE);
    EthSM_TcpIpModeIndication(ETHSM_NETWORK_1, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();

    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_COM_READY, EthSM_GetInternalState(ETHSM_NETWORK_1));

    /* COM_READY + NO_COMMUNICATION → SetControllerMode(1, DOWN) */
    TEST_ASSERT_EQUAL_INT(E_OK, EthSM_RequestComMode(ETHSM_NETWORK_1, COMM_NO_COMMUNICATION));
    EthSM_MainFunction();

    /* 调用序列: [0,ACTIVE], [1,ACTIVE], [1,DOWN] */
    TEST_ASSERT_EQUAL_UINT8(3U, mock_setMode_count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_1, mock_setMode_ctrl[2]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_DOWN, mock_setMode_mode[2]);
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_NO_COM, EthSM_GetInternalState(ETHSM_NETWORK_1));
}

/* P0-A 回归: DeInit 按网络序号逐个关控制器 (0, 1) */
void test_EthSM_DeInit_UsesPerNetworkCtrlIdx(void)
{
    ethsm_setup();

    EthSM_DeInit();

    TEST_ASSERT_EQUAL_UINT8(2U, mock_setMode_count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_0, mock_setMode_ctrl[0]);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_CTRL_IDX_NETWORK_1, mock_setMode_ctrl[1]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_DOWN, mock_setMode_mode[0]);
    TEST_ASSERT_EQUAL_INT((int)ETHIF_MODE_DOWN, mock_setMode_mode[1]);
}

/* 冒烟: 基本状态机行为 (双网络互不干扰) */
void test_EthSM_Smoke_DualNetworkStatesIndependent(void)
{
    ethsm_setup();

    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_NO_COM, EthSM_GetInternalState(ETHSM_NETWORK_0));
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_NO_COM, EthSM_GetInternalState(ETHSM_NETWORK_1));

    /* 只请求网络 0 → 网络 1 应保持 NO_COM */
    TEST_ASSERT_EQUAL_INT(E_OK, EthSM_RequestComMode(ETHSM_NETWORK_0, COMM_FULL_COMMUNICATION));
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(ETHSM_NETWORK_0));
    TEST_ASSERT_EQUAL_INT(ETHSM_STATE_NO_COM, EthSM_GetInternalState(ETHSM_NETWORK_1));
}

/*==================================================================================================
*                                      TEST MAIN
*==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_EthSM_DualNetwork_ProcessNOCOM_UsesNetworkCtrlIdx);
    RUN_TEST(test_EthSM_DualNetwork_CheckLinkState_UsesNetworkTrcvIdx);
    RUN_TEST(test_EthSM_DualNetwork_NoComShutdown_UsesNetworkCtrlIdx);
    RUN_TEST(test_EthSM_DualNetwork_WaitOnlineTimeout_UsesNetworkCtrlIdx);
    RUN_TEST(test_EthSM_DualNetwork_ComReadyShutdown_UsesNetworkCtrlIdx);
    RUN_TEST(test_EthSM_DeInit_UsesPerNetworkCtrlIdx);
    RUN_TEST(test_EthSM_Smoke_DualNetworkStatesIndependent);

    return UNITY_END();
}

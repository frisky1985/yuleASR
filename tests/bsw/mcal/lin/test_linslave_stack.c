/**
 * @file test_linslave_stack.c
 * @brief Host unit tests for the LinSlave protocol stack
 *        (LinSlave.c / LinSlave_CfgTable.c / LinSlave_Tp.c / LinSlave_Uds.c).
 *
 * No hardware is touched: the LinSlave HAL is a pure simulation and the
 * protocol stack is driven byte by byte through its public entry points.
 * LinSlave.c is included directly so that static state can be restored
 * between test groups.
 */
#include <string.h>

#include "unity.h"

#include "LinSlave.h"
#include "LinSlave_Pid.h"
#include "LinSlave_Checksum.h"
#include "LinSlave_Hal.h"

/* White-box inclusion of the core state machine. */
#include "../../../../src/bsw/mcal/lin/src/LinSlave.c"

/*----------------------------------------------------------------------------
 * Shared test fixtures
 *---------------------------------------------------------------------------*/
static uint8 LinTest_RxCallbackCount;
static uint8 LinTest_ErrorCallbackCount;
static uint8 LinTest_FrameRxCount;
static uint8 LinTest_FrameTxCount;
static uint8 LinTest_CfgTableRxCount;
static uint8 LinTest_TpRxCount;
static uint8 LinTest_TpTxCount;
static uint8 LinTest_TpLastStatus;

static const uint8 LinTest_Response[3] = { 0x11U, 0x22U, 0x33U };

static void LinTest_RxCallback(uint8 Pid, uint8* ResponseDataPtr, uint8* ResponseLengthPtr,
                               uint8* ChecksumTypePtr)
{
    (void)Pid;
    (void)memcpy(ResponseDataPtr, LinTest_Response, sizeof(LinTest_Response));
    *ResponseLengthPtr = (uint8)sizeof(LinTest_Response);
    *ChecksumTypePtr = (uint8)LINSLAVE_CHECKSUM_ENHANCED;
    LinTest_RxCallbackCount++;
}

static void LinTest_ErrorCallback(uint8 Pid, LinSlave_ErrorType ErrorCode)
{
    (void)ErrorCode;
    (void)Pid;
    LinTest_ErrorCallbackCount++;
}

static void LinTest_UncondRx(uint8 FrameIndex, const uint8* DataPtr, uint8 Length, void* UserData)
{
    (void)FrameIndex;
    (void)DataPtr;
    (void)Length;
    (void)UserData;
    LinTest_CfgTableRxCount++;
}

static void LinTest_UncondTx(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr, void* UserData)
{
    (void)FrameIndex;
    (void)UserData;
    DataPtr[0] = 0xA1U;
    DataPtr[1] = 0xA2U;
    DataPtr[2] = 0xA3U;
    *LengthPtr = 3U;
    LinTest_FrameTxCount++;
}

static void LinTest_UncondTxTooLong(uint8 FrameIndex, uint8* DataPtr, uint8* LengthPtr,
                                    void* UserData)
{
    (void)FrameIndex;
    (void)DataPtr;
    (void)UserData;
    *LengthPtr = 9U;
}

static void LinTest_TpRx(uint8 ChannelId, const uint8* DataPtr, uint16 Length)
{
    (void)ChannelId;
    (void)DataPtr;
    (void)Length;
    LinTest_TpRxCount++;
}

static void LinTest_TpTx(uint8 ChannelId, LinSlave_Tp_StatusType Status)
{
    (void)ChannelId;
    LinTest_TpLastStatus = (uint8)Status;
    LinTest_TpTxCount++;
}

static LinSlave_ConfigType LinTest_Cfg;

static void LinTest_ResetCounters(void)
{
    LinTest_RxCallbackCount = 0U;
    LinTest_ErrorCallbackCount = 0U;
    LinTest_FrameRxCount = 0U;
    LinTest_FrameTxCount = 0U;
    LinTest_CfgTableRxCount = 0U;
    LinTest_TpRxCount = 0U;
    LinTest_TpTxCount = 0U;
    LinTest_TpLastStatus = 0U;
}

static void LinTest_InitRaw(uint8 NodeId, uint8 ResponseLength)
{
    LinTest_Cfg.NodeId = NodeId;
    LinTest_Cfg.BaudRate = 0U;
    LinTest_Cfg.ResponseLength = ResponseLength;
    LinTest_Cfg.ChecksumType = (uint8)LINSLAVE_CHECKSUM_ENHANCED;
    LinTest_Cfg.BreakThresholdUs = 1000U;
    LinTest_Cfg.TimeoutMs = 100U;
    (void)LinSlave_Init(&LinTest_Cfg);
}

/*----------------------------------------------------------------------------
 * Configuration table fixture
 *---------------------------------------------------------------------------*/
static LinSlave_UnconditionalFrameConfigType LinTest_Frames[5];
static LinSlave_EventFrameConfigType LinTest_EventFrames[1];
static LinSlave_SporadicFrameConfigType LinTest_SporadicFrames[1];
static LinSlave_DiagnosticFrameConfigType LinTest_DiagFrames[1];
static const uint8 LinTest_AssociatedPids[1] = { 0x10U };
static LinSlave_ConfigTableType LinTest_ConfigTable;

static uint8 LinTest_PidOf(uint8 Id)
{
    return LinSlave_CalculatePid(Id);
}

static void LinTest_BuildConfigTable(boolean UseDiagnostic)
{
    (void)memset(LinTest_Frames, 0, sizeof(LinTest_Frames));
    (void)memset(LinTest_EventFrames, 0, sizeof(LinTest_EventFrames));
    (void)memset(LinTest_SporadicFrames, 0, sizeof(LinTest_SporadicFrames));
    (void)memset(LinTest_DiagFrames, 0, sizeof(LinTest_DiagFrames));

    LinTest_Frames[0].Pid = LinTest_PidOf(0x10U);
    LinTest_Frames[0].Length = 8U;
    LinTest_Frames[0].Direction = LINSLAVE_DIR_RX;
    LinTest_Frames[0].ChecksumType = LINSLAVE_CHECKSUM_ENHANCED;
    LinTest_Frames[0].RxCallback = LinTest_UncondRx;

    LinTest_Frames[1].Pid = LinTest_PidOf(0x11U);
    LinTest_Frames[1].Length = 8U;
    LinTest_Frames[1].Direction = LINSLAVE_DIR_TX;
    LinTest_Frames[1].ChecksumType = LINSLAVE_CHECKSUM_CLASSIC;
    LinTest_Frames[1].TxCallback = LinTest_UncondTx;

    LinTest_Frames[2].Pid = LinTest_PidOf(0x12U);
    LinTest_Frames[2].Length = 8U;
    LinTest_Frames[2].Direction = LINSLAVE_DIR_RX_TX;
    LinTest_Frames[2].ChecksumType = LINSLAVE_CHECKSUM_ENHANCED;

    LinTest_Frames[3].Pid = LinTest_PidOf(0x13U);
    LinTest_Frames[3].Length = 8U;
    LinTest_Frames[3].Direction = LINSLAVE_DIR_TX;
    LinTest_Frames[3].ChecksumType = LINSLAVE_CHECKSUM_CLASSIC;

    LinTest_Frames[4].Pid = LinTest_PidOf(0x14U);
    LinTest_Frames[4].Length = 8U;
    LinTest_Frames[4].Direction = LINSLAVE_DIR_TX;
    LinTest_Frames[4].ChecksumType = LINSLAVE_CHECKSUM_CLASSIC;
    LinTest_Frames[4].TxCallback = LinTest_UncondTxTooLong;

    LinTest_EventFrames[0].Pid = LinTest_PidOf(0x15U);
    LinTest_EventFrames[0].AssociatedFrameCount = 1U;
    LinTest_EventFrames[0].AssociatedPids = LinTest_AssociatedPids;

    LinTest_SporadicFrames[0].Pid = LinTest_PidOf(0x16U);
    LinTest_SporadicFrames[0].AssociatedFrameCount = 1U;
    LinTest_SporadicFrames[0].AssociatedPids = LinTest_AssociatedPids;

    LinTest_DiagFrames[0].RequestPid = LinTest_PidOf(0x20U);
    LinTest_DiagFrames[0].ResponsePid = LinTest_PidOf(0x21U);
    LinTest_DiagFrames[0].Callback = NULL_PTR;
    LinTest_DiagFrames[0].ErrorCallback = NULL_PTR;
    LinTest_DiagFrames[0].UserData = NULL_PTR;

    (void)memset(&LinTest_ConfigTable, 0, sizeof(LinTest_ConfigTable));
    LinTest_ConfigTable.NodeId = 5U;
    LinTest_ConfigTable.BaudRate = 19200U;
    LinTest_ConfigTable.UnconditionalFrameCount = 5U;
    LinTest_ConfigTable.UnconditionalFrames = LinTest_Frames;
    LinTest_ConfigTable.EventFrameCount = 1U;
    LinTest_ConfigTable.EventFrames = LinTest_EventFrames;
    LinTest_ConfigTable.SporadicFrameCount = 1U;
    LinTest_ConfigTable.SporadicFrames = LinTest_SporadicFrames;
    LinTest_ConfigTable.DiagnosticFrames = LinTest_DiagFrames;
    LinTest_ConfigTable.UseDiagnostic = UseDiagnostic;
    LinTest_ConfigTable.GlobalErrorCallback = LinTest_ErrorCallback;
}

static void LinTest_FeedHeader(uint8 Pid)
{
    LinSlave_BreakDetected();
    LinSlave_RxInterruptHandler(0x55U);
    LinSlave_RxInterruptHandler(Pid);
}

static void LinTest_FeedData(uint8 Value, uint8 Count)
{
    uint8 i;

    for (i = 0U; i < Count; i++) {
        LinSlave_RxInterruptHandler(Value);
    }
}

/*============================================================================
 * Tests that must run before anything is initialised
 *===========================================================================*/
void test_linslave_BeforeInit_core(void)
{
    Std_VersionInfoType version;
    uint8 payload[2] = { 1U, 2U };

    LinTest_ResetCounters();

    LinSlave_DeInit();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_UNINIT, (uint8)LinSlave_GetState());
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_ERROR_NONE, (uint8)LinSlave_GetLastError());

    LinSlave_BreakDetected();
    LinSlave_RxInterruptHandler(0x55U);
    LinSlave_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_SetResponseData(payload, 2U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_Init(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_InitWithConfigTable(NULL_PTR));

    /* LinSlave_GetVersionInfo() has no implementation in LinSlave.c; the
     * declaration exists but calling it would not link on the host. */
    (void)version;
}

void test_linslave_cfgtable_BeforeInit(void)
{
    uint8 pids[4] = { 0U, 0U, 0U, 0U };
    uint8 data[2] = { 1U, 2U };

    TEST_ASSERT_NULL(LinSlave_CfgTable_FindUnconditionalByPid(0x10U));
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetUnconditionalEntry(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_GetUnconditionalCount());
    TEST_ASSERT_NULL(LinSlave_CfgTable_FindEventFrame(0x15U));
    TEST_ASSERT_NULL(LinSlave_CfgTable_FindSporadicFrame(0x16U));
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetDiagnosticConfig());
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_FRAME_STATUS_ERROR,
                            (uint8)LinSlave_CfgTable_GetFrameStatus(0U));
    LinSlave_CfgTable_SetFrameStatus(0U, LINSLAVE_FRAME_STATUS_UPDATED);
    LinSlave_CfgTable_ClearUpdateFlag(0U);
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_IsFrameUpdated(0U));
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetFrameData(0U));
    LinSlave_CfgTable_SetFrameData(0U, data, 2U);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, LinSlave_CfgTable_GetPidByIndex(0U));
    TEST_ASSERT_EQUAL_UINT8(0xFFU, LinSlave_CfgTable_GetIndexByPid(0x10U));
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_GetAllUnconditionalPids(pids, 4U));
    TEST_ASSERT_FALSE(LinSlave_CfgTable_IsUnconditionalFrame(0x10U));
    TEST_ASSERT_FALSE(LinSlave_CfgTable_IsEventFrame(0x15U));
    TEST_ASSERT_FALSE(LinSlave_CfgTable_IsSporadicFrame(0x16U));
    TEST_ASSERT_FALSE(LinSlave_CfgTable_IsDiagnosticFrame(0x3CU));
}

void test_linslave_tp_BeforeInit(void)
{
    LinTest_ResetCounters();

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_NOT_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x01U, NULL_PTR, 0U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_NOT_OK,
                            (uint8)LinSlave_Tp_Transmit(0U, NULL_PTR, 0U));
    LinSlave_Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_IDLE, (uint8)LinSlave_Tp_GetState(0U));
    TEST_ASSERT_FALSE(LinSlave_Tp_IsBusy(0U));
    LinSlave_Tp_Cancel(0U);
    LinSlave_Tp_DeInit();
}

void test_linslave_uds_BeforeInit(void)
{
    uint8 request = 0x10U;

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(&request, 1U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_SendPositiveResponse(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_SendNegativeResponse(0x10U, 0x10U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_RegisterService(NULL_PTR));
    LinSlave_Uds_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_SECURITY_LOCKED,
                            (uint8)LinSlave_Uds_GetSecurityLevel());
    LinSlave_Uds_DeInit();
}

/*============================================================================
 * Core state machine (raw configuration)
 *===========================================================================*/
void test_linslave_InitParameterChecks(void)
{
    LinTest_ResetCounters();

    LinTest_Cfg.NodeId = 5U;
    LinTest_Cfg.BaudRate = 0U;
    LinTest_Cfg.ResponseLength = 3U;
    LinTest_Cfg.ChecksumType = 0U;
    LinTest_Cfg.BreakThresholdUs = 1000U;
    LinTest_Cfg.TimeoutMs = 100U;

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_Init(NULL_PTR));

    LinTest_Cfg.NodeId = 60U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_Init(&LinTest_Cfg));

    LinTest_Cfg.NodeId = 5U;
    LinTest_Cfg.ResponseLength = 0U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_Init(&LinTest_Cfg));

    LinTest_Cfg.ResponseLength = 9U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_Init(&LinTest_Cfg));

    /* Baudrate 1 -> 19200 branch. */
    LinTest_Cfg.ResponseLength = 3U;
    LinTest_Cfg.BaudRate = 1U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_OK, (uint8)LinSlave_Init(&LinTest_Cfg));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_Init(&LinTest_Cfg));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    LinSlave_DeInit();
    LinSlave_DeInit();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_UNINIT, (uint8)LinSlave_GetState());
}

void test_linslave_SetResponseDataChecks(void)
{
    uint8 payload[3] = { 1U, 2U, 3U };
    uint8 tooLong[9] = { 0U };

    LinTest_InitRaw(5U, 3U);
    LinTest_ResetCounters();

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_SetResponseData(NULL_PTR, 3U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_SetResponseData(payload, 0U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_SetResponseData(tooLong, 9U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_OK, (uint8)LinSlave_SetResponseData(payload, 3U));

    LinSlave_DeInit();
}

void test_linslave_RawFrameReception(void)
{
    uint8 pid = LinTest_PidOf(5U);
    uint8 csum;

    LinTest_InitRaw(5U, 3U);
    LinTest_ResetCounters();
    LinSlave_RegisterRxCallback(LinTest_RxCallback);
    LinSlave_RegisterErrorCallback(LinTest_ErrorCallback);

    /* Healthy frame: header + 3 data bytes + checksum over the response. */
    LinTest_FeedHeader(pid);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_RX_DATA, (uint8)LinSlave_GetState());
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_RxCallbackCount);
    LinTest_FeedData(0xA0U, 3U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_RX_CSUM, (uint8)LinSlave_GetState());

    csum = LinSlave_CalculateChecksum(LinTest_Response, 3U, pid, LINSLAVE_CHECKSUM_ENHANCED);
    LinSlave_RxInterruptHandler(csum);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* Wrong checksum. */
    LinTest_FeedHeader(pid);
    LinTest_FeedData(0xA0U, 3U);
    LinSlave_RxInterruptHandler((uint8)(csum + 1U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_ERROR_CHECKSUM, (uint8)LinSlave_GetLastError());
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_ErrorCallbackCount);

    /* Wrong sync byte. */
    LinSlave_BreakDetected();
    LinSlave_RxInterruptHandler(0x54U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_ERROR_SYNC, (uint8)LinSlave_GetLastError());

    /* Invalid PID (bad protection bits). */
    LinTest_FeedHeader(0x00U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_ERROR_PID, (uint8)LinSlave_GetLastError());

    /* Valid PID belonging to another node. */
    LinTest_FeedHeader(LinTest_PidOf(7U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    LinSlave_DeInit();
}

void test_linslave_RawFrameWithoutCallback(void)
{
    uint8 pid = LinTest_PidOf(5U);

    LinTest_InitRaw(5U, 3U);
    LinTest_ResetCounters();
    LinSlave_RegisterRxCallback(NULL_PTR);
    LinSlave_RegisterErrorCallback(NULL_PTR);

    LinTest_FeedHeader(pid);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_RX_DATA, (uint8)LinSlave_GetState());

    /* No response configured -> framing error. */
    LinSlave_RxInterruptHandler(0x01U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_ERROR_FRAMING, (uint8)LinSlave_GetLastError());

    LinSlave_DeInit();
}

void test_linslave_StateMachineIgnoredStates(void)
{
    LinTest_InitRaw(5U, 3U);
    LinTest_ResetCounters();

    /* IDLE ignores stray bytes. */
    LinSlave_RxInterruptHandler(0x12U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* TX_RESPONSE ignores incoming bytes. */
    LinSlave_State = LINSLAVE_STATE_TX_RESPONSE;
    LinSlave_RxInterruptHandler(0x12U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_TX_RESPONSE, (uint8)LinSlave_GetState());

    /* Out-of-range state hits the default branch. */
    LinSlave_State = (LinSlave_StateType)0x7FU;
    LinSlave_RxInterruptHandler(0x12U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    LinSlave_DeInit();
}

/*============================================================================
 * Configuration table API
 *===========================================================================*/
void test_linslave_CfgTableInitChecks(void)
{
    LinSlave_ConfigTableType table;
    uint8 dummyFrame[8] = { 0U };

    (void)memset(&table, 0, sizeof(table));

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_CfgTable_Init(NULL_PTR));

    table.UnconditionalFrameCount = (uint8)(LINSLAVE_MAX_UNCONDITIONAL_FRAMES + 1U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_CfgTable_Init(&table));

    (void)memset(&table, 0, sizeof(table));
    table.EventFrameCount = (uint8)(LINSLAVE_MAX_EVENT_FRAMES + 1U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_CfgTable_Init(&table));

    (void)memset(&table, 0, sizeof(table));
    table.SporadicFrameCount = (uint8)(LINSLAVE_MAX_SPORADIC_FRAMES + 1U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK, (uint8)LinSlave_CfgTable_Init(&table));

    /* Counts valid but frame arrays NULL. */
    (void)memset(&table, 0, sizeof(table));
    table.UnconditionalFrameCount = 1U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_OK, (uint8)LinSlave_CfgTable_Init(&table));
    TEST_ASSERT_NULL(LinSlave_CfgTable_FindUnconditionalByPid(0x10U));
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetUnconditionalEntry(0U));

    (void)dummyFrame;
}

void test_linslave_CfgTableAccessors(void)
{
    uint8 pids[8];
    uint8 data[10];
    uint8 i;

    LinTest_BuildConfigTable(TRUE);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_OK, (uint8)LinSlave_CfgTable_Init(&LinTest_ConfigTable));

    TEST_ASSERT_EQUAL_UINT8(5U, LinSlave_CfgTable_GetUnconditionalCount());
    TEST_ASSERT_NOT_NULL(LinSlave_CfgTable_FindUnconditionalByPid(LinTest_PidOf(0x10U)));
    TEST_ASSERT_NULL(LinSlave_CfgTable_FindUnconditionalByPid(0x7FU));
    TEST_ASSERT_NOT_NULL(LinSlave_CfgTable_GetUnconditionalEntry(0U));
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetUnconditionalEntry(5U));
    TEST_ASSERT_NOT_NULL(LinSlave_CfgTable_FindEventFrame(LinTest_PidOf(0x15U)));
    TEST_ASSERT_NULL(LinSlave_CfgTable_FindEventFrame(0x7FU));
    TEST_ASSERT_NOT_NULL(LinSlave_CfgTable_FindSporadicFrame(LinTest_PidOf(0x16U)));
    TEST_ASSERT_NULL(LinSlave_CfgTable_FindSporadicFrame(0x7FU));

    TEST_ASSERT_NOT_NULL(LinSlave_CfgTable_GetDiagnosticConfig());
    TEST_ASSERT_TRUE(LinSlave_CfgTable_IsDiagnosticFrame(LinTest_PidOf(0x20U)));
    TEST_ASSERT_TRUE(LinSlave_CfgTable_IsDiagnosticFrame(LinTest_PidOf(0x21U)));
    TEST_ASSERT_FALSE(LinSlave_CfgTable_IsDiagnosticFrame(0x00U));

    TEST_ASSERT_TRUE(LinSlave_CfgTable_IsUnconditionalFrame(LinTest_PidOf(0x10U)));
    TEST_ASSERT_TRUE(LinSlave_CfgTable_IsEventFrame(LinTest_PidOf(0x15U)));
    TEST_ASSERT_TRUE(LinSlave_CfgTable_IsSporadicFrame(LinTest_PidOf(0x16U)));

    for (i = 0U; i < 10U; i++) {
        data[i] = (uint8)(i + 1U);
    }

    LinSlave_CfgTable_SetFrameStatus(1U, LINSLAVE_FRAME_STATUS_ERROR);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_FRAME_STATUS_ERROR,
                            (uint8)LinSlave_CfgTable_GetFrameStatus(1U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_FRAME_STATUS_ERROR,
                            (uint8)LinSlave_CfgTable_GetFrameStatus(9U));

    LinSlave_CfgTable_SetFrameData(1U, data, 10U);   /* clamped to 8 */
    TEST_ASSERT_EQUAL_UINT8(1U, LinSlave_CfgTable_IsFrameUpdated(1U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_FRAME_STATUS_UPDATED,
                            (uint8)LinSlave_CfgTable_GetFrameStatus(1U));
    TEST_ASSERT_NOT_NULL(LinSlave_CfgTable_GetFrameData(1U));
    TEST_ASSERT_EQUAL_UINT8(8U, LinSlave_CfgTable_GetFrameData(1U)[7]);
    LinSlave_CfgTable_ClearUpdateFlag(1U);
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_IsFrameUpdated(1U));

    LinSlave_CfgTable_SetFrameData(1U, NULL_PTR, 3U);
    LinSlave_CfgTable_SetFrameData(1U, data, 0U);
    LinSlave_CfgTable_SetFrameData(9U, data, 3U);
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetFrameData(9U));
    LinSlave_CfgTable_SetFrameStatus(9U, LINSLAVE_FRAME_STATUS_UPDATED);
    LinSlave_CfgTable_ClearUpdateFlag(9U);
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_IsFrameUpdated(9U));

    TEST_ASSERT_EQUAL_UINT8(LinTest_PidOf(0x10U), LinSlave_CfgTable_GetPidByIndex(0U));
    TEST_ASSERT_EQUAL_UINT8(0xFFU, LinSlave_CfgTable_GetPidByIndex(9U));
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_GetIndexByPid(LinTest_PidOf(0x10U)));
    TEST_ASSERT_EQUAL_UINT8(0xFFU, LinSlave_CfgTable_GetIndexByPid(0x00U));

    TEST_ASSERT_EQUAL_UINT8(5U, LinSlave_CfgTable_GetAllUnconditionalPids(pids, 8U));
    TEST_ASSERT_EQUAL_UINT8(3U, LinSlave_CfgTable_GetAllUnconditionalPids(pids, 3U));
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_GetAllUnconditionalPids(NULL_PTR, 4U));
    TEST_ASSERT_EQUAL_UINT8(0U, LinSlave_CfgTable_GetAllUnconditionalPids(pids, 0U));

    /* Diagnostic disabled -> NULL configuration. */
    LinTest_ConfigTable.UseDiagnostic = FALSE;
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetDiagnosticConfig());
    TEST_ASSERT_FALSE(LinSlave_CfgTable_IsDiagnosticFrame(LinTest_PidOf(0x20U)));
}

/*============================================================================
 * Configuration table driven state machine
 *===========================================================================*/
void test_linslave_ConfigTableStateMachine(void)
{
    LinTest_BuildConfigTable(TRUE);
    LinTest_ResetCounters();

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_OK,
                            (uint8)LinSlave_InitWithConfigTable(&LinTest_ConfigTable));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_NOT_OK,
                            (uint8)LinSlave_InitWithConfigTable(&LinTest_ConfigTable));

    LinSlave_RegisterErrorCallback(LinTest_ErrorCallback);

    /* RX direction: eight data bytes are buffered and delivered. */
    LinTest_FeedHeader(LinTest_PidOf(0x10U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_RX_DATA, (uint8)LinSlave_GetState());
    LinTest_FeedData(0x5AU, 8U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_CfgTableRxCount);
    TEST_ASSERT_EQUAL_UINT8(1U, LinSlave_CfgTable_IsFrameUpdated(0U));

    /* TX direction with a callback providing data. */
    LinTest_FeedHeader(LinTest_PidOf(0x11U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_FrameTxCount);

    /* TX direction without callback -> nothing transmitted. */
    LinTest_FeedHeader(LinTest_PidOf(0x13U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* TX direction with an invalid length. */
    LinTest_FeedHeader(LinTest_PidOf(0x14U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* RX_TX direction without framework callback. */
    LinTest_FeedHeader(LinTest_PidOf(0x12U));
    LinTest_FeedData(0x11U, 8U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* Diagnostic frame: collected as raw data, no unconditional callback. */
    LinTest_FeedHeader(LinTest_PidOf(0x20U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_RX_DATA, (uint8)LinSlave_GetState());
    LinTest_FeedData(0x22U, 8U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* Event triggered frame. */
    LinTest_FeedHeader(LinTest_PidOf(0x15U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* Sporadic frame. */
    LinTest_FeedHeader(LinTest_PidOf(0x16U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* Valid but unconfigured PID. */
    LinTest_FeedHeader(LinTest_PidOf(0x1EU));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    /* Invalid PID. */
    LinTest_FeedHeader(0x00U);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_ERROR_PID, (uint8)LinSlave_GetLastError());

    LinSlave_DeInit();
}

void test_linslave_ConfigTableNoDiagnostic(void)
{
    LinTest_BuildConfigTable(FALSE);
    LinTest_ResetCounters();

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_OK,
                            (uint8)LinSlave_InitWithConfigTable(&LinTest_ConfigTable));
    TEST_ASSERT_NULL(LinSlave_CfgTable_GetDiagnosticConfig());

    /* Diagnostic PID is treated as a plain unknown frame when disabled. */
    LinTest_FeedHeader(LinTest_PidOf(0x20U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_STATE_IDLE, (uint8)LinSlave_GetState());

    LinSlave_DeInit();
    LinSlave_MainFunction();
}

/*============================================================================
 * LIN TP
 *===========================================================================*/
void test_linslave_TpSingleFrame(void)
{
    uint8 payload[3] = { 1U, 2U, 3U };
    uint8 shortBuf[2] = { 1U, 2U };

    LinTest_ResetCounters();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK, (uint8)LinSlave_Tp_Init());
    LinSlave_Tp_RegisterRxIndication(LinTest_TpRx);
    LinSlave_Tp_RegisterTxConfirmation(LinTest_TpTx);

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x03U, payload, 3U));
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_TpRxCount);

    /* Zero length, too long, and truncated SF. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_INVALID_PCI,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x00U, payload, 3U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_INVALID_PCI,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x08U, payload, 3U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_INVALID_PCI,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x03U, shortBuf, 2U));

    /* Invalid PCI type. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_INVALID_PCI,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x40U, payload, 3U));

    /* SF transmission. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_Transmit(0U, payload, 3U));
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_TpTxCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)LINSLAVE_TP_OK, LinTest_TpLastStatus);

    LinSlave_Tp_DeInit();
}

void test_linslave_TpConsecutiveFrames(void)
{
    uint8 first[7] = { 10U, 1U, 2U, 3U, 4U, 5U, 6U };
    uint8 cf[4] = { 7U, 8U, 9U, 10U };
    uint8 firstZero[7] = { 0U, 1U, 2U, 3U, 4U, 5U, 6U };
    uint16 i;

    LinTest_ResetCounters();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK, (uint8)LinSlave_Tp_Init());
    LinSlave_Tp_RegisterRxIndication(LinTest_TpRx);

    /* CF before any FF is rejected. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_NOT_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x21U, cf, 4U));

    /* First frame announcing 10 bytes, carrying 6. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x10U, first, 7U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_RX_CF, (uint8)LinSlave_Tp_GetState(0U));
    TEST_ASSERT_TRUE(LinSlave_Tp_IsBusy(0U));

    /* Wrong sequence number resets the channel. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_NOT_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x22U, cf, 4U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_IDLE, (uint8)LinSlave_Tp_GetState(0U));

    /* Restart and complete with the correct SN. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x10U, first, 7U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x21U, cf, 4U));
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_TpRxCount);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_IDLE, (uint8)LinSlave_Tp_GetState(0U));

    /* Zero length announcement is rejected as oversized. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_OVERSIZE,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x10U, firstZero, 7U));

    /* Partial CF then receive timeout through the main function. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x10U, first, 7U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x21U, cf, 1U));
    for (i = 0U; i <= 1001U; i++) {
        LinSlave_Tp_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_IDLE, (uint8)LinSlave_Tp_GetState(0U));

    LinSlave_Tp_DeInit();
}

void test_linslave_TpTransmitAndFlowControl(void)
{
    uint8 payload[10];
    uint8 fc[2] = { 5U, 10U };
    uint16 i;

    for (i = 0U; i < 10U; i++) {
        payload[i] = (uint8)i;
    }

    LinTest_ResetCounters();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK, (uint8)LinSlave_Tp_Init());
    LinSlave_Tp_RegisterTxConfirmation(LinTest_TpTx);

    /* Parameter checks. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_INVALID_PCI,
                            (uint8)LinSlave_Tp_Transmit(0U, NULL_PTR, 10U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_INVALID_PCI,
                            (uint8)LinSlave_Tp_Transmit(0U, payload, 0U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_NOT_OK,
                            (uint8)LinSlave_Tp_Transmit(5U, payload, 10U));

    /* Multi-frame transmission enters WAIT_FC. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_Transmit(0U, payload, 10U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_WAIT_FC, (uint8)LinSlave_Tp_GetState(0U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_BUSY,
                            (uint8)LinSlave_Tp_Transmit(0U, payload, 10U));

    /* Flow control transitions to TX_CF. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x30U, fc, 2U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_TX_CF, (uint8)LinSlave_Tp_GetState(0U));

    /* FC received in the wrong state. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_E_NOT_OK,
                            (uint8)LinSlave_Tp_ProcessFrame(0x10U, 0x30U, fc, 2U));

    /* Waiting-for-FC timeout notifies the confirmation callback. */
    LinSlave_Tp_Cancel(0U);
    LinTest_ResetCounters();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_Transmit(0U, payload, 10U));
    for (i = 0U; i <= 1001U; i++) {
        LinSlave_Tp_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_TpTxCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)LINSLAVE_TP_E_TIMEOUT, LinTest_TpLastStatus);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_IDLE, (uint8)LinSlave_Tp_GetState(0U));

    /* Cancellation of an active transfer notifies the callback. */
    LinTest_ResetCounters();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_OK,
                            (uint8)LinSlave_Tp_Transmit(0U, payload, 10U));
    LinSlave_Tp_Cancel(0U);
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_TpTxCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)LINSLAVE_TP_E_NOT_OK, LinTest_TpLastStatus);

    LinSlave_Tp_Cancel(0U);
    LinSlave_Tp_Cancel(5U);
    TEST_ASSERT_EQUAL_UINT8(1U, LinTest_TpTxCount);

    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_TP_STATE_IDLE, (uint8)LinSlave_Tp_GetState(5U));
    TEST_ASSERT_FALSE(LinSlave_Tp_IsBusy(5U));

    LinSlave_Tp_DeInit();
    LinSlave_Tp_MainFunction();
}

/*============================================================================
 * LIN UDS
 *===========================================================================*/
static LinSlave_Uds_StatusType LinTest_CustomHandler(const LinSlave_Uds_RequestType* Request,
                                                     LinSlave_Uds_ResponseType* Response)
{
    Response->Sid = Request->Sid;
    Response->DataPtr[0] = 0x42U;
    Response->DataLength = 1U;
    return LinSlave_Uds_SendPositiveResponse(Response);
}

static LinSlave_Uds_StatusType LinTest_HandlerNoResponse(const LinSlave_Uds_RequestType* Request,
                                                         LinSlave_Uds_ResponseType* Response)
{
    (void)Request;
    (void)Response;
    return LINSLAVE_UDS_OK;
}

void test_linslave_UdsServices(void)
{
    uint8 sessionReq[2] = { LINSLAVE_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, 0x03U };
    uint8 resetReq[2] = { LINSLAVE_UDS_SID_ECU_RESET, 0x01U };
    uint8 testerReq[2] = { LINSLAVE_UDS_SID_TESTER_PRESENT, 0x00U };
    uint8 testerNoResp[2] = { LINSLAVE_UDS_SID_TESTER_PRESENT, 0x80U };
    uint8 unknownReq = 0xAAU;
    uint8 emptyReq = LINSLAVE_UDS_SID_TESTER_PRESENT;
    LinSlave_Uds_ServiceConfigType service;

    (void)LinSlave_Tp_Init();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK, (uint8)LinSlave_Uds_Init());
    LinSlave_Uds_RegisterDefaultServices();

    /* Parameter checks. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(NULL_PTR, 1U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(&emptyReq, 0U));

    /* Session control. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(sessionReq, 2U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_SESSION_EXTENDED,
                            (uint8)LinSlave_Uds_GetSessionType());

    /* ECU reset now permitted in the extended session. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(resetReq, 2U));

    /* Tester present with and without response. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(testerReq, 2U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(testerNoResp, 2U));

    /* Unknown SID. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_INVALID_SID,
                            (uint8)LinSlave_Uds_ProcessRequest(&unknownReq, 1U));

    /* Session restricted service in the default session. */
    LinSlave_Uds_SetSessionType(LINSLAVE_UDS_SESSION_DEFAULT);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_SESSION_NOT_SUPPORTED,
                            (uint8)LinSlave_Uds_ProcessRequest(resetReq, 2U));

    /* Security restricted service. */
    service.Sid = 0x99U;
    service.Handler = LinTest_CustomHandler;
    service.NeedsSecurity = TRUE;
    service.NeedsSession = FALSE;
    service.MinSession = LINSLAVE_UDS_SESSION_DEFAULT;
    service.MinSecurityLevel = LINSLAVE_UDS_SECURITY_LEVEL2;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_SECURITY_DENIED,
                            (uint8)LinSlave_Uds_ProcessRequest(&service.Sid, 1U));

    LinSlave_Uds_SetSecurityLevel(LINSLAVE_UDS_SECURITY_LEVEL2);
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(&service.Sid, 1U));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_SECURITY_LEVEL2,
                            (uint8)LinSlave_Uds_GetSecurityLevel());

    /* Register validation. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_RegisterService(NULL_PTR));
    service.Sid = 0x10U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service)); /* duplicate */
    service.Sid = 0x9AU;
    service.Handler = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service));

    /* Fill the table, then verify that no more services can be added. */
    service.Handler = LinTest_CustomHandler;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service));
    {
        uint8 sid;
        for (sid = 0x50U; sid < 0x50U + 40U; sid++) {
            service.Sid = sid;
            (void)LinSlave_Uds_RegisterService(&service);
        }
    }
    service.Sid = 0x7EU;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service)); /* full */

    /* Positive / negative response builders. */
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_SendPositiveResponse(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_SendNegativeResponse(0x10U, 0x10U));
    {
        uint8 respData = 0x5AU;
        LinSlave_Uds_ResponseType response;

        response.Sid = 0x10U;
        response.DataPtr = &respData;
        response.DataLength = 1U;
        response.IsNegative = FALSE;
        response.NegativeResponseCode = 0U;
        TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                                (uint8)LinSlave_Uds_SendPositiveResponse(&response));
        response.DataLength = 0U;
        TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                                (uint8)LinSlave_Uds_SendPositiveResponse(&response));
        response.DataLength = 1U;
        response.DataPtr = NULL_PTR;
        TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                                (uint8)LinSlave_Uds_SendPositiveResponse(&response));
    }

    LinSlave_Uds_DeInit();
    LinSlave_Uds_DeInit();
}

void test_linslave_UdsRegistrationAndTimeout(void)
{
    LinSlave_Uds_ServiceConfigType service;
    uint8 sid;
    uint16 i;

    (void)LinSlave_Tp_Init();
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK, (uint8)LinSlave_Uds_Init());

    /* Handled service that returns without sending a response. */
    service.Sid = 0x77U;
    service.Handler = LinTest_HandlerNoResponse;
    service.NeedsSecurity = FALSE;
    service.NeedsSession = FALSE;
    service.MinSession = LINSLAVE_UDS_SESSION_DEFAULT;
    service.MinSecurityLevel = LINSLAVE_UDS_SECURITY_LOCKED;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service));
    sid = 0x77U;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_OK,
                            (uint8)LinSlave_Uds_ProcessRequest(&sid, 1U));

    /* Fill the service table until it reports full. */
    service.Handler = LinTest_CustomHandler;
    for (i = 0x40U; i < 0x40U + 40U; i++) {
        service.Sid = i;
        (void)LinSlave_Uds_RegisterService(&service);
    }
    service.Sid = 0x7EU;
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_E_NOT_OK,
                            (uint8)LinSlave_Uds_RegisterService(&service));

    /* S3 timeout returns to the default session. */
    LinSlave_Uds_SetSessionType(LINSLAVE_UDS_SESSION_EXTENDED);
    LinSlave_Uds_SetSecurityLevel(LINSLAVE_UDS_SECURITY_LEVEL1);
    for (i = 0U; i < 100U; i++) {
        LinSlave_Uds_MainFunction();
    }
    TEST_ASSERT_FALSE(LinSlave_Uds_IsSessionTimeout());
    for (i = 0U; i < 5001U; i++) {
        LinSlave_Uds_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_SESSION_DEFAULT,
                            (uint8)LinSlave_Uds_GetSessionType());
    TEST_ASSERT_EQUAL_UINT8(LINSLAVE_UDS_SECURITY_LOCKED,
                            (uint8)LinSlave_Uds_GetSecurityLevel());

    /* Default services re-registration fails silently once the table is full. */
    LinSlave_Uds_RegisterDefaultServices();

    LinSlave_Uds_DeInit();
}

/*============================================================================
 * Helper modules
 *===========================================================================*/
void test_linslave_Helpers(void)
{
    uint8 data[3] = { 0x10U, 0x20U, 0x30U };

    TEST_ASSERT_EQUAL_UINT8(0x05U, LinSlave_ExtractId(0x45U));
    TEST_ASSERT_TRUE(LinSlave_ValidatePid(LinSlave_CalculatePid(5U)));
    TEST_ASSERT_FALSE(LinSlave_ValidatePid(0x00U));

    TEST_ASSERT_EQUAL_UINT8(LinSlave_CalculateChecksum(data, 3U, 0x10U, LINSLAVE_CHECKSUM_CLASSIC),
                            LinSlave_CalculateChecksum(data, 3U, 0x00U,
                                                       LINSLAVE_CHECKSUM_CLASSIC));
    TEST_ASSERT_TRUE(LinSlave_ValidateChecksum(
        data, 3U, 0x10U, LINSLAVE_CHECKSUM_ENHANCED,
        LinSlave_CalculateChecksum(data, 3U, 0x10U, LINSLAVE_CHECKSUM_ENHANCED)));
    TEST_ASSERT_FALSE(LinSlave_ValidateChecksum(
        data, 3U, 0x10U, LINSLAVE_CHECKSUM_ENHANCED, 0x00U));

    /* Zero length checksum still produces a value. */
    (void)LinSlave_CalculateChecksum(NULL_PTR, 0U, 0x00U, LINSLAVE_CHECKSUM_CLASSIC);

    (void)LinSlave_Hal_GetTimestampMs();
    LinSlave_Hal_DelayUs(10U);
    LinSlave_Hal_UartSendBuffer(data, 3U);
}

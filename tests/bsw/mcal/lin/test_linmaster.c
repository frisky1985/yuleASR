/**
 * @file test_linmaster.c
 * @brief LinMaster stack Unit Tests (substantiated)
 * @version 1.0.0
 * @date 2026-10-26
 *
 * Links the real production sources src/bsw/mcal/lin/src/LinMaster*.c
 * (core, Hal, Schedule, Diagnostic, Tp) against the host-safe HAL mock.
 *
 * Notes on product behaviour covered here:
 *  - LinMaster_Hal_GetCurrentTimeMs() increments by exactly 1 ms per call,
 *    so every MainFunction state transition is deterministic and the exact
 *    state after each call is asserted below (see traces in comments).
 *  - The RX response timeout is hard-wired to LINMASTER_RX_RESPONSE_TIMEOUT_MS
 *    (100); the config field ResponseTimeoutMs is never read by the product.
 *  - LinMaster_ProcessError() reports the byte position as RxIndex only in
 *    WAIT_RX_DATA, otherwise TxIndex (0 for receive frames) - documented below.
 *  - LinMaster_Tp_RxIndication() forwards the whole frame (PCI byte included)
 *    to the TP RxCallback, so the Diagnostic layer sees the PCI byte as the
 *    first UDS byte. A raw negative response (0x7F...) can therefore never
 *    reach LinMaster_Diag_ProcessResponse() through the public API - product
 *    gap, recorded here, not changed.
 *  - LinMaster_Schedule uses an internal call counter as time source, so
 *    elapsed-time values depend on the number of API calls (deterministic).
 *  - LinMaster_Schedule_AddEntry() writes the grown count back into the
 *    caller's (const) schedule table.
 */

// @tests src/bsw/mcal/lin/src/LinMaster.c  @tests src/bsw/mcal/lin/include/LinMaster.h
// @tests src/bsw/mcal/lin/src/LinMaster_Hal.c  @tests src/bsw/mcal/lin/include/LinMaster_Hal.h
// @tests src/bsw/mcal/lin/src/LinMaster_Schedule.c  @tests src/bsw/mcal/lin/include/LinMaster_Schedule.h
// @tests src/bsw/mcal/lin/src/LinMaster_Diagnostic.c  @tests src/bsw/mcal/lin/include/LinMaster_Diagnostic.h
// @tests src/bsw/mcal/lin/src/LinMaster_Tp.c  @tests src/bsw/mcal/lin/include/LinMaster_Tp.h

#include <string.h>
#include "unity.h"
#include "mock_registers.h"

#include "LinMaster.h"
#include "LinMaster_Hal.h"
#include "LinMaster_Cfg.h"
#include "LinMaster_Schedule.h"
#include "LinMaster_Diagnostic.h"
#include "LinMaster_Tp.h"
#include "LinSlave_Hal.h"

/*===========================================================================
 * Host stubs for LinSlave_Hal (used by LinMaster_Tp.c; the real
 * LinSlave_Hal.c is NOT linked into this test binary).
 *==========================================================================*/
static uint8 test_SlaveTxBuf[16];
static uint8 test_SlaveTxLen;
static uint32 test_SlaveTimeMs = 1000U;

void LinSlave_Hal_UartSendBuffer(const uint8* Buffer, uint8 Length)
{
    if ((Buffer != NULL) && (Length <= (uint8)sizeof(test_SlaveTxBuf))) {
        (void)memcpy(test_SlaveTxBuf, Buffer, Length);
        test_SlaveTxLen = Length;
    }
}

uint32_t LinSlave_Hal_GetTimestampMs(void)
{
    return test_SlaveTimeMs;
}

/*===========================================================================
 * LinMaster core callback captures
 *==========================================================================*/
static boolean test_RxCbCalled;
static uint8 test_RxCbPid;
static uint8 test_RxCbData[8];
static uint8 test_RxCbLen;
static boolean test_RxCbChecksumValid;

static boolean test_TxCbCalled;
static uint8 test_TxCbPid;
static uint8 test_TxCbData[8];
static uint8 test_TxCbLen;
static boolean test_TxCbSuccess;

static boolean test_ErrCbCalled;
static LinMaster_ErrorType test_ErrCbError;
static uint8 test_ErrCbPid;
static uint8 test_ErrCbPos;

static boolean test_StateCbCalled;
static uint32 test_StateCbCount;
static LinMaster_StateType test_StateCbOld;
static LinMaster_StateType test_StateCbNew;

static void test_Core_RxCb(uint8 Pid, const uint8* DataPtr, uint8 Length, boolean IsChecksumValid)
{
    test_RxCbCalled = TRUE;
    test_RxCbPid = Pid;
    test_RxCbLen = Length;
    test_RxCbChecksumValid = IsChecksumValid;
    if ((DataPtr != NULL) && (Length <= sizeof(test_RxCbData))) {
        (void)memcpy(test_RxCbData, DataPtr, Length);
    }
}

static void test_Core_TxCb(uint8 Pid, uint8* DataPtr, uint8 Length, boolean IsSuccessful)
{
    test_TxCbCalled = TRUE;
    test_TxCbPid = Pid;
    test_TxCbLen = Length;
    test_TxCbSuccess = IsSuccessful;
    if ((DataPtr != NULL) && (Length <= sizeof(test_TxCbData))) {
        (void)memcpy(test_TxCbData, DataPtr, Length);
    }
}

static void test_Core_ErrCb(LinMaster_ErrorType ErrorCode, uint8 Pid, uint8 BytePosition)
{
    test_ErrCbCalled = TRUE;
    test_ErrCbError = ErrorCode;
    test_ErrCbPid = Pid;
    test_ErrCbPos = BytePosition;
}

static void test_Core_StateCb(LinMaster_StateType OldState, LinMaster_StateType NewState)
{
    test_StateCbCalled = TRUE;
    test_StateCbCount++;
    test_StateCbOld = OldState;
    test_StateCbNew = NewState;
}

/*===========================================================================
 * Schedule callback captures
 *==========================================================================*/
static uint32 test_Sch_EntryCbCount;
static uint8 test_Sch_EntryCbIndex;
static LinMaster_EntryStateType test_Sch_EntryCbState;
static uint8 test_Sch_EntryCbPid;

static boolean test_Sch_TableCompleteCalled;
static const LinMaster_ScheduleTableType* test_Sch_CompleteTable;

static boolean test_Sch_SwitchCbCalled;
static const LinMaster_ScheduleTableType* test_Sch_SwitchOld;
static const LinMaster_ScheduleTableType* test_Sch_SwitchNew;

static void test_Sch_EntryCb(const LinMaster_ScheduleEntryType* Entry, uint8 EntryIndex,
                             LinMaster_EntryStateType State)
{
    test_Sch_EntryCbCount++;
    test_Sch_EntryCbIndex = EntryIndex;
    test_Sch_EntryCbState = State;
    test_Sch_EntryCbPid = (Entry != NULL) ? Entry->Pid : 0U;
}

static void test_Sch_CompleteCb(const LinMaster_ScheduleTableType* Table)
{
    test_Sch_TableCompleteCalled = TRUE;
    test_Sch_CompleteTable = Table;
}

static void test_Sch_SwitchCb(const LinMaster_ScheduleTableType* OldTable,
                              const LinMaster_ScheduleTableType* NewTable)
{
    test_Sch_SwitchCbCalled = TRUE;
    test_Sch_SwitchOld = OldTable;
    test_Sch_SwitchNew = NewTable;
}

/*===========================================================================
 * Diagnostic callback capture
 *==========================================================================*/
static boolean test_Diag_CbCalled;
static LinMaster_Diag_StatusType test_Diag_CbStatus;
static boolean test_Diag_CbRespValid;
static uint8 test_Diag_CbRespSid;
static uint8 test_Diag_CbRespNrc;
static boolean test_Diag_CbRespNegative;

static void test_Diag_Cb(LinMaster_Diag_StatusType Status, const LinMaster_Diag_ResponseType* ResponsePtr)
{
    test_Diag_CbCalled = TRUE;
    test_Diag_CbStatus = Status;
    if (ResponsePtr != NULL) {
        test_Diag_CbRespValid = TRUE;
        test_Diag_CbRespSid = ResponsePtr->ResponseSid;
        test_Diag_CbRespNrc = ResponsePtr->Nrc;
        test_Diag_CbRespNegative = ResponsePtr->IsNegative;
    } else {
        test_Diag_CbRespValid = FALSE;
    }
}

/*===========================================================================
 * TP callback captures
 *==========================================================================*/
static boolean test_Tp_RxCbCalled;
static uint8 test_Tp_RxCbData[16];
static uint16 test_Tp_RxCbLen;

static boolean test_Tp_ConfirmCalled;
static uint32 test_Tp_ConfirmCount;
static LinMaster_Tp_StatusType test_Tp_ConfirmStatus;

static void test_Tp_RxCb(uint8 ChannelId, const uint8* DataPtr, uint16 Length)
{
    (void)ChannelId;
    test_Tp_RxCbCalled = TRUE;
    test_Tp_RxCbLen = Length;
    if ((DataPtr != NULL) && (Length <= sizeof(test_Tp_RxCbData))) {
        (void)memcpy(test_Tp_RxCbData, DataPtr, Length);
    }
}

static void test_Tp_ConfirmCb(uint8 ChannelId, LinMaster_Tp_StatusType Status)
{
    (void)ChannelId;
    test_Tp_ConfirmCalled = TRUE;
    test_Tp_ConfirmCount++;
    test_Tp_ConfirmStatus = Status;
}

/*===========================================================================
 * Test configuration / helpers
 *==========================================================================*/
/* Default config: InterFrameDelayMs = 0 so the DELAY state expires on the
 * next MainFunction call (StartTimeout(0) -> IsTimeout always true). */
static LinMaster_ConfigType test_CoreConfig = {
    1U,     /* BaudRate = 19200 */
    0U,     /* CustomBaudRate */
    728U,   /* BreakDurationUs */
    0U,     /* InterFrameDelayMs */
    100U    /* ResponseTimeoutMs (NOTE: unused by product) */
};

static LinMaster_ConfigType test_CoreConfigDelay = {
    1U, 0U, 728U,
    2U,     /* InterFrameDelayMs = 2 -> DELAY holds for 2 MainFunction calls */
    100U
};

static void test_ResetCaptures(void)
{
    (void)memset(&test_RxCbData, 0, sizeof(test_RxCbData));
    test_RxCbCalled = FALSE; test_RxCbPid = 0U; test_RxCbLen = 0U; test_RxCbChecksumValid = FALSE;
    (void)memset(&test_TxCbData, 0, sizeof(test_TxCbData));
    test_TxCbCalled = FALSE; test_TxCbPid = 0U; test_TxCbLen = 0U; test_TxCbSuccess = FALSE;
    test_ErrCbCalled = FALSE; test_ErrCbError = LINMASTER_ERROR_NONE; test_ErrCbPid = 0U; test_ErrCbPos = 0U;
    test_StateCbCalled = FALSE; test_StateCbCount = 0U;
    test_StateCbOld = LINMASTER_STATE_UNINIT; test_StateCbNew = LINMASTER_STATE_UNINIT;

    test_Sch_EntryCbCount = 0U; test_Sch_EntryCbIndex = 0U;
    test_Sch_EntryCbState = LINMASTER_ENTRY_STATE_IDLE; test_Sch_EntryCbPid = 0U;
    test_Sch_TableCompleteCalled = FALSE; test_Sch_CompleteTable = NULL;
    test_Sch_SwitchCbCalled = FALSE; test_Sch_SwitchOld = NULL; test_Sch_SwitchNew = NULL;

    test_Diag_CbCalled = FALSE; test_Diag_CbStatus = LINMASTER_DIAG_OK;
    test_Diag_CbRespValid = FALSE; test_Diag_CbRespSid = 0U; test_Diag_CbRespNrc = 0U;
    test_Diag_CbRespNegative = FALSE;

    test_Tp_RxCbCalled = FALSE; test_Tp_RxCbLen = 0U;
    (void)memset(&test_Tp_RxCbData, 0, sizeof(test_Tp_RxCbData));
    test_Tp_ConfirmCalled = FALSE; test_Tp_ConfirmCount = 0U;
    test_Tp_ConfirmStatus = LINMASTER_TP_OK;
}

/* DeInit (no-op when uninit) + Init: restores a canonical post-Init state. */
static void test_Core_EnsureInit(const LinMaster_ConfigType* ConfigPtr)
{
    LinMaster_DeInit();
    (void)LinMaster_Init(ConfigPtr);
}

void setUp(void)
{
    MockRegisters_Reset();
    test_ResetCaptures();
    test_SlaveTxLen = 0U;
    (void)memset(test_SlaveTxBuf, 0, sizeof(test_SlaveTxBuf));
}

void tearDown(void)
{
    LinMaster_DeInit();
    LinMaster_Schedule_DeInit();
    LinMaster_Diag_DeInit();
    LinMaster_Tp_DeInit();
}

/*===========================================================================
 * LinMaster core - BeforeInit tests (executed first by generated runner)
 *===========================================================================*/

/** Uninitialised module rejects every API and keeps the UNINIT state. */
void test_LinMaster_BeforeInit_ApiReturnsNotOk_StateStaysUninit(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendHeader(0xC1U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendBreak());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendSync());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendWakeup());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_GoToSleep());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_UNINIT, LinMaster_GetState());

    /* Interrupt handlers and MainFunction must be no-ops before Init. */
    LinMaster_MainFunction();
    LinMaster_RxInterruptHandler(0x55U);
    LinMaster_TxCompleteInterruptHandler();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_UNINIT, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NONE, LinMaster_GetLastError());
}

/** DeInit before Init is a no-op and must not change the state. */
void test_LinMaster_BeforeInit_DeInit_IsNoOp(void)
{
    LinMaster_DeInit();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_UNINIT, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NONE, LinMaster_GetLastError());
}

/*===========================================================================
 * LinMaster core - Init / DeInit / VersionInfo
 *===========================================================================*/

/** NOTE: LinMaster_Init reports errors only via return value, no DET. */
void test_LinMaster_Init_NullConfig_ReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_Init(NULL));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_UNINIT, LinMaster_GetState());
}

void test_LinMaster_Init_Valid_ReturnsOk_EntersIdle(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Init(&test_CoreConfig));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NONE, LinMaster_GetLastError());
}

/** Re-Init while initialized is rejected (LINMASTER_DEV_ERROR_DETECT == ON). */
void test_LinMaster_Init_Twice_SecondReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Init(&test_CoreConfig));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_Init(&test_CoreConfig));
}

/** All three baud rate selectors are accepted (0=9600, 1=19200, 2=custom). */
void test_LinMaster_Init_ConfigVariants_AllAccepted(void)
{
    LinMaster_ConfigType cfg;

    cfg = test_CoreConfig; cfg.BaudRate = 0U; cfg.CustomBaudRate = 0U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Init(&cfg));
    LinMaster_DeInit();

    cfg = test_CoreConfig; cfg.BaudRate = 2U; cfg.CustomBaudRate = 10400U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Init(&cfg));
    LinMaster_DeInit();

    cfg = test_CoreConfig; cfg.BaudRate = 1U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Init(&cfg));
}

void test_LinMaster_DeInit_ResetsState_AndClearsCallbacks(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    /* NOTE: Init raises one state callback (UNINIT -> IDLE), but it fires
     * before this registration, so the counter is still 0 here. */
    LinMaster_RegisterStateCallback(test_Core_StateCb);
    TEST_ASSERT_EQUAL_UINT32(0U, test_StateCbCount);

    /* NOTE: DeInit clears all callbacks BEFORE changing the state, so the
     * UNINIT transition itself does not raise the registered callback. */
    LinMaster_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, test_StateCbCount);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_UNINIT, LinMaster_GetState());

    /* Callbacks cleared by DeInit: re-Init must not raise the old callback. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Init(&test_CoreConfig));
    TEST_ASSERT_EQUAL_UINT32(0U, test_StateCbCount);
}

void test_LinMaster_GetVersionInfo_ReturnsModuleVersions(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    LinMaster_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(0x00U, info.vendorID);
    TEST_ASSERT_EQUAL_UINT32(0x51U, info.moduleID);
    TEST_ASSERT_EQUAL_UINT32(1U, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT32(0U, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT32(0U, info.sw_patch_version);

    /* NULL pointer tolerated (no crash, no write). */
    LinMaster_GetVersionInfo(NULL);
}

/*===========================================================================
 * LinMaster core - PID helpers
 *===========================================================================*/

/** LIN parity: P0 = ID0^ID1^ID2^ID4, P1 = ~(ID1^ID3^ID4^ID5) & 1. */
void test_LinMaster_CalculateProtectedId_KnownVectors(void)
{
    TEST_ASSERT_EQUAL_UINT8(0xC1U, LinMaster_CalculateProtectedId(0x01U));
    TEST_ASSERT_EQUAL_UINT8(0xF0U, LinMaster_CalculateProtectedId(0x30U));
    TEST_ASSERT_EQUAL_UINT8(0xFBU, LinMaster_CalculateProtectedId(0x3BU));
}

/** IDs above 59 are masked with 0x3F before the parity computation. */
void test_LinMaster_CalculateProtectedId_MasksIdTo6Bits(void)
{
    TEST_ASSERT_EQUAL_UINT8(0x03U, LinMaster_CalculateProtectedId(0x43U));
}

void test_LinMaster_ValidateProtectedId_AcceptsOnlyValidParity(void)
{
    TEST_ASSERT_EQUAL_UINT32(TRUE, LinMaster_ValidateProtectedId(0xC1U));
    TEST_ASSERT_EQUAL_UINT32(TRUE, LinMaster_ValidateProtectedId(0xF0U));   /* ID 0x30 */
    TEST_ASSERT_EQUAL_UINT32(FALSE, LinMaster_ValidateProtectedId(0x30U));  /* missing parity */
    TEST_ASSERT_EQUAL_UINT32(FALSE, LinMaster_ValidateProtectedId(0xC2U));  /* ID 2 -> 0x42 */
    TEST_ASSERT_EQUAL_UINT32(FALSE, LinMaster_ValidateProtectedId(0xFFU));  /* ID 0x3F -> 0xBF */
}

void test_LinMaster_ExtractId_StripsParityBits(void)
{
    TEST_ASSERT_EQUAL_UINT8(0x01U, LinMaster_ExtractId(0xC1U));
    TEST_ASSERT_EQUAL_UINT8(0x3DU, LinMaster_ExtractId(0x3DU));
}

/*===========================================================================
 * LinMaster core - parameter validation
 *===========================================================================*/

void test_LinMaster_SendHeader_InvalidPid_ReturnsNotOk(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendHeader(0xC2U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

void test_LinMaster_SendFrame_ParamValidation(void)
{
    uint8 data[2] = {0x01U, 0x02U};

    test_Core_EnsureInit(&test_CoreConfig);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendFrame(0xC1U, NULL, 1U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendFrame(0xC1U, data, 0U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendFrame(0xC1U, data, 9U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_SendFrame(0xC2U, data, 1U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

void test_LinMaster_SendFrame_LengthBoundary8_Accepted(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8 i;

    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterTxCallback(test_Core_TxCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendFrame(0xC1U, data, 8U, LINMASTER_CHECKSUM_CLASSIC));
    /* Break, Sync, PID, 8 data bytes, checksum, delay expiry. */
    for (i = 0U; i < 13U; i++) {
        LinMaster_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_TRUE(test_TxCbCalled);
    TEST_ASSERT_EQUAL_UINT8(8U, test_TxCbLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, test_TxCbData, 8U);
}

void test_LinMaster_ReceiveFrame_ParamValidation(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_ReceiveFrame(0xC1U, 0U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_ReceiveFrame(0xC1U, 9U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_ReceiveFrame(0xC2U, 1U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

/*===========================================================================
 * LinMaster core - busy / timeout behaviour
 *===========================================================================*/

/**
 * While an operation is pending every new request returns LINMASTER_BUSY.
 * The pending request is an RX frame (ReceiveFrame forces direction RX), so
 * without slave data the state machine waits in WAIT_RX_DATA and times out
 * after LINMASTER_RX_RESPONSE_TIMEOUT_MS (100) MainFunction calls with
 * LINMASTER_ERROR_NO_RESPONSE.
 */
void test_LinMaster_Busy_RequestsRejected_ThenNoResponseTimeout(void)
{
    uint8 data[1] = {0x55U};
    uint32 i;

    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterErrorCallback(test_Core_ErrCb);

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_ReceiveFrame(0xC1U, 1U, LINMASTER_CHECKSUM_CLASSIC));

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_BUSY, LinMaster_SendHeader(0xC1U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_BUSY, LinMaster_SendFrame(0xC1U, data, 1U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_BUSY, LinMaster_ReceiveFrame(0xC1U, 1U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_BUSY, LinMaster_SendWakeup());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_BUSY, LinMaster_GoToSleep());

    /* Drive header: BREAK -> SYNC -> PID -> WAIT_RX_DATA. */
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_RX_DATA, LinMaster_GetState());

    /* 100 ms RX timeout -> NO_RESPONSE. Loop with early exit for robustness. */
    for (i = 0U; i < 200U; i++) {
        LinMaster_MainFunction();
        if (LinMaster_GetState() != LINMASTER_STATE_WAIT_RX_DATA) {
            break;
        }
    }
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NO_RESPONSE, LinMaster_GetLastError());
    TEST_ASSERT_TRUE(test_ErrCbCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NO_RESPONSE, test_ErrCbError);
    TEST_ASSERT_EQUAL_UINT8(0xC1U, test_ErrCbPid);
    TEST_ASSERT_EQUAL_UINT8(0U, test_ErrCbPos);

    /* Error path cleared the pending flag: a new request is accepted. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendHeader(0xC1U));
}

/*===========================================================================
 * LinMaster core - TX frame flow
 *===========================================================================*/

/**
 * Full TX frame with classic checksum, deterministic per MainFunction call:
 *  #1 BREAK -> SYNC, #2 SYNC -> PID, #3 PID -> TX_DATA, #4 data byte,
 *  #5 WAIT_CHECKSUM (checksum + TxCallback -> DELAY), #6 DELAY -> IDLE.
 */
void test_LinMaster_SendFrame_Classic_CompletesWithTxCallback(void)
{
    uint8 data[1] = {0x01U};

    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterTxCallback(test_Core_TxCb);

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendFrame(0xC1U, data, 1U, LINMASTER_CHECKSUM_CLASSIC));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_SEND_BREAK, LinMaster_GetState());

    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_SEND_SYNC, LinMaster_GetState());
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_SEND_PID, LinMaster_GetState());
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_SEND_TX_DATA, LinMaster_GetState());
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_CHECKSUM, LinMaster_GetState());
    TEST_ASSERT_FALSE(test_TxCbCalled);
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_DELAY, LinMaster_GetState());
    TEST_ASSERT_TRUE(test_TxCbCalled);
    TEST_ASSERT_EQUAL_UINT8(0xC1U, test_TxCbPid);
    TEST_ASSERT_EQUAL_UINT8(1U, test_TxCbLen);
    TEST_ASSERT_EQUAL_UINT8(0x01U, test_TxCbData[0]);
    TEST_ASSERT_EQUAL_UINT32(TRUE, test_TxCbSuccess);

    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NONE, LinMaster_GetLastError());

    /* Frame complete: module is free again. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendHeader(0xC1U));
}

/** Enhanced checksum differs from classic only by including the PID. */
void test_LinMaster_SendFrame_Enhanced_Completes(void)
{
    uint8 data[2] = {0x01U, 0x02U};

    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterTxCallback(test_Core_TxCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendFrame(0xC1U, data, 2U, LINMASTER_CHECKSUM_ENHANCED));

    /* Break, Sync, PID, 2 data bytes, checksum, delay expiry. */
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_CHECKSUM, LinMaster_GetState());
    TEST_ASSERT_FALSE(test_TxCbCalled);
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_DELAY, LinMaster_GetState());
    TEST_ASSERT_TRUE(test_TxCbCalled);
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

/** TX callback data must match the buffer passed to SendFrame. */
void test_LinMaster_TxCallback_DeliversOriginalData(void)
{
    uint8 data[3] = {0xA1U, 0xB2U, 0xC3U};

    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterTxCallback(test_Core_TxCb);
    /* 0xF0 is the protected id of identifier 0x30. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendFrame(0xF0U, data, 3U, LINMASTER_CHECKSUM_CLASSIC));

    /* Break, Sync, PID, 3 data bytes, checksum, delay expiry. */
    /* NOTE: product never clears HeaderRequested on the success path, so the
     * MainFunction call after the frame completes (state back in IDLE) starts
     * another header cycle.  Stop exactly when IDLE is reached (8 calls). */
    uint8 i;
    for (i = 0U; i < 8U; i++) {
        LinMaster_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT8(0xF0U, test_TxCbPid);
    TEST_ASSERT_EQUAL_UINT8(3U, test_TxCbLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, test_TxCbData, 3U);
}

/*===========================================================================
 * LinMaster core - RX frame flow
 *===========================================================================*/

/**
 * Receive flow (classic): 3 MainFunction calls bring the state machine to
 * WAIT_RX_DATA; the checksum is computed over the data only (PID excluded).
 */
void test_LinMaster_ReceiveFrame_Classic_RxCallbackFires(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterRxCallback(test_Core_RxCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_ReceiveFrame(0xC1U, 2U, LINMASTER_CHECKSUM_CLASSIC));

    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_RX_DATA, LinMaster_GetState());

    LinMaster_RxInterruptHandler(0x02U);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_RX_DATA, LinMaster_GetState());
    LinMaster_RxInterruptHandler(0x03U);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_CHECKSUM, LinMaster_GetState());

    /* Classic checksum over {0x02, 0x03} = ~0x05 = 0xFA. */
    LinMaster_RxInterruptHandler(0xFAU);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_DELAY, LinMaster_GetState());
    TEST_ASSERT_TRUE(test_RxCbCalled);
    TEST_ASSERT_EQUAL_UINT8(0xC1U, test_RxCbPid);
    TEST_ASSERT_EQUAL_UINT8(2U, test_RxCbLen);
    TEST_ASSERT_EQUAL_UINT8(0x02U, test_RxCbData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x03U, test_RxCbData[1]);
    TEST_ASSERT_EQUAL_UINT32(TRUE, test_RxCbChecksumValid);

    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

/** Enhanced checksum includes the PID: sum = 0xC1 + 0x01 = 0xC2 -> ~0xC2. */
void test_LinMaster_ReceiveFrame_Enhanced_ChecksumIncludesPid(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterRxCallback(test_Core_RxCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_ReceiveFrame(0xC1U, 1U, LINMASTER_CHECKSUM_ENHANCED));

    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_RX_DATA, LinMaster_GetState());

    LinMaster_RxInterruptHandler(0x01U);
    LinMaster_RxInterruptHandler(0x3DU);    /* ~0xC2 */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_DELAY, LinMaster_GetState());
    TEST_ASSERT_TRUE(test_RxCbCalled);
    TEST_ASSERT_EQUAL_UINT32(TRUE, test_RxCbChecksumValid);
}

/** Wrong checksum: frame rejected, error callback, back to IDLE. */
void test_LinMaster_ReceiveFrame_BadChecksum_ErrorCallback(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RegisterRxCallback(test_Core_RxCb);
    LinMaster_RegisterErrorCallback(test_Core_ErrCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_ReceiveFrame(0xC1U, 1U, LINMASTER_CHECKSUM_CLASSIC));

    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_RxInterruptHandler(0x02U);    /* data */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_WAIT_CHECKSUM, LinMaster_GetState());

    /* NOTE: product reports BytePosition = TxIndex (0) here because the
     * position selector only uses RxIndex in WAIT_RX_DATA state. */
    LinMaster_RxInterruptHandler(0x00U);    /* wrong checksum (expected 0xFD) */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_CHECKSUM, LinMaster_GetLastError());
    TEST_ASSERT_FALSE(test_RxCbCalled);
    TEST_ASSERT_TRUE(test_ErrCbCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_CHECKSUM, test_ErrCbError);
    TEST_ASSERT_EQUAL_UINT8(0xC1U, test_ErrCbPid);
    TEST_ASSERT_EQUAL_UINT8(0x00U, test_ErrCbPos);
}

/** Bytes received while IDLE are ignored (default case of the handler). */
void test_LinMaster_RxInterruptHandler_IdleState_IgnoresByte(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    LinMaster_RxInterruptHandler(0x55U);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ERROR_NONE, LinMaster_GetLastError());
}

/** The configured InterFrameDelayMs holds the DELAY state (2 ms here). */
void test_LinMaster_DelayState_WaitsInterFrameDelay(void)
{
    test_Core_EnsureInit(&test_CoreConfigDelay);
    LinMaster_RegisterRxCallback(test_Core_RxCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_ReceiveFrame(0xC1U, 1U, LINMASTER_CHECKSUM_CLASSIC));

    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_MainFunction();
    LinMaster_RxInterruptHandler(0x07U);    /* data */
    LinMaster_RxInterruptHandler(0xF8U);    /* ~0x07 checksum */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_DELAY, LinMaster_GetState());

    LinMaster_MainFunction();               /* elapsed 1 < 2 */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_DELAY, LinMaster_GetState());
    LinMaster_MainFunction();               /* elapsed 2 >= 2 */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

/*===========================================================================
 * LinMaster core - misc APIs
 *===========================================================================*/

void test_LinMaster_SendWakeup_GoToSleep_AfterInit_ReturnOk(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendWakeup());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_GoToSleep());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

void test_LinMaster_SendBreak_SendSync_AfterInit_ReturnOk(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendBreak());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendSync());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_IDLE, LinMaster_GetState());
}

void test_LinMaster_TxCompleteInterruptHandler_IsNoOp(void)
{
    test_Core_EnsureInit(&test_CoreConfig);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_SendHeader(0xC1U));
    LinMaster_TxCompleteInterruptHandler();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_STATE_SEND_BREAK, LinMaster_GetState());
}

/*===========================================================================
 * LinMaster HAL (host-safe mock implementation)
 *===========================================================================*/

void test_LinMasterHal_UartInit_ReturnsOk_BusRecessive(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Hal_UartInit(9600U));
    TEST_ASSERT_EQUAL_UINT8(1U, LinMaster_Hal_GetBusState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Hal_SendByte(0x55U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Hal_SendBreak());
    TEST_ASSERT_EQUAL_UINT8(1U, LinMaster_Hal_GetBusState());   /* recessive again */
}

void test_LinMasterHal_GetCurrentTimeMs_IncrementsByOne(void)
{
    uint32 t1 = LinMaster_Hal_GetCurrentTimeMs();
    uint32 t2 = LinMaster_Hal_GetCurrentTimeMs();
    TEST_ASSERT_EQUAL_UINT32(t1 + 1U, t2);
}

void test_LinMasterHal_SendBlock_ParamValidation(void)
{
    uint8 data[3] = {1U, 2U, 3U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_Hal_SendBlock(NULL, 3U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_NOT_OK, LinMaster_Hal_SendBlock(data, 0U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Hal_SendBlock(data, 3U));
}

void test_LinMasterHal_InterruptEnableDisable_NoCrash(void)
{
    LinMaster_Hal_EnableRxInterrupt();
    LinMaster_Hal_DisableRxInterrupt();
    LinMaster_Hal_EnableTxInterrupt();
    LinMaster_Hal_DisableTxInterrupt();
    LinMaster_Hal_DelayMs(1U);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_OK, LinMaster_Hal_UartInit(19200U));
}

/*===========================================================================
 * Schedule - BeforeInit tests
 *===========================================================================*/

void test_Schedule_BeforeInit_ApiGuarded(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_UNINIT, LinMaster_Schedule_GetState());
    TEST_ASSERT_NULL(LinMaster_Schedule_GetCurrentTable());
    TEST_ASSERT_NULL(LinMaster_Schedule_GetCurrentEntry());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Stop());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_SetDelay(10U));
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_SetEntryIndex(0U));
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Reset());
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
    TEST_ASSERT_EQUAL_UINT32(0U, LinMaster_Schedule_GetElapsedTime());
    TEST_ASSERT_EQUAL_UINT32(0U, LinMaster_Schedule_GetRemainingTime());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsRunning());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsCyclic());

    LinMaster_Schedule_Process();       /* no-op */
    LinMaster_Schedule_SetCyclic(TRUE); /* no-op */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_UNINIT, LinMaster_Schedule_GetState());
}

/*===========================================================================
 * Schedule - Init / lifecycle
 *===========================================================================*/

static LinMaster_ScheduleEntryType test_SchEntry0 = {
    .Pid = 0xC1U, .DataLength = 1U, .DelayMs = 1U,
    .EntryType = LINMASTER_ENTRY_UNCONDITIONAL,
    .Direction = LINMASTER_FRAME_DIR_TX, .ChecksumType = LINMASTER_CHECKSUM_CLASSIC
};
static LinMaster_ScheduleEntryType test_SchEntry1 = {
    .Pid = 0x42U, .DataLength = 2U, .DelayMs = 1U,
    .EntryType = LINMASTER_ENTRY_UNCONDITIONAL,
    .Direction = LINMASTER_FRAME_DIR_TX, .ChecksumType = LINMASTER_CHECKSUM_CLASSIC
};
static LinMaster_ScheduleEntryType test_SchEntry2 = {
    .Pid = 0x03U, .DataLength = 3U, .DelayMs = 1U,
    .EntryType = LINMASTER_ENTRY_UNCONDITIONAL,
    .Direction = LINMASTER_FRAME_DIR_TX, .ChecksumType = LINMASTER_CHECKSUM_CLASSIC
};
static LinMaster_ScheduleEntryType test_SchEntrySlow = {
    .Pid = 0xC1U, .DataLength = 1U, .DelayMs = 5U,
    .EntryType = LINMASTER_ENTRY_UNCONDITIONAL,
    .Direction = LINMASTER_FRAME_DIR_TX, .ChecksumType = LINMASTER_CHECKSUM_CLASSIC
};
static LinMaster_ScheduleEntryType test_SchEntryAdded = {
    .Pid = 0x42U, .DataLength = 4U, .DelayMs = 1U,
    .EntryType = LINMASTER_ENTRY_EVENT_TRIGGERED,
    .Direction = LINMASTER_FRAME_DIR_RX, .ChecksumType = LINMASTER_CHECKSUM_ENHANCED
};

static LinMaster_ScheduleTableType test_SchTblStart = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = FALSE, .LastEntryTime = 0U, .Name = "start"
};
static LinMaster_ScheduleTableType test_SchTblNonCyclic = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = FALSE, .LastEntryTime = 0U, .Name = "once"
};
static LinMaster_ScheduleTableType test_SchTblCyclic = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = TRUE, .LastEntryTime = 0U, .Name = "cyclic"
};
static LinMaster_ScheduleTableType test_SchTblSingle = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = TRUE, .LastEntryTime = 0U, .Name = "single"
};
static LinMaster_ScheduleTableType test_SchTblSlow = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = FALSE, .LastEntryTime = 0U, .Name = "slow"
};
static LinMaster_ScheduleTableType test_SchTblAdd = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = FALSE, .LastEntryTime = 0U, .Name = "add"
};
static LinMaster_ScheduleTableType test_SchTblSwA = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = TRUE, .LastEntryTime = 0U, .Name = "swA"
};
static LinMaster_ScheduleTableType test_SchTblSwB = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = FALSE, .LastEntryTime = 0U, .Name = "swB"
};
static LinMaster_ScheduleTableType test_SchTblBig = {
    .Entries = NULL, .EntryCount = 0U, .EntryIndex = 0U,
    .IsRunning = FALSE, .IsCyclic = FALSE, .LastEntryTime = 0U, .Name = "big"
};

/* Populate a table with the given entries before Init. */
static void test_Sch_FillTable(LinMaster_ScheduleTableType* Table,
                               const LinMaster_ScheduleEntryType* Entries,
                               uint8 Count)
{
    Table->Entries = Entries;
    Table->EntryCount = Count;
    Table->EntryIndex = 0U;
    Table->IsRunning = FALSE;
}

void test_Schedule_Init_NullTable_Ok_ButStartFails(void)
{
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(NULL));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_IDLE, LinMaster_Schedule_GetState());
    TEST_ASSERT_NULL(LinMaster_Schedule_GetCurrentTable());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Stop());
}

/** Entry counts above LINMASTER_SCHEDULE_MAX_ENTRIES (32) are rejected. */
void test_Schedule_Init_TooManyEntries_ReturnsNotOk(void)
{
    const LinMaster_ScheduleEntryType entries[1] = {{0}};
    test_Sch_FillTable(&test_SchTblBig, entries, 33U);
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Init(&test_SchTblBig));
}

void test_Schedule_StartStop_Lifecycle(void)
{
    test_Sch_FillTable(&test_SchTblStart, &test_SchEntry0, 2U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblStart));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_IDLE, LinMaster_Schedule_GetState());
    TEST_ASSERT_EQUAL_PTR(&test_SchTblStart, LinMaster_Schedule_GetCurrentTable());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsRunning());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsCyclic());

    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());
    TEST_ASSERT_TRUE(LinMaster_Schedule_IsRunning());
    TEST_ASSERT_EQUAL_UINT32(1U, test_Sch_EntryCbCount);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ENTRY_STATE_EXECUTING, test_Sch_EntryCbState);
    TEST_ASSERT_EQUAL_UINT8(0U, test_Sch_EntryCbIndex);

    /* Start while running is accepted. */
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Stop());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_IDLE, LinMaster_Schedule_GetState());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsRunning());
    /* Stop when not running fails. */
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Stop());
}

/**
 * 3 entries x 1 ms delay, non-cyclic: each Process() completes the current
 * entry and advances; the third Process finishes the table (IDLE + complete
 * callback). NOTE: GetSystemTime() is an internal call counter, so the delay
 * of 1 ms is satisfied on the first Process after Start.
 */
void test_Schedule_Process_NonCyclic_TableCompletes(void)
{
    const LinMaster_ScheduleEntryType entries[3] = {test_SchEntry0, test_SchEntry1, test_SchEntry2};

    test_Sch_FillTable(&test_SchTblNonCyclic, entries, 3U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblNonCyclic));
    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    LinMaster_Schedule_RegisterTableCompleteCallback(test_Sch_CompleteCb);

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());

    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT8(1U, LinMaster_Schedule_GetEntryIndex());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());

    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT8(2U, LinMaster_Schedule_GetEntryIndex());

    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_IDLE, LinMaster_Schedule_GetState());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsRunning());
    TEST_ASSERT_TRUE(test_Sch_TableCompleteCalled);
    TEST_ASSERT_EQUAL_PTR(&test_SchTblNonCyclic, test_Sch_CompleteTable);
    /* Last callback was COMPLETED of entry index 2. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ENTRY_STATE_COMPLETED, test_Sch_EntryCbState);
    TEST_ASSERT_EQUAL_UINT8(2U, test_Sch_EntryCbIndex);
}

/** Cyclic table wraps back to entry 0 and keeps running. */
void test_Schedule_Process_Cyclic_WrapsAround(void)
{
    const LinMaster_ScheduleEntryType entries[3] = {test_SchEntry0, test_SchEntry1, test_SchEntry2};

    test_Sch_FillTable(&test_SchTblCyclic, entries, 3U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblCyclic));
    TEST_ASSERT_TRUE(LinMaster_Schedule_IsCyclic());
    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());

    LinMaster_Schedule_Process();
    LinMaster_Schedule_Process();
    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_ENTRY_STATE_EXECUTING, test_Sch_EntryCbState);
    TEST_ASSERT_EQUAL_UINT8(0U, test_Sch_EntryCbIndex);
    /* 1 (start) + 3 x (COMPLETED + EXECUTING) callbacks. */
    TEST_ASSERT_EQUAL_UINT32(7U, test_Sch_EntryCbCount);
}

/** Single-entry cyclic table repeats the same entry without leaving RUNNING. */
void test_Schedule_Process_SingleEntryCyclic_StaysOnEntry(void)
{
    test_Sch_FillTable(&test_SchTblSingle, &test_SchEntry0, 1U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblSingle));
    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());

    LinMaster_Schedule_Process();
    LinMaster_Schedule_Process();
    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());
    TEST_ASSERT_EQUAL_UINT8(0xC1U, test_Sch_EntryCbPid);
}

void test_Schedule_PauseResume_Lifecycle(void)
{
    test_Sch_FillTable(&test_SchTblStart, &test_SchEntry0, 2U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblStart));
    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Pause());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_PAUSED, LinMaster_Schedule_GetState());
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsRunning());

    /* While paused Process must not advance. */
    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
    TEST_ASSERT_EQUAL_UINT32(1U, test_Sch_EntryCbCount);

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Resume());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());
    TEST_ASSERT_TRUE(LinMaster_Schedule_IsRunning());

    /* Pause/Resume outside their required states fail. */
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Stop());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Pause());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Resume());
}

void test_Schedule_SetEntryIndex_Guards(void)
{
    test_Sch_FillTable(&test_SchTblStart, &test_SchEntry0, 2U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblStart));

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_SetEntryIndex(2U));   /* out of range */
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SetEntryIndex(1U));
    TEST_ASSERT_EQUAL_UINT8(1U, LinMaster_Schedule_GetEntryIndex());

    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT8(1U, test_Sch_EntryCbIndex);     /* started at index 1 */
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_SetEntryIndex(0U));   /* running */

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Stop());
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SetEntryIndex(0U));
}

/** AddEntry grows the runtime table and writes the count into the table. */
void test_Schedule_AddEntry_GrowsRuntimeTable(void)
{
    test_Sch_FillTable(&test_SchTblAdd, &test_SchEntry0, 1U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblAdd));
    TEST_ASSERT_EQUAL_UINT8(1U, test_SchTblAdd.EntryCount);

    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_AddEntry(NULL));
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_AddEntry(&test_SchEntryAdded));
    /* NOTE: product mutates the caller's (const) table EntryCount. */
    TEST_ASSERT_EQUAL_UINT8(2U, test_SchTblAdd.EntryCount);

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SetEntryIndex(1U));
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT8(1U, test_Sch_EntryCbIndex);
    TEST_ASSERT_EQUAL_UINT8(0x42U, test_Sch_EntryCbPid);
}

void test_Schedule_SwitchTable_StopsOld_AutoStartsNew(void)
{
    test_Sch_FillTable(&test_SchTblSwA, &test_SchEntry0, 1U);
    test_Sch_FillTable(&test_SchTblSwB, &test_SchEntry1, 2U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblSwA));
    LinMaster_Schedule_RegisterEntryCallback(test_Sch_EntryCb);
    LinMaster_Schedule_RegisterTableSwitchCallback(test_Sch_SwitchCb);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_TRUE(LinMaster_Schedule_IsRunning());

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SwitchTable(&test_SchTblSwB, TRUE));
    TEST_ASSERT_TRUE(test_Sch_SwitchCbCalled);
    TEST_ASSERT_EQUAL_PTR(&test_SchTblSwA, test_Sch_SwitchOld);
    TEST_ASSERT_EQUAL_PTR(&test_SchTblSwB, test_Sch_SwitchNew);
    TEST_ASSERT_FALSE(test_SchTblSwA.IsRunning);
    TEST_ASSERT_EQUAL_PTR(&test_SchTblSwB, LinMaster_Schedule_GetCurrentTable());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());
    TEST_ASSERT_TRUE(test_SchTblSwB.IsRunning);
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());    /* Restart */
    TEST_ASSERT_EQUAL_UINT8(0x42U, test_Sch_EntryCbPid);                /* new entry 0 = Entry1 */

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_SwitchTable(NULL, TRUE));
}

/** Restart=FALSE keeps the new table index; an out-of-range index resets. */
void test_Schedule_SwitchTable_KeepIndex_ClampsOutOfRange(void)
{
    test_Sch_FillTable(&test_SchTblSwA, &test_SchEntry0, 1U);
    test_Sch_FillTable(&test_SchTblSwB, &test_SchEntry1, 2U);
    test_SchTblSwB.EntryIndex = 5U;
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblSwA));
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SwitchTable(&test_SchTblSwB, FALSE));
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
}

/**
 * NOTE: the schedule time source is an internal call counter. Start()
 * consumes counter calls (start + first entry execution), so the first
 * GetElapsedTime() returns 1. GetRemainingTime() internally consumes one
 * extra counter tick (it re-reads the elapsed time), hence DelayMs - 2.
 */
void test_Schedule_Timing_ElapsedAndRemaining(void)
{
    test_Sch_FillTable(&test_SchTblSlow, &test_SchEntrySlow, 1U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblSlow));
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());

    TEST_ASSERT_EQUAL_UINT32(1U, LinMaster_Schedule_GetElapsedTime());
    TEST_ASSERT_EQUAL_UINT32(3U, LinMaster_Schedule_GetRemainingTime());

    /* With a 5 ms delay the table must not advance yet. */
    LinMaster_Schedule_Process();
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_RUNNING, LinMaster_Schedule_GetState());
}

void test_Schedule_SetDelay_SetCyclic_Reset_Guards(void)
{
    test_Sch_FillTable(&test_SchTblStart, &test_SchEntry0, 2U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblStart));

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SetDelay(10U));
    LinMaster_Schedule_SetCyclic(TRUE);
    TEST_ASSERT_TRUE(LinMaster_Schedule_IsCyclic());
    LinMaster_Schedule_SetCyclic(FALSE);
    TEST_ASSERT_FALSE(LinMaster_Schedule_IsCyclic());

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Reset());     /* running */

    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Stop());
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_SetEntryIndex(1U));
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Reset());
    TEST_ASSERT_EQUAL_UINT8(0U, LinMaster_Schedule_GetEntryIndex());
}

void test_Schedule_DeInit_StopsAndResets(void)
{
    test_Sch_FillTable(&test_SchTblStart, &test_SchEntry0, 2U);
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Init(&test_SchTblStart));
    TEST_ASSERT_EQUAL_UINT32(E_OK, LinMaster_Schedule_Start());
    TEST_ASSERT_TRUE(test_SchTblStart.IsRunning);

    LinMaster_Schedule_DeInit();
    TEST_ASSERT_FALSE(test_SchTblStart.IsRunning);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_SCHEDULE_STATE_UNINIT, LinMaster_Schedule_GetState());
    TEST_ASSERT_NULL(LinMaster_Schedule_GetCurrentTable());
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, LinMaster_Schedule_Start());
}

/*===========================================================================
 * Diagnostic - BeforeInit tests
 *===========================================================================*/

void test_Diag_BeforeInit_ApiReturnsNotInitialized(void)
{
    LinMaster_Diag_RequestType req;
    LinMaster_Diag_ResponseType resp;

    (void)memset(&req, 0, sizeof(req));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_INITIALIZED, LinMaster_Diag_SendRequest(&req));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_INITIALIZED, LinMaster_Diag_GetResponse(&resp));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_INITIALIZED, LinMaster_Diag_SessionControl(0x03U));
    TEST_ASSERT_TRUE(LinMaster_Diag_IsRequestComplete());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_GetLastError());

    /* MainFunction / RegisterCallback / SetTimeout / Cancel are no-ops. */
    LinMaster_Diag_MainFunction();
    LinMaster_Diag_RegisterCallback(test_Diag_Cb);
    LinMaster_Diag_SetTimeout(10U);
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_TRUE(LinMaster_Diag_IsRequestComplete());
}

/*===========================================================================
 * Diagnostic - lifecycle and request handling
 *===========================================================================*/

void test_Diag_Init_DeInit_Guards(void)
{
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());
    LinMaster_Diag_DeInit();

    {
        LinMaster_Diag_RequestType req;
        (void)memset(&req, 0, sizeof(req));
        TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_INITIALIZED, LinMaster_Diag_SendRequest(&req));
    }
}

void test_Diag_SendRequest_ParamValidation_AndBusy(void)
{
    LinMaster_Diag_RequestType req;

    (void)LinMaster_Tp_Init();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_INVALID_PARAM, LinMaster_Diag_SendRequest(NULL));

    (void)memset(&req, 0, sizeof(req));
    req.Sid = 0x22U;
    req.Length = (LINMASTER_DIAG_MAX_BUFFER_SIZE + 1U);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_INVALID_PARAM, LinMaster_Diag_SendRequest(&req));

    req.Length = 2U;
    req.Data[0] = 0xF1U;
    req.Data[1] = 0x90U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SendRequest(&req));
    TEST_ASSERT_FALSE(LinMaster_Diag_IsRequestComplete());

    /* Module is busy until the response has been consumed. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_BUSY, LinMaster_Diag_SendRequest(&req));
}

/** TP not initialized -> Transmit fails -> diagnostic enters error state. */
void test_Diag_MainFunction_TxError_WhenTpNotInitialized(void)
{
    LinMaster_Diag_RequestType req;

    LinMaster_Tp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());
    LinMaster_Diag_RegisterCallback(test_Diag_Cb);

    (void)memset(&req, 0, sizeof(req));
    req.Sid = 0x22U;
    req.SubFunction = 0x00U;
    req.Data[0] = 0xF1U;
    req.Data[1] = 0x90U;
    req.Length = 2U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SendRequest(&req));

    LinMaster_Diag_MainFunction();
    TEST_ASSERT_TRUE(LinMaster_Diag_IsRequestComplete());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_OK, LinMaster_Diag_GetLastError());
    TEST_ASSERT_TRUE(test_Diag_CbCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_OK, test_Diag_CbStatus);
    TEST_ASSERT_FALSE(test_Diag_CbRespValid);
}

/**
 * Happy path: request (4 bytes -> TP single frame), then a response frame is
 * injected via LinMaster_Tp_RxIndication.
 * NOTE: the TP layer forwards the whole frame including the PCI byte, so the
 * diagnostic layer sees the PCI byte as the first UDS byte. A frame starting
 * with 0x22 maps to PCI type CF and reaches the diagnostic layer intact;
 * a raw UDS negative response (0x7F...) would be dropped as invalid PCI and
 * can never reach ProcessResponse through the public API (product gap).
 */
void test_Diag_HappyPath_PositiveResponse(void)
{
    LinMaster_Diag_RequestType req;
    LinMaster_Diag_ResponseType resp;
    uint8 respFrame[3] = {0x22U, 0xF1U, 0x90U};

    (void)LinMaster_Tp_Init();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());
    LinMaster_Diag_RegisterCallback(test_Diag_Cb);

    (void)memset(&req, 0, sizeof(req));
    req.Sid = 0x22U;
    req.SubFunction = 0x00U;
    req.Data[0] = 0xF1U;
    req.Data[1] = 0x90U;
    req.Length = 2U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SendRequest(&req));

    /* MainFunction #1: TX_REQUEST -> TP single frame -> WAIT_RESPONSE. */
    LinMaster_Diag_MainFunction();
    TEST_ASSERT_FALSE(LinMaster_Diag_IsRequestComplete());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_GetState());

    /* MainFunction #2: still waiting. */
    LinMaster_Diag_MainFunction();
    TEST_ASSERT_FALSE(test_Diag_CbCalled);

    /* Inject response (PCI nibble 0x2 = CF -> forwarded to the diag layer). */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(respFrame, 3U));

    /* MainFunction #3: WAIT_RESPONSE -> RX_RESPONSE. */
    LinMaster_Diag_MainFunction();
    /* MainFunction #4: process response -> COMPLETE. */
    LinMaster_Diag_MainFunction();
    TEST_ASSERT_TRUE(LinMaster_Diag_IsRequestComplete());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_GetLastError());
    TEST_ASSERT_TRUE(test_Diag_CbCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, test_Diag_CbStatus);
    TEST_ASSERT_TRUE(test_Diag_CbRespValid);
    TEST_ASSERT_EQUAL_UINT8(0x22U, test_Diag_CbRespSid);

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_GetResponse(&resp));
    TEST_ASSERT_EQUAL_UINT8(0x22U, resp.ResponseSid);
    TEST_ASSERT_EQUAL_UINT16(2U, resp.Length);
    TEST_ASSERT_EQUAL_UINT8(0xF1U, resp.Data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x90U, resp.Data[1]);
    TEST_ASSERT_EQUAL_UINT32(FALSE, resp.IsNegative);

    /* Response consumed -> second GetResponse reports busy. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_BUSY, LinMaster_Diag_GetResponse(&resp));
}

/** SetTimeout(2) makes the WAIT_RESPONSE state time out on the 3rd call. */
void test_Diag_ResponseTimeout_ReportsErrorCallback(void)
{
    LinMaster_Diag_RequestType req;

    (void)LinMaster_Tp_Init();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());
    LinMaster_Diag_RegisterCallback(test_Diag_Cb);
    LinMaster_Diag_SetTimeout(2U);

    (void)memset(&req, 0, sizeof(req));
    req.Sid = 0x3EU;
    req.SubFunction = 0x00U;
    req.Length = 0U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SendRequest(&req));

    LinMaster_Diag_MainFunction();      /* TX_REQUEST -> WAIT_RESPONSE */
    LinMaster_Diag_MainFunction();      /* timer 1 */
    LinMaster_Diag_MainFunction();      /* timer 2 -> timeout */
    TEST_ASSERT_TRUE(LinMaster_Diag_IsRequestComplete());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_TIMEOUT, LinMaster_Diag_GetLastError());
    TEST_ASSERT_TRUE(test_Diag_CbCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_TIMEOUT, test_Diag_CbStatus);
    TEST_ASSERT_FALSE(test_Diag_CbRespValid);
}

void test_Diag_CancelRequest_ResetsState(void)
{
    LinMaster_Diag_RequestType req;

    (void)LinMaster_Tp_Init();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());

    (void)memset(&req, 0, sizeof(req));
    req.Sid = 0x10U;
    req.SubFunction = 0x03U;
    req.Length = 0U;
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SendRequest(&req));

    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_TRUE(LinMaster_Diag_IsRequestComplete());
    /* NOTE: product sets LastError = E_NOT_OK on cancel. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_NOT_OK, LinMaster_Diag_GetLastError());

    /* After cancel a new request is accepted again. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SendRequest(&req));
}

/** All UDS service wrappers build a request and report OK (idle module). */
void test_Diag_ServiceWrappers_ReturnOk(void)
{
    uint8 payload[4] = {0x11U, 0x22U, 0x33U, 0x44U};

    (void)LinMaster_Tp_Init();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_Init());

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SessionControl(0x03U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_EcuReset(0x01U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_ReadDataById(0xF190U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_WriteDataById(0xF190U, payload, 2U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SecurityAccess(0x01U, NULL, 0U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_SecurityAccess(0x02U, payload, 4U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_RoutineControl(0x01U, 0x0200U, payload, 2U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_RoutineControl(0x01U, 0x0200U, NULL, 0U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_ClearDiagnosticInformation(0x00FFFFFFUL));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_ReadDTCInformation(0x01U, 0x08U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_TesterPresent(0x80U));
    LinMaster_Diag_CancelRequest();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_OK, LinMaster_Diag_CommunicationControl(0x00U, 0x03U));
    LinMaster_Diag_CancelRequest();

    /* Invalid wrapper parameters. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_INVALID_PARAM, LinMaster_Diag_WriteDataById(0xF190U, NULL, 2U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_DIAG_E_INVALID_PARAM, LinMaster_Diag_WriteDataById(0xF190U, payload, 0U));
    /* NOTE: RoutineControl's oversize check (Length > 4093) is dead code -
     * its Length parameter is uint8, so the threshold can never be reached
     * (compiler warns tautological-compare).  Not triggerable via the API. */
}

/*===========================================================================
 * TP - BeforeInit tests
 *===========================================================================*/

void test_Tp_BeforeInit_ApiGuarded(void)
{
    uint8 data[1] = {0x01U};
    uint8 frame[1] = {0x00U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_NOT_OK, LinMaster_Tp_Transmit(1U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_NOT_OK, LinMaster_Tp_RxIndication(frame, 1U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
    TEST_ASSERT_FALSE(LinMaster_Tp_IsBusy());

    LinMaster_Tp_MainFunction();    /* no-op */
    LinMaster_Tp_Cancel();          /* no-op */
    LinMaster_Tp_DeInit();          /* no-op */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
}

/*===========================================================================
 * TP - lifecycle
 *===========================================================================*/

void test_Tp_Init_DeInit_VersionInfo(void)
{
    Std_VersionInfoType info;
    uint8 data[1] = {0x01U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());

    (void)memset(&info, 0, sizeof(info));
    LinMaster_Tp_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(0U, info.vendorID);
    TEST_ASSERT_EQUAL_UINT32(0U, info.moduleID);
    TEST_ASSERT_EQUAL_UINT32(1U, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT32(0U, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT32(0U, info.sw_patch_version);
    LinMaster_Tp_GetVersionInfo(NULL);

    LinMaster_Tp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_NOT_OK, LinMaster_Tp_Transmit(1U, data));
}

/*===========================================================================
 * TP - transmit
 *===========================================================================*/

/** Length <= 6 is sent as a single frame (PCI = 0x00 | length). */
void test_Tp_Transmit_SingleFrame_SendsPciAndData(void)
{
    uint8 data[3] = {0xAAU, 0xBBU, 0xCCU};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(3U, data));
    TEST_ASSERT_EQUAL_UINT8(4U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x03U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAAU, test_SlaveTxBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(0xBBU, test_SlaveTxBuf[2]);
    TEST_ASSERT_EQUAL_UINT8(0xCCU, test_SlaveTxBuf[3]);
    /* SF completes synchronously and fires the confirm. */
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, test_Tp_ConfirmStatus);
    TEST_ASSERT_FALSE(LinMaster_Tp_IsBusy());
}

void test_Tp_Transmit_ParamErrors(void)
{
    uint8 data[1] = {0x01U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PARAM, LinMaster_Tp_Transmit(1U, NULL));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PARAM, LinMaster_Tp_Transmit(0U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_OVERSIZE, LinMaster_Tp_Transmit((uint16)(LINMASTER_TP_MAX_FRAME_LEN + 1U), data));
}

/**
 * Length > 6 is sent as a first frame (PCI 0x10 | len hi, len lo + 5 data
 * bytes) and the channel waits for a flow control frame.
 */
void test_Tp_Transmit_FirstFrame_WaitsFlowControl(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));
    TEST_ASSERT_EQUAL_UINT8(7U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x10U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x08U, test_SlaveTxBuf[1]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data[0], &test_SlaveTxBuf[2], 5U);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());
    TEST_ASSERT_TRUE(LinMaster_Tp_IsBusy());

    /* Busy while waiting for flow control. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_BUSY, LinMaster_Tp_Transmit(1U, data));
}

/*===========================================================================
 * TP - flow control / receive
 *===========================================================================*/

/** FC CTS with STmin=0: the consecutive frame is sent on the next MainFunction. */
void test_Tp_FlowControl_Cts_CompletesConsecutiveFrame(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8 fc[3] = {0x30U, 0x00U, 0x00U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(fc, 3U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_TX_CF, LinMaster_Tp_GetState());

    LinMaster_Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(4U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x21U, test_SlaveTxBuf[0]);     /* CF, seq 1 */
    TEST_ASSERT_EQUAL_UINT8(6U, test_SlaveTxBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(7U, test_SlaveTxBuf[2]);
    TEST_ASSERT_EQUAL_UINT8(8U, test_SlaveTxBuf[3]);
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, test_Tp_ConfirmStatus);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
}

/** FC WT keeps waiting; FC OVFLW aborts; short/invalid FC is rejected. */
void test_Tp_FlowControl_Wait_Overflow_AndInvalid(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8 fcWt[3] = {0x31U, 0x00U, 0x00U};
    uint8 fcOv[3] = {0x32U, 0x00U, 0x00U};
    uint8 fcShort[2] = {0x30U, 0x00U};
    uint8 fcBad[3] = {0x33U, 0x00U, 0x00U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(fcWt, 3U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_OVERSIZE, LinMaster_Tp_RxIndication(fcOv, 3U));
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_OVERSIZE, test_Tp_ConfirmStatus);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());

    /* Invalid flow control frames do not disturb the channel state. */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PCI, LinMaster_Tp_RxIndication(fcShort, 2U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PCI, LinMaster_Tp_RxIndication(fcBad, 3U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());
}

/** SF/FF/CF frames are forwarded to the RxCallback; PCI 0x40 is rejected. */
void test_Tp_RxIndication_DataFrameForwarded_AndInvalidPci(void)
{
    uint8 sf[3] = {0x02U, 0x50U, 0x03U};
    uint8 bad[2] = {0x40U, 0x00U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterRxCallback(test_Tp_RxCb);

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(sf, 3U));
    TEST_ASSERT_TRUE(test_Tp_RxCbCalled);
    TEST_ASSERT_EQUAL_UINT16(3U, test_Tp_RxCbLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(sf, test_Tp_RxCbData, 3U);

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PCI, LinMaster_Tp_RxIndication(bad, 2U));

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PARAM, LinMaster_Tp_RxIndication(NULL, 2U));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_INVALID_PARAM, LinMaster_Tp_RxIndication(sf, 0U));
}

/** N_BS timeout in WAIT_FC: confirm E_TIMEOUT and reset to idle. */
void test_Tp_WaitFc_Timeout_ConfirmFails(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());

    test_SlaveTimeMs += 2000U;      /* jump beyond LINMASTER_TP_N_BS_TIMEOUT */
    LinMaster_Tp_MainFunction();
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_TIMEOUT, test_Tp_ConfirmStatus);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
}

/** 20 bytes: FF(5) + CF seq1(6) + CF seq2(6) + CF seq3(3) -> confirm. */
void test_Tp_MultiConsecutiveFrames_SequenceNumbers(void)
{
    uint8 data[20];
    uint8 fc[3] = {0x30U, 0x00U, 0x00U};
    uint8 i;

    for (i = 0U; i < 20U; i++) {
        data[i] = (uint8)(i + 1U);
    }

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(20U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(fc, 3U));

    LinMaster_Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(7U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x21U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(6U, test_SlaveTxBuf[1]);

    LinMaster_Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(7U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x22U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(12U, test_SlaveTxBuf[1]);

    LinMaster_Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(4U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x23U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(18U, test_SlaveTxBuf[1]);
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
}

/** BlockSize=1 pauses the transfer after one CF until a new FC arrives. */
void test_Tp_BlockSize_PausesForNewFlowControl(void)
{
    uint8 data[14];
    uint8 fc[3] = {0x30U, 0x01U, 0x00U};    /* BS=1, STmin=0 */
    uint8 i;

    for (i = 0U; i < 14U; i++) {
        data[i] = (uint8)(i + 1U);
    }

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(14U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(fc, 3U));

    LinMaster_Tp_MainFunction();    /* CF seq 1 (6 bytes) -> BS exhausted */
    TEST_ASSERT_EQUAL_UINT8(0x21U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());

    LinMaster_Tp_MainFunction();    /* still waiting */
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(fc, 3U));
    LinMaster_Tp_MainFunction();    /* final CF seq 2 (3 bytes) */
    TEST_ASSERT_EQUAL_UINT8(0x22U, test_SlaveTxBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(4U, test_SlaveTxLen);
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
}

/** STmin=5 paces the consecutive frame: sent on the 5th MainFunction. */
void test_Tp_Stmin_PacesConsecutiveFrames(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8 fc[3] = {0x30U, 0x00U, 0x05U};
    uint8 i;

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_RxIndication(fc, 3U));
    TEST_ASSERT_EQUAL_UINT8(7U, test_SlaveTxLen);   /* FF still last sent frame */

    for (i = 0U; i < 4U; i++) {
        LinMaster_Tp_MainFunction();
        TEST_ASSERT_EQUAL_UINT8(7U, test_SlaveTxLen);   /* nothing sent yet */
    }

    LinMaster_Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(4U, test_SlaveTxLen);
    TEST_ASSERT_EQUAL_UINT8(0x21U, test_SlaveTxBuf[0]);
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());
}

/** Cancel during WAIT_FC confirms E_NOT_OK and resets; idle cancel is no-op. */
void test_Tp_Cancel_ConfirmsNotOk_AndResets(void)
{
    uint8 data[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};

    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Init());
    LinMaster_Tp_RegisterTxConfirmCallback(test_Tp_ConfirmCb);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_OK, LinMaster_Tp_Transmit(8U, data));
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_WAIT_FC, LinMaster_Tp_GetState());

    LinMaster_Tp_Cancel();
    TEST_ASSERT_TRUE(test_Tp_ConfirmCalled);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_E_NOT_OK, test_Tp_ConfirmStatus);
    TEST_ASSERT_EQUAL_UINT32(LINMASTER_TP_STATE_IDLE, LinMaster_Tp_GetState());

    /* Cancel while idle must not fire another confirm. */
    LinMaster_Tp_Cancel();
    TEST_ASSERT_EQUAL_UINT32(1U, test_Tp_ConfirmCount);
}

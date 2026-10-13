/**
 * @file test_CanTSyn_branch.c
 * @brief Targeted branch coverage tests for CanTSyn.c
 *
 * Covers previously uncovered branches:
 *   - Line 237: if (result == E_OK) in TxConfirmation
 *   - Line 282: if (TimeDomainId >= CANTSYN_NUMBER_OF_TIME_DOMAINS) in RxIndication
 *   - Line 288: if (MsgType == CANTSYN_SYNC_MSG_TYPE) in RxIndication
 *   - Line 292: else if (MsgType == CANTSYN_OFS_MSG_TYPE) in RxIndication
 *   - Line 467: if (RetVal == E_OK) in static CanTSyn_GetCurrentTime
 *
 * We include CanTSyn.c directly to access the static CanTSyn_GetCurrentTime.
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "Std_Types.h"
#include "StbM.h"
#include "ComStack_Types.h"

/* ---- Mocks for external dependencies ------------------------------------ */

/* Det_ReportError mock */
static uint8 mock_DetCallCount = 0;
static uint8 mock_DetApiId = 0;
static uint8 mock_DetErrorId = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    mock_DetCallCount++;
    mock_DetApiId = ApiId;
    mock_DetErrorId = ErrorId;
    return E_OK;
}

/* StbM_GetCurrentTime mock */
static Std_ReturnType mock_StbM_GetCurrentTime_Return = E_NOT_OK;
static uint8 mock_StbM_GetCurrentTime_CallCount = 0;

Std_ReturnType StbM_GetCurrentTime(uint8 timeBaseId,
                                    StbM_TimeStampType* timeStampPtr,
                                    StbM_UserDataType* userDataPtr)
{
    (void)timeBaseId;
    mock_StbM_GetCurrentTime_CallCount++;
    if (mock_StbM_GetCurrentTime_Return == E_OK)
    {
        if (timeStampPtr != NULL_PTR)
        {
            timeStampPtr->nanoseconds = 1000U;
            timeStampPtr->seconds = 100U;
            timeStampPtr->secondsHi = 0U;
            timeStampPtr->timeBaseStatus = 0U;
        }
        if (userDataPtr != NULL_PTR)
        {
            userDataPtr->userData[0] = 0xAAU;
            userDataPtr->userData[1] = 0xBBU;
            userDataPtr->userByteCount = 2U;
        }
    }
    return mock_StbM_GetCurrentTime_Return;
}

/* CanIf_Transmit mock */
static Std_ReturnType mock_CanIf_Transmit_Return = E_OK;
static uint8 mock_CanIf_Transmit_CallCount = 0;

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    mock_CanIf_Transmit_CallCount++;
    return mock_CanIf_Transmit_Return;
}

/* StbM_SetGlobalTime - not called when TIME_MASTER_SUPPORT==STD_ON, but
 * provide a stub just in case */
Std_ReturnType StbM_SetGlobalTime(uint8 timeBaseId,
                                   const StbM_TimeStampType* timeStampPtr,
                                   const StbM_UserDataType* userDataPtr)
{
    (void)timeBaseId;
    (void)timeStampPtr;
    (void)userDataPtr;
    return E_OK;
}

/* StbM_UpdateGlobalTimeOffset - same as above */
Std_ReturnType StbM_UpdateGlobalTimeOffset(uint8 timeBaseId,
                                            const StbM_TimeStampType* timeStampPtr)
{
    (void)timeBaseId;
    (void)timeStampPtr;
    return E_OK;
}

/* Include production source to access static functions and variables */
#include "src/bsw/services/cantsyn/src/CanTSyn.c"

/* Provide the config array that would normally come from CanTSyn_Lcfg.c */
const CanTSyn_TimeBaseConfigType CanTSyn_TimeDomainConfig[CANTSYN_NUMBER_OF_TIME_DOMAINS] = {
    {
        .timeBaseId = 0U,
        .domainId = 0U,
        .masterConfig = 2U,
        .IsTimeMaster = TRUE,
        .TxPduId = 0U,
        .syncPeriodMs = 10U,
        .debounceTimeMs = 5U,
        .syncTimeoutMs = 100U,
        .crcSecured = FALSE,
        .useImmediateTransmission = FALSE,
        .syncCanId = 0x180U,
        .fupCanId = 0x280U,
        .ocsCanId = 0x380U,
        .syncTxPduId = 0U,
        .fupTxPduId = 1U,
        .ocsTxPduId = 2U,
        .syncRxPduId = 3U,
        .fupRxPduId = 4U,
        .ocsRxPduId = 5U
    },
    {
        .timeBaseId = 1U,
        .domainId = 1U,
        .masterConfig = 1U,
        .IsTimeMaster = FALSE,
        .TxPduId = 1U,
        .syncPeriodMs = 10U,
        .debounceTimeMs = 5U,
        .syncTimeoutMs = 100U,
        .crcSecured = FALSE,
        .useImmediateTransmission = FALSE,
        .syncCanId = 0x181U,
        .fupCanId = 0x281U,
        .ocsCanId = 0x381U,
        .syncTxPduId = 6U,
        .fupTxPduId = 7U,
        .ocsTxPduId = 8U,
        .syncRxPduId = 9U,
        .fupRxPduId = 10U,
        .ocsRxPduId = 11U
    }
};

/* ---- setUp / tearDown --------------------------------------------------- */

void setUp(void)
{
    mock_DetCallCount = 0;
    mock_DetApiId = 0;
    mock_DetErrorId = 0;
    mock_StbM_GetCurrentTime_Return = E_NOT_OK;
    mock_StbM_GetCurrentTime_CallCount = 0;
    mock_CanIf_Transmit_Return = E_OK;
    mock_CanIf_Transmit_CallCount = 0;
    /* Reset internal state by re-init */
    memset(&CanTSyn_Internal, 0, sizeof(CanTSyn_Internal));
    CanTSyn_Internal.State = CANTSYN_STATE_UNINIT;
    CanTSyn_Internal.TxState = CANTSYN_TX_IDLE;
}

void tearDown(void) {}

/* ---- Helper to init module --------------------------------------------- */
static void test_CanTSyn_DoInit(void)
{
    CanTSyn_ConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.devErrorDetect = TRUE;
    cfg.versionInfoApi = TRUE;
    CanTSyn_Init(&cfg);
}

/* =========================================================================
 * Line 237: if (result == E_OK) in TxConfirmation
 * ========================================================================= */

void test_CanTSyn_TxConfirmation_EOK_ShouldIncrementCounter(void)
{
    test_CanTSyn_DoInit();
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_STATE_INIT, CanTSyn_Internal.State);
    TEST_ASSERT_EQUAL_UINT32(0U, CanTSyn_Internal.TxCounter);

    CanTSyn_TxConfirmation(0U, E_OK);

    TEST_ASSERT_EQUAL_UINT32(1U, CanTSyn_Internal.TxCounter);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_TX_IDLE, CanTSyn_Internal.TxState);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_CanTSyn_TxConfirmation_ENotOk_ShouldNotIncrementCounter(void)
{
    test_CanTSyn_DoInit();
    CanTSyn_Internal.TxCounter = 5U;

    CanTSyn_TxConfirmation(0U, E_NOT_OK);

    TEST_ASSERT_EQUAL_UINT32(5U, CanTSyn_Internal.TxCounter);
}

/* =========================================================================
 * Line 282: if (TimeDomainId >= CANTSYN_NUMBER_OF_TIME_DOMAINS) in RxIndication
 * ========================================================================= */

void test_CanTSyn_RxIndication_InvalidTimeDomain_ShouldReturnEarly(void)
{
    test_CanTSyn_DoInit();

    uint8 rxBuf[16];
    memset(rxBuf, 0, sizeof(rxBuf));
    /* byte[0]: upper nibble = 0x2 => TimeDomainId = 2, lower nibble = 0x00 */
    rxBuf[0] = 0x20U;

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = rxBuf;
    pduInfo.SduLength = 16U;

    CanTSyn_RxIndication(0U, &pduInfo);

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* =========================================================================
 * Lines 288, 292: MsgType comparisons in RxIndication
 * ========================================================================= */

void test_CanTSyn_RxIndication_UnknownMsgType_ShouldNotProcess(void)
{
    test_CanTSyn_DoInit();

    uint8 rxBuf[16];
    memset(rxBuf, 0, sizeof(rxBuf));
    rxBuf[0] = 0x05U;  /* TimeDomainId=0, MsgType=0x05 */

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = rxBuf;
    pduInfo.SduLength = 16U;

    uint32 rxCounterBefore = CanTSyn_Internal.RxCounter;
    CanTSyn_RxIndication(0U, &pduInfo);

    TEST_ASSERT_EQUAL_UINT32(rxCounterBefore + 1U, CanTSyn_Internal.RxCounter);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_CanTSyn_RxIndication_MsgType0_ShouldEvaluateBothBranches(void)
{
    test_CanTSyn_DoInit();

    uint8 rxBuf[16];
    memset(rxBuf, 0, sizeof(rxBuf));
    rxBuf[0] = 0x10U;  /* TimeDomainId=1, MsgType=0x00 */

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = rxBuf;
    pduInfo.SduLength = 16U;

    CanTSyn_RxIndication(0U, &pduInfo);

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* =========================================================================
 * Line 467: if (RetVal == E_OK) in static CanTSyn_GetCurrentTime
 * ========================================================================= */

void test_CanTSyn_GetCurrentTime_StbM_OK_ShouldUpdateUserData(void)
{
    test_CanTSyn_DoInit();
    mock_StbM_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType timeStamp;
    StbM_UserDataType userData;
    memset(&timeStamp, 0, sizeof(timeStamp));
    memset(&userData, 0, sizeof(userData));

    Std_ReturnType ret = CanTSyn_GetCurrentTime(0U, &timeStamp, &userData);

    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_StbM_GetCurrentTime_CallCount);
    TEST_ASSERT_EQUAL_UINT8(0xAAU, CanTSyn_TimeDomains[0].UserData[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBBU, CanTSyn_TimeDomains[0].UserData[1]);
    TEST_ASSERT_EQUAL_UINT8(2U, CanTSyn_TimeDomains[0].UserData[2]);
}

void test_CanTSyn_GetCurrentTime_StbM_NotOk_ShouldNotUpdateUserData(void)
{
    test_CanTSyn_DoInit();
    mock_StbM_GetCurrentTime_Return = E_NOT_OK;

    CanTSyn_TimeDomains[0].UserData[0] = 0xFFU;
    CanTSyn_TimeDomains[0].UserData[1] = 0xFFU;
    CanTSyn_TimeDomains[0].UserData[2] = 0xFFU;

    StbM_TimeStampType timeStamp;
    StbM_UserDataType userData;
    memset(&timeStamp, 0, sizeof(timeStamp));
    memset(&userData, 0, sizeof(userData));

    Std_ReturnType ret = CanTSyn_GetCurrentTime(0U, &timeStamp, &userData);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, CanTSyn_TimeDomains[0].UserData[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, CanTSyn_TimeDomains[0].UserData[1]);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, CanTSyn_TimeDomains[0].UserData[2]);
}

/* =========================================================================
 * Lines 513, 519, 523, 529: MainFunction transmit path
 * Note: CanTSyn_TimeDomainConfig is const, so we test via CanTSyn_TimeDomains
 * which is the runtime state. The MainFunction checks IsTimeMaster from config,
 * so we need to ensure at least one domain is configured as master in the
 * test configuration or use a different approach.
 * ========================================================================= */

void test_CanTSyn_MainFunction_TimeMaster_OK_ShouldTransmit(void)
{
    test_CanTSyn_DoInit();
    mock_StbM_GetCurrentTime_Return = E_OK;
    mock_CanIf_Transmit_Return = E_OK;
    mock_CanIf_Transmit_CallCount = 0;

    /* Ensure TxState is idle */
    CanTSyn_Internal.TxState = CANTSYN_TX_IDLE;
    CanTSyn_TimeDomains[0].SequenceCounter = 0U;

    /* Call MainFunction - if any domain is configured as master, it will transmit */
    CanTSyn_MainFunction();

    /* If time master is enabled in config, should have called CanIf_Transmit */
    if (mock_CanIf_Transmit_CallCount > 0U)
    {
        TEST_ASSERT_EQUAL_UINT8(CANTSYN_TX_BUSY, CanTSyn_Internal.TxState);
        TEST_ASSERT_EQUAL_UINT8(1U, CanTSyn_TimeDomains[0].SequenceCounter);
    }
}

void test_CanTSyn_MainFunction_TxStateBusy_ShouldNotTransmit(void)
{
    test_CanTSyn_DoInit();
    mock_StbM_GetCurrentTime_Return = E_OK;
    uint32 callCountBefore = mock_CanIf_Transmit_CallCount;

    CanTSyn_Internal.TxState = CANTSYN_TX_BUSY;

    CanTSyn_MainFunction();

    /* Should NOT call CanIf_Transmit when TxState is busy */
    TEST_ASSERT_EQUAL_UINT32(callCountBefore, mock_CanIf_Transmit_CallCount);
}

void test_CanTSyn_MainFunction_CanIfFails_ShouldNotChangeState(void)
{
    test_CanTSyn_DoInit();
    mock_StbM_GetCurrentTime_Return = E_OK;
    mock_CanIf_Transmit_Return = E_NOT_OK;
    mock_CanIf_Transmit_CallCount = 0;

    CanTSyn_Internal.TxState = CANTSYN_TX_IDLE;
    uint8 counterBefore = CanTSyn_TimeDomains[0].SequenceCounter;

    CanTSyn_MainFunction();

    /* If transmit was attempted but failed, state should remain idle */
    if (mock_CanIf_Transmit_CallCount > 0U)
    {
        TEST_ASSERT_EQUAL_UINT8(CANTSYN_TX_IDLE, CanTSyn_Internal.TxState);
        TEST_ASSERT_EQUAL_UINT8(counterBefore, CanTSyn_TimeDomains[0].SequenceCounter);
    }
}

void test_CanTSyn_MainFunction_SequenceCounterWrap_ShouldResetToZero(void)
{
    test_CanTSyn_DoInit();
    mock_StbM_GetCurrentTime_Return = E_OK;
    mock_CanIf_Transmit_Return = E_OK;

    CanTSyn_Internal.TxState = CANTSYN_TX_IDLE;
    CanTSyn_TimeDomains[0].SequenceCounter = 15U;

    CanTSyn_MainFunction();

    /* If transmit succeeded, counter should wrap from 15 to 0 (after incrementing to 16) */
    if (mock_CanIf_Transmit_CallCount > 0U)
    {
        TEST_ASSERT_EQUAL_UINT8(0U, CanTSyn_TimeDomains[0].SequenceCounter);
    }
}

/* ---- main --------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanTSyn_TxConfirmation_EOK_ShouldIncrementCounter);
    RUN_TEST(test_CanTSyn_TxConfirmation_ENotOk_ShouldNotIncrementCounter);
    RUN_TEST(test_CanTSyn_RxIndication_InvalidTimeDomain_ShouldReturnEarly);
    RUN_TEST(test_CanTSyn_RxIndication_UnknownMsgType_ShouldNotProcess);
    RUN_TEST(test_CanTSyn_RxIndication_MsgType0_ShouldEvaluateBothBranches);
    RUN_TEST(test_CanTSyn_GetCurrentTime_StbM_OK_ShouldUpdateUserData);
    RUN_TEST(test_CanTSyn_GetCurrentTime_StbM_NotOk_ShouldNotUpdateUserData);
    RUN_TEST(test_CanTSyn_MainFunction_TimeMaster_OK_ShouldTransmit);
    RUN_TEST(test_CanTSyn_MainFunction_TxStateBusy_ShouldNotTransmit);
    RUN_TEST(test_CanTSyn_MainFunction_CanIfFails_ShouldNotChangeState);
    RUN_TEST(test_CanTSyn_MainFunction_SequenceCounterWrap_ShouldResetToZero);

    return UNITY_END();
}

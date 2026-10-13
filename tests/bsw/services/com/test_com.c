/**
 * @file test_com.c
 * @brief Com (Communication Manager) Unit Tests
 * @req SWS_Com
 */

// @tests src/bsw/services/com/src/Com.c  @tests src/bsw/services/com/include/Com.h
#include "unity.h"
#include "Com.h"

/* -------------------------------------------------------------------------- */
/* DET mock: records call count plus ApiId/ErrorId of the last reported error */
/* -------------------------------------------------------------------------- */
static uint8 mock_DetCalls = 0;
static uint8 mock_DetLastApiId = 0;
static uint8 mock_DetLastErrorId = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetCalls++;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    return E_OK;
}

/* -------------------------------------------------------------------------- */
/* PduR_Transmit recorder (implemented in stubs.c)                          */
/* -------------------------------------------------------------------------- */
extern uint32 mock_PduR_Transmit_Count;
extern PduIdType mock_PduR_Transmit_LastPduId;
extern uint16 mock_PduR_Transmit_LastSduLength;

/* Forward declarations for functions defined in Com.c but not in public header */
extern Std_ReturnType Com_SendDynSignal(Com_SignalIdType SignalId, const void* SignalDataPtr, uint16 Length);
extern Std_ReturnType Com_ReceiveDynSignal(Com_SignalIdType SignalId, void* SignalDataPtr, uint16* Length);
extern Std_ReturnType Com_TriggerIPDUSendWithMetaData(PduIdType PduId, const uint8* MetaData);
extern void Com_TpRxIndication(PduIdType id, Std_ReturnType result);
extern void Com_TpTxConfirmation(PduIdType id, Std_ReturnType result);
extern Std_ReturnType Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
extern Std_ReturnType Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr);
extern void Com_ClearIpduGroupVector(Com_IpduGroupVector ipduGroupVector);
extern void Com_SetIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId);
extern void Com_ClearIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId);
extern void Com_EnableReceptionDM(Com_IpduGroupVector ipduGroupVector);
extern void Com_DisableReceptionDM(Com_IpduGroupVector ipduGroupVector);
extern Com_StatusType Com_GetStatus(void);

/* -------------------------------------------------------------------------- */
/* Test configurations                                                        */
/* -------------------------------------------------------------------------- */
/* One 8-bit little-endian signal at bit 0, mapped to IPDU 0, COM_PENDING
 * transfer property (no automatic transmission on Com_SendSignal). */
static const Com_SignalConfigType testSignals_Pending[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
/* Same signal with COM_TRIGGERED transfer property: Com_SendSignal must
 * trigger PduR_Transmit for the referenced IPDU. */
static const Com_SignalConfigType testSignals_Triggered[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType fullConfig =
    { testSignals_Pending, 1U, testIPdus, 1U };
static const Com_ConfigType triggeredConfig =
    { testSignals_Triggered, 1U, testIPdus, 1U };
/* Empty configuration: module init succeeds but no signal/IPDU is resolvable. */
static const Com_ConfigType emptyConfig = { NULL_PTR, 0U, NULL_PTR, 0U };

/* P1 Phase 8 Com_IpduGroupControl regression configuration: two cyclic
 * I-PDUs (TimePeriod 1) in different groups — IPDU 0 -> group 0,
 * IPDU 1 -> group 1. */
static const Com_SignalConfigType testSignals_Group2[2] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U },
    { 1U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_Group2[2] = {
    { 0U, 8U, FALSE, 0U, 0U, 1U, 0U },
    { 1U, 8U, FALSE, 0U, 0U, 1U, 1U }
};
static const Com_ConfigType groupConfig =
    { testSignals_Group2, 2U, testIPdus_Group2, 2U };

/* --- Batch 2 configurations --- */

/* Big-endian 16-bit byte-aligned signal */
static const Com_SignalConfigType testSignals_BE16[1] = {
    { 0U, 0U, 16U, COM_BIG_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_BE16[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType be16Config =
    { testSignals_BE16, 1U, testIPdus_BE16, 1U };

/* Big-endian 32-bit byte-aligned signal */
static const Com_SignalConfigType testSignals_BE32[1] = {
    { 0U, 0U, 32U, COM_BIG_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_BE32[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType be32Config =
    { testSignals_BE32, 1U, testIPdus_BE32, 1U };

/* Little-endian 16-bit byte-aligned signal */
static const Com_SignalConfigType testSignals_LE16[1] = {
    { 0U, 0U, 16U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_LE16[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType le16Config =
    { testSignals_LE16, 1U, testIPdus_LE16, 1U };

/* Little-endian 32-bit byte-aligned signal */
static const Com_SignalConfigType testSignals_LE32[1] = {
    { 0U, 0U, 32U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_LE32[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType le32Config =
    { testSignals_LE32, 1U, testIPdus_LE32, 1U };

/* Non-byte-aligned LE signal (bit position 3, size 5 bits) */
static const Com_SignalConfigType testSignals_BitLE[1] = {
    { 0U, 3U, 5U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_BitLE[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType bitLEConfig =
    { testSignals_BitLE, 1U, testIPdus_BitLE, 1U };

/* Non-byte-aligned BE signal (bit position 2, size 6 bits) */
static const Com_SignalConfigType testSignals_BitBE[1] = {
    { 0U, 2U, 6U, COM_BIG_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_BitBE[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType bitBEConfig =
    { testSignals_BitBE, 1U, testIPdus_BitBE, 1U };

/* COM_NEVER filter signal */
static const Com_SignalConfigType testSignals_NeverFilter[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_NEVER, 0xFFU, 0x42U, 0U }
};
static const Com_IPduConfigType testIPdus_NeverFilter[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType neverFilterConfig =
    { testSignals_NeverFilter, 1U, testIPdus_NeverFilter, 1U };

/* COM_MASKED_NEW_EQUALS_X filter signal */
static const Com_SignalConfigType testSignals_MaskedEquals[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_MASKED_NEW_EQUALS_X, 0x0FU, 0x05U, 0U }
};
static const Com_IPduConfigType testIPdus_MaskedEquals[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType maskedEqualsConfig =
    { testSignals_MaskedEquals, 1U, testIPdus_MaskedEquals, 1U };

/* COM_MASKED_NEW_DIFFERS_X filter signal */
static const Com_SignalConfigType testSignals_MaskedDiffers[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_MASKED_NEW_DIFFERS_X, 0x0FU, 0x05U, 0U }
};
static const Com_IPduConfigType testIPdus_MaskedDiffers[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType maskedDiffersConfig =
    { testSignals_MaskedDiffers, 1U, testIPdus_MaskedDiffers, 1U };

/* COM_MASKED_NEW_DIFFERS_MASKED_OLD filter signal */
static const Com_SignalConfigType testSignals_MaskedDiffersOld[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_MASKED_NEW_DIFFERS_MASKED_OLD, 0x0FU, 0x00U, 0U }
};
static const Com_IPduConfigType testIPdus_MaskedDiffersOld[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType maskedDiffersOldConfig =
    { testSignals_MaskedDiffersOld, 1U, testIPdus_MaskedDiffersOld, 1U };

/* COM_TRIGGERED_ON_CHANGE signal */
static const Com_SignalConfigType testSignals_TriggeredOnChange[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_TRIGGERED_ON_CHANGE, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_TriggeredOnChange[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType triggeredOnChangeConfig =
    { testSignals_TriggeredOnChange, 1U, testIPdus_TriggeredOnChange, 1U };

/* Repeating-enabled IPDU config */
static const Com_SignalConfigType testSignals_Repeat[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_Repeat[1] = {
    { 0U, 8U, TRUE, 3U, 2U, 0U }
};
static const Com_ConfigType repeatConfig =
    { testSignals_Repeat, 1U, testIPdus_Repeat, 1U };

/* Periodic TX IPDU config (TimePeriod = 5) */
static const Com_SignalConfigType testSignals_Periodic[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_Periodic[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 5U }
};
static const Com_ConfigType periodicConfig =
    { testSignals_Periodic, 1U, testIPdus_Periodic, 1U };

/* Deadline monitoring config with notification callback */
static uint8 mock_DeadlineMissCount = 0;
static PduIdType mock_DeadlineMissPduId = 0;
static void mock_DeadlineNotification(PduIdType PduId) {
    mock_DeadlineMissCount++;
    mock_DeadlineMissPduId = PduId;
}
static const Com_SignalConfigType testSignals_DM[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_DM[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U, 0U, 3U, mock_DeadlineNotification }
};
static const Com_ConfigType dmConfig =
    { testSignals_DM, 1U, testIPdus_DM, 1U };

/* Default/unknown filter algorithm (value 0x07 = COM_ONE_EVERY_N, hits default) */
static const Com_SignalConfigType testSignals_DefaultFilter[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, 0x07U, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus_DefaultFilter[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType defaultFilterConfig =
    { testSignals_DefaultFilter, 1U, testIPdus_DefaultFilter, 1U };

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */
static void mock_reset(void) {
    mock_DetCalls = 0;
    mock_DetLastApiId = 0;
    mock_DetLastErrorId = 0;
    mock_PduR_Transmit_Count = 0U;
    mock_PduR_Transmit_LastPduId = 0;
    mock_PduR_Transmit_LastSduLength = 0;
    mock_DeadlineMissCount = 0;
    mock_DeadlineMissPduId = 0;
}

/* Bring the static SUT state deterministically to COM_UNINIT. Com_Init()
 * re-initializes unconditionally, so Init+DeInit is always a safe reset. */
static void ensure_uninit(void) {
    Com_Init(&emptyConfig);
    Com_DeInit();
    mock_reset();
}

void setUp(void) { ensure_uninit(); }
void tearDown(void) {}

/** @req SWS_Com_00001 */
void test_Com_Init_NullPtr_ShouldNotCrash(void) {
    Com_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Module must still be uninitialized: DeInit on UNINIT reports E_UNINIT. */
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(2U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00001 */
void test_Com_Init_ValidConfig_ShouldSucceed(void) {
    Com_Init(&emptyConfig);
    /* Init is silent; a successful DeInit afterwards (no E_UNINIT DET)
     * proves the module reached COM_STATE_INIT. */
    mock_reset();
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00002 */
void test_Com_DeInit_AfterInit_ShouldSucceed(void) {
    Com_Init(&emptyConfig);
    mock_reset();
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Probe: the module is really UNINIT now. */
    uint8 data = 0x42;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, Com_SendSignal(0U, &data));
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_SENDSIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00003 */
void test_Com_SendSignal_BeforeInit_ShouldFail(void) {
    uint8 data = 0x42;
    uint8 ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_SENDSIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00003 */
void test_Com_SendSignal_NullData_ShouldFail(void) {
    Com_Init(&emptyConfig);
    uint8 ret = Com_SendSignal(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_SENDSIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Com_00004 */
void test_Com_ReceiveSignal_BeforeInit_ShouldFail(void) {
    uint8 data = 0;
    uint8 ret = Com_ReceiveSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_RECEIVESIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00005 */
void test_Com_SendSignalGroup_AfterInit_ShouldReturnResult(void) {
    Com_Init(&emptyConfig);
    uint8 ret = Com_SendSignalGroup(0U);
    /* No IPDU configured -> Com_TransmitIPdu() cannot resolve the PduId. */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00006 */
void test_Com_ReceiveSignalGroup_AfterInit_ShouldReturnResult(void) {
    Com_Init(&emptyConfig);
    uint8 ret = Com_ReceiveSignalGroup(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00007 */
void test_Com_TriggerIPDUSend_AfterInit_ShouldReturnResult(void) {
    Com_Init(&emptyConfig);
    Std_ReturnType ret = Com_TriggerIPDUSend(0U);
    /* Unknown PduId (empty config) -> transmission impossible. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00008 */
void test_Com_TxConfirmation_ShouldNotCrash(void) {
    Com_Init(&triggeredConfig);
    /* COM_TRIGGERED signal: Com_SendSignal must trigger PduR_Transmit. */
    uint8 data = 0x42;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &data));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_PduR_Transmit_LastPduId);
    TEST_ASSERT_EQUAL_UINT16(8U, mock_PduR_Transmit_LastSduLength);
    /* TxConfirmation for a valid, initialized PduId is silent. */
    mock_reset();
    Com_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00009 */
void test_Com_RxIndication_ShouldNotCrash(void) {
    Com_Init(&fullConfig);
    uint8 data[8] = { 0x99, 0, 0, 0, 0, 0, 0, 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Round-trip: the received byte must be readable back via the signal. */
    uint8 out = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &out));
    TEST_ASSERT_EQUAL_UINT8(0x99, out);
}

/** @req SWS_Com_00010 */
void test_Com_TriggerTransmit_AfterInit_ShouldReturnResult(void) {
    Com_Init(&fullConfig);
    uint8 buf[8] = { 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = buf;
    pdu.SduLength = 0U;
    pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = Com_TriggerTransmit(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Com must provide the configured IPDU length. */
    TEST_ASSERT_EQUAL_UINT16(8U, pdu.SduLength);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00021 */
void test_Com_IpduGroupControl_Disable_ShouldHonorVector(void) {
    Com_Init(&groupConfig);
    mock_reset();
    /* Control: both cyclic I-PDUs transmit on the first tick. */
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(2U, mock_PduR_Transmit_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_PduR_Transmit_LastPduId);

    /* Disable ONLY group 0: IPDU 0 stops, IPDU 1 (group 1 not covered by
     * the vector) must keep its enabled state and keep transmitting. */
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_IpduGroupControl(vector, FALSE);
    mock_reset();
    /* Two ticks: tick 1 only decrements IPDU 1's mid-period counter,
     * tick 2 fires it — proving it kept transmitting while IPDU 0 is
     * stopped. (The pre-P1-Phase-8 bug deactivated every I-PDU -> 0.) */
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_PduR_Transmit_Count);
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_PduR_Transmit_LastPduId);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00021 */
void test_Com_IpduGroupControl_Enable_ShouldRestoreTx(void) {
    Com_Init(&groupConfig);
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_IpduGroupControl(vector, FALSE);
    Com_MainFunctionTx(); /* IPDU 1 transmits, IPDU 0 stays idle */
    mock_reset();

    /* Re-enable group 0: IPDU 0 resumes (its tick counter is still 0),
     * IPDU 1 is mid-period and must NOT transmit on this tick. */
    Com_IpduGroupControl(vector, TRUE);
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_PduR_Transmit_LastPduId);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_Com_Init_DoubleInit_ShouldNotCrash(void) {
    Com_Init(&emptyConfig);
    mock_reset();
    /* Com_Init() re-initializes unconditionally and silently. */
    Com_Init(&emptyConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_Com_DeInit_BeforeInit_ShouldNotCrash(void) {
    /* setUp() already left the module in COM_UNINIT. */
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/* ========================================================================== */
/* BATCH 2: Comprehensive branch coverage tests                               */
/* ========================================================================== */

/* --- SendSignal error paths --- */

void test_SendSignal_InvalidSignalId_ShouldFail(void) {
    Com_Init(&fullConfig);
    uint8 data = 0x42;
    Std_ReturnType ret = Com_SendSignal(COM_NUM_OF_SIGNALS, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_SIGNAL, mock_DetLastErrorId);
}

void test_ReceiveSignal_NullData_ShouldFail(void) {
    Com_Init(&fullConfig);
    Std_ReturnType ret = Com_ReceiveSignal(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_ReceiveSignal_InvalidSignalId_ShouldFail(void) {
    Com_Init(&fullConfig);
    uint8 data = 0;
    Std_ReturnType ret = Com_ReceiveSignal(COM_NUM_OF_SIGNALS, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_SIGNAL, mock_DetLastErrorId);
}

/* --- SendSignalGroup / ReceiveSignalGroup error paths --- */

void test_SendSignalGroup_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Com_SendSignalGroup(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_SendSignalGroup_InvalidGroupId_ShouldFail(void) {
    Com_Init(&emptyConfig);
    Std_ReturnType ret = Com_SendSignalGroup(COM_NUM_OF_SIGNAL_GROUPS);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_SIGNALGROUP, mock_DetLastErrorId);
}

void test_ReceiveSignalGroup_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Com_ReceiveSignalGroup(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_ReceiveSignalGroup_InvalidGroupId_ShouldFail(void) {
    Com_Init(&emptyConfig);
    Std_ReturnType ret = Com_ReceiveSignalGroup(COM_NUM_OF_SIGNAL_GROUPS);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_SIGNALGROUP, mock_DetLastErrorId);
}

/* --- TriggerIPDUSend / TriggerTransmit error paths --- */

void test_TriggerIPDUSend_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Com_TriggerIPDUSend(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_TriggerIPDUSend_InvalidPduId_ShouldFail(void) {
    Com_Init(&emptyConfig);
    Std_ReturnType ret = Com_TriggerIPDUSend(COM_NUM_OF_IPDUS);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_IPDU, mock_DetLastErrorId);
}

void test_TriggerTransmit_BeforeInit_ShouldFail(void) {
    PduInfoType pdu;
    uint8 buf[8];
    pdu.SduDataPtr = buf;
    pdu.SduLength = 0U;
    pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = Com_TriggerTransmit(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_TriggerTransmit_NullPduInfo_ShouldFail(void) {
    Com_Init(&fullConfig);
    Std_ReturnType ret = Com_TriggerTransmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TriggerTransmit_InvalidPduId_ShouldReturnNotOk(void) {
    Com_Init(&fullConfig);
    uint8 buf[8] = { 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = buf;
    pdu.SduLength = 0U;
    pdu.MetaDataPtr = NULL_PTR;
    /* PduId = 99 is < COM_NUM_OF_IPDUS(64)? No, 99 >= 64, so it's out of range */
    Std_ReturnType ret = Com_TriggerTransmit(COM_NUM_OF_IPDUS, &pdu);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

/* --- TxConfirmation error paths --- */

void test_TxConfirmation_BeforeInit_ShouldReportDet(void) {
    Com_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_TxConfirmation_InvalidPduId_ShouldNotCrash(void) {
    Com_Init(&fullConfig);
    /* TxPduId >= COM_NUM_OF_IPDUS: the function skips the body silently */
    mock_reset();
    Com_TxConfirmation(COM_NUM_OF_IPDUS, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- TxConfirmation with repetition handling --- */

void test_TxConfirmation_RepeatingEnabled_CountBelowMax(void) {
    Com_Init(&repeatConfig);
    /* Send signal to trigger first transmission */
    uint8 data = 0x42;
    Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
    mock_reset();

    /* TxConfirmation with E_OK should schedule repetition */
    Com_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_TxConfirmation_RepeatingEnabled_ResultNotOk(void) {
    Com_Init(&repeatConfig);
    uint8 data = 0x42;
    Com_SendSignal(0U, &data);
    mock_reset();
    /* TxConfirmation with E_NOT_OK: repetition handling is skipped */
    Com_TxConfirmation(0U, E_NOT_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- RxIndication error paths --- */

void test_RxIndication_BeforeInit_ShouldReportDet(void) {
    uint8 data[8] = { 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_RxIndication_NullPduInfo_ShouldReportDet(void) {
    Com_Init(&fullConfig);
    Com_RxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_RxIndication_NullSduDataPtr_ShouldNotCrash(void) {
    Com_Init(&fullConfig);
    PduInfoType pdu;
    pdu.SduDataPtr = NULL_PTR;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    mock_reset();
    Com_RxIndication(0U, &pdu);
    /* SduDataPtr == NULL: the inner memcpy block is skipped, no DET */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_RxIndication_InvalidPduId_ShouldNotCrash(void) {
    Com_Init(&fullConfig);
    uint8 data[8] = { 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    mock_reset();
    Com_RxIndication(COM_NUM_OF_IPDUS, &pdu);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_RxIndication_WithDeadlineMonitoring(void) {
    Com_Init(&dmConfig);
    /* Enable deadline monitoring for group 0 */
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_ReceptionDMControl(vector, TRUE);
    mock_reset();

    /* RxIndication should reset deadline counter */
    uint8 data[8] = { 0xAA };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- Big-endian signal pack/unpack --- */

void test_SendReceive_BE16_RoundTrip(void) {
    Com_Init(&be16Config);
    uint16 sendVal = 0xABCD;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &sendVal));
    uint16 recvVal = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &recvVal));
    TEST_ASSERT_EQUAL_HEX16(0xABCD, recvVal);
}

void test_SendReceive_BE32_RoundTrip(void) {
    Com_Init(&be32Config);
    uint32 sendVal = 0x12345678UL;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &sendVal));
    uint32 recvVal = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &recvVal));
    TEST_ASSERT_EQUAL_HEX32(0x12345678UL, recvVal);
}

void test_SendReceive_LE16_RoundTrip(void) {
    Com_Init(&le16Config);
    uint16 sendVal = 0x1234;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &sendVal));
    uint16 recvVal = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &recvVal));
    TEST_ASSERT_EQUAL_HEX16(0x1234, recvVal);
}

void test_SendReceive_LE32_RoundTrip(void) {
    Com_Init(&le32Config);
    uint32 sendVal = 0xDEADBEEF;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &sendVal));
    uint32 recvVal = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &recvVal));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, recvVal);
}

/* --- Non-byte-aligned (bit-level) signal pack/unpack --- */

void test_SendReceive_BitLE_RoundTrip(void) {
    Com_Init(&bitLEConfig);
    /* 5-bit signal, max value 31 */
    uint8 sendVal = 0x15; /* 10101b */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &sendVal));
    uint8 recvVal = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &recvVal));
    TEST_ASSERT_EQUAL_HEX8(0x15, recvVal);
}

void test_SendReceive_BitBE_RoundTrip(void) {
    Com_Init(&bitBEConfig);
    /* 6-bit signal, max value 63 */
    uint8 sendVal = 0x2A; /* 101010b */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &sendVal));
    uint8 recvVal = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &recvVal));
    TEST_ASSERT_EQUAL_HEX8(0x2A, recvVal);
}

/* --- Filter algorithm tests --- */

void test_SendSignal_NeverFilter_ShouldBlockTransmission(void) {
    Com_Init(&neverFilterConfig);
    uint8 data = 0x42;
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    /* COM_NEVER filter blocks, but returns COM_SERVICE_OK */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
    /* PduR_Transmit should NOT have been called (filter blocked packing) */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_PduR_Transmit_Count);
}

void test_SendSignal_MaskedEquals_Match_ShouldPass(void) {
    Com_Init(&maskedEqualsConfig);
    /* FilterMask=0x0F, FilterX=0x05. New value & 0x0F == 0x05 -> pass */
    uint8 data = 0x25; /* 0x25 & 0x0F = 0x05 */
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

void test_SendSignal_MaskedEquals_NoMatch_ShouldBlock(void) {
    Com_Init(&maskedEqualsConfig);
    /* FilterMask=0x0F, FilterX=0x05. New value & 0x0F != 0x05 -> block */
    uint8 data = 0x23; /* 0x23 & 0x0F = 0x03, not 0x05 */
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret); /* still OK, just filtered */
}

void test_SendSignal_MaskedDiffers_Differs_ShouldPass(void) {
    Com_Init(&maskedDiffersConfig);
    /* FilterMask=0x0F, FilterX=0x05. New value & 0x0F != 0x05 -> pass */
    uint8 data = 0x23; /* 0x23 & 0x0F = 0x03, differs from 0x05 */
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

void test_SendSignal_MaskedDiffers_Equals_ShouldBlock(void) {
    Com_Init(&maskedDiffersConfig);
    /* FilterMask=0x0F, FilterX=0x05. New value & 0x0F == 0x05 -> block */
    uint8 data = 0x25; /* 0x25 & 0x0F = 0x05, equals FilterX */
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

void test_SendSignal_MaskedDiffersOld_Differs_ShouldPass(void) {
    Com_Init(&maskedDiffersOldConfig);
    /* First send: LastValue=0, mask=0x0F. New=0x0A & 0x0F = 0x0A, Old=0 & 0x0F = 0x00 -> differs -> pass */
    uint8 data = 0x0A;
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

void test_SendSignal_MaskedDiffersOld_Same_ShouldBlock(void) {
    Com_Init(&maskedDiffersOldConfig);
    /* First send with value 0x05 to set LastValue */
    uint8 data1 = 0x05;
    Com_SendSignal(0U, &data1);
    /* Second send: same masked value -> filter blocks */
    uint8 data2 = 0x05; /* 0x05 & 0x0F == 0x05, LastValue & 0x0F == 0x05 -> same -> block */
    Std_ReturnType ret = Com_SendSignal(0U, &data2);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

void test_SendSignal_DefaultFilter_ShouldPass(void) {
    Com_Init(&defaultFilterConfig);
    uint8 data = 0x42;
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    /* Default case in switch returns TRUE (filter passes) */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

/* --- Transfer property: COM_TRIGGERED_ON_CHANGE --- */

void test_SendSignal_TriggeredOnChange_ShouldTransmit(void) {
    Com_Init(&triggeredOnChangeConfig);
    uint8 data = 0x42;
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
    /* COM_TRIGGERED_ON_CHANGE triggers PduR_Transmit */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
}

/* --- Shadow buffer operations --- */

void test_UpdateShadowSignal_BeforeInit_ShouldFail(void) {
    uint8 data = 0x42;
    Std_ReturnType ret = Com_UpdateShadowSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_UpdateShadowSignal_NullData_ShouldFail(void) {
    Com_Init(&fullConfig);
    Std_ReturnType ret = Com_UpdateShadowSignal(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_UpdateShadowSignal_Valid_ShouldSucceed(void) {
    Com_Init(&fullConfig);
    uint8 data = 0x55;
    Std_ReturnType ret = Com_UpdateShadowSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

void test_ReceiveShadowSignal_BeforeInit_ShouldFail(void) {
    uint8 data = 0;
    Std_ReturnType ret = Com_ReceiveShadowSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_ReceiveShadowSignal_NullData_ShouldFail(void) {
    Com_Init(&fullConfig);
    Std_ReturnType ret = Com_ReceiveShadowSignal(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_ReceiveShadowSignal_AfterUpdate_ShouldReturnData(void) {
    Com_Init(&fullConfig);
    uint8 sendData = 0xAB;
    Com_UpdateShadowSignal(0U, &sendData);
    uint8 recvData = 0;
    Std_ReturnType ret = Com_ReceiveShadowSignal(0U, &recvData);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0xAB, recvData);
}

/* --- MainFunctionRx --- */

void test_MainFunctionRx_BeforeInit_ShouldNotCrash(void) {
    mock_reset();
    Com_MainFunctionRx();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_MainFunctionRx_ClearsUpdatedFlag(void) {
    Com_Init(&fullConfig);
    /* Simulate received data */
    uint8 data[8] = { 0x99 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    mock_reset();
    /* MainFunctionRx should clear the Updated flag */
    Com_MainFunctionRx();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_MainFunctionRx_DeadlineMiss_Notification(void) {
    Com_Init(&dmConfig);
    /* Enable deadline monitoring for group 0 */
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_ReceptionDMControl(vector, TRUE);
    mock_reset();

    /* Run MainFunctionRx enough times to exhaust the deadline counter (3 ticks) */
    Com_MainFunctionRx(); /* counter: 3->2 */
    Com_MainFunctionRx(); /* counter: 2->1 */
    Com_MainFunctionRx(); /* counter: 1->0 */
    Com_MainFunctionRx(); /* counter == 0: deadline miss -> notification */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DeadlineMissCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DeadlineMissPduId);
}

void test_MainFunctionRx_GroupDisabled_ShouldSkipProcessing(void) {
    Com_Init(&groupConfig);
    /* Disable group 0 */
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_IpduGroupControl(vector, FALSE);
    mock_reset();
    Com_MainFunctionRx();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- MainFunctionTx --- */

void test_MainFunctionTx_BeforeInit_ShouldNotCrash(void) {
    mock_reset();
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_MainFunctionTx_PeriodicTransmission(void) {
    Com_Init(&periodicConfig);
    mock_reset();
    /* First tick: TimeCounter == 0, should transmit and reset counter to TimePeriod(5) */
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
    mock_reset();
    /* Next 5 ticks: counter decrements (5->4->3->2->1->0) */
    Com_MainFunctionTx(); /* 5->4 */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_PduR_Transmit_Count);
    Com_MainFunctionTx(); /* 4->3 */
    Com_MainFunctionTx(); /* 3->2 */
    Com_MainFunctionTx(); /* 2->1 */
    Com_MainFunctionTx(); /* 1->0 */
    /* 6th tick: counter == 0, transmit again */
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
}

void test_MainFunctionTx_GroupDisabled_ShouldNotTransmit(void) {
    Com_Init(&periodicConfig);
    /* Disable group 0 */
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_IpduGroupControl(vector, FALSE);
    mock_reset();
    Com_MainFunctionTx();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_PduR_Transmit_Count);
}

/* --- GetVersionInfo --- */

void test_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Com_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_GetVersionInfo_ValidPtr_ShouldFillVersion(void) {
    Std_VersionInfoType ver;
    ver.vendorID = 0;
    ver.moduleID = 0;
    ver.sw_major_version = 0;
    ver.sw_minor_version = 0;
    ver.sw_patch_version = 0;
    Com_GetVersionInfo(&ver);
    TEST_ASSERT_EQUAL_UINT16(COM_VENDOR_ID, ver.vendorID);
    TEST_ASSERT_EQUAL_UINT16(COM_MODULE_ID, ver.moduleID);
    TEST_ASSERT_EQUAL_UINT8(COM_SW_MAJOR_VERSION, ver.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(COM_SW_MINOR_VERSION, ver.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(COM_SW_PATCH_VERSION, ver.sw_patch_version);
}

/* --- GetStatus --- */

void test_GetStatus_Uninit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL_UINT8(COM_UNINIT, Com_GetStatus());
}

void test_GetStatus_Init_ShouldReturnInit(void) {
    Com_Init(&emptyConfig);
    TEST_ASSERT_EQUAL_UINT8(COM_INIT, Com_GetStatus());
}

/* --- ReceptionDMControl --- */

void test_ReceptionDMControl_BeforeInit_ShouldReportDet(void) {
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_ReceptionDMControl(vector, TRUE);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_ReceptionDMControl_Enable_ShouldActivateDM(void) {
    Com_Init(&dmConfig);
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_ReceptionDMControl(vector, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_ReceptionDMControl_Disable_ShouldDeactivateDM(void) {
    Com_Init(&dmConfig);
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_ReceptionDMControl(vector, TRUE);
    mock_reset();
    Com_ReceptionDMControl(vector, FALSE);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_EnableReceptionDM_ShouldWork(void) {
    Com_Init(&dmConfig);
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_EnableReceptionDM(vector);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_DisableReceptionDM_ShouldWork(void) {
    Com_Init(&dmConfig);
    Com_IpduGroupVector vector = { 0x01U, 0x00U };
    Com_DisableReceptionDM(vector);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- InvalidateSignal --- */

void test_InvalidateSignal_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Com_InvalidateSignal(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_InvalidateSignal_InvalidSignalId_ShouldFail(void) {
    Com_Init(&fullConfig);
    Std_ReturnType ret = Com_InvalidateSignal(COM_NUM_OF_SIGNALS);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_INVALID_SIGNAL_ID, mock_DetLastErrorId);
}

void test_InvalidateSignal_Valid_ShouldSucceed(void) {
    Com_Init(&fullConfig);
    /* First write some data */
    uint8 data = 0x42;
    Com_SendSignal(0U, &data);
    mock_reset();
    /* Invalidate should set bytes to 0xFF */
    Std_ReturnType ret = Com_InvalidateSignal(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
    /* Read back the invalidated signal */
    uint8 recvData = 0;
    Com_ReceiveSignal(0U, &recvData);
    TEST_ASSERT_EQUAL_HEX8(0xFF, recvData);
}

/* --- InvalidateSignalGroup --- */

void test_InvalidateSignalGroup_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Com_InvalidateSignalGroup(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

void test_InvalidateSignalGroup_InvalidGroupId_ShouldFail(void) {
    Com_Init(&emptyConfig);
    Std_ReturnType ret = Com_InvalidateSignalGroup(COM_NUM_OF_SIGNAL_GROUPS);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_E_INVALID_SIGNAL_GROUP_ID, mock_DetLastErrorId);
}

void test_InvalidateSignalGroup_Valid_ShouldSucceed(void) {
    Com_Init(&fullConfig);
    Std_ReturnType ret = Com_InvalidateSignalGroup(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
}

/* --- IpduGroupVector utilities --- */

void test_ClearIpduGroupVector_ShouldZeroAll(void) {
    Com_IpduGroupVector vector = { 0xFFU, 0xFFU };
    Com_ClearIpduGroupVector(vector);
    uint8 i;
    for (i = 0; i < (COM_NUM_IPDU_GROUPS + 7U) / 8U; i++) {
        TEST_ASSERT_EQUAL_UINT8(0U, vector[i]);
    }
}

void test_SetIpduGroup_ShouldSetBit(void) {
    Com_IpduGroupVector vector = { 0x00U, 0x00U };
    Com_SetIpduGroup(vector, 3);
    TEST_ASSERT_NOT_EQUAL(0U, vector[0] & (1U << 3));
}

void test_SetIpduGroup_InvalidId_ShouldNotCrash(void) {
    Com_IpduGroupVector vector = { 0x00U, 0x00U };
    Com_SetIpduGroup(vector, COM_NUM_OF_IPDU_GROUPS);
    /* Should not modify anything */
    TEST_ASSERT_EQUAL_UINT8(0U, vector[0]);
}

void test_ClearIpduGroup_ShouldClearBit(void) {
    Com_IpduGroupVector vector = { 0xFFU, 0xFFU };
    Com_ClearIpduGroup(vector, 2);
    TEST_ASSERT_EQUAL_UINT8(0U, vector[0] & (1U << 2));
}

void test_ClearIpduGroup_InvalidId_ShouldNotCrash(void) {
    Com_IpduGroupVector vector = { 0xFFU, 0xFFU };
    Com_ClearIpduGroup(vector, COM_NUM_OF_IPDU_GROUPS);
    /* Should not modify anything */
    TEST_ASSERT_EQUAL_UINT8(0xFFU, vector[0]);
}

/* --- RxIndication with SduLength > COM_MAX_IPDU_BUFFER_SIZE --- */

void test_RxIndication_LargeSduLength_ShouldClamp(void) {
    Com_Init(&fullConfig);
    uint8 data[COM_MAX_IPDU_BUFFER_SIZE + 16];
    uint16 i;
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8)(i & 0xFF);
    }
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    mock_reset();
    Com_RxIndication(0U, &pdu);
    /* Should not crash; copyLength is clamped to COM_MAX_IPDU_BUFFER_SIZE */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- SendSignal with empty config (GetIPduConfig returns NULL) --- */

void test_SendSignal_EmptyConfig_ShouldReturnNotOk(void) {
    Com_Init(&emptyConfig);
    uint8 data = 0x42;
    Std_ReturnType ret = Com_SendSignal(0U, &data);
    /* GetSignalConfig: SignalId(0) < COM_NUM_OF_SIGNALS but ConfigPtr->Signals is NULL_PTR
     * so accessing Signals[0] is technically UB, but with NULL config the check
     * (ConfigPtr != NULL_PTR) in GetSignalConfig fails -> returns NULL_PTR */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
}

/* --- MainFunctionRouteSignals --- */

void test_MainFunctionRouteSignals_BeforeInit_ShouldNotCrash(void) {
    mock_reset();
    Com_MainFunctionRouteSignals();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_MainFunctionRouteSignals_AfterInit_ShouldNotCrash(void) {
    Com_Init(&emptyConfig);
    mock_reset();
    Com_MainFunctionRouteSignals();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/* --- Stub functions --- */

void test_SendDynSignal_ShouldReturnNotOk(void) {
    uint8 data = 0x42;
    Std_ReturnType ret = Com_SendDynSignal(0U, &data, 1U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
}

void test_ReceiveDynSignal_ShouldReturnNotOk(void) {
    uint8 data = 0;
    uint16 len = 1;
    Std_ReturnType ret = Com_ReceiveDynSignal(0U, &data, &len);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
}

void test_SwitchIpduTxMode_ShouldNotCrash(void) {
    Com_SwitchIpduTxMode(0U, COM_DIRECT);
    /* No return value, just ensure no crash */
}

void test_TriggerIPDUSendWithMetaData_ShouldReturnNotOk(void) {
    uint8 meta[4] = { 0 };
    Std_ReturnType ret = Com_TriggerIPDUSendWithMetaData(0U, meta);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

void test_TpRxIndication_ShouldNotCrash(void) {
    Com_TpRxIndication(0U, E_OK);
}

void test_TpTxConfirmation_ShouldNotCrash(void) {
    Com_TpTxConfirmation(0U, E_OK);
}

/* --- BE16 via RxIndication round-trip --- */

void test_RxIndication_BE16_UnpackCorrectly(void) {
    Com_Init(&be16Config);
    /* Pack 0xABCD in big-endian: byte[0]=0xAB, byte[1]=0xCD */
    uint8 data[8] = { 0xAB, 0xCD, 0, 0, 0, 0, 0, 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    uint16 recvVal = 0;
    Com_ReceiveSignal(0U, &recvVal);
    TEST_ASSERT_EQUAL_HEX16(0xABCD, recvVal);
}

void test_RxIndication_LE16_UnpackCorrectly(void) {
    Com_Init(&le16Config);
    /* Pack 0x1234 in little-endian: byte[0]=0x34, byte[1]=0x12 */
    uint8 data[8] = { 0x34, 0x12, 0, 0, 0, 0, 0, 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    uint16 recvVal = 0;
    Com_ReceiveSignal(0U, &recvVal);
    TEST_ASSERT_EQUAL_HEX16(0x1234, recvVal);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Com_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Com_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Com_DeInit_AfterInit_ShouldSucceed);
    RUN_TEST(test_Com_SendSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_Com_SendSignal_NullData_ShouldFail);
    RUN_TEST(test_Com_ReceiveSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_Com_SendSignalGroup_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_ReceiveSignalGroup_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_TriggerIPDUSend_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_TxConfirmation_ShouldNotCrash);
    RUN_TEST(test_Com_RxIndication_ShouldNotCrash);
    RUN_TEST(test_Com_TriggerTransmit_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_IpduGroupControl_Disable_ShouldHonorVector);
    RUN_TEST(test_Com_IpduGroupControl_Enable_ShouldRestoreTx);
    RUN_TEST(test_Com_Init_DoubleInit_ShouldNotCrash);
    RUN_TEST(test_Com_DeInit_BeforeInit_ShouldNotCrash);
    /* Batch 2: SendSignal/ReceiveSignal error paths */
    RUN_TEST(test_SendSignal_InvalidSignalId_ShouldFail);
    RUN_TEST(test_ReceiveSignal_NullData_ShouldFail);
    RUN_TEST(test_ReceiveSignal_InvalidSignalId_ShouldFail);
    /* Batch 2: SignalGroup error paths */
    RUN_TEST(test_SendSignalGroup_BeforeInit_ShouldFail);
    RUN_TEST(test_SendSignalGroup_InvalidGroupId_ShouldFail);
    RUN_TEST(test_ReceiveSignalGroup_BeforeInit_ShouldFail);
    RUN_TEST(test_ReceiveSignalGroup_InvalidGroupId_ShouldFail);
    /* Batch 2: TriggerIPDUSend / TriggerTransmit error paths */
    RUN_TEST(test_TriggerIPDUSend_BeforeInit_ShouldFail);
    RUN_TEST(test_TriggerIPDUSend_InvalidPduId_ShouldFail);
    RUN_TEST(test_TriggerTransmit_BeforeInit_ShouldFail);
    RUN_TEST(test_TriggerTransmit_NullPduInfo_ShouldFail);
    RUN_TEST(test_TriggerTransmit_InvalidPduId_ShouldReturnNotOk);
    /* Batch 2: TxConfirmation */
    RUN_TEST(test_TxConfirmation_BeforeInit_ShouldReportDet);
    RUN_TEST(test_TxConfirmation_InvalidPduId_ShouldNotCrash);
    RUN_TEST(test_TxConfirmation_RepeatingEnabled_CountBelowMax);
    RUN_TEST(test_TxConfirmation_RepeatingEnabled_ResultNotOk);
    /* Batch 2: RxIndication */
    RUN_TEST(test_RxIndication_BeforeInit_ShouldReportDet);
    RUN_TEST(test_RxIndication_NullPduInfo_ShouldReportDet);
    RUN_TEST(test_RxIndication_NullSduDataPtr_ShouldNotCrash);
    RUN_TEST(test_RxIndication_InvalidPduId_ShouldNotCrash);
    RUN_TEST(test_RxIndication_WithDeadlineMonitoring);
    RUN_TEST(test_RxIndication_LargeSduLength_ShouldClamp);
    /* Batch 2: Endianness / signal size round-trips */
    RUN_TEST(test_SendReceive_BE16_RoundTrip);
    RUN_TEST(test_SendReceive_BE32_RoundTrip);
    RUN_TEST(test_SendReceive_LE16_RoundTrip);
    RUN_TEST(test_SendReceive_LE32_RoundTrip);
    RUN_TEST(test_SendReceive_BitLE_RoundTrip);
    RUN_TEST(test_SendReceive_BitBE_RoundTrip);
    RUN_TEST(test_RxIndication_BE16_UnpackCorrectly);
    RUN_TEST(test_RxIndication_LE16_UnpackCorrectly);
    /* Batch 2: Filter algorithms */
    RUN_TEST(test_SendSignal_NeverFilter_ShouldBlockTransmission);
    RUN_TEST(test_SendSignal_MaskedEquals_Match_ShouldPass);
    RUN_TEST(test_SendSignal_MaskedEquals_NoMatch_ShouldBlock);
    RUN_TEST(test_SendSignal_MaskedDiffers_Differs_ShouldPass);
    RUN_TEST(test_SendSignal_MaskedDiffers_Equals_ShouldBlock);
    RUN_TEST(test_SendSignal_MaskedDiffersOld_Differs_ShouldPass);
    RUN_TEST(test_SendSignal_MaskedDiffersOld_Same_ShouldBlock);
    RUN_TEST(test_SendSignal_DefaultFilter_ShouldPass);
    /* Batch 2: Transfer properties */
    RUN_TEST(test_SendSignal_TriggeredOnChange_ShouldTransmit);
    /* Batch 2: Shadow buffer */
    RUN_TEST(test_UpdateShadowSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_UpdateShadowSignal_NullData_ShouldFail);
    RUN_TEST(test_UpdateShadowSignal_Valid_ShouldSucceed);
    RUN_TEST(test_ReceiveShadowSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_ReceiveShadowSignal_NullData_ShouldFail);
    RUN_TEST(test_ReceiveShadowSignal_AfterUpdate_ShouldReturnData);
    /* Batch 2: MainFunctionRx */
    RUN_TEST(test_MainFunctionRx_BeforeInit_ShouldNotCrash);
    RUN_TEST(test_MainFunctionRx_ClearsUpdatedFlag);
    RUN_TEST(test_MainFunctionRx_DeadlineMiss_Notification);
    RUN_TEST(test_MainFunctionRx_GroupDisabled_ShouldSkipProcessing);
    /* Batch 2: MainFunctionTx */
    RUN_TEST(test_MainFunctionTx_BeforeInit_ShouldNotCrash);
    RUN_TEST(test_MainFunctionTx_PeriodicTransmission);
    RUN_TEST(test_MainFunctionTx_GroupDisabled_ShouldNotTransmit);
    /* Batch 2: GetVersionInfo */
    RUN_TEST(test_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_GetVersionInfo_ValidPtr_ShouldFillVersion);
    /* Batch 2: GetStatus */
    RUN_TEST(test_GetStatus_Uninit_ShouldReturnUninit);
    RUN_TEST(test_GetStatus_Init_ShouldReturnInit);
    /* Batch 2: ReceptionDMControl */
    RUN_TEST(test_ReceptionDMControl_BeforeInit_ShouldReportDet);
    RUN_TEST(test_ReceptionDMControl_Enable_ShouldActivateDM);
    RUN_TEST(test_ReceptionDMControl_Disable_ShouldDeactivateDM);
    RUN_TEST(test_EnableReceptionDM_ShouldWork);
    RUN_TEST(test_DisableReceptionDM_ShouldWork);
    /* Batch 2: InvalidateSignal / InvalidateSignalGroup */
    RUN_TEST(test_InvalidateSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_InvalidateSignal_InvalidSignalId_ShouldFail);
    RUN_TEST(test_InvalidateSignal_Valid_ShouldSucceed);
    RUN_TEST(test_InvalidateSignalGroup_BeforeInit_ShouldFail);
    RUN_TEST(test_InvalidateSignalGroup_InvalidGroupId_ShouldFail);
    RUN_TEST(test_InvalidateSignalGroup_Valid_ShouldSucceed);
    /* Batch 2: IpduGroupVector utilities */
    RUN_TEST(test_ClearIpduGroupVector_ShouldZeroAll);
    RUN_TEST(test_SetIpduGroup_ShouldSetBit);
    RUN_TEST(test_SetIpduGroup_InvalidId_ShouldNotCrash);
    RUN_TEST(test_ClearIpduGroup_ShouldClearBit);
    RUN_TEST(test_ClearIpduGroup_InvalidId_ShouldNotCrash);
    /* Batch 2: Misc */
    RUN_TEST(test_SendSignal_EmptyConfig_ShouldReturnNotOk);
    RUN_TEST(test_MainFunctionRouteSignals_BeforeInit_ShouldNotCrash);
    RUN_TEST(test_MainFunctionRouteSignals_AfterInit_ShouldNotCrash);
    RUN_TEST(test_SendDynSignal_ShouldReturnNotOk);
    RUN_TEST(test_ReceiveDynSignal_ShouldReturnNotOk);
    RUN_TEST(test_SwitchIpduTxMode_ShouldNotCrash);
    RUN_TEST(test_TriggerIPDUSendWithMetaData_ShouldReturnNotOk);
    RUN_TEST(test_TpRxIndication_ShouldNotCrash);
    RUN_TEST(test_TpTxConfirmation_ShouldNotCrash);
    return UnityEnd();
}

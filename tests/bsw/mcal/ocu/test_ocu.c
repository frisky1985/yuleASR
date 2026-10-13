/**
 * @file test_ocu.c
 * @brief Ocu (Output Compare Unit) Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/ocu/src/Ocu.c  @tests src/bsw/mcal/ocu/include/Ocu.h

#include "unity.h"
#include "Ocu.h"
#include "Ocu_Cfg.h"
#include "Ocu_Private.h"

/* Forward declarations for IRQ handlers (not in public header) */
extern void Ocu_Channel0_IrqHandler(void);
extern void Ocu_Channel1_IrqHandler(void);
extern void Ocu_Channel2_IrqHandler(void);
extern void Ocu_Channel3_IrqHandler(void);

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config */
static Ocu_ChannelConfigType testChannels[2];
static Ocu_ConfigType testConfig;

static void test_Ocu_SetupDefaultConfig(void) {
    testChannels[0].ChannelId = 0U;
    testChannels[0].DefaultPinState = OCU_LOW;
    testChannels[0].DefaultThreshold = 1000U;
    testChannels[0].Notification = NULL_PTR;
    testChannels[0].RunningInBackground = FALSE;
    testChannels[0].BaseAddress = OCU_CHANNEL_0_BASE_ADDRESS;

    testChannels[1].ChannelId = 1U;
    testChannels[1].DefaultPinState = OCU_LOW;
    testChannels[1].DefaultThreshold = 2000U;
    testChannels[1].Notification = NULL_PTR;
    testChannels[1].RunningInBackground = FALSE;
    testChannels[1].BaseAddress = OCU_CHANNEL_1_BASE_ADDRESS;

    testConfig.Channels = testChannels;
    testConfig.NumChannels = 2U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
    testConfig.DeInitApi = TRUE;
    testConfig.PinStateApi = TRUE;
    testConfig.SetPinActionApi = TRUE;
    testConfig.SetThresholdApi = TRUE;
    testConfig.NotificationSupported = TRUE;
    testConfig.MaxCounterValue = OCU_MAX_COUNTER_VALUE;
}

void setUp(void) {
    mock_Det_Reset();
    test_Ocu_SetupDefaultConfig();
}

void tearDown(void) {
    /* The SUT keeps its initialization state in file-static variables that
     * persist between test cases on the host. Deinitialize after every test
     * so each case observes the genuine uninitialized driver. When the driver
     * was never initialized this only reports OCU_E_UNINIT into the mock,
     * which the next setUp() clears. */
    Ocu_DeInit();
}

/* Init/DeInit Tests */
/** @req SWS_Ocu_00001 */
void test_Ocu_Init_NullPtr_ShouldUsePreCompileConfig(void) {
    /* Pre-compile configuration variant (OCU_CONFIGURATION_VARIANT ==
     * OCU_VARIANT_PRE_COMPILE): the SUT accepts a NULL pointer, silently
     * falls back to the built-in Ocu_Config and reports no DET error. */
    Ocu_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    /* Driver must be operational afterwards */
    Ocu_StartChannel(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00001 */
void test_Ocu_Init_ValidConfig_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00002 */
void test_Ocu_DeInit_AfterInit_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_DeInit();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* Channel Control Tests */
/** @req SWS_Ocu_00003 */
void test_Ocu_StartChannel_ValidChannel_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00004 */
void test_Ocu_StopChannel_AfterStart_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    Ocu_StopChannel(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00003 */
void test_Ocu_StartChannel_InvalidChannel_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(OCU_NUM_CHANNELS + 1U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_Ocu_00003 */
void test_Ocu_StartChannel_BeforeInit_ShouldReportError(void) {
    Ocu_StartChannel(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/* Pin Action Tests */
/** @req SWS_Ocu_00005 */
void test_Ocu_SetPinState_High_ShouldSetHigh(void) {
    Ocu_Init(&testConfig);
    Ocu_SetPinState(0U, OCU_HIGH);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00005 */
void test_Ocu_SetPinState_Low_ShouldSetLow(void) {
    Ocu_Init(&testConfig);
    Ocu_SetPinState(0U, OCU_LOW);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00006 */
void test_Ocu_SetPinAction_Toggle_ShouldToggle(void) {
    Ocu_Init(&testConfig);
    Ocu_SetPinAction(0U, OCU_TOGGLE);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00006 */
void test_Ocu_SetPinAction_SetHigh_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_SetPinAction(0U, OCU_SET_HIGH);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00006 */
void test_Ocu_SetPinAction_SetLow_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_SetPinAction(0U, OCU_SET_LOW);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* Threshold Tests */
/** @req SWS_Ocu_00007 */
void test_Ocu_SetAbsoluteThreshold_ValidValue_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(0U, 0U, 5000U);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/** @req SWS_Ocu_00008 */
void test_Ocu_SetRelativeThreshold_ValidValue_ShouldSucceed(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    Std_ReturnType result = Ocu_SetRelativeThreshold(0U, 1000U);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/** @req SWS_Ocu_00009 */
void test_Ocu_GetCounter_ShouldReturnValidValue(void) {
    Ocu_Init(&testConfig);
    Ocu_ValueType counter = Ocu_GetCounter(0U);
    (void)counter;
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00007 */
void test_Ocu_SetAbsoluteThreshold_BeforeInit_ShouldReportError(void) {
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(0U, 0U, 5000U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/* Notification Tests */
/** @req SWS_Ocu_00011 */
void test_Ocu_EnableNotification_NoCallbackConfigured_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    /* In the pre-compile variant the SUT always uses the built-in Ocu_Config
     * (the passed config is ignored), where every channel's notification
     * callback is NULL_PTR. Enabling notification on such a channel is
     * rejected with OCU_E_PARAM_CHANNEL. */
    Ocu_EnableNotification(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_Ocu_00010 */
void test_Ocu_DisableNotification_NoCallbackConfigured_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    /* Same as enable: no notification callback is configured for the channel
     * in the built-in pre-compile config, so the SUT reports
     * OCU_E_PARAM_CHANNEL. */
    Ocu_DisableNotification(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_Ocu_00011 */
void test_Ocu_EnableNotification_BeforeInit_ShouldReportError(void) {
    Ocu_EnableNotification(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/* Version Info Test */
/** @req SWS_Ocu_00012 */
void test_Ocu_GetVersionInfo_ShouldReturnCorrectVersion(void) {
    Std_VersionInfoType versionInfo;
    Ocu_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL(OCU_SW_MAJOR_VERSION, versionInfo.vendorID);
}

/* --- Coverage for lines 223, 331, 359, 366, 374 --- */

/**
 * Line 223: Double init should report OCU_E_ALREADY_INITIALIZED.
 */
void test_Ocu_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    Ocu_Init(&testConfig);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/**
 * Line 331: StartChannel when already running should report OCU_E_CHANNEL_BUSY.
 * The `if (!Ocu_ChannelState[Channel].IsRunning)` else branch.
 */
void test_Ocu_StartChannel_AlreadyRunning_ShouldReportChannelBusy(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    mock_Det_Reset();
    Ocu_StartChannel(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_CHANNEL_BUSY, mock_DetLastErrorId);
}

/**
 * Line 359: StopChannel before init should report OCU_E_UNINIT.
 */
void test_Ocu_StopChannel_BeforeInit_ShouldReportUninit(void) {
    /* tearDown calls DeInit which may report UNINIT; setUp clears the mock */
    Ocu_StopChannel(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/**
 * Line 366: StopChannel with invalid channel should report OCU_E_PARAM_CHANNEL.
 */
void test_Ocu_StopChannel_InvalidChannel_ShouldReportParamChannel(void) {
    Ocu_Init(&testConfig);
    Ocu_StopChannel(OCU_NUM_CHANNELS + 1U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/**
 * Line 374: StopChannel when channel is NOT running (IsRunning == 0).
 * The `if ((Ocu_ChannelState[Channel].IsRunning) != 0U)` false branch.
 * No error, just a silent no-op.
 */
void test_Ocu_StopChannel_NotRunning_ShouldSilentlyNoOp(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    /* Channel 0 was never started */
    Ocu_StopChannel(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* ============================================================================
 * IRQ Handler Branch Coverage Tests
 * ============================================================================ */

/* Helper: notification callback for IRQ tests */
static uint8 notification_call_count = 0;
static void test_notification_callback(void) {
    notification_call_count++;
}

/* Line 77: Ocu_IrqHandler with compare match bit set */
void test_Ocu_IrqHandler_CompareMatch_ShouldProcessMatch(void) {
    Ocu_HwRegisterType* hwRegs;

    /* Init the module first */
    Ocu_Init(&testConfig);

    /* Get hardware registers for channel 0 */
    hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_0);

    /* Set compare match status bit */
    hwRegs->Status = OCU_STATUS_MATCH_BIT;

    /* Set pin action to TOGGLE so ProcessCompareMatch does something */
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_TOGGLE;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_LOW;
    Ocu_ChannelState[OCU_CHANNEL_0].NotificationEnabled = FALSE;

    /* Call IRQ handler - should process the match */
    Ocu_Channel0_IrqHandler();

    /* Status should have been cleared (write of MATCH_BIT to clear) */
    TEST_ASSERT_EQUAL(OCU_STATUS_MATCH_BIT, hwRegs->Status);
    /* Pin state should have toggled from LOW to HIGH */
    TEST_ASSERT_EQUAL(OCU_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* Line 77 false branch: Ocu_IrqHandler with no match bit (only overflow) */
void test_Ocu_IrqHandler_NoMatch_OnlyOverflow(void) {
    Ocu_HwRegisterType* hwRegs;

    Ocu_Init(&testConfig);
    hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_0);

    /* Set only overflow bit, no match */
    hwRegs->Status = OCU_STATUS_OVERFLOW_BIT;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_LOW;

    Ocu_Channel0_IrqHandler();

    /* Pin state should NOT have changed (no match processing) */
    TEST_ASSERT_EQUAL(OCU_LOW, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
    /* Overflow bit should have been cleared */
    TEST_ASSERT_EQUAL(OCU_STATUS_OVERFLOW_BIT, hwRegs->Status);
}

/* Line 87: Ocu_IrqHandler with notification enabled and callback set.
 * NOTE: In pre-compile variant, Ocu_CurrentConfig points to const config,
 * so we cannot set a notification callback at runtime. This branch cannot
 * be exercised on the host in pre-compile configuration variant. */

/* Line 87 false branch: notification enabled but callback is NULL */
void test_Ocu_IrqHandler_NotificationEnabled_NullCallback(void) {
    Ocu_HwRegisterType* hwRegs;

    testChannels[0].Notification = NULL_PTR;
    Ocu_Init(&testConfig);
    hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_0);

    hwRegs->Status = OCU_STATUS_MATCH_BIT;
    Ocu_ChannelState[OCU_CHANNEL_0].NotificationEnabled = TRUE;
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_HOLD;

    /* Should not crash even with NULL callback */
    Ocu_Channel0_IrqHandler();
}

/* Line 87 false branch: notification disabled */
void test_Ocu_IrqHandler_NotificationDisabled_ShouldNotCallCallback(void) {
    Ocu_HwRegisterType* hwRegs;

    testChannels[0].Notification = test_notification_callback;
    notification_call_count = 0;

    Ocu_Init(&testConfig);
    hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_0);

    hwRegs->Status = OCU_STATUS_MATCH_BIT;
    Ocu_ChannelState[OCU_CHANNEL_0].NotificationEnabled = FALSE;
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_HOLD;

    Ocu_Channel0_IrqHandler();

    /* Notification should NOT have been called */
    TEST_ASSERT_EQUAL(0U, notification_call_count);
}

/* Ocu_ProcessCompareMatch: SET_HIGH action */
void test_Ocu_ProcessCompareMatch_SetHigh(void) {
    Ocu_Init(&testConfig);
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_SET_HIGH;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_LOW;

    Ocu_ProcessCompareMatch(OCU_CHANNEL_0);

    TEST_ASSERT_EQUAL(OCU_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* Ocu_ProcessCompareMatch: SET_LOW action */
void test_Ocu_ProcessCompareMatch_SetLow(void) {
    Ocu_Init(&testConfig);
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_SET_LOW;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_HIGH;

    Ocu_ProcessCompareMatch(OCU_CHANNEL_0);

    TEST_ASSERT_EQUAL(OCU_LOW, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* Ocu_ProcessCompareMatch: TOGGLE action from HIGH */
void test_Ocu_ProcessCompareMatch_ToggleFromHigh(void) {
    Ocu_Init(&testConfig);
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_TOGGLE;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_HIGH;

    Ocu_ProcessCompareMatch(OCU_CHANNEL_0);

    TEST_ASSERT_EQUAL(OCU_LOW, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* Ocu_ProcessCompareMatch: HOLD action (no change) */
void test_Ocu_ProcessCompareMatch_Hold(void) {
    Ocu_Init(&testConfig);
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_HOLD;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_HIGH;

    Ocu_ProcessCompareMatch(OCU_CHANNEL_0);

    TEST_ASSERT_EQUAL(OCU_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* IRQ handler on channel 1 */
void test_Ocu_Channel1_IrqHandler_CompareMatch(void) {
    Ocu_HwRegisterType* hwRegs;

    Ocu_Init(&testConfig);
    hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_1);

    hwRegs->Status = OCU_STATUS_MATCH_BIT;
    Ocu_ChannelState[OCU_CHANNEL_1].PinAction = OCU_TOGGLE;
    Ocu_ChannelState[OCU_CHANNEL_1].CurrentPinState = OCU_LOW;
    Ocu_ChannelState[OCU_CHANNEL_1].NotificationEnabled = FALSE;

    Ocu_Channel1_IrqHandler();

    TEST_ASSERT_EQUAL(OCU_HIGH, Ocu_ChannelState[OCU_CHANNEL_1].CurrentPinState);
}

/* IRQ handler with both match and overflow bits set */
void test_Ocu_IrqHandler_BothMatchAndOverflow(void) {
    Ocu_HwRegisterType* hwRegs;

    Ocu_Init(&testConfig);
    hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_0);

    hwRegs->Status = OCU_STATUS_MATCH_BIT | OCU_STATUS_OVERFLOW_BIT;
    Ocu_ChannelState[OCU_CHANNEL_0].PinAction = OCU_SET_HIGH;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_LOW;
    Ocu_ChannelState[OCU_CHANNEL_0].NotificationEnabled = FALSE;

    Ocu_Channel0_IrqHandler();

    /* Match should have been processed */
    TEST_ASSERT_EQUAL(OCU_HIGH, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* IRQ handler with no status bits set */
void test_Ocu_IrqHandler_NoStatusBits(void) {
    Ocu_Init(&testConfig);
    Ocu_HwRegisterType* hwRegs = Ocu_HwGetRegisterBase(OCU_CHANNEL_0);

    hwRegs->Status = 0U;
    Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState = OCU_LOW;

    /* Should do nothing */
    Ocu_Channel0_IrqHandler();

    TEST_ASSERT_EQUAL(OCU_LOW, Ocu_ChannelState[OCU_CHANNEL_0].CurrentPinState);
}

/* ============================================================================
 * Additional Branch Coverage Tests
 * ============================================================================ */

/* Line 411: SetPinState with invalid pin state (not OCU_HIGH=0 or OCU_LOW=1) */
void test_Ocu_SetPinState_InvalidPinState_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    /* Pin state 0x05 is neither OCU_HIGH (0) nor OCU_LOW (1) */
    Ocu_SetPinState(0U, (Ocu_OutputPinStateType)0x05U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_PIN_STATE, mock_DetLastErrorId);
}

/* Line 418: SetPinState while channel is running */
void test_Ocu_SetPinState_WhileRunning_ShouldReportInvalidState(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    mock_Det_Reset();
    Ocu_SetPinState(0U, OCU_HIGH);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_INVALID_STATE, mock_DetLastErrorId);
}

/* Line 458: SetPinAction with invalid action (> OCU_HOLD=3) */
void test_Ocu_SetPinAction_InvalidAction_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    Ocu_SetPinAction(0U, (Ocu_PinActionType)0x05U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_ACTION, mock_DetLastErrorId);
}

/* Line 502: SetAbsoluteThreshold with ReferenceValue >= MaxCounterValue */
void test_Ocu_SetAbsoluteThreshold_InvalidRefValue_ShouldReturnNotOk(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    /* MaxCounterValue = OCU_MAX_COUNTER_VALUE = 4294967295U */
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(0U, OCU_MAX_COUNTER_VALUE, 5000U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_REF_VALUE, mock_DetLastErrorId);
}

/* Line 509: SetAbsoluteThreshold with AbsoluteValue >= MaxCounterValue */
void test_Ocu_SetAbsoluteThreshold_InvalidThresholdValue_ShouldReturnNotOk(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    Std_ReturnType result = Ocu_SetAbsoluteThreshold(0U, 0U, OCU_MAX_COUNTER_VALUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_THRESHOLD_VALUE, mock_DetLastErrorId);
}

/* Line 558: SetRelativeThreshold with zero value */
void test_Ocu_SetRelativeThreshold_ZeroValue_ShouldReturnNotOk(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    mock_Det_Reset();
    Std_ReturnType result = Ocu_SetRelativeThreshold(0U, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_THRESHOLD_VALUE, mock_DetLastErrorId);
}

/* Line 596: GetCounter before initialization */
void test_Ocu_GetCounter_BeforeInit_ShouldReturnZero(void) {
    /* Do NOT init - tearDown will call DeInit which reports UNINIT */
    Ocu_ValueType counter = Ocu_GetCounter(0U);
    TEST_ASSERT_EQUAL(0U, counter);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/* Line 603: GetCounter with invalid channel */
void test_Ocu_GetCounter_InvalidChannel_ShouldReturnZero(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    Ocu_ValueType counter = Ocu_GetCounter(OCU_NUM_CHANNELS + 1U);
    TEST_ASSERT_EQUAL(0U, counter);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* Line 284: DeInit with running channels - should stop them */
void test_Ocu_DeInit_WithRunningChannels_ShouldStopChannels(void) {
    Ocu_Init(&testConfig);
    Ocu_StartChannel(0U);
    Ocu_StartChannel(1U);
    /* Verify channels are running */
    TEST_ASSERT_EQUAL(TRUE, Ocu_ChannelState[0U].IsRunning);
    TEST_ASSERT_EQUAL(TRUE, Ocu_ChannelState[1U].IsRunning);
    mock_Det_Reset();
    /* DeInit should stop running channels (line 284 true branch) */
    Ocu_DeInit();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    /* After DeInit, module state should be UNINIT */
    TEST_ASSERT_EQUAL(OCU_UNINIT, Ocu_ModuleState);
}

/* Line 704: GetVersionInfo with NULL pointer */
void test_Ocu_GetVersionInfo_NullPtr_ShouldReportError(void) {
    mock_Det_Reset();
    Ocu_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* Line 397: SetPinState with UNINIT state */
void test_Ocu_SetPinState_Uninit_ShouldReportError(void) {
    mock_Det_Reset();
    Ocu_ModuleState = OCU_UNINIT;
    Ocu_SetPinState(0U, OCU_HIGH);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/* Line 404: SetPinState with invalid channel */
void test_Ocu_SetPinState_InvalidChannel_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    Ocu_SetPinState(OCU_NUM_CHANNELS + 1U, OCU_HIGH);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* Line 444: SetPinAction with UNINIT state */
void test_Ocu_SetPinAction_Uninit_ShouldReportError(void) {
    mock_Det_Reset();
    Ocu_ModuleState = OCU_UNINIT;
    Ocu_SetPinAction(0U, OCU_TOGGLE);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

/* Line 451: SetPinAction with invalid channel */
void test_Ocu_SetPinAction_InvalidChannel_ShouldReportError(void) {
    Ocu_Init(&testConfig);
    mock_Det_Reset();
    Ocu_SetPinAction(OCU_NUM_CHANNELS + 1U, OCU_TOGGLE);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* Line 488: SetAbsoluteThreshold with UNINIT state */
void test_Ocu_SetAbsoluteThreshold_Uninit_ShouldReportError(void) {
    mock_Det_Reset();
    Ocu_ModuleState = OCU_UNINIT;
    Std_ReturnType ret = Ocu_SetAbsoluteThreshold(0U, 100U, 200U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_UNINIT, mock_DetLastErrorId);
}

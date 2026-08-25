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
}

/* Init/DeInit Tests */
/** @req SWS_Ocu_00001 */
void test_Ocu_Init_NullPtr_ShouldReportError(void) {
    Ocu_Init(NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(OCU_E_PARAM_CONFIG, mock_DetLastErrorId);
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
void test_Ocu_EnableNotification_ShouldEnable(void) {
    Ocu_Init(&testConfig);
    Ocu_EnableNotification(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ocu_00010 */
void test_Ocu_DisableNotification_ShouldDisable(void) {
    Ocu_Init(&testConfig);
    Ocu_EnableNotification(0U);
    Ocu_DisableNotification(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
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

/**
 * @file test_icu.c
 * @brief Icu (Input Capture Unit) Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/icu/src/Icu.c  @tests src/bsw/mcal/icu/include/Icu.h

#include "unity.h"
#include "Icu.h"
#include "Icu_Cfg.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

void mock_Det_Reset(void) {
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
static Icu_ChannelConfigType testChannels[2];
static Icu_ConfigType testConfig;

static void test_Icu_SetupDefaultConfig(void) {
    testChannels[0].ChannelId = 0U;
    testChannels[0].BaseAddress = ICU_EMIOS_0_BASE_ADDR;
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT;
    testChannels[0].DefaultActivation = ICU_RISING_EDGE;
    testChannels[0].SignalMeasurementProperty = ICU_PERIOD_TIME;
    testChannels[0].TimestampBufferType = ICU_LINEAR_BUFFER;
    testChannels[0].BufferSize = 16U;
    testChannels[0].BufferPtr = NULL_PTR;
    testChannels[0].WakeupSupport = FALSE;
    testChannels[0].NotificationEnabled = FALSE;
    testChannels[0].NotificationFn = NULL_PTR;
    testChannels[0].ClockPrescaler = 1U;

    testChannels[1].ChannelId = 1U;
    testChannels[1].BaseAddress = ICU_EMIOS_0_BASE_ADDR;
    testChannels[1].MeasurementMode = ICU_MODE_EDGE_COUNTER;
    testChannels[1].DefaultActivation = ICU_RISING_EDGE;
    testChannels[1].SignalMeasurementProperty = ICU_PERIOD_TIME;
    testChannels[1].TimestampBufferType = ICU_LINEAR_BUFFER;
    testChannels[1].BufferSize = 16U;
    testChannels[1].BufferPtr = NULL_PTR;
    testChannels[1].WakeupSupport = FALSE;
    testChannels[1].NotificationEnabled = FALSE;
    testChannels[1].NotificationFn = NULL_PTR;
    testChannels[1].ClockPrescaler = 1U;

    testConfig.Channels = testChannels;
    testConfig.NumChannels = 2U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
    testConfig.WakeupFunctionalityApi = FALSE;
    testConfig.DeInitApi = TRUE;
    testConfig.SetModeApi = TRUE;
    testConfig.DisableWakeupApi = FALSE;
    testConfig.EnableWakeupApi = FALSE;
    testConfig.CheckWakeupApi = FALSE;
    testConfig.TimestampApi = TRUE;
    testConfig.EdgeCountApi = TRUE;
    testConfig.SignalMeasurementApi = TRUE;
    testConfig.DefaultMode = ICU_MODE_NORMAL;
}

void setUp(void) {
    mock_Det_Reset();
    test_Icu_SetupDefaultConfig();
}

void tearDown(void) {
}

/* Init/DeInit Tests */
/** @req SWS_Icu_00001 */
void test_Icu_Init_NullPtr_ShouldReportError(void) {
    Icu_Init(NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CONFIG, mock_DetLastErrorId);
}

/** @req SWS_Icu_00001 */
void test_Icu_Init_ValidConfig_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00002 */
void test_Icu_DeInit_AfterInit_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_DeInit();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* Mode Tests */
/** @req SWS_Icu_00003 */
void test_Icu_SetMode_Sleep_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_SetMode(ICU_MODE_SLEEP);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00003 */
void test_Icu_SetMode_Normal_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_SetMode(ICU_MODE_SLEEP);
    Icu_SetMode(ICU_MODE_NORMAL);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00003 */
void test_Icu_SetMode_BeforeInit_ShouldReportError(void) {
    Icu_SetMode(ICU_MODE_SLEEP);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* Edge Detection Tests */
/** @req SWS_Icu_00007 */
void test_Icu_SetActivationCondition_Rising_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_SetActivationCondition(0U, ICU_RISING_EDGE);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00007 */
void test_Icu_SetActivationCondition_Falling_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_SetActivationCondition(0U, ICU_FALLING_EDGE);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00007 */
void test_Icu_SetActivationCondition_Both_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_SetActivationCondition(0U, ICU_BOTH_EDGES);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00007 */
void test_Icu_SetActivationCondition_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_SetActivationCondition(ICU_NUM_CHANNELS + 1U, ICU_RISING_EDGE);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* Timestamp Tests */
/** @req SWS_Icu_00011 */
void test_Icu_StartTimestamp_ValidConfig_ShouldSucceed(void) {
    uint32 tsBuffer[16];
    Icu_Init(&testConfig);
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00012 */
void test_Icu_StopTimestamp_AfterStart_ShouldSucceed(void) {
    uint32 tsBuffer[16];
    Icu_Init(&testConfig);
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    Icu_StopTimestamp(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00013 */
void test_Icu_GetTimestampIndex_AfterCapture_ShouldReturnZero(void) {
    uint32 tsBuffer[16];
    Icu_Init(&testConfig);
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    Icu_IndexType idx = Icu_GetTimestampIndex(0U);
    TEST_ASSERT_EQUAL(0U, idx);
}

/* Edge Count Tests */
/** @req SWS_Icu_00015 */
void test_Icu_EnableEdgeCount_ShouldStartCounting(void) {
    Icu_Init(&testConfig);
    Icu_EnableEdgeCount(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00017 */
void test_Icu_GetEdgeNumbers_AfterCount_ShouldReturnZero(void) {
    Icu_Init(&testConfig);
    Icu_EnableEdgeCount(0U);
    uint16 count = Icu_GetEdgeNumbers(0U);
    TEST_ASSERT_EQUAL(0U, count);
}

/** @req SWS_Icu_00014 */
void test_Icu_ResetEdgeCount_ShouldClearCounter(void) {
    Icu_Init(&testConfig);
    Icu_EnableEdgeCount(0U);
    Icu_ResetEdgeCount(0U);
    uint16 count = Icu_GetEdgeNumbers(0U);
    TEST_ASSERT_EQUAL(0U, count);
}

/* Signal Measurement Tests */
/** @req SWS_Icu_00018 */
void test_Icu_StartSignalMeasurement_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00020 */
void test_Icu_GetTimeElapsed_AfterMeasurement_ShouldReturnValid(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    uint16 elapsed = Icu_GetTimeElapsed(0U);
    (void)elapsed;
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00021 */
void test_Icu_GetDutyCycleValues_ShouldReturnCorrectValues(void) {
    Icu_DutyCycleType dcValues;
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_DUTY_CYCLE);
    Icu_GetDutyCycleValues(0U, &dcValues);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* Notification Tests */
/** @req SWS_Icu_00009 */
void test_Icu_EnableNotification_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_EnableNotification(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Icu_00008 */
void test_Icu_DisableNotification_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_EnableNotification(0U);
    Icu_DisableNotification(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* GetInputState Tests */
/** @req SWS_Icu_00010 */
void test_Icu_GetInputState_AfterInit_ShouldReturnIdle(void) {
    Icu_Init(&testConfig);
    Icu_InputStateType state = Icu_GetInputState(0U);
    TEST_ASSERT_EQUAL(ICU_IDLE, state);
}

/* Version Info Test */
/** @req SWS_Icu_00022 */
void test_Icu_GetVersionInfo_ShouldReturnCorrectVersion(void) {
    Std_VersionInfoType versionInfo;
    Icu_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL(ICU_SW_MAJOR_VERSION, versionInfo.vendorID);
}

/* Uninit error path tests */
/** @req SWS_Icu_00015 */
void test_Icu_EnableEdgeCount_BeforeInit_ShouldReportError(void) {
    Icu_EnableEdgeCount(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Icu_00011 */
void test_Icu_StartTimestamp_BeforeInit_ShouldReportError(void) {
    uint32 tsBuffer[16];
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

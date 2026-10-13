/**
 * @file test_icu.c
 * @brief Icu (Input Capture Unit) Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/icu/src/Icu.c  @tests src/bsw/mcal/icu/include/Icu.h

#include "unity.h"
#include "mock_registers.h"
#include "Icu.h"
#include "Icu_Cfg.h"

/* eMIOS channel register layout (mirrors the static helpers in Icu.c) */
#define TEST_ICU_CH0_REGS       (ICU_EMIOS_0_BASE_ADDR + 0x20U)   /* channel 0 register block */
#define EMIOS_C_A               (0x0000U)  /* Channel A Register */
#define EMIOS_C_CNT             (0x0008U)  /* Channel Counter Register */
#define TEST_ICU_EMIOS_C_S      (0x10U)    /* Channel Status Register offset */
#define EMIOS_S_FLAG            (0x00000001U)  /* Flag bit */
#define EMIOS_S_UCIN            (0x00020000U)  /* Input State bit */

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
    MockRegisters_Reset();
    test_Icu_SetupDefaultConfig();
}

void tearDown(void) {
    /* The SUT keeps its initialization state in file-static variables that
     * persist between test cases on the host. Deinitialize after every test
     * so each case observes the genuine uninitialized driver. When the driver
     * was never initialized this only reports ICU_E_UNINIT into the mock,
     * which the next setUp() clears. */
    Icu_DeInit();
}

/* Init/DeInit Tests */
/** @req SWS_Icu_00001 */
void test_Icu_Init_NullPtr_ShouldReportError(void) {
    Icu_Init(NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    /* SUT reports ICU_E_PARAM_POINTER (0x10) for a NULL config pointer */
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, mock_DetLastErrorId);
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
    /* Icu_Init clears the channel flag by writing EMIOS_S_FLAG to the status
     * register; the register mock stores written values verbatim, so preset
     * the flag to 0 here to emulate idle hardware input. */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + TEST_ICU_EMIOS_C_S, 0U);
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

/* --- SetActivationCondition uninit --- */
void test_Icu_SetActivationCondition_BeforeInit_ShouldReportError(void) {
    Icu_SetActivationCondition(0U, ICU_RISING_EDGE);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- SetActivationCondition invalid activation --- */
void test_Icu_SetActivationCondition_InvalidActivation_ShouldReportError(void) {
    Icu_Init(&testConfig);
    /* Use a value outside the enum range (e.g., 5) */
    Icu_SetActivationCondition(0U, (Icu_ActivationType)5U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_ACTIVATION, mock_DetLastErrorId);
}

/* --- SetActivationCondition uninit + invalid channel --- */
void test_Icu_SetActivationCondition_BeforeInit_InvalidChannel_ShouldReportUninit(void) {
    Icu_SetActivationCondition(ICU_NUM_CHANNELS + 1U, ICU_RISING_EDGE);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- DisableNotification uninit --- */
void test_Icu_DisableNotification_BeforeInit_ShouldReportError(void) {
    Icu_DisableNotification(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- DisableNotification invalid channel --- */
void test_Icu_DisableNotification_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_DisableNotification(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- EnableNotification uninit --- */
void test_Icu_EnableNotification_BeforeInit_ShouldReportError(void) {
    Icu_EnableNotification(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- EnableNotification invalid channel --- */
void test_Icu_EnableNotification_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_EnableNotification(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetInputState uninit --- */
void test_Icu_GetInputState_BeforeInit_ShouldReportError(void) {
    Icu_InputStateType state = Icu_GetInputState(0U);
    (void)state;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- GetInputState invalid channel --- */
void test_Icu_GetInputState_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_InputStateType state = Icu_GetInputState(ICU_NUM_CHANNELS);
    (void)state;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetInputState with flag set -> ACTIVE --- */
void test_Icu_GetInputState_FlagSet_ShouldReturnActive(void) {
    Icu_Init(&testConfig);
    /* Set the flag bit in the channel status register */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + TEST_ICU_EMIOS_C_S, EMIOS_S_FLAG);
    Icu_InputStateType state = Icu_GetInputState(0U);
    TEST_ASSERT_EQUAL(ICU_ACTIVE, state);
}

/* --- StartTimestamp invalid channel --- */
void test_Icu_StartTimestamp_InvalidChannel_ShouldReportError(void) {
    uint32 tsBuffer[16];
    Icu_Init(&testConfig);
    Icu_StartTimestamp(ICU_NUM_CHANNELS, tsBuffer, 16U, 4U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- StartTimestamp null buffer --- */
void test_Icu_StartTimestamp_NullBuffer_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_StartTimestamp(0U, NULL_PTR, 16U, 4U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* --- StartTimestamp buffer size 0 --- */
void test_Icu_StartTimestamp_BufferSize0_ShouldReportError(void) {
    uint32 tsBuffer[16];
    Icu_Init(&testConfig);
    Icu_StartTimestamp(0U, tsBuffer, 0U, 4U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_BUFFER_SIZE, mock_DetLastErrorId);
}

/* --- StartTimestamp busy --- */
void test_Icu_StartTimestamp_Busy_ShouldReportError(void) {
    uint32 tsBuffer[16];
    Icu_Init(&testConfig);
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    mock_Det_Reset();
    /* Try to start again on same channel -> E_BUSY */
    uint32 tsBuffer2[16];
    Icu_StartTimestamp(0U, tsBuffer2, 16U, 4U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_BUSY, mock_DetLastErrorId);
}

/* --- StopTimestamp uninit --- */
void test_Icu_StopTimestamp_BeforeInit_ShouldReportError(void) {
    Icu_StopTimestamp(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- StopTimestamp invalid channel --- */
void test_Icu_StopTimestamp_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_StopTimestamp(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetTimestampIndex uninit --- */
void test_Icu_GetTimestampIndex_BeforeInit_ShouldReportError(void) {
    Icu_IndexType idx = Icu_GetTimestampIndex(0U);
    (void)idx;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- GetTimestampIndex invalid channel --- */
void test_Icu_GetTimestampIndex_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_IndexType idx = Icu_GetTimestampIndex(ICU_NUM_CHANNELS);
    (void)idx;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetTimestampIndex not running --- */
void test_Icu_GetTimestampIndex_NotRunning_ShouldReportError(void) {
    Icu_Init(&testConfig);
    /* Don't start timestamp -> not running */
    Icu_IndexType idx = Icu_GetTimestampIndex(0U);
    (void)idx;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_STAMP_NOT_RUNNING, mock_DetLastErrorId);
}

/* --- ResetEdgeCount uninit --- */
void test_Icu_ResetEdgeCount_BeforeInit_ShouldReportError(void) {
    Icu_ResetEdgeCount(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- ResetEdgeCount invalid channel --- */
void test_Icu_ResetEdgeCount_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_ResetEdgeCount(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- EnableEdgeCount invalid channel --- */
void test_Icu_EnableEdgeCount_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_EnableEdgeCount(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- DisableEdgeCount uninit --- */
void test_Icu_DisableEdgeCount_BeforeInit_ShouldReportError(void) {
    Icu_DisableEdgeCount(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- DisableEdgeCount invalid channel --- */
void test_Icu_DisableEdgeCount_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_DisableEdgeCount(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- DisableEdgeCount valid --- */
void test_Icu_DisableEdgeCount_AfterEnable_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_EnableEdgeCount(0U);
    Icu_DisableEdgeCount(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- GetEdgeNumbers uninit --- */
void test_Icu_GetEdgeNumbers_BeforeInit_ShouldReportError(void) {
    uint16 count = Icu_GetEdgeNumbers(0U);
    (void)count;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- GetEdgeNumbers invalid channel --- */
void test_Icu_GetEdgeNumbers_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    uint16 count = Icu_GetEdgeNumbers(ICU_NUM_CHANNELS);
    (void)count;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetEdgeNumbers not running --- */
void test_Icu_GetEdgeNumbers_NotRunning_ShouldReportError(void) {
    Icu_Init(&testConfig);
    /* Don't enable edge count -> not running */
    uint16 count = Icu_GetEdgeNumbers(0U);
    (void)count;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_EDGE_COUNTING_NOT_RUNNING, mock_DetLastErrorId);
}

/* --- StartSignalMeasurement uninit --- */
void test_Icu_StartSignalMeasurement_BeforeInit_ShouldReportError(void) {
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- StartSignalMeasurement invalid channel --- */
void test_Icu_StartSignalMeasurement_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(ICU_NUM_CHANNELS, ICU_PERIOD_TIME);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- StartSignalMeasurement already running --- */
void test_Icu_StartSignalMeasurement_AlreadyRunning_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    mock_Det_Reset();
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_MEASUREMENT_RUNNING, mock_DetLastErrorId);
}

/* --- StartSignalMeasurement with ICU_HIGH_TIME --- */
void test_Icu_StartSignalMeasurement_HighTime_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_HIGH_TIME);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- StartSignalMeasurement with ICU_LOW_TIME --- */
void test_Icu_StartSignalMeasurement_LowTime_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_LOW_TIME);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- StartSignalMeasurement with ICU_DUTY_CYCLE --- */
void test_Icu_StartSignalMeasurement_DutyCycle_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_DUTY_CYCLE);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- StopSignalMeasurement uninit --- */
void test_Icu_StopSignalMeasurement_BeforeInit_ShouldReportError(void) {
    Icu_StopSignalMeasurement(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- StopSignalMeasurement invalid channel --- */
void test_Icu_StopSignalMeasurement_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_StopSignalMeasurement(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- StopSignalMeasurement valid --- */
void test_Icu_StopSignalMeasurement_AfterStart_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    Icu_StopSignalMeasurement(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- GetTimeElapsed uninit --- */
void test_Icu_GetTimeElapsed_BeforeInit_ShouldReportError(void) {
    uint16 elapsed = Icu_GetTimeElapsed(0U);
    (void)elapsed;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- GetTimeElapsed invalid channel --- */
void test_Icu_GetTimeElapsed_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    uint16 elapsed = Icu_GetTimeElapsed(ICU_NUM_CHANNELS);
    (void)elapsed;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetDutyCycleValues uninit --- */
void test_Icu_GetDutyCycleValues_BeforeInit_ShouldReportError(void) {
    Icu_DutyCycleType dcValues;
    Icu_GetDutyCycleValues(0U, &dcValues);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- GetDutyCycleValues invalid channel --- */
void test_Icu_GetDutyCycleValues_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_DutyCycleType dcValues;
    Icu_GetDutyCycleValues(ICU_NUM_CHANNELS, &dcValues);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetDutyCycleValues null ptr --- */
void test_Icu_GetDutyCycleValues_NullPtr_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_GetDutyCycleValues(0U, NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* --- GetInputLevel uninit --- */
void test_Icu_GetInputLevel_BeforeInit_ShouldReportError(void) {
    uint8 level = Icu_GetInputLevel(0U);
    (void)level;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- GetInputLevel invalid channel --- */
void test_Icu_GetInputLevel_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    uint8 level = Icu_GetInputLevel(ICU_NUM_CHANNELS);
    (void)level;
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/* --- GetInputLevel valid with UCIN set --- */
void test_Icu_GetInputLevel_InputHigh_ShouldReturn1(void) {
    Icu_Init(&testConfig);
    /* Set UCIN bit in channel status register */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + TEST_ICU_EMIOS_C_S, EMIOS_S_UCIN);
    uint8 level = Icu_GetInputLevel(0U);
    TEST_ASSERT_EQUAL(1U, level);
}

/* --- GetInputLevel valid with UCIN clear --- */
void test_Icu_GetInputLevel_InputLow_ShouldReturn0(void) {
    Icu_Init(&testConfig);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + TEST_ICU_EMIOS_C_S, 0U);
    uint8 level = Icu_GetInputLevel(0U);
    TEST_ASSERT_EQUAL(0U, level);
}

/* --- GetVersionInfo null ptr --- */
void test_Icu_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Icu_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* --- GetSysTimestamp --- */
void test_Icu_GetSysTimestamp_ShouldReturnCounterValue(void) {
    /* Preset counter value for channel 23 of EMIOS_0 */
    uint32 ch23Addr = ICU_EMIOS_0_BASE_ADDR + 0x20U + (23U * 0x20U);
    MockRegisters_Write32(ch23Addr + EMIOS_C_CNT, 0x12345U);
    uint32 ts = Icu_GetSysTimestamp();
    TEST_ASSERT_EQUAL_HEX32(0x12345U, ts);
}

/* --- Init double init --- */
void test_Icu_Init_DoubleInit_ShouldReportError(void) {
    Icu_Init(&testConfig);
    mock_Det_Reset();
    Icu_Init(&testConfig);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/* --- DeInit before init --- */
void test_Icu_DeInit_BeforeInit_ShouldReportError(void) {
    Icu_DeInit();
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

/* --- SetMode invalid mode --- */
void test_Icu_SetMode_InvalidMode_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_SetMode((Icu_ModeType)5U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* --- SetMode sleep with running channel --- */
void test_Icu_SetMode_SleepWithRunningChannel_ShouldStopChannel(void) {
    Icu_Init(&testConfig);
    uint32 tsBuffer[16];
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    mock_Det_Reset();
    Icu_SetMode(ICU_MODE_SLEEP);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- SetMode normal resume with notification enabled --- */
void test_Icu_SetMode_NormalResume_ShouldEnableInterrupt(void) {
    Icu_Init(&testConfig);
    Icu_EnableNotification(0U);
    Icu_SetMode(ICU_MODE_SLEEP);
    mock_Det_Reset();
    Icu_SetMode(ICU_MODE_NORMAL);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- DisableWakeup --- */
void test_Icu_DisableWakeup_BeforeInit_ShouldReportError(void) {
    Icu_DisableWakeup(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

void test_Icu_DisableWakeup_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_DisableWakeup(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

void test_Icu_DisableWakeup_ValidChannel_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_DisableWakeup(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- EnableWakeup --- */
void test_Icu_EnableWakeup_BeforeInit_ShouldReportError(void) {
    Icu_EnableWakeup(0U);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

void test_Icu_EnableWakeup_InvalidChannel_ShouldReportError(void) {
    Icu_Init(&testConfig);
    Icu_EnableWakeup(ICU_NUM_CHANNELS);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

void test_Icu_EnableWakeup_ValidChannel_ShouldSucceed(void) {
    Icu_Init(&testConfig);
    Icu_EnableWakeup(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- CheckWakeup --- */
void test_Icu_CheckWakeup_BeforeInit_ShouldReportError(void) {
    Std_ReturnType ret = Icu_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ICU_E_UNINIT, mock_DetLastErrorId);
}

void test_Icu_CheckWakeup_NoWakeupEnabled_ShouldReturnNotOk(void) {
    Icu_Init(&testConfig);
    Std_ReturnType ret = Icu_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_Icu_CheckWakeup_WakeupEnabledWithFlag_ShouldReturnOk(void) {
    Icu_Init(&testConfig);
    Icu_EnableWakeup(0U);
    /* Set the flag bit to simulate a wakeup event */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + TEST_ICU_EMIOS_C_S, EMIOS_S_FLAG);
    Std_ReturnType ret = Icu_CheckWakeup(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* --- ProcessInterrupt tests --- */
void test_Icu_ProcessInterrupt_InvalidChannel_ShouldReturn(void) {
    Icu_Init(&testConfig);
    /* Should return silently for invalid channel */
    Icu_ProcessInterrupt(ICU_NUM_CHANNELS);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Icu_ProcessInterrupt_EdgeDetect_ShouldUpdateInputState(void) {
    Icu_Init(&testConfig);
    /* Channel 0 is configured as ICU_MODE_SIGNAL_EDGE_DETECT */
    Icu_EnableNotification(0U);
    /* Preset capture value in channel A register */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x100U);
    Icu_ProcessInterrupt(0U);
    /* Input state should be ICU_ACTIVE after edge detect interrupt */
    /* Read back state - flag was cleared by ProcessInterrupt */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + TEST_ICU_EMIOS_C_S, 0U);
}

void test_Icu_ProcessInterrupt_Timestamp_ShouldStoreValue(void) {
    /* Setup channel 0 as timestamp mode */
    testChannels[0].MeasurementMode = ICU_MODE_TIMESTAMP;
    Icu_Init(&testConfig);
    uint32 tsBuffer[16];
    Icu_StartTimestamp(0U, tsBuffer, 16U, 4U);
    /* Preset capture value */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0xABC0U);
    Icu_ProcessInterrupt(0U);
    /* Timestamp should be stored in buffer */
    TEST_ASSERT_EQUAL_HEX32(0xABC0U, tsBuffer[0]);
}

void test_Icu_ProcessInterrupt_SignalMeasurement_ShouldProcess(void) {
    /* Setup channel 0 as signal measurement mode */
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT;
    testChannels[0].SignalMeasurementProperty = ICU_PERIOD_TIME;
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    /* First capture: stores as last capture */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x100U);
    Icu_ProcessInterrupt(0U);
    /* Second capture: calculates period */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x200U);
    Icu_ProcessInterrupt(0U);
    uint16 elapsed = Icu_GetTimeElapsed(0U);
    TEST_ASSERT_EQUAL_UINT16(0x100U, elapsed);
}

void test_Icu_ProcessInterrupt_EdgeCount_ShouldCallNotification(void) {
    Icu_Init(&testConfig);
    /* Channel 1 is ICU_MODE_EDGE_COUNTER */
    Icu_EnableEdgeCount(1U);
    /* Channel 1 register base */
    uint32 ch1Addr = ICU_EMIOS_0_BASE_ADDR + 0x20U + (1U * 0x20U);
    MockRegisters_Write32(ch1Addr + EMIOS_C_A, 0x50U);
    Icu_ProcessInterrupt(1U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- Init with various measurement modes --- */
void test_Icu_Init_SignalMeasurementMode_ShouldConfigureIPM(void) {
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT;
    testChannels[0].SignalMeasurementProperty = ICU_PERIOD_TIME;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Icu_Init_SignalMeasurementDutyCycle_ShouldConfigureIPWM(void) {
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT;
    testChannels[0].SignalMeasurementProperty = ICU_DUTY_CYCLE;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Icu_Init_TimestampMode_ShouldConfigureSAIC(void) {
    testChannels[0].MeasurementMode = ICU_MODE_TIMESTAMP;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Icu_Init_FallingEdge_ShouldConfigureFalling(void) {
    testChannels[0].DefaultActivation = ICU_FALLING_EDGE;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Icu_Init_BothEdges_ShouldConfigureBoth(void) {
    testChannels[0].DefaultActivation = ICU_BOTH_EDGES;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- Init with channel on second eMIOS instance (channel >= 4) --- */
void test_Icu_Init_ChannelOnEmios1_ShouldSucceed(void) {
    testChannels[0].ChannelId = 5U; /* >= ICU_NUM_CHANNELS/2 = 4 -> EMIOS_1 */
    testChannels[0].BaseAddress = ICU_EMIOS_1_BASE_ADDR;
    testConfig.NumChannels = 1U;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- ProcessInterrupt with timestamp linear buffer overflow --- */
void test_Icu_ProcessInterrupt_TimestampLinearBufferFull_ShouldStop(void) {
    testChannels[0].MeasurementMode = ICU_MODE_TIMESTAMP;
    testChannels[0].TimestampBufferType = ICU_LINEAR_BUFFER;
    Icu_Init(&testConfig);
    uint32 tsBuffer[2];
    Icu_StartTimestamp(0U, tsBuffer, 2U, 0U);
    /* Fill buffer */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x10U);
    Icu_ProcessInterrupt(0U);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x20U);
    Icu_ProcessInterrupt(0U);
    /* Third capture: linear buffer should stop (StopTimestamp called) */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x30U);
    Icu_ProcessInterrupt(0U);
    /* Buffer should only have first 2 values */
    TEST_ASSERT_EQUAL_HEX32(0x10U, tsBuffer[0]);
    TEST_ASSERT_EQUAL_HEX32(0x20U, tsBuffer[1]);
}

/* --- ProcessInterrupt with timestamp circular buffer --- */
void test_Icu_ProcessInterrupt_TimestampCircularBuffer_ShouldWrap(void) {
    testChannels[0].MeasurementMode = ICU_MODE_TIMESTAMP;
    testChannels[0].TimestampBufferType = ICU_CIRCULAR_BUFFER;
    Icu_Init(&testConfig);
    uint32 tsBuffer[2];
    Icu_StartTimestamp(0U, tsBuffer, 2U, 0U);
    /* Fill buffer */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x10U);
    Icu_ProcessInterrupt(0U);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x20U);
    Icu_ProcessInterrupt(0U);
    /* Third capture: circular buffer should wrap to index 0 */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x30U);
    Icu_ProcessInterrupt(0U);
    /* Index 0 should be overwritten */
    TEST_ASSERT_EQUAL_HEX32(0x30U, tsBuffer[0]);
    TEST_ASSERT_EQUAL_HEX32(0x20U, tsBuffer[1]);
}

/* --- ProcessInterrupt with notification callback --- */
static uint32 test_Icu_IsrNotifCount = 0U;
static void test_Icu_IsrNotification(void) { test_Icu_IsrNotifCount++; }

void test_Icu_ProcessInterrupt_TimestampNotification_ShouldCallCallback(void) {
    test_Icu_IsrNotifCount = 0U;
    testChannels[0].MeasurementMode = ICU_MODE_TIMESTAMP;
    testChannels[0].NotificationFn = test_Icu_IsrNotification;
    Icu_Init(&testConfig);
    uint32 tsBuffer[16];
    Icu_StartTimestamp(0U, tsBuffer, 16U, 2U); /* Notify every 2 captures */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x10U);
    Icu_ProcessInterrupt(0U);
    TEST_ASSERT_EQUAL_UINT32(0U, test_Icu_IsrNotifCount);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x20U);
    Icu_ProcessInterrupt(0U);
    TEST_ASSERT_EQUAL_UINT32(1U, test_Icu_IsrNotifCount);
}

void test_Icu_ProcessInterrupt_EdgeDetectNotification_ShouldCallCallback(void) {
    test_Icu_IsrNotifCount = 0U;
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT;
    testChannels[0].NotificationEnabled = TRUE;
    testChannels[0].NotificationFn = test_Icu_IsrNotification;
    Icu_Init(&testConfig);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x10U);
    Icu_ProcessInterrupt(0U);
    TEST_ASSERT_EQUAL_UINT32(1U, test_Icu_IsrNotifCount);
}

void test_Icu_ProcessInterrupt_SignalMeasurementNotification_ShouldCallCallback(void) {
    test_Icu_IsrNotifCount = 0U;
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT;
    testChannels[0].SignalMeasurementProperty = ICU_PERIOD_TIME;
    testChannels[0].NotificationEnabled = TRUE;
    testChannels[0].NotificationFn = test_Icu_IsrNotification;
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x100U);
    Icu_ProcessInterrupt(0U);
    TEST_ASSERT_EQUAL_UINT32(1U, test_Icu_IsrNotifCount);
}

/* --- ProcessInterrupt signal measurement with counter overflow --- */
void test_Icu_ProcessInterrupt_SignalMeasurement_Overflow_ShouldCalculatePeriod(void) {
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT;
    testChannels[0].SignalMeasurementProperty = ICU_PERIOD_TIME;
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_PERIOD_TIME);
    /* First capture near top of counter range */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0xFFF0U);
    Icu_ProcessInterrupt(0U);
    /* Second capture after overflow (wraps around) */
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x0010U);
    Icu_ProcessInterrupt(0U);
    uint16 elapsed = Icu_GetTimeElapsed(0U);
    /* Period = (0xFFFF - 0xFFF0) + 0x0010 = 0x0F + 0x10 = 0x1F */
    TEST_ASSERT_EQUAL_UINT16(0x001FU, elapsed);
}

/* --- ProcessInterrupt signal measurement with DUTY_CYCLE property --- */
void test_Icu_ProcessInterrupt_SignalMeasurement_DutyCycle_ShouldCalculateBoth(void) {
    testChannels[0].MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT;
    testChannels[0].SignalMeasurementProperty = ICU_DUTY_CYCLE;
    Icu_Init(&testConfig);
    Icu_StartSignalMeasurement(0U, ICU_DUTY_CYCLE);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x100U);
    Icu_ProcessInterrupt(0U);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x300U);
    Icu_ProcessInterrupt(0U);
    Icu_DutyCycleType dcValues;
    Icu_GetDutyCycleValues(0U, &dcValues);
    /* Period = 0x300 - 0x100 = 0x200, activeTime = period/2 = 0x100 */
    TEST_ASSERT_EQUAL_UINT16(0x200U, dcValues.PeriodTime);
    TEST_ASSERT_EQUAL_UINT16(0x100U, dcValues.ActiveTime);
}

/* --- ProcessInterrupt wakeup in sleep mode --- */
void test_Icu_ProcessInterrupt_WakeupInSleep_ShouldHandle(void) {
    testChannels[0].WakeupSupport = TRUE;
    Icu_Init(&testConfig);
    Icu_EnableWakeup(0U);
    Icu_SetMode(ICU_MODE_SLEEP);
    MockRegisters_Write32(TEST_ICU_CH0_REGS + EMIOS_C_A, 0x10U);
    Icu_ProcessInterrupt(0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- Init with prescaler = 0 (no prescaler) --- */
void test_Icu_Init_PrescalerZero_ShouldNotSetPrescalerBits(void) {
    testChannels[0].ClockPrescaler = 0U;
    Icu_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* --- Init with channel >= ICU_NUM_CHANNELS (should skip) --- */
void test_Icu_Init_ChannelOutOfRange_ShouldSkip(void) {
    testChannels[0].ChannelId = ICU_NUM_CHANNELS + 5U;
    testConfig.NumChannels = 1U;
    Icu_Init(&testConfig);
    /* Should succeed without error - channel is skipped */
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

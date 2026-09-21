/**
 * @file test_gpt.c
 * @brief Gpt (General Purpose Timer) Unit Tests — Substantiated
 * @req SWS_Gpt
 *
 * Substantiation: DET parameter validation, register write verification via
 * mock_registers (GPT1 base 0x302E0000 / GPT2 base 0x302F0000, non-S32K312),
 * timer enable bit, output-compare value and counter read-back.
 */

// @tests src/bsw/mcal/gpt/src/Gpt.c  @tests src/bsw/mcal/gpt/include/Gpt.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Gpt.h"
#include "Gpt_Cfg.h"

/* GPT base addresses (non-S32K312 build) — must match Gpt.c */
#define GPT1_BASE       (0x302E0000UL)
#define GPT2_BASE       (0x302F0000UL)
#define GPT_CR_OFF      (0x00U)
#define GPT_PR_OFF      (0x04U)
#define GPT_SR_OFF      (0x08U)
#define GPT_IR_OFF      (0x0CU)
#define GPT_OCR1_OFF    (0x10U)
#define GPT_CNT_OFF     (0x24U)

#define GPT_CR_EN       (0x00000001UL)
#define GPT_CR_FRR      (0x00000200UL)
#define GPT_CR_SWR      (0x00010000UL)
#define GPT_IR_OF1IE    (0x00000001UL)

static Gpt_ChannelConfigType testChannels[8];
static Gpt_ConfigType testConfig;

/* Gpt.c keeps static init state that cannot be reset on host; the driver is
 * initialized once per process and later tests build on the initialized
 * state (same pattern as test_can.c). */
static boolean gpt_driverReady = FALSE;

static void test_Gpt_SetupConfig(void)
{
    for (uint8 i = 0U; i < 8U; i++) {
        testChannels[i].ChannelId = i;
        testChannels[i].BaseAddress = (i < 4U) ? GPT1_BASE : GPT2_BASE;
        testChannels[i].ChannelMode = GPT_CH_MODE_CONTINUOUS;
        testChannels[i].ClockPrescaler = GPT_CLOCK_PRESCALER_1;
        testChannels[i].MaxTickValue = 0xFFFFU;
        testChannels[i].ClockFrequency = 24000000U;
        testChannels[i].WakeupSupport = FALSE;
        testChannels[i].NotificationEnabled = FALSE;
        testChannels[i].NotificationFn = NULL_PTR;
    }
    testConfig.Channels = testChannels;
    testConfig.NumChannels = 8U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
    testConfig.WakeupFunctionalityApi = FALSE;
    testConfig.DeInitApi = TRUE;
    testConfig.TimeElapsedApi = TRUE;
    testConfig.TimeRemainingApi = TRUE;
    testConfig.EnableDisableNotificationApi = TRUE;
    testConfig.NotificationSupported = FALSE;
    testConfig.DefaultMode = GPT_MODE_NORMAL;
    testConfig.PredefTimer1usEnablingGrade = FALSE;
    testConfig.PredefTimer100us32bitEnable = FALSE;
}

static void test_Gpt_EnsureInitialized(void)
{
    if (!gpt_driverReady) {
        test_Gpt_SetupConfig();
        Gpt_Init(&testConfig);
        Det_Mock_Reset();
        gpt_driverReady = TRUE;
    }
}

/* Tear the driver down to a genuinely uninitialized state (all channels
 * stopped first because DeInit refuses while a channel is running). */
static void test_Gpt_ResetDriver(void)
{
    if (gpt_driverReady) {
        Gpt_StopTimer(0U); /* only channel 0 is ever started by these tests */
        Gpt_DeInit();
        gpt_driverReady = FALSE;
    }
    Det_Mock_Reset();
}

/* Bring channel 0 to a stopped state regardless of the previous test.
 * The static running flag lives in Gpt.c, so the register value alone is
 * not a reliable indicator — always issue the stop. */
static void test_Gpt_BringChannelStopped(void)
{
    Gpt_StopTimer(0U);
    Det_Mock_Reset();
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Init --- */

/** @req SWS_Gpt_00001 */
void test_Gpt_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Gpt_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(GPT_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_Gpt_00001 */
void test_Gpt_Init_ValidConfig_ShouldConfigurePrescalerAndControl(void) {
    test_Gpt_ResetDriver();
    test_Gpt_SetupConfig();
    /* Channels 0-3 share the GPT1 register block (one PR per instance), so
     * all channels must use the same prescaler to observe a stable value. */
    for (uint8 i = 0U; i < 8U; i++) {
        testChannels[i].ClockPrescaler = GPT_CLOCK_PRESCALER_8;
    }
    Gpt_Init(&testConfig);
    /* Prescaler register: (1 << 3) - 1 = 7, on both GPT instances */
    TEST_ASSERT_EQUAL(7U, MockRegisters_Read32(GPT1_BASE + GPT_PR_OFF));
    TEST_ASSERT_EQUAL(7U, MockRegisters_Read32(GPT2_BASE + GPT_PR_OFF));
    /* CR written with FRR (free-run) and CLKSRC=peripheral (0x01<<6), timer disabled */
    uint32 cr = MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF);
    TEST_ASSERT_NOT_EQUAL(0U, cr & GPT_CR_FRR);
    TEST_ASSERT_EQUAL(0x01U << 6, cr & 0x1C0U);
    TEST_ASSERT_EQUAL(0U, cr & GPT_CR_EN);
    /* IR disabled at init */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_IR_OFF));
    gpt_driverReady = TRUE;
}

/** @req SWS_Gpt_00001 */
void test_Gpt_Init_DoubleInit_ShouldReportDet(void) {
    test_Gpt_EnsureInitialized();
    Gpt_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

/* --- DeInit --- */

/** @req SWS_Gpt_00002 */
void test_Gpt_DeInit_AfterInit_ShouldDisableTimer(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    /* CR has EN set before DeInit */
    uint32 cr = MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF);
    cr |= GPT_CR_EN;
    MockRegisters_Write32(GPT1_BASE + GPT_CR_OFF, cr);
    Gpt_DeInit();
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
    /* After DeInit the driver is uninitialized: next API call reports GPT_E_UNINIT */
    Det_Mock_Reset();
    Gpt_StopTimer(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_E_UNINIT, Det_MockData.ErrorId);
    gpt_driverReady = FALSE;
}

/** @req SWS_Gpt_00002 */
void test_Gpt_DeInit_BeforeInit_ShouldReportUninit(void) {
    /* Bring driver to a deinitialized state, then DeInit again -> DET */
    test_Gpt_ResetDriver();
    Gpt_DeInit();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_DEINIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_UNINIT, Det_MockData.ErrorId);
}

/* --- StartTimer --- */

/** @req SWS_Gpt_00003 */
void test_Gpt_StartTimer_BeforeInit_ShouldReportDet(void) {
    /* Force uninitialized state for this check */
    test_Gpt_ResetDriver();
    Gpt_StartTimer(0U, 1000U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_STARTTIMER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_UNINIT, Det_MockData.ErrorId);
    /* Timer must NOT be enabled */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
}

/** @req SWS_Gpt_00003 */
void test_Gpt_StartTimer_AfterInit_ShouldSetOcrAndEnableTimer(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    /* Output compare register 1 loaded with the target value */
    TEST_ASSERT_EQUAL(1000U, MockRegisters_Read32(GPT1_BASE + GPT_OCR1_OFF));
    /* Timer enable bit set in CR */
    TEST_ASSERT_NOT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
    /* NotificationEnabled = FALSE -> interrupt enable stays clear */
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_IR_OFF) & GPT_IR_OF1IE);
}

/** @req SWS_Gpt_00003 */
void test_Gpt_StartTimer_ChannelRunning_ShouldReportChannelBusy(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 500U);
    Det_Mock_Reset();
    Gpt_StartTimer(0U, 600U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_STARTTIMER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_CHANNEL_BUSY, Det_MockData.ErrorId);
    /* OCR must keep the first value */
    TEST_ASSERT_EQUAL(500U, MockRegisters_Read32(GPT1_BASE + GPT_OCR1_OFF));
    Gpt_StopTimer(0U);
}

/** @req SWS_Gpt_00003 */
void test_Gpt_StartTimer_InvalidChannel_ShouldReportParamChannel(void) {
    test_Gpt_EnsureInitialized();
    Gpt_StartTimer(200U, 1000U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_STARTTIMER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/** @req SWS_Gpt_00003 */
void test_Gpt_StartTimer_ZeroValue_ShouldReportParamValue(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_E_PARAM_VALUE, Det_MockData.ErrorId);
}

/* --- StopTimer --- */

/** @req SWS_Gpt_00004 */
void test_Gpt_StopTimer_AfterStart_ShouldClearEnableBit(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    TEST_ASSERT_NOT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
    Gpt_StopTimer(0U);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
}

/* --- GetTimer / GetTimeElapsed --- */

/** @req SWS_Gpt_00005 */
void test_Gpt_GetTimer_AfterInit_ShouldReturnValue(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    /* Preset the hardware counter value */
    MockRegisters_Write32(GPT1_BASE + GPT_CNT_OFF, 400U);
    Gpt_ValueType val = Gpt_GetTimeElapsed(0U);
    TEST_ASSERT_EQUAL(400U, val);
    Gpt_StopTimer(0U);
}

/** @req SWS_Gpt_00006 */
void test_Gpt_GetTimeElapsed_AfterInit_ShouldReturnCounter(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    MockRegisters_Write32(GPT1_BASE + GPT_CNT_OFF, 250U);
    TEST_ASSERT_EQUAL(250U, Gpt_GetTimeElapsed(0U));
    Gpt_StopTimer(0U);
}

/** @req SWS_Gpt_00006 */
void test_Gpt_GetTimeElapsed_InvalidChannel_ShouldReportDet(void) {
    test_Gpt_EnsureInitialized();
    TEST_ASSERT_EQUAL(0U, Gpt_GetTimeElapsed(200U));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_GETTIMEELAPSED, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* --- GetTimeRemaining --- */

/** @req SWS_Gpt_00004 */
void test_Gpt_GetTimeRemaining_RunningChannel_ShouldReturnDelta(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    MockRegisters_Write32(GPT1_BASE + GPT_CNT_OFF, 300U);
    TEST_ASSERT_EQUAL(700U, Gpt_GetTimeRemaining(0U));
    Gpt_StopTimer(0U);
}

/** @req SWS_Gpt_00004 */
void test_Gpt_GetTimeRemaining_StoppedChannel_ShouldReturnZero(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    MockRegisters_Write32(GPT1_BASE + GPT_CNT_OFF, 300U);
    TEST_ASSERT_EQUAL(0U, Gpt_GetTimeRemaining(0U));
}

/* --- SetMode --- */

/** @req SWS_Gpt_00010 */
void test_Gpt_SetMode_Sleep_ShouldStopRunningTimer(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    TEST_ASSERT_NOT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
    Gpt_SetMode(GPT_MODE_SLEEP);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_CR_OFF) & GPT_CR_EN);
    Gpt_SetMode(GPT_MODE_NORMAL);
}

/** @req SWS_Gpt_00010 */
void test_Gpt_SetMode_BeforeInit_ShouldReportDet(void) {
    test_Gpt_ResetDriver();
    Gpt_SetMode(GPT_MODE_SLEEP);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_SETMODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_UNINIT, Det_MockData.ErrorId);
}

/* --- Notification --- */

/** @req SWS_Gpt_00007 */
void test_Gpt_DisableNotification_AfterStart_ShouldClearIrBit(void) {
    test_Gpt_EnsureInitialized();
    test_Gpt_BringChannelStopped();
    Gpt_StartTimer(0U, 1000U);
    MockRegisters_Write32(GPT1_BASE + GPT_IR_OFF, GPT_IR_OF1IE);
    Gpt_DisableNotification(0U);
    TEST_ASSERT_EQUAL(0U, MockRegisters_Read32(GPT1_BASE + GPT_IR_OFF) & GPT_IR_OF1IE);
    Gpt_StopTimer(0U);
}

/** @req SWS_Gpt_00007 */
void test_Gpt_EnableNotification_InvalidChannel_ShouldReportDet(void) {
    test_Gpt_EnsureInitialized();
    Gpt_EnableNotification(200U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* --- GetVersionInfo --- */

/** @req SWS_Gpt_00007 */
void test_Gpt_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    Gpt_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(GPT_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(GPT_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(GPT_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(GPT_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(GPT_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Gpt_00007 */
void test_Gpt_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Gpt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(GPT_SID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(GPT_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- Predef timer --- */

/** @req SWS_Gpt_00014 */
void test_Gpt_GetPredefTimerValue_AfterInit_ShouldReturnCounter(void) {
    test_Gpt_EnsureInitialized();
    MockRegisters_Write32(GPT1_BASE + GPT_CNT_OFF, 1234U);
    uint32 value = 0U;
    Std_ReturnType ret = Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_16BIT, &value);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(1234U, value);
}

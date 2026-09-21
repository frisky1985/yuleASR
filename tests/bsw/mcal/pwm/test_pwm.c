/**
 * @file test_pwm.c
 * @brief Pwm (PWM Driver) Unit Tests — Substantiated
 * @req SWS_Pwm
 *
 * Substantiation: register-level verification of channel initialization
 * (CR/PR/SAR via mock_registers on the i.MX8M Mini PWM1-4 bases), duty
 * cycle sample computation (SAR = duty * period / 32768), period updates
 * restricted by channel class, output state comparison against the counter
 * register, notification interrupt-enable bits, DeInit state reset, and
 * DET parameter validation.
 */

// @tests src/bsw/mcal/pwm/src/Pwm.c  @tests src/bsw/mcal/pwm/include/Pwm.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Pwm.h"
#include "Pwm_Cfg.h"

/* i.MX8M Mini PWM register bases (must match Pwm.c non-S32K312 build) */
#define PWM1_TEST_BASE      (0x30660000UL)
#define PWM2_TEST_BASE      (0x30670000UL)
#define PWM3_TEST_BASE      (0x30680000UL)
#define PWM4_TEST_BASE      (0x30690000UL)

/* Register offsets (Pwm.c internal defines, mirrored here with _OFF suffix) */
#define PWM_CR_OFF          (0x00U)
#define PWM_IR_OFF          (0x08U)
#define PWM_SAR_OFF         (0x0CU)
#define PWM_PR_OFF          (0x10U)
#define PWM_CNR_OFF         (0x14U)

/* CR bits */
#define PWM_CR_EN_BIT       (0x00000001UL)

/* IR bits written by Pwm_EnableNotification */
#define PWM_IR_FIE_BIT      (0x00000001UL)
#define PWM_IR_CIE_BIT      (0x00000004UL)

/*
 * Eight channel entries are required: Pwm_Init/Pwm_DeInit loop over
 * PWM_NUM_CHANNELS (8) entries of the Channels array regardless of
 * NumChannels. Channels 4..7 carry channel IDs without a hardware mapping
 * (Pwm_GetBaseAddr returns 0) and are skipped by the driver.
 *
 * Expected programmed values:
 *   ch0: PR=1000, SAR=16384*1000/32768=500,  CR=(0<<4)|EN=0x1
 *   ch1: PR=2000, SAR=16384*2000/32768=1000, CR=(2<<4)|EN=0x21
 *   ch2: PR=500,  SAR= 8192* 500/32768=125,  CR=0x1 (FIXED_PERIOD class)
 *   ch3: PR=300,  SAR= 4096* 300/32768=37,   CR=0x1
 */
static Pwm_ChannelConfigType testChannels[PWM_NUM_CHANNELS] = {
    /* ChannelId, BaseAddress, Class, Period, Duty, Idle, Polarity, Clock, Prescaler, Notif, NotifFn */
    {0U, PWM1_TEST_BASE, PWM_VARIABLE_PERIOD, 1000U, 16384U, PWM_IDLE_LOW, PWM_POLARITY_HIGH, PWM_CLOCK_SYSTEM, 0U, TRUE, NULL_PTR},
    {1U, PWM2_TEST_BASE, PWM_VARIABLE_PERIOD, 2000U, 16384U, PWM_IDLE_LOW, PWM_POLARITY_HIGH, PWM_CLOCK_SYSTEM, 2U, FALSE, NULL_PTR},
    {2U, PWM3_TEST_BASE, PWM_FIXED_PERIOD, 500U, 8192U, PWM_IDLE_LOW, PWM_POLARITY_HIGH, PWM_CLOCK_SYSTEM, 0U, FALSE, NULL_PTR},
    {3U, PWM4_TEST_BASE, PWM_VARIABLE_PERIOD, 300U, 4096U, PWM_IDLE_LOW, PWM_POLARITY_HIGH, PWM_CLOCK_SYSTEM, 0U, FALSE, NULL_PTR},
    {4U, 0U, PWM_VARIABLE_PERIOD, 100U, 0U, PWM_IDLE_LOW, PWM_POLARITY_LOW, PWM_CLOCK_SYSTEM, 0U, FALSE, NULL_PTR},
    {5U, 0U, PWM_VARIABLE_PERIOD, 100U, 0U, PWM_IDLE_LOW, PWM_POLARITY_LOW, PWM_CLOCK_SYSTEM, 0U, FALSE, NULL_PTR},
    {6U, 0U, PWM_VARIABLE_PERIOD, 100U, 0U, PWM_IDLE_LOW, PWM_POLARITY_LOW, PWM_CLOCK_SYSTEM, 0U, FALSE, NULL_PTR},
    {7U, 0U, PWM_VARIABLE_PERIOD, 100U, 0U, PWM_IDLE_LOW, PWM_POLARITY_LOW, PWM_CLOCK_SYSTEM, 0U, FALSE, NULL_PTR},
};

static const Pwm_ConfigType testConfig = {
    testChannels,           /* Channels */
    PWM_NUM_CHANNELS,       /* NumChannels */
    TRUE,                   /* DevErrorDetect */
    TRUE,                   /* VersionInfoApi */
    TRUE,                   /* DeInitApi */
    TRUE,                   /* SetDutyCycleApi */
    TRUE,                   /* SetPeriodAndDutyApi */
    TRUE,                   /* SetOutputToIdleApi */
    TRUE,                   /* GetOutputStateApi */
    TRUE,                   /* NotificationSupported */
    FALSE                   /* PowerStateSupported */
};

/* Pwm.c keeps static init state; Pwm_DeInit resets it, so tests re-init via
 * this helper (DeInit on an uninitialized driver only reports DET, which is
 * cleared afterwards). */
static void test_Pwm_EnsureInit(void)
{
    (void)Pwm_DeInit();
    MockRegisters_Reset();
    Det_Mock_Reset();
    Pwm_Init(&testConfig);
    Det_Mock_Reset();
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Before initialization --- */

/** @req SWS_Pwm_00001 */
void test_Pwm_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Pwm_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PWM_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_PARAM_CONFIG, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00003 */
void test_Pwm_SetDutyCycle_BeforeInit_ShouldReportUninitDet(void) {
    Pwm_SetDutyCycle(0U, 16384U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PWM_SID_SETDUTYCYCLE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00004 */
void test_Pwm_SetPeriodAndDuty_BeforeInit_ShouldReportUninitDet(void) {
    Pwm_SetPeriodAndDuty(0U, 1000U, 16384U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_SETPERIODANDDUTY, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00005 */
void test_Pwm_SetOutputToIdle_BeforeInit_ShouldReportUninitDet(void) {
    Pwm_SetOutputToIdle(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_SETOUTPUTTOIDLE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00006 */
void test_Pwm_GetOutputState_BeforeInit_ShouldReportDetAndReturnLow(void) {
    Pwm_OutputStateType state = Pwm_GetOutputState(0U);
    TEST_ASSERT_EQUAL(PWM_LOW, state);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_GETOUTPUTSTATE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00008 */
void test_Pwm_EnableNotification_BeforeInit_ShouldReportUninitDet(void) {
    Pwm_EnableNotification(0U, PWM_BOTH_EDGES);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_ENABLENOTIFICATION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00007 */
void test_Pwm_DisableNotification_BeforeInit_ShouldReportUninitDet(void) {
    Pwm_DisableNotification(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_DISABLENOTIFICATION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00002 */
void test_Pwm_DeInit_BeforeInit_ShouldReportUninitDet(void) {
    Pwm_DeInit();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PWM_SID_DEINIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/* --- Init --- */

/** @req SWS_Pwm_00001 */
void test_Pwm_Init_ValidConfig_ShouldProgramChannelRegisters(void) {
    test_Pwm_EnsureInit();
    /* Channel 0: period, duty sample and control word */
    TEST_ASSERT_EQUAL_HEX32(1000U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_PR_OFF));
    TEST_ASSERT_EQUAL_HEX32(500U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_SAR_OFF));
    TEST_ASSERT_EQUAL_HEX32(PWM_CR_EN_BIT, MockRegisters_Read32(PWM1_TEST_BASE + PWM_CR_OFF));
    /* Channel 1: prescaler 2 lands in CR bits 4..11 */
    TEST_ASSERT_EQUAL_HEX32(2000U, MockRegisters_Read32(PWM2_TEST_BASE + PWM_PR_OFF));
    TEST_ASSERT_EQUAL_HEX32(1000U, MockRegisters_Read32(PWM2_TEST_BASE + PWM_SAR_OFF));
    TEST_ASSERT_EQUAL_HEX32((2UL << 4) | PWM_CR_EN_BIT, MockRegisters_Read32(PWM2_TEST_BASE + PWM_CR_OFF));
    /* Channel 2 (FIXED_PERIOD) and channel 3 are programmed likewise */
    TEST_ASSERT_EQUAL_HEX32(125U, MockRegisters_Read32(PWM3_TEST_BASE + PWM_SAR_OFF));
    TEST_ASSERT_EQUAL_HEX32(37U, MockRegisters_Read32(PWM4_TEST_BASE + PWM_SAR_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Pwm_00001 */
void test_Pwm_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    test_Pwm_EnsureInit();
    Pwm_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PWM_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

/* --- DeInit --- */

/** @req SWS_Pwm_00002 */
void test_Pwm_DeInit_AfterInit_ShouldDisableChannelsAndResetState(void) {
    test_Pwm_EnsureInit();
    Pwm_DeInit();
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
    /* Control registers cleared on all mapped channels */
    TEST_ASSERT_EQUAL_HEX32(0U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_CR_OFF));
    TEST_ASSERT_EQUAL_HEX32(0U, MockRegisters_Read32(PWM2_TEST_BASE + PWM_CR_OFF));
    TEST_ASSERT_EQUAL_HEX32(0U, MockRegisters_Read32(PWM3_TEST_BASE + PWM_CR_OFF));
    TEST_ASSERT_EQUAL_HEX32(0U, MockRegisters_Read32(PWM4_TEST_BASE + PWM_CR_OFF));
    /* State reset: any API now reports uninitialized */
    Pwm_SetDutyCycle(0U, 16384U);
    TEST_ASSERT_EQUAL(PWM_E_UNINIT, Det_MockData.ErrorId);
}

/* --- SetDutyCycle --- */

/** @req SWS_Pwm_00003 */
void test_Pwm_SetDutyCycle_AfterInit_ShouldUpdateSampleRegister(void) {
    test_Pwm_EnsureInit();
    /* SAR = DutyCycle * PR / 32768 = 8192 * 1000 / 32768 = 250 */
    Pwm_SetDutyCycle(0U, 8192U);
    TEST_ASSERT_EQUAL_HEX32(250U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_SAR_OFF));
    /* Period register untouched */
    TEST_ASSERT_EQUAL_HEX32(1000U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_PR_OFF));
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_Pwm_00003 */
void test_Pwm_SetDutyCycle_InvalidChannel_ShouldReportDet(void) {
    test_Pwm_EnsureInit();
    Pwm_SetDutyCycle(PWM_NUM_CHANNELS, 16384U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_SETDUTYCYCLE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* --- SetPeriodAndDuty --- */

/** @req SWS_Pwm_00004 */
void test_Pwm_SetPeriodAndDuty_VariablePeriod_ShouldUpdatePrAndSar(void) {
    test_Pwm_EnsureInit();
    /* SAR = 16384 * 4000 / 32768 = 2000 */
    Pwm_SetPeriodAndDuty(0U, 4000U, 16384U);
    TEST_ASSERT_EQUAL_HEX32(4000U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_PR_OFF));
    TEST_ASSERT_EQUAL_HEX32(2000U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_SAR_OFF));
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_Pwm_00004 */
void test_Pwm_SetPeriodAndDuty_FixedPeriod_ShouldReportDetAndKeepRegisters(void) {
    test_Pwm_EnsureInit();
    /* Channel 2 is configured PWM_FIXED_PERIOD */
    Pwm_SetPeriodAndDuty(2U, 9000U, 16384U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_SETPERIODANDDUTY, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_PERIOD_UNCHANGEABLE, Det_MockData.ErrorId);
    /* Registers must stay at the initialized values */
    TEST_ASSERT_EQUAL_HEX32(500U, MockRegisters_Read32(PWM3_TEST_BASE + PWM_PR_OFF));
    TEST_ASSERT_EQUAL_HEX32(125U, MockRegisters_Read32(PWM3_TEST_BASE + PWM_SAR_OFF));
}

/* --- SetOutputToIdle --- */

/** @req SWS_Pwm_00005 */
void test_Pwm_SetOutputToIdle_ShouldClearSampleRegister(void) {
    test_Pwm_EnsureInit();
    TEST_ASSERT_EQUAL_HEX32(500U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_SAR_OFF));
    Pwm_SetOutputToIdle(0U);
    TEST_ASSERT_EQUAL_HEX32(0U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_SAR_OFF));
}

/* --- GetOutputState --- */

/** @req SWS_Pwm_00006 */
void test_Pwm_GetOutputState_CounterBelowSample_ShouldReturnHigh(void) {
    test_Pwm_EnsureInit();
    /* Initialized SAR = 500; counter below the sample -> output high */
    MockRegisters_Write32(PWM1_TEST_BASE + PWM_CNR_OFF, 100U);
    TEST_ASSERT_EQUAL(PWM_HIGH, Pwm_GetOutputState(0U));
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_Pwm_00006 */
void test_Pwm_GetOutputState_CounterAtOrAboveSample_ShouldReturnLow(void) {
    test_Pwm_EnsureInit();
    MockRegisters_Write32(PWM1_TEST_BASE + PWM_CNR_OFF, 600U);
    TEST_ASSERT_EQUAL(PWM_LOW, Pwm_GetOutputState(0U));
    MockRegisters_Write32(PWM1_TEST_BASE + PWM_CNR_OFF, 500U);
    TEST_ASSERT_EQUAL(PWM_LOW, Pwm_GetOutputState(0U));
}

/** @req SWS_Pwm_00006 */
void test_Pwm_GetOutputState_InvalidChannel_ShouldReportDetAndReturnLow(void) {
    test_Pwm_EnsureInit();
    TEST_ASSERT_EQUAL(PWM_LOW, Pwm_GetOutputState(PWM_NUM_CHANNELS));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_GETOUTPUTSTATE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/* --- Notifications --- */

/** @req SWS_Pwm_00008 */
void test_Pwm_EnableNotification_ShouldSetEdgeInterruptBits(void) {
    test_Pwm_EnsureInit();
    /* Rising edge selects the FIFO interrupt enable bit */
    Pwm_EnableNotification(0U, PWM_RISING_EDGE);
    TEST_ASSERT_EQUAL_HEX32(PWM_IR_FIE_BIT, MockRegisters_Read32(PWM1_TEST_BASE + PWM_IR_OFF));
    /* Falling edge selects the compare interrupt enable bit */
    Pwm_EnableNotification(0U, PWM_FALLING_EDGE);
    TEST_ASSERT_EQUAL_HEX32(PWM_IR_CIE_BIT, MockRegisters_Read32(PWM1_TEST_BASE + PWM_IR_OFF));
    /* Both edges select both bits */
    Pwm_EnableNotification(0U, PWM_BOTH_EDGES);
    TEST_ASSERT_EQUAL_HEX32(PWM_IR_FIE_BIT | PWM_IR_CIE_BIT,
                             MockRegisters_Read32(PWM1_TEST_BASE + PWM_IR_OFF));
}

/** @req SWS_Pwm_00008 */
void test_Pwm_EnableNotification_InvalidChannel_ShouldReportDet(void) {
    test_Pwm_EnsureInit();
    Pwm_EnableNotification(PWM_NUM_CHANNELS, PWM_RISING_EDGE);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_SID_ENABLENOTIFICATION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_PARAM_CHANNEL, Det_MockData.ErrorId);
}

/** @req SWS_Pwm_00007 */
void test_Pwm_DisableNotification_ShouldClearInterruptRegister(void) {
    test_Pwm_EnsureInit();
    Pwm_EnableNotification(0U, PWM_BOTH_EDGES);
    TEST_ASSERT_EQUAL_HEX32(PWM_IR_FIE_BIT | PWM_IR_CIE_BIT,
                             MockRegisters_Read32(PWM1_TEST_BASE + PWM_IR_OFF));
    Pwm_DisableNotification(0U);
    TEST_ASSERT_EQUAL_HEX32(0U, MockRegisters_Read32(PWM1_TEST_BASE + PWM_IR_OFF));
}

/* --- GetVersionInfo --- */

/** @req SWS_Pwm_00009 */
void test_Pwm_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType info;
    Pwm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(PWM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(PWM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(PWM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(PWM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(PWM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Pwm_00009 */
void test_Pwm_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Pwm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PWM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PWM_SID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PWM_E_PARAM_POINTER, Det_MockData.ErrorId);
}

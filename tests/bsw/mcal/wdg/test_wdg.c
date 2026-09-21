/**
 * @file test_wdg.c
 * @brief Wdg (Watchdog Driver) Unit Tests — Substantiated
 * @req SWS_Wdg
 *
 * Substantiation: state machine transitions, DET parameter validation,
 * trigger counter verification, register write checks via mock_registers.
 */

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Wdg.h"
#include "Wdg_Cfg.h"

/* WDG base address (must match Wdg.c non-S32K312 build) */
#define WDG_BASE_ADDR   (0x30280000UL)
#define WDG_WCR_OFF     (0x00U)
#define WDG_WICR_OFF    (0x06U)

/* WCR bit definitions (must match Wdg.c) */
#define WCR_WDE         (0x0004U)
#define WCR_WDT         (0x0020U)
#define WCR_WT_MASK     (0xFF00U)

static Wdg_ConfigType testConfig;

static void test_Wdg_SetupFastConfig(void) {
    testConfig.InitialMode = WDGIF_FAST_MODE;
    testConfig.FastModeSettings.TimeoutPeriod = 1000U;
    testConfig.FastModeSettings.WindowModeEnabled = FALSE;
    testConfig.FastModeSettings.WindowStart = 0U;
    testConfig.FastModeSettings.WindowEnd = 0U;
    testConfig.FastModeSettings.InterruptMode = FALSE;
    testConfig.FastModeSettings.TimeoutPreWarningUs = 0U;
    testConfig.SlowModeSettings.TimeoutPeriod = 5000U;
    testConfig.SlowModeSettings.WindowModeEnabled = FALSE;
    testConfig.SlowModeSettings.InterruptMode = FALSE;
    testConfig.PreWarningCallback = NULL_PTR;
    testConfig.WindowViolationCallback = NULL_PTR;
}

static void test_Wdg_SetupOffConfig(void) {
    test_Wdg_SetupFastConfig();
    testConfig.InitialMode = WDGIF_OFF_MODE;
}

void setUp(void) {
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Init --- */

/** @req SWS_Wdg_00001 */
void test_Wdg_Init_NullPtr_ShouldReportDet(void) {
    Wdg_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(WDG_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_PARAM_CONFIG, Det_MockData.ErrorId);
}

/** @req SWS_Wdg_00001 */
void test_Wdg_Init_FastMode_ShouldEnterRunningStateAndEnableWatchdog(void) {
    test_Wdg_SetupFastConfig();
    Wdg_Init(&testConfig);
    TEST_ASSERT_EQUAL(WDG_STATE_RUNNING, Wdg_GetStatus());
    uint32 wcr = MockRegisters_Read32(WDG_BASE_ADDR + WDG_WCR_OFF);
    TEST_ASSERT_NOT_EQUAL(0U, wcr & WCR_WDE);
}

/** @req SWS_Wdg_00001 */
void test_Wdg_Init_OffMode_AfterInit_ShouldReportAlreadyInitialized(void) {
    test_Wdg_SetupOffConfig();
    Det_Mock_Reset();
    Wdg_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(WDG_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    /* Second init is rejected; state stays at the previous RUNNING setting. */
    TEST_ASSERT_EQUAL(WDG_STATE_RUNNING, Wdg_GetStatus());
}

/** @req SWS_Wdg_00001 */
void test_Wdg_Init_DoubleInit_ShouldReportDet(void) {
    test_Wdg_SetupFastConfig();
    Wdg_Init(&testConfig);
    Det_Mock_Reset();
    Wdg_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
}

/* --- SetMode --- */

/** @req SWS_Wdg_00002 */
void test_Wdg_SetMode_BeforeInit_ShouldReportDetAndFail(void) {
    Std_ReturnType ret = Wdg_SetMode(WDGIF_FAST_MODE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_SETMODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Wdg_00002 */
void test_Wdg_SetMode_AfterInit_ShouldSucceed(void) {
    test_Wdg_SetupFastConfig();
    Wdg_Init(&testConfig);
    Det_Mock_Reset();
    Std_ReturnType ret = Wdg_SetMode(WDGIF_SLOW_MODE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_Wdg_00002 */
void test_Wdg_SetMode_InvalidMode_ShouldFail(void) {
    test_Wdg_SetupFastConfig();
    Wdg_Init(&testConfig);
    Det_Mock_Reset();
    Std_ReturnType ret = Wdg_SetMode((WdgIf_ModeType)0xFFU);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- Trigger --- */

/** @req SWS_Wdg_00003 */
void test_Wdg_Trigger_BeforeInit_ShouldReportDet(void) {
    Wdg_Trigger();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_TRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Wdg_00003 */
void test_Wdg_Trigger_AfterInit_ShouldIncrementCounter(void) {
    test_Wdg_SetupFastConfig();
    Wdg_Init(&testConfig);
    uint32 counterBefore = Wdg_GetTriggerCounter();
    Wdg_Trigger();
    uint32 counterAfter = Wdg_GetTriggerCounter();
    TEST_ASSERT_GREATER_THAN(counterBefore, counterAfter);
}

/* --- SetTriggerCondition --- */

/** @req SWS_Wdg_00005 */
void test_Wdg_SetTriggerCondition_AfterInit_ShouldSucceed(void) {
    test_Wdg_SetupFastConfig();
    Wdg_Init(&testConfig);
    Det_Mock_Reset();
    Std_ReturnType ret = Wdg_SetTriggerCondition(500U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/* --- GetVersionInfo --- */

/** @req SWS_Wdg_00004 */
void test_Wdg_GetVersionInfo_ValidPtr_ShouldReturnCorrectVersion(void) {
    Std_VersionInfoType info;
    Wdg_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(WDG_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(WDG_MODULE_ID, info.moduleID);
}

/** @req SWS_Wdg_00004 */
void test_Wdg_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Wdg_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/* --- GetStatus --- */

/** @req SWS_Wdg_00006 */
void test_Wdg_GetStatus_BeforeInit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL(WDG_STATE_UNINIT, Wdg_GetStatus());
}

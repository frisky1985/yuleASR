/**
 * @file test_wdg_offmode.c
 * @brief Wdg (Watchdog Driver) Unit Tests - OFF Mode Coverage
 * @req SWS_Wdg
 *
 * This test file is compiled against a patched Wdg_offmode.c that replaces
 * the compile-time guard  #if (WDG_DISABLE_ALLOWED == STD_OFF)  with
 * #ifdef WDG_DISABLE_OFF_MODE_ALLOWED  so that OFF-mode runtime branches
 * become reachable when the test binary is built with
 * -DWDG_DISABLE_OFF_MODE_ALLOWED=1.
 *
 * IMPORTANT: Because Wdg has static state with no DeInit, Wdg_Init can only
 * succeed ONCE per binary.  Tests are structured as a sequential flow that
 * exercises different code paths via Wdg_SetMode transitions after the
 * single Init call.
 */

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Wdg.h"
#include "Wdg_Cfg.h"

/* WDG base address (must match Wdg.c non-S32K312 build) */
#define WDG_BASE_ADDR   (0x30280000UL)
#define WDG_WCR_OFF     (0x00U)
#define WDG_WSR_OFF     (0x02U)
#define WDG_WICR_OFF    (0x06U)

/* WCR bit definitions (must match Wdg.c) */
#define WCR_WDE         (0x0004U)
#define WCR_WDT         (0x0020U)

/* WICR bit definitions */
#define WICR_WIE        (0x0001U)

static Wdg_ConfigType testConfig;

void setUp(void) {
    /* NOTE: Do NOT reset MockRegisters here - register state must persist
     * across tests to match the persistent static driver state. Only reset
     * the DET mock which tracks per-test error reporting. */
    Det_Mock_Reset();
}

void tearDown(void) {}

/* =========================================================================
 * Test 0: Sanity check - verify register mock works
 * ========================================================================= */

void test_Wdg_OM_00_RegisterMockSanity(void) {
    /* Write a value and read it back */
    MockRegisters_Write16(0xDEAD0000U, 0x1234U);
    uint16 val = MockRegisters_Read16(0xDEAD0000U);
    TEST_ASSERT_EQUAL_HEX16(0x1234U, val);

    /* Write16 should preserve upper 16 bits */
    MockRegisters_Write32(0xBEEF0000U, 0xABCD0000U);
    MockRegisters_Write16(0xBEEF0000U, 0x5678U);
    uint32 val32 = MockRegisters_Read32(0xBEEF0000U);
    TEST_ASSERT_EQUAL_HEX32(0xABCD5678U, val32);

    /* Clean up test addresses (won't affect other tests since they use
     * different addresses) */
}

/* =========================================================================
 * Test 1: Init with OFF mode
 * Covers Init lines: 232-233 (OFF modeSettings=NULL), 250-251 (timeout=0),
 *                      262-263 (ModuleState=IDLE)
 * ========================================================================= */

void test_Wdg_OM_01_Init_OffMode_ShouldSetIdleState(void) {
    testConfig.InitialMode = WDGIF_OFF_MODE;
    testConfig.FastModeSettings.TimeoutPeriod = 1000U;
    testConfig.FastModeSettings.WindowModeEnabled = FALSE;
    testConfig.FastModeSettings.WindowStart = 0U;
    testConfig.FastModeSettings.WindowEnd = 0U;
    testConfig.FastModeSettings.InterruptMode = FALSE;
    testConfig.FastModeSettings.TimeoutPreWarningUs = 0U;
    testConfig.SlowModeSettings.TimeoutPeriod = 5000U;
    testConfig.SlowModeSettings.WindowModeEnabled = FALSE;
    testConfig.SlowModeSettings.WindowStart = 0U;
    testConfig.SlowModeSettings.WindowEnd = 0U;
    testConfig.SlowModeSettings.InterruptMode = FALSE;
    testConfig.SlowModeSettings.TimeoutPreWarningUs = 0U;
    testConfig.PreWarningCallback = NULL_PTR;
    testConfig.WindowViolationCallback = NULL_PTR;

    Wdg_Init(&testConfig);

    /* Should be IDLE since Init mode is OFF */
    TEST_ASSERT_EQUAL(WDG_STATE_IDLE, Wdg_GetStatus());

    /* WDE should NOT be set */
    uint16 wcr = MockRegisters_Read16(WDG_BASE_ADDR + WDG_WCR_OFF);
    TEST_ASSERT_EQUAL(0U, wcr & WCR_WDE);

    /* Trigger counter should be 0 */
    TEST_ASSERT_EQUAL(0U, Wdg_GetTriggerCounter());
}

/* =========================================================================
 * Test 2: Trigger in OFF mode (right after Init)
 * Covers Trigger lines: 357-358 (TRUE branch of OFF mode check),
 *                        359 (Det_ReportError FORBIDDEN), 361 (return)
 * ========================================================================= */

void test_Wdg_OM_02_Trigger_InOffMode_ShouldReportForbidden(void) {
    Wdg_Trigger();

    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_TRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_FORBIDDEN_INVOCATION, Det_MockData.ErrorId);
}

/* =========================================================================
 * Test 3: SetTriggerCondition in OFF mode
 * Covers SetTriggerCondition line: 453-454 (OFF mode -> return E_NOT_OK)
 * ========================================================================= */

void test_Wdg_OM_03_SetTriggerCondition_InOffMode_ShouldFail(void) {
    Std_ReturnType ret = Wdg_SetTriggerCondition(500U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* =========================================================================
 * Test 4: SetMode OFF -> invalid mode
 * Covers SetMode lines: 283-284 (TRUE, OFF mode check),
 *                        285 (Det_ReportError MODE_TRANSITION), 286 (return)
 * ========================================================================= */

void test_Wdg_OM_04_SetMode_OffToInvalid_ShouldReportModeTransition(void) {
    Std_ReturnType ret = Wdg_SetMode((WdgIf_ModeType)99U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_SETMODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_MODE_TRANSITION, Det_MockData.ErrorId);
}

/* =========================================================================
 * Test 5: SetMode OFF -> FAST (valid transition)
 * Covers SetMode lines: 283-284 (TRUE), 285 (FALSE), 290-295 (skipped,
 *          not OFF target), 302-307 (OFF case skipped), 314-315 (FAST),
 *          326-339 (modeSettings != NULL: WDE, timeout update, RUNNING)
 * ========================================================================= */

void test_Wdg_OM_05_SetMode_OffToFast_ShouldEnableWatchdog(void) {
    Std_ReturnType ret = Wdg_SetMode(WDGIF_FAST_MODE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDG_STATE_RUNNING, Wdg_GetStatus());

    /* WDE should be set now */
    uint16 wcr = MockRegisters_Read16(WDG_BASE_ADDR + WDG_WCR_OFF);
    TEST_ASSERT_NOT_EQUAL(0U, wcr & WCR_WDE);
}

/* =========================================================================
 * Test 6: Trigger in FAST mode (service sequence)
 * Verifies normal trigger operation after OFF->FAST transition
 * ========================================================================= */

void test_Wdg_OM_06_Trigger_InFastMode_ShouldService(void) {
    Wdg_Trigger();

    /* No DET errors expected (window mode disabled) */
    TEST_ASSERT_EQUAL(0U, Det_MockData.CallCount);

    /* Trigger counter should be 1 */
    TEST_ASSERT_EQUAL(1U, Wdg_GetTriggerCounter());
}

/* =========================================================================
 * Test 7: SetMode FAST -> invalid mode
 * Covers SetMode switch default case: 318-322 (Det_ReportError PARAM_MODE)
 * ========================================================================= */

void test_Wdg_OM_07_SetMode_FastToInvalid_ShouldReportParamMode(void) {
    Std_ReturnType ret = Wdg_SetMode((WdgIf_ModeType)99U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_SETMODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_PARAM_MODE, Det_MockData.ErrorId);

    /* Verify WCR still has WDE set (SetMode failed, no change) */
    uint16 wcr = MockRegisters_Read16(WDG_BASE_ADDR + WDG_WCR_OFF);
    TEST_ASSERT_NOT_EQUAL(0U, wcr & WCR_WDE);
}

/* =========================================================================
 * Test 8: SetMode FAST -> OFF
 * Covers SetMode switch OFF case: 302 (case OFF), 304 (~WDE),
 *          305 (REG_WRITE), 306 (IDLE), 307 (NULL_PTR)
 * ========================================================================= */

void test_Wdg_OM_08_SetMode_FastToOff_ShouldDisableWatchdog(void) {
    /* Debug: check WCR before SetMode */
    uint16 wcr_before = MockRegisters_Read16(WDG_BASE_ADDR + WDG_WCR_OFF);

    Std_ReturnType ret = Wdg_SetMode(WDGIF_OFF_MODE);

    /* Debug: check WCR after SetMode */
    uint16 wcr_after = MockRegisters_Read16(WDG_BASE_ADDR + WDG_WCR_OFF);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDG_STATE_IDLE, Wdg_GetStatus());

    /* WDE should be cleared */
    TEST_ASSERT_EQUAL_HEX16(0U, wcr_after & WCR_WDE);
}

/* =========================================================================
 * Test 9: Trigger in OFF mode (after FAST->OFF transition)
 * Re-covers Trigger lines 357-361 from a different state path
 * ========================================================================= */

void test_Wdg_OM_09_Trigger_AfterFastToOff_ShouldReportForbidden(void) {
    Wdg_Trigger();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(WDG_SID_TRIGGER, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(WDG_E_FORBIDDEN_INVOCATION, Det_MockData.ErrorId);
}

/* =========================================================================
 * Test 10: SetMode OFF -> SLOW (valid transition)
 * Covers SetMode line: 310-311 (SLOW case: modeSettings = SlowModeSettings)
 * ========================================================================= */

void test_Wdg_OM_10_SetMode_OffToSlow_ShouldSucceed(void) {
    Std_ReturnType ret = Wdg_SetMode(WDGIF_SLOW_MODE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDG_STATE_RUNNING, Wdg_GetStatus());
}

/* =========================================================================
 * Test 11: SetMode SLOW -> OFF, then OFF -> FAST, then Trigger
 * Verifies full OFF->FAST round-trip and trigger counter behavior
 * ========================================================================= */

void test_Wdg_OM_11_OffToSlow_ThenOff_ThenFast_Trigger(void) {
    /* SLOW -> OFF */
    Std_ReturnType ret = Wdg_SetMode(WDGIF_OFF_MODE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDG_STATE_IDLE, Wdg_GetStatus());

    /* OFF -> FAST */
    ret = Wdg_SetMode(WDGIF_FAST_MODE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDG_STATE_RUNNING, Wdg_GetStatus());

    /* Trigger should work; counter was 1 from test 6, reset to 0 by SetMode
     * to FAST (via LastTriggerTime reset), but TriggerCounter is NOT reset
     * by SetMode. It was 1 from test 6. */
    Wdg_Trigger();
    TEST_ASSERT_EQUAL(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(2U, Wdg_GetTriggerCounter());
}

/* =========================================================================
 * Test 12: SetTriggerCondition after OFF->FAST transition
 * Verifies SetTriggerCondition works in FAST mode after OFF transition
 * ========================================================================= */

void test_Wdg_OM_12_SetTriggerCondition_AfterOffToFast_ShouldSucceed(void) {
    Std_ReturnType ret = Wdg_SetTriggerCondition(500U);
    TEST_ASSERT_EQUAL(E_OK, ret);

    /* SetTriggerCondition internally calls Wdg_Trigger, so counter
     * goes from 2 to 3 */
    TEST_ASSERT_EQUAL(3U, Wdg_GetTriggerCounter());
}

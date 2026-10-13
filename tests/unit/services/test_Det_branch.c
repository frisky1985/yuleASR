/**
 * @file test_Det_branch.c
 * @brief Targeted branch coverage tests for Det.c
 *
 * Supplements existing det_test with tests for branches that were
 * previously uncovered:
 *   - Det_Init: already STARTED state (second OR operand true)
 *   - Det_Start: called before Init (not-initialized path)
 *   - Det_GetVersionInfo: NULL ptr else branch (error reporting)
 *   - Det_CallErrorHooks: with registered hooks and NULL slots
 *   - Det_CallRuntimeCallouts: with registered callouts and NULL slots
 *   - Det_CallTransientCallouts: with registered callouts and NULL slots
 *   - Det_ReportRuntimeError: with callouts returning E_OK
 *   - Det_ReportTransientFault: with callouts returning E_OK
 *
 * We include Det.c directly to access static variables (hook tables,
 * Det_State) for full branch coverage.
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "Std_Types.h"
#include "Det.h"
#include "Det_Cfg.h"

/* Include production source to access static variables */
#include "src/bsw/services/det/src/Det.c"

/* ---- Test hook/callout tracking ---------------------------------------- */

static uint8 g_hookCallCount = 0;
static uint16 g_hookModuleId = 0;
static uint8 g_hookInstanceId = 0;
static uint8 g_hookApiId = 0;
static uint8 g_hookErrorId = 0;

static void test_ErrorHook(uint16 ModuleId, uint8 InstanceId,
                           uint8 ApiId, uint8 ErrorId) {
    g_hookCallCount++;
    g_hookModuleId = ModuleId;
    g_hookInstanceId = InstanceId;
    g_hookApiId = ApiId;
    g_hookErrorId = ErrorId;
}

static uint8 g_runtimeCalloutCount = 0;
static void test_RuntimeCallout(uint16 ModuleId, uint8 InstanceId,
                                uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId; (void)ApiId; (void)ErrorId;
    g_runtimeCalloutCount++;
}

static uint8 g_transientCalloutCount = 0;
static void test_TransientCallout(uint16 ModuleId, uint8 InstanceId,
                                  uint8 ApiId, uint8 FaultId) {
    (void)ModuleId; (void)InstanceId; (void)ApiId; (void)FaultId;
    g_transientCalloutCount++;
}

/* ---- Helpers to reset Det internal state ------------------------------- */

/* Force Det back to UNINITIALIZED for test isolation */
static void resetDetState(void) {
    Det_State = DET_UNINITIALIZED;
    DetInitialized = FALSE;
    DetConfigPtr = NULL_PTR;
    Det_ErrorCounter = 0u;
    Det_RuntimeErrorCounter = 0u;
    Det_TransientFaultCounter = 0u;
    Det_NumRegisteredHooks = 0u;
    Det_NumRuntimeCallouts = 0u;
    Det_NumTransientCallouts = 0u;
    for (uint8 i = 0u; i < DET_MAX_ERROR_HOOKS; i++) {
        Det_ErrorHooks[i] = NULL_PTR;
    }
    for (uint8 i = 0u; i < DET_MAX_RUNTIME_CALLOUTS; i++) {
        Det_RuntimeCallouts[i] = NULL_PTR;
    }
    for (uint8 i = 0u; i < DET_MAX_TRANSIENT_CALLOUTS; i++) {
        Det_TransientCallouts[i] = NULL_PTR;
    }
}

void setUp(void) {
    resetDetState();
    g_hookCallCount = 0;
    g_hookModuleId = 0;
    g_hookInstanceId = 0;
    g_hookApiId = 0;
    g_hookErrorId = 0;
    g_runtimeCalloutCount = 0;
    g_transientCalloutCount = 0;
}

void tearDown(void) {}

/* ========================================================================
 * 1. Det_Init: already STARTED state
 *    Branch: (Det_State == DET_INITIALIZED) || (Det_State == DET_STARTED)
 *    When state is STARTED, first OR operand is false, second is true.
 * ======================================================================== */
void test_DetInit_AlreadyStarted_ShouldReturnEarly(void) {
    /* Manually set state to STARTED */
    Det_State = DET_STARTED;
    DetInitialized = TRUE;

    Det_ConfigType cfg = {0};
    Det_Init(&cfg);

    /* Should return early, state stays STARTED (not re-initialized) */
    TEST_ASSERT_EQUAL(DET_STARTED, Det_State);
}

/* ========================================================================
 * 2. Det_Init: already INITIALIZED state
 *    First OR operand is true -> short-circuit, second not evaluated.
 * ======================================================================== */
void test_DetInit_AlreadyInitialized_ShouldReturnEarly(void) {
    Det_State = DET_INITIALIZED;
    DetInitialized = TRUE;

    Det_ConfigType cfg = {0};
    Det_Init(&cfg);

    /* Should return early */
    TEST_ASSERT_EQUAL(DET_INITIALIZED, Det_State);
}

/* ========================================================================
 * 3. Det_Start: called before Init (state == UNINITIALIZED)
 *    Branch: if (Det_State == DET_INITIALIZED) -> false
 * ======================================================================== */
void test_DetStart_BeforeInit_ShouldHaveNoEffect(void) {
    TEST_ASSERT_EQUAL(DET_UNINITIALIZED, Det_State);

    Det_Start();

    /* State should remain UNINITIALIZED */
    TEST_ASSERT_EQUAL(DET_UNINITIALIZED, Det_State);
}

/* ========================================================================
 * 4. Det_Start: called after Init (state == INITIALIZED)
 *    Branch: if (Det_State == DET_INITIALIZED) -> true
 * ======================================================================== */
void test_DetStart_AfterInit_ShouldTransitionToStarted(void) {
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    TEST_ASSERT_EQUAL(DET_INITIALIZED, Det_State);

    Det_Start();

    TEST_ASSERT_EQUAL(DET_STARTED, Det_State);
}

/* ========================================================================
 * 5. Det_Start: called when already STARTED
 *    Branch: if (Det_State == DET_INITIALIZED) -> false (already STARTED)
 * ======================================================================== */
void test_DetStart_AlreadyStarted_ShouldRemainStarted(void) {
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    Det_Start();
    TEST_ASSERT_EQUAL(DET_STARTED, Det_State);

    /* Call Start again */
    Det_Start();
    TEST_ASSERT_EQUAL(DET_STARTED, Det_State);
}

/* ========================================================================
 * 6. Det_GetVersionInfo: NULL ptr -> else branch (DET_ENABLED == ON)
 *    Branch: if (versioninfo != NULL_PTR) -> false, enters else
 * ======================================================================== */
void test_DetGetVersionInfo_NullPtr_ShouldReportError(void) {
    /* Init Det first so Det_ReportError works properly */
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);

    uint32 counterBefore = Det_ErrorCounter;

    /* Pass NULL -> should trigger Det_ReportError in else branch */
    Det_GetVersionInfo(NULL_PTR);

    /* Error counter should have incremented */
    TEST_ASSERT_EQUAL(counterBefore + 1U, Det_ErrorCounter);
}

/* ========================================================================
 * 7. Det_CallErrorHooks: with registered hooks
 *    Branch: for loop body entered (i < Det_NumRegisteredHooks)
 *    Branch: if (Det_ErrorHooks[i] != NULL_PTR) -> true
 * ======================================================================== */
void test_DetCallErrorHooks_WithRegisteredHook_ShouldCallHook(void) {
    /* Register a hook */
    Det_ErrorHooks[0] = test_ErrorHook;
    Det_NumRegisteredHooks = 1U;

    /* Trigger error reporting which calls Det_CallErrorHooks */
    Det_ReportError(0x10U, 0x01U, 0x02U, 0x03U);

    /* Hook should have been called */
    TEST_ASSERT_EQUAL(1U, g_hookCallCount);
    TEST_ASSERT_EQUAL(0x10U, g_hookModuleId);
    TEST_ASSERT_EQUAL(0x01U, g_hookInstanceId);
    TEST_ASSERT_EQUAL(0x02U, g_hookApiId);
    TEST_ASSERT_EQUAL(0x03U, g_hookErrorId);
}

/* ========================================================================
 * 8. Det_CallErrorHooks: with NULL hook slot
 *    Branch: if (Det_ErrorHooks[i] != NULL_PTR) -> false
 * ======================================================================== */
void test_DetCallErrorHooks_WithNullHookSlot_ShouldSkip(void) {
    /* Register 2 hooks but leave slot 1 as NULL */
    Det_ErrorHooks[0] = test_ErrorHook;
    Det_ErrorHooks[1] = NULL_PTR;  /* NULL slot */
    Det_NumRegisteredHooks = 2U;

    Det_ReportError(0x10U, 0x01U, 0x02U, 0x03U);

    /* Only the first hook should be called (slot 1 is NULL, skipped) */
    TEST_ASSERT_EQUAL(1U, g_hookCallCount);
}

/* ========================================================================
 * 9. Det_CallErrorHooks: no hooks registered
 *    Branch: for loop condition false on first iteration (0 < 0)
 * ======================================================================== */
void test_DetCallErrorHooks_NoHooks_ShouldNotIterate(void) {
    Det_NumRegisteredHooks = 0U;

    Det_ReportError(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(0U, g_hookCallCount);
}

/* ========================================================================
 * 10. Det_CallErrorHooks: multiple hooks
 * ======================================================================== */
void test_DetCallErrorHooks_MultipleHooks_ShouldCallAll(void) {
    Det_ErrorHooks[0] = test_ErrorHook;
    Det_ErrorHooks[1] = test_ErrorHook;
    Det_NumRegisteredHooks = 2U;

    Det_ReportError(0x20U, 0x02U, 0x03U, 0x04U);

    /* Both hooks should be called */
    TEST_ASSERT_EQUAL(2U, g_hookCallCount);
}

/* ========================================================================
 * 11. Det_CallRuntimeCallouts: with registered callout
 *    Branch: for loop body entered + if != NULL_PTR -> true
 * ======================================================================== */
void test_DetRuntimeCallout_WithRegisteredCallout_ShouldCall(void) {
    Det_RuntimeCallouts[0] = test_RuntimeCallout;
    Det_NumRuntimeCallouts = 1U;

    Std_ReturnType result = Det_ReportRuntimeError(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(1U, g_runtimeCalloutCount);
}

/* ========================================================================
 * 12. Det_CallRuntimeCallouts: with NULL callout slot
 *    Branch: if (Det_RuntimeCallouts[i] != NULL_PTR) -> false
 * ======================================================================== */
void test_DetRuntimeCallout_WithNullSlot_ShouldSkip(void) {
    Det_RuntimeCallouts[0] = NULL_PTR;
    Det_NumRuntimeCallouts = 1U;

    Std_ReturnType result = Det_ReportRuntimeError(0x10U, 0x01U, 0x02U, 0x03U);

    /* result is E_OK because DET_RUNTIME_ERROR_CALLOUTS is ON */
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0U, g_runtimeCalloutCount);
}

/* ========================================================================
 * 13. Det_CallRuntimeCallouts: no callouts registered
 *    Branch: for loop condition false (0 < 0)
 * ======================================================================== */
void test_DetRuntimeCallout_NoCallouts_ShouldNotIterate(void) {
    Det_NumRuntimeCallouts = 0U;

    Std_ReturnType result = Det_ReportRuntimeError(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0U, g_runtimeCalloutCount);
}

/* ========================================================================
 * 14. Det_CallTransientCallouts: with registered callout
 *    Branch: for loop body entered + if != NULL_PTR -> true
 * ======================================================================== */
void test_DetTransientCallout_WithRegisteredCallout_ShouldCall(void) {
    Det_TransientCallouts[0] = test_TransientCallout;
    Det_NumTransientCallouts = 1U;

    Std_ReturnType result = Det_ReportTransientFault(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(1U, g_transientCalloutCount);
}

/* ========================================================================
 * 15. Det_CallTransientCallouts: with NULL callout slot
 *    Branch: if (Det_TransientCallouts[i] != NULL_PTR) -> false
 * ======================================================================== */
void test_DetTransientCallout_WithNullSlot_ShouldSkip(void) {
    Det_TransientCallouts[0] = NULL_PTR;
    Det_NumTransientCallouts = 1U;

    Std_ReturnType result = Det_ReportTransientFault(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0U, g_transientCalloutCount);
}

/* ========================================================================
 * 16. Det_CallTransientCallouts: no callouts registered
 *    Branch: for loop condition false (0 < 0)
 * ======================================================================== */
void test_DetTransientCallout_NoCallouts_ShouldNotIterate(void) {
    Det_NumTransientCallouts = 0U;

    Std_ReturnType result = Det_ReportTransientFault(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0U, g_transientCalloutCount);
}

/* ========================================================================
 * 17. Det_ReportError: error counter increments
 * ======================================================================== */
void test_DetReportError_IncrementsErrorCounter(void) {
    TEST_ASSERT_EQUAL(0U, Det_ErrorCounter);

    Det_ReportError(0x01U, 0x00U, 0x01U, 0x01U);
    TEST_ASSERT_EQUAL(1U, Det_ErrorCounter);

    Det_ReportError(0x02U, 0x00U, 0x02U, 0x02U);
    TEST_ASSERT_EQUAL(2U, Det_ErrorCounter);
}

/* ========================================================================
 * 18. Det_ReportRuntimeError: runtime error counter increments
 * ======================================================================== */
void test_DetReportRuntimeError_IncrementsCounter(void) {
    Det_NumRuntimeCallouts = 0U;
    TEST_ASSERT_EQUAL(0U, Det_RuntimeErrorCounter);

    Det_ReportRuntimeError(0x01U, 0x00U, 0x01U, 0x01U);
    TEST_ASSERT_EQUAL(1U, Det_RuntimeErrorCounter);
}

/* ========================================================================
 * 19. Det_ReportTransientFault: transient fault counter increments
 * ======================================================================== */
void test_DetReportTransientFault_IncrementsCounter(void) {
    Det_NumTransientCallouts = 0U;
    TEST_ASSERT_EQUAL(0U, Det_TransientFaultCounter);

    Det_ReportTransientFault(0x01U, 0x00U, 0x01U, 0x01U);
    TEST_ASSERT_EQUAL(1U, Det_TransientFaultCounter);
}

/* ========================================================================
 * 20. Det_Init: normal init from UNINITIALIZED
 *    Branch: if ((Det_State == DET_INITIALIZED) || (Det_State == DET_STARTED))
 *            -> both false, enters init code
 * ======================================================================== */
void test_DetInit_FromUninitialized_ShouldSucceed(void) {
    TEST_ASSERT_EQUAL(DET_UNINITIALIZED, Det_State);

    Det_ConfigType cfg = {0};
    Det_Init(&cfg);

    TEST_ASSERT_EQUAL(DET_INITIALIZED, Det_State);
    TEST_ASSERT_TRUE(DetInitialized);
    TEST_ASSERT_EQUAL(0U, Det_ErrorCounter);
    TEST_ASSERT_EQUAL(0U, Det_RuntimeErrorCounter);
    TEST_ASSERT_EQUAL(0U, Det_TransientFaultCounter);
    TEST_ASSERT_EQUAL(0U, Det_NumRegisteredHooks);
    TEST_ASSERT_EQUAL(0U, Det_NumRuntimeCallouts);
    TEST_ASSERT_EQUAL(0U, Det_NumTransientCallouts);
}

/* ========================================================================
 * 21. Det_Init: hook tables cleared during init
 * ======================================================================== */
void test_DetInit_ClearsHookTables(void) {
    /* Pre-populate hook tables */
    Det_ErrorHooks[0] = test_ErrorHook;
    Det_NumRegisteredHooks = 1U;
    Det_RuntimeCallouts[0] = test_RuntimeCallout;
    Det_NumRuntimeCallouts = 1U;
    Det_TransientCallouts[0] = test_TransientCallout;
    Det_NumTransientCallouts = 1U;

    /* Init should clear all hooks */
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);

    TEST_ASSERT_EQUAL(0U, Det_NumRegisteredHooks);
    TEST_ASSERT_NULL(Det_ErrorHooks[0]);
    TEST_ASSERT_EQUAL(0U, Det_NumRuntimeCallouts);
    TEST_ASSERT_NULL(Det_RuntimeCallouts[0]);
    TEST_ASSERT_EQUAL(0U, Det_NumTransientCallouts);
    TEST_ASSERT_NULL(Det_TransientCallouts[0]);
}

/* ========================================================================
 * 22. Det_CallErrorHooks: loop iteration boundary
 *    Register DET_MAX_ERROR_HOOKS hooks, verify all are called
 * ======================================================================== */
void test_DetCallErrorHooks_MaxHooks_AllCalled(void) {
    for (uint8 i = 0U; i < DET_MAX_ERROR_HOOKS; i++) {
        Det_ErrorHooks[i] = test_ErrorHook;
    }
    Det_NumRegisteredHooks = DET_MAX_ERROR_HOOKS;

    Det_ReportError(0x30U, 0x00U, 0x01U, 0x01U);

    TEST_ASSERT_EQUAL(DET_MAX_ERROR_HOOKS, g_hookCallCount);
}

/* ========================================================================
 * 23. Det_CallRuntimeCallouts: multiple callouts
 * ======================================================================== */
void test_DetRuntimeCallout_MultipleCallouts_AllCalled(void) {
    Det_RuntimeCallouts[0] = test_RuntimeCallout;
    Det_RuntimeCallouts[1] = test_RuntimeCallout;
    Det_NumRuntimeCallouts = 2U;

    Det_ReportRuntimeError(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(2U, g_runtimeCalloutCount);
}

/* ========================================================================
 * 24. Det_CallTransientCallouts: multiple callouts
 * ======================================================================== */
void test_DetTransientCallout_MultipleCallouts_AllCalled(void) {
    Det_TransientCallouts[0] = test_TransientCallout;
    Det_TransientCallouts[1] = test_TransientCallout;
    Det_NumTransientCallouts = 2U;

    Det_ReportTransientFault(0x10U, 0x01U, 0x02U, 0x03U);

    TEST_ASSERT_EQUAL(2U, g_transientCalloutCount);
}

/* ========================================================================
 * 25. Det_GetVersionInfo: valid pointer -> if branch true
 * ======================================================================== */
void test_DetGetVersionInfo_ValidPtr_ShouldFillVersionInfo(void) {
    Std_VersionInfoType info;
    memset(&info, 0xFF, sizeof(info));

    Det_GetVersionInfo(&info);

    TEST_ASSERT_EQUAL(DET_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(DET_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(DET_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(DET_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(DET_SW_PATCH_VERSION, info.sw_patch_version);
}

/* ========================================================================
 * main
 * ======================================================================== */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_DetInit_AlreadyStarted_ShouldReturnEarly);
    RUN_TEST(test_DetInit_AlreadyInitialized_ShouldReturnEarly);
    RUN_TEST(test_DetStart_BeforeInit_ShouldHaveNoEffect);
    RUN_TEST(test_DetStart_AfterInit_ShouldTransitionToStarted);
    RUN_TEST(test_DetStart_AlreadyStarted_ShouldRemainStarted);
    RUN_TEST(test_DetGetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_DetCallErrorHooks_WithRegisteredHook_ShouldCallHook);
    RUN_TEST(test_DetCallErrorHooks_WithNullHookSlot_ShouldSkip);
    RUN_TEST(test_DetCallErrorHooks_NoHooks_ShouldNotIterate);
    RUN_TEST(test_DetCallErrorHooks_MultipleHooks_ShouldCallAll);
    RUN_TEST(test_DetRuntimeCallout_WithRegisteredCallout_ShouldCall);
    RUN_TEST(test_DetRuntimeCallout_WithNullSlot_ShouldSkip);
    RUN_TEST(test_DetRuntimeCallout_NoCallouts_ShouldNotIterate);
    RUN_TEST(test_DetTransientCallout_WithRegisteredCallout_ShouldCall);
    RUN_TEST(test_DetTransientCallout_WithNullSlot_ShouldSkip);
    RUN_TEST(test_DetTransientCallout_NoCallouts_ShouldNotIterate);
    RUN_TEST(test_DetReportError_IncrementsErrorCounter);
    RUN_TEST(test_DetReportRuntimeError_IncrementsCounter);
    RUN_TEST(test_DetReportTransientFault_IncrementsCounter);
    RUN_TEST(test_DetInit_FromUninitialized_ShouldSucceed);
    RUN_TEST(test_DetInit_ClearsHookTables);
    RUN_TEST(test_DetCallErrorHooks_MaxHooks_AllCalled);
    RUN_TEST(test_DetRuntimeCallout_MultipleCallouts_AllCalled);
    RUN_TEST(test_DetTransientCallout_MultipleCallouts_AllCalled);
    RUN_TEST(test_DetGetVersionInfo_ValidPtr_ShouldFillVersionInfo);

    return UnityEnd();
}

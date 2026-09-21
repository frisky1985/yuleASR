/**
 * @file test_wdgm.c
 * @brief WdgM (Watchdog Manager) Unit Tests
 * @req SWS_WdgM
 *
 * Notes on SUT actual behavior reflected here:
 *  - WdgM_DeInit() is always rejected after init (WdgM_DisableAllowed is never
 *    settable through the API), returning E_NOT_OK + WDGM_E_DISABLE_NOT_ALLOWED.
 *  - WdgM_SetMode(WDGM_WATCHDOG_MODE_OFF) is likewise rejected.
 *  - WdgM_HandleLockstepError()/WdgM_HandleRamSafetyError() end in
 *    WdgM_PerformReset(), whose fall-back is an infinite loop; only the
 *    UNINIT guard of these handlers is observable without hanging the test.
 *  - WdgM_GetVersionInfo() silently ignores a NULL pointer (no Det report).
 *  - The module state cannot be reset once initialized (DeInit never
 *    succeeds), so the runner executes all UNINIT-state tests before the
 *    first successful Init.
 */
// @tests src/bsw/services/wdgm/src/WdgM.c  @tests src/bsw/services/wdgm/include/WdgM.h

#include "unity.h"
#include "WdgM.h"
#include "WdgM_Cfg.h"

static uint8 mock_DetApiId = 0xFFU;
static uint8 mock_DetErrorId = 0xFFU;
static uint8 mock_DetCalls = 0;

static void mock_Det_Reset(void) {
    mock_DetApiId = 0xFFU;
    mock_DetErrorId = 0xFFU;
    mock_DetCalls = 0;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetApiId = ApiId;
    mock_DetErrorId = ErrorId;
    mock_DetCalls++;
    return E_OK;
}

void Mcal_DisableAllInterrupts(void) {}
void Mcal_EnableAllInterrupts(void) {}

static WdgM_ConfigType testConfig;
static WdgM_ConfigType testConfigInvalid;

void setUp(void) { mock_Det_Reset(); }
void tearDown(void) {}

/* ------------------------------------------------------------------------ */
/* UNINIT-state tests (must run before the first successful WdgM_Init)      */
/* ------------------------------------------------------------------------ */

/** @req SWS_WdgM_00001 */
void test_WdgM_Init_NullPtr_ShouldFail(void) {
    Std_ReturnType ret = WdgM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(0x00U, mock_DetApiId);              /* WDGM_API_INIT */
    TEST_ASSERT_EQUAL(WDGM_E_PARAM_POINTER, mock_DetErrorId);
}

/** @req SWS_WdgM_00001 */
void test_WdgM_Init_InvalidConfig_ShouldFail(void) {
    /* failureThreshold == 0 violates WdgM_ValidateConfig */
    testConfigInvalid.watchdogs = NULL_PTR;
    testConfigInvalid.numWatchdogs = 0U;
    testConfigInvalid.entities = NULL_PTR;
    testConfigInvalid.numEntities = 0U;
    testConfigInvalid.failureThreshold = 0U;
    testConfigInvalid.supervisionCycleMs = 10U;
    testConfigInvalid.lockstepIntegration = FALSE;
    testConfigInvalid.ramSafetyIntegration = FALSE;
    Std_ReturnType ret = WdgM_Init(&testConfigInvalid);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, mock_DetErrorId);
}

/** @req SWS_WdgM_00003 */
void test_WdgM_GetState_BeforeInit_ShouldReturnUninit(void) {
    WdgM_StateType state = WdgM_GetState();
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, state);
}

/** @req SWS_WdgM_00007 */
void test_WdgM_CheckpointReached_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = WdgM_CheckpointReached(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, mock_DetErrorId);
}

/**
 * @req SWS_WdgM_00017
 * The initialized path ends in WdgM_PerformReset()'s infinite loop, so only
 * the UNINIT guard is observable without hanging the test runner.
 */
void test_WdgM_HandleLockstepError_BeforeInit_ReportsDet(void) {
    WdgM_HandleLockstepError(0x01U);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(0x20U, mock_DetApiId);              /* WDGM_API_HANDLE_LOCKSTEP_ERROR */
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, mock_DetErrorId);
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, WdgM_GetState());
}

/**
 * @req SWS_WdgM_00018
 * Same restriction as HandleLockstepError: the initialized path would loop
 * forever in WdgM_PerformReset().
 */
void test_WdgM_HandleRamSafetyError_BeforeInit_ReportsDet(void) {
    WdgM_HandleRamSafetyError(0x01U);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(0x21U, mock_DetApiId);              /* WDGM_API_HANDLE_RAMSAFETY_ERROR */
    TEST_ASSERT_EQUAL(WDGM_E_NOT_INITIALIZED, mock_DetErrorId);
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, WdgM_GetState());
}

/** @req SWS_WdgM_00012 */
void test_WdgM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    WdgM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(WDGM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(0x0DU, info.moduleID);       /* WDGM_MODULE_ID */
    TEST_ASSERT_EQUAL(1U, info.sw_major_version);
    TEST_ASSERT_EQUAL(0U, info.sw_minor_version);
    TEST_ASSERT_EQUAL(0U, info.sw_patch_version);
}

/**
 * @req SWS_WdgM_00012
 * Actual SUT behavior: WdgM_GetVersionInfo() silently ignores NULL (the
 * pointer is guarded, no Det report is raised).
 */
void test_WdgM_GetVersionInfo_NullPtr_NoDetNoCrash(void) {
    WdgM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(0U, mock_DetCalls);
}

/* ------------------------------------------------------------------------ */
/* ACTIVE-state tests (module stays initialized for the rest of the run)    */
/* ------------------------------------------------------------------------ */

/** @req SWS_WdgM_00001 */
void test_WdgM_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.watchdogs = NULL_PTR;
    testConfig.numWatchdogs = 0U;
    testConfig.entities = NULL_PTR;
    testConfig.numEntities = 0U;
    testConfig.failureThreshold = WDGM_CFG_FAILURE_THRESHOLD;
    testConfig.supervisionCycleMs = WDGM_CFG_SUPERVISION_CYCLE_MS;
    testConfig.lockstepIntegration = FALSE;
    testConfig.ramSafetyIntegration = FALSE;
    Std_ReturnType ret = WdgM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/** @req SWS_WdgM_00001 */
void test_WdgM_Init_AlreadyInitialized_ShouldFail(void) {
    Std_ReturnType ret = WdgM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(WDGM_E_ALREADY_INITIALIZED, mock_DetErrorId);
}

/**
 * @req SWS_WdgM_00002
 * Actual SUT behavior: DeInit is always rejected because WdgM_IsDisableAllowed()
 * never returns TRUE (WdgM_DisableAllowed has no setter), so the module can
 * never be deinitialized once initialized.
 */
void test_WdgM_DeInit_AfterInit_NotAllowed(void) {
    Std_ReturnType ret = WdgM_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(WDGM_E_DISABLE_NOT_ALLOWED, mock_DetErrorId);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/** @req SWS_WdgM_00003 */
void test_WdgM_GetState_AfterInit_ShouldReturnActive(void) {
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/** @req SWS_WdgM_00004 */
void test_WdgM_SetMode_ValidMode_ShouldSucceed(void) {
    Std_ReturnType ret = WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_SLOW, WdgM_GetMode());
}

/** @req SWS_WdgM_00005 */
void test_WdgM_GetMode_AfterSet_ShouldReturnSetMode(void) {
    Std_ReturnType ret = WdgM_SetMode(WDGM_WATCHDOG_MODE_FAST);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_FAST, WdgM_GetMode());
}

/**
 * @req SWS_WdgM_00004
 * WDGM_WATCHDOG_MODE_OFF requires WdgM_IsDisableAllowed() == TRUE, which the
 * SUT never allows; expect rejection with WDGM_E_DISABLE_NOT_ALLOWED.
 */
void test_WdgM_SetMode_Off_NotAllowed(void) {
    Std_ReturnType ret = WdgM_SetMode(WDGM_WATCHDOG_MODE_OFF);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(WDGM_E_DISABLE_NOT_ALLOWED, mock_DetErrorId);
    /* previous mode retained */
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_FAST, WdgM_GetMode());
}

/** @req SWS_WdgM_00004 */
void test_WdgM_SetMode_InvalidMode_ShouldFail(void) {
    Std_ReturnType ret = WdgM_SetMode(WDGM_WATCHDOG_MODE_FAST + 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL(WDGM_E_PARAM_MODE, mock_DetErrorId);
}

/**
 * @req SWS_WdgM_00007
 * With no configured entities, runtime slot 0 (seId 0) matches seId 0, so the
 * checkpoint is accepted.
 */
void test_WdgM_CheckpointReached_ValidId_ShouldSucceed(void) {
    Std_ReturnType ret = WdgM_CheckpointReached(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0U, mock_DetCalls);
}

/** @req SWS_WdgM_00008 */
void test_WdgM_UpdateAliveIndication_ValidId_ShouldSucceed(void) {
    Std_ReturnType ret = WdgM_UpdateAliveIndication(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/**
 * @req SWS_WdgM_00009
 * No entities are configured, so the matched runtime slot stays in the
 * DEACTIVATED state.
 */
void test_WdgM_GetSEState_ValidId_ShouldSucceed(void) {
    WdgM_SEStateType state = WDGM_SE_STATE_CORRECT;
    Std_ReturnType ret = WdgM_GetSEState(0U, &state);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDGM_SE_STATE_DEACTIVATED, state);
}

/** @req SWS_WdgM_00013 */
void test_WdgM_GetGlobalStatus_AfterInit_ShouldSucceed(void) {
    WdgM_GlobalStatusType status;
    Std_ReturnType ret = WdgM_GetGlobalStatus(&status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(WDGM_WATCHDOG_MODE_FAST, status.currentMode);
}

/**
 * @req SWS_WdgM_00013
 * With supervisionCycleMs = 10, twelve MainFunction calls must trigger the
 * underlying watchdog at least once (totalRefreshes increments).
 */
void test_WdgM_MainFunction_AfterInit_TriggersWatchdog(void) {
    uint8 i;
    WdgM_GlobalStatusType status;
    for (i = 0U; i < 12U; i++) {
        WdgM_MainFunction();
    }
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
    TEST_ASSERT_EQUAL(E_OK, WdgM_GetGlobalStatus(&status));
    TEST_ASSERT_TRUE(status.totalRefreshes >= 1U);
}

/** @req SWS_WdgM_00016 */
void test_WdgM_GetFirstExpiredSEID_NoExpiry_ShouldFail(void) {
    uint16 seId = 0xFFFFU;
    Std_ReturnType ret = WdgM_GetFirstExpiredSEID(&seId);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_WdgM_00010 */
void test_WdgM_DeactivateSE_ValidId_ShouldSucceed(void) {
    Std_ReturnType ret = WdgM_DeactivateSupervisionEntity(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00011 */
void test_WdgM_ActivateSE_ValidId_ShouldSucceed(void) {
    Std_ReturnType ret = WdgM_ActivateSupervisionEntity(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/**
 * @req SWS_WdgM_00006
 * Actual SUT behavior: WdgM_DisableAllowed is never set, so this is always
 * FALSE.
 */
void test_WdgM_IsDisableAllowed_AfterInit_ShouldReturnFalse(void) {
    TEST_ASSERT_EQUAL(FALSE, WdgM_IsDisableAllowed());
}

/* ------------------------------------------------------------------------ */
/* Test runner                                                              */
/* ------------------------------------------------------------------------ */
int main(void) {
    UNITY_BEGIN();
    /* UNINIT-state tests first: the module cannot be deinitialized once
     * initialized, so state-dependent ordering is mandatory. */
    RUN_TEST(test_WdgM_Init_NullPtr_ShouldFail);
    RUN_TEST(test_WdgM_Init_InvalidConfig_ShouldFail);
    RUN_TEST(test_WdgM_GetState_BeforeInit_ShouldReturnUninit);
    RUN_TEST(test_WdgM_CheckpointReached_BeforeInit_ShouldFail);
    RUN_TEST(test_WdgM_HandleLockstepError_BeforeInit_ReportsDet);
    RUN_TEST(test_WdgM_HandleRamSafetyError_BeforeInit_ReportsDet);
    RUN_TEST(test_WdgM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_WdgM_GetVersionInfo_NullPtr_NoDetNoCrash);
    /* From here on the module stays in WDGM_STATE_ACTIVE. */
    RUN_TEST(test_WdgM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_WdgM_Init_AlreadyInitialized_ShouldFail);
    RUN_TEST(test_WdgM_DeInit_AfterInit_NotAllowed);
    RUN_TEST(test_WdgM_GetState_AfterInit_ShouldReturnActive);
    RUN_TEST(test_WdgM_SetMode_ValidMode_ShouldSucceed);
    RUN_TEST(test_WdgM_GetMode_AfterSet_ShouldReturnSetMode);
    RUN_TEST(test_WdgM_SetMode_Off_NotAllowed);
    RUN_TEST(test_WdgM_SetMode_InvalidMode_ShouldFail);
    RUN_TEST(test_WdgM_CheckpointReached_ValidId_ShouldSucceed);
    RUN_TEST(test_WdgM_UpdateAliveIndication_ValidId_ShouldSucceed);
    RUN_TEST(test_WdgM_GetSEState_ValidId_ShouldSucceed);
    RUN_TEST(test_WdgM_GetGlobalStatus_AfterInit_ShouldSucceed);
    RUN_TEST(test_WdgM_MainFunction_AfterInit_TriggersWatchdog);
    RUN_TEST(test_WdgM_GetFirstExpiredSEID_NoExpiry_ShouldFail);
    RUN_TEST(test_WdgM_DeactivateSE_ValidId_ShouldSucceed);
    RUN_TEST(test_WdgM_ActivateSE_ValidId_ShouldSucceed);
    RUN_TEST(test_WdgM_IsDisableAllowed_AfterInit_ShouldReturnFalse);
    return UnityEnd();
}

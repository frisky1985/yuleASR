/**
 * @file test_ecum.c
 * @brief EcuM (ECU State Manager) Unit Tests
 * @req SWS_EcuM
 */

// @tests src/bsw/services/ecum/src/EcuM.c  @tests src/bsw/services/ecum/include/EcuM.h
#include "unity.h"
#include "EcuM.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_EcuM_00001 */
void test_EcuM_Init_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_StateType state;
    EcuM_GetState(&state);
    TEST_ASSERT_NOT_EQUAL(ECUM_STATE_UNINIT, state);
}

/** @req SWS_EcuM_00002 */
void test_EcuM_GetState_AfterInit_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_StateType state;
    Std_ReturnType ret = EcuM_GetState(&state);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00002 */
void test_EcuM_GetState_NullPtr_ShouldFail(void) {
    EcuM_Init();
    Std_ReturnType ret = EcuM_GetState(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_EcuM_00003 */
void test_EcuM_RequestRUN_ValidUser_ShouldSucceed(void) {
    EcuM_Init();
    Std_ReturnType ret = EcuM_RequestRUN(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00004 */
void test_EcuM_ReleaseRUN_AfterRequest_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_RequestRUN(0U);
    Std_ReturnType ret = EcuM_ReleaseRUN(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00005 */
void test_EcuM_SelectShutdownTarget_ValidTarget_ShouldSucceed(void) {
    EcuM_Init();
    Std_ReturnType ret = EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_OFF, 0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00006 */
void test_EcuM_GetShutdownTarget_AfterSelect_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_OFF, 0U);
    EcuM_ShutdownTargetType target; uint8 mode;
    Std_ReturnType ret = EcuM_GetShutdownTarget(&target, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00007 */
void test_EcuM_SetWakeupEvent_ShouldNotCrash(void) {
    EcuM_Init();
    EcuM_SetWakeupEvent(ECUM_WAKEUP_SOURCE_TIMER);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuM_00008 */
void test_EcuM_ClearWakeupEvent_ShouldNotCrash(void) {
    EcuM_Init();
    EcuM_ClearWakeupEvent(ECUM_WAKEUP_SOURCE_TIMER);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuM_00009 */
void test_EcuM_CheckWakeup_ShouldNotCrash(void) {
    EcuM_Init();
    EcuM_CheckWakeup(ECUM_WAKEUP_SOURCE_TIMER);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuM_00010 */
void test_EcuM_EnableWakeupSources_ShouldSucceed(void) {
    EcuM_Init();
    Std_ReturnType ret = EcuM_EnableWakeupSources(ECUM_WAKEUP_SOURCE_TIMER);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00011 */
void test_EcuM_DisableWakeupSources_ShouldSucceed(void) {
    EcuM_Init();
    Std_ReturnType ret = EcuM_DisableWakeupSources(ECUM_WAKEUP_SOURCE_TIMER);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00012 */
void test_EcuM_GetSubState_AfterInit_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_SubStateType subState;
    Std_ReturnType ret = EcuM_GetSubState(&subState);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_EcuM_00013 */
void test_EcuM_KillAllRUNRequests_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_RequestRUN(0U);
    Std_ReturnType ret = EcuM_KillAllRUNRequests();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_EcuM_MainFunction_ShouldNotCrash(void) {
    EcuM_Init();
    EcuM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

void test_EcuM_GoSleep_ShouldNotCrash(void) {
    EcuM_Init();
    EcuM_GoSleep();
    TEST_ASSERT_TRUE(1);
}

void test_EcuM_Shutdown_ShouldNotCrash(void) {
    EcuM_Init();
    EcuM_Shutdown();
    TEST_ASSERT_TRUE(1);
}

void test_EcuM_GetLastShutdownTarget_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_ShutdownTargetType target; uint8 mode;
    Std_ReturnType ret = EcuM_GetLastShutdownTarget(&target, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_EcuM_SelectShutdownCause_ShouldSucceed(void) {
    EcuM_Init();
    Std_ReturnType ret = EcuM_SelectShutdownCause(ECUM_SHUTDOWN_CAUSE_POWER_OFF);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_EcuM_GetShutdownCause_ShouldSucceed(void) {
    EcuM_Init();
    EcuM_ShutdownCauseType cause;
    Std_ReturnType ret = EcuM_GetShutdownCause(&cause);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

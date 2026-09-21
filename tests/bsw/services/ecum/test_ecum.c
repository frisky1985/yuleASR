/**
 * @file test_ecum.c
 * @brief EcuM (ECU State Manager) Unit Tests — Substantiated
 * @req SWS_EcuM
 *
 * Substantiation: state machine transitions, DET parameter validation,
 * null pointer checks, API ordering checks, RUN request lifecycle.
 * EcuM is pure software logic — no hardware register mock needed.
 */

#include <setjmp.h>
#include "unity.h"
#include "mock_det.h"

#include "EcuM.h"
#include "EcuM_Cfg.h"

/* Constants from EcuM.c */
#define TEST_ECUM_MODULE_ID     (0x0Au)
#define TEST_ECUM_INSTANCE_ID   (0x00u)

/* Longjmp buffer used to escape EcuM_PerformShutdown's infinite loop.
 * EcuM_AL_SwitchOff is overridden below to assert the shutdown state
 * and longjmp back to the test before the while(1) is reached. */
static jmp_buf g_ecum_shutdown_jmp;

void setUp(void) {
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Init / Startup --- */

/** @req SWS_EcuM_00001 */
void test_EcuM_Init_ShouldTransitionToRun(void) {
    EcuM_Init();
    EcuM_StateType state = 0xFFU;
    (void)EcuM_GetState(&state);
    TEST_ASSERT_EQUAL(ECUM_STATE_RUN, state);
}

/** @req SWS_EcuM_00010 */
void test_EcuM_StartupOne_BeforeInit_ShouldReportDet(void) {
    EcuM_StartupOne();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(TEST_ECUM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ECUM_STARTUPONE_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00010 */
void test_EcuM_StartupOne_AfterInit_ShouldReportWrongApiOrder(void) {
    /* EcuM_Init() already drives the complete startup sequence, so the
     * sub-state is no longer ECUM_SUBSTATE_STARTUP_ONE. */
    Det_Mock_Reset();
    EcuM_StartupOne();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_STARTUPONE_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_WRONG_API_ORDER, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00011 */
void test_EcuM_StartupTwo_BeforeInit_ShouldReportDet(void) {
    EcuM_StartupTwo();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(TEST_ECUM_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(ECUM_STARTUPTWO_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00011 */
void test_EcuM_StartupTwo_AfterInit_ShouldReportWrongApiOrder(void) {
    Det_Mock_Reset();
    EcuM_StartupTwo();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_STARTUPTWO_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_WRONG_API_ORDER, Det_MockData.ErrorId);
}

/* --- GetState --- */

/** @req SWS_EcuM_00002 */
void test_EcuM_GetState_BeforeInit_ShouldReportDet(void) {
    EcuM_StateType state;
    (void)EcuM_GetState(&state);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_GETSTATE_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00002 */
void test_EcuM_GetState_NullPtr_ShouldReportDet(void) {
    Det_Mock_Reset();
    (void)EcuM_GetState(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_GETSTATE_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NULL_POINTER, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00002 */
void test_EcuM_GetState_AfterInit_ShouldReturnRun(void) {
    Det_Mock_Reset();
    EcuM_StateType state = 0xFFU;
    Std_ReturnType ret = EcuM_GetState(&state);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(ECUM_STATE_RUN, state);
}

/* --- Shutdown --- */

/** @req SWS_EcuM_00003 */
void test_EcuM_Shutdown_BeforeInit_ShouldReportDet(void) {
    EcuM_Shutdown();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_SHUTDOWN_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00003 */
void test_EcuM_Shutdown_AfterInit_ShouldTransitionToShutdown(void) {
    Det_Mock_Reset();
    if (setjmp(g_ecum_shutdown_jmp) == 0) {
        EcuM_Shutdown();
        TEST_FAIL_MESSAGE("EcuM_Shutdown should not return in test environment");
    }

    EcuM_StateType state = 0xFFU;
    (void)EcuM_GetState(&state);
    TEST_ASSERT_EQUAL(ECUM_STATE_SHUTDOWN, state);
}

/* --- RequestRUN / ReleaseRUN --- */

/** @req SWS_EcuM_00004 */
void test_EcuM_RequestRUN_BeforeInit_ShouldReportDet(void) {
    (void)EcuM_RequestRUN(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_REQUESTRUN_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00004 */
void test_EcuM_ReleaseRUN_BeforeInit_ShouldReportDet(void) {
    (void)EcuM_ReleaseRUN(0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_RELEASERUN_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00004 */
void test_EcuM_RequestRUN_AfterInit_ShouldSucceed(void) {
    Det_Mock_Reset();
    Std_ReturnType ret = EcuM_RequestRUN(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_EcuM_00004 */
void test_EcuM_ReleaseRUN_AfterInit_ShouldSucceedAndEnterPostRun(void) {
    Det_Mock_Reset();
    (void)EcuM_RequestRUN(0U);
    Std_ReturnType ret = EcuM_ReleaseRUN(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);

    EcuM_StateType state = 0xFFU;
    (void)EcuM_GetState(&state);
    TEST_ASSERT_EQUAL(ECUM_STATE_POST_RUN, state);
}

/* --- SelectShutdownTarget --- */

/** @req SWS_EcuM_00005 */
void test_EcuM_SelectShutdownTarget_BeforeInit_ShouldReportDet(void) {
    (void)EcuM_SelectShutdownTarget(0U, 0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_SELECTSHUTDOWNTARGET_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/* --- GetShutdownTarget --- */

/** @req SWS_EcuM_00006 */
void test_EcuM_GetShutdownTarget_NullPtr_ShouldReportDet(void) {
    Det_Mock_Reset();
    (void)EcuM_GetShutdownTarget(NULL_PTR, NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_GETSHUTDOWNTARGET_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NULL_POINTER, Det_MockData.ErrorId);
}

/* --- ComM RequestComMode --- */

/** @req SWS_EcuM_00007 */
void test_EcuM_ComM_RequestComMode_BeforeInit_ShouldReportDet(void) {
    (void)EcuM_ComM_RequestComMode(0U, 0U);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_COMMODEREQUEST_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/* --- MainFunction --- */

/** @req SWS_EcuM_00008 */
void test_EcuM_MainFunction_BeforeInit_ShouldReportDet(void) {
    EcuM_MainFunction();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_MAINFUNCTION_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/* --- KillAllRUNRequests --- */

/** @req SWS_EcuM_00092 */
void test_EcuM_KillAllRUNRequests_BeforeInit_ShouldReportDet(void) {
    (void)EcuM_KillAllRUNRequests();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(ECUM_KILLALLRUNREQUESTS_SID, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(ECUM_E_NOT_INITIALIZED, Det_MockData.ErrorId);
}

/** @req SWS_EcuM_00092 */
void test_EcuM_KillAllRUNRequests_AfterInit_ShouldSucceed(void) {
    Det_Mock_Reset();
    Std_ReturnType ret = EcuM_KillAllRUNRequests();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/* --- Stubs for dependencies referenced by EcuM.c --- */

void BswM_EcuM_CurrentState(EcuM_StateType CurrentState)
{
    (void)CurrentState;
}

void BswM_EcuM_CurrentWakeup(EcuM_WakeupSourceType WakeupSource,
                             EcuM_WakeupStatusType WakeupStatus)
{
    (void)WakeupSource;
    (void)WakeupStatus;
}

void BswM_Init(const void* config)
{
    (void)config;
}

void SchM_Init(const void* config)
{
    (void)config;
}

void SchM_DeInit(void)
{
}

void Rte_Start(void)
{
}

void ComM_Init(const void* config)
{
    (void)config;
}

void ComM_DeInit(void)
{
}

void NvM_Init(const void* config)
{
    (void)config;
}

void NvM_ReadAll(void)
{
}

void NvM_WriteAll(void)
{
}

/* Override the weak abstraction-layer callout so the shutdown sequence
 * can be verified without hanging the test runner. */
void EcuM_AL_SwitchOff(void)
{
    EcuM_StateType state = 0xFFU;
    (void)EcuM_GetState(&state);
    TEST_ASSERT_EQUAL(ECUM_STATE_SHUTDOWN, state);
    longjmp(g_ecum_shutdown_jmp, 1);
}

/* --- Test runner --- */

int main(void)
{
    UNITY_BEGIN();

    /* API-order / not-initialized checks while EcuM is still OFF. */
    RUN_TEST(test_EcuM_StartupOne_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_StartupTwo_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_GetState_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_Shutdown_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_RequestRUN_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_ReleaseRUN_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_SelectShutdownTarget_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_ComM_RequestComMode_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_MainFunction_BeforeInit_ShouldReportDet);
    RUN_TEST(test_EcuM_KillAllRUNRequests_BeforeInit_ShouldReportDet);

    /* Full startup via EcuM_Init(). */
    RUN_TEST(test_EcuM_Init_ShouldTransitionToRun);

    /* RUN-phase behavior. */
    RUN_TEST(test_EcuM_GetState_NullPtr_ShouldReportDet);
    RUN_TEST(test_EcuM_GetState_AfterInit_ShouldReturnRun);
    RUN_TEST(test_EcuM_StartupOne_AfterInit_ShouldReportWrongApiOrder);
    RUN_TEST(test_EcuM_StartupTwo_AfterInit_ShouldReportWrongApiOrder);
    RUN_TEST(test_EcuM_RequestRUN_AfterInit_ShouldSucceed);
    RUN_TEST(test_EcuM_ReleaseRUN_AfterInit_ShouldSucceedAndEnterPostRun);
    RUN_TEST(test_EcuM_GetShutdownTarget_NullPtr_ShouldReportDet);
    RUN_TEST(test_EcuM_KillAllRUNRequests_AfterInit_ShouldSucceed);

    /* Shutdown must be last — EcuM_AL_SwitchOff escapes via longjmp. */
    RUN_TEST(test_EcuM_Shutdown_AfterInit_ShouldTransitionToShutdown);

    return UNITY_END();
}

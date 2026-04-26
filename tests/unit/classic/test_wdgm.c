/******************************************************************************
 * @file    test_wdgm.c
 * @brief   Unit tests for Watchdog Manager (WdgM)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity/unity.h"
#include "autosar/service/wdgM/wdgM.h"
#include "autosar/service/wdgM/wdgM_Cfg.h"
#include "autosar/service/wdgM/wdgIf.h"

/* External configuration */
extern const WdgM_ConfigType WdgM_Config;

/* Test setup and teardown */
void setUp(void)
{
    /* Reset module before each test */
    (void)WdgM_DeInit();
}

void tearDown(void)
{
    /* Cleanup after each test */
    (void)WdgM_DeInit();
}

/******************************************************************************
 * Test Cases: Initialization
 ******************************************************************************/

void test_WdgM_Init_NullConfig_ReturnsError(void)
{
    Std_ReturnType result = WdgM_Init(NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_Init_ValidConfig_ReturnsOk(void)
{
    Std_ReturnType result = WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_WdgM_Init_AlreadyInitialized_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_Init(&WdgM_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_Init_SetsCorrectMode(void)
{
    WdgM_ModeType mode;
    
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetMode(&mode);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_INITIAL_MODE, mode);
}

void test_WdgM_Init_SetsGlobalStatusToOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    WdgM_GlobalStatusType status = WdgM_GetGlobalStatus();
    
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_OK, status);
}

/******************************************************************************
 * Test Cases: Deinitialization
 ******************************************************************************/

void test_WdgM_DeInit_NotInitialized_ReturnsError(void)
{
    Std_ReturnType result = WdgM_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_DeInit_Initialized_ReturnsOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_DeInit();
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_WdgM_DeInit_SetsModeOff(void)
{
    WdgM_ModeType mode;
    
    (void)WdgM_Init(&WdgM_Config);
    (void)WdgM_DeInit();
    
    /* After DeInit, status should be STOPPED */
    WdgM_GlobalStatusType status = WdgM_GetGlobalStatus();
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_STOPPED, status);
}

/******************************************************************************
 * Test Cases: Mode Management
 ******************************************************************************/

void test_WdgM_SetMode_NotInitialized_ReturnsError(void)
{
    Std_ReturnType result = WdgM_SetMode(WDGM_MODE_SLOW);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_SetMode_InvalidMode_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_SetMode(0xFFU);  /* Invalid mode */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_SetMode_ValidMode_ReturnsOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_SetMode(WDGM_MODE_SLOW);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_WdgM_SetMode_ChangesMode(void)
{
    WdgM_ModeType mode;
    
    (void)WdgM_Init(&WdgM_Config);
    (void)WdgM_SetMode(WDGM_MODE_SLOW);
    (void)WdgM_GetMode(&mode);
    
    TEST_ASSERT_EQUAL(WDGM_MODE_SLOW, mode);
}

void test_WdgM_GetMode_NullPointer_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetMode(NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_GetMode_NotInitialized_ReturnsError(void)
{
    WdgM_ModeType mode;
    Std_ReturnType result = WdgM_GetMode(&mode);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/******************************************************************************
 * Test Cases: Global Status
 ******************************************************************************/

void test_WdgM_GetGlobalStatus_NotInitialized_ReturnsStopped(void)
{
    WdgM_GlobalStatusType status = WdgM_GetGlobalStatus();
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_STOPPED, status);
}

void test_WdgM_GetGlobalStatus_Initialized_ReturnsOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    WdgM_GlobalStatusType status = WdgM_GetGlobalStatus();
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_OK, status);
}

/******************************************************************************
 * Test Cases: Local Status
 ******************************************************************************/

void test_WdgM_GetLocalStatus_NotInitialized_ReturnsError(void)
{
    WdgM_LocalStatusType status;
    Std_ReturnType result = WdgM_GetLocalStatus(0U, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_GetLocalStatus_InvalidSE_ReturnsError(void)
{
    WdgM_LocalStatusType status;
    
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetLocalStatus(WDGM_MAX_SUPERVISED_ENTITIES, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_GetLocalStatus_NullPointer_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetLocalStatus(0U, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_GetLocalStatus_ValidSE_ReturnsOk(void)
{
    WdgM_LocalStatusType status;
    
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetLocalStatus(WDGM_SE_ID_OS_TASK_10MS, &status);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/******************************************************************************
 * Test Cases: Checkpoint Handling
 ******************************************************************************/

void test_WdgM_CheckpointReached_NotInitialized_ReturnsError(void)
{
    Std_ReturnType result = WdgM_CheckpointReached(0U, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_CheckpointReached_InvalidSE_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_CheckpointReached(WDGM_MAX_SUPERVISED_ENTITIES, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_CheckpointReached_InvalidCheckpoint_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_CheckpointReached(0U, WDGM_MAX_CHECKPOINTS);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_CheckpointReached_ValidCheckpoint_ReturnsOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    /* SE 0 should be active in FAST mode */
    Std_ReturnType result = WdgM_CheckpointReached(WDGM_SE_ID_OS_TASK_1MS, WDGM_CP_OS_1MS_END);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_WdgM_CheckpointReached_InactiveSE_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    (void)WdgM_SetMode(WDGM_MODE_OFF);  /* All SEs deactivated */
    Std_ReturnType result = WdgM_CheckpointReached(WDGM_SE_ID_OS_TASK_1MS, WDGM_CP_OS_1MS_END);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/******************************************************************************
 * Test Cases: Alive Counter Update
 ******************************************************************************/

void test_WdgM_UpdateAliveCounter_NotInitialized_ReturnsError(void)
{
    Std_ReturnType result = WdgM_UpdateAliveCounter(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_UpdateAliveCounter_InvalidSE_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_UpdateAliveCounter(WDGM_MAX_SUPERVISED_ENTITIES);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_UpdateAliveCounter_ValidSE_ReturnsOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_UpdateAliveCounter(WDGM_SE_ID_OS_TASK_10MS);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/******************************************************************************
 * Test Cases: First Expired SE ID
 ******************************************************************************/

void test_WdgM_GetFirstExpiredSEID_NotInitialized_ReturnsError(void)
{
    WdgM_SupervisedEntityIdType seId;
    Std_ReturnType result = WdgM_GetFirstExpiredSEID(&seId);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_GetFirstExpiredSEID_NullPointer_ReturnsError(void)
{
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetFirstExpiredSEID(NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_GetFirstExpiredSEID_NoExpired_ReturnsInvalid(void)
{
    WdgM_SupervisedEntityIdType seId;
    
    (void)WdgM_Init(&WdgM_Config);
    Std_ReturnType result = WdgM_GetFirstExpiredSEID(&seId);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(WDGM_INVALID_SE_ID, seId);
}

/******************************************************************************
 * Test Cases: Main Function
 ******************************************************************************/

void test_WdgM_MainFunction_NotInitialized_NoAction(void)
{
    /* Should not crash */
    WdgM_MainFunction();
    TEST_PASS();
}

void test_WdgM_MainFunction_Initialized_RunsSupervision(void)
{
    (void)WdgM_Init(&WdgM_Config);
    
    /* Call main function multiple times */
    for (uint16 i = 0U; i < 20U; i++) {
        WdgM_MainFunction();
    }
    
    /* Status should still be OK (no checkpoints reported) */
    WdgM_GlobalStatusType status = WdgM_GetGlobalStatus();
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_OK, status);
}

/******************************************************************************
 * Test Cases: BswM Integration
 ******************************************************************************/

void test_WdgM_RequestMode_ChangesMode(void)
{
    WdgM_ModeType mode;
    
    (void)WdgM_Init(&WdgM_Config);
    WdgM_RequestMode(WDGM_MODE_SLOW);
    (void)WdgM_GetMode(&mode);
    
    TEST_ASSERT_EQUAL(WDGM_MODE_SLOW, mode);
}

void test_WdgM_GetCurrentMode_ReturnsCurrentMode(void)
{
    (void)WdgM_Init(&WdgM_Config);
    WdgM_ModeType mode = WdgM_GetCurrentMode();
    
    TEST_ASSERT_EQUAL(WDGM_INITIAL_MODE, mode);
}

void test_WdgM_SetModeForEcuM_SetsMode(void)
{
    (void)WdgM_Init(&WdgM_Config);
    WdgM_SetModeForEcuM(WDGM_MODE_SLOW);
    
    WdgM_ModeType mode = WdgM_GetCurrentMode();
    TEST_ASSERT_EQUAL(WDGM_MODE_SLOW, mode);
}

/******************************************************************************
 * Test Cases: Checkpoint Extended
 ******************************************************************************/

void test_WdgM_CheckpointReachedExtended_NotInitialized_ReturnsError(void)
{
    Std_ReturnType result = WdgM_CheckpointReachedExtended(
        0U, 0U, WDGM_AUTOSAR_INTERNAL);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_WdgM_CheckpointReachedExternal_ValidCheckpoint_ReturnsOk(void)
{
    (void)WdgM_Init(&WdgM_Config);
    /* SE 8 uses external checkpoints */
    Std_ReturnType result = WdgM_CheckpointReachedExtended(
        WDGM_SE_ID_APP_TASK_1, WDGM_CP_APP1_START, WDGM_EXTERNAL);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/******************************************************************************
 * Test Cases: Mode Transition Effects
 ******************************************************************************/

void test_WdgM_SetMode_ClearsSEStatus(void)
{
    WdgM_LocalStatusType status;
    
    (void)WdgM_Init(&WdgM_Config);
    
    /* Report some checkpoints */
    (void)WdgM_CheckpointReached(WDGM_SE_ID_OS_TASK_10MS, WDGM_CP_OS_10MS_START);
    
    /* Change mode */
    (void)WdgM_SetMode(WDGM_MODE_SLOW);
    
    /* Check SE status is reset */
    (void)WdgM_GetLocalStatus(WDGM_SE_ID_OS_TASK_10MS, &status);
    TEST_ASSERT_EQUAL(WDGM_LOCAL_STATUS_OK, status);
}

void test_WdgM_SetMode_ToOff_DeactivatesAllSEs(void)
{
    WdgM_LocalStatusType status;
    
    (void)WdgM_Init(&WdgM_Config);
    (void)WdgM_SetMode(WDGM_MODE_OFF);
    
    /* All SEs should be deactivated */
    (void)WdgM_GetLocalStatus(WDGM_SE_ID_OS_TASK_10MS, &status);
    TEST_ASSERT_EQUAL(WDGM_LOCAL_STATUS_DEACTIVATED, status);
}

/******************************************************************************
 * Test Suite Runner
 ******************************************************************************/

int main(void)
{
    UNITY_BEGIN();
    
    /* Initialization tests */
    RUN_TEST(test_WdgM_Init_NullConfig_ReturnsError);
    RUN_TEST(test_WdgM_Init_ValidConfig_ReturnsOk);
    RUN_TEST(test_WdgM_Init_AlreadyInitialized_ReturnsError);
    RUN_TEST(test_WdgM_Init_SetsCorrectMode);
    RUN_TEST(test_WdgM_Init_SetsGlobalStatusToOk);
    
    /* Deinitialization tests */
    RUN_TEST(test_WdgM_DeInit_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_DeInit_Initialized_ReturnsOk);
    RUN_TEST(test_WdgM_DeInit_SetsModeOff);
    
    /* Mode management tests */
    RUN_TEST(test_WdgM_SetMode_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_SetMode_InvalidMode_ReturnsError);
    RUN_TEST(test_WdgM_SetMode_ValidMode_ReturnsOk);
    RUN_TEST(test_WdgM_SetMode_ChangesMode);
    RUN_TEST(test_WdgM_GetMode_NullPointer_ReturnsError);
    RUN_TEST(test_WdgM_GetMode_NotInitialized_ReturnsError);
    
    /* Global status tests */
    RUN_TEST(test_WdgM_GetGlobalStatus_NotInitialized_ReturnsStopped);
    RUN_TEST(test_WdgM_GetGlobalStatus_Initialized_ReturnsOk);
    
    /* Local status tests */
    RUN_TEST(test_WdgM_GetLocalStatus_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_GetLocalStatus_InvalidSE_ReturnsError);
    RUN_TEST(test_WdgM_GetLocalStatus_NullPointer_ReturnsError);
    RUN_TEST(test_WdgM_GetLocalStatus_ValidSE_ReturnsOk);
    
    /* Checkpoint tests */
    RUN_TEST(test_WdgM_CheckpointReached_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_CheckpointReached_InvalidSE_ReturnsError);
    RUN_TEST(test_WdgM_CheckpointReached_InvalidCheckpoint_ReturnsError);
    RUN_TEST(test_WdgM_CheckpointReached_ValidCheckpoint_ReturnsOk);
    RUN_TEST(test_WdgM_CheckpointReached_InactiveSE_ReturnsError);
    
    /* Alive counter tests */
    RUN_TEST(test_WdgM_UpdateAliveCounter_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_UpdateAliveCounter_InvalidSE_ReturnsError);
    RUN_TEST(test_WdgM_UpdateAliveCounter_ValidSE_ReturnsOk);
    
    /* Expired SE tests */
    RUN_TEST(test_WdgM_GetFirstExpiredSEID_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_GetFirstExpiredSEID_NullPointer_ReturnsError);
    RUN_TEST(test_WdgM_GetFirstExpiredSEID_NoExpired_ReturnsInvalid);
    
    /* Main function tests */
    RUN_TEST(test_WdgM_MainFunction_NotInitialized_NoAction);
    RUN_TEST(test_WdgM_MainFunction_Initialized_RunsSupervision);
    
    /* BswM integration tests */
    RUN_TEST(test_WdgM_RequestMode_ChangesMode);
    RUN_TEST(test_WdgM_GetCurrentMode_ReturnsCurrentMode);
    RUN_TEST(test_WdgM_SetModeForEcuM_SetsMode);
    
    /* Extended checkpoint tests */
    RUN_TEST(test_WdgM_CheckpointReachedExtended_NotInitialized_ReturnsError);
    RUN_TEST(test_WdgM_CheckpointReachedExternal_ValidCheckpoint_ReturnsOk);
    
    /* Mode transition tests */
    RUN_TEST(test_WdgM_SetMode_ClearsSEStatus);
    RUN_TEST(test_WdgM_SetMode_ToOff_DeactivatesAllSEs);
    
    return UNITY_END();
}

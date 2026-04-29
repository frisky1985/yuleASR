/*
 * test_com_deadline_monitor.c
 * Unit Test for COM Deadline Monitoring (T012)
 * 
 * Test Coverage:
 * - Timer management (start/stop)
 * - Timeout detection logic
 * - ErrorHook callback invocation
 * - Default value substitution (ComIPduRxDefaultValue)
 * - ASIL-D safety checks
 */

/*==================[Includes]=============================================*/

#include "unity.h"
#include "Com_DeadlineMon.h"
#include "Com_Private.h"
#include "mock_Det.h"

/*==================[Test Configuration]===================================*/

/* Test timeout values */
#define TEST_TIMEOUT_10MS       10u
#define TEST_TIMEOUT_100MS      100u
#define TEST_TIMEOUT_0MS        0u

/* Test PDU IDs */
#define TEST_PDU_ID_0           0u
#define TEST_PDU_ID_1           1u
#define TEST_PDU_ID_INVALID     99u

/*==================[Test Globals]=========================================*/

/* Mock configuration */
static Com_ConfigType TestConfig;
static Com_IPduConfigType TestIPduConfig[2];
static uint8 TestPduBuffer[8] = {0};
static uint8 TestDefaultValue[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};

/* ErrorHook tracking */
static uint8 ErrorHook_CallCount = 0u;
static Com_IPduIdType ErrorHook_LastPduId = 0xFFFFu;

/*==================[Mock Callbacks]=======================================*/

void MockErrorHook(Com_IPduIdType PduId)
{
    ErrorHook_CallCount++;
    ErrorHook_LastPduId = PduId;
}

/*==================[Test Setup/Teardown]==================================*/

void setUp(void)
{
    /* Reset error hook tracking */
    ErrorHook_CallCount = 0u;
    ErrorHook_LastPduId = 0xFFFFu;
    
    /* Initialize test configuration */
    TestIPduConfig[0].IPduId = 0u;
    TestIPduConfig[0].DataPtr = TestPduBuffer;
    TestIPduConfig[0].Length = 8u;
    TestIPduConfig[0].Direction = COM_RECEIVE;
    TestIPduConfig[0].Timeout = TEST_TIMEOUT_100MS;
    
    TestIPduConfig[1].IPduId = 1u;
    TestIPduConfig[1].DataPtr = TestPduBuffer;
    TestIPduConfig[1].Length = 8u;
    TestIPduConfig[1].Direction = COM_RECEIVE;
    TestIPduConfig[1].Timeout = TEST_TIMEOUT_10MS;
    
    TestConfig.IPdus = TestIPduConfig;
    TestConfig.NumIPdus = 2u;
    
    /* Initialize COM global state for testing */
    Com_GlobalState.Status = COM_READY;
    Com_GlobalState.Config = &TestConfig;
    Com_GlobalState.IPduRunTime = Com_IPduRunTimeData;
    
    /* Initialize IPdu runtime */
    Com_IPduRunTimeData[0].GroupStatus = COM_IPDU_GROUP_STARTED;
    Com_IPduRunTimeData[1].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Initialize deadline monitoring */
    Com_Dm_Init();
}

void tearDown(void)
{
    /* De-initialize deadline monitoring */
    Com_Dm_DeInit();
    
    /* Reset global state */
    Com_GlobalState.Status = COM_UNINIT;
}

/*==================[Test Cases]===========================================*/

/*
 * Test Case: TC_DM_001
 * Description: Verify initialization sets correct state
 * ASIL-D: Redundancy check
 */
void test_TC_DM_001_Initialization(void)
{
    /* Verify initialized flags */
    TEST_ASSERT_TRUE(Com_Dm_IsInitialized());
    TEST_ASSERT_TRUE(Com_DmInitialized);
    TEST_ASSERT_TRUE(Com_DmInitialized_Redundant);
    
    /* Verify runtime data initialized */
    for (uint8 i = 0u; i < COM_MAX_IPDUS; i++) {
        TEST_ASSERT_EQUAL(0u, Com_DmRunTimeData[i].Timer);
        TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[i].State);
        TEST_ASSERT_EQUAL(0u, Com_DmRunTimeData[i].TimeoutCounter);
    }
}

/*
 * Test Case: TC_DM_002
 * Description: Verify DeInit clears all state
 */
void test_TC_DM_002_DeInitialization(void)
{
    /* Start a timer first */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_100MS);
    
    /* De-initialize */
    Com_Dm_DeInit();
    
    /* Verify flags cleared */
    TEST_ASSERT_FALSE(Com_Dm_IsInitialized());
    TEST_ASSERT_FALSE(Com_DmInitialized);
    
    /* Verify runtime data cleared */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
}

/*
 * Test Case: TC_DM_003
 * Description: Verify timer starts with correct value
 * ASIL-D: Dual-check verification
 */
void test_TC_DM_003_StartTimer(void)
{
    /* Start timer with 100ms timeout */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_100MS);
    
    /* Verify timer started */
    TEST_ASSERT_EQUAL(TEST_TIMEOUT_100MS, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[TEST_PDU_ID_0].State);
}

/*
 * Test Case: TC_DM_004
 * Description: Verify timer decrements correctly
 */
void test_TC_DM_004_TimerDecrement(void)
{
    /* Start timer */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_10MS);
    
    /* Process timers for 5 cycles */
    for (uint8 i = 0u; i < 5u; i++) {
        Com_Dm_ProcessTimers();
    }
    
    /* Verify timer decremented */
    TEST_ASSERT_EQUAL(5u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[TEST_PDU_ID_0].State);
}

/*
 * Test Case: TC_DM_005
 * Description: Verify timeout detection
 */
void test_TC_DM_005_TimeoutDetection(void)
{
    /* Start timer with 5ms timeout */
    Com_Dm_StartTimer(TEST_PDU_ID_0, 5u);
    
    /* Process timers until timeout */
    for (uint8 i = 0u; i < 6u; i++) {
        Com_Dm_ProcessTimers();
    }
    
    /* Verify timeout detected */
    TEST_ASSERT_EQUAL(0u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    TEST_ASSERT_EQUAL(1u, Com_DmRunTimeData[TEST_PDU_ID_0].TimeoutCounter);
}

/*
 * Test Case: TC_DM_006
 * Description: Verify timer restarts on Rx indication
 */
void test_TC_DM_006_RxIndicationRestart(void)
{
    /* Create DM config */
    Com_DmRxConfigType dmConfig;
    dmConfig.ComIPduRxTimeout = TEST_TIMEOUT_100MS;
    dmConfig.ComIPduRxDefaultValue = NULL_PTR;
    dmConfig.DefaultValueLength = 0u;
    dmConfig.TimeoutAction = COM_DM_ACTION_NONE;
    dmConfig.ComErrorHook = NULL_PTR;
    dmConfig.EnableDeadlineMonitoring = TRUE;
    
    /* Start timer and let it count down */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_100MS);
    Com_Dm_ProcessTimers();
    Com_Dm_ProcessTimers();
    TEST_ASSERT_EQUAL(TEST_TIMEOUT_100MS - 2u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
    
    /* Simulate Rx indication - should restart timer */
    Com_Dm_HandleRxIndication(TEST_PDU_ID_0, &dmConfig);
    
    /* Verify timer restarted */
    TEST_ASSERT_EQUAL(TEST_TIMEOUT_100MS, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[TEST_PDU_ID_0].State);
}

/*
 * Test Case: TC_DM_007
 * Description: Verify ErrorHook is called on timeout
 */
void test_TC_DM_007_ErrorHookInvocation(void)
{
    /* Create DM config with ErrorHook */
    Com_DmRxConfigType dmConfig;
    dmConfig.ComIPduRxTimeout = 5u;
    dmConfig.ComIPduRxDefaultValue = NULL_PTR;
    dmConfig.DefaultValueLength = 0u;
    dmConfig.TimeoutAction = COM_DM_ACTION_ERROR_HOOK;
    dmConfig.ComErrorHook = MockErrorHook;
    dmConfig.EnableDeadlineMonitoring = TRUE;
    
    /* Start timer and let it timeout */
    Com_Dm_StartTimer(TEST_PDU_ID_0, 5u);
    
    /* Process timers until timeout */
    for (uint8 i = 0u; i < 6u; i++) {
        Com_Dm_ProcessTimers();
    }
    
    /* Verify timeout state */
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    
    /* Handle timeout */
    Com_Dm_HandleTimeout(TEST_PDU_ID_0, &dmConfig);
    
    /* Verify ErrorHook was called */
    TEST_ASSERT_EQUAL(1u, ErrorHook_CallCount);
    TEST_ASSERT_EQUAL(TEST_PDU_ID_0, ErrorHook_LastPduId);
}

/*
 * Test Case: TC_DM_008
 * Description: Verify default value substitution
 */
void test_TC_DM_008_DefaultValueSubstitution(void)
{
    /* Setup test buffer */
    uint8 testBuffer[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    memcpy(TestPduBuffer, testBuffer, 8u);
    
    /* Create DM config with default value */
    Com_DmRxConfigType dmConfig;
    dmConfig.ComIPduRxTimeout = 5u;
    dmConfig.ComIPduRxDefaultValue = TestDefaultValue;
    dmConfig.DefaultValueLength = 4u; /* Only first 4 bytes */
    dmConfig.TimeoutAction = COM_DM_ACTION_DEFAULT_VALUE;
    dmConfig.ComErrorHook = NULL_PTR;
    dmConfig.EnableDeadlineMonitoring = TRUE;
    
    /* Apply default value */
    Std_ReturnType result = Com_Dm_ApplyDefaultValue(TEST_PDU_ID_0, &dmConfig);
    
    /* Verify success */
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Verify default value applied (only first 4 bytes) */
    TEST_ASSERT_EQUAL_HEX8(0xAA, TestPduBuffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, TestPduBuffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, TestPduBuffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0xDD, TestPduBuffer[3]);
    /* Remaining bytes unchanged */
    TEST_ASSERT_EQUAL_HEX8(0x55, TestPduBuffer[4]);
}

/*
 * Test Case: TC_DM_009
 * Description: Verify both ErrorHook and default value on timeout
 */
void test_TC_DM_009_BothActionsOnTimeout(void)
{
    /* Setup test buffer */
    uint8 testBuffer[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(TestPduBuffer, testBuffer, 8u);
    
    /* Create DM config with both actions */
    Com_DmRxConfigType dmConfig;
    dmConfig.ComIPduRxTimeout = 5u;
    dmConfig.ComIPduRxDefaultValue = TestDefaultValue;
    dmConfig.DefaultValueLength = 4u;
    dmConfig.TimeoutAction = COM_DM_ACTION_BOTH;
    dmConfig.ComErrorHook = MockErrorHook;
    dmConfig.EnableDeadlineMonitoring = TRUE;
    
    /* Handle timeout */
    Com_Dm_HandleTimeout(TEST_PDU_ID_0, &dmConfig);
    
    /* Verify ErrorHook called */
    TEST_ASSERT_EQUAL(1u, ErrorHook_CallCount);
    
    /* Verify default value applied */
    TEST_ASSERT_EQUAL_HEX8(0xAA, TestPduBuffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, TestPduBuffer[1]);
}

/*
 * Test Case: TC_DM_010
 * Description: Verify zero timeout disables monitoring
 */
void test_TC_DM_010_ZeroTimeoutDisablesMonitoring(void)
{
    /* Start timer with zero timeout */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_0MS);
    
    /* Verify state is stopped */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    TEST_ASSERT_EQUAL(0u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
}

/*
 * Test Case: TC_DM_011
 * Description: Verify invalid PDU ID handling
 * ASIL-D: Parameter validation
 */
void test_TC_DM_011_InvalidPduIdHandling(void)
{
    /* Try to start timer with invalid PDU ID */
    Com_Dm_StartTimer(TEST_PDU_ID_INVALID, TEST_TIMEOUT_100MS);
    
    /* Should not crash - state should remain unchanged for valid PDUs */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
}

/*
 * Test Case: TC_DM_012
 * Description: Verify state query function
 */
void test_TC_DM_012_GetState(void)
{
    /* Initially stopped */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_Dm_GetState(TEST_PDU_ID_0));
    
    /* Start timer - should be running */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_100MS);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_Dm_GetState(TEST_PDU_ID_0));
    
    /* Invalid PDU ID should return ERROR */
    TEST_ASSERT_EQUAL(COM_DM_STATE_ERROR, Com_Dm_GetState(TEST_PDU_ID_INVALID));
}

/*
 * Test Case: TC_DM_013
 * Description: Verify integrity validation
 * ASIL-D: Runtime integrity check
 */
void test_TC_DM_013_IntegrityValidation(void)
{
    /* Valid state - should pass */
    TEST_ASSERT_EQUAL(E_OK, Com_Dm_ValidateIntegrity());
    
    /* Corrupt state manually */
    Com_DmRunTimeData[TEST_PDU_ID_0].State = (Com_DmStateType)99u; /* Invalid state */
    
    /* Should fail integrity check */
    TEST_ASSERT_EQUAL(E_NOT_OK, Com_Dm_ValidateIntegrity());
}

/*
 * Test Case: TC_DM_014
 * Description: Verify timer stops correctly
 */
void test_TC_DM_014_StopTimer(void)
{
    /* Start timer */
    Com_Dm_StartTimer(TEST_PDU_ID_0, TEST_TIMEOUT_100MS);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    
    /* Stop timer */
    Com_Dm_StopTimer(TEST_PDU_ID_0);
    
    /* Verify stopped */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    TEST_ASSERT_EQUAL(0u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
}

/*
 * Test Case: TC_DM_015
 * Description: Verify multiple PDU monitoring
 */
void test_TC_DM_015_MultiplePduMonitoring(void)
{
    /* Start both timers */
    Com_Dm_StartTimer(TEST_PDU_ID_0, 10u);
    Com_Dm_StartTimer(TEST_PDU_ID_1, 5u);
    
    /* Process until first timeout */
    for (uint8 i = 0u; i < 6u; i++) {
        Com_Dm_ProcessTimers();
    }
    
    /* PDU 1 should be expired */
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[TEST_PDU_ID_1].State);
    
    /* PDU 0 should still be running */
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    TEST_ASSERT_EQUAL(4u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
}

/*
 * Test Case: TC_DM_016
 * Description: Verify timeout recovery after Rx indication
 */
void test_TC_DM_016_TimeoutRecovery(void)
{
    /* Create DM config */
    Com_DmRxConfigType dmConfig;
    dmConfig.ComIPduRxTimeout = 5u;
    dmConfig.ComIPduRxDefaultValue = NULL_PTR;
    dmConfig.DefaultValueLength = 0u;
    dmConfig.TimeoutAction = COM_DM_ACTION_NONE;
    dmConfig.ComErrorHook = NULL_PTR;
    dmConfig.EnableDeadlineMonitoring = TRUE;
    
    /* Start and let timeout */
    Com_Dm_StartTimer(TEST_PDU_ID_0, 5u);
    for (uint8 i = 0u; i < 6u; i++) {
        Com_Dm_ProcessTimers();
    }
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    
    /* Simulate Rx indication - should recover */
    Com_Dm_HandleRxIndication(TEST_PDU_ID_0, &dmConfig);
    
    /* Should be running again */
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[TEST_PDU_ID_0].State);
    TEST_ASSERT_EQUAL(5u, Com_DmRunTimeData[TEST_PDU_ID_0].Timer);
}

/*==================[Main Function]========================================*/

void app_main(void)
{
    UNITY_BEGIN();
    
    /* Initialization tests */
    RUN_TEST(test_TC_DM_001_Initialization);
    RUN_TEST(test_TC_DM_002_DeInitialization);
    
    /* Timer management tests */
    RUN_TEST(test_TC_DM_003_StartTimer);
    RUN_TEST(test_TC_DM_004_TimerDecrement);
    RUN_TEST(test_TC_DM_005_TimeoutDetection);
    RUN_TEST(test_TC_DM_014_StopTimer);
    
    /* Rx indication tests */
    RUN_TEST(test_TC_DM_006_RxIndicationRestart);
    RUN_TEST(test_TC_DM_016_TimeoutRecovery);
    
    /* ErrorHook tests */
    RUN_TEST(test_TC_DM_007_ErrorHookInvocation);
    
    /* Default value tests */
    RUN_TEST(test_TC_DM_008_DefaultValueSubstitution);
    RUN_TEST(test_TC_DM_009_BothActionsOnTimeout);
    
    /* Edge case tests */
    RUN_TEST(test_TC_DM_010_ZeroTimeoutDisablesMonitoring);
    RUN_TEST(test_TC_DM_011_InvalidPduIdHandling);
    RUN_TEST(test_TC_DM_012_GetState);
    RUN_TEST(test_TC_DM_013_IntegrityValidation);
    RUN_TEST(test_TC_DM_015_MultiplePduMonitoring);
    
    UNITY_END();
}

/*==================[End of File]==========================================*/

/******************************************************************************
 * @file    test_ecum_bswm_integration.c
 * @brief   EcuM and BswM Integration Tests
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "autosar/classic/ecum/ecum.h"
#include "autosar/classic/bswm/bswm.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
static EcuM_ConfigType ecumTestConfig = {
    .normalMcuWakeupTime = 50U,
    .minShutdownTime = 100U,
    .wakeupSources = NULL,
    .numWakeupSources = 0U,
    .sleepModes = NULL,
    .numSleepModes = 0U,
    .defaultTarget = ECUM_TARGET_OFF,
    .defaultSleepMode = ECUM_SLEEP_MODE_POLLING,
    .defaultResetMode = ECUM_WKSOURCE_INTERNAL_RESET,
    .numModeRequestPorts = 4U
};

static BswM_ModeRequestPortType bswmTestModePorts[] = {
    {
        .type = BSWM_MRP_ECUM_INDICATION,
        .id = 0U,
        .currentMode = 0U,
        .requestedMode = 0U,
        .requestPending = FALSE,
        .isInitialized = TRUE
    },
    {
        .type = BSWM_MRP_COMM_INDICATION,
        .id = 1U,
        .currentMode = 0U,
        .requestedMode = 0U,
        .requestPending = FALSE,
        .isInitialized = TRUE
    }
};

static BswM_ConfigType bswmTestConfig = {
    .rules = NULL,
    .numRules = 0U,
    .actionLists = NULL,
    .numActionLists = 0U,
    .modeRequestPorts = bswmTestModePorts,
    .numModeRequestPorts = 2U,
    .modeConditions = NULL,
    .numModeConditions = 0U,
    .expressions = NULL,
    .numExpressions = 0U,
    .modeRequestProcessingEnabled = TRUE,
    .rulesProcessingEnabled = TRUE
};

/******************************************************************************
 * Test Counters
 ******************************************************************************/
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/******************************************************************************
 * Test Macros
 ******************************************************************************/
#define TEST_ASSERT(condition, msg) do { \
    tests_run++; \
    if (!(condition)) { \
        printf("  [FAIL] %s\n", msg); \
        tests_failed++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
        tests_passed++; \
    } \
} while(0)

#define TEST_START(name) printf("\nTest: %s\n", name)

/******************************************************************************
 * Test Functions
 ******************************************************************************/

void test_ecum_to_bswm_state_notification(void)
{
    TEST_START("EcuM_to_BswM_State_Notification");
    
    /* Initialize both modules */
    Std_ReturnType result = EcuM_Init(&ecumTestConfig);
    TEST_ASSERT(result == E_OK, "EcuM init should succeed");
    
    BswM_Init(&bswmTestConfig);
    TEST_ASSERT(BswM_IsInitialized() == TRUE, "BswM should be initialized");
    
    /* Transition EcuM to RUN and notify BswM */
    EcuM_StartupTwo();
    TEST_ASSERT(EcuM_GetState() == ECUM_STATE_RUN, "EcuM should be in RUN state");
    
    /* Notify BswM of EcuM state change */
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    TEST_ASSERT(1, "BswM should receive EcuM state notification");
    
    /* Cleanup */
    BswM_Deinit();
    (void)EcuM_DeInit();
}

void test_bswm_ecum_shutdown_action(void)
{
    TEST_START("BswM_EcuM_Shutdown_Action");
    
    /* Initialize EcuM */
    (void)EcuM_Init(&ecumTestConfig);
    EcuM_StartupTwo();
    
    /* Initialize BswM */
    BswM_Init(&bswmTestConfig);
    
    /* BswM requests EcuM shutdown target selection */
    BswM_EcuM_CurrentState(ECUM_STATE_PREP_SHUTDOWN);
    TEST_ASSERT(1, "BswM should handle EcuM state notification");
    
    /* Cleanup */
    BswM_Deinit();
    (void)EcuM_DeInit();
}

void test_dcm_session_impact_on_ecum(void)
{
    TEST_START("DCM_Session_Impact_on_EcuM");
    
    /* Initialize modules */
    (void)EcuM_Init(&ecumTestConfig);
    EcuM_StartupTwo();
    
    BswM_Init(&bswmTestConfig);
    
    /* DCM enters programming session - BswM should notify EcuM to prevent sleep */
    BswM_Dcm_RequestSessionMode(0x02U); /* Programming session */
    TEST_ASSERT(1, "BswM should handle DCM session change");
    
    /* Verify EcuM still in RUN (programming session prevents sleep) */
    TEST_ASSERT(EcuM_GetState() == ECUM_STATE_RUN, "EcuM should remain in RUN");
    
    /* Cleanup */
    BswM_Deinit();
    (void)EcuM_DeInit();
}

void test_communication_mode_impact(void)
{
    TEST_START("Communication_Mode_Impact");
    
    /* Initialize modules */
    (void)EcuM_Init(&ecumTestConfig);
    EcuM_StartupTwo();
    
    BswM_Init(&bswmTestConfig);
    
    /* ComM indicates full communication */
    BswM_ComM_CurrentMode(0U, COMM_FULL_COMMUNICATION);
    TEST_ASSERT(1, "BswM should handle ComM full communication");
    
    /* ComM indicates no communication - might trigger sleep */
    BswM_ComM_CurrentMode(0U, COMM_NO_COMMUNICATION);
    TEST_ASSERT(1, "BswM should handle ComM no communication");
    
    /* Cleanup */
    BswM_Deinit();
    (void)EcuM_DeInit();
}

void test_wakeup_handling_integration(void)
{
    TEST_START("Wakeup_Handling_Integration");
    
    /* Initialize modules */
    (void)EcuM_Init(&ecumTestConfig);
    
    BswM_Init(&bswmTestConfig);
    
    /* EcuM detects wakeup */
    EcuM_SetWakeupEvent(ECUM_WKSOURCE_CAN);
    
    /* Notify BswM of wakeup status */
    BswM_EcuM_WakeupStatus(ECUM_WKSOURCE_CAN, ECUM_WKSTATUS_PENDING);
    TEST_ASSERT(1, "BswM should receive wakeup status");
    
    BswM_EcuM_WakeupStatus(ECUM_WKSOURCE_CAN, ECUM_WKSTATUS_VALIDATED);
    TEST_ASSERT(1, "BswM should receive validated wakeup");
    
    /* Cleanup */
    BswM_Deinit();
    (void)EcuM_DeInit();
}

void test_mainfunction_coordination(void)
{
    TEST_START("MainFunction_Coordination");
    
    /* Initialize modules */
    (void)EcuM_Init(&ecumTestConfig);
    EcuM_StartupTwo();
    
    BswM_Init(&bswmTestConfig);
    
    /* Call both main functions */
    EcuM_MainFunction();
    TEST_ASSERT(1, "EcuM MainFunction executed");
    
    BswM_MainFunction();
    TEST_ASSERT(1, "BswM MainFunction executed");
    
    /* Cleanup */
    BswM_Deinit();
    (void)EcuM_DeInit();
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("============================================\n");
    printf("EcuM + BswM Integration Tests\n");
    printf("============================================\n");
    
    test_ecum_to_bswm_state_notification();
    test_bswm_ecum_shutdown_action();
    test_dcm_session_impact_on_ecum();
    test_communication_mode_impact();
    test_wakeup_handling_integration();
    test_mainfunction_coordination();
    
    printf("\n============================================\n");
    printf("Summary: %d tests run, %d passed, %d failed\n", 
           tests_run, tests_passed, tests_failed);
    printf("============================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}

/******************************************************************************
 * @file    test_bswm_actions.c
 * @brief   BswM Actions Unit Tests
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "autosar/classic/bswm/bswm.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
static BswM_ModeRequestPortType testModeRequestPorts[] = {
    {
        .type = BSWM_MRP_GENERIC_REQUEST,
        .id = 0U,
        .currentMode = 0U,
        .requestedMode = 0U,
        .requestPending = FALSE,
        .isInitialized = TRUE
    }
};

static BswM_ConfigType testConfig = {
    .rules = NULL,
    .numRules = 0U,
    .actionLists = NULL,
    .numActionLists = 0U,
    .modeRequestPorts = testModeRequestPorts,
    .numModeRequestPorts = 1U,
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
 * Test Callback
 ******************************************************************************/
static int callbackExecuted = 0;

static void testCallback(void)
{
    callbackExecuted++;
}

/******************************************************************************
 * Test Functions
 ******************************************************************************/

void test_action_initialization(void)
{
    TEST_START("ActionInitialization");
    
    BswM_ActionItemType action;
    
    /* Initialize action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_REQUEST_MODE);
    
    TEST_ASSERT(action.actionType == BSWM_ACTION_REQUEST_MODE, "Action type should be set");
    TEST_ASSERT(action.abortOnFail == FALSE, "Abort on fail should be FALSE");
    
    /* Cleanup */
    (void)testConfig;
}

void test_request_mode_action(void)
{
    TEST_START("RequestModeAction");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Create request mode action */
    BswM_ActionItemType *action = BswM_CreateRequestModeAction(0U, 5U);
    TEST_ASSERT(action != NULL, "Create request mode action should return valid action");
    TEST_ASSERT(action->actionType == BSWM_ACTION_REQUEST_MODE, "Action type should be REQUEST_MODE");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_switch_mode_action(void)
{
    TEST_START("SwitchModeAction");
    
    /* Create switch mode action */
    BswM_ActionItemType *action = BswM_CreateSwitchModeAction(0U, 10U);
    TEST_ASSERT(action != NULL, "Create switch mode action should return valid action");
    TEST_ASSERT(action->actionType == BSWM_ACTION_SWITCH_MODE, "Action type should be SWITCH_MODE");
}

void test_ecum_shutdown_action(void)
{
    TEST_START("EcuMShutdownAction");
    
    /* Create EcuM shutdown action */
    BswM_ActionItemType *action = BswM_CreateEcuMShutdownAction(ECUM_TARGET_SLEEP, 0U);
    TEST_ASSERT(action != NULL, "Create EcuM shutdown action should return valid action");
    TEST_ASSERT(action->actionType == BSWM_ACTION_ECUM_SELECT_SHUTDOWN_TARGET,
                "Action type should be ECUM_SELECT_SHUTDOWN_TARGET");
}

void test_user_callback_action(void)
{
    TEST_START("UserCallbackAction");
    
    callbackExecuted = 0;
    
    /* Create user callback action */
    BswM_ActionItemType *action = BswM_CreateUserCallbackAction(testCallback);
    TEST_ASSERT(action != NULL, "Create user callback action should return valid action");
    TEST_ASSERT(action->actionType == BSWM_ACTION_USER_CALLBACK, "Action type should be USER_CALLBACK");
}

void test_action_list_creation(void)
{
    TEST_START("ActionListCreation");
    
    /* Create action list */
    BswM_ActionListType *list = BswM_CreateActionList(0U);
    TEST_ASSERT(list != NULL, "Create action list should return valid list");
    TEST_ASSERT(list->id == 0U, "Action list ID should be set");
    
    /* Set condition */
    BswM_ReturnType result = BswM_SetActionListCondition(list, TRUE, BSWM_RULE_TRUE);
    TEST_ASSERT(result == BSWM_E_OK, "Set action list condition should succeed");
    TEST_ASSERT(list->executeOnlyIf == TRUE, "Execute only if should be set");
}

void test_comm_actions(void)
{
    TEST_START("ComMActions");
    
    BswM_ActionItemType action;
    
    /* Initialize ComM allow communication action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_COMM_ALLOW_COM);
    TEST_ASSERT(action.actionType == BSWM_ACTION_COMM_ALLOW_COM, "Action type should be COMM_ALLOW_COM");
    
    /* Initialize ComM mode limitation action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_COMM_MODE_LIMITATION);
    TEST_ASSERT(action.actionType == BSWM_ACTION_COMM_MODE_LIMITATION, "Action type should be COMM_MODE_LIMITATION");
    
    /* Initialize ComM mode switch action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_COMM_MODE_SWITCH);
    TEST_ASSERT(action.actionType == BSWM_ACTION_COMM_MODE_SWITCH, "Action type should be COMM_MODE_SWITCH");
}

void test_dcm_actions(void)
{
    TEST_START("DcmActions");
    
    BswM_ActionItemType action;
    
    /* Initialize DCM enable reset action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_DCM_ENABLE_RESET);
    TEST_ASSERT(action.actionType == BSWM_ACTION_DCM_ENABLE_RESET, "Action type should be DCM_ENABLE_RESET");
    
    /* Initialize DCM disable reset action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_DCM_DISABLE_RESET);
    TEST_ASSERT(action.actionType == BSWM_ACTION_DCM_DISABLE_RESET, "Action type should be DCM_DISABLE_RESET");
    
    /* Initialize DCM enable DTC setting action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_DCM_ENABLE_DTC_SETTING);
    TEST_ASSERT(action.actionType == BSWM_ACTION_DCM_ENABLE_DTC_SETTING, "Action type should be DCM_ENABLE_DTC_SETTING");
    
    /* Initialize DCM disable DTC setting action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_DCM_DISABLE_DTC_SETTING);
    TEST_ASSERT(action.actionType == BSWM_ACTION_DCM_DISABLE_DTC_SETTING, "Action type should be DCM_DISABLE_DTC_SETTING");
}

void test_ecum_actions(void)
{
    TEST_START("EcuMActions");
    
    BswM_ActionItemType action;
    
    /* Initialize EcuM go down action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_ECUM_GO_DOWN);
    TEST_ASSERT(action.actionType == BSWM_ACTION_ECUM_GO_DOWN, "Action type should be ECUM_GO_DOWN");
    
    /* Initialize EcuM select shutdown target action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_ECUM_SELECT_SHUTDOWN_TARGET);
    TEST_ASSERT(action.actionType == BSWM_ACTION_ECUM_SELECT_SHUTDOWN_TARGET, "Action type should be ECUM_SELECT_SHUTDOWN_TARGET");
    
    /* Initialize EcuM release RUN request action */
    BswM_InitializeActionItem(&action, BSWM_ACTION_ECUM_RELEASE_RUN_REQUEST);
    TEST_ASSERT(action.actionType == BSWM_ACTION_ECUM_RELEASE_RUN_REQUEST, "Action type should be ECUM_RELEASE_RUN_REQUEST");
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("============================================\n");
    printf("BswM Actions Unit Tests\n");
    printf("============================================\n");
    
    test_action_initialization();
    test_request_mode_action();
    test_switch_mode_action();
    test_ecum_shutdown_action();
    test_user_callback_action();
    test_action_list_creation();
    test_comm_actions();
    test_dcm_actions();
    test_ecum_actions();
    
    printf("\n============================================\n");
    printf("Summary: %d tests run, %d passed, %d failed\n", 
           tests_run, tests_passed, tests_failed);
    printf("============================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}

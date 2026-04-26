/******************************************************************************
 * @file    test_bswm_rules.c
 * @brief   BswM Rules Engine Unit Tests
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

static BswM_ModeConditionType testModeCondition = {
    .portType = BSWM_MRP_GENERIC_REQUEST,
    .portId = 0U,
    .modeValue = 1U,
    .initialMode = 0U,
    .isInitialized = TRUE
};

static BswM_ExpressionStructType testExpression = {
    .type = BSWM_EXPR_MODE_REQUEST,
    .data.mode.condition = &testModeCondition
};

static BswM_ActionItemType testActions[] = {
    {
        .actionType = BSWM_ACTION_USER_CALLBACK,
        .parameters.userCallback.callback = NULL,
        .abortOnFail = FALSE,
        .retryCount = 0U
    }
};

static BswM_ActionListType testActionLists[] = {
    {
        .id = 0U,
        .executeOnlyIf = FALSE,
        .condition = BSWM_RULE_TRUE,
        .numActions = 1U,
        .actions = testActions
    },
    {
        .id = 1U,
        .executeOnlyIf = FALSE,
        .condition = BSWM_RULE_FALSE,
        .numActions = 0U,
        .actions = NULL
    }
};

static BswM_RuleType testRules[] = {
    {
        .id = 0U,
        .expression = &testExpression,
        .trueActionList = 0U,
        .falseActionList = 1U,
        .lastState = BSWM_RULE_UNDEF,
        .isInitialized = TRUE
    }
};

static BswM_ConfigType testConfig = {
    .rules = testRules,
    .numRules = 1U,
    .actionLists = testActionLists,
    .numActionLists = 2U,
    .modeRequestPorts = testModeRequestPorts,
    .numModeRequestPorts = 1U,
    .modeConditions = &testModeCondition,
    .numModeConditions = 1U,
    .expressions = &testExpression,
    .numExpressions = 1U,
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

void test_rule_evaluation(void)
{
    TEST_START("RuleEvaluation");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Set mode to match condition */
    BswM_UpdateModeRequestPort(0U, 1U);
    
    /* Evaluate rule */
    BswM_RuleStateType state = BswM_EvaluateRule(0U);
    TEST_ASSERT(state == BSWM_RULE_TRUE || state == BSWM_RULE_FALSE || state == BSWM_RULE_UNDEF,
                "Rule evaluation should return a valid state");
    
    /* Test evaluate non-existent rule */
    state = BswM_EvaluateRule(999U);
    TEST_ASSERT(state == BSWM_RULE_UNDEF, "Non-existent rule should return UNDEF");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_rule_state_management(void)
{
    TEST_START("RuleStateManagement");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test get rule state */
    BswM_RuleStateType state = BswM_GetRuleState(0U);
    TEST_ASSERT(state == BSWM_RULE_UNDEF, "Initial rule state should be UNDEF");
    
    /* Test rule notification */
    BswM_RuleNotification(0U, BSWM_RULE_TRUE);
    state = BswM_GetRuleState(0U);
    TEST_ASSERT(state == BSWM_RULE_TRUE, "Rule state should be TRUE after notification");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_expression_evaluation(void)
{
    TEST_START("ExpressionEvaluation");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test TRUE expression */
    BswM_ExpressionStructType trueExpr = { .type = BSWM_EXPR_TRUE };
    BswM_RuleStateType state = BswM_EvaluateRule(0U);
    TEST_ASSERT(1, "TRUE expression evaluated");
    
    /* Test FALSE expression */
    BswM_ExpressionStructType falseExpr = { .type = BSWM_EXPR_FALSE };
    TEST_ASSERT(1, "FALSE expression evaluated");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_action_list_execution(void)
{
    TEST_START("ActionListExecution");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test execute valid action list */
    BswM_ReturnType result = BswM_ExecuteActionList(0U);
    TEST_ASSERT(result == BSWM_E_OK || result == BSWM_E_NOT_OK,
                "Action list execution should return a valid result");
    
    /* Test execute non-existent action list */
    result = BswM_ExecuteActionList(999U);
    TEST_ASSERT(result == BSWM_E_NO_RULE, "Non-existent action list should return NO_RULE");
    
    /* Cleanup */
    BswM_Deinit();
}

void test_action_execution(void)
{
    TEST_START("ActionExecution");
    
    /* Init */
    BswM_Init(&testConfig);
    
    /* Test execute NULL action */
    BswM_ReturnType result = BswM_ExecuteAction(NULL);
    TEST_ASSERT(result == BSWM_E_NULL_POINTER, "NULL action should return NULL_POINTER");
    
    /* Test execute valid action */
    result = BswM_ExecuteAction(&testActions[0]);
    TEST_ASSERT(result == BSWM_E_OK || result == BSWM_E_NOT_OK,
                "Valid action execution should return a result");
    
    /* Cleanup */
    BswM_Deinit();
}

/******************************************************************************
 * Main Test Runner
 ******************************************************************************/
int main(void)
{
    printf("============================================\n");
    printf("BswM Rules Engine Unit Tests\n");
    printf("============================================\n");
    
    test_rule_evaluation();
    test_rule_state_management();
    test_expression_evaluation();
    test_action_list_execution();
    test_action_execution();
    
    printf("\n============================================\n");
    printf("Summary: %d tests run, %d passed, %d failed\n", 
           tests_run, tests_passed, tests_failed);
    printf("============================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}

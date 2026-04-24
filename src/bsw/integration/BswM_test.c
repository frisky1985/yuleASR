/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : BswM Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "BswM.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static uint8 mock_com_ipdu_group_control_called = 0U;
static uint8 mock_pdur_enable_routing_called = 0U;
static uint8 mock_pdur_disable_routing_called = 0U;
static uint8 mock_det_called = 0U;

static uint8 mock_rule_condition_result = 0U;
static uint8 mock_rule_true_action_called = 0U;
static uint8 mock_rule_false_action_called = 0U;

/*==================================================================================================
*                                     MOCK FUNCTIONS - SERVICE
==================================================================================================*/
void Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean Initialize)
{
    (void)IpduGroupVector;
    (void)Initialize;
    mock_com_ipdu_group_control_called++;
}

void PduR_EnableRouting(uint8 id)
{
    (void)id;
    mock_pdur_enable_routing_called++;
}

void PduR_DisableRouting(uint8 id)
{
    (void)id;
    mock_pdur_disable_routing_called++;
}

/*==================================================================================================
*                                     MOCK FUNCTIONS - DET
==================================================================================================*/
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    mock_det_called++;
    return E_OK;
}

/*==================================================================================================
*                                     RULE STUBS
==================================================================================================*/
boolean Rule_StartupComplete(void)
{
    return (boolean)mock_rule_condition_result;
}

void Action_StartCom(void)
{
    mock_rule_true_action_called++;
}

void Action_StopCom(void)
{
    mock_rule_false_action_called++;
}

/*==================================================================================================
*                                  TEST CONFIGURATION
==================================================================================================*/
static BswM_RuleConfigType testRules[] = {
    {
        /* ConditionFnc  */ Rule_StartupComplete,
        /* TrueActionList */ Action_StartCom,
        /* FalseActionList */ Action_StopCom
    }
};

static BswM_ConfigType testConfig = {
    testRules,
    1U
};

/*==================================================================================================
*                                  TEST HELPER FUNCTIONS
==================================================================================================*/
static void reset_mocks(void)
{
    mock_com_ipdu_group_control_called = 0U;
    mock_pdur_enable_routing_called = 0U;
    mock_pdur_disable_routing_called = 0U;
    mock_det_called = 0U;
    mock_rule_condition_result = 0U;
    mock_rule_true_action_called = 0U;
    mock_rule_false_action_called = 0U;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

TEST_CASE_DECLARE(bswm_init_sets_state_to_init)
{
    reset_mocks();

    BswM_Init(&testConfig);

    ASSERT_EQ(BSWM_STATE_INIT, BswM_GetState());

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_init_with_null_config_reports_det)
{
    reset_mocks();

    BswM_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_called);
    ASSERT_EQ(BSWM_STATE_UNINIT, BswM_GetState());

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_request_mode_queues_request)
{
    reset_mocks();

    BswM_Init(&testConfig);
    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN);

    /* Mode is not immediately applied; applied in MainFunction */
    ASSERT_EQ(BSWM_MODE_STARTUP, BswM_GetCurrentMode(BSWM_USER_ECU_STATE));

    /* Process the request */
    BswM_MainFunction();

    /* After MainFunction, mode should be updated */
    ASSERT_EQ(BSWM_MODE_RUN, BswM_GetCurrentMode(BSWM_USER_ECU_STATE));

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_mainfunction_executes_mode_actions_run)
{
    reset_mocks();

    BswM_Init(&testConfig);
    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN);
    BswM_MainFunction();

    /* RUN mode actions: enable COM and PDU routing */
    ASSERT_EQ(1U, mock_com_ipdu_group_control_called);
    ASSERT_EQ(1U, mock_pdur_enable_routing_called);
    ASSERT_EQ(0U, mock_pdur_disable_routing_called);

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_mainfunction_executes_mode_actions_shutdown)
{
    reset_mocks();

    BswM_Init(&testConfig);
    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN);
    BswM_MainFunction();

    /* Reset mocks to isolate shutdown test */
    mock_com_ipdu_group_control_called = 0U;
    mock_pdur_enable_routing_called = 0U;

    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_SHUTDOWN);
    BswM_MainFunction();

    /* SHUTDOWN mode actions: disable COM and PDU routing */
    ASSERT_EQ(1U, mock_com_ipdu_group_control_called);
    ASSERT_EQ(0U, mock_pdur_enable_routing_called);
    ASSERT_EQ(1U, mock_pdur_disable_routing_called);

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_mainfunction_executes_rules)
{
    reset_mocks();

    BswM_Init(&testConfig);

    /* Set condition to TRUE */
    mock_rule_condition_result = 1U;
    BswM_MainFunction();

    ASSERT_EQ(1U, mock_rule_true_action_called);
    ASSERT_EQ(0U, mock_rule_false_action_called);

    /* Set condition to FALSE */
    mock_rule_condition_result = 0U;
    BswM_MainFunction();

    ASSERT_EQ(1U, mock_rule_true_action_called);
    ASSERT_EQ(1U, mock_rule_false_action_called);

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_request_mode_invalid_user_reports_det)
{
    reset_mocks();

    BswM_Init(&testConfig);
    BswM_RequestMode(BSWM_NUM_REQUESTING_USERS, BSWM_MODE_RUN);

    ASSERT_EQ(1U, mock_det_called);

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_request_mode_invalid_mode_reports_det)
{
    reset_mocks();

    BswM_Init(&testConfig);
    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_MAX);

    ASSERT_EQ(1U, mock_det_called);

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_deinit_sets_state_to_uninit)
{
    reset_mocks();

    BswM_Init(&testConfig);
    ASSERT_EQ(BSWM_STATE_INIT, BswM_GetState());

    BswM_Deinit();
    ASSERT_EQ(BSWM_STATE_UNINIT, BswM_GetState());

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_get_current_mode_invalid_user_reports_det)
{
    reset_mocks();

    BswM_Init(&testConfig);
    BswM_ModeType mode = BswM_GetCurrentMode(BSWM_NUM_REQUESTING_USERS);

    ASSERT_EQ(1U, mock_det_called);
    ASSERT_EQ(BSWM_MODE_STARTUP, mode);

    TEST_PASS();
}

TEST_CASE_DECLARE(bswm_mode_arbitration_selects_highest_priority)
{
    reset_mocks();

    BswM_Init(&testConfig);

    /* User 0 requests STARTUP, User 1 requests RUN */
    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_STARTUP);
    BswM_RequestMode(BSWM_USER_COMMUNICATION, BSWM_MODE_RUN);
    BswM_MainFunction();

    /* RUN has higher priority, so mode actions for RUN should execute */
    ASSERT_EQ(1U, mock_pdur_enable_routing_called);

    TEST_PASS();
}

/*==================================================================================================
*                                    TEST SUITE
==================================================================================================*/
void test_suite_bswm(void)
{
    g_test_stats.current_suite = "BswM";
    printf("\n=== Test Suite: BswM ===\n");

    RUN_TEST(bswm_init_sets_state_to_init);
    RUN_TEST(bswm_init_with_null_config_reports_det);
    RUN_TEST(bswm_request_mode_queues_request);
    RUN_TEST(bswm_mainfunction_executes_mode_actions_run);
    RUN_TEST(bswm_mainfunction_executes_mode_actions_shutdown);
    RUN_TEST(bswm_mainfunction_executes_rules);
    RUN_TEST(bswm_request_mode_invalid_user_reports_det);
    RUN_TEST(bswm_request_mode_invalid_mode_reports_det);
    RUN_TEST(bswm_deinit_sets_state_to_uninit);
    RUN_TEST(bswm_get_current_mode_invalid_user_reports_det);
    RUN_TEST(bswm_mode_arbitration_selects_highest_priority);
}

/*==================================================================================================
*                                    MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    test_suite_bswm();
TEST_MAIN_END()

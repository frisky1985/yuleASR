/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : EcuM Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "EcuM.h"
#include "mock_services.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: EcuM_Init should initialize and enter STARTUP state */
TEST_CASE(ecum_init_startup_state)
{
    EcuM_StateType state;
    
    EcuM_Init();
    
    EcuM_GetState(&state);
    ASSERT_EQ(ECUM_STATE_STARTUP, state);
    TEST_PASS();
}

/* Test: EcuM_StartupTwo should transition to RUN state */
TEST_CASE(ecum_startup_two_run_state)
{
    EcuM_StateType state;
    
    EcuM_Init();
    EcuM_StartupTwo();
    
    EcuM_GetState(&state);
    ASSERT_EQ(ECUM_STATE_RUN, state);
    TEST_PASS();
}

/* Test: EcuM_RequestRUN should keep RUN state */
TEST_CASE(ecum_request_run)
{
    EcuM_StateType state;
    
    EcuM_Init();
    EcuM_StartupTwo();
    
    EcuM_RequestRUN(0);
    EcuM_GetState(&state);
    ASSERT_EQ(ECUM_STATE_RUN, state);
    TEST_PASS();
}

/* Test: EcuM_ReleaseRUN should transition to SHUTDOWN */
TEST_CASE(ecum_release_run_shutdown)
{
    EcuM_StateType state;
    
    EcuM_Init();
    EcuM_StartupTwo();
    
    EcuM_ReleaseRUN(0);
    EcuM_GetState(&state);
    ASSERT_EQ(ECUM_STATE_SHUTDOWN, state);
    TEST_PASS();
}

/* Test: EcuM_SelectShutdownTarget should set target */
TEST_CASE(ecum_select_shutdown_target)
{
    EcuM_StateType target;
    uint8_t reason;
    
    EcuM_Init();
    EcuM_SelectShutdownTarget(ECUM_STATE_SLEEP, 1);
    EcuM_GetShutdownTarget(&target, &reason);
    
    ASSERT_EQ(ECUM_STATE_SLEEP, target);
    ASSERT_EQ(1, reason);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(ecum)
{
    mock_Det_Reset();
}

TEST_SUITE_TEARDOWN(ecum)
{
}

TEST_SUITE(ecum)
{
    RUN_TEST(ecum_init_startup_state);
    RUN_TEST(ecum_startup_two_run_state);
    RUN_TEST(ecum_request_run);
    RUN_TEST(ecum_release_run_shutdown);
    RUN_TEST(ecum_select_shutdown_target);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(ecum);
TEST_MAIN_END()

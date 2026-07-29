/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : EcuM-BswM Integration Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "EcuM.h"
#include "BswM.h"
#include "mock_services.h"
#include "mock_det.h"

static uint8_t g_initCounter = 0;

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: EcuM startup sequence with BswM mode request */
TEST_CASE(ecum_startup_initializes_bswm)
{
    EcuM_StateType state;
    
    /* Initialize EcuM */
    EcuM_Init();
    EcuM_StartupTwo();
    
    /* Verify EcuM is in RUN state */
    EcuM_GetState(&state);
    ASSERT_EQ(ECUM_STATE_RUN, state);
    
    /* Initialize BswM */
    BswM_Init(NULL_PTR);
    
    /* Request mode through BswM */
    BswM_RequestMode(0, 1);
    
    TEST_PASS();
}

/* Test: Mode switch during runtime */
TEST_CASE(mode_switch_during_runtime)
{
    EcuM_StateType state;
    
    /* Initialize */
    EcuM_Init();
    EcuM_StartupTwo();
    BswM_Init(NULL_PTR);
    
    /* Request RUN mode */
    EcuM_RequestRUN(0);
    EcuM_GetState(&state);
    ASSERT_EQ(ECUM_STATE_RUN, state);
    
    /* Change mode via BswM */
    BswM_RequestMode(0, 2);
    
    /* Release RUN */
    EcuM_ReleaseRUN(0);
    
    TEST_PASS();
}

/* Test: Shutdown sequence */
TEST_CASE(shutdown_sequence)
{
    /* Initialize */
    EcuM_Init();
    EcuM_StartupTwo();
    BswM_Init(NULL_PTR);
    
    /* Set shutdown target */
    EcuM_SelectShutdownTarget(ECUM_STATE_OFF, 0);
    
    /* Deinitialize BswM */
    BswM_Deinit();
    
    /* Shutdown EcuM */
    EcuM_Shutdown();
    
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(ecum_bswm_integration)
{
    g_initCounter = 0;
    mock_Det_Reset();
}

TEST_SUITE_TEARDOWN(ecum_bswm_integration)
{
}

TEST_SUITE(ecum_bswm_integration)
{
    RUN_TEST(ecum_startup_initializes_bswm);
    RUN_TEST(mode_switch_during_runtime);
    RUN_TEST(shutdown_sequence);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(ecum_bswm_integration);
TEST_MAIN_END()

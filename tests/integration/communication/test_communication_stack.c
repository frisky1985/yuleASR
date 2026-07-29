/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Communication Stack Integration Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "ComM.h"
#include "Nm.h"
#include "CanIf.h"
#include "EcuM.h"
#include "mock_services.h"
#include "mock_ecual.h"
#include "mock_det.h"

static ComM_ConfigType g_commConfig;
static Nm_ConfigType g_nmConfig;
static CanIf_ConfigType g_canIfConfig;

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Full network startup sequence */
TEST_CASE(network_startup_sequence)
{
    /* Step 1: Initialize ECU */
    EcuM_Init();
    
    /* Step 2: Initialize ComM */
    g_commConfig.dummy = 0;
    ComM_Init(&g_commConfig);
    
    /* Step 3: Initialize NM */
    g_nmConfig.dummy = 0;
    Nm_Init(&g_nmConfig);
    
    /* Step 4: Initialize CanIf */
    CanIf_Init(&g_canIfConfig);
    
    /* Step 5: Request full communication */
    Std_ReturnType result = ComM_RequestComMode(0, COMM_FULL_COMMUNICATION);
    ASSERT_EQ(E_OK, result);
    
    /* Step 6: Request network access */
    result = Nm_NetworkRequest(0);
    ASSERT_EQ(E_OK, result);
    
    /* Step 7: Verify network state */
    Nm_StateType nmState;
    Nm_GetState(0, &nmState);
    ASSERT_EQ(NM_STATE_REPEAT_MESSAGE, nmState);
    
    TEST_PASS();
}

/* Test: Network release and shutdown sequence */
TEST_CASE(network_shutdown_sequence)
{
    /* Initialize all modules */
    EcuM_Init();
    ComM_Init(&g_commConfig);
    Nm_Init(&g_nmConfig);
    CanIf_Init(&g_canIfConfig);
    
    /* Start network */
    ComM_RequestComMode(0, COMM_FULL_COMMUNICATION);
    Nm_NetworkRequest(0);
    
    /* Release network */
    Std_ReturnType result = Nm_NetworkRelease(0);
    ASSERT_EQ(E_OK, result);
    
    /* Check state transition */
    Nm_StateType nmState;
    Nm_GetState(0, &nmState);
    ASSERT_EQ(NM_STATE_READY_SLEEP, nmState);
    
    /* Request no communication */
    result = ComM_RequestComMode(0, COMM_NO_COMMUNICATION);
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/* Test: Diagnostic session over communication stack */
TEST_CASE(diagnostic_session_active)
{
    ComM_ModeType mode;
    
    /* Initialize */
    EcuM_Init();
    ComM_Init(&g_commConfig);
    Nm_Init(&g_nmConfig);
    
    /* Activate diagnostic session */
    Std_ReturnType result = ComM_DCM_ActiveDiagnostic(0);
    ASSERT_EQ(E_OK, result);
    
    /* Verify communication mode */
    result = ComM_GetCurrentComMode(0, &mode);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(COMM_FULL_COMMUNICATION, mode);
    
    /* Deactivate diagnostic */
    result = ComM_DCM_InactiveDiagnostic(0);
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(communication_stack)
{
    mock_Det_Reset();
    g_commConfig.dummy = 0;
    g_nmConfig.dummy = 0;
    g_canIfConfig.CanIfInitConfig = NULL_PTR;
}

TEST_SUITE_TEARDOWN(communication_stack)
{
}

TEST_SUITE(communication_stack)
{
    RUN_TEST(network_startup_sequence);
    RUN_TEST(network_shutdown_sequence);
    RUN_TEST(diagnostic_session_active);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(communication_stack);
TEST_MAIN_END()

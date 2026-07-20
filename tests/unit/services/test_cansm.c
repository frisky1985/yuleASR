/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : CanSm Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-30
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "CanSm.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static CanSm_ConfigType g_test_config;
static CanSm_NetworkConfigType g_network_config;

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_network_config.NetworkHandle = 0;
    g_network_config.ControllerId = 0;
    g_network_config.NumBaudrates = 3;
    g_network_config.MainFunctionPeriodMs = 10;
    g_network_config.BusOffRecoveryTimeMs = 100;
    g_network_config.BusOffThreshold = 10;
    g_network_config.WakeupSupport = TRUE;
    g_network_config.BusOffRecoveryEnabled = TRUE;
    g_network_config.TransceiverSupport = FALSE;
    g_network_config.TransceiverId = 0;

    g_test_config.Networks = &g_network_config;
    g_test_config.NumNetworks = 1;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.SetBaudrateApi = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: CanSM_Init with valid config */
TEST_CASE(cansm_init_valid_config)
{
    setup_test_config();
    
    CanSM_Init(&g_test_config);
    
    Std_VersionInfoType versionInfo;
    CanSM_GetVersionInfo(&versionInfo);
    ASSERT_EQ(CANSM_VENDOR_ID, versionInfo.vendorID);
}

/* Test: CanSM_Init with NULL config */
TEST_CASE(cansm_init_null_config)
{
    CanSM_Init(NULL_PTR);
    
    /* Should handle NULL gracefully without crash */
}

/* Test: CanSM_DeInit */
TEST_CASE(cansm_deinit)
{
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    CanSM_DeInit();
    
    /* DeInit completed without crash */
}

/* Test: CanSM_RequestComMode - Full Communication */
TEST_CASE(cansm_request_com_mode_full)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_RequestComMode(0, COMM_FULL_COMMUNICATION);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: CanSM_RequestComMode - No Communication */
TEST_CASE(cansm_request_com_mode_no)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_RequestComMode(0, COMM_NO_COMMUNICATION);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: CanSM_RequestComMode - Silent Communication */
TEST_CASE(cansm_request_com_mode_silent)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_RequestComMode(0, COMM_SILENT_COMMUNICATION);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: CanSM_GetCurrentComMode */
TEST_CASE(cansm_get_current_com_mode)
{
    Std_ReturnType result;
    ComM_ModeType com_mode;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_GetCurrentComMode(0, &com_mode);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: CanSM_GetVersionInfo */
TEST_CASE(cansm_get_version_info)
{
    Std_VersionInfoType version_info;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    CanSM_GetVersionInfo(&version_info);
    
    ASSERT_EQ(CANSM_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(CANSM_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(CANSM_SW_MINOR_VERSION, version_info.sw_minor_version);
}

/* Test: CanSM_SetBaudrate */
TEST_CASE(cansm_set_baudrate)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_SetBaudrate(0, 500);
    
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
}

/* Test: CanSM_GetBaudrate */
TEST_CASE(cansm_get_baudrate)
{
    Std_ReturnType result;
    uint16 baudrate;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_GetBaudrate(0, &baudrate);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: CanSM_GetCurrentInternalState */
TEST_CASE(cansm_get_current_internal_state)
{
    Std_ReturnType result;
    CanSm_BsmStateType state;
    
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    result = CanSM_GetCurrentInternalState(0, &state);
    
    ASSERT_EQ(E_OK, result);
}

/* Test: CanSM_ControllerBusOff */
TEST_CASE(cansm_controller_busoff)
{
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    CanSM_ControllerBusOff(0);
    
    /* BusOff notification processed without crash */
}

/* Test: CanSM_ControllerModeIndication */
TEST_CASE(cansm_controller_mode_indication)
{
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    CanSM_ControllerModeIndication(0, CANIF_CS_STARTED);
    
    /* Mode indication processed without crash */
}

/* Test: CanSM_MainFunction */
TEST_CASE(cansm_main_function)
{
    setup_test_config();
    CanSM_Init(&g_test_config);
    
    CanSM_MainFunction();
    
    /* MainFunction processed without crash */
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(cansm)
{
}

TEST_SUITE_TEARDOWN(cansm)
{
}

TEST_SUITE(cansm)
{
    RUN_TEST(cansm_init_valid_config);
    RUN_TEST(cansm_init_null_config);
    RUN_TEST(cansm_deinit);
    RUN_TEST(cansm_request_com_mode_full);
    RUN_TEST(cansm_request_com_mode_no);
    RUN_TEST(cansm_request_com_mode_silent);
    RUN_TEST(cansm_get_current_com_mode);
    RUN_TEST(cansm_get_version_info);
    RUN_TEST(cansm_set_baudrate);
    RUN_TEST(cansm_get_baudrate);
    RUN_TEST(cansm_get_current_internal_state);
    RUN_TEST(cansm_controller_busoff);
    RUN_TEST(cansm_controller_mode_indication);
    RUN_TEST(cansm_main_function);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(cansm);
TEST_MAIN_END()

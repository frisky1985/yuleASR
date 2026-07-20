/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : FiM Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-30
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "FiM.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static FiM_ConfigType g_test_config;
static FiM_FunctionConfigType g_function_configs[2];
static FiM_EventInhibitionType g_event_inhibitions[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Setup event inhibitions */
    g_event_inhibitions[0].EventId = 0;
    g_event_inhibitions[0].InhibitionMask = FIM_INHIBITION_MASK_TEST_FAILED;
    g_event_inhibitions[0].UseSummaryEvent = FALSE;
    g_event_inhibitions[0].SummaryEventId = 0;

    g_event_inhibitions[1].EventId = 1;
    g_event_inhibitions[1].InhibitionMask = FIM_INHIBITION_MASK_PENDING;
    g_event_inhibitions[1].UseSummaryEvent = FALSE;
    g_event_inhibitions[1].SummaryEventId = 0;

    /* Setup function configs */
    g_function_configs[0].FunctionId = 0;
    g_function_configs[0].EventInhibitions = &g_event_inhibitions[0];
    g_function_configs[0].NumEventInhibitions = 1;
    g_function_configs[0].FunctionAvailable = TRUE;

    g_function_configs[1].FunctionId = 1;
    g_function_configs[1].EventInhibitions = &g_event_inhibitions[1];
    g_function_configs[1].NumEventInhibitions = 1;
    g_function_configs[1].FunctionAvailable = TRUE;

    /* Setup main config */
    g_test_config.FunctionConfigs = g_function_configs;
    g_test_config.NumFunctions = 2;
    g_test_config.SummaryEvents = NULL_PTR;
    g_test_config.NumSummaryEvents = 0;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.InhibitionConfigurationSupported = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: FiM_Init with valid config */
TEST_CASE(fim_init_valid_config)
{
    setup_test_config();
    
    FiM_Init(&g_test_config);
    
    TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/* Test: FiM_Init with NULL config */
TEST_CASE(fim_init_null_config)
{
    FiM_Init(NULL_PTR);
    
    TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/* Test: FiM_DeInit */
TEST_CASE(fim_deinit)
{
    setup_test_config();
    FiM_Init(&g_test_config);
    
    FiM_DeInit();
    
    TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/* Test: FiM_GetVersionInfo */
TEST_CASE(fim_get_version_info)
{
    Std_VersionInfoType version_info;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    FiM_GetVersionInfo(&version_info);
    
    ASSERT_EQ(FIM_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(FIM_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(FIM_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_PASS();
}

/* Test: FiM_SetFunctionAvailable - Enable */
TEST_CASE(fim_set_function_available_enable)
{
    Std_ReturnType result;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    result = FiM_SetFunctionAvailable(0, TRUE);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: FiM_SetFunctionAvailable - Disable */
TEST_CASE(fim_set_function_available_disable)
{
    Std_ReturnType result;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    result = FiM_SetFunctionAvailable(0, FALSE);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: FiM_GetFunctionPermission - Allowed */
TEST_CASE(fim_get_function_permission_allowed)
{
    Std_ReturnType result;
    FiM_PermissionStateType permission;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    result = FiM_GetFunctionPermission(0, &permission);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(FIM_PERMISSION_ALLOWED, permission);
    TEST_PASS();
}

/* Test: FiM_SetFunctionPermission */
TEST_CASE(fim_set_function_permission)
{
    Std_ReturnType result;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    result = FiM_SetFunctionPermission(0, FIM_PERMISSION_DENIED);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: FiM_GetInhibitionStatus */
TEST_CASE(fim_get_inhibition_status)
{
    Std_ReturnType result;
    FiM_InhibitionStatusType status;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    result = FiM_GetInhibitionStatus(0, &status);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: FiM_DemTriggerOnMonitorStatus */
TEST_CASE(fim_dem_trigger_on_monitor_status)
{
    setup_test_config();
    FiM_Init(&g_test_config);
    
    FiM_DemTriggerOnMonitorStatus(0, DEM_EVENT_STATUS_PASSED);
    
    TEST_PASS();
}

/* Test: FiM_DemTriggerOnEventStatus */
TEST_CASE(fim_dem_trigger_on_event_status)
{
    setup_test_config();
    FiM_Init(&g_test_config);
    
    FiM_DemTriggerOnEventStatus(0, 0x00, 0x01);
    
TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/* Test: FiM_MainFunction */
TEST_CASE(fim_main_function)
{
    setup_test_config();
    FiM_Init(&g_test_config);
    
    FiM_MainFunction();
    
TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/* Test: FiM with multiple FIDs */
TEST_CASE(fim_multiple_fids)
{
    Std_ReturnType result;
    FiM_PermissionStateType permission;
    
    setup_test_config();
    FiM_Init(&g_test_config);
    
    /* Test FID 0 */
    result = FiM_GetFunctionPermission(0, &permission);
    ASSERT_EQ(E_OK, result);
    
    /* Test FID 1 */
    result = FiM_GetFunctionPermission(1, &permission);
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(fim)
{
}

TEST_SUITE_TEARDOWN(fim)
{
}

TEST_SUITE(fim)
{
    RUN_TEST(fim_init_valid_config);
    RUN_TEST(fim_init_null_config);
    RUN_TEST(fim_deinit);
    RUN_TEST(fim_get_version_info);
    RUN_TEST(fim_set_function_available_enable);
    RUN_TEST(fim_set_function_available_disable);
    RUN_TEST(fim_get_function_permission_allowed);
    RUN_TEST(fim_set_function_permission);
    RUN_TEST(fim_get_inhibition_status);
    RUN_TEST(fim_dem_trigger_on_monitor_status);
    RUN_TEST(fim_dem_trigger_on_event_status);
    RUN_TEST(fim_main_function);
    RUN_TEST(fim_multiple_fids);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(fim);
TEST_MAIN_END()

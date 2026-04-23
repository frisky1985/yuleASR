/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : ComM Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "ComM.h"
#include "mock_services.h"
#include "mock_det.h"

static ComM_ConfigType g_test_config;

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: ComM_Init with valid config */
TEST_CASE(comm_init_valid_config)
{
    g_test_config.dummy = 0;
    
    ComM_Init(&g_test_config);
    
    /* Should be initialized */
    TEST_PASS();
}

/* Test: ComM_Init with NULL config */
TEST_CASE(comm_init_null_config)
{
    mock_Det_ReportError_set_return(E_OK);
    
    ComM_Init(NULL_PTR);
    
    /* Should report DET error */
    TEST_PASS();
}

/* Test: ComM_DeInit */
TEST_CASE(comm_deinit)
{
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    ComM_DeInit();
    
    TEST_PASS();
}

/* Test: ComM_RequestComMode with valid user */
TEST_CASE(comm_request_com_mode_valid)
{
    Std_ReturnType result;
    
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    result = ComM_RequestComMode(0, COMM_FULL_COMMUNICATION);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: ComM_RequestComMode with invalid user */
TEST_CASE(comm_request_com_mode_invalid_user)
{
    Std_ReturnType result;
    
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    result = ComM_RequestComMode(COMM_MAX_USERS + 1, COMM_FULL_COMMUNICATION);
    
    ASSERT_EQ(E_NOT_OK, result);
    TEST_PASS();
}

/* Test: ComM_GetMaxComMode */
TEST_CASE(comm_get_max_com_mode)
{
    Std_ReturnType result;
    ComM_ModeType mode;
    
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    /* Request full communication for user 0 */
    ComM_RequestComMode(0, COMM_FULL_COMMUNICATION);
    
    result = ComM_GetMaxComMode(0, &mode);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(COMM_FULL_COMMUNICATION, mode);
    TEST_PASS();
}

/* Test: ComM_GetCurrentComMode */
TEST_CASE(comm_get_current_com_mode)
{
    Std_ReturnType result;
    ComM_ModeType mode;
    
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    result = ComM_GetCurrentComMode(0, &mode);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: ComM_DCM_ActiveDiagnostic */
TEST_CASE(comm_dcm_active_diagnostic)
{
    Std_ReturnType result;
    
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    result = ComM_DCM_ActiveDiagnostic(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: ComM_DCM_InactiveDiagnostic */
TEST_CASE(comm_dcm_inactive_diagnostic)
{
    Std_ReturnType result;
    
    g_test_config.dummy = 0;
    ComM_Init(&g_test_config);
    
    ComM_DCM_ActiveDiagnostic(0);
    result = ComM_DCM_InactiveDiagnostic(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: ComM_GetVersionInfo */
TEST_CASE(comm_get_version_info)
{
    Std_VersionInfoType version;
    
    ComM_GetVersionInfo(&version);
    
    ASSERT_EQ(0x0001, version.vendorID);
    ASSERT_EQ(0x12, version.moduleID);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(comm)
{
    mock_Det_Reset();
}

TEST_SUITE_TEARDOWN(comm)
{
}

TEST_SUITE(comm)
{
    RUN_TEST(comm_init_valid_config);
    RUN_TEST(comm_init_null_config);
    RUN_TEST(comm_deinit);
    RUN_TEST(comm_request_com_mode_valid);
    RUN_TEST(comm_request_com_mode_invalid_user);
    RUN_TEST(comm_get_max_com_mode);
    RUN_TEST(comm_get_current_com_mode);
    RUN_TEST(comm_dcm_active_diagnostic);
    RUN_TEST(comm_dcm_inactive_diagnostic);
    RUN_TEST(comm_get_version_info);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(comm);
TEST_MAIN_END()

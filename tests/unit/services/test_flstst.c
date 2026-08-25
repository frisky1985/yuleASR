/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Flash Test Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

// @tests src/bsw/services/flstst/src/FlStSt.c  @tests src/bsw/services/flstst/include/FlStSt.h

#include "../test_framework.h"
#include "FlStSt.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static FlStSt_SectorType g_sectors[2];
static FlStSt_ConfigType g_test_config;

static void setup_default_config(void)
{
    g_sectors[0].StartAddr = 0x08000000;
    g_sectors[0].Size      = 65536;
    g_sectors[0].SectorId  = 0;
    g_sectors[0].PageSize  = 256;

    g_sectors[1].StartAddr = 0x08010000;
    g_sectors[1].Size      = 65536;
    g_sectors[1].SectorId  = 1;
    g_sectors[1].PageSize  = 256;

    g_test_config.NumSectors     = 2;
    g_test_config.Sectors        = g_sectors;
    g_test_config.Algorithm      = FLSTST_ALGO_MARCH_C;
    g_test_config.RunOnInit      = FALSE;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(flstst_init_valid)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
}

TEST_CASE(flstst_init_null)
{
    FlStSt_Init(NULL_PTR);
}

TEST_CASE(flstst_init_twice)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_Init(&g_test_config);
}

TEST_CASE(flstst_deinit)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_DeInit();
}

TEST_CASE(flstst_deinit_uninit)
{
    FlStSt_DeInit();
}

TEST_CASE(flstst_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_GetVersionInfo(&ver);
    ASSERT_EQ(FLSTST_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(FLSTST_MODULE_ID, ver.moduleID);
}

TEST_CASE(flstst_run_test)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_OK, FlStSt_RunTest(0));
}

TEST_CASE(flstst_run_test_invalid_sector)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, FlStSt_RunTest(99));
}

TEST_CASE(flstst_run_test_uninit)
{
    ASSERT_EQ(E_NOT_OK, FlStSt_RunTest(0));
}

TEST_CASE(flstst_verify_erase)
{
    boolean result;
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_OK, FlStSt_VerifyErase(0, &result));
}

TEST_CASE(flstst_verify_erase_null_result)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, FlStSt_VerifyErase(0, NULL_PTR));
}

TEST_CASE(flstst_verify_program)
{
    boolean result;
    uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_OK, FlStSt_VerifyProgram(0, data, sizeof(data), &result));
}

TEST_CASE(flstst_verify_program_null_data)
{
    boolean result;
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, FlStSt_VerifyProgram(0, NULL_PTR, 0, &result));
}

TEST_CASE(flstst_get_result)
{
    FlStSt_ResultType result;
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_OK, FlStSt_GetResult(&result));
    ASSERT_EQ(FLSTST_RESULT_NOT_RUN, result);
}

TEST_CASE(flstst_get_result_null)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, FlStSt_GetResult(NULL_PTR));
}

TEST_CASE(flstst_abort)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_RunTest(0);
    ASSERT_EQ(E_OK, FlStSt_Abort());
}

TEST_CASE(flstst_abort_not_busy)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    ASSERT_EQ(E_OK, FlStSt_Abort());
}

TEST_CASE(flstst_main_function_idle)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_MainFunction();
}

TEST_CASE(flstst_main_function_active)
{
    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_RunTest(0);
    FlStSt_MainFunction();
}

TEST_CASE(flstst_main_function_uninit)
{
    FlStSt_MainFunction();
}

TEST_CASE(flstst_run_then_get_result)
{
    FlStSt_ResultType result;
    uint8 i;

    setup_default_config();
    FlStSt_Init(&g_test_config);
    FlStSt_RunTest(0);

    /* Run MainFunction until complete */
    for (i = 0U; i < 200U; i++)
    {
        FlStSt_MainFunction();
    }

    FlStSt_GetResult(&result);
    ASSERT_EQ(FLSTST_RESULT_NOT_RUN, result);  /* After reset */
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(flstst) { }

TEST_SUITE_TEARDOWN(flstst) { }

TEST_SUITE(flstst)
{
    RUN_TEST(flstst_init_valid);
    RUN_TEST(flstst_init_null);
    RUN_TEST(flstst_init_twice);
    RUN_TEST(flstst_deinit);
    RUN_TEST(flstst_deinit_uninit);
    RUN_TEST(flstst_get_version_info);
    RUN_TEST(flstst_run_test);
    RUN_TEST(flstst_run_test_invalid_sector);
    RUN_TEST(flstst_run_test_uninit);
    RUN_TEST(flstst_verify_erase);
    RUN_TEST(flstst_verify_erase_null_result);
    RUN_TEST(flstst_verify_program);
    RUN_TEST(flstst_verify_program_null_data);
    RUN_TEST(flstst_get_result);
    RUN_TEST(flstst_get_result_null);
    RUN_TEST(flstst_abort);
    RUN_TEST(flstst_abort_not_busy);
    RUN_TEST(flstst_main_function_idle);
    RUN_TEST(flstst_main_function_active);
    RUN_TEST(flstst_main_function_uninit);
    RUN_TEST(flstst_run_then_get_result);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(flstst);
TEST_MAIN_END()

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : RAM Test Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "RamTst.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static uint8 g_ram_buffer[256];  /* Simulated RAM region */
static RamTst_RegionType g_regions[2];
static RamTst_ConfigType g_test_config;

static void setup_default_config(void)
{
    g_regions[0].StartAddr = (uint32)(uintptr)g_ram_buffer;
    g_regions[0].Size      = 256;
    g_regions[0].RegionId  = 0;

    g_regions[1].StartAddr = 0;
    g_regions[1].Size      = 0;
    g_regions[1].RegionId  = 1;

    g_test_config.NumRegions     = 1;
    g_test_config.Regions        = g_regions;
    g_test_config.Algorithm      = RAMTST_ALGO_MARCH_C;
    g_test_config.RunOnStartup   = FALSE;
    g_test_config.CompletionCb   = NULL_PTR;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(ramtst_init_valid)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
}

TEST_CASE(ramtst_init_null)
{
    RamTst_Init(NULL_PTR);
}

TEST_CASE(ramtst_init_twice)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_Init(&g_test_config);
}

TEST_CASE(ramtst_deinit)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_DeInit();
}

TEST_CASE(ramtst_deinit_uninit)
{
    RamTst_DeInit();
}

TEST_CASE(ramtst_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_GetVersionInfo(&ver);
    ASSERT_EQ(RAMTST_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(RAMTST_MODULE_ID, ver.moduleID);
}

TEST_CASE(ramtst_run_test)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    ASSERT_EQ(E_OK, RamTst_RunTest(0));
}

TEST_CASE(ramtst_run_test_invalid_region)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, RamTst_RunTest(99));
}

TEST_CASE(ramtst_run_test_uninit)
{
    ASSERT_EQ(E_NOT_OK, RamTst_RunTest(0));
}

TEST_CASE(ramtst_get_result)
{
    RamTst_ResultType result;
    setup_default_config();
    RamTst_Init(&g_test_config);
    ASSERT_EQ(E_OK, RamTst_GetResult(&result));
    ASSERT_EQ(RAMTST_RESULT_NOT_RUN, result);
}

TEST_CASE(ramtst_get_result_null)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, RamTst_GetResult(NULL_PTR));
}

TEST_CASE(ramtst_abort)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_RunTest(0);
    ASSERT_EQ(E_OK, RamTst_Abort());
}

TEST_CASE(ramtst_abort_not_busy)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    ASSERT_EQ(E_OK, RamTst_Abort());
}

TEST_CASE(ramtst_main_function_idle)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_MainFunction();
}

TEST_CASE(ramtst_main_function_busy)
{
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_RunTest(0);
    RamTst_MainFunction();
}

TEST_CASE(ramtst_main_function_uninit)
{
    RamTst_MainFunction();
}

TEST_CASE(ramtst_run_then_complete)
{
    uint8 i;
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_RunTest(0);

    /* Step through MainFunction until complete */
    for (i = 0U; i < 100U; i++)
    {
        RamTst_MainFunction();
    }
}

TEST_CASE(ramtst_abort_during_test)
{
    uint8 i;
    setup_default_config();
    RamTst_Init(&g_test_config);
    RamTst_RunTest(0);

    /* Let some steps run, then abort */
    for (i = 0U; i < 5U; i++)
    {
        RamTst_MainFunction();
    }
    ASSERT_EQ(E_OK, RamTst_Abort());
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(ramtst) { }

TEST_SUITE_TEARDOWN(ramtst) { }

TEST_SUITE(ramtst)
{
    RUN_TEST(ramtst_init_valid);
    RUN_TEST(ramtst_init_null);
    RUN_TEST(ramtst_init_twice);
    RUN_TEST(ramtst_deinit);
    RUN_TEST(ramtst_deinit_uninit);
    RUN_TEST(ramtst_get_version_info);
    RUN_TEST(ramtst_run_test);
    RUN_TEST(ramtst_run_test_invalid_region);
    RUN_TEST(ramtst_run_test_uninit);
    RUN_TEST(ramtst_get_result);
    RUN_TEST(ramtst_get_result_null);
    RUN_TEST(ramtst_abort);
    RUN_TEST(ramtst_abort_not_busy);
    RUN_TEST(ramtst_main_function_idle);
    RUN_TEST(ramtst_main_function_busy);
    RUN_TEST(ramtst_main_function_uninit);
    RUN_TEST(ramtst_run_then_complete);
    RUN_TEST(ramtst_abort_during_test);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(ramtst);
TEST_MAIN_END()

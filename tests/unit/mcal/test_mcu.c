/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Mcu Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Mcu.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Mcu_ConfigType g_test_config;

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_config.ClockSetting = 0;
    g_test_config.ClockFrequency = 16000000;
    g_test_config.PllMultiplier = 16;
    g_test_config.PllDivider = 1;
    g_test_config.PllEnabled = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Mcu_Init with valid config */
TEST_CASE(mcu_init_valid_config)
{
    Std_ReturnType result;
    
    setup_test_config();
    
    result = Mcu_Init(&g_test_config);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(MCU_CLOCK_UNINIT, Mcu_MockState);
    TEST_PASS();
}

/* Test: Mcu_Init with NULL config */
TEST_CASE(mcu_init_null_config)
{
    Std_ReturnType result;
    
    Det_Mock_Reset();
    
    result = Mcu_Init(NULL);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Mcu_Init when already initialized */
TEST_CASE(mcu_init_already_initialized)
{
    setup_test_config();
    Mcu_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Second init should report error */
    Mcu_Init(&g_test_config);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Mcu_InitClock */
TEST_CASE(mcu_init_clock)
{
    Std_ReturnType result;
    
    setup_test_config();
    Mcu_Init(&g_test_config);
    
    result = Mcu_InitClock(0);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(MCU_CLOCK_INITIALIZED, Mcu_MockState);
    TEST_PASS();
}

/* Test: Mcu_DistributePllClock */
TEST_CASE(mcu_distribute_pll_clock)
{
    Std_ReturnType result;
    
    setup_test_config();
    Mcu_Init(&g_test_config);
    Mcu_InitClock(0);
    
    result = Mcu_DistributePllClock();
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Mcu_GetPllStatus */
TEST_CASE(mcu_get_pll_status)
{
    Mcu_PllStatusType status;
    
    setup_test_config();
    Mcu_Init(&g_test_config);
    Mcu_InitClock(0);
    
    status = Mcu_GetPllStatus();
    
    ASSERT_EQ(MCU_PLL_STATUS_LOCKED, status);
    TEST_PASS();
}

/* Test: Mcu_SetMode */
TEST_CASE(mcu_set_mode)
{
    setup_test_config();
    Mcu_Init(&g_test_config);
    Mcu_InitClock(0);
    
    Mcu_SetMode(MCU_MODE_NORMAL);
    
    ASSERT_EQ(MCU_MODE_NORMAL, Mcu_MockState);
    TEST_PASS();
}

/* Test: Mcu_GetResetReason */
TEST_CASE(mcu_get_reset_reason)
{
    Mcu_ResetType reason;
    
    setup_test_config();
    Mcu_Init(&g_test_config);
    
    reason = Mcu_GetResetReason();
    
    ASSERT_EQ(MCU_RESET_POWER_ON, reason);
    TEST_PASS();
}

/* Test: Mcu_GetResetRawValue */
TEST_CASE(mcu_get_reset_raw_value)
{
    Mcu_RawResetType raw;
    
    setup_test_config();
    Mcu_Init(&g_test_config);
    
    raw = Mcu_GetResetRawValue();
    
    ASSERT_EQ(0x01, raw);
    TEST_PASS();
}

/* Test: Mcu_GetVersionInfo */
TEST_CASE(mcu_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Mcu_GetVersionInfo(&version_info);
    
    ASSERT_EQ(MCU_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(MCU_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

/* Test: Mcu_GetVersionInfo with NULL pointer */
TEST_CASE(mcu_get_version_info_null)
{
    Det_Mock_Reset();
    
    Mcu_GetVersionInfo(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Mcu_DistributePllClock when not initialized */
TEST_CASE(mcu_distribute_pll_not_init)
{
    Std_ReturnType result;
    
    Det_Mock_Reset();
    Mcu_MockState = MCU_UNINIT;
    
    result = Mcu_DistributePllClock();
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(mcu)
{
    Mcu_Mock_Reset();
    Det_Mock_Reset();
    setup_test_config();
}

TEST_SUITE_TEARDOWN(mcu)
{
    /* Cleanup after all tests */
}

TEST_SUITE(mcu)
{
    RUN_TEST(mcu_init_valid_config);
    RUN_TEST(mcu_init_null_config);
    RUN_TEST(mcu_init_already_initialized);
    RUN_TEST(mcu_init_clock);
    RUN_TEST(mcu_distribute_pll_clock);
    RUN_TEST(mcu_get_pll_status);
    RUN_TEST(mcu_set_mode);
    RUN_TEST(mcu_get_reset_reason);
    RUN_TEST(mcu_get_reset_raw_value);
    RUN_TEST(mcu_get_version_info);
    RUN_TEST(mcu_get_version_info_null);
    RUN_TEST(mcu_distribute_pll_not_init);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- Mcu Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(mcu);
}
TEST_MAIN_END()

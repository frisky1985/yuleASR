/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : WDG Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Wdg.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Wdg_ConfigType g_test_config;

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_config.BaseAddress = 0x40003000;
    g_test_config.FastModeSettings.TimeoutPeriod = 100;
    g_test_config.FastModeSettings.ClockPrescaler = WDG_PRESCALER_8;
    g_test_config.FastModeSettings.WindowModeEnabled = FALSE;
    g_test_config.FastModeSettings.WindowStart = 0;
    g_test_config.FastModeSettings.WindowEnd = 0;
    g_test_config.FastModeSettings.InterruptMode = FALSE;
    
    g_test_config.SlowModeSettings.TimeoutPeriod = 500;
    g_test_config.SlowModeSettings.ClockPrescaler = WDG_PRESCALER_64;
    g_test_config.SlowModeSettings.WindowModeEnabled = TRUE;
    g_test_config.SlowModeSettings.WindowStart = 50;
    g_test_config.SlowModeSettings.WindowEnd = 450;
    g_test_config.SlowModeSettings.InterruptMode = FALSE;
    
    g_test_config.InitialMode = WDGIF_FAST_MODE;
    g_test_config.DefaultTimeout = 100;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.DisableAllowed = FALSE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(wdg_init_valid)
{
    setup_test_config();
    Wdg_Init(&g_test_config);
    
    ASSERT_TRUE(Wdg_MockState.Initialized);
    ASSERT_EQ(WDGIF_FAST_MODE, Wdg_MockState.Mode);
    ASSERT_EQ(100, Wdg_MockState.Timeout);
    TEST_PASS();
}

TEST_CASE(wdg_init_null)
{
    Det_Mock_Reset();
    
    Wdg_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(WDG_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(wdg_init_already_initialized)
{
    setup_test_config();
    Wdg_Init(&g_test_config);
    Det_Mock_Reset();
    
    Wdg_Init(&g_test_config);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(WDG_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(wdg_set_mode_fast)
{
    Std_ReturnType result;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    
    result = Wdg_SetMode(WDGIF_FAST_MODE);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(WDGIF_FAST_MODE, Wdg_MockState.Mode);
    ASSERT_EQ(100, Wdg_MockState.Timeout);
    TEST_PASS();
}

TEST_CASE(wdg_set_mode_slow)
{
    Std_ReturnType result;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    
    result = Wdg_SetMode(WDGIF_SLOW_MODE);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(WDGIF_SLOW_MODE, Wdg_MockState.Mode);
    ASSERT_EQ(500, Wdg_MockState.Timeout);
    TEST_PASS();
}

TEST_CASE(wdg_set_mode_off_not_allowed)
{
    Std_ReturnType result;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Disable not allowed in config */
    result = Wdg_SetMode(WDGIF_OFF_MODE);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(WDG_E_DISABLE_NOT_ALLOWED, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(wdg_set_mode_off_allowed)
{
    Std_ReturnType result;
    
    setup_test_config();
    g_test_config.DisableAllowed = TRUE;
    Wdg_Init(&g_test_config);
    
    result = Wdg_SetMode(WDGIF_OFF_MODE);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(WDGIF_OFF_MODE, Wdg_MockState.Mode);
    TEST_PASS();
}

TEST_CASE(wdg_trigger)
{
    uint32 trigger_count_before;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    
    trigger_count_before = Wdg_MockState.TriggerCount;
    
    Wdg_Trigger();
    
    ASSERT_EQ(trigger_count_before + 1, Wdg_MockState.TriggerCount);
    TEST_PASS();
}

TEST_CASE(wdg_trigger_not_initialized)
{
    Det_Mock_Reset();
    
    Wdg_Trigger();
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(WDG_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(wdg_set_trigger_condition)
{
    Std_ReturnType result;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    
    result = Wdg_SetTriggerCondition(200);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(200, Wdg_MockState.Timeout);
    TEST_PASS();
}

TEST_CASE(wdg_set_trigger_condition_invalid)
{
    Std_ReturnType result;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Timeout exceeds configured maximum */
    result = Wdg_SetTriggerCondition(1000);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(WDG_E_PARAM_TIMEOUT, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(wdg_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Wdg_GetVersionInfo(&version_info);
    
    ASSERT_EQ(WDG_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(WDG_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

TEST_CASE(wdg_get_version_info_null)
{
    Det_Mock_Reset();
    
    Wdg_GetVersionInfo(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(WDG_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(wdg_multiple_trigger)
{
    int i;
    
    setup_test_config();
    Wdg_Init(&g_test_config);
    
    for (i = 0; i < 10; i++)
    {
        Wdg_Trigger();
    }
    
    ASSERT_EQ(10, Wdg_MockState.TriggerCount);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(wdg)
{
    Wdg_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(wdg)
{
}

TEST_SUITE(wdg)
{
    RUN_TEST(wdg_init_valid);
    RUN_TEST(wdg_init_null);
    RUN_TEST(wdg_init_already_initialized);
    RUN_TEST(wdg_set_mode_fast);
    RUN_TEST(wdg_set_mode_slow);
    RUN_TEST(wdg_set_mode_off_not_allowed);
    RUN_TEST(wdg_set_mode_off_allowed);
    RUN_TEST(wdg_trigger);
    RUN_TEST(wdg_trigger_not_initialized);
    RUN_TEST(wdg_set_trigger_condition);
    RUN_TEST(wdg_set_trigger_condition_invalid);
    RUN_TEST(wdg_get_version_info);
    RUN_TEST(wdg_get_version_info_null);
    RUN_TEST(wdg_multiple_trigger);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- WDG Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(wdg);
}
TEST_MAIN_END()

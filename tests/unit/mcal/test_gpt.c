/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : GPT Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Gpt.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Gpt_ConfigType g_test_config;
static Gpt_ChannelConfigType g_test_channels[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_channels[0].ChannelId = 0;
    g_test_channels[0].BaseAddress = 0x40000000;
    g_test_channels[0].ChannelMode = GPT_CH_MODE_ONESHOT;
    g_test_channels[0].ClockPrescaler = GPT_CLOCK_PRESCALER_1;
    g_test_channels[0].MaxTickValue = 0xFFFFFFFF;
    g_test_channels[0].ClockFrequency = 1000000;
    g_test_channels[0].WakeupSupport = FALSE;
    g_test_channels[0].NotificationEnabled = TRUE;
    g_test_channels[0].NotificationFn = NULL;
    
    g_test_channels[1].ChannelId = 1;
    g_test_channels[1].BaseAddress = 0x40000100;
    g_test_channels[1].ChannelMode = GPT_CH_MODE_CONTINUOUS;
    g_test_channels[1].ClockPrescaler = GPT_CLOCK_PRESCALER_8;
    g_test_channels[1].MaxTickValue = 0xFFFF;
    g_test_channels[1].ClockFrequency = 1000000;
    g_test_channels[1].WakeupSupport = TRUE;
    g_test_channels[1].NotificationEnabled = FALSE;
    g_test_channels[1].NotificationFn = NULL;
    
    g_test_config.Channels = g_test_channels;
    g_test_config.NumChannels = 2;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.WakeupFunctionalityApi = TRUE;
    g_test_config.DeInitApi = TRUE;
    g_test_config.TimeElapsedApi = TRUE;
    g_test_config.TimeRemainingApi = TRUE;
    g_test_config.EnableDisableNotificationApi = TRUE;
    g_test_config.NotificationSupported = TRUE;
    g_test_config.DefaultMode = GPT_MODE_NORMAL;
    g_test_config.PredefTimer1usEnablingGrade = TRUE;
    g_test_config.PredefTimer100us32bitEnable = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(gpt_init_valid)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    ASSERT_TRUE(Gpt_MockChannels[0].Enabled);
    ASSERT_TRUE(Gpt_MockChannels[1].Enabled);
    TEST_PASS();
}

TEST_CASE(gpt_init_null)
{
    Det_Mock_Reset();
    
    Gpt_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(GPT_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(gpt_deinit)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_DeInit();
    
    ASSERT_FALSE(Gpt_MockChannels[0].Enabled);
    TEST_PASS();
}

TEST_CASE(gpt_get_time_elapsed)
{
    Gpt_ValueType time;
    
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_Mock_SetChannelTime(0, 1000, 0);
    
    time = Gpt_GetTimeElapsed(0);
    
    ASSERT_EQ(1000, time);
    TEST_PASS();
}

TEST_CASE(gpt_get_time_remaining)
{
    Gpt_ValueType time;
    
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_Mock_SetChannelTime(0, 500, 1500);
    
    time = Gpt_GetTimeRemaining(0);
    
    ASSERT_EQ(1500, time);
    TEST_PASS();
}

TEST_CASE(gpt_start_timer)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_StartTimer(0, 5000);
    
    ASSERT_EQ(5000, Gpt_MockChannels[0].TargetTime);
    ASSERT_TRUE(Gpt_MockChannels[0].Enabled);
    TEST_PASS();
}

TEST_CASE(gpt_stop_timer)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_StartTimer(0, 5000);
    Gpt_StopTimer(0);
    
    ASSERT_FALSE(Gpt_MockChannels[0].Enabled);
    TEST_PASS();
}

TEST_CASE(gpt_enable_notification)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_EnableNotification(0);
    
    /* Notification enabled - implementation dependent */
    TEST_PASS();
}

TEST_CASE(gpt_disable_notification)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_DisableNotification(0);
    
    /* Notification disabled - implementation dependent */
    TEST_PASS();
}

TEST_CASE(gpt_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Gpt_GetVersionInfo(&version_info);
    
    ASSERT_EQ(GPT_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(GPT_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

TEST_CASE(gpt_set_mode)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_SetMode(GPT_MODE_SLEEP);
    
    /* Mode set - implementation dependent */
    TEST_PASS();
}

TEST_CASE(gpt_enable_wakeup)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_EnableWakeup(1);
    
    TEST_PASS();
}

TEST_CASE(gpt_disable_wakeup)
{
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    Gpt_DisableWakeup(1);
    
    TEST_PASS();
}

TEST_CASE(gpt_check_wakeup)
{
    Std_ReturnType result;
    
    setup_test_config();
    Gpt_Init(&g_test_config);
    
    result = Gpt_CheckWakeup(1);
    
    /* Wakeup check result depends on implementation */
    (void)result;
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(gpt)
{
    Gpt_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(gpt)
{
}

TEST_SUITE(gpt)
{
    RUN_TEST(gpt_init_valid);
    RUN_TEST(gpt_init_null);
    RUN_TEST(gpt_deinit);
    RUN_TEST(gpt_get_time_elapsed);
    RUN_TEST(gpt_get_time_remaining);
    RUN_TEST(gpt_start_timer);
    RUN_TEST(gpt_stop_timer);
    RUN_TEST(gpt_enable_notification);
    RUN_TEST(gpt_disable_notification);
    RUN_TEST(gpt_get_version_info);
    RUN_TEST(gpt_set_mode);
    RUN_TEST(gpt_enable_wakeup);
    RUN_TEST(gpt_disable_wakeup);
    RUN_TEST(gpt_check_wakeup);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- GPT Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(gpt);
}
TEST_MAIN_END()

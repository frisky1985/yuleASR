/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : PWM Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Pwm.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Pwm_ConfigType g_test_config;
static Pwm_ChannelConfigType g_test_channels[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_channels[0].ChannelId = 0;
    g_test_channels[0].BaseAddress = 0x40010000;
    g_test_channels[0].ChannelClass = PWM_FIXED_PERIOD;
    g_test_channels[0].DefaultPeriod = 1000;
    g_test_channels[0].DefaultDutyCycle = 5000; /* 50% */
    g_test_channels[0].IdleState = PWM_IDLE_LOW;
    g_test_channels[0].Polarity = PWM_POLARITY_HIGH;
    g_test_channels[0].ClockSource = PWM_CLOCK_SYSTEM;
    g_test_channels[0].ClockPrescaler = 1;
    g_test_channels[0].NotificationSupported = TRUE;
    g_test_channels[0].NotificationFn = NULL;
    
    g_test_channels[1].ChannelId = 1;
    g_test_channels[1].BaseAddress = 0x40010100;
    g_test_channels[1].ChannelClass = PWM_VARIABLE_PERIOD;
    g_test_channels[1].DefaultPeriod = 2000;
    g_test_channels[1].DefaultDutyCycle = 2500; /* 25% */
    g_test_channels[1].IdleState = PWM_IDLE_HIGH;
    g_test_channels[1].Polarity = PWM_POLARITY_LOW;
    g_test_channels[1].ClockSource = PWM_CLOCK_BUS;
    g_test_channels[1].ClockPrescaler = 2;
    g_test_channels[1].NotificationSupported = FALSE;
    g_test_channels[1].NotificationFn = NULL;
    
    g_test_config.Channels = g_test_channels;
    g_test_config.NumChannels = 2;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.DeInitApi = TRUE;
    g_test_config.SetDutyCycleApi = TRUE;
    g_test_config.SetPeriodAndDutyApi = TRUE;
    g_test_config.SetOutputToIdleApi = TRUE;
    g_test_config.GetOutputStateApi = TRUE;
    g_test_config.NotificationSupported = TRUE;
    g_test_config.PowerStateSupported = FALSE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(pwm_init_valid)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    ASSERT_TRUE(Pwm_MockChannels[0].Initialized);
    ASSERT_TRUE(Pwm_MockChannels[1].Initialized);
    TEST_PASS();
}

TEST_CASE(pwm_init_null)
{
    Det_Mock_Reset();
    
    Pwm_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PWM_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(pwm_deinit)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    Pwm_DeInit();
    
    ASSERT_FALSE(Pwm_MockChannels[0].Initialized);
    TEST_PASS();
}

TEST_CASE(pwm_set_duty_cycle)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    Pwm_SetDutyCycle(0, 7500); /* 75% */
    
    ASSERT_EQ(7500, Pwm_MockChannels[0].DutyCycle);
    TEST_PASS();
}

TEST_CASE(pwm_set_period_and_duty)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    /* Variable period channel */
    Pwm_SetPeriodAndDuty(1, 3000, 4000);
    
    ASSERT_EQ(3000, Pwm_MockChannels[1].Period);
    ASSERT_EQ(4000, Pwm_MockChannels[1].DutyCycle);
    TEST_PASS();
}

TEST_CASE(pwm_set_output_to_idle)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    Pwm_SetOutputToIdle(0);
    
    ASSERT_EQ(PWM_IDLE_LOW, Pwm_MockChannels[0].OutputState);
    TEST_PASS();
}

TEST_CASE(pwm_get_output_state)
{
    Pwm_OutputStateType state;
    
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    Pwm_MockChannels[0].OutputState = PWM_HIGH;
    
    state = Pwm_GetOutputState(0);
    
    ASSERT_EQ(PWM_HIGH, state);
    TEST_PASS();
}

TEST_CASE(pwm_disable_notification)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    Pwm_DisableNotification(0);
    
    ASSERT_FALSE(Pwm_MockChannels[0].NotificationEnabled);
    TEST_PASS();
}

TEST_CASE(pwm_enable_notification)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    
    Pwm_EnableNotification(0, PWM_RISING_EDGE);
    
    ASSERT_TRUE(Pwm_MockChannels[0].NotificationEnabled);
    TEST_PASS();
}

TEST_CASE(pwm_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Pwm_GetVersionInfo(&version_info);
    
    ASSERT_EQ(PWM_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(PWM_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

TEST_CASE(pwm_set_period_and_duty_fixed)
{
    setup_test_config();
    Pwm_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Try to set period on fixed period channel - should error */
    Pwm_SetPeriodAndDuty(0, 5000, 3000);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PWM_E_PERIOD_UNCHANGEABLE, Det_MockData.ErrorId);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(pwm)
{
    Pwm_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(pwm)
{
}

TEST_SUITE(pwm)
{
    RUN_TEST(pwm_init_valid);
    RUN_TEST(pwm_init_null);
    RUN_TEST(pwm_deinit);
    RUN_TEST(pwm_set_duty_cycle);
    RUN_TEST(pwm_set_period_and_duty);
    RUN_TEST(pwm_set_output_to_idle);
    RUN_TEST(pwm_get_output_state);
    RUN_TEST(pwm_disable_notification);
    RUN_TEST(pwm_enable_notification);
    RUN_TEST(pwm_get_version_info);
    RUN_TEST(pwm_set_period_and_duty_fixed);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- PWM Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(pwm);
}
TEST_MAIN_END()

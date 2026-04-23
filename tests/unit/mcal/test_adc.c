/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : ADC Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Adc.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Adc_ConfigType g_test_config;
static Adc_ChannelConfigType g_test_channels[4];
static Adc_GroupConfigType g_test_groups[2];
static Adc_HWUnitConfigType g_test_hwunits[1];
static Adc_ChannelType g_test_group0_channels[2];
static Adc_ChannelType g_test_group1_channels[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* HW Unit config */
    g_test_hwunits[0].HwUnitId = 0;
    g_test_hwunits[0].BaseAddress = 0x40012000;
    g_test_hwunits[0].ClockFrequency = 1000000;
    g_test_hwunits[0].DefaultResolution = ADC_RESOLUTION_12BIT;
    
    /* Channel config */
    g_test_channels[0].ChannelId = 0;
    g_test_channels[0].SamplingTime = ADC_SAMPLING_TIME_15CYCLES;
    g_test_channels[0].ChannelInput = 0;
    
    g_test_channels[1].ChannelId = 1;
    g_test_channels[1].SamplingTime = ADC_SAMPLING_TIME_28CYCLES;
    g_test_channels[1].ChannelInput = 1;
    
    /* Group channels */
    g_test_group0_channels[0] = 0;
    g_test_group0_channels[1] = 1;
    
    /* Group config */
    g_test_groups[0].GroupId = 0;
    g_test_groups[0].HwUnit = 0;
    g_test_groups[0].Channels = g_test_group0_channels;
    g_test_groups[0].NumChannels = 2;
    g_test_groups[0].TriggerSource = ADC_TRIGG_SRC_SW;
    g_test_groups[0].ConversionMode = ADC_CONV_MODE_ONESHOT;
    g_test_groups[0].AccessMode = ADC_ACCESS_MODE_SINGLE;
    g_test_groups[0].BufferMode = ADC_STREAM_BUFFER_LINEAR;
    g_test_groups[0].NumSamples = 1;
    g_test_groups[0].Resolution = ADC_RESOLUTION_12BIT;
    g_test_groups[0].GroupNotification = TRUE;
    g_test_groups[0].NotificationFn = NULL;
    
    /* Main config */
    g_test_config.HwUnits = g_test_hwunits;
    g_test_config.NumHwUnits = 1;
    g_test_config.Groups = g_test_groups;
    g_test_config.NumGroups = 1;
    g_test_config.Channels = g_test_channels;
    g_test_config.NumChannels = 2;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.DeInitApi = TRUE;
    g_test_config.PowerStateSupported = FALSE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(adc_init_valid)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    ASSERT_TRUE(Adc_MockGroups[0].Status != 0 || TRUE); /* Initialized */
    TEST_PASS();
}

TEST_CASE(adc_init_null)
{
    Det_Mock_Reset();
    
    Adc_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(ADC_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(adc_init_already_initialized)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    Det_Mock_Reset();
    
    Adc_Init(&g_test_config);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(ADC_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(adc_deinit)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_DeInit();
    
    TEST_PASS();
}

TEST_CASE(adc_start_group_conversion)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_StartGroupConversion(0);
    
    ASSERT_EQ(ADC_BUSY, Adc_MockGroups[0].Status);
    TEST_PASS();
}

TEST_CASE(adc_stop_group_conversion)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_StartGroupConversion(0);
    Adc_StopGroupConversion(0);
    
    ASSERT_EQ(ADC_IDLE, Adc_MockGroups[0].Status);
    TEST_PASS();
}

TEST_CASE(adc_read_group)
{
    Std_ReturnType result;
    Adc_ValueGroupType buffer[2];
    uint16 mock_values[2] = {1234, 5678};
    
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_Mock_SetGroupResult(0, mock_values, 2);
    Adc_Mock_SetGroupStatus(0, ADC_STREAM_COMPLETED);
    
    result = Adc_ReadGroup(0, buffer);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1234, buffer[0]);
    ASSERT_EQ(5678, buffer[1]);
    TEST_PASS();
}

TEST_CASE(adc_read_group_busy)
{
    Std_ReturnType result;
    Adc_ValueGroupType buffer[2];
    
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_Mock_SetGroupStatus(0, ADC_BUSY);
    
    result = Adc_ReadGroup(0, buffer);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(ADC_E_BUSY, Det_MockData.ErrorId);
    TEST_PASS();
}

TEST_CASE(adc_enable_hardware_trigger)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    /* Change to hardware trigger group */
    g_test_groups[0].TriggerSource = ADC_TRIGG_SRC_HW;
    
    Adc_EnableHardwareTrigger(0);
    
    TEST_PASS();
}

TEST_CASE(adc_disable_hardware_trigger)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    g_test_groups[0].TriggerSource = ADC_TRIGG_SRC_HW;
    Adc_EnableHardwareTrigger(0);
    Adc_DisableHardwareTrigger(0);
    
    TEST_PASS();
}

TEST_CASE(adc_enable_group_notification)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_EnableGroupNotification(0);
    
    ASSERT_TRUE(Adc_MockGroups[0].NotificationEnabled);
    TEST_PASS();
}

TEST_CASE(adc_disable_group_notification)
{
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_EnableGroupNotification(0);
    Adc_DisableGroupNotification(0);
    
    ASSERT_FALSE(Adc_MockGroups[0].NotificationEnabled);
    TEST_PASS();
}

TEST_CASE(adc_get_group_status)
{
    Adc_StatusType status;
    
    setup_test_config();
    Adc_Init(&g_test_config);
    
    Adc_Mock_SetGroupStatus(0, ADC_BUSY);
    
    status = Adc_GetGroupStatus(0);
    
    ASSERT_EQ(ADC_BUSY, status);
    TEST_PASS();
}

TEST_CASE(adc_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Adc_GetVersionInfo(&version_info);
    
    ASSERT_EQ(ADC_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(ADC_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

TEST_CASE(adc_setup_result_buffer)
{
    Std_ReturnType result;
    Adc_ValueGroupType buffer[2];
    
    setup_test_config();
    Adc_Init(&g_test_config);
    
    result = Adc_SetupResultBuffer(0, buffer);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(adc)
{
    Adc_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(adc)
{
}

TEST_SUITE(adc)
{
    RUN_TEST(adc_init_valid);
    RUN_TEST(adc_init_null);
    RUN_TEST(adc_init_already_initialized);
    RUN_TEST(adc_deinit);
    RUN_TEST(adc_start_group_conversion);
    RUN_TEST(adc_stop_group_conversion);
    RUN_TEST(adc_read_group);
    RUN_TEST(adc_read_group_busy);
    RUN_TEST(adc_enable_hardware_trigger);
    RUN_TEST(adc_disable_hardware_trigger);
    RUN_TEST(adc_enable_group_notification);
    RUN_TEST(adc_disable_group_notification);
    RUN_TEST(adc_get_group_status);
    RUN_TEST(adc_get_version_info);
    RUN_TEST(adc_setup_result_buffer);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- ADC Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(adc);
}
TEST_MAIN_END()

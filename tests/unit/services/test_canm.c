/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : CanNm Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-30
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "CanNm.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static CanNm_ConfigType g_test_config;
static CanNm_ChannelConfigType g_channel_config;
static CanNm_TimingType g_timing_config;
static CanNm_PduType g_pdu_config;

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Setup timing configuration */
    g_timing_config.MsgCycleTime = 100;
    g_timing_config.MsgTimeoutTime = 200;
    g_timing_config.RepeatMessageTime = 500;
    g_timing_config.WaitBusSleepTime = 2000;
    g_timing_config.TimeoutTime = 1000;
    g_timing_config.ImmediateNmCycleTime = 10;
    g_timing_config.ImmediateNmTransmissions = 5;

    /* Setup PDU configuration */
    static uint8 tx_pdu_data[8];
    static uint8 rx_pdu_data[8];
    g_pdu_config.TxPduData = tx_pdu_data;
    g_pdu_config.RxPduData = rx_pdu_data;
    g_pdu_config.TxPduLength = 8;
    g_pdu_config.RxPduLength = 8;
    g_pdu_config.TxPduId = 0;
    g_pdu_config.RxPduId = 1;

    /* Setup channel configuration */
    g_channel_config.NodeId = 0x01;
    g_channel_config.ClusterId = 0x00;
    g_channel_config.PassiveModeEnabled = FALSE;
    g_channel_config.RepeatMessageIndEnabled = TRUE;
    g_channel_config.NodeDetectionEnabled = TRUE;
    g_channel_config.NodeIdEnabled = TRUE;
    g_channel_config.BusSynchronizationEnabled = TRUE;
    g_channel_config.RemoteSleepIndEnabled = TRUE;
    g_channel_config.UserDataEnabled = FALSE;
    g_channel_config.UserDataOffset = 2;
    g_channel_config.UserDataLength = 6;
    g_channel_config.Timing = &g_timing_config;
    g_channel_config.Pdu = &g_pdu_config;

    /* Setup main configuration */
    g_test_config.ChannelConfig = &g_channel_config;
    g_test_config.NumberOfChannels = 1;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.BusLoadReductionEnabled = FALSE;
    g_test_config.ComControlEnabled = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: CanNm_Init with valid config */
TEST_CASE(canm_init_valid_config)
{
    setup_test_config();
    
    CanNm_Init(&g_test_config);
    
    ASSERT_EQ(CANNM_STATE_BUS_SLEEP, CanNm_GetInternalState(0));
    TEST_PASS();
}

/* Test: CanNm_Init with NULL config */
TEST_CASE(canm_init_null_config)
{
    CanNm_Init(NULL_PTR);
    
    /* Should report error but not crash */
    TEST_PASS();
}

/* Test: CanNm_DeInit */
TEST_CASE(canm_deinit)
{
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    CanNm_DeInit();
    
    ASSERT_EQ(CANNM_STATE_UNINIT, CanNm_GetInternalState(0));
    TEST_PASS();
}

/* Test: CanNm_PassiveStartUp */
TEST_CASE(canm_passive_startup)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    result = CanNm_PassiveStartUp(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanNm_NetworkRequest */
TEST_CASE(canm_network_request)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    result = CanNm_NetworkRequest(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanNm_NetworkRelease */
TEST_CASE(canm_network_release)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    CanNm_NetworkRequest(0);
    
    result = CanNm_NetworkRelease(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanNm_GetVersionInfo */
TEST_CASE(canm_get_version_info)
{
    Std_VersionInfoType version_info;
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    CanNm_GetVersionInfo(&version_info);
    
    ASSERT_EQ(CANNM_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(CANNM_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(CANNM_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_PASS();
}

/* Test: CanNm_SetUserData */
TEST_CASE(canm_set_user_data)
{
    Std_ReturnType result;
    uint8 user_data[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    result = CanNm_SetUserData(0, user_data);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanNm_GetUserData */
TEST_CASE(canm_get_user_data)
{
    Std_ReturnType result;
    uint8 user_data[6] = {0};
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    result = CanNm_GetUserData(0, user_data);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanNm_SetSleepReadyBit */
TEST_CASE(canm_set_sleep_ready_bit)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    result = CanNm_SetSleepReadyBit(0, TRUE);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanNm_MainFunction */
TEST_CASE(canm_main_function)
{
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    /* Should not crash */
    CanNm_MainFunction();
    
    TEST_PASS();
}

/* Test: CanNm_RxIndication */
TEST_CASE(canm_rx_indication)
{
    PduInfoType pdu_info;
    uint8 pdu_data[8] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    pdu_info.SduDataPtr = pdu_data;
    pdu_info.SduLength = 8;
    pdu_info.MetaDataPtr = NULL_PTR;
    
    CanNm_RxIndication(1, &pdu_info);
    
    TEST_PASS();
}

/* Test: CanNm_TxConfirmation */
TEST_CASE(canm_tx_confirmation)
{
    setup_test_config();
    CanNm_Init(&g_test_config);
    
    CanNm_TxConfirmation(0);
    
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(canm)
{
}

TEST_SUITE_TEARDOWN(canm)
{
}

TEST_SUITE(canm)
{
    RUN_TEST(canm_init_valid_config);
    RUN_TEST(canm_init_null_config);
    RUN_TEST(canm_deinit);
    RUN_TEST(canm_passive_startup);
    RUN_TEST(canm_network_request);
    RUN_TEST(canm_network_release);
    RUN_TEST(canm_get_version_info);
    RUN_TEST(canm_set_user_data);
    RUN_TEST(canm_get_user_data);
    RUN_TEST(canm_set_sleep_ready_bit);
    RUN_TEST(canm_main_function);
    RUN_TEST(canm_rx_indication);
    RUN_TEST(canm_tx_confirmation);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(canm);
TEST_MAIN_END()

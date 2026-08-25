/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Service Discovery Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

// @tests src/bsw/services/sd/src/Sd.c  @tests src/bsw/services/sd/include/Sd.h

#include "../test_framework.h"
#include "Sd.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Sd_ConfigType g_test_config;

static void setup_default_config(void)
{
    g_test_config.MaxServices        = 16;
    g_test_config.MaxSubscriptions   = 8;
    g_test_config.OfferCycleTimeMs   = 2000;
    g_test_config.FindCycleTimeMs    = 3000;
    g_test_config.TtlDefault         = 5;
    g_test_config.DevErrorDetect     = TRUE;
    g_test_config.VersionInfoApi     = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(sd_init_valid)
{
    setup_default_config();
    Sd_Init(&g_test_config);
}

TEST_CASE(sd_init_null)
{
    Sd_Init(NULL_PTR);
}

TEST_CASE(sd_init_twice)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    Sd_Init(&g_test_config);
}

TEST_CASE(sd_deinit)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    Sd_DeInit();
}

TEST_CASE(sd_deinit_uninit)
{
    Sd_DeInit();
}

TEST_CASE(sd_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    Sd_Init(&g_test_config);
    Sd_GetVersionInfo(&ver);
    ASSERT_EQ(SD_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(SD_MODULE_ID, ver.moduleID);
}

TEST_CASE(sd_offer_service)
{
    Sd_Ipv4EndpointType ep;
    setup_default_config();
    Sd_Init(&g_test_config);

    ep.Addr     = 0xC0A80002;
    ep.Port     = 30490;
    ep.Protocol = SD_PROTO_UDP;

    ASSERT_EQ(E_OK, Sd_OfferService(0x1234, 0x0001, 1, 0, &ep));
}

TEST_CASE(sd_offer_service_null_endpoint)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, Sd_OfferService(0x1234, 0x0001, 1, 0, NULL_PTR));
}

TEST_CASE(sd_offer_service_uninit)
{
    Sd_Ipv4EndpointType ep;
    ep.Addr = 0xC0A80002;
    ep.Port = 30490;
    ep.Protocol = SD_PROTO_UDP;

    ASSERT_EQ(E_NOT_OK, Sd_OfferService(0x1234, 0x0001, 1, 0, &ep));
}

TEST_CASE(sd_stop_service)
{
    Sd_Ipv4EndpointType ep;
    setup_default_config();
    Sd_Init(&g_test_config);

    ep.Addr     = 0xC0A80002;
    ep.Port     = 30490;
    ep.Protocol = SD_PROTO_UDP;

    Sd_OfferService(0x1234, 0x0001, 1, 0, &ep);
    ASSERT_EQ(E_OK, Sd_StopService(0x1234, 0x0001));
}

TEST_CASE(sd_stop_service_not_offered)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, Sd_StopService(0x1234, 0x0001));
}

TEST_CASE(sd_find_service_not_found)
{
    Sd_Ipv4EndpointType ep;
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, Sd_FindService(0x1234, 0x0001, &ep));
}

TEST_CASE(sd_find_service_null)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, Sd_FindService(0x1234, 0x0001, NULL_PTR));
}

TEST_CASE(sd_subscribe_event_group)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_OK, Sd_SubscribeEventGroup(0x1234, 0x0001, 0x0001));
}

TEST_CASE(sd_subscribe_duplicate)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_OK, Sd_SubscribeEventGroup(0x1234, 0x0001, 0x0001));
    ASSERT_EQ(E_OK, Sd_SubscribeEventGroup(0x1234, 0x0001, 0x0001));
}

TEST_CASE(sd_unsubscribe_event_group)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    Sd_SubscribeEventGroup(0x1234, 0x0001, 0x0001);
    ASSERT_EQ(E_OK, Sd_UnsubscribeEventGroup(0x1234, 0x0001, 0x0001));
}

TEST_CASE(sd_unsubscribe_not_subscribed)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, Sd_UnsubscribeEventGroup(0x1234, 0x0001, 0x0001));
}

TEST_CASE(sd_set_event_status)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    Sd_SubscribeEventGroup(0x1234, 0x0001, 0x0001);
    ASSERT_EQ(E_OK, Sd_SetEventStatus(0x1234, 0x0001, 0x0001, SD_EVENTGROUP_READY));
}

TEST_CASE(sd_set_event_status_no_sub)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, Sd_SetEventStatus(0x1234, 0x0001, 0x0001, SD_EVENTGROUP_READY));
}

TEST_CASE(sd_main_function)
{
    setup_default_config();
    Sd_Init(&g_test_config);
    Sd_MainFunction();
}

TEST_CASE(sd_main_function_uninit)
{
    Sd_MainFunction();
}

TEST_CASE(sd_handle_message)
{
    uint8 msg[] = {0x00, 0x01, 0x02, 0x03};
    setup_default_config();
    Sd_Init(&g_test_config);
    ASSERT_EQ(E_OK, Sd_HandleMessage(msg, sizeof(msg)));
}

TEST_CASE(sd_handle_message_uninit)
{
    uint8 msg[] = {0x00};
    ASSERT_EQ(E_NOT_OK, Sd_HandleMessage(msg, sizeof(msg)));
}

TEST_CASE(sd_offer_multiple_services)
{
    Sd_Ipv4EndpointType ep;
    uint8 i;
    setup_default_config();
    Sd_Init(&g_test_config);

    ep.Addr     = 0xC0A80002;
    ep.Port     = 30490;
    ep.Protocol = SD_PROTO_UDP;

    for (i = 0U; i < 5U; i++)
    {
        ASSERT_EQ(E_OK, Sd_OfferService(0x1000 + i, 1, 1, 0, &ep));
    }
}

TEST_CASE(sd_offer_then_subscribe)
{
    Sd_Ipv4EndpointType ep;
    setup_default_config();
    Sd_Init(&g_test_config);

    ep.Addr     = 0xC0A80002;
    ep.Port     = 30490;
    ep.Protocol = SD_PROTO_UDP;

    Sd_OfferService(0x1234, 0x0001, 1, 0, &ep);
    ASSERT_EQ(E_OK, Sd_SubscribeEventGroup(0x1234, 0x0001, 1));
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(sd) { }

TEST_SUITE_TEARDOWN(sd) { }

TEST_SUITE(sd)
{
    RUN_TEST(sd_init_valid);
    RUN_TEST(sd_init_null);
    RUN_TEST(sd_init_twice);
    RUN_TEST(sd_deinit);
    RUN_TEST(sd_deinit_uninit);
    RUN_TEST(sd_get_version_info);
    RUN_TEST(sd_offer_service);
    RUN_TEST(sd_offer_service_null_endpoint);
    RUN_TEST(sd_offer_service_uninit);
    RUN_TEST(sd_stop_service);
    RUN_TEST(sd_stop_service_not_offered);
    RUN_TEST(sd_find_service_not_found);
    RUN_TEST(sd_find_service_null);
    RUN_TEST(sd_subscribe_event_group);
    RUN_TEST(sd_subscribe_duplicate);
    RUN_TEST(sd_unsubscribe_event_group);
    RUN_TEST(sd_unsubscribe_not_subscribed);
    RUN_TEST(sd_set_event_status);
    RUN_TEST(sd_set_event_status_no_sub);
    RUN_TEST(sd_main_function);
    RUN_TEST(sd_main_function_uninit);
    RUN_TEST(sd_handle_message);
    RUN_TEST(sd_handle_message_uninit);
    RUN_TEST(sd_offer_multiple_services);
    RUN_TEST(sd_offer_then_subscribe);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(sd);
TEST_MAIN_END()

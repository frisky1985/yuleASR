/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Socket Adapter Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "SoAd.h"
#include "TcpIp.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static TcpIp_ConfigType g_tcpip_config;
static SoAd_ConfigType g_soad_config;

static void setup_default_config(void)
{
    /* TCP/IP config */
    g_tcpip_config.NumSockets            = 8;
    g_tcpip_config.NumTcpPbufs           = 16;
    g_tcpip_config.TcpRcvBufSize         = 4096;
    g_tcpip_config.TcpSndBufSize         = 4096;
    g_tcpip_config.UdpRcvBufSize         = 2048;
    g_tcpip_config.EthLinkCheckIntervalMs = 100;

    /* SoAd config */
    g_soad_config.SocketConfigs    = NULL_PTR;
    g_soad_config.NumSockets       = 0;
    g_soad_config.ConnectionConfigs = NULL_PTR;
    g_soad_config.NumConnections   = 0;
    g_soad_config.PduRouteConfigs  = NULL_PTR;
    g_soad_config.NumPduRoutes     = 0;
    g_soad_config.DevErrorDetect   = TRUE;
    g_soad_config.VersionInfoApi   = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(soad_init_valid)
{
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
}

TEST_CASE(soad_init_null)
{
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(NULL_PTR);
}

TEST_CASE(soad_init_twice)
{
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_Init(&g_soad_config);
}

TEST_CASE(soad_deinit)
{
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_DeInit();
}

TEST_CASE(soad_deinit_uninit)
{
    SoAd_DeInit();
}

TEST_CASE(soad_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_GetVersionInfo(&ver);
    ASSERT_EQ(SOAD_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(SOAD_MODULE_ID, ver.moduleID);
}

TEST_CASE(soad_main_function)
{
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_MainFunction();
}

TEST_CASE(soad_main_function_uninit)
{
    SoAd_MainFunction();
}

TEST_CASE(soad_tx_confirmation)
{
    PduIdType pduId = 0;
    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_TxConfirmation(pduId, E_NOT_OK);
    /* Should not crash */
}

TEST_CASE(soad_rx_indication)
{
    PduIdType pduId = 0;
    PduInfoType info;
    uint8 data[8];
    info.SduDataPtr  = data;
    info.SduLength   = 8;
    info.MetaDataPtr = NULL_PTR;

    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_RxIndication(pduId, &info);
    /* Should not crash */
}

TEST_CASE(soad_trigger_transmit)
{
    PduIdType pduId = 0;
    PduInfoType info;
    uint8 data[8];
    info.SduDataPtr  = data;
    info.SduLength   = 8;
    info.MetaDataPtr = NULL_PTR;

    setup_default_config();
    TcpIp_Init(&g_tcpip_config);
    SoAd_Init(&g_soad_config);
    SoAd_TriggerTransmit(pduId, &info);
    /* Should not crash */
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(soad) { }

TEST_SUITE_TEARDOWN(soad) { }

TEST_SUITE(soad)
{
    RUN_TEST(soad_init_valid);
    RUN_TEST(soad_init_null);
    RUN_TEST(soad_init_twice);
    RUN_TEST(soad_deinit);
    RUN_TEST(soad_deinit_uninit);
    RUN_TEST(soad_get_version_info);
    RUN_TEST(soad_main_function);
    RUN_TEST(soad_main_function_uninit);
    RUN_TEST(soad_tx_confirmation);
    RUN_TEST(soad_rx_indication);
    RUN_TEST(soad_trigger_transmit);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(soad);
TEST_MAIN_END()

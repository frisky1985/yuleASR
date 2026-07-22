/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Ethernet Switch Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "EthSwt.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static EthSwt_PortConfigType g_port_configs[2];
static EthSwt_VlanConfigType g_vlan_config;
static EthSwt_ConfigType g_test_config;

static void setup_default_config(void)
{
    /* Port 0 config */
    g_port_configs[0].PortId = 0;
    g_port_configs[0].Speed  = ETHSWT_SPEED_1000MBPS;
    g_port_configs[0].Duplex = ETHSWT_DUPLEX_FULL;
    g_port_configs[0].Enable = ETHSWT_PORT_ENABLED;
    g_port_configs[0].Pvid   = 1;
    g_port_configs[0].MacAddress.octet[0] = 0x00;
    g_port_configs[0].MacAddress.octet[1] = 0x11;
    g_port_configs[0].MacAddress.octet[2] = 0x22;
    g_port_configs[0].MacAddress.octet[3] = 0x33;
    g_port_configs[0].MacAddress.octet[4] = 0x44;
    g_port_configs[0].MacAddress.octet[5] = 0x55;

    /* Port 1 config */
    g_port_configs[1].PortId = 1;
    g_port_configs[1].Speed  = ETHSWT_SPEED_100MBPS;
    g_port_configs[1].Duplex = ETHSWT_DUPLEX_HALF;
    g_port_configs[1].Enable = ETHSWT_PORT_ENABLED;
    g_port_configs[1].Pvid   = 1;
    g_port_configs[1].MacAddress.octet[0] = 0x00;
    g_port_configs[1].MacAddress.octet[1] = 0x11;
    g_port_configs[1].MacAddress.octet[2] = 0x22;
    g_port_configs[1].MacAddress.octet[3] = 0x33;
    g_port_configs[1].MacAddress.octet[4] = 0x44;
    g_port_configs[1].MacAddress.octet[5] = 0x56;

    /* VLAN config */
    g_vlan_config.VlanId   = 1;
    g_vlan_config.PortMask = 0x03;  /* Ports 0 & 1 */
    g_vlan_config.Tagged   = TRUE;

    /* Main config */
    g_test_config.NumPorts       = 2;
    g_test_config.PortConfigs    = g_port_configs;
    g_test_config.NumVlans       = 1;
    g_test_config.VlanConfigs    = &g_vlan_config;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(ethswt_init_valid)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
}

TEST_CASE(ethswt_init_null)
{
    EthSwt_Init(NULL_PTR);
}

TEST_CASE(ethswt_init_twice)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_Init(&g_test_config);
}

TEST_CASE(ethswt_deinit)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_DeInit();
}

TEST_CASE(ethswt_deinit_uninit)
{
    EthSwt_DeInit();
}

TEST_CASE(ethswt_set_port_enable)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetPortEnable(0, ETHSWT_PORT_DISABLED));
    ASSERT_EQ(E_OK, EthSwt_SetPortEnable(0, ETHSWT_PORT_ENABLED));
}

TEST_CASE(ethswt_set_port_enable_invalid)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetPortEnable(99, ETHSWT_PORT_ENABLED));
}

TEST_CASE(ethswt_set_speed)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetSpeed(0, ETHSWT_SPEED_1000MBPS, ETHSWT_DUPLEX_FULL));
}

TEST_CASE(ethswt_set_speed_invalid_port)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetSpeed(99, ETHSWT_SPEED_1000MBPS, ETHSWT_DUPLEX_FULL));
}

TEST_CASE(ethswt_get_link_state)
{
    EthSwt_LinkStateType state;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetLinkState(0, &state));
}

TEST_CASE(ethswt_get_link_state_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetLinkState(0, NULL_PTR));
}

TEST_CASE(ethswt_get_link_state_invalid_port)
{
    EthSwt_LinkStateType state;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetLinkState(99, &state));
}

TEST_CASE(ethswt_config_vlan)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    vlan.VlanId   = 10;
    vlan.PortMask = 0x05;
    vlan.Tagged   = FALSE;

    ASSERT_EQ(E_OK, EthSwt_ConfigVlan(&vlan));
}

TEST_CASE(ethswt_config_vlan_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_ConfigVlan(NULL_PTR));
}

TEST_CASE(ethswt_forward_frame)
{
    uint8 frame[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x08, 0x00};
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame)));
}

TEST_CASE(ethswt_forward_frame_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_ForwardFrame(0, 0x02, NULL_PTR, 0));
}

TEST_CASE(ethswt_get_port_stats)
{
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetPortStats(0, &stats));
}

TEST_CASE(ethswt_get_port_stats_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPortStats(0, NULL_PTR));
}

TEST_CASE(ethswt_set_mac_filter)
{
    EthSwt_MacAddrType mac;
    mac.octet[0] = 0x00;
    mac.octet[1] = 0x11;
    mac.octet[2] = 0x22;
    mac.octet[3] = 0x33;
    mac.octet[4] = 0x44;
    mac.octet[5] = 0x55;

    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetMacFilter(0, &mac, TRUE));
}

TEST_CASE(ethswt_set_mac_filter_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetMacFilter(0, NULL_PTR, TRUE));
}

TEST_CASE(ethswt_main_function)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_MainFunction();
}

TEST_CASE(ethswt_main_function_uninit)
{
    EthSwt_MainFunction();
}

TEST_CASE(ethswt_reset)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_Reset());
}

TEST_CASE(ethswt_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_GetVersionInfo(&ver);
    ASSERT_EQ(ETHSWT_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(ETHSWT_MODULE_ID, ver.moduleID);
}

TEST_CASE(ethswt_forward_stats_update)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(1U, stats.TxFrames);
    ASSERT_EQ(sizeof(frame), stats.TxBytes);

    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.RxFrames);
    ASSERT_EQ(sizeof(frame), stats.RxBytes);
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(ethswt) { }

TEST_SUITE_TEARDOWN(ethswt) { }

TEST_SUITE(ethswt)
{
    RUN_TEST(ethswt_init_valid);
    RUN_TEST(ethswt_init_null);
    RUN_TEST(ethswt_init_twice);
    RUN_TEST(ethswt_deinit);
    RUN_TEST(ethswt_deinit_uninit);
    RUN_TEST(ethswt_set_port_enable);
    RUN_TEST(ethswt_set_port_enable_invalid);
    RUN_TEST(ethswt_set_speed);
    RUN_TEST(ethswt_set_speed_invalid_port);
    RUN_TEST(ethswt_get_link_state);
    RUN_TEST(ethswt_get_link_state_null);
    RUN_TEST(ethswt_get_link_state_invalid_port);
    RUN_TEST(ethswt_config_vlan);
    RUN_TEST(ethswt_config_vlan_null);
    RUN_TEST(ethswt_forward_frame);
    RUN_TEST(ethswt_forward_frame_null);
    RUN_TEST(ethswt_get_port_stats);
    RUN_TEST(ethswt_get_port_stats_null);
    RUN_TEST(ethswt_set_mac_filter);
    RUN_TEST(ethswt_set_mac_filter_null);
    RUN_TEST(ethswt_main_function);
    RUN_TEST(ethswt_main_function_uninit);
    RUN_TEST(ethswt_reset);
    RUN_TEST(ethswt_get_version_info);
    RUN_TEST(ethswt_forward_stats_update);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(ethswt);
TEST_MAIN_END()

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Ethernet Switch Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* B2 deep-dive (2026-08-09): compiles the real production source EthSwt.c
* with a Det_ReportError mock, covering init lifecycle, port config,
* VLAN member table / ingress-egress filtering / PVID / VID-PCP mapping,
* flow control (pause frames, watermarks), per-port statistics with
* GetStatistics/ResetStatistics, and port mirroring.
*
* Use: cmake -DBUILD_TESTING=ON .. && make EthSwt_UnitTest && ctest -R EthSwt
==================================================================================================*/

// @tests src/bsw/ecual/ethswt/src/EthSwt.c  @tests src/bsw/ecual/ethswt/include/EthSwt.h

#include "../test_framework.h"
#include "EthSwt.h"
#include <string.h>

/*==================================================================================================
*                                      DET MOCK
*==================================================================================================*/
#define MOCK_MAX_DET_CALLS                      (64U)

static uint16 g_det_module[MOCK_MAX_DET_CALLS];
static uint8  g_det_api[MOCK_MAX_DET_CALLS];
static uint8  g_det_err[MOCK_MAX_DET_CALLS];
static uint16 g_det_count;

Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    (void)InstanceId;
    if (g_det_count < (uint16)MOCK_MAX_DET_CALLS)
    {
        g_det_module[g_det_count] = ModuleId;
        g_det_api[g_det_count] = ApiId;
        g_det_err[g_det_count] = ErrorId;
        g_det_count++;
    }
    return E_OK;
}

static void mock_det_reset(void)
{
    g_det_count = 0U;
    (void)memset(g_det_module, 0, sizeof(g_det_module));
    (void)memset(g_det_api, 0, sizeof(g_det_api));
    (void)memset(g_det_err, 0, sizeof(g_det_err));
}

static uint16 mock_det_count_for(uint8 ApiId)
{
    uint16 i;
    uint16 count = 0U;
    for (i = 0U; i < g_det_count; i++)
    {
        if (g_det_api[i] == ApiId)
        {
            count++;
        }
    }
    return count;
}

/*==================================================================================================
*                                      TEST GLOBALS
*==================================================================================================*/
static EthSwt_PortConfigType g_port_configs[2];
static EthSwt_VlanConfigType g_vlan_config;
static EthSwt_ConfigType g_test_config;

/* Ensure clean module state between cases */
static void test_ethswt_reset_module(void)
{
    EthSwt_DeInit();
    mock_det_reset();
}

static void setup_default_config(void)
{
    /* Fresh module state for every case */
    test_ethswt_reset_module();

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
    g_vlan_config.VlanPriority = 3;
    g_vlan_config.DropUntagged = FALSE;

    /* Main config */
    g_test_config.NumPorts       = 2;
    g_test_config.PortConfigs    = g_port_configs;
    g_test_config.NumVlans       = 1;
    g_test_config.VlanConfigs    = &g_vlan_config;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.FlowControlConfigs = NULL_PTR;
    g_test_config.MirrorConfig = NULL_PTR;
}

/*==================================================================================================
*                                      TEST CASES
*==================================================================================================*/

/** @req SWS_EthSwt_00001 */
TEST_CASE(ethswt_init_valid)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
}

/** @req SWS_EthSwt_00001 */
TEST_CASE(ethswt_init_null)
{
    EthSwt_Init(NULL_PTR);
}

/** @req SWS_EthSwt_00001 */
TEST_CASE(ethswt_init_twice)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_Init(&g_test_config);
}

/** @req SWS_EthSwt_00002 */
TEST_CASE(ethswt_deinit)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_DeInit();
}

/** @req SWS_EthSwt_00002 */
TEST_CASE(ethswt_deinit_uninit)
{
    EthSwt_DeInit();
}

/** @req SWS_EthSwt_00004 */
TEST_CASE(ethswt_set_port_enable)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetPortEnable(0, ETHSWT_PORT_DISABLED));
    ASSERT_EQ(E_OK, EthSwt_SetPortEnable(0, ETHSWT_PORT_ENABLED));
}

/** @req SWS_EthSwt_00004 */
TEST_CASE(ethswt_set_port_enable_invalid)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetPortEnable(99, ETHSWT_PORT_ENABLED));
}

/** @req SWS_EthSwt_00006 */
TEST_CASE(ethswt_set_speed)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetSpeed(0, ETHSWT_SPEED_1000MBPS, ETHSWT_DUPLEX_FULL));
}

/** @req SWS_EthSwt_00006 */
TEST_CASE(ethswt_set_speed_invalid_port)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetSpeed(99, ETHSWT_SPEED_1000MBPS, ETHSWT_DUPLEX_FULL));
}

/** @req SWS_EthSwt_00008 */
TEST_CASE(ethswt_get_link_state)
{
    EthSwt_LinkStateType state;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetLinkState(0, &state));
}

/** @req SWS_EthSwt_00008 */
TEST_CASE(ethswt_get_link_state_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetLinkState(0, NULL_PTR));
}

/** @req SWS_EthSwt_00008 */
TEST_CASE(ethswt_get_link_state_invalid_port)
{
    EthSwt_LinkStateType state;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetLinkState(99, &state));
}

/** @req SWS_EthSwt_00009 */
TEST_CASE(ethswt_config_vlan)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    vlan.VlanId   = 10;
    vlan.PortMask = 0x05;
    vlan.Tagged   = FALSE;
    vlan.VlanPriority = 0;
    vlan.DropUntagged = FALSE;

    ASSERT_EQ(E_OK, EthSwt_ConfigVlan(&vlan));
}

/** @req SWS_EthSwt_00009 */
TEST_CASE(ethswt_config_vlan_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_ConfigVlan(NULL_PTR));
}

/** @req SWS_EthSwt_00018 */
TEST_CASE(ethswt_forward_frame)
{
    uint8 frame[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x08, 0x00};
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame)));
}

/** @req SWS_EthSwt_00018 */
TEST_CASE(ethswt_forward_frame_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_ForwardFrame(0, 0x02, NULL_PTR, 0));
}

/** @req SWS_EthSwt_00020 */
TEST_CASE(ethswt_get_port_stats)
{
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetPortStats(0, &stats));
}

/** @req SWS_EthSwt_00020 */
TEST_CASE(ethswt_get_port_stats_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPortStats(0, NULL_PTR));
}

/** @req SWS_EthSwt_00023 */
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

/** @req SWS_EthSwt_00023 */
TEST_CASE(ethswt_set_mac_filter_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetMacFilter(0, NULL_PTR, TRUE));
}

/** @req SWS_EthSwt_00032 */
TEST_CASE(ethswt_main_function)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_MainFunction();
}

/** @req SWS_EthSwt_00032 */
TEST_CASE(ethswt_main_function_uninit)
{
    EthSwt_MainFunction();
}

/** @req SWS_EthSwt_00033 */
TEST_CASE(ethswt_reset)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_Reset());
}

/** @req SWS_EthSwt_00003 */
TEST_CASE(ethswt_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    EthSwt_GetVersionInfo(&ver);
    ASSERT_EQ(ETHSWT_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(ETHSWT_MODULE_ID, ver.moduleID);
}

/** @req SWS_EthSwt_00018 */
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

/* ══════════════════════════════════════════════════════════════════════
 * B2: port query APIs (GetPortEnable / GetSpeed / GetMacFilter)
 * ══════════════════════════════════════════════════════════════════════ */

/** @req SWS_EthSwt_00005 */
TEST_CASE(ethswt_get_port_enable)
{
    EthSwt_PortEnableType enable;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetPortEnable(0, &enable));
    ASSERT_EQ(ETHSWT_PORT_ENABLED, enable);
}

/** @req SWS_EthSwt_00005 */
TEST_CASE(ethswt_get_port_enable_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPortEnable(0, NULL_PTR));
}

/** @req SWS_EthSwt_00005 */
TEST_CASE(ethswt_get_port_enable_invalid)
{
    EthSwt_PortEnableType enable;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPortEnable(99, &enable));
}

/** @req SWS_EthSwt_00007 */
TEST_CASE(ethswt_get_speed)
{
    EthSwt_SpeedType speed;
    EthSwt_DuplexType duplex;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetSpeed(0, &speed, &duplex));
    ASSERT_EQ(ETHSWT_SPEED_1000MBPS, speed);
    ASSERT_EQ(ETHSWT_DUPLEX_FULL, duplex);
}

/** @req SWS_EthSwt_00007 */
TEST_CASE(ethswt_get_speed_null)
{
    EthSwt_SpeedType speed;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetSpeed(0, &speed, NULL_PTR));
}

/** @req SWS_EthSwt_00024 */
TEST_CASE(ethswt_get_mac_filter)
{
    EthSwt_MacAddrType mac;
    boolean enable;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_GetMacFilter(0, &mac, &enable));
    ASSERT_EQ(FALSE, enable);
}

/** @req SWS_EthSwt_00024 */
TEST_CASE(ethswt_get_mac_filter_null)
{
    EthSwt_MacAddrType mac;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetMacFilter(0, &mac, NULL_PTR));
}

/* ══════════════════════════════════════════════════════════════════════
 * B2: VLAN — member table, ingress/egress filtering, PVID, VID-PCP
 * ══════════════════════════════════════════════════════════════════════ */

/** @req SWS_EthSwt_00010 */
TEST_CASE(ethswt_vlan_set_get)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    vlan.VlanId = 100;
    vlan.PortMask = 0x05;
    vlan.Tagged = TRUE;
    vlan.VlanPriority = 5;
    vlan.DropUntagged = TRUE;

    ASSERT_EQ(E_OK, EthSwt_SetVlanConfig(&vlan));
    ASSERT_EQ(E_OK, EthSwt_GetVlanConfig(100, &vlan));
    ASSERT_EQ(0x05U, vlan.PortMask);
    ASSERT_EQ(5U, vlan.VlanPriority);
    ASSERT_EQ(TRUE, vlan.DropUntagged);
}

/** @req SWS_EthSwt_00010 */
TEST_CASE(ethswt_vlan_set_upsert)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* Upsert: replace existing VLAN 1 entry */
    vlan.VlanId = 1;
    vlan.PortMask = 0x01;
    vlan.Tagged = FALSE;
    vlan.VlanPriority = 7;
    vlan.DropUntagged = FALSE;

    ASSERT_EQ(E_OK, EthSwt_SetVlanConfig(&vlan));
    ASSERT_EQ(E_OK, EthSwt_GetVlanConfig(1, &vlan));
    ASSERT_EQ(0x01U, vlan.PortMask);
    ASSERT_EQ(7U, vlan.VlanPriority);
}

/** @req SWS_EthSwt_00010 */
TEST_CASE(ethswt_vlan_set_invalid_pcp)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    vlan.VlanId = 200;
    vlan.PortMask = 0x03;
    vlan.Tagged = TRUE;
    vlan.VlanPriority = 8;  /* PCP > 7 */
    vlan.DropUntagged = FALSE;

    ASSERT_EQ(E_NOT_OK, EthSwt_SetVlanConfig(&vlan));
    ASSERT_EQ(1U, mock_det_count_for(ETHSWT_SID_SETVLANCONFIG));
}

/** @req SWS_EthSwt_00010 */
TEST_CASE(ethswt_vlan_set_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetVlanConfig(NULL_PTR));
}

/** @req SWS_EthSwt_00011 */
TEST_CASE(ethswt_vlan_get_missing)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetVlanConfig(999, &vlan));
}

/** @req SWS_EthSwt_00011 */
TEST_CASE(ethswt_vlan_get_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetVlanConfig(1, NULL_PTR));
}

/** @req SWS_EthSwt_00012 */
TEST_CASE(ethswt_vlan_add_remove_member)
{
    EthSwt_VlanConfigType vlan;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* VLAN 1 starts as {0,1} — add port 2 */
    ASSERT_EQ(E_OK, EthSwt_AddVlanMember(1, 2, TRUE));
    ASSERT_EQ(E_OK, EthSwt_GetVlanConfig(1, &vlan));
    ASSERT_EQ(0x07U, vlan.PortMask);

    /* Remove port 2 again */
    ASSERT_EQ(E_OK, EthSwt_RemoveVlanMember(1, 2));
    ASSERT_EQ(E_OK, EthSwt_GetVlanConfig(1, &vlan));
    ASSERT_EQ(0x03U, vlan.PortMask);
}

/** @req SWS_EthSwt_00012 */
TEST_CASE(ethswt_vlan_add_member_missing_vlan)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_AddVlanMember(500, 2, TRUE));
}

/** @req SWS_EthSwt_00013 */
TEST_CASE(ethswt_vlan_remove_member_missing_vlan)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_RemoveVlanMember(500, 2));
}

/** @req SWS_EthSwt_00012 */
TEST_CASE(ethswt_vlan_add_member_invalid_port)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_AddVlanMember(1, 99, TRUE));
}

/** @req SWS_EthSwt_00014 */
TEST_CASE(ethswt_pvid_set_get)
{
    uint16 pvid;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetPvid(0, 42));
    ASSERT_EQ(E_OK, EthSwt_GetPvid(0, &pvid));
    ASSERT_EQ(42U, pvid);
}

/** @req SWS_EthSwt_00015 */
TEST_CASE(ethswt_pvid_invalid_port)
{
    uint16 pvid;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPvid(99, &pvid));
}

/** @req SWS_EthSwt_00015 */
TEST_CASE(ethswt_pvid_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPvid(0, NULL_PTR));
}

/** @req SWS_EthSwt_00016 */
TEST_CASE(ethswt_vid_pcp_map)
{
    uint8 pcp;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetVidPcpMap(1, 6));
    ASSERT_EQ(E_OK, EthSwt_GetVidPcpMap(1, &pcp));
    ASSERT_EQ(6U, pcp);
}

/** @req SWS_EthSwt_00016 */
TEST_CASE(ethswt_vid_pcp_invalid)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetVidPcpMap(1, 8));
    ASSERT_EQ(1U, mock_det_count_for(ETHSWT_SID_SETVIDPCP));
}

/** @req SWS_EthSwt_00017 */
TEST_CASE(ethswt_vid_pcp_missing_vlan)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetVidPcpMap(999, 3));
}

/** @req SWS_EthSwt_00019 */
TEST_CASE(ethswt_forward_vlan_member)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* Tagged VLAN 1 frame from port 0 to port 1 (both members) */
    ASSERT_EQ(E_OK, EthSwt_ForwardFrameVlan(0, 1, 0x02, frame, sizeof(frame)));
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.RxFrames);
    ASSERT_EQ(1U, stats.RxVlanFrames);
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(1U, stats.TxVlanFrames);
}

/** @req SWS_EthSwt_00019 */
TEST_CASE(ethswt_forward_vlan_ingress_filter)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* Port 2 is NOT a member of VLAN 1 (mask 0x03) — ingress filtered */
    ASSERT_EQ(E_OK, EthSwt_ForwardFrameVlan(2, 1, 0x03, frame, sizeof(frame)));
    EthSwt_GetPortStats(2, &stats);
    ASSERT_EQ(1U, stats.RxFilteredFrames);
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(0U, stats.RxFrames);
}

/** @req SWS_EthSwt_00019 */
TEST_CASE(ethswt_forward_vlan_egress_filter)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* Port 2 is NOT a member of VLAN 1 — egress filtered, port 1 still gets it */
    ASSERT_EQ(E_OK, EthSwt_ForwardFrameVlan(0, 1, 0x06, frame, sizeof(frame)));
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(1U, stats.TxFilteredFrames);
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.RxFrames);
    EthSwt_GetPortStats(2, &stats);
    ASSERT_EQ(0U, stats.RxFrames);
}

/** @req SWS_EthSwt_00019 */
TEST_CASE(ethswt_forward_vlan_drop_untagged)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_VlanConfigType vlan;
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* Enable drop-untagged on VLAN 1 */
    vlan.VlanId = 1;
    vlan.PortMask = 0x03;
    vlan.Tagged = TRUE;
    vlan.VlanPriority = 3;
    vlan.DropUntagged = TRUE;
    ASSERT_EQ(E_OK, EthSwt_SetVlanConfig(&vlan));

    /* Untagged frame (VlanId 0) on port 0, PVID 1 → dropped */
    ASSERT_EQ(E_OK, EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame)));
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(1U, stats.RxFilteredFrames);
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(0U, stats.RxFrames);
}

/* ══════════════════════════════════════════════════════════════════════
 * B2: flow control — pause frames, high/low watermarks
 * ══════════════════════════════════════════════════════════════════════ */

/** @req SWS_EthSwt_00025 */
TEST_CASE(ethswt_flow_control_set_get)
{
    EthSwt_FlowControlConfigType fc;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    fc.TxPauseEnable = TRUE;
    fc.RxPauseEnable = TRUE;
    fc.HighWatermark = 16;
    fc.LowWatermark = 4;
    fc.PauseTime = 256;

    ASSERT_EQ(E_OK, EthSwt_SetFlowControl(0, &fc));
    ASSERT_EQ(E_OK, EthSwt_GetFlowControl(0, &fc));
    ASSERT_EQ(TRUE, fc.TxPauseEnable);
    ASSERT_EQ(16U, fc.HighWatermark);
    ASSERT_EQ(4U, fc.LowWatermark);
}

/** @req SWS_EthSwt_00025 */
TEST_CASE(ethswt_flow_control_invalid_watermark)
{
    EthSwt_FlowControlConfigType fc;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    fc.TxPauseEnable = TRUE;
    fc.RxPauseEnable = FALSE;
    fc.HighWatermark = 4;   /* high <= low → invalid */
    fc.LowWatermark = 8;
    fc.PauseTime = 256;

    ASSERT_EQ(E_NOT_OK, EthSwt_SetFlowControl(0, &fc));
    ASSERT_EQ(1U, mock_det_count_for(ETHSWT_SID_SETFLOWCONTROL));
}

/** @req SWS_EthSwt_00025 */
TEST_CASE(ethswt_flow_control_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetFlowControl(0, NULL_PTR));
}

/** @req SWS_EthSwt_00025 */
TEST_CASE(ethswt_flow_control_pause_emission)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_FlowControlConfigType fc;
    EthSwt_PortStatsType stats;
    uint8 i;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* High watermark 3, low 1 → 3rd forwarded frame triggers pause */
    fc.TxPauseEnable = TRUE;
    fc.RxPauseEnable = FALSE;
    fc.HighWatermark = 3;
    fc.LowWatermark = 1;
    fc.PauseTime = 64;
    ASSERT_EQ(E_OK, EthSwt_SetFlowControl(0, &fc));

    for (i = 0U; i < 3U; i++)
    {
        (void)EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));
    }
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(1U, stats.TxPauseFrames);

    /* Drain below low watermark → pause released */
    (void)EthSwt_MainFunction();   /* depth 2 */
    (void)EthSwt_MainFunction();   /* depth 1 == low → release */
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(1U, stats.TxPauseFrames);   /* no duplicate emission yet */

    (void)EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));  /* depth 2, pause re-arm */
    (void)EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));  /* depth 3 → pause again */
    EthSwt_GetPortStats(0, &stats);
    ASSERT_EQ(2U, stats.TxPauseFrames);
}

/** @req SWS_EthSwt_00027 */
TEST_CASE(ethswt_flow_control_pause_time)
{
    uint16 pause_time;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_OK, EthSwt_SetPauseTime(0, 1024));
    ASSERT_EQ(E_OK, EthSwt_GetPauseTime(0, &pause_time));
    ASSERT_EQ(1024U, pause_time);
}

/** @req SWS_EthSwt_00029 */
TEST_CASE(ethswt_flow_control_rx_pause)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_FlowControlConfigType fc;
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    /* Port 1 honors received pause frames */
    fc.TxPauseEnable = FALSE;
    fc.RxPauseEnable = TRUE;
    fc.HighWatermark = 8;
    fc.LowWatermark = 2;
    fc.PauseTime = 128;
    ASSERT_EQ(E_OK, EthSwt_SetFlowControl(1, &fc));

    /* Pause arrives on port 1 → frames forwarded to it are dropped */
    ASSERT_EQ(E_OK, EthSwt_IndicatePause(1, TRUE));
    ASSERT_EQ(E_OK, EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame)));
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.RxPauseFrames);
    ASSERT_EQ(1U, stats.DroppedFrames);
    ASSERT_EQ(0U, stats.RxFrames);

    /* Pause released → forwarding resumes */
    ASSERT_EQ(E_OK, EthSwt_IndicatePause(1, FALSE));
    (void)EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.RxFrames);
}

/* ══════════════════════════════════════════════════════════════════════
 * B2: statistics — GetStatistics / ResetStatistics
 * ══════════════════════════════════════════════════════════════════════ */

/** @req SWS_EthSwt_00021 */
TEST_CASE(ethswt_get_statistics)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    ASSERT_EQ(E_OK, EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame)));
    ASSERT_EQ(E_OK, EthSwt_GetStatistics(0, &stats));
    ASSERT_EQ(1U, stats.TxFrames);
    ASSERT_EQ(sizeof(frame), stats.TxBytes);
}

/** @req SWS_EthSwt_00021 */
TEST_CASE(ethswt_get_statistics_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetStatistics(0, NULL_PTR));
}

/** @req SWS_EthSwt_00021 */
TEST_CASE(ethswt_get_statistics_invalid_port)
{
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_GetStatistics(99, &stats));
}

/** @req SWS_EthSwt_00022 */
TEST_CASE(ethswt_reset_statistics)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    (void)EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));
    ASSERT_EQ(E_OK, EthSwt_ResetStatistics(0));
    ASSERT_EQ(E_OK, EthSwt_GetStatistics(0, &stats));
    ASSERT_EQ(0U, stats.TxFrames);
    ASSERT_EQ(0U, stats.RxBytes);
}

/** @req SWS_EthSwt_00022 */
TEST_CASE(ethswt_reset_statistics_all)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    (void)EthSwt_ForwardFrame(0, 0x02, frame, sizeof(frame));
    ASSERT_EQ(E_OK, EthSwt_ResetStatistics(ETHSWT_ALL_PORTS));
    ASSERT_EQ(E_OK, EthSwt_GetStatistics(0, &stats));
    ASSERT_EQ(0U, stats.TxFrames);
    ASSERT_EQ(E_OK, EthSwt_GetStatistics(1, &stats));
    ASSERT_EQ(0U, stats.RxFrames);
}

/** @req SWS_EthSwt_00022 */
TEST_CASE(ethswt_reset_statistics_invalid_port)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_ResetStatistics(99));
}

/* ══════════════════════════════════════════════════════════════════════
 * B2: mirroring — mirror source/dest port config
 * ══════════════════════════════════════════════════════════════════════ */

/** @req SWS_EthSwt_00030 */
TEST_CASE(ethswt_mirror_set_get)
{
    EthSwt_MirrorConfigType mirror;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    mirror.MirrorSourcePortMask = 0x01;   /* mirror port 0 */
    mirror.MirrorDestinationPort = 1;
    mirror.MirrorEnabled = TRUE;

    ASSERT_EQ(E_OK, EthSwt_SetPortMirroring(&mirror));
    ASSERT_EQ(E_OK, EthSwt_GetPortMirroring(&mirror));
    ASSERT_EQ(0x01U, mirror.MirrorSourcePortMask);
    ASSERT_EQ(1U, mirror.MirrorDestinationPort);
    ASSERT_EQ(TRUE, mirror.MirrorEnabled);
}

/** @req SWS_EthSwt_00030 */
TEST_CASE(ethswt_mirror_set_invalid_dest)
{
    EthSwt_MirrorConfigType mirror;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    mirror.MirrorSourcePortMask = 0x01;
    mirror.MirrorDestinationPort = 99;
    mirror.MirrorEnabled = TRUE;

    ASSERT_EQ(E_NOT_OK, EthSwt_SetPortMirroring(&mirror));
    ASSERT_EQ(1U, mock_det_count_for(ETHSWT_SID_SETPORTMIRRORING));
}

/** @req SWS_EthSwt_00030 */
TEST_CASE(ethswt_mirror_set_null)
{
    setup_default_config();
    EthSwt_Init(&g_test_config);
    ASSERT_EQ(E_NOT_OK, EthSwt_SetPortMirroring(NULL_PTR));
}

/** @req SWS_EthSwt_00030 */
TEST_CASE(ethswt_mirror_forward_copy)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_MirrorConfigType mirror;
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    mirror.MirrorSourcePortMask = 0x01;   /* mirror port 0 traffic */
    mirror.MirrorDestinationPort = 1;
    mirror.MirrorEnabled = TRUE;
    ASSERT_EQ(E_OK, EthSwt_SetPortMirroring(&mirror));

    /* Port 0 → port 0 (self) plus mirrored copy to port 1 */
    (void)EthSwt_ForwardFrame(0, 0x01, frame, sizeof(frame));
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.RxFrames);
    ASSERT_EQ(1U, stats.MirroredFrames);

    /* Non-source port traffic is not mirrored */
    (void)EthSwt_ForwardFrame(1, 0x01, frame, sizeof(frame));
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(1U, stats.MirroredFrames);   /* unchanged */
}

/** @req SWS_EthSwt_00030 */
TEST_CASE(ethswt_mirror_disabled)
{
    uint8 frame[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    EthSwt_MirrorConfigType mirror;
    EthSwt_PortStatsType stats;
    setup_default_config();
    EthSwt_Init(&g_test_config);

    mirror.MirrorSourcePortMask = 0x01;
    mirror.MirrorDestinationPort = 1;
    mirror.MirrorEnabled = FALSE;
    ASSERT_EQ(E_OK, EthSwt_SetPortMirroring(&mirror));

    (void)EthSwt_ForwardFrame(0, 0x01, frame, sizeof(frame));
    EthSwt_GetPortStats(1, &stats);
    ASSERT_EQ(0U, stats.MirroredFrames);
}

/* ══════════════════════════════════════════════════════════════════════
 * B2: DET — uninitialised APIs report errors
 * ══════════════════════════════════════════════════════════════════════ */

/** @req SWS_EthSwt_00001 */
TEST_CASE(ethswt_det_uninit_new_apis)
{
    EthSwt_VlanConfigType vlan;
    EthSwt_FlowControlConfigType fc;
    EthSwt_MirrorConfigType mirror;
    EthSwt_PortStatsType stats;
    uint8 pcp;
    uint16 pvid;
    boolean enable;

    /* Guarantee UNINIT state, then clear mock */
    EthSwt_DeInit();
    mock_det_reset();

    (void)memset(&vlan, 0, sizeof(vlan));
    (void)memset(&fc, 0, sizeof(fc));
    (void)memset(&mirror, 0, sizeof(mirror));
    (void)memset(&stats, 0, sizeof(stats));

    ASSERT_EQ(E_NOT_OK, EthSwt_SetVlanConfig(&vlan));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetVlanConfig(1, &vlan));
    ASSERT_EQ(E_NOT_OK, EthSwt_AddVlanMember(1, 0, TRUE));
    ASSERT_EQ(E_NOT_OK, EthSwt_RemoveVlanMember(1, 0));
    ASSERT_EQ(E_NOT_OK, EthSwt_SetPvid(0, 1));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPvid(0, &pvid));
    ASSERT_EQ(E_NOT_OK, EthSwt_SetVidPcpMap(1, 3));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetVidPcpMap(1, &pcp));
    ASSERT_EQ(E_NOT_OK, EthSwt_ForwardFrameVlan(0, 1, 0x02, (const uint8*)"x", 1U));
    ASSERT_EQ(E_NOT_OK, EthSwt_SetFlowControl(0, &fc));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetFlowControl(0, &fc));
    ASSERT_EQ(E_NOT_OK, EthSwt_SetPauseTime(0, 64));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPauseTime(0, &pvid));
    ASSERT_EQ(E_NOT_OK, EthSwt_IndicatePause(0, TRUE));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetStatistics(0, &stats));
    ASSERT_EQ(E_NOT_OK, EthSwt_ResetStatistics(0));
    ASSERT_EQ(E_NOT_OK, EthSwt_SetPortMirroring(&mirror));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPortMirroring(&mirror));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetPortEnable(0, &enable));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetSpeed(0, (EthSwt_SpeedType*)&pcp, (EthSwt_DuplexType*)&enable));
    ASSERT_EQ(E_NOT_OK, EthSwt_GetMacFilter(0, (EthSwt_MacAddrType*)&enable, &enable));

    /* 21 uninit detections for the new APIs */
    ASSERT_EQ(21U, mock_det_count_for(ETHSWT_SID_SETVLANCONFIG) +
                   mock_det_count_for(ETHSWT_SID_GETVLANCONFIG) +
                   mock_det_count_for(ETHSWT_SID_ADDVLANMEMBER) +
                   mock_det_count_for(ETHSWT_SID_REMOVEVLANMEMBER) +
                   mock_det_count_for(ETHSWT_SID_SETPVID) +
                   mock_det_count_for(ETHSWT_SID_GETPVID) +
                   mock_det_count_for(ETHSWT_SID_SETVIDPCP) +
                   mock_det_count_for(ETHSWT_SID_GETVIDPCP) +
                   mock_det_count_for(ETHSWT_SID_FORWARDFRAMEVLAN) +
                   mock_det_count_for(ETHSWT_SID_SETFLOWCONTROL) +
                   mock_det_count_for(ETHSWT_SID_GETFLOWCONTROL) +
                   mock_det_count_for(ETHSWT_SID_SETPAUSETIME) +
                   mock_det_count_for(ETHSWT_SID_GETPAUSETIME) +
                   mock_det_count_for(ETHSWT_SID_INDICATEPAUSE) +
                   mock_det_count_for(ETHSWT_SID_GETSTATISTICS) +
                   mock_det_count_for(ETHSWT_SID_RESETSTATISTICS) +
                   mock_det_count_for(ETHSWT_SID_SETPORTMIRRORING) +
                   mock_det_count_for(ETHSWT_SID_GETPORTMIRRORING) +
                   mock_det_count_for(ETHSWT_SID_GETPORTENABLE) +
                   mock_det_count_for(ETHSWT_SID_GETSPEED) +
                   mock_det_count_for(ETHSWT_SID_GETMACFILTER));
}

/*==================================================================================================
*                                      TEST SUITE
*==================================================================================================*/
/* NOTE: suite setup/teardown macros unused — each case resets module state
 * via setup_default_config()/test_ethswt_reset_module() for isolation.
 * All cases are registered directly in main() below. */

/*==================================================================================================
*                                      MAIN
*==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ethswt_init_valid);
    RUN_TEST(test_ethswt_init_null);
    RUN_TEST(test_ethswt_init_twice);
    RUN_TEST(test_ethswt_deinit);
    RUN_TEST(test_ethswt_deinit_uninit);
    RUN_TEST(test_ethswt_set_port_enable);
    RUN_TEST(test_ethswt_set_port_enable_invalid);
    RUN_TEST(test_ethswt_set_speed);
    RUN_TEST(test_ethswt_set_speed_invalid_port);
    RUN_TEST(test_ethswt_get_link_state);
    RUN_TEST(test_ethswt_get_link_state_null);
    RUN_TEST(test_ethswt_get_link_state_invalid_port);
    RUN_TEST(test_ethswt_config_vlan);
    RUN_TEST(test_ethswt_config_vlan_null);
    RUN_TEST(test_ethswt_forward_frame);
    RUN_TEST(test_ethswt_forward_frame_null);
    RUN_TEST(test_ethswt_get_port_stats);
    RUN_TEST(test_ethswt_get_port_stats_null);
    RUN_TEST(test_ethswt_set_mac_filter);
    RUN_TEST(test_ethswt_set_mac_filter_null);
    RUN_TEST(test_ethswt_main_function);
    RUN_TEST(test_ethswt_main_function_uninit);
    RUN_TEST(test_ethswt_reset);
    RUN_TEST(test_ethswt_get_version_info);
    RUN_TEST(test_ethswt_forward_stats_update);

    /* B2: port queries */
    RUN_TEST(test_ethswt_get_port_enable);
    RUN_TEST(test_ethswt_get_port_enable_null);
    RUN_TEST(test_ethswt_get_port_enable_invalid);
    RUN_TEST(test_ethswt_get_speed);
    RUN_TEST(test_ethswt_get_speed_null);
    RUN_TEST(test_ethswt_get_mac_filter);
    RUN_TEST(test_ethswt_get_mac_filter_null);

    /* B2: VLAN */
    RUN_TEST(test_ethswt_vlan_set_get);
    RUN_TEST(test_ethswt_vlan_set_upsert);
    RUN_TEST(test_ethswt_vlan_set_invalid_pcp);
    RUN_TEST(test_ethswt_vlan_set_null);
    RUN_TEST(test_ethswt_vlan_get_missing);
    RUN_TEST(test_ethswt_vlan_get_null);
    RUN_TEST(test_ethswt_vlan_add_remove_member);
    RUN_TEST(test_ethswt_vlan_add_member_missing_vlan);
    RUN_TEST(test_ethswt_vlan_remove_member_missing_vlan);
    RUN_TEST(test_ethswt_vlan_add_member_invalid_port);
    RUN_TEST(test_ethswt_pvid_set_get);
    RUN_TEST(test_ethswt_pvid_invalid_port);
    RUN_TEST(test_ethswt_pvid_null);
    RUN_TEST(test_ethswt_vid_pcp_map);
    RUN_TEST(test_ethswt_vid_pcp_invalid);
    RUN_TEST(test_ethswt_vid_pcp_missing_vlan);
    RUN_TEST(test_ethswt_forward_vlan_member);
    RUN_TEST(test_ethswt_forward_vlan_ingress_filter);
    RUN_TEST(test_ethswt_forward_vlan_egress_filter);
    RUN_TEST(test_ethswt_forward_vlan_drop_untagged);

    /* B2: flow control */
    RUN_TEST(test_ethswt_flow_control_set_get);
    RUN_TEST(test_ethswt_flow_control_invalid_watermark);
    RUN_TEST(test_ethswt_flow_control_null);
    RUN_TEST(test_ethswt_flow_control_pause_emission);
    RUN_TEST(test_ethswt_flow_control_pause_time);
    RUN_TEST(test_ethswt_flow_control_rx_pause);

    /* B2: statistics */
    RUN_TEST(test_ethswt_get_statistics);
    RUN_TEST(test_ethswt_get_statistics_null);
    RUN_TEST(test_ethswt_get_statistics_invalid_port);
    RUN_TEST(test_ethswt_reset_statistics);
    RUN_TEST(test_ethswt_reset_statistics_all);
    RUN_TEST(test_ethswt_reset_statistics_invalid_port);

    /* B2: mirroring */
    RUN_TEST(test_ethswt_mirror_set_get);
    RUN_TEST(test_ethswt_mirror_set_invalid_dest);
    RUN_TEST(test_ethswt_mirror_set_null);
    RUN_TEST(test_ethswt_mirror_forward_copy);
    RUN_TEST(test_ethswt_mirror_disabled);

    /* B2: DET */
    RUN_TEST(test_ethswt_det_uninit_new_apis);

    return UNITY_END();
}

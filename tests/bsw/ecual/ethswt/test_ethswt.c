/**
 * @file test_ethswt.c
 * @brief EthSwt Unit Tests — substantiated against the production EthSwt
 *        implementation (src/bsw/ecual/ethswt/src/EthSwt.c).
 * @version 2.0.0
 * @date 2026-09-17
 */

// @tests src/bsw/ecual/ethswt/src/EthSwt.c  @tests src/bsw/ecual/ethswt/include/EthSwt.h

#include "unity.h"

/* The vendored Unity lacks 64-bit asserts; provide a compatible shim. */
#define TEST_ASSERT_EQUAL_UINT64(expected, actual) \
    TEST_ASSERT_TRUE((expected) == (actual))
#include "EthSwt.h"
#include <string.h>

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint32 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    TEST_ASSERT_EQUAL_UINT16(ETHSWT_MODULE_ID, ModuleId);
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test configuration: ports 0..2 enabled, remaining ports disabled,
   no VLANs pre-configured, no flow-control / mirror config. */
static const EthSwt_PortConfigType swt_ports[3] = {
    {0U, ETHSWT_SPEED_100MBPS, ETHSWT_DUPLEX_FULL, ETHSWT_PORT_ENABLED, {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05}}, 0U},
    {1U, ETHSWT_SPEED_100MBPS, ETHSWT_DUPLEX_FULL, ETHSWT_PORT_ENABLED, {{0x00, 0x01, 0x02, 0x03, 0x04, 0x06}}, 0U},
    {2U, ETHSWT_SPEED_100MBPS, ETHSWT_DUPLEX_FULL, ETHSWT_PORT_ENABLED, {{0x00, 0x01, 0x02, 0x03, 0x04, 0x07}}, 0U},
};

static const EthSwt_ConfigType swt_config = {
    3U,                     /* NumPorts */
    swt_ports,              /* PortConfigs */
    0U,                     /* NumVlans */
    NULL_PTR,               /* VlanConfigs */
    TRUE,                   /* DevErrorDetect */
    TRUE,                   /* VersionInfoApi */
    NULL_PTR,               /* FlowControlConfigs */
    NULL_PTR                /* MirrorConfig */
};

static const uint8 frame_data[64] = {0xDE, 0xAD, 0xBE, 0xEF};

static boolean swt_initialized = FALSE;

static void swt_init_default(void) {
    EthSwt_Init(&swt_config);
    swt_initialized = TRUE;
}

static EthSwt_PortStatsType swt_get_stats(EthSwt_PortIdType port) {
    EthSwt_PortStatsType stats;
    (void)memset(&stats, 0, sizeof(stats));
    (void)EthSwt_GetPortStats(port, &stats);
    return stats;
}

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
    if (swt_initialized == TRUE) {
        EthSwt_DeInit();
        swt_initialized = FALSE;
    }
}

/** @req SWS_EthSwt_00001 */
void test_EthSwt_Init_NullPtr_ShouldReportError(void) {
    EthSwt_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00001 */
void test_EthSwt_Init_DoubleInit_ShouldReportError(void) {
    swt_init_default();
    EthSwt_Init(&swt_config);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00001 */
void test_EthSwt_Init_ValidConfig_ShouldApplyPortConfig(void) {
    EthSwt_PortEnableType enable = ETHSWT_PORT_DISABLED;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetPortEnable(0U, &enable));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_PORT_ENABLED, enable);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetPortEnable(3U, &enable));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_PORT_DISABLED, enable);
}

/** @req SWS_EthSwt_00002 */
void test_EthSwt_DeInit_Uninit_ShouldReportError(void) {
    EthSwt_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00002 */
void test_EthSwt_DeInit_AfterInit_ShouldBlockFurtherCalls(void) {
    EthSwt_PortEnableType enable = ETHSWT_PORT_ENABLED;
    swt_init_default();
    EthSwt_DeInit();
    swt_initialized = FALSE;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_GetPortEnable(0U, &enable));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_GETPORTENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00003 */
void test_EthSwt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    swt_init_default();
    EthSwt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00003 */
void test_EthSwt_GetVersionInfo_ValidPtr_ShouldReturnIds(void) {
    Std_VersionInfoType vi = {0U, 0U, 0U, 0U, 0U};
    swt_init_default();
    EthSwt_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(ETHSWT_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(ETHSWT_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_EthSwt_00004 */
void test_EthSwt_SetPortEnable_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetPortEnable(0U, ETHSWT_PORT_ENABLED));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_SETPORTENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00004 */
void test_EthSwt_SetPortEnable_InvalidPort_ShouldReportError(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetPortEnable(ETHSWT_MAX_PORTS, ETHSWT_PORT_ENABLED));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_SETPORTENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_INVALID_PORT, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00004 */
void test_EthSwt_SetPortEnable_Disable_ShouldClearLinkState(void) {
    EthSwt_LinkStateType link = ETHSWT_LINK_DOWN;
    uint8 i;
    swt_init_default();
    /* Drive link up via periodic MainFunction polling (100 ms / 10 ms = 10 ticks) */
    for (i = 0U; i < 10U; i++) {
        EthSwt_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetLinkState(0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_LINK_UP, link);
    /* Disabling the port must drop the link */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPortEnable(0U, ETHSWT_PORT_DISABLED));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetLinkState(0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_LINK_DOWN, link);
}

/** @req SWS_EthSwt_00005 */
void test_EthSwt_GetPortEnable_NullPtr_ShouldReportError(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_GetPortEnable(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_GETPORTENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00006 */
void test_EthSwt_SetSpeed_DisabledPort_ShouldFailWithoutDet(void) {
    swt_init_default();
    /* Port 3 is not in the config, hence disabled after init */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetSpeed(3U, ETHSWT_SPEED_1000MBPS, ETHSWT_DUPLEX_HALF));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_EthSwt_00006 */
void test_EthSwt_SetSpeed_EnabledPort_ShouldRoundtrip(void) {
    EthSwt_SpeedType speed = ETHSWT_SPEED_AUTO;
    EthSwt_DuplexType duplex = ETHSWT_DUPLEX_FULL;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetSpeed(0U, ETHSWT_SPEED_1000MBPS, ETHSWT_DUPLEX_HALF));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetSpeed(0U, &speed, &duplex));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SPEED_1000MBPS, speed);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_DUPLEX_HALF, duplex);
}

/** @req SWS_EthSwt_00009 */
void test_EthSwt_ConfigVlan_NullPtr_ShouldReportError(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_ConfigVlan(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_CONFIGVLAN, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00009 */
void test_EthSwt_ConfigVlan_InvalidPcp_ShouldBeSanitized(void) {
    EthSwt_VlanConfigType vlan = {10U, 0x03U, FALSE, 9U, FALSE};
    EthSwt_VlanConfigType readback = {0U, 0U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ConfigVlan(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetVlanConfig(10U, &readback));
    TEST_ASSERT_EQUAL_UINT16(10U, readback.VlanId);
    TEST_ASSERT_EQUAL_UINT8(0x03U, readback.PortMask);
    TEST_ASSERT_EQUAL_UINT8(0U, readback.VlanPriority);   /* 9 > 7 sanitized to 0 */
}

/** @req SWS_EthSwt_00009 */
void test_EthSwt_ConfigVlan_TableFull_ShouldFail(void) {
    EthSwt_VlanConfigType vlan = {0U, 0x01U, FALSE, 0U, FALSE};
    uint8 i;
    swt_init_default();
    for (i = 0U; i < ETHSWT_MAX_VLANS; i++) {
        vlan.VlanId = (uint16)(100U + i);
        TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ConfigVlan(&vlan));
    }
    vlan.VlanId = 200U;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_ConfigVlan(&vlan));
}

/** @req SWS_EthSwt_00010 */
void test_EthSwt_SetVlanConfig_InvalidPcp_ShouldReportError(void) {
    EthSwt_VlanConfigType vlan = {20U, 0x01U, FALSE, 8U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_SETVLANCONFIG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_INVALID_PCP, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00010 */
void test_EthSwt_SetVlanConfig_SameVlanId_ShouldUpsert(void) {
    EthSwt_VlanConfigType vlan = {21U, 0x01U, FALSE, 3U, FALSE};
    EthSwt_VlanConfigType readback = {0U, 0U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    vlan.VlanPriority = 5U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetVlanConfig(21U, &readback));
    TEST_ASSERT_EQUAL_UINT8(5U, readback.VlanPriority);
}

/** @req SWS_EthSwt_00011 */
void test_EthSwt_GetVlanConfig_NotFound_ShouldFailSilently(void) {
    EthSwt_VlanConfigType readback = {0U, 0U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_GetVlanConfig(999U, &readback));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_EthSwt_00012 */
void test_EthSwt_AddVlanMember_InvalidPort_ShouldReportError(void) {
    EthSwt_VlanConfigType vlan = {22U, 0x00U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_AddVlanMember(22U, ETHSWT_MAX_PORTS, TRUE));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_ADDVLANMEMBER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_INVALID_PORT, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00012 */
void test_EthSwt_AddVlanMember_ValidCall_ShouldSetMaskBit(void) {
    EthSwt_VlanConfigType vlan = {23U, 0x00U, FALSE, 0U, FALSE};
    EthSwt_VlanConfigType readback = {0U, 0U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_AddVlanMember(23U, 1U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetVlanConfig(23U, &readback));
    TEST_ASSERT_EQUAL_UINT8(0x02U, readback.PortMask);
    TEST_ASSERT_EQUAL_UINT8(TRUE, readback.Tagged);
}

/** @req SWS_EthSwt_00013 */
void test_EthSwt_RemoveVlanMember_ShouldClearMaskBit(void) {
    EthSwt_VlanConfigType vlan = {24U, 0x06U, FALSE, 0U, FALSE};
    EthSwt_VlanConfigType readback = {0U, 0U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_RemoveVlanMember(24U, 1U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetVlanConfig(24U, &readback));
    TEST_ASSERT_EQUAL_UINT8(0x04U, readback.PortMask);
}

/** @req SWS_EthSwt_00014 */
void test_EthSwt_SetPvid_GetPvid_ShouldRoundtrip(void) {
    uint16 pvid = 0xFFFFU;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPvid(0U, 42U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetPvid(0U, &pvid));
    TEST_ASSERT_EQUAL_UINT16(42U, pvid);
}

/** @req SWS_EthSwt_00015 */
void test_EthSwt_GetPvid_NullPtr_ShouldReportError(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_GetPvid(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_GETPVID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00016 */
void test_EthSwt_SetVidPcpMap_InvalidPcp_ShouldReportError(void) {
    EthSwt_VlanConfigType vlan = {25U, 0x01U, FALSE, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetVidPcpMap(25U, 8U));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_SETVIDPCP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_INVALID_PCP, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00016 */
void test_EthSwt_SetVidPcpMap_UnknownVlan_ShouldFailSilently(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetVidPcpMap(999U, 3U));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_EthSwt_00017 */
void test_EthSwt_GetVidPcpMap_ShouldReturnMappedPcp(void) {
    EthSwt_VlanConfigType vlan = {26U, 0x01U, FALSE, 2U, FALSE};
    uint8 pcp = 0xFFU;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVidPcpMap(26U, 6U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetVidPcpMap(26U, &pcp));
    TEST_ASSERT_EQUAL_UINT8(6U, pcp);
}

/** @req SWS_EthSwt_00018 */
void test_EthSwt_ForwardFrame_NullPtr_ShouldReportError(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_ForwardFrame(0U, 0x02U, NULL_PTR, 64U));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_FORWARDFRAME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00018 */
void test_EthSwt_ForwardFrame_BasicForwarding_ShouldAccountStats(void) {
    EthSwt_PortStatsType s0;
    EthSwt_PortStatsType s1;
    EthSwt_PortStatsType s2;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x06U, frame_data, 100U));
    s0 = swt_get_stats(0U);
    s1 = swt_get_stats(1U);
    s2 = swt_get_stats(2U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.TxFrames);
    TEST_ASSERT_EQUAL_UINT64(100U, s0.TxBytes);
    TEST_ASSERT_EQUAL_UINT64(1U, s1.RxFrames);
    TEST_ASSERT_EQUAL_UINT64(100U, s1.RxBytes);
    TEST_ASSERT_EQUAL_UINT64(1U, s2.RxFrames);
}

/** @req SWS_EthSwt_00018 */
void test_EthSwt_ForwardFrame_DisabledDst_ShouldCountDrops(void) {
    EthSwt_PortStatsType s0;
    EthSwt_PortStatsType s3;
    swt_init_default();
    /* Port 3 is disabled: mask 0x08 selects it */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x08U, frame_data, 100U));
    s0 = swt_get_stats(0U);
    s3 = swt_get_stats(3U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.DroppedFrames);
    TEST_ASSERT_EQUAL_UINT64(1U, s3.DroppedFrames);
    TEST_ASSERT_EQUAL_UINT64(0U, s3.RxFrames);
}

/** @req SWS_EthSwt_00019 */
void test_EthSwt_ForwardFrame_IngressFilter_ShouldDropNonMemberSrc(void) {
    EthSwt_VlanConfigType vlan = {30U, 0x02U, FALSE, 0U, FALSE};  /* members: port 1 only */
    EthSwt_PortStatsType s0;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPvid(0U, 30U));
    /* Untagged frame on port 0: effective VLAN = PVID 30; port 0 not a member */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x02U, frame_data, 100U));
    s0 = swt_get_stats(0U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.RxFilteredFrames);
    TEST_ASSERT_EQUAL_UINT64(0U, s0.TxFrames);   /* frame consumed, not forwarded */
}

/** @req SWS_EthSwt_00019 */
void test_EthSwt_ForwardFrame_DropUntagged_ShouldDropUntaggedOnly(void) {
    EthSwt_VlanConfigType vlan = {40U, 0x03U, FALSE, 0U, TRUE};   /* members: 0,1; drop untagged */
    EthSwt_PortStatsType s0;
    EthSwt_PortStatsType s1;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPvid(0U, 40U));
    /* Untagged frame must be dropped */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x02U, frame_data, 100U));
    s0 = swt_get_stats(0U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.RxFilteredFrames);
    /* Tagged frame (VlanId 40) must pass */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrameVlan(0U, 40U, 0x02U, frame_data, 100U));
    s1 = swt_get_stats(1U);
    TEST_ASSERT_EQUAL_UINT64(1U, s1.RxFrames);
    TEST_ASSERT_EQUAL_UINT64(1U, s1.RxVlanFrames);
}

/** @req SWS_EthSwt_00019 */
void test_EthSwt_ForwardFrame_EgressFilter_ShouldCountTxFiltered(void) {
    EthSwt_VlanConfigType vlan = {50U, 0x01U, FALSE, 0U, FALSE};  /* members: port 0 only */
    EthSwt_PortStatsType s0;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    /* Tagged frame from port 0 (member) to ports 1,2 (non-members) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrameVlan(0U, 50U, 0x06U, frame_data, 100U));
    s0 = swt_get_stats(0U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.TxFrames);
    TEST_ASSERT_EQUAL_UINT64(2U, s0.TxFilteredFrames);
}

/** @req SWS_EthSwt_00019 */
void test_EthSwt_ForwardFrame_Tagged_ShouldCountVlanFrames(void) {
    EthSwt_VlanConfigType vlan = {60U, 0x03U, TRUE, 0U, FALSE};   /* members: 0,1 */
    EthSwt_PortStatsType s0;
    EthSwt_PortStatsType s1;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetVlanConfig(&vlan));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrameVlan(0U, 60U, 0x02U, frame_data, 64U));
    s0 = swt_get_stats(0U);
    s1 = swt_get_stats(1U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.TxVlanFrames);
    TEST_ASSERT_EQUAL_UINT64(1U, s1.RxVlanFrames);
    TEST_ASSERT_EQUAL_UINT64(64U, s1.RxBytes);
}

/** @req SWS_EthSwt_00020 */
void test_EthSwt_GetPortStats_NullPtr_ShouldReportError(void) {
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_GetPortStats(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_GETPORTSTATS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00022 */
void test_EthSwt_ResetStatistics_AllPorts_ShouldZeroStats(void) {
    EthSwt_PortStatsType s0;
    EthSwt_PortStatsType s1;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x02U, frame_data, 100U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ResetStatistics(ETHSWT_ALL_PORTS));
    s0 = swt_get_stats(0U);
    s1 = swt_get_stats(1U);
    TEST_ASSERT_EQUAL_UINT64(0U, s0.TxFrames);
    TEST_ASSERT_EQUAL_UINT64(0U, s0.TxBytes);
    TEST_ASSERT_EQUAL_UINT64(0U, s1.RxFrames);
    TEST_ASSERT_EQUAL_UINT64(0U, s1.RxBytes);
}

/** @req SWS_EthSwt_00023 */
void test_EthSwt_SetMacFilter_GetMacFilter_ShouldRoundtrip(void) {
    EthSwt_MacAddrType mac = {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
    EthSwt_MacAddrType readback = {{0}};
    boolean enabled = FALSE;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetMacFilter(0U, &mac, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetMacFilter(0U, &readback, &enabled));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.octet, readback.octet, 6U);
    TEST_ASSERT_EQUAL_UINT8(TRUE, enabled);
}

/** @req SWS_EthSwt_00025 */
void test_EthSwt_SetFlowControl_InvalidWatermark_ShouldReportError(void) {
    EthSwt_FlowControlConfigType fc = {TRUE, FALSE, 5U, 5U, 512U};  /* high == low */
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetFlowControl(0U, &fc));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_SETFLOWCONTROL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_INVALID_WATERMARK, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00025 */
void test_EthSwt_SetFlowControl_GetFlowControl_ShouldRoundtrip(void) {
    EthSwt_FlowControlConfigType fc = {TRUE, TRUE, 20U, 4U, 99U};
    EthSwt_FlowControlConfigType readback = {FALSE, FALSE, 0U, 0U, 0U};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetFlowControl(1U, &fc));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetFlowControl(1U, &readback));
    TEST_ASSERT_EQUAL_UINT8(TRUE, readback.TxPauseEnable);
    TEST_ASSERT_EQUAL_UINT8(TRUE, readback.RxPauseEnable);
    TEST_ASSERT_EQUAL_UINT16(20U, readback.HighWatermark);
    TEST_ASSERT_EQUAL_UINT16(4U, readback.LowWatermark);
    TEST_ASSERT_EQUAL_UINT16(99U, readback.PauseTime);
}

/** @req SWS_EthSwt_00025 */
void test_EthSwt_ForwardFrame_TxPause_ShouldTriggerOnce(void) {
    EthSwt_FlowControlConfigType fc = {TRUE, FALSE, 2U, 1U, 512U};
    EthSwt_PortStatsType s0;
    uint8 i;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetFlowControl(0U, &fc));
    for (i = 0U; i < 3U; i++) {
        TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x00U, frame_data, 10U));
    }
    s0 = swt_get_stats(0U);
    TEST_ASSERT_EQUAL_UINT64(1U, s0.TxPauseFrames);   /* pause asserted once at high watermark */
}

/** @req SWS_EthSwt_00029 */
void test_EthSwt_IndicatePause_ShouldCountOncePerPause(void) {
    EthSwt_FlowControlConfigType fc = {FALSE, TRUE, 32U, 8U, 512U};
    EthSwt_PortStatsType s1;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetFlowControl(1U, &fc));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_IndicatePause(1U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_IndicatePause(1U, TRUE));
    s1 = swt_get_stats(1U);
    TEST_ASSERT_EQUAL_UINT64(1U, s1.RxPauseFrames);   /* counted once while pause held */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_IndicatePause(1U, FALSE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_IndicatePause(1U, TRUE));
    s1 = swt_get_stats(1U);
    TEST_ASSERT_EQUAL_UINT64(2U, s1.RxPauseFrames);   /* re-counted after release */
}

/** @req SWS_EthSwt_00029 */
void test_EthSwt_IndicatePause_RxPauseDisabled_ShouldNotCount(void) {
    EthSwt_PortStatsType s0;
    swt_init_default();
    /* Default config: RxPauseEnable FALSE */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_IndicatePause(0U, TRUE));
    s0 = swt_get_stats(0U);
    TEST_ASSERT_EQUAL_UINT64(0U, s0.RxPauseFrames);
}

/** @req SWS_EthSwt_00030 */
void test_EthSwt_SetPortMirroring_InvalidDst_ShouldReportError(void) {
    EthSwt_MirrorConfigType mirror = {0x01U, ETHSWT_MAX_PORTS, TRUE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthSwt_SetPortMirroring(&mirror));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_SID_SETPORTMIRRORING, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_E_MIRROR_INVALID, mock_DetLastErrorId);
}

/** @req SWS_EthSwt_00030 */
void test_EthSwt_SetPortMirroring_ValidConfig_ShouldCopyFrames(void) {
    EthSwt_MirrorConfigType mirror = {0x01U, 2U, TRUE};   /* mirror port 0 traffic to port 2 */
    EthSwt_PortStatsType s2;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPortMirroring(&mirror));
    /* No regular egress (mask 0); mirrored copy must still reach port 2 */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_ForwardFrame(0U, 0x00U, frame_data, 100U));
    s2 = swt_get_stats(2U);
    TEST_ASSERT_EQUAL_UINT64(1U, s2.RxFrames);
    TEST_ASSERT_EQUAL_UINT64(100U, s2.RxBytes);
    TEST_ASSERT_EQUAL_UINT64(1U, s2.MirroredFrames);
}

/** @req SWS_EthSwt_00031 */
void test_EthSwt_GetPortMirroring_ShouldRoundtrip(void) {
    EthSwt_MirrorConfigType mirror = {0x05U, 1U, TRUE};
    EthSwt_MirrorConfigType readback = {0U, 0U, FALSE};
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPortMirroring(&mirror));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetPortMirroring(&readback));
    TEST_ASSERT_EQUAL_UINT8(0x05U, readback.MirrorSourcePortMask);
    TEST_ASSERT_EQUAL_UINT8(1U, readback.MirrorDestinationPort);
    TEST_ASSERT_EQUAL_UINT8(TRUE, readback.MirrorEnabled);
}

/** @req SWS_EthSwt_00032 */
void test_EthSwt_MainFunction_EnabledPort_ShouldReportLinkUp(void) {
    EthSwt_LinkStateType link0 = ETHSWT_LINK_DOWN;
    EthSwt_LinkStateType link3 = ETHSWT_LINK_UP;
    uint8 i;
    swt_init_default();
    /* Link poll period = LINK_POLL_INTERVAL_MS / MAIN_FUNCTION_PERIOD_MS = 10 ticks */
    for (i = 0U; i < 9U; i++) {
        EthSwt_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetLinkState(0U, &link0));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_LINK_DOWN, link0);   /* not yet polled */
    EthSwt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetLinkState(0U, &link0));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_LINK_UP, link0);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetLinkState(3U, &link3));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_LINK_DOWN, link3);   /* disabled port stays down */
}

/** @req SWS_EthSwt_00033 */
void test_EthSwt_Reset_ShouldRestoreConfiguredState(void) {
    EthSwt_PortEnableType enable = ETHSWT_PORT_DISABLED;
    swt_init_default();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_SetPortEnable(0U, ETHSWT_PORT_DISABLED));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetPortEnable(0U, &enable));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_PORT_DISABLED, enable);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_Reset());
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSwt_GetPortEnable(0U, &enable));
    TEST_ASSERT_EQUAL_UINT8(ETHSWT_PORT_ENABLED, enable);   /* restored from config */
}

/* ---------------------------------------------------------------------------
 * Runner
 * ------------------------------------------------------------------------- */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_EthSwt_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_EthSwt_Init_ValidConfig_ShouldApplyPortConfig);
    RUN_TEST(test_EthSwt_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_EthSwt_DeInit_AfterInit_ShouldBlockFurtherCalls);
    RUN_TEST(test_EthSwt_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_GetVersionInfo_ValidPtr_ShouldReturnIds);
    RUN_TEST(test_EthSwt_SetPortEnable_Uninit_ShouldReportError);
    RUN_TEST(test_EthSwt_SetPortEnable_InvalidPort_ShouldReportError);
    RUN_TEST(test_EthSwt_SetPortEnable_Disable_ShouldClearLinkState);
    RUN_TEST(test_EthSwt_GetPortEnable_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_SetSpeed_DisabledPort_ShouldFailWithoutDet);
    RUN_TEST(test_EthSwt_SetSpeed_EnabledPort_ShouldRoundtrip);
    RUN_TEST(test_EthSwt_ConfigVlan_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_ConfigVlan_InvalidPcp_ShouldBeSanitized);
    RUN_TEST(test_EthSwt_ConfigVlan_TableFull_ShouldFail);
    RUN_TEST(test_EthSwt_SetVlanConfig_InvalidPcp_ShouldReportError);
    RUN_TEST(test_EthSwt_SetVlanConfig_SameVlanId_ShouldUpsert);
    RUN_TEST(test_EthSwt_GetVlanConfig_NotFound_ShouldFailSilently);
    RUN_TEST(test_EthSwt_AddVlanMember_InvalidPort_ShouldReportError);
    RUN_TEST(test_EthSwt_AddVlanMember_ValidCall_ShouldSetMaskBit);
    RUN_TEST(test_EthSwt_RemoveVlanMember_ShouldClearMaskBit);
    RUN_TEST(test_EthSwt_SetPvid_GetPvid_ShouldRoundtrip);
    RUN_TEST(test_EthSwt_GetPvid_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_SetVidPcpMap_InvalidPcp_ShouldReportError);
    RUN_TEST(test_EthSwt_SetVidPcpMap_UnknownVlan_ShouldFailSilently);
    RUN_TEST(test_EthSwt_GetVidPcpMap_ShouldReturnMappedPcp);
    RUN_TEST(test_EthSwt_ForwardFrame_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_ForwardFrame_BasicForwarding_ShouldAccountStats);
    RUN_TEST(test_EthSwt_ForwardFrame_DisabledDst_ShouldCountDrops);
    RUN_TEST(test_EthSwt_ForwardFrame_IngressFilter_ShouldDropNonMemberSrc);
    RUN_TEST(test_EthSwt_ForwardFrame_DropUntagged_ShouldDropUntaggedOnly);
    RUN_TEST(test_EthSwt_ForwardFrame_EgressFilter_ShouldCountTxFiltered);
    RUN_TEST(test_EthSwt_ForwardFrame_Tagged_ShouldCountVlanFrames);
    RUN_TEST(test_EthSwt_GetPortStats_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSwt_ResetStatistics_AllPorts_ShouldZeroStats);
    RUN_TEST(test_EthSwt_SetMacFilter_GetMacFilter_ShouldRoundtrip);
    RUN_TEST(test_EthSwt_SetFlowControl_InvalidWatermark_ShouldReportError);
    RUN_TEST(test_EthSwt_SetFlowControl_GetFlowControl_ShouldRoundtrip);
    RUN_TEST(test_EthSwt_ForwardFrame_TxPause_ShouldTriggerOnce);
    RUN_TEST(test_EthSwt_IndicatePause_ShouldCountOncePerPause);
    RUN_TEST(test_EthSwt_IndicatePause_RxPauseDisabled_ShouldNotCount);
    RUN_TEST(test_EthSwt_SetPortMirroring_InvalidDst_ShouldReportError);
    RUN_TEST(test_EthSwt_SetPortMirroring_ValidConfig_ShouldCopyFrames);
    RUN_TEST(test_EthSwt_GetPortMirroring_ShouldRoundtrip);
    RUN_TEST(test_EthSwt_MainFunction_EnabledPort_ShouldReportLinkUp);
    RUN_TEST(test_EthSwt_Reset_ShouldRestoreConfiguredState);

    return UNITY_END();
}

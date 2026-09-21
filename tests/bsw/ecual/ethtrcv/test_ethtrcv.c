/**
 * @file test_ethtrcv.c
 * @brief EthTrcv Unit Tests — substantiated against the production EthTrcv
 *        driver (src/bsw/ecual/ethtrcv/src/EthTrcv.c).
 * @version 2.0.0
 * @date 2026-09-17
 */

// @tests src/bsw/ecual/ethtrcv/src/EthTrcv.c  @tests src/bsw/ecual/ethtrcv/include/EthTrcv.h

#include "unity.h"
#include "EthTrcv.h"
#include "Eth.h"
#include <string.h>

/* ---------------------------------------------------------------------------
 * Det mock
 * ------------------------------------------------------------------------- */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint32 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_MODULE_ID, ModuleId);
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---------------------------------------------------------------------------
 * Eth MII stubs: a per-transceiver PHY register bank with fault injection.
 * Trcv0 PHY address 0, Trcv1 PHY address 1 (see EthTrcv_TrcvConfig below).
 * ------------------------------------------------------------------------- */
static uint16 mock_phy_regs[ETHTRCV_NUMBER_OF_TRCVS][32];
static uint8 mock_mii_read_fail_trcv = 0xFFU;
static uint8 mock_mii_write_fail_trcv = 0xFFU;
static uint8 mock_mii_last_write_reg[ETHTRCV_NUMBER_OF_TRCVS];
static uint16 mock_mii_last_write_val[ETHTRCV_NUMBER_OF_TRCVS];
static uint32 mock_mii_write_count[ETHTRCV_NUMBER_OF_TRCVS];

static uint8 mock_mii_trcv_from_phy(uint8 PhyAddr) {
    return (PhyAddr < ETHTRCV_NUMBER_OF_TRCVS) ? PhyAddr : 0U;
}

Std_ReturnType Eth_ReadMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr,
                           Eth_RegAddrType RegAddr, Eth_DataType* DataPtr) {
    uint8 trcv = mock_mii_trcv_from_phy(PhyAddr);
    (void)CtrlIdx;
    if (mock_mii_read_fail_trcv == trcv) {
        return E_NOT_OK;
    }
    if ((DataPtr == NULL_PTR) || (RegAddr >= 32U)) {
        return E_NOT_OK;
    }
    *DataPtr = mock_phy_regs[trcv][RegAddr];
    return E_OK;
}

Std_ReturnType Eth_WriteMii(Eth_ControllerType CtrlIdx, Eth_PhyAddrType PhyAddr,
                            Eth_RegAddrType RegAddr, Eth_DataType Data) {
    uint8 trcv = mock_mii_trcv_from_phy(PhyAddr);
    (void)CtrlIdx;
    if (mock_mii_write_fail_trcv == trcv) {
        return E_NOT_OK;
    }
    if (RegAddr >= 32U) {
        return E_NOT_OK;
    }
    /* The TJA1100 cable-test bit self-clears when the test completes. */
    if ((RegAddr == ETHTRCV_TJA1100_REG_EXTENDED_CTRL)
        && ((Data & ETHTRCV_TJA1100_EXT_CTRL_CABLE_TEST) != 0U)) {
        Data = (uint16)(Data & (uint16)(~ETHTRCV_TJA1100_EXT_CTRL_CABLE_TEST));
    }
    /* A real PHY completes the reset internally and clears BMCR_RESET; without
     * this the production init's do-while reset poll never terminates. */
    if ((RegAddr == ETHTRCV_PHY_REG_BMCR) && ((Data & ETHTRCV_BMCR_RESET) != 0U)) {
        Data = (uint16)(Data & (uint16)(~ETHTRCV_BMCR_RESET));
    }
    mock_phy_regs[trcv][RegAddr] = Data;
    mock_mii_last_write_reg[trcv] = RegAddr;
    mock_mii_last_write_val[trcv] = Data;
    mock_mii_write_count[trcv]++;
    return E_OK;
}

/* ---------------------------------------------------------------------------
 * Callback recorders
 * ------------------------------------------------------------------------- */
static uint32 mock_LinkChg_Count = 0U;
static uint8 mock_LinkChg_LastCtrl = 0xFFU;
static uint8 mock_LinkChg_LastState = 0xFFU;

static uint32 mock_Wakeup_Count = 0U;
static uint8 mock_Wakeup_LastTrcv = 0xFFU;

static void test_LinkStateChgCb(uint8 CtrlIdx, EthTrcv_LinkStateType LinkState) {
    mock_LinkChg_Count++;
    mock_LinkChg_LastCtrl = CtrlIdx;
    mock_LinkChg_LastState = LinkState;
}

static void test_WakeupIndCb(uint8 TrcvIdx) {
    mock_Wakeup_Count++;
    mock_Wakeup_LastTrcv = TrcvIdx;
}

/* ---------------------------------------------------------------------------
 * Module configuration symbols normally provided by EthTrcv_Lcfg.c
 * ------------------------------------------------------------------------- */
const EthTrcv_TrcvConfigType EthTrcv_TrcvConfig[ETHTRCV_NUMBER_OF_TRCVS] = {
    {   /* Trcv 0: TJA1100, PHY addr 0, MII access */
        0U,                              /* TrcvIdx */
        0U,                              /* CtrlIdx */
        0U,                              /* PhyAddress */
        ETHTRCV_TYPE_TJA1100,            /* TrcvType */
        ETHTRCV_TYPE_TJA1100,            /* DetectedType */
        ETHTRCV_INTERFACE_RMII,          /* InterfaceType */
        ETHTRCV_ACCESS_MII,              /* AccessInterface */
        ETHTRCV_MODE_ACTIVE,             /* DefaultMode */
        TRUE,                            /* AutoNegotiationEnable */
        ETHTRCV_BAUD_RATE_100MBIT,       /* FixedSpeed */
        ETHTRCV_DUPLEX_MODE_FULL,        /* FixedDuplexMode */
        TRUE,                            /* WakeupSupport */
        2U,                              /* WakeupMode */
        ECUM_WKSOURCE_ETH,               /* WakeupSource */
        TRUE,                            /* CableDiagnosticsSupport */
        TRUE,                            /* SignalQualitySupport */
        0U,                              /* ResetDelayUs */
        0U,                              /* LinkUpDelayMs */
        NULL_PTR                         /* VendorSpecificConfig */
    },
    {   /* Trcv 1: RTL8211, PHY addr 1, MII access */
        1U,
        0U,
        1U,
        ETHTRCV_TYPE_RTL8211,
        ETHTRCV_TYPE_RTL8211,
        ETHTRCV_INTERFACE_RGMII,
        ETHTRCV_ACCESS_MII,
        ETHTRCV_MODE_ACTIVE,
        TRUE,
        ETHTRCV_BAUD_RATE_1000MBIT,
        ETHTRCV_DUPLEX_MODE_FULL,
        FALSE,
        0U,
        0U,
        FALSE,
        FALSE,
        0U,
        0U,
        NULL_PTR
    }
};

const EthTrcv_ConfigType EthTrcv_Config = {
    ETHTRCV_NUMBER_OF_TRCVS,
    EthTrcv_TrcvConfig,
    NULL_PTR,
    ETHTRCV_MAIN_FUNCTION_PERIOD_MS,
    ETHTRCV_PHY_ACCESS_TIMEOUT_MS,
    ETHTRCV_LINK_DEBOUNCE_COUNT,
    TRUE,
    TRUE,
    TRUE,
    TRUE,
    TRUE
};

const EthTrcv_LinkStateChgCbkType EthTrcv_LinkStateChgCallback = test_LinkStateChgCb;
const EthTrcv_WakeupIndicationCbkType EthTrcv_WakeupIndicationCallback = test_WakeupIndCb;

/* ---------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */
static boolean trcv_initialized = FALSE;

static void trcv_init(void) {
    EthTrcv_Init(NULL_PTR);
    trcv_initialized = TRUE;
}

void setUp(void) {
    uint8 t;
    mock_Det_Reset();
    memset(mock_phy_regs, 0, sizeof(mock_phy_regs));
    for (t = 0U; t < ETHTRCV_NUMBER_OF_TRCVS; t++) {
        mock_mii_last_write_reg[t] = 0xFFU;
        mock_mii_last_write_val[t] = 0U;
        mock_mii_write_count[t] = 0U;
    }
    mock_mii_read_fail_trcv = 0xFFU;
    mock_mii_write_fail_trcv = 0xFFU;
    mock_LinkChg_Count = 0U;
    mock_LinkChg_LastCtrl = 0xFFU;
    mock_LinkChg_LastState = 0xFFU;
    mock_Wakeup_Count = 0U;
    mock_Wakeup_LastTrcv = 0xFFU;
    /* PHY ID defaults: Trcv0 -> NXP TJA1100, Trcv1 -> Realtek RTL8211 */
    mock_phy_regs[0][ETHTRCV_PHY_REG_PHYIDR1] = 7U;          /* OUI 0x1C1 >> ... */
    mock_phy_regs[0][ETHTRCV_PHY_REG_PHYIDR2] = 0x0440U;     /* model 4 (TJA1100) */
    mock_phy_regs[1][ETHTRCV_PHY_REG_PHYIDR1] = 0x42U;       /* OUI 0x10A0 */
    mock_phy_regs[1][ETHTRCV_PHY_REG_PHYIDR2] = 0x8000U;     /* model 0 (RTL8211) */
}

void tearDown(void) {
    if (trcv_initialized == TRUE) {
        EthTrcv_DeInit();
        trcv_initialized = FALSE;
    }
}

/** @req SWS_EthTrcv_00001 */
void test_EthTrcv_Init_NullConfig_ShouldUseDefaultConfig(void) {
    EthTrcv_ModeType mode = ETHTRCV_MODE_DOWN;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetTransceiverMode(0U, 0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_MODE_ACTIVE, mode);   /* DefaultMode applied */
}

/** @req SWS_EthTrcv_00001 */
void test_EthTrcv_Init_DoubleInit_ShouldReportError(void) {
    trcv_init();
    EthTrcv_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00002 */
void test_EthTrcv_DeInit_Uninit_ShouldReportError(void) {
    EthTrcv_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00002 */
void test_EthTrcv_DeInit_AfterInit_ShouldBlockFurtherCalls(void) {
    EthTrcv_LinkStateType link = ETHTRCV_LINK_STATE_ACTIVE;
    trcv_init();
    EthTrcv_DeInit();
    trcv_initialized = FALSE;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetLinkState(0U, 0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_LINK_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00003 */
void test_EthTrcv_GetVersionInfo_NullPtr_ShouldReportError(void) {
    trcv_init();
    EthTrcv_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_VERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00003 */
void test_EthTrcv_GetVersionInfo_ValidPtr_ShouldReturnIds(void) {
    Std_VersionInfoType vi = {0U, 0U, 0U, 0U, 0U};
    trcv_init();
    EthTrcv_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_SetTransceiverMode_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_SetTransceiverMode(0U, 0U, ETHTRCV_MODE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_SET_TRANSCEIVER_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_SetTransceiverMode_InvalidTrcvIdx_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_SetTransceiverMode(ETHTRCV_NUMBER_OF_TRCVS, 0U, ETHTRCV_MODE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_SET_TRANSCEIVER_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_TRCV_IDX, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_SetTransceiverMode_InvalidMode_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_SetTransceiverMode(0U, 0U, 0x55U));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_SET_TRANSCEIVER_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_TRCV_MODE, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_SetTransceiverMode_TJA1100_ModeMap_ShouldWriteExtCtrl(void) {
    EthTrcv_ModeType mode = ETHTRCV_MODE_DOWN;
    trcv_init();
    /* STANDBY */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_SetTransceiverMode(0U, 0U, ETHTRCV_MODE_STANDBY));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_TJA1100_REG_EXTENDED_CTRL, mock_mii_last_write_reg[0]);
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_TJA1100_EXT_CTRL_PWR_STANDBY, mock_mii_last_write_val[0]);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetTransceiverMode(0U, 0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_MODE_STANDBY, mode);
    /* SLEEP */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_SetTransceiverMode(0U, 0U, ETHTRCV_MODE_SLEEP));
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_TJA1100_EXT_CTRL_PWR_SLEEP, mock_mii_last_write_val[0]);
    /* DOWN */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_SetTransceiverMode(0U, 0U, ETHTRCV_MODE_DOWN));
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_TJA1100_EXT_CTRL_PWR_DISABLE, mock_mii_last_write_val[0]);
    /* ACTIVE */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_SetTransceiverMode(0U, 0U, ETHTRCV_MODE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_TJA1100_EXT_CTRL_PWR_NORMAL, mock_mii_last_write_val[0]);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_SetTransceiverMode_RTL8211_StandbyUnsupported_ShouldFail(void) {
    EthTrcv_ModeType mode = ETHTRCV_MODE_DOWN;
    trcv_init();
    /* RTL8211 supports only DOWN / ACTIVE */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_SetTransceiverMode(1U, 0U, ETHTRCV_MODE_STANDBY));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetTransceiverMode(1U, 0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_MODE_ACTIVE, mode);   /* unchanged */
    /* DOWN: BMCR gains power-down bit (init left ANEG_ENABLE set) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_SetTransceiverMode(1U, 0U, ETHTRCV_MODE_DOWN));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_PHY_REG_BMCR, mock_mii_last_write_reg[1]);
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_BMCR_ANEG_ENABLE | ETHTRCV_BMCR_POWER_DOWN,
                             mock_mii_last_write_val[1]);
    /* ACTIVE: power-down cleared, ANEG re-enabled */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_SetTransceiverMode(1U, 0U, ETHTRCV_MODE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT16(ETHTRCV_BMCR_ANEG_ENABLE, mock_mii_last_write_val[1]);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_SetTransceiverMode_MiiWriteFailure_ShouldFail(void) {
    EthTrcv_ModeType mode = ETHTRCV_MODE_ACTIVE;
    mock_mii_write_fail_trcv = 0U;   /* reset write fails during init */
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_SetTransceiverMode(0U, 0U, ETHTRCV_MODE_ACTIVE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetTransceiverMode(0U, 0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_MODE_DOWN, mode);   /* init also failed: still memset state */
}

/** @req SWS_EthTrcv_00006 */
void test_EthTrcv_GetTransceiverMode_NullPtr_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetTransceiverMode(0U, 0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_TRANSCEIVER_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00007 */
void test_EthTrcv_GetLinkState_NullPtr_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetLinkState(0U, 0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_LINK_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00008 */
void test_EthTrcv_GetBaudRate_NullPtr_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetBaudRate(0U, 0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_BAUD_RATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00009 */
void test_EthTrcv_GetDuplexMode_NullPtr_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetDuplexMode(0U, 0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_DUPLEX_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_MainFunction_Uninit_ShouldDoNothing(void) {
    EthTrcv_MainFunction();
    EthTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_LinkChg_Count);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_MainFunction_LinkUpAfterDebounce_ShouldUpdateStateAndNotify(void) {
    EthTrcv_LinkStateType link = ETHTRCV_LINK_STATE_DOWN;
    EthTrcv_BaudRateType baud = ETHTRCV_BAUD_RATE_10MBIT;
    EthTrcv_DuplexModeType duplex = ETHTRCV_DUPLEX_MODE_HALF;
    uint8 i;
    trcv_init();
    /* Trcv1 (RTL8211): link up, 100 Mbit full duplex in PHYSR */
    mock_phy_regs[1][ETHTRCV_PHY_REG_BMSR] = ETHTRCV_BMSR_LINK_STATUS;
    mock_phy_regs[1][ETHTRCV_RTL8211_REG_PHYSR] =
        (uint16)(ETHTRCV_RTL8211_PHYSR_SPEED_100 | ETHTRCV_RTL8211_PHYSR_DUPLEX);
    /* Debounce: LINK_DEBOUNCE_COUNT (3) cycles to assert link up */
    for (i = 0U; i < 2U; i++) {
        EthTrcv_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetLinkState(1U, 0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_DOWN, link);   /* not yet debounced */
    /* Cycle 3 asserts the link; cycle 4 raises the change callback */
    for (i = 0U; i < 2U; i++) {
        EthTrcv_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetLinkState(1U, 0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_ACTIVE, link);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetBaudRate(1U, 0U, &baud));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_BAUD_RATE_100MBIT, baud);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetDuplexMode(1U, 0U, &duplex));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_DUPLEX_MODE_FULL, duplex);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_LinkChg_Count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_LinkChg_LastCtrl);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_ACTIVE, mock_LinkChg_LastState);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_MainFunction_LinkDown_ShouldNotifyDown(void) {
    EthTrcv_LinkStateType link = ETHTRCV_LINK_STATE_DOWN;
    uint8 i;
    trcv_init();
    mock_phy_regs[1][ETHTRCV_PHY_REG_BMSR] = ETHTRCV_BMSR_LINK_STATUS;
    for (i = 0U; i < 5U; i++) {
        EthTrcv_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT32(1U, mock_LinkChg_Count);
    /* Link drops */
    mock_phy_regs[1][ETHTRCV_PHY_REG_BMSR] = 0U;
    for (i = 0U; i < 2U; i++) {
        EthTrcv_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetLinkState(1U, 0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_DOWN, link);
    TEST_ASSERT_EQUAL_UINT32(2U, mock_LinkChg_Count);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_DOWN, mock_LinkChg_LastState);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_MainFunction_MiiReadFailure_ShouldLeaveLinkDown(void) {
    EthTrcv_LinkStateType link = ETHTRCV_LINK_STATE_ACTIVE;
    mock_mii_read_fail_trcv = 1U;   /* detection + init fail for Trcv1 */
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetLinkState(1U, 0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_DOWN, link);
    EthTrcv_MainFunction();
    EthTrcv_MainFunction();
    EthTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_LinkChg_Count);   /* never initialized: no polling */
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetLinkState(1U, 0U, &link));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_LINK_STATE_DOWN, link);
}

/** @req SWS_EthTrcv_00010 */
void test_EthTrcv_CheckWakeup_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_CheckWakeup(ECUM_WKSOURCE_ETH));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_CHECK_WAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00010 */
void test_EthTrcv_CheckWakeup_TJA1100_RemoteWakeupFlag_ShouldIndicate(void) {
    trcv_init();
    /* No remote wake-up pending */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_CheckWakeup(ECUM_WKSOURCE_ETH));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_Wakeup_Count);
    /* Remote wake-up reason latched in COMM_STATUS */
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_COMM_STATUS] = ETHTRCV_TJA1100_COMM_REM_WUR;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_CheckWakeup(ECUM_WKSOURCE_ETH));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_Wakeup_Count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_Wakeup_LastTrcv);
    /* Non-matching source: no transceiver configured for it */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_CheckWakeup(ECUM_WKSOURCE_CAN));
}

/** @req SWS_EthTrcv_00011 */
void test_EthTrcv_ReadMiiIndication_InvalidTrcvIdx_ShouldReportError(void) {
    uint16 val = 0U;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_ReadMiiIndication(ETHTRCV_NUMBER_OF_TRCVS, 5U, &val));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_PHY_REG_READ, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_TRCV_IDX, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00011 */
void test_EthTrcv_ReadMiiIndication_NullPtr_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_ReadMiiIndication(0U, 5U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_PHY_REG_READ, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00011 */
void test_EthTrcv_ReadMiiIndication_ValidCall_ShouldSucceed(void) {
    uint16 val = 0x1234U;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_ReadMiiIndication(0U, 5U, &val));
}

/** @req SWS_EthTrcv_00012 */
void test_EthTrcv_WriteMiiIndication_InvalidTrcvIdx_ShouldReportError(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_WriteMiiIndication(ETHTRCV_NUMBER_OF_TRCVS, 5U, 0U));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_PHY_REG_WRITE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_TRCV_IDX, mock_DetLastErrorId);
}

/** @req SWS_EthTrcv_00012 */
void test_EthTrcv_WriteMiiIndication_ValidCall_ShouldSucceed(void) {
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_WriteMiiIndication(0U, 5U, 0x00FFU));
}

/** @req SWS_EthTrcv_00013 */
void test_EthTrcv_GetSignalQuality_NullPtr_ShouldReportError(void) {
    EthTrcv_SignalQualityType quality = ETHTRCV_SIGNAL_QUALITY_INVALID;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetSignalQuality(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_SIGNAL_QUALITY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
    (void)quality;
}

/** @req SWS_EthTrcv_00013 */
void test_EthTrcv_GetSignalQuality_UnsupportedTrcv_ShouldReturnInvalid(void) {
    EthTrcv_SignalQualityType quality = ETHTRCV_SIGNAL_QUALITY_EXCELLENT;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetSignalQuality(1U, &quality));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_SIGNAL_QUALITY_INVALID, (int)quality);
}

/** @req SWS_EthTrcv_00013 */
void test_EthTrcv_GetSignalQuality_TJA1100_LinkFailCounterMap(void) {
    EthTrcv_SignalQualityType quality = ETHTRCV_SIGNAL_QUALITY_INVALID;
    trcv_init();
    /* Link down -> no connection */
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_COMM_STATUS] = 0U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetSignalQuality(0U, &quality));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_SIGNAL_QUALITY_NO_CONNECTION, (int)quality);
    /* Link up, fail counter mapped to quality bands */
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_COMM_STATUS] = ETHTRCV_TJA1100_COMM_LINK_UP;
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_LINK_FAIL_COUNTER] = 0U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetSignalQuality(0U, &quality));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_SIGNAL_QUALITY_EXCELLENT, (int)quality);
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_LINK_FAIL_COUNTER] = 3U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetSignalQuality(0U, &quality));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_SIGNAL_QUALITY_GOOD, (int)quality);
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_LINK_FAIL_COUNTER] = 10U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetSignalQuality(0U, &quality));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_SIGNAL_QUALITY_WEAK, (int)quality);
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_LINK_FAIL_COUNTER] = 25U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetSignalQuality(0U, &quality));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_SIGNAL_QUALITY_POOR, (int)quality);
}

/** @req SWS_EthTrcv_00014 */
void test_EthTrcv_GetCableDiagnosticsResult_NullPtr_ShouldReportError(void) {
    EthTrcv_CableDiagnosticsResultType result = ETHTRCV_CABLE_DIAGNOSTICS_OK;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetCableDiagnosticsResult(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_SID_GET_CABLE_DIAGNOSTICS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHTRCV_E_INV_POINTER, mock_DetLastErrorId);
    (void)result;
}

/** @req SWS_EthTrcv_00014 */
void test_EthTrcv_GetCableDiagnosticsResult_UnsupportedTrcv_ShouldFail(void) {
    EthTrcv_CableDiagnosticsResultType result = ETHTRCV_CABLE_DIAGNOSTICS_OK;
    trcv_init();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, EthTrcv_GetCableDiagnosticsResult(1U, &result));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_CABLE_DIAGNOSTICS_FAILED, (int)result);
}

/** @req SWS_EthTrcv_00014 */
void test_EthTrcv_GetCableDiagnosticsResult_TJA1100_PhyStateMap(void) {
    EthTrcv_CableDiagnosticsResultType result = ETHTRCV_CABLE_DIAGNOSTICS_FAILED;
    trcv_init();
    /* PHY state 5 (active, 0x280 in COMM_STATUS bits 9:7) -> cable OK */
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_COMM_STATUS] = ETHTRCV_TJA1100_COMM_PHY_STATE_ACTIVE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetCableDiagnosticsResult(0U, &result));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_CABLE_DIAGNOSTICS_OK, (int)result);
    /* PHY state 0 (idle/error) -> open circuit */
    mock_phy_regs[0][ETHTRCV_TJA1100_REG_COMM_STATUS] = 0U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthTrcv_GetCableDiagnosticsResult(0U, &result));
    TEST_ASSERT_EQUAL_INT((int)ETHTRCV_CABLE_DIAGNOSTICS_OPEN_CIRCUIT, (int)result);
}

/* ---------------------------------------------------------------------------
 * Runner
 * ------------------------------------------------------------------------- */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_EthTrcv_Init_NullConfig_ShouldUseDefaultConfig);
    RUN_TEST(test_EthTrcv_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_EthTrcv_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_EthTrcv_DeInit_AfterInit_ShouldBlockFurtherCalls);
    RUN_TEST(test_EthTrcv_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_GetVersionInfo_ValidPtr_ShouldReturnIds);
    RUN_TEST(test_EthTrcv_SetTransceiverMode_Uninit_ShouldReportError);
    RUN_TEST(test_EthTrcv_SetTransceiverMode_InvalidTrcvIdx_ShouldReportError);
    RUN_TEST(test_EthTrcv_SetTransceiverMode_InvalidMode_ShouldReportError);
    RUN_TEST(test_EthTrcv_SetTransceiverMode_TJA1100_ModeMap_ShouldWriteExtCtrl);
    RUN_TEST(test_EthTrcv_SetTransceiverMode_RTL8211_StandbyUnsupported_ShouldFail);
    RUN_TEST(test_EthTrcv_SetTransceiverMode_MiiWriteFailure_ShouldFail);
    RUN_TEST(test_EthTrcv_GetTransceiverMode_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_GetLinkState_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_GetBaudRate_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_GetDuplexMode_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_MainFunction_Uninit_ShouldDoNothing);
    RUN_TEST(test_EthTrcv_MainFunction_LinkUpAfterDebounce_ShouldUpdateStateAndNotify);
    RUN_TEST(test_EthTrcv_MainFunction_LinkDown_ShouldNotifyDown);
    RUN_TEST(test_EthTrcv_MainFunction_MiiReadFailure_ShouldLeaveLinkDown);
    RUN_TEST(test_EthTrcv_CheckWakeup_Uninit_ShouldReportError);
    RUN_TEST(test_EthTrcv_CheckWakeup_TJA1100_RemoteWakeupFlag_ShouldIndicate);
    RUN_TEST(test_EthTrcv_ReadMiiIndication_InvalidTrcvIdx_ShouldReportError);
    RUN_TEST(test_EthTrcv_ReadMiiIndication_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_ReadMiiIndication_ValidCall_ShouldSucceed);
    RUN_TEST(test_EthTrcv_WriteMiiIndication_InvalidTrcvIdx_ShouldReportError);
    RUN_TEST(test_EthTrcv_WriteMiiIndication_ValidCall_ShouldSucceed);
    RUN_TEST(test_EthTrcv_GetSignalQuality_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_GetSignalQuality_UnsupportedTrcv_ShouldReturnInvalid);
    RUN_TEST(test_EthTrcv_GetSignalQuality_TJA1100_LinkFailCounterMap);
    RUN_TEST(test_EthTrcv_GetCableDiagnosticsResult_NullPtr_ShouldReportError);
    RUN_TEST(test_EthTrcv_GetCableDiagnosticsResult_UnsupportedTrcv_ShouldFail);
    RUN_TEST(test_EthTrcv_GetCableDiagnosticsResult_TJA1100_PhyStateMap);

    return UNITY_END();
}

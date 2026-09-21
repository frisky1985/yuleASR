/**
 * @file test_eth.c
 * @brief Eth Unit Tests — substantiated against the production Eth driver
 *        (src/bsw/mcal/eth/src/Eth.c, software-emulated HW layer).
 * @version 2.0.0
 * @date 2026-09-17
 */

// @tests src/bsw/mcal/eth/src/Eth.c  @tests src/bsw/mcal/eth/include/Eth.h

#include "unity.h"
#include "Eth_Cfg.h"  /* defines ETH_VERSION_INFO_API before Eth.h is parsed */
#include "Eth.h"
#include "Eth_Private.h"

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
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test configuration: one controller */
/* Non-const: Eth_SetPhysAddr writes the new MAC back through
   Eth_CtrlState[].ConfigPtr->MacAddr (production stores it in the config). */
static Eth_ControllerConfigType testCtrlConfig = {
    0U,                            /* CtrlIdx */
    {0x02U, 0x00U, 0x00U, 0x12U, 0x34U, 0x56U},  /* MacAddr */
    ETH_RATE_100MBPS,              /* Speed */
    TRUE,                          /* FullDuplex */
    FALSE,                         /* RxChecksumOffload */
    FALSE,                         /* TxChecksumOffload */
    0x01U,                         /* PhyAddress */
    ETH_MAX_TX_BUFS,               /* TxBufCount */
    ETH_MAX_RX_BUFS,               /* RxBufCount */
    ETH_DEFAULT_FRAME_SIZE         /* BufSize */
};

static const Eth_ConfigType testConfig = {
    &testCtrlConfig,               /* CtrlConfig */
    1U,                            /* NumControllers */
    TRUE,                          /* DevErrorDetect */
    TRUE                           /* VersionInfoApi */
};

/* Direct module-state reset: Eth_DeInit() would report DET when already
   uninitialized, so the test pokes the exported state instead. */
static void test_Eth_ResetModuleState(void) {
    uint8 ctrlIdx;
    Eth_InternalState.ModuleState = ETH_STATE_UNINIT;
    Eth_InternalState.Initialized = FALSE;
    Eth_InternalState.NumControllers = 0U;
    for (ctrlIdx = 0U; ctrlIdx < ETH_MAX_CONTROLLERS; ctrlIdx++) {
        Eth_CtrlState[ctrlIdx].State = ETH_STATE_UNINIT;
        Eth_CtrlState[ctrlIdx].Mode = ETH_MODE_DOWN;
        Eth_CtrlState[ctrlIdx].InitDone = FALSE;
    }
}

/* Init helper: module + controller 0 fully initialized (mode DOWN) */
static void test_Eth_InitModule(void) {
    Eth_Init(&testConfig);
    Eth_ControllerInit(0U, &testCtrlConfig);
}

/* Init helper: module + controller 0 in ACTIVE mode */
static void test_Eth_InitModuleActive(void) {
    test_Eth_InitModule();
    (void)Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
}

void setUp(void) {
    mock_Det_Reset();
    test_Eth_ResetModuleState();
}

void tearDown(void) {
}

/** @req SWS_Eth_00001 */
void test_Eth_Init_NullPtr_ShouldReportError(void) {
    Eth_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_INIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
    /* Module must remain uninitialized */
    TEST_ASSERT_FALSE(Eth_InternalState.Initialized);
}

/** @req SWS_Eth_00001 */
void test_Eth_Init_ValidConfig_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_TRUE(Eth_InternalState.Initialized);
    TEST_ASSERT_EQUAL_UINT8(ETH_STATE_INIT, Eth_InternalState.ModuleState);
    TEST_ASSERT_EQUAL_UINT8(1U, Eth_InternalState.NumControllers);
    TEST_ASSERT_EQUAL_UINT8(ETH_STATE_INIT, Eth_CtrlState[0].State);
    TEST_ASSERT_EQUAL_UINT8(ETH_MAX_TX_BUFS, Eth_CtrlState[0].TxBufCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_MAX_RX_BUFS, Eth_CtrlState[0].RxBufCount);
}

/** @req SWS_Eth_00001 */
void test_Eth_Init_DoubleInit_ShouldReinitialize(void) {
    Eth_Init(&testConfig);
    Eth_Init(&testConfig);
    TEST_ASSERT_TRUE(Eth_InternalState.Initialized);
    TEST_ASSERT_EQUAL_UINT8(ETH_STATE_INIT, Eth_CtrlState[0].State);
}

/** @req SWS_Eth_00002 */
void test_Eth_DeInit_AfterInit_ShouldReturnToUninit(void) {
    Eth_Init(&testConfig);
    Eth_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_FALSE(Eth_InternalState.Initialized);
    TEST_ASSERT_EQUAL_UINT8(ETH_STATE_UNINIT, Eth_InternalState.ModuleState);
    TEST_ASSERT_EQUAL_UINT8(ETH_STATE_UNINIT, Eth_CtrlState[0].State);
    TEST_ASSERT_FALSE(Eth_CtrlState[0].InitDone);
}

/** @req SWS_Eth_00002 */
void test_Eth_DeInit_Uninit_ShouldReportError(void) {
    Eth_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_DEINIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00003 */
void test_Eth_ControllerInit_NullPtr_ShouldReportError(void) {
    Eth_ControllerInit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_INIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00003 */
void test_Eth_ControllerInit_InvalidCtrl_ShouldReportError(void) {
    Eth_ControllerInit(0x05U, &testCtrlConfig);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_CTRL_INDEX, mock_DetLastErrorId);
}

/** @req SWS_Eth_00003 */
void test_Eth_ControllerInit_Valid_ShouldSetInitDone(void) {
    Eth_ControllerInit(0U, &testCtrlConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_TRUE(Eth_CtrlState[0].InitDone);
    TEST_ASSERT_EQUAL_UINT8(ETH_STATE_INIT, Eth_CtrlState[0].State);
    TEST_ASSERT_EQUAL_UINT8(ETH_MODE_DOWN, Eth_CtrlState[0].Mode);
}

/** @req SWS_Eth_00004 */
void test_Eth_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Eth_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_GETVERSIONINFO_APIID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00004 */
void test_Eth_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType version;
    Eth_GetVersionInfo(&version);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x01U, version.vendorID);
    TEST_ASSERT_EQUAL_UINT8(ETH_MODULE_ID, version.moduleID);
    TEST_ASSERT_EQUAL_UINT8(ETH_SW_MAJOR_VERSION, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(ETH_SW_MINOR_VERSION, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(ETH_SW_PATCH_VERSION, version.sw_patch_version);
}

/** @req SWS_Eth_00005 */
void test_Eth_SetControllerMode_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_SETCONTROLLERMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00005 */
void test_Eth_SetControllerMode_InvalidCtrl_ShouldReportError(void) {
    Std_ReturnType ret;
    Eth_Init(&testConfig);
    ret = Eth_SetControllerMode(0x05U, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_CTRL_INDEX, mock_DetLastErrorId);
}

/** @req SWS_Eth_00005 */
void test_Eth_SetControllerMode_InvalidMode_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_SetControllerMode(0U, 0x55U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_MODE, mock_DetLastErrorId);
}

/** @req SWS_Eth_00005 */
void test_Eth_SetControllerMode_CtrlNotInit_ShouldReportError(void) {
    Std_ReturnType ret;
    /* Module initialized, but controller InitDone is still FALSE */
    Eth_Init(&testConfig);
    ret = Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00005 */
void test_Eth_SetControllerMode_ValidTransitions_ShouldSucceed(void) {
    Std_ReturnType ret;
    Eth_ModeType mode = ETH_MODE_DOWN;
    test_Eth_InitModule();
    ret = Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Eth_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(ETH_MODE_ACTIVE, mode);
    ret = Eth_SetControllerMode(0U, ETH_MODE_DOWN);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(E_OK, Eth_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(ETH_MODE_DOWN, mode);
}

/** @req SWS_Eth_00006 */
void test_Eth_GetControllerMode_CtrlNotInit_ShouldFail(void) {
    Std_ReturnType ret;
    Eth_ModeType mode = ETH_MODE_ACTIVE;
    Eth_Init(&testConfig);
    ret = Eth_GetControllerMode(0U, &mode);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00006 */
void test_Eth_GetControllerMode_NullPtr_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_GetControllerMode(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00007 */
void test_Eth_GetControllerIdx_NullName_ShouldReportError(void) {
    uint8 idx = Eth_GetControllerIdx(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(ETH_INVALID_CONTROLLER_INDEX, idx);
    TEST_ASSERT_EQUAL_UINT8(ETH_GETCONTROLLERIDX_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00007 */
void test_Eth_GetControllerIdx_ValidName_ShouldReturnZero(void) {
    uint8 idx = Eth_GetControllerIdx((const uint8*)"ETH0");
    TEST_ASSERT_EQUAL_UINT8(0U, idx);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Eth_00008 */
void test_Eth_GetPhysAddr_NullPtr_ShouldReportError(void) {
    Eth_GetPhysAddr(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_GETPHYSADDR_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00008 */
void test_Eth_GetPhysAddr_InvalidCtrl_ShouldReportError(void) {
    uint8 mac[6] = {0U};
    Eth_GetPhysAddr(0x05U, mac);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_CTRL_INDEX, mock_DetLastErrorId);
}

/** @req SWS_Eth_00008 */
void test_Eth_GetPhysAddr_AfterCtrlInit_ShouldReturnConfiguredMac(void) {
    uint8 mac[6] = {0U};
    test_Eth_InitModule();
    Eth_GetPhysAddr(0U, mac);
    TEST_ASSERT_EQUAL_UINT8(0x02U, mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0x56U, mac[5]);
}

/** @req SWS_Eth_00009 */
void test_Eth_SetPhysAddr_AfterCtrlInit_ShouldUpdateMac(void) {
    const uint8 newMac[6] = {0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU};
    uint8 mac[6] = {0U};
    test_Eth_InitModule();
    Eth_SetPhysAddr(0U, newMac);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    Eth_GetPhysAddr(0U, mac);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(newMac, mac, 6U);
}

/** @req SWS_Eth_00010 */
void test_Eth_UpdatePhysAddrFilter_Uninit_ShouldFail(void) {
    const uint8 mac[6] = {0x01U, 0x00U, 0x5EU, 0x00U, 0x00U, 0x01U};
    Std_ReturnType ret = Eth_UpdatePhysAddrFilter(0U, mac, ETH_FILTER_ACTION_ADD);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00010 */
void test_Eth_UpdatePhysAddrFilter_NullPtr_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_UpdatePhysAddrFilter(0U, NULL_PTR, ETH_FILTER_ACTION_ADD);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00010 */
void test_Eth_UpdatePhysAddrFilter_InvalidAction_ShouldReportError(void) {
    const uint8 mac[6] = {0x01U, 0x00U, 0x5EU, 0x00U, 0x00U, 0x01U};
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_UpdatePhysAddrFilter(0U, mac, 0x55U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_PARAM, mock_DetLastErrorId);
}

/** @req SWS_Eth_00010 */
void test_Eth_UpdatePhysAddrFilter_ValidAddRemove_ShouldSucceed(void) {
    const uint8 mac[6] = {0x01U, 0x00U, 0x5EU, 0x00U, 0x00U, 0x01U};
    test_Eth_InitModule();
    TEST_ASSERT_EQUAL_UINT8(E_OK, Eth_UpdatePhysAddrFilter(0U, mac, ETH_FILTER_ACTION_ADD));
    TEST_ASSERT_EQUAL_UINT8(E_OK, Eth_UpdatePhysAddrFilter(0U, mac, ETH_FILTER_ACTION_REMOVE));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Eth_00011 */
void test_Eth_WriteMii_CtrlNotInit_ShouldFail(void) {
    Std_ReturnType ret;
    Eth_Init(&testConfig);
    ret = Eth_WriteMii(0U, 0x01U, ETH_MII_REG_BMCR, 0x1000U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00011 */
void test_Eth_WriteMii_AfterCtrlInit_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_WriteMii(0U, 0x01U, ETH_MII_REG_BMCR, 0x1000U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Eth_00012 */
void test_Eth_ReadMii_NullPtr_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_ReadMii(0U, 0x01U, ETH_MII_REG_BMSR, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00012 */
void test_Eth_ReadMii_AfterCtrlInit_ShouldReturnZero(void) {
    Eth_DataType data = 0xFFFFU;
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_ReadMii(0U, 0x01U, ETH_MII_REG_BMSR, &data);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(0U, data);
}

/** @req SWS_Eth_00013 */
void test_Eth_ProvideTxBuffer_Uninit_ShouldReportError(void) {
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    BufReq_ReturnType ret = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    TEST_ASSERT_EQUAL_UINT8(BUFREQ_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00013 */
void test_Eth_ProvideTxBuffer_NullPtr_ShouldReportError(void) {
    uint16 len = 64U;
    BufReq_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, NULL_PTR, NULL_PTR, &len);
    TEST_ASSERT_EQUAL_UINT8(BUFREQ_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Eth_00013 */
void test_Eth_ProvideTxBuffer_AfterCtrlInit_ShouldProvideBuffer(void) {
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    BufReq_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    TEST_ASSERT_EQUAL_UINT8(BUFREQ_E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(ETH_INVALID_BUF_INDEX, bufIdx);
    TEST_ASSERT_NOT_NULL(bufPtr);
    TEST_ASSERT_TRUE(Eth_TxDesc[0][bufIdx].DataPtr == bufPtr);
    TEST_ASSERT_EQUAL_UINT8(ETH_BUF_STATE_BUSY, Eth_TxDesc[0][bufIdx].State);
}

/** @req SWS_Eth_00013 */
void test_Eth_ProvideTxBuffer_ExhaustAllBuffers_ShouldReturnBusy(void) {
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    BufReq_ReturnType ret = BUFREQ_E_OK;
    uint8 i;
    test_Eth_InitModule();
    for (i = 0U; i < ETH_MAX_TX_BUFS; i++) {
        ret = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
        TEST_ASSERT_EQUAL_UINT8(BUFREQ_E_OK, ret);
    }
    ret = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    TEST_ASSERT_EQUAL_UINT8(BUFREQ_E_BUSY, ret);
}

/** @req SWS_Eth_00014 */
void test_Eth_Transmit_NotActiveMode_ShouldFail(void) {
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    Std_ReturnType ret;
    test_Eth_InitModule();  /* mode stays DOWN */
    (void)Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    ret = Eth_Transmit(0U, bufIdx, 0x0800U, TRUE, 64U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Eth_00014 */
void test_Eth_Transmit_InvalidBufIdx_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Eth_InitModuleActive();
    ret = Eth_Transmit(0U, 0xFFU, 0x0800U, TRUE, 64U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_BUF_INDEX, mock_DetLastErrorId);
}

/** @req SWS_Eth_00014 */
void test_Eth_Transmit_BufNotBusy_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Eth_InitModuleActive();
    /* Buffer 0 was never provided, so its state is FREE, not BUSY */
    ret = Eth_Transmit(0U, 0U, 0x0800U, TRUE, 64U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_PARAM, mock_DetLastErrorId);
}

/** @req SWS_Eth_00014 */
void test_Eth_Transmit_ValidFrame_ShouldSucceed(void) {
    const uint8 dstMac[6] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    Std_ReturnType ret;
    test_Eth_InitModuleActive();
    (void)Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    ret = Eth_Transmit(0U, bufIdx, 0x0800U, TRUE, 64U, dstMac);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_BUF_STATE_TRANSMITTING, Eth_TxDesc[0][bufIdx].State);
    TEST_ASSERT_EQUAL_UINT16(64U, Eth_TxDesc[0][bufIdx].Len);
    TEST_ASSERT_EQUAL_UINT16(0x0800U, Eth_TxDesc[0][bufIdx].FrameType);
    TEST_ASSERT_TRUE(Eth_TxDesc[0][bufIdx].TxConfirmation);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(dstMac, &Eth_TxDesc[0][bufIdx].DataPtr[0], 6U);
}

/** @req SWS_Eth_00016 */
void test_Eth_TxConfirmation_InvalidCtrl_ShouldReportError(void) {
    Eth_TxConfirmation(0x05U, 0U);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_TXCONFIRMATION_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_CTRL_INDEX, mock_DetLastErrorId);
}

/** @req SWS_Eth_00016 */
void test_Eth_TxConfirmation_AfterTransmit_ShouldFreeBuffer(void) {
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    test_Eth_InitModuleActive();
    (void)Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    (void)Eth_Transmit(0U, bufIdx, 0x0800U, TRUE, 64U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(ETH_BUF_STATE_TRANSMITTING, Eth_TxDesc[0][bufIdx].State);
    Eth_TxConfirmation(0U, bufIdx);
    TEST_ASSERT_EQUAL_UINT8(ETH_BUF_STATE_FREE, Eth_TxDesc[0][bufIdx].State);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Eth_00015 */
void test_Eth_Receive_NullPtr_ShouldReportError(void) {
    uint8 rxStatus = 0U;
    Eth_BufIdxType bufIdx = 0U;
    Eth_FrameStructType* frame = NULL_PTR;
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_Receive(0U, NULL_PTR, &bufIdx, &frame);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_POINTER, mock_DetLastErrorId);
    (void)rxStatus;
}

/** @req SWS_Eth_00015 */
void test_Eth_Receive_InvalidCtrl_ShouldReportError(void) {
    uint8 rxStatus = 0U;
    Eth_BufIdxType bufIdx = 0U;
    Eth_FrameStructType* frame = NULL_PTR;
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_Receive(0x05U, &rxStatus, &bufIdx, &frame);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_INV_CTRL_INDEX, mock_DetLastErrorId);
}

/** @req SWS_Eth_00015 */
void test_Eth_Receive_NoFramePending_ShouldReportEmpty(void) {
    uint8 rxStatus = 0xFFU;
    Eth_BufIdxType bufIdx = 0U;
    Eth_FrameStructType* frame = NULL_PTR;
    Std_ReturnType ret;
    test_Eth_InitModule();
    ret = Eth_Receive(0U, &rxStatus, &bufIdx, &frame);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0x00U, rxStatus);
}

/** @req SWS_Eth_00017 */
void test_Eth_EnableIrq_Uninit_ShouldReportError(void) {
    Eth_EnableIrq();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_ENABLEIRQ_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00017 */
void test_Eth_EnableDisableIrq_AfterInit_ShouldToggleFlag(void) {
    test_Eth_InitModule();
    Eth_EnableIrq();
    TEST_ASSERT_TRUE(Eth_CtrlState[0].InterruptsEnabled);
    Eth_DisableIrq();
    TEST_ASSERT_FALSE(Eth_CtrlState[0].InterruptsEnabled);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Eth_00020 */
void test_Eth_MainFunction_Uninit_ShouldReportError(void) {
    Eth_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETH_MAINFUNCTION_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETH_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Eth_00020 */
void test_Eth_MainFunction_AfterInit_ShouldConfirmTransmittedBuffers(void) {
    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 64U;
    test_Eth_InitModuleActive();
    (void)Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    (void)Eth_Transmit(0U, bufIdx, 0x0800U, TRUE, 64U, NULL_PTR);
    /* Simulate HW completing the transmission */
    Eth_TxDesc[0][bufIdx].State = ETH_BUF_STATE_READY;
    Eth_CtrlState[0].TxPendingCount = 1U;
    Eth_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETH_BUF_STATE_FREE, Eth_TxDesc[0][bufIdx].State);
    TEST_ASSERT_EQUAL_UINT32(0U, Eth_CtrlState[0].TxPendingCount);
}

/* ---------------------------------------------------------------------------
 * Runner
 * ------------------------------------------------------------------------- */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_Eth_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Eth_Init_DoubleInit_ShouldReinitialize);
    RUN_TEST(test_Eth_DeInit_AfterInit_ShouldReturnToUninit);
    RUN_TEST(test_Eth_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_Eth_ControllerInit_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_ControllerInit_InvalidCtrl_ShouldReportError);
    RUN_TEST(test_Eth_ControllerInit_Valid_ShouldSetInitDone);
    RUN_TEST(test_Eth_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_GetVersionInfo_ValidPtr_ShouldFillFields);
    RUN_TEST(test_Eth_SetControllerMode_Uninit_ShouldReportError);
    RUN_TEST(test_Eth_SetControllerMode_InvalidCtrl_ShouldReportError);
    RUN_TEST(test_Eth_SetControllerMode_InvalidMode_ShouldReportError);
    RUN_TEST(test_Eth_SetControllerMode_CtrlNotInit_ShouldReportError);
    RUN_TEST(test_Eth_SetControllerMode_ValidTransitions_ShouldSucceed);
    RUN_TEST(test_Eth_GetControllerMode_CtrlNotInit_ShouldFail);
    RUN_TEST(test_Eth_GetControllerMode_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_GetControllerIdx_NullName_ShouldReportError);
    RUN_TEST(test_Eth_GetControllerIdx_ValidName_ShouldReturnZero);
    RUN_TEST(test_Eth_GetPhysAddr_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_GetPhysAddr_InvalidCtrl_ShouldReportError);
    RUN_TEST(test_Eth_GetPhysAddr_AfterCtrlInit_ShouldReturnConfiguredMac);
    RUN_TEST(test_Eth_SetPhysAddr_AfterCtrlInit_ShouldUpdateMac);
    RUN_TEST(test_Eth_UpdatePhysAddrFilter_Uninit_ShouldFail);
    RUN_TEST(test_Eth_UpdatePhysAddrFilter_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_UpdatePhysAddrFilter_InvalidAction_ShouldReportError);
    RUN_TEST(test_Eth_UpdatePhysAddrFilter_ValidAddRemove_ShouldSucceed);
    RUN_TEST(test_Eth_WriteMii_CtrlNotInit_ShouldFail);
    RUN_TEST(test_Eth_WriteMii_AfterCtrlInit_ShouldSucceed);
    RUN_TEST(test_Eth_ReadMii_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_ReadMii_AfterCtrlInit_ShouldReturnZero);
    RUN_TEST(test_Eth_ProvideTxBuffer_Uninit_ShouldReportError);
    RUN_TEST(test_Eth_ProvideTxBuffer_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_ProvideTxBuffer_AfterCtrlInit_ShouldProvideBuffer);
    RUN_TEST(test_Eth_ProvideTxBuffer_ExhaustAllBuffers_ShouldReturnBusy);
    RUN_TEST(test_Eth_Transmit_NotActiveMode_ShouldFail);
    RUN_TEST(test_Eth_Transmit_InvalidBufIdx_ShouldReportError);
    RUN_TEST(test_Eth_Transmit_BufNotBusy_ShouldReportError);
    RUN_TEST(test_Eth_Transmit_ValidFrame_ShouldSucceed);
    RUN_TEST(test_Eth_TxConfirmation_InvalidCtrl_ShouldReportError);
    RUN_TEST(test_Eth_TxConfirmation_AfterTransmit_ShouldFreeBuffer);
    RUN_TEST(test_Eth_Receive_NullPtr_ShouldReportError);
    RUN_TEST(test_Eth_Receive_InvalidCtrl_ShouldReportError);
    RUN_TEST(test_Eth_Receive_NoFramePending_ShouldReportEmpty);
    RUN_TEST(test_Eth_EnableIrq_Uninit_ShouldReportError);
    RUN_TEST(test_Eth_EnableDisableIrq_AfterInit_ShouldToggleFlag);
    RUN_TEST(test_Eth_MainFunction_Uninit_ShouldReportError);
    RUN_TEST(test_Eth_MainFunction_AfterInit_ShouldConfirmTransmittedBuffers);

    return UNITY_END();
}

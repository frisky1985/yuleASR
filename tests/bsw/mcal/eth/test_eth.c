/**
 * @file test_eth.c
 * @brief Eth (Ethernet Driver) Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/eth/src/Eth.c  @tests src/bsw/mcal/eth/include/Eth.h

#include "unity.h"
#include "Eth_Cfg.h"
#include "Eth.h"
#include "Eth_Private.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

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

/* Test config */
static Eth_ControllerConfigType testCtrlConfig;
static Eth_ConfigType testConfig;

static void test_Eth_SetupDefaultConfig(void) {
    testCtrlConfig.CtrlIdx = 0U;
    testCtrlConfig.MacAddr[0] = 0x00U;
    testCtrlConfig.MacAddr[1] = 0x11U;
    testCtrlConfig.MacAddr[2] = 0x22U;
    testCtrlConfig.MacAddr[3] = 0x33U;
    testCtrlConfig.MacAddr[4] = 0x44U;
    testCtrlConfig.MacAddr[5] = 0x55U;
    testCtrlConfig.Speed = ETH_RATE_100MBPS;
    testCtrlConfig.FullDuplex = TRUE;
    testCtrlConfig.RxChecksumOffload = FALSE;
    testCtrlConfig.TxChecksumOffload = FALSE;
    testCtrlConfig.PhyAddress = 0x01U;
    testCtrlConfig.TxBufCount = 4U;
    testCtrlConfig.RxBufCount = 4U;
    testCtrlConfig.BufSize = 1536U;

    testConfig.CtrlConfig = &testCtrlConfig;
    testConfig.NumControllers = 1U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
}

void setUp(void) {
    mock_Det_Reset();
    Eth_InternalState.ModuleState = ETH_STATE_UNINIT;
    Eth_InternalState.Initialized = FALSE;
    Eth_InternalState.NumControllers = 0U;
    test_Eth_SetupDefaultConfig();
}

void tearDown(void) {
}

/*==================================================================================================
 *                                    INIT/DEINIT TESTS
 *==================================================================================================*/
/** @req SWS_Eth_00001 */
void test_Eth_Init_NullPtr_ShouldReportError(void) {
    Eth_Init(NULL_PTR);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(ETH_E_INV_POINTER, mock_DetLastErrorId);
}

void test_Eth_Init_ValidConfig_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    TEST_ASSERT_EQUAL(ETH_STATE_INIT, Eth_InternalState.ModuleState);
}

void test_Eth_DeInit_AfterInit_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Eth_DeInit();
    TEST_ASSERT_EQUAL(ETH_STATE_UNINIT, Eth_InternalState.ModuleState);
}

/*==================================================================================================
 *                                    CONTROLLER MODE TESTS
 *==================================================================================================*/
void test_Eth_SetControllerMode_Uninit_ShouldReportError(void) {
    Std_ReturnType result = Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
}

void test_Eth_SetControllerMode_ValidMode_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Std_ReturnType result = Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Eth_GetControllerMode_AfterSet_ShouldMatch(void) {
    Eth_Init(&testConfig);
    Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);

    Eth_ModeType mode = ETH_MODE_DOWN;
    Std_ReturnType result = Eth_GetControllerMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(ETH_MODE_ACTIVE, mode);
}

void test_Eth_SetControllerMode_InvalidCtrl_ShouldReportError(void) {
    Eth_Init(&testConfig);
    Std_ReturnType result = Eth_SetControllerMode(0xFFU, ETH_MODE_ACTIVE);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================================================================================================
 *                                    BUFFER MANAGEMENT TESTS
 *==================================================================================================*/
void test_Eth_ProvideTxBuffer_Uninit_ShouldReturnError(void) {
    Eth_BufIdxType bufIdx;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 0U;
    BufReq_ReturnType result = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    TEST_ASSERT_FALSE(BUFREQ_OK == result);
}

void test_Eth_ProvideTxBuffer_ValidRequest_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);

    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 0U;
    BufReq_ReturnType result = Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);
    TEST_ASSERT_EQUAL(BUFREQ_OK, result);
    TEST_ASSERT_FALSE(ETH_INVALID_BUF_INDEX == bufIdx);
}

/*==================================================================================================
 *                                    TRANSMISSION TESTS
 *==================================================================================================*/
void test_Eth_Transmit_Uninit_ShouldReturnError(void) {
    uint8 data[64];
    Std_ReturnType result = Eth_Transmit(0U, 0U, 0x0800U, TRUE, 64U, data);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_Eth_Transmit_NullPtr_ShouldReturnError(void) {
    Eth_Init(&testConfig);
    Std_ReturnType result = Eth_Transmit(0U, 0U, 0x0800U, TRUE, 64U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_Eth_Transmit_ValidFrame_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);

    Eth_BufIdxType bufIdx = ETH_INVALID_BUF_INDEX;
    uint8* bufPtr = NULL_PTR;
    uint16 len = 0U;
    Eth_ProvideTxBuffer(0U, 0x0800U, 0U, &bufIdx, &bufPtr, &len);

    uint8 data[64] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Std_ReturnType result = Eth_Transmit(0U, bufIdx, 0x0800U, TRUE, 64U, data);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/*==================================================================================================
 *                                    PHY INTERFACE TESTS
 *==================================================================================================*/
void test_Eth_WriteMii_ValidAddress_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Std_ReturnType result = Eth_WriteMii(0U, 0x01U, ETH_MII_REG_BMCR, 0x1000U);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Eth_ReadMii_ValidAddress_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Eth_DataType value = 0U;
    Std_ReturnType result = Eth_ReadMii(0U, 0x01U, ETH_MII_REG_BMSR, &value);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Eth_WriteMii_BeforeInit_ShouldReportError(void) {
    Std_ReturnType result = Eth_WriteMii(0U, 0x01U, ETH_MII_REG_BMCR, 0x1000U);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
}

/*==================================================================================================
 *                                    MAIN FUNCTION TEST
 *==================================================================================================*/
void test_Eth_MainFunction_Uninit_ShouldReturnImmediately(void) {
    Eth_MainFunction();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Eth_MainFunction_NormalOperation_ShouldNotCrash(void) {
    Eth_Init(&testConfig);
    Eth_SetControllerMode(0U, ETH_MODE_ACTIVE);
    Eth_MainFunction();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/*==================================================================================================
 *                                    IRQ CONTROL TESTS
 *==================================================================================================*/
void test_Eth_EnableIrq_AfterInit_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Eth_EnableIrq();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Eth_DisableIrq_AfterInit_ShouldSucceed(void) {
    Eth_Init(&testConfig);
    Eth_DisableIrq();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

void test_Eth_EnableIrq_BeforeInit_ShouldReportError(void) {
    Eth_EnableIrq();
    TEST_ASSERT_GREATER_THAN(0U, mock_DetCallCount);
}

/*==================================================================================================
 *                                    VERSION INFO TEST
 *==================================================================================================*/
void test_Eth_GetVersionInfo_ShouldReturnCorrectVersion(void) {
    Std_VersionInfoType versionInfo;
    Eth_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL(ETH_SW_MAJOR_VERSION, versionInfo.vendorID);
}

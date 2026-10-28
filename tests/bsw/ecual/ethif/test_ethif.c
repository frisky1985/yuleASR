/**
 * @file test_ethif.c
 * @brief EthIf (Ethernet Interface) Unit Tests
 * @version 2.0.0
 * @date 2026-10-27
 *
 * 直接编译 EthIf.c 到测试可执行文件, Det mock 由本文件提供.
 * EthIf.c 不依赖真实 Eth 驱动 (本 baseline 只实现 controller mode FSM
 * + RX filter 表), 不需要底层桩.
 */

// @tests src/bsw/ecual/ethif/src/EthIf.c  @tests src/bsw/ecual/ethif/include/EthIf.h

#include "unity.h"
#include "EthIf.h"
#include "EthIf_Cfg.h"
#include <string.h>

/* ---------- Det mock ---------- */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---------- EthIf.c 内部 SID/ERR 常量 (与产品代码一致) ---------- */
#define LOCAL_SID_INIT              0x00U
#define LOCAL_SID_TRANSMIT          0x02U
#define LOCAL_SID_SET_CTRL_MODE     0x05U
#define LOCAL_SID_GET_CTRL_MODE     0x06U
#define LOCAL_SID_RX_INDICATION     0x03U
#define LOCAL_SID_MAINFUNCTION      0x04U
#define LOCAL_E_PARAM_POINTER       0x10U
#define LOCAL_E_UNINIT              0x20U
#define LOCAL_E_PARAM_CONTROLLER    0x30U

/* ---------- Rx callback recorder ---------- */
static uint8 mock_RxCbCount = 0U;
static uint8 mock_RxCbLastCtrl = 0xFFU;
static EthIf_PduType mock_RxCbLastPdu;

static void mock_RxCallback(uint8 ControllerId, const EthIf_PduType* PduInfoPtr) {
    mock_RxCbCount++;
    mock_RxCbLastCtrl = ControllerId;
    if (PduInfoPtr != NULL_PTR) {
        mock_RxCbLastPdu = *PduInfoPtr;
    }
}

/* ---------- Test configs ---------- */
static const EthIf_RxFilterType testFilters[2] = {
    { ETHIF_FILTER_ETHERTYPE, 0U, {0}, 0x88B5U, 0U, mock_RxCallback },
    { ETHIF_FILTER_MAC,       0U, {0x01,0x02,0x03,0x04,0x05,0x06}, 0, 0, mock_RxCallback },
};

static const EthIf_ConfigType testConfig = {
    1U, NULL_PTR, 0U, NULL_PTR, 2U, testFilters
};

static const EthIf_ConfigType emptyConfig = {
    0U, NULL_PTR, 0U, NULL_PTR, 0U, NULL_PTR
};

static const EthIf_ConfigType oversizedConfig = {
    8U, NULL_PTR, 0U, NULL_PTR, 0U, NULL_PTR  /* > ETHIF_MAX_CONTROLLERS (4) */
};

static void mock_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
    mock_RxCbCount = 0U;
    mock_RxCbLastCtrl = 0xFFU;
    (void)memset(&mock_RxCbLastPdu, 0, sizeof(mock_RxCbLastPdu));
    /* DeInit so next test starts from UNINIT state */
    EthIf_DeInit();
}

void setUp(void) { mock_Reset(); }
void tearDown(void) {}

/* ===== Init / DeInit ===== */

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_NullPtr_ShouldReportDet(void) {
    EthIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LOCAL_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_ValidConfig_ShouldEnterInitState(void) {
    EthIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    /* After Init: SetControllerMode succeeds on Ctrl 0 */
    TEST_ASSERT_EQUAL(E_OK, EthIf_SetControllerMode(0U, ETHIF_CS_STOPPED));
}

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_DoubleInit_ShouldReinit(void) {
    EthIf_Init(&testConfig);
    EthIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_EthIf_00002 */
void test_EthIf_DeInit_ShouldResetState(void) {
    EthIf_Init(&testConfig);
    EthIf_DeInit();
    /* After DeInit: SetControllerMode fails with E_UNINIT */
    TEST_ASSERT_EQUAL(E_NOT_OK, EthIf_SetControllerMode(0U, ETHIF_CS_STARTED));
    TEST_ASSERT_EQUAL(LOCAL_SID_SET_CTRL_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_OversizedNumControllers_ShouldAbortInit(void) {
    EthIf_Init(&oversizedConfig);
    /* activeControllerCount stays 0, state stays UNINIT */
    TEST_ASSERT_EQUAL(E_NOT_OK, EthIf_SetControllerMode(0U, ETHIF_CS_STARTED));
}

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_ZeroControllers_ShouldSucceed(void) {
    EthIf_Init(&emptyConfig);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    /* SetControllerMode on Ctrl 0 fails (no controllers configured) */
    TEST_ASSERT_EQUAL(E_NOT_OK, EthIf_SetControllerMode(0U, ETHIF_CS_STARTED));
    TEST_ASSERT_EQUAL(LOCAL_E_PARAM_CONTROLLER, mock_DetLastErrorId);
}

/* ===== Transmit ===== */

/** @req SWS_EthIf_00003 */
void test_EthIf_Transmit_BeforeInit_ShouldReportUninit(void) {
    EthIf_PduType pdu;
    EthIf_Transmit(0U, 0U, &pdu);
    TEST_ASSERT_EQUAL(LOCAL_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00003 */
void test_EthIf_Transmit_NullPdu_ShouldReportDet(void) {
    EthIf_Init(&testConfig);
    EthIf_Transmit(0U, 0U, NULL_PTR);
    TEST_ASSERT_EQUAL(LOCAL_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00003 */
void test_EthIf_Transmit_InvalidController_ShouldReportDet(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    EthIf_Transmit(5U, 0U, &pdu);
    TEST_ASSERT_EQUAL(LOCAL_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_PARAM_CONTROLLER, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00003 */
void test_EthIf_Transmit_StoppedMode_ShouldReturnNotOk(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    /* Default mode after Init is ETHIF_CS_STOPPED */
    TEST_ASSERT_EQUAL(E_NOT_OK, EthIf_Transmit(0U, 0U, &pdu));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount); /* no DET — just not started */
}

/** @req SWS_EthIf_00003 */
void test_EthIf_Transmit_StartedMode_ShouldReturnOk(void) {
    EthIf_Init(&testConfig);
    (void)EthIf_SetControllerMode(0U, ETHIF_CS_STARTED);
    EthIf_PduType pdu;
    TEST_ASSERT_EQUAL(E_OK, EthIf_Transmit(0U, 0U, &pdu));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_EthIf_00003 */
void test_EthIf_Transmit_SleepMode_ShouldReturnNotOk(void) {
    EthIf_Init(&testConfig);
    (void)EthIf_SetControllerMode(0U, ETHIF_CS_SLEEP);
    EthIf_PduType pdu;
    TEST_ASSERT_EQUAL(E_NOT_OK, EthIf_Transmit(0U, 0U, &pdu));
}

/* ===== Set/Get Controller Mode ===== */

/** @req SWS_EthIf_00004 */
void test_EthIf_SetControllerMode_BeforeInit_ShouldReportUninit(void) {
    EthIf_SetControllerMode(0U, ETHIF_CS_STARTED);
    TEST_ASSERT_EQUAL(LOCAL_SID_SET_CTRL_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00004 */
void test_EthIf_SetControllerMode_InvalidController_ShouldReportDet(void) {
    EthIf_Init(&testConfig);
    EthIf_SetControllerMode(5U, ETHIF_CS_STARTED);
    TEST_ASSERT_EQUAL(LOCAL_SID_SET_CTRL_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LOCAL_E_PARAM_CONTROLLER, mock_DetLastErrorId);
}

/** @req SWS_EthIf_00004 */
void test_EthIf_SetControllerMode_Valid_ShouldStoreAndReflect(void) {
    EthIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, EthIf_SetControllerMode(0U, ETHIF_CS_STARTED));
    TEST_ASSERT_EQUAL(ETHIF_CS_STARTED, EthIf_GetControllerMode(0U));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    TEST_ASSERT_EQUAL(E_OK, EthIf_SetControllerMode(0U, ETHIF_CS_SLEEP));
    TEST_ASSERT_EQUAL(ETHIF_CS_SLEEP, EthIf_GetControllerMode(0U));

    TEST_ASSERT_EQUAL(E_OK, EthIf_SetControllerMode(0U, ETHIF_CS_STOPPED));
    TEST_ASSERT_EQUAL(ETHIF_CS_STOPPED, EthIf_GetControllerMode(0U));
}

/** @req SWS_EthIf_00005 */
void test_EthIf_GetControllerMode_InvalidController_ShouldReturnStopped(void) {
    /* Before init: activeControllerCount = 0, Ctrl 5 >= 0 -> returns STOPPED */
    TEST_ASSERT_EQUAL(ETHIF_CS_STOPPED, EthIf_GetControllerMode(5U));
}

/** @req SWS_EthIf_00005 */
void test_EthIf_GetControllerMode_BeforeInit_ShouldReturnStopped(void) {
    EthIf_Init(&testConfig);
    EthIf_DeInit();
    /* activeControllerCount back to 0 */
    TEST_ASSERT_EQUAL(ETHIF_CS_STOPPED, EthIf_GetControllerMode(0U));
}

/* ===== RxIndication + filtering ===== */

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_NullPdu_ShouldReturnSilently(void) {
    EthIf_Init(&testConfig);
    EthIf_RxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(0U, mock_RxCbCount);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_InvalidController_ShouldReturnSilently(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    EthIf_RxIndication(5U, &pdu);
    TEST_ASSERT_EQUAL(0U, mock_RxCbCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount); /* no DET for invalid ctrl in RxIndication */
}

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_EtherTypeMatch_ShouldCallCallback(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    pdu.EtherType = 0x88B5U;
    EthIf_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL(1U, mock_RxCbCount);
    TEST_ASSERT_EQUAL(0U, mock_RxCbLastCtrl);
    TEST_ASSERT_EQUAL(0x88B5U, mock_RxCbLastPdu.EtherType);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_MacMatch_ShouldCallCallback(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    (void)memset(&pdu, 0, sizeof(pdu));
    pdu.EtherType = 0x1234U; /* only MAC filter matches */
    pdu.MacAddress[0] = 0x01; pdu.MacAddress[1] = 0x02; pdu.MacAddress[2] = 0x03;
    pdu.MacAddress[3] = 0x04; pdu.MacAddress[4] = 0x05; pdu.MacAddress[5] = 0x06;
    EthIf_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL(1U, mock_RxCbCount);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_NoMatch_ShouldNotCallCallback(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    pdu.EtherType = 0x1234U; /* no filter matches */
    EthIf_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL(0U, mock_RxCbCount);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_WrongController_ShouldNotMatch(void) {
    EthIf_Init(&testConfig);
    EthIf_PduType pdu;
    pdu.EtherType = 0x88B5U; /* matches filter type, but filter is on Ctrl 0 */
    /* Controller 0 has valid active count. But there's no Ctrl 1 — so RxIndication silently returns */
    TEST_ASSERT_EQUAL(E_NOT_OK, EthIf_SetControllerMode(1U, ETHIF_CS_STARTED));
    /* Controller 1 not active -> RxIndication returns without filter */
    EthIf_RxIndication(1U, &pdu);
    TEST_ASSERT_EQUAL(0U, mock_RxCbCount);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_RxIndication_NullConfigPtr_ShouldReturnSilently(void) {
    /* emptyConfig has no filters (RxFilters = NULL_PTR) */
    EthIf_Init(&emptyConfig);
    EthIf_PduType pdu;
    pdu.EtherType = 0x88B5U;
    EthIf_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL(0U, mock_RxCbCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* ===== TxConfirmation / MainFunction ===== */

/** @req SWS_EthIf_00007 */
void test_EthIf_TxConfirmation_ShouldBeSilent(void) {
    EthIf_Init(&testConfig);
    /* No state changes expected */
    EthIf_TxConfirmation(0U, 0U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(0U, mock_RxCbCount);
}

/** @req SWS_EthIf_00008 */
void test_EthIf_MainFunction_AfterInit_ShouldReturnSilently(void) {
    EthIf_Init(&testConfig);
    EthIf_MainFunction();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_EthIf_00008 */
void test_EthIf_MainFunction_BeforeInit_ShouldReturnSilently(void) {
    EthIf_MainFunction();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* ===== GetVersionInfo ===== */

/** @req SWS_EthIf_00009 */
void test_EthIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    (void)memset(&info, 0xFF, sizeof(info));
    EthIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(ETHIF_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(ETHIF_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(1U, info.sw_major_version);
}

/** @req SWS_EthIf_00009 */
void test_EthIf_GetVersionInfo_NullPtr_ShouldReturnSilently(void) {
    EthIf_GetVersionInfo(NULL_PTR);
    /* EthIf.c checks NULL_PTR and returns without DET */
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* ===== TxConfirmation/Transmit with BufferHandle edge ===== */

/** @req SWS_EthIf_00007 */
void test_EthIf_TxConfirmation_BeforeInit_ShouldBeSilent(void) {
    EthIf_TxConfirmation(99U, 0xDEADBEEFU);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_EthIf_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_EthIf_Init_ValidConfig_ShouldEnterInitState);
    RUN_TEST(test_EthIf_Init_DoubleInit_ShouldReinit);
    RUN_TEST(test_EthIf_DeInit_ShouldResetState);
    RUN_TEST(test_EthIf_Init_OversizedNumControllers_ShouldAbortInit);
    RUN_TEST(test_EthIf_Init_ZeroControllers_ShouldSucceed);

    RUN_TEST(test_EthIf_Transmit_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_EthIf_Transmit_NullPdu_ShouldReportDet);
    RUN_TEST(test_EthIf_Transmit_InvalidController_ShouldReportDet);
    RUN_TEST(test_EthIf_Transmit_StoppedMode_ShouldReturnNotOk);
    RUN_TEST(test_EthIf_Transmit_StartedMode_ShouldReturnOk);
    RUN_TEST(test_EthIf_Transmit_SleepMode_ShouldReturnNotOk);

    RUN_TEST(test_EthIf_SetControllerMode_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_EthIf_SetControllerMode_InvalidController_ShouldReportDet);
    RUN_TEST(test_EthIf_SetControllerMode_Valid_ShouldStoreAndReflect);
    RUN_TEST(test_EthIf_GetControllerMode_InvalidController_ShouldReturnStopped);
    RUN_TEST(test_EthIf_GetControllerMode_BeforeInit_ShouldReturnStopped);

    RUN_TEST(test_EthIf_RxIndication_NullPdu_ShouldReturnSilently);
    RUN_TEST(test_EthIf_RxIndication_InvalidController_ShouldReturnSilently);
    RUN_TEST(test_EthIf_RxIndication_EtherTypeMatch_ShouldCallCallback);
    RUN_TEST(test_EthIf_RxIndication_MacMatch_ShouldCallCallback);
    RUN_TEST(test_EthIf_RxIndication_NoMatch_ShouldNotCallCallback);
    RUN_TEST(test_EthIf_RxIndication_WrongController_ShouldNotMatch);
    RUN_TEST(test_EthIf_RxIndication_NullConfigPtr_ShouldReturnSilently);

    RUN_TEST(test_EthIf_TxConfirmation_ShouldBeSilent);
    RUN_TEST(test_EthIf_MainFunction_AfterInit_ShouldReturnSilently);
    RUN_TEST(test_EthIf_MainFunction_BeforeInit_ShouldReturnSilently);
    RUN_TEST(test_EthIf_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_EthIf_GetVersionInfo_NullPtr_ShouldReturnSilently);
    RUN_TEST(test_EthIf_TxConfirmation_BeforeInit_ShouldBeSilent);

    return UnityEnd();
}

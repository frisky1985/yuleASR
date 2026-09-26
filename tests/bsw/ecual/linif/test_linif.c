/**
 * @file test_linif.c
 * @brief LinIf (LIN Interface) Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/linif/src/LinIf.c  @tests src/bsw/ecual/linif/src/LinIf_Lcfg.c  @tests src/bsw/ecual/linif/include/LinIf.h

#include "unity.h"
#include "LinIf.h"

/* Det SID/error literals are private to LinIf.c (not exported in the header) */
#define LINIF_SID_INIT          (0x00U)
#define LINIF_SID_TRANSMIT      (0x02U)
#define LINIF_SID_SCHEDULE      (0x05U)
#define LINIF_E_PARAM_POINTER   (0x10U)
#define LINIF_E_UNINIT          (0x20U)

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

void setUp(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

void tearDown(void) {
}

/* NOTE: LinIf keeps static state across tests and the runner executes in
 * declaration order, so uninitialized-behavior tests come first. */

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_NullPtr_ShouldReportDet(void) {
    LinIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LINIF_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_BeforeInit_ShouldReportUninit(void) {
    PduInfoType pdu;
    uint8 data[4] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 4U; pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = LinIf_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(LINIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_SetSchedule_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = LinIf_SetSchedule(LINIF_Normal);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(LINIF_SID_SCHEDULE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_MainFunction_BeforeInit_ShouldReturnSilently(void) {
    LinIf_MainFunction(); /* must not crash, must not report */
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_ValidConfig_ShouldActivateModule(void) {
    PduInfoType pdu;
    uint8 data[4] = {0xAAU, 0xBBU, 0xCCU, 0xDDU};
    pdu.SduDataPtr = data; pdu.SduLength = 4U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    /* Module must be operational after Init: Transmit accepted */
    TEST_ASSERT_EQUAL(E_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_DoubleInit_ShouldStayOperational(void) {
    PduInfoType pdu;
    uint8 data[1] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 1U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_NullPdu_ShouldReportDet(void) {
    LinIf_Init(&LinIf_Config);
    Std_ReturnType ret = LinIf_Transmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(LINIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_SetSchedule_ValidSchedule_ShouldSucceed(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetSchedule(LINIF_Normal));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    /* Drive the scheduler over the real config (entries at 5 ms / 10 ms) */
    for (uint8 i = 0U; i < 12U; i++) {
        LinIf_MainFunction();
    }
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00005 */
void test_LinIf_RxIndication_ShouldNotCrash(void) {
    LinIf_PduType rxPdu;
    uint8 data[8] = {0U};
    rxPdu.Id = 0x3CU; rxPdu.Dlc = 8U; rxPdu.DataPtr = data;

    LinIf_Init(&LinIf_Config);
    LinIf_RxIndication(0U, &rxPdu);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_MainFunction_AfterInit_ShouldRunScheduleTables(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);
    for (uint8 i = 0U; i < 20U; i++) {
        LinIf_MainFunction();
    }
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00002 */
void test_LinIf_DeInit_AfterInit_ShouldDeactivateModule(void) {
    PduInfoType pdu;
    uint8 data[1] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 1U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    LinIf_DeInit();

    /* After DeInit the module is uninitialized again: Transmit must fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00007 */
void test_LinIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    LinIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(LINIF_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(LINIF_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(LINIF_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(LINIF_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(LINIF_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_LinIf_00007 */
void test_LinIf_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    LinIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_LinIf_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_LinIf_Transmit_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_LinIf_SetSchedule_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_LinIf_MainFunction_BeforeInit_ShouldReturnSilently);
    RUN_TEST(test_LinIf_Init_ValidConfig_ShouldActivateModule);
    RUN_TEST(test_LinIf_Init_DoubleInit_ShouldStayOperational);
    RUN_TEST(test_LinIf_Transmit_NullPdu_ShouldReportDet);
    RUN_TEST(test_LinIf_SetSchedule_ValidSchedule_ShouldSucceed);
    RUN_TEST(test_LinIf_RxIndication_ShouldNotCrash);
    RUN_TEST(test_LinIf_MainFunction_AfterInit_ShouldRunScheduleTables);
    RUN_TEST(test_LinIf_DeInit_AfterInit_ShouldDeactivateModule);
    RUN_TEST(test_LinIf_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_LinIf_GetVersionInfo_NullPtr_ShouldReportDet);

    return UnityEnd();
}

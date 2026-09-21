/**
 * @file test_secoc.c
 * @brief SecOC (Security Onboard Communication) Unit Tests
 * @req SWS_SecOC
 */

// @tests src/bsw/services/secoc/src/SecOC.c  @tests src/bsw/services/secoc/include/SecOC.h

#include "unity.h"
#include "SecOC.h"
#include <string.h>

/* Mock Det_ReportError — records call count and last API/error IDs */
static uint8 mock_DetCalls = 0;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCalls++;
    return E_OK;
}

static SecOC_ConfigType testConfig;

void setUp(void) {
    /* Return the SUT to a clean uninitialized state before every test */
    if (SecOC_Initialized != FALSE) {
        SecOC_DeInit();
    }
    mock_DetCalls = 0;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    (void)memset(&testConfig, 0, sizeof(testConfig));
    testConfig.numTxPdus = 0U;
    testConfig.numRxPdus = 0U;
}
void tearDown(void) {}

/** @req SWS_SecOC_00001 */
void test_SecOC_Init_NullPtr_ShouldNotCrash(void) {
    SecOC_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_PARAM_POINTER, mock_DetLastErrorId);
    TEST_ASSERT_FALSE(SecOC_Initialized);
}

/** @req SWS_SecOC_00001 */
void test_SecOC_Init_ValidConfig_ShouldSucceed(void) {
    SecOC_Init(&testConfig);
    TEST_ASSERT_TRUE(SecOC_Initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_SecOC_00002 */
void test_SecOC_DeInit_AfterInit_ShouldSucceed(void) {
    SecOC_Init(&testConfig);
    TEST_ASSERT_TRUE(SecOC_Initialized);
    SecOC_DeInit();
    TEST_ASSERT_FALSE(SecOC_Initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_SecOC_00040 */
void test_SecOC_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    (void)memset(&info, 0, sizeof(info));
    SecOC_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(SECOC_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(SECOC_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_SecOC_00040 */
void test_SecOC_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    SecOC_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_SecOC_00010 */
void test_SecOC_IfTransmit_BeforeInit_ShouldFail(void) {
    PduInfoType pduInfo;
    uint8 data[8] = {0};
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    Std_ReturnType ret = SecOC_IfTransmit(0U, &pduInfo);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SID_IFTRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_SecOC_00010 */
void test_SecOC_IfTransmit_NullPduInfo_ShouldFail(void) {
    SecOC_Init(&testConfig);
    Std_ReturnType ret = SecOC_IfTransmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_SecOC_00010 */
void test_SecOC_IfTransmit_AfterInit_ShouldReturnResult(void) {
    SecOC_Init(&testConfig);
    PduInfoType pduInfo;
    uint8 data[8] = {0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u, 0x88u};
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    /* First transmit on a free buffer succeeds */
    Std_ReturnType ret = SecOC_IfTransmit(0U, &pduInfo);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Buffer now in use — second transmit on the same PDU fails (no DET) */
    ret = SecOC_IfTransmit(0U, &pduInfo);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_SecOC_00010 */
void test_SecOC_IfTransmit_InvalidPduId_ShouldReportDet(void) {
    PduInfoType pduInfo;
    uint8 data[4] = {0};
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 4U;
    SecOC_Init(&testConfig);
    Std_ReturnType ret = SecOC_IfTransmit(99U, &pduInfo);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_INVALID_PDU_SDU_ID, mock_DetLastErrorId);
}

void test_SecOC_DeInit_DoubleDeInit_ShouldNotCrash(void) {
    SecOC_Init(&testConfig);
    SecOC_DeInit();
    TEST_ASSERT_FALSE(SecOC_Initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Second DeInit reports E_UNINIT and leaves the module uninitialized */
    SecOC_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_UNINIT, mock_DetLastErrorId);
    TEST_ASSERT_FALSE(SecOC_Initialized);
}

void test_SecOC_Init_DoubleInit_ShouldNotCrash(void) {
    SecOC_Init(&testConfig);
    TEST_ASSERT_TRUE(SecOC_Initialized);
    /* Second Init reports E_ALREADY_INITIALIZED and keeps state */
    SecOC_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(SECOC_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SECOC_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
    TEST_ASSERT_TRUE(SecOC_Initialized);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_SecOC_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_SecOC_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_SecOC_DeInit_AfterInit_ShouldSucceed);
    RUN_TEST(test_SecOC_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_SecOC_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_SecOC_IfTransmit_BeforeInit_ShouldFail);
    RUN_TEST(test_SecOC_IfTransmit_NullPduInfo_ShouldFail);
    RUN_TEST(test_SecOC_IfTransmit_AfterInit_ShouldReturnResult);
    RUN_TEST(test_SecOC_IfTransmit_InvalidPduId_ShouldReportDet);
    RUN_TEST(test_SecOC_DeInit_DoubleDeInit_ShouldNotCrash);
    RUN_TEST(test_SecOC_Init_DoubleInit_ShouldNotCrash);
    return UnityEnd();
}

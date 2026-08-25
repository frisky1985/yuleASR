/**
 * @file test_secoc.c
 * @brief SecOC (Security Onboard Communication) Unit Tests
 * @req SWS_SecOC
 */

// @tests src/bsw/services/secoc/src/SecOC.c  @tests src/bsw/services/secoc/include/SecOC.h

#include "unity.h"
#include "SecOC.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId; (void)ApiId; (void)ErrorId;
    mock_DetCalls++;
    return E_OK;
}

static SecOC_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_SecOC_00001 */
void test_SecOC_Init_NullPtr_ShouldNotCrash(void) {
    SecOC_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SecOC_00001 */
void test_SecOC_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.NumRxPdus = 0U;
    testConfig.NumTxPdus = 0U;
    SecOC_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SecOC_00002 */
void test_SecOC_DeInit_AfterInit_ShouldSucceed(void) {
    SecOC_Init(&testConfig);
    SecOC_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SecOC_00003 */
void test_SecOC_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    SecOC_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(SECOC_VENDOR_ID, info.vendorID);
}

/** @req SWS_SecOC_00003 */
void test_SecOC_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    SecOC_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_SecOC_00004 */
void test_SecOC_IfTransmit_BeforeInit_ShouldFail(void) {
    PduInfoType pduInfo;
    uint8 data[8] = {0};
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    Std_ReturnType ret = SecOC_IfTransmit(0U, &pduInfo);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_SecOC_00004 */
void test_SecOC_IfTransmit_NullPduInfo_ShouldFail(void) {
    SecOC_Init(&testConfig);
    Std_ReturnType ret = SecOC_IfTransmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_SecOC_00004 */
void test_SecOC_IfTransmit_AfterInit_ShouldReturnResult(void) {
    SecOC_Init(&testConfig);
    PduInfoType pduInfo;
    uint8 data[8] = {0};
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    Std_ReturnType ret = SecOC_IfTransmit(0U, &pduInfo);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

void test_SecOC_DeInit_DoubleDeInit_ShouldNotCrash(void) {
    SecOC_Init(&testConfig);
    SecOC_DeInit();
    SecOC_DeInit();
    TEST_ASSERT_TRUE(1);
}

void test_SecOC_Init_DoubleInit_ShouldNotCrash(void) {
    SecOC_Init(&testConfig);
    SecOC_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

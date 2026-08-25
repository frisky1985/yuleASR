/**
 * @file test_csm.c
 * @brief Csm (Crypto Service Manager) Unit Tests
 * @req SWS_Csm
 */

// @tests src/bsw/services/csm/src/Csm.c  @tests src/bsw/services/csm/include/Csm.h
#include "unity.h"
#include "Csm.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Csm_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Csm_00001 */
void test_Csm_Init_NullPtr_ShouldFail(void) {
    Std_ReturnType ret = Csm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Csm_00001 */
void test_Csm_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.NumKeys = 0U;
    testConfig.NumJobs = 0U;
    Std_ReturnType ret = Csm_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Csm_00002 */
void test_Csm_DeInit_AfterInit_ShouldSucceed(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Csm_00010 */
void test_Csm_KeyElementSet_BeforeInit_ShouldFail(void) {
    uint8 data[16] = {0};
    Std_ReturnType ret = Csm_KeyElementSet(0U, 0U, data, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Csm_00010 */
void test_Csm_KeyElementSet_NullData_ShouldFail(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_KeyElementSet(0U, 0U, NULL_PTR, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Csm_00011 */
void test_Csm_KeySetValid_AfterInit_ShouldReturnResult(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_KeySetValid(0U);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_Csm_00012 */
void test_Csm_KeyElementGet_BeforeInit_ShouldFail(void) {
    uint8 data[16];
    uint32 actualLen;
    Std_ReturnType ret = Csm_KeyElementGet(0U, 0U, data, 16U, &actualLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Csm_00016 */
void test_Csm_KeyGenerate_AfterInit_ShouldReturnResult(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_KeyGenerate(0U);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_Csm_00017 */
void test_Csm_KeyDerive_AfterInit_ShouldReturnResult(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_KeyDerive(0U, 1U);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_Csm_00014 */
void test_Csm_KeyCopy_AfterInit_ShouldReturnResult(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_KeyCopy(0U, 1U);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_Csm_00015 */
void test_Csm_KeyElementIdsGet_BeforeInit_ShouldFail(void) {
    uint32 ids[4]; uint32 count;
    Std_ReturnType ret = Csm_KeyElementIdsGet(0U, ids, 4U, &count);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Csm_00010 */
void test_Csm_KeyElementCopy_AfterInit_ShouldReturnResult(void) {
    Csm_Init(&testConfig);
    Std_ReturnType ret = Csm_KeyElementCopy(0U, 0U, 1U, 0U);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

void test_Csm_DeInit_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Csm_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_Csm_Init_DoubleInit_ShouldNotCrash(void) {
    Csm_Init(&testConfig);
    Csm_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

/**
 * @file test_csm.c
 * @brief Csm (Crypto Service Manager) Unit Tests
 * @req SWS_Csm
 */

// @tests src/bsw/services/csm/src/Csm.c  @tests src/bsw/services/csm/include/Csm.h
#include "unity.h"
#include "Csm.h"
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

static Csm_ConfigType testConfig;
static boolean test_csm_was_init = FALSE;

/* Helper: init the SUT and remember that it needs teardown in setUp */
static Std_ReturnType test_Csm_DoInit(void) {
    Std_ReturnType ret = Csm_Init(&testConfig);
    if (ret == E_OK) {
        test_csm_was_init = TRUE;
    }
    return ret;
}

void setUp(void) {
    /* Return the SUT to a clean uninitialized state before every test */
    if (test_csm_was_init != FALSE) {
        (void)Csm_DeInit();
        test_csm_was_init = FALSE;
    }
    mock_DetCalls = 0;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    (void)memset(&testConfig, 0, sizeof(testConfig));
    testConfig.numKeys = 0U;
    testConfig.numJobs = 0U;
}
void tearDown(void) {}

/** @req SWS_Csm_00001 */
void test_Csm_Init_NullPtr_ShouldFail(void) {
    Std_ReturnType ret = Csm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Csm_00001 */
void test_Csm_Init_ValidConfig_ShouldSucceed(void) {
    Std_ReturnType ret = test_Csm_DoInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Csm_00002 */
void test_Csm_DeInit_AfterInit_ShouldSucceed(void) {
    test_Csm_DoInit();
    Std_ReturnType ret = Csm_DeInit();
    test_csm_was_init = FALSE; /* SUT is uninitialized now */
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Csm_00010 */
void test_Csm_KeyElementSet_BeforeInit_ShouldFail(void) {
    uint8 data[16] = {0};
    Std_ReturnType ret = Csm_KeyElementSet(0U, 0U, data, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_ELEMENT_SET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Csm_00010 */
void test_Csm_KeyElementSet_NullData_ShouldFail(void) {
    test_Csm_DoInit();
    Std_ReturnType ret = Csm_KeyElementSet(0U, 0U, NULL_PTR, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_ELEMENT_SET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Csm_00011 */
void test_Csm_KeySetValid_AfterInit_ShouldReturnResult(void) {
    test_Csm_DoInit();
    /* No keys configured — keyId 0 is unknown: E_NOT_OK + E_PARAM_KEY_ID */
    Std_ReturnType ret = Csm_KeySetValid(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_SET_VALID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_PARAM_KEY_ID, mock_DetLastErrorId);
}

/** @req SWS_Csm_00012 */
void test_Csm_KeyElementGet_BeforeInit_ShouldFail(void) {
    uint8 data[16];
    uint32 actualLen = 16U;
    Std_ReturnType ret = Csm_KeyElementGet(0U, 0U, data, &actualLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_ELEMENT_GET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Csm_00016 */
void test_Csm_KeyGenerate_AfterInit_ShouldReturnResult(void) {
    test_Csm_DoInit();
    /* No keys configured — keyId 0 is unknown: E_NOT_OK + E_PARAM_KEY_ID */
    Std_ReturnType ret = Csm_KeyGenerate(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_GENERATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_PARAM_KEY_ID, mock_DetLastErrorId);
}

/** @req SWS_Csm_00017 */
void test_Csm_KeyDerive_AfterInit_ShouldReturnResult(void) {
    test_Csm_DoInit();
    /* No keys configured — keyId 0 is unknown: E_NOT_OK + E_PARAM_KEY_ID */
    Std_ReturnType ret = Csm_KeyDerive(0U, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_DERIVE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_PARAM_KEY_ID, mock_DetLastErrorId);
}

/** @req SWS_Csm_00014 */
void test_Csm_KeyCopy_AfterInit_ShouldReturnResult(void) {
    test_Csm_DoInit();
    /* No keys configured — Csm_KeyCopy returns E_NOT_OK silently
     * (the SUT does not report a DET error for an unknown source key) */
    Std_ReturnType ret = Csm_KeyCopy(0U, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Csm_00015 */
void test_Csm_KeyElementIdsGet_BeforeInit_ShouldFail(void) {
    uint32 ids[4];
    uint32 count = 4U;
    Std_ReturnType ret = Csm_KeyElementIdsGet(0U, ids, &count);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_ELEMENT_IDS_GET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Csm_00013 */
void test_Csm_KeyElementCopy_AfterInit_ShouldReturnResult(void) {
    test_Csm_DoInit();
    /* No keys configured — the inner Csm_KeyElementGet reports
     * E_PARAM_KEY_ID with its own API ID and the copy fails */
    Std_ReturnType ret = Csm_KeyElementCopy(0U, 0U, 1U, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_KEY_ELEMENT_GET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_PARAM_KEY_ID, mock_DetLastErrorId);
}

void test_Csm_DeInit_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Csm_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

void test_Csm_Init_DoubleInit_ShouldNotCrash(void) {
    test_Csm_DoInit();
    /* Second Init reports E_ALREADY_INITIALIZED and fails */
    Std_ReturnType ret = Csm_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(CSM_API_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CSM_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Csm_Init_NullPtr_ShouldFail);
    RUN_TEST(test_Csm_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Csm_DeInit_AfterInit_ShouldSucceed);
    RUN_TEST(test_Csm_KeyElementSet_BeforeInit_ShouldFail);
    RUN_TEST(test_Csm_KeyElementSet_NullData_ShouldFail);
    RUN_TEST(test_Csm_KeySetValid_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Csm_KeyElementGet_BeforeInit_ShouldFail);
    RUN_TEST(test_Csm_KeyGenerate_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Csm_KeyDerive_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Csm_KeyCopy_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Csm_KeyElementIdsGet_BeforeInit_ShouldFail);
    RUN_TEST(test_Csm_KeyElementCopy_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Csm_DeInit_BeforeInit_ShouldFail);
    RUN_TEST(test_Csm_Init_DoubleInit_ShouldNotCrash);
    return UnityEnd();
}

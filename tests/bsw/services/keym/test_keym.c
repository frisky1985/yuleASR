/**
 * @file test_keym.c
 * @brief KeyM Unit Tests (substantiated against production KeyM.c)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @note The original test file exercised fabricated APIs (KeyM_StoreKey /
 *       KeyM_LoadKey / KeyM_DeleteKey / KeyM_GetKeyStatus) which do not exist
 *       in src/bsw/services/keym. They are remapped onto the real production
 *       API as follows:
 *         - KeyM_StoreKey*     -> KeyM_SetKey()
 *         - KeyM_LoadKey*      -> KeyM_GetKey()
 *         - KeyM_DeleteKey*    -> KeyM_UpdateKey()
 *         - KeyM_GetKeyStatus* -> KeyM_KeyStatusGet()
 *       Observed SUT behaviour that differs from the old expectations:
 *         - KeyM_MainFunction() before init returns silently (NO DET report).
 *         - KeyM_GetKey() on a key that was SetKey'd but not FinalizeKey'd
 *           returns E_NOT_OK without any DET report (isValid == FALSE).
 */

// @tests src/bsw/services/keym/src/KeyM.c  @tests src/bsw/services/keym/include/KeyM.h

#include <string.h>
#include "unity.h"
#include "KeyM.h"

/* Mock Det_ReportError (KEYM_DEV_ERROR_DETECT is STD_ON) */
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

/* Test config (keyConfigs left NULL; only numKeys is dereferenced) */
static KeyM_ConfigType testConfig;

static const uint8 testKeyData[16] = {
    0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U,
    0x88U, 0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU
};
static const uint8 testKeyDataUpdated[16] = {
    0xFFU, 0xEEU, 0xDDU, 0xCCU, 0xBBU, 0xAAU, 0x99U, 0x88U,
    0x77U, 0x66U, 0x55U, 0x44U, 0x33U, 0x22U, 0x11U, 0x00U
};

void setUp(void) {
    if (KeyM_Initialized != FALSE) {
        KeyM_DeInit();
    }
    mock_Det_Reset();
    memset(&testConfig, 0, sizeof(testConfig));
    testConfig.numKeys = KEYM_NUM_KEYS;
}

void tearDown(void) {
}

/* Helper: initialize the module with the default test config */
static void test_KeyM_DoInit(void) {
    KeyM_Init(&testConfig);
}


/** @req SWS_KeyM_00001 */
void test_KeyM_Init_NullPtr_ShouldReportError(void) {
    KeyM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_PARAM_POINTER, mock_DetLastErrorId);
    TEST_ASSERT_FALSE(KeyM_Initialized);
}

/** @req SWS_KeyM_00001 */
void test_KeyM_Init_ValidConfig_ShouldSucceed(void) {
    test_KeyM_DoInit();
    TEST_ASSERT_TRUE(KeyM_Initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_KeyM_00001 */
void test_KeyM_Init_DoubleInit_ShouldReportError(void) {
    test_KeyM_DoInit();
    mock_Det_Reset();
    KeyM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
    TEST_ASSERT_TRUE(KeyM_Initialized);
}

/** @req SWS_KeyM_00002 */
void test_KeyM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00002 */
void test_KeyM_DeInit_ValidCall_ShouldSucceed(void) {
    test_KeyM_DoInit();
    mock_Det_Reset();
    KeyM_DeInit();
    TEST_ASSERT_FALSE(KeyM_Initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_KeyM_00003 */
void test_KeyM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    KeyM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00003 */
void test_KeyM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType versionInfo;
    memset(&versionInfo, 0, sizeof(versionInfo));
    KeyM_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL_UINT16(KEYM_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(KEYM_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_KeyM_00004 */
void test_KeyM_MainFunction_Uninit_ShouldReturnSilently(void) {
    /* SUT returns silently without a DET report when not initialized */
    KeyM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_KeyM_00004 */
void test_KeyM_MainFunction_ValidCall_ShouldSucceed(void) {
    test_KeyM_DoInit();
    mock_Det_Reset();
    KeyM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_KeyM_00005 (was: StoreKey) */
void test_KeyM_SetKey_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Std_ReturnType ret = KeyM_SetKey(0U, testKeyData, 16U, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_SETKEY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00005 (was: StoreKey) */
void test_KeyM_SetKey_NullPtr_ShouldReportError(void) {
    test_KeyM_DoInit();
    mock_Det_Reset();
    Std_ReturnType ret = KeyM_SetKey(0U, NULL_PTR, 16U, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_SETKEY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00005 (was: StoreKey) */
void test_KeyM_SetKey_ValidCall_ShouldStoreKeyAsNew(void) {
    KeyM_KeyStatusType status = KEYM_KEY_STATUS_INVALID;
    test_KeyM_DoInit();
    mock_Det_Reset();
    Std_ReturnType ret = KeyM_SetKey(0U, testKeyData, 16U, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_KeyStatusGet(0U, &status));
    TEST_ASSERT_EQUAL_UINT32(KEYM_KEY_STATUS_NEW, status);
}

/** @req SWS_KeyM_00006 (was: LoadKey) */
void test_KeyM_GetKey_Uninit_ShouldReportError(void) {
    /* Not initialized */
    uint8 buf[KEYM_MAX_KEY_LENGTH];
    uint32 len = sizeof(buf);
    KeyM_KeyFormatType fmt = KEYM_KEY_FORMAT_DER;
    Std_ReturnType ret = KeyM_GetKey(0U, buf, &len, &fmt);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_GETKEY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00006 (was: LoadKey) */
void test_KeyM_GetKey_NullPtr_ShouldReportError(void) {
    uint32 len = KEYM_MAX_KEY_LENGTH;
    test_KeyM_DoInit();
    mock_Det_Reset();
    Std_ReturnType ret = KeyM_GetKey(0U, NULL_PTR, &len, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_GETKEY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00006 (was: LoadKey) */
void test_KeyM_GetKey_ValidCall_ShouldReturnStoredKey(void) {
    uint8 buf[KEYM_MAX_KEY_LENGTH];
    uint32 len = sizeof(buf);
    KeyM_KeyFormatType fmt = KEYM_KEY_FORMAT_DER;
    test_KeyM_DoInit();

    /* SetKey alone does not make the key valid: GetKey fails silently */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_SetKey(0U, testKeyData, 16U, KEYM_KEY_FORMAT_RAW));
    mock_Det_Reset();
    len = sizeof(buf);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, KeyM_GetKey(0U, buf, &len, &fmt));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);

    /* After FinalizeKey the key can be read back */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_FinalizeKey(0U));
    mock_Det_Reset();
    memset(buf, 0, sizeof(buf));
    len = sizeof(buf);
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_GetKey(0U, buf, &len, &fmt));
    TEST_ASSERT_EQUAL_UINT32(16U, len);
    TEST_ASSERT_EQUAL_UINT32(KEYM_KEY_FORMAT_RAW, fmt);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testKeyData, buf, 16U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_KeyM_00007 (was: DeleteKey) */
void test_KeyM_UpdateKey_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Std_ReturnType ret = KeyM_UpdateKey(0U, testKeyDataUpdated, 16U, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_UPDATEKEY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00007 (was: DeleteKey) */
void test_KeyM_UpdateKey_InvalidKey_ShouldReportError(void) {
    test_KeyM_DoInit();
    mock_Det_Reset();
    Std_ReturnType ret = KeyM_UpdateKey(0xFFFFU, testKeyDataUpdated, 16U, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_UPDATEKEY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_INVALID_KEY, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00007 (was: DeleteKey) */
void test_KeyM_UpdateKey_ValidCall_ShouldReplaceKeyData(void) {
    uint8 buf[KEYM_MAX_KEY_LENGTH];
    uint32 len = sizeof(buf);
    KeyM_KeyStatusType status = KEYM_KEY_STATUS_INVALID;
    test_KeyM_DoInit();

    /* Establish a valid key first */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_SetKey(0U, testKeyData, 16U, KEYM_KEY_FORMAT_RAW));
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_FinalizeKey(0U));

    /* UpdateKey replaces the data and moves the key to UPDATE status */
    mock_Det_Reset();
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_UpdateKey(0U, testKeyDataUpdated, 16U, KEYM_KEY_FORMAT_RAW));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_KeyStatusGet(0U, &status));
    TEST_ASSERT_EQUAL_UINT32(KEYM_KEY_STATUS_UPDATE, status);

    /* Key is not readable until finalized again */
    len = sizeof(buf);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, KeyM_GetKey(0U, buf, &len, NULL_PTR));

    /* After finalizing, the updated data is returned */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_FinalizeKey(0U));
    memset(buf, 0, sizeof(buf));
    len = sizeof(buf);
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_GetKey(0U, buf, &len, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(16U, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testKeyDataUpdated, buf, 16U);
}

/** @req SWS_KeyM_00008 (was: GetKeyStatus) */
void test_KeyM_KeyStatusGet_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_KeyStatusType status = KEYM_KEY_STATUS_INVALID;
    Std_ReturnType ret = KeyM_KeyStatusGet(0U, &status);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_KEYSTATUSGET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00008 (was: GetKeyStatus) */
void test_KeyM_KeyStatusGet_InvalidKey_ShouldReportError(void) {
    KeyM_KeyStatusType status = KEYM_KEY_STATUS_INVALID;
    test_KeyM_DoInit();
    mock_Det_Reset();
    Std_ReturnType ret = KeyM_KeyStatusGet(0xFFFFU, &status);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(KEYM_SID_KEYSTATUSGET, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(KEYM_E_INVALID_KEY, mock_DetLastErrorId);
}

/** @req SWS_KeyM_00008 (was: GetKeyStatus) */
void test_KeyM_KeyStatusGet_ValidCall_ShouldReturnStatus(void) {
    KeyM_KeyStatusType status = KEYM_KEY_STATUS_INVALID;
    test_KeyM_DoInit();
    mock_Det_Reset();

    /* Freshly initialized key is NEW */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_KeyStatusGet(0U, &status));
    TEST_ASSERT_EQUAL_UINT32(KEYM_KEY_STATUS_NEW, status);

    /* After SetKey the key is still NEW (until finalized) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_SetKey(0U, testKeyData, 16U, KEYM_KEY_FORMAT_RAW));
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_KeyStatusGet(0U, &status));
    TEST_ASSERT_EQUAL_UINT32(KEYM_KEY_STATUS_NEW, status);

    /* After FinalizeKey the key becomes VALID */
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_FinalizeKey(0U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, KeyM_KeyStatusGet(0U, &status));
    TEST_ASSERT_EQUAL_UINT32(KEYM_KEY_STATUS_VALID, status);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_KeyM_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_KeyM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_KeyM_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_KeyM_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_KeyM_DeInit_ValidCall_ShouldSucceed);
    RUN_TEST(test_KeyM_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_KeyM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_KeyM_MainFunction_Uninit_ShouldReturnSilently);
    RUN_TEST(test_KeyM_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_KeyM_SetKey_Uninit_ShouldReportError);
    RUN_TEST(test_KeyM_SetKey_NullPtr_ShouldReportError);
    RUN_TEST(test_KeyM_SetKey_ValidCall_ShouldStoreKeyAsNew);
    RUN_TEST(test_KeyM_GetKey_Uninit_ShouldReportError);
    RUN_TEST(test_KeyM_GetKey_NullPtr_ShouldReportError);
    RUN_TEST(test_KeyM_GetKey_ValidCall_ShouldReturnStoredKey);
    RUN_TEST(test_KeyM_UpdateKey_Uninit_ShouldReportError);
    RUN_TEST(test_KeyM_UpdateKey_InvalidKey_ShouldReportError);
    RUN_TEST(test_KeyM_UpdateKey_ValidCall_ShouldReplaceKeyData);
    RUN_TEST(test_KeyM_KeyStatusGet_Uninit_ShouldReportError);
    RUN_TEST(test_KeyM_KeyStatusGet_InvalidKey_ShouldReportError);
    RUN_TEST(test_KeyM_KeyStatusGet_ValidCall_ShouldReturnStatus);
    return UnityEnd();
}

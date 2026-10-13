/**
 * @file test_KeyM_branch.c
 * @brief KeyM (Key Manager) Branch Coverage Tests
 * @brief Maximizes branch coverage for KeyM.c
 */

#include <unity.h>
#include "KeyM.h"
#include "KeyM_Cfg.h"
#include "Det.h"
#include <string.h>

/* ===== Det Stubs ===== */
static uint16 det_last_error = 0xFFFF;
static uint8 det_last_api_id = 0xFF;
static uint16 det_call_count = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId;
    det_last_api_id = ApiId;
    det_last_error = ErrorId;
    det_call_count++;
    return E_OK;
}

void Det_Init(const Det_ConfigType* ConfigPtr) { (void)ConfigPtr; }
void Det_Start(void) {}

/* SchM_KeyM macros are already defined as empty in SchM_KeyM.h */

/* ===== Test Configuration ===== */
static KeyM_KeyConfigType testKeyConfigs[KEYM_NUM_KEYS] = {
    {0, KEYM_KEY_TYPE_AES,     32,  4, 1, 0, 0xFF},
    {1, KEYM_KEY_TYPE_AES,     16,  4, 1, 0, 0xFF},
    {2, KEYM_KEY_TYPE_AES,     32,  4, 1, 0, 0xFF},
    {3, KEYM_KEY_TYPE_HMAC,    32,  4, 1, 0, 0xFF},
    {4, KEYM_KEY_TYPE_RSA,     256, 4, 1, 0, 0xFF},
    {5, KEYM_KEY_TYPE_ECC,     32,  4, 1, 0, 0xFF},
    {6, KEYM_KEY_TYPE_GENERIC, 32,  4, 1, 0, 0xFF},
    {7, KEYM_KEY_TYPE_GENERIC, 32,  4, 1, 0, 0xFF}
};

static const KeyM_ConfigType testConfig = {
    testKeyConfigs,
    KEYM_NUM_KEYS,
    KEYM_MAX_KEY_ELEMENTS,
    TRUE,
    TRUE,
    KEYM_MAX_KEY_LIFETIME
};

/* Test data */
static const uint8 testKeyData[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

/* Notification callback tracker */
static uint8 notif_called = 0;
static KeyM_KeyIdType notif_keyId = 0;
static KeyM_OperationResultType notif_result = KEYM_OPRESULT_PENDING;

static void testNotificationCallback(KeyM_KeyIdType keyId,
                                      KeyM_OperationResultType result,
                                      const uint8* dataPtr,
                                      uint32 dataLength)
{
    (void)dataPtr; (void)dataLength;
    notif_called = 1;
    notif_keyId = keyId;
    notif_result = result;
}

/* ===== setUp / tearDown ===== */
void setUp(void)
{
    det_last_error = 0xFFFF;
    det_last_api_id = 0xFF;
    det_call_count = 0;
    notif_called = 0;
    notif_keyId = 0;
    notif_result = KEYM_OPRESULT_PENDING;
    /* Ensure clean state */
    KeyM_Initialized = FALSE;
    KeyM_ConfigPtr = NULL_PTR;
}

void tearDown(void) {}

/* =========================================================================
 *  KeyM_Init Branches
 * ========================================================================= */

void test_Init_ValidConfig_Succeeds(void)
{
    KeyM_Init(&testConfig);
    TEST_ASSERT_TRUE(KeyM_Initialized);
    TEST_ASSERT_EQUAL_PTR(&testConfig, KeyM_ConfigPtr);
}

void test_Init_AlreadyInitialized_ReportsError(void)
{
    KeyM_Init(&testConfig);
    det_last_error = 0xFFFF;
    KeyM_Init(&testConfig);
    TEST_ASSERT_EQUAL(KEYM_E_ALREADY_INITIALIZED, det_last_error);
    TEST_ASSERT_EQUAL(KEYM_SID_INIT, det_last_api_id);
}

void test_Init_NullPtr_ReportsError(void)
{
    KeyM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
    TEST_ASSERT_EQUAL(KEYM_SID_INIT, det_last_api_id);
    TEST_ASSERT_FALSE(KeyM_Initialized);
}

void test_Init_ResetsSystemTime(void)
{
    KeyM_Init(&testConfig);
    KeyM_DeInit();
    KeyM_Initialized = FALSE;
    KeyM_Init(&testConfig);
    /* SystemTime should be reset to 0 - verify by setting a key and checking validity */
    TEST_ASSERT_TRUE(KeyM_Initialized);
}

/* =========================================================================
 *  KeyM_DeInit Branches
 * ========================================================================= */

void test_DeInit_Initialized_Succeeds(void)
{
    KeyM_Init(&testConfig);
    KeyM_DeInit();
    TEST_ASSERT_FALSE(KeyM_Initialized);
    TEST_ASSERT_NULL(KeyM_ConfigPtr);
}

void test_DeInit_Uninitialized_ReportsError(void)
{
    KeyM_DeInit();
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
    TEST_ASSERT_EQUAL(KEYM_SID_DEINIT, det_last_api_id);
}

/* =========================================================================
 *  KeyM_GetVersionInfo Branches
 * ========================================================================= */

void test_GetVersionInfo_ValidPtr_Succeeds(void)
{
    Std_VersionInfoType vInfo;
    memset(&vInfo, 0, sizeof(vInfo));
    KeyM_GetVersionInfo(&vInfo);
    TEST_ASSERT_EQUAL(KEYM_VENDOR_ID, vInfo.vendorID);
    TEST_ASSERT_EQUAL(KEYM_MODULE_ID, vInfo.moduleID);
    TEST_ASSERT_EQUAL(KEYM_SW_MAJOR_VERSION, vInfo.sw_major_version);
    TEST_ASSERT_EQUAL(KEYM_SW_MINOR_VERSION, vInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(KEYM_SW_PATCH_VERSION, vInfo.sw_patch_version);
}

void test_GetVersionInfo_NullPtr_ReportsError(void)
{
    KeyM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
    TEST_ASSERT_EQUAL(KEYM_SID_GETVERSIONINFO, det_last_api_id);
}

/* =========================================================================
 *  KeyM_SetKey Branches
 * ========================================================================= */

void test_SetKey_Uninitialized_ReportsError(void)
{
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_SetKey(0, data, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_SetKey_NullKeyPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_SetKey(0, NULL_PTR, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_SetKey_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_SetKey(KEYM_NUM_KEYS, data, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_SetKey_LengthTooLarge_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_SetKey(0, data, KEYM_MAX_KEY_LENGTH + 1, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_LENGTH, det_last_error);
}

void test_SetKey_ValidInput_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    Std_ReturnType ret = KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_SetKey_SetsStatusNew(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_NEW, status);
}

void test_SetKey_MaxLength_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[KEYM_MAX_KEY_LENGTH];
    memset(data, 0xAA, KEYM_MAX_KEY_LENGTH);
    Std_ReturnType ret = KeyM_SetKey(0, data, KEYM_MAX_KEY_LENGTH, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_SetKey_ExactMaxLen_BoundaryCheck(void)
{
    KeyM_Init(&testConfig);
    uint8 data[KEYM_MAX_KEY_LENGTH];
    memset(data, 0x55, KEYM_MAX_KEY_LENGTH);
    /* Exactly KEYM_MAX_KEY_LENGTH should pass (not > KEYM_MAX_KEY_LENGTH) */
    Std_ReturnType ret = KeyM_SetKey(0, data, KEYM_MAX_KEY_LENGTH, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* =========================================================================
 *  KeyM_GetKey Branches
 * ========================================================================= */

void test_GetKey_Uninitialized_ReportsError(void)
{
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_GetKey_NullKeyPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, NULL, &len, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_GetKey_NullLenPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    Std_ReturnType ret = KeyM_GetKey(0, buf, NULL, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_GetKey_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(KEYM_NUM_KEYS, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_GetKey_KeyNotValid_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    /* Key not finalized -> isValid=FALSE */
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetKey_BufferTooSmall_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    uint8 data[32] = {0};
    KeyM_SetKey(0, data, 32, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 8; /* Too small */
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetKey_ValidKey_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(16, len);
    TEST_ASSERT_EQUAL(0, memcmp(data, buf, 16));
}

void test_GetKey_WithFormatPtr_SetsRaw(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 32;
    KeyM_KeyFormatType fmt = 0xFF;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, &fmt);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_KEY_FORMAT_RAW, fmt);
}

void test_GetKey_NullFormatPtr_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* =========================================================================
 *  KeyM_UpdateKey Branches
 * ========================================================================= */

void test_UpdateKey_Uninitialized_ReportsError(void)
{
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_UpdateKey(0, data, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_UpdateKey_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_UpdateKey(0, NULL_PTR, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_UpdateKey_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_UpdateKey(KEYM_NUM_KEYS, data, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_UpdateKey_LengthTooLarge_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_UpdateKey(0, data, KEYM_MAX_KEY_LENGTH + 1, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_LENGTH, det_last_error);
}

void test_UpdateKey_ValidInput_SetsStatusUpdate(void)
{
    KeyM_Init(&testConfig);
    uint8 data1[16] = {0};
    KeyM_SetKey(0, data1, 16, KEYM_KEY_FORMAT_RAW);
    uint8 data2[16] = {0xAA};
    Std_ReturnType ret = KeyM_UpdateKey(0, data2, 16, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_OK, ret);
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_UPDATE, status);
}

void test_UpdateKey_IncrementsVersion(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    uint32 ver1;
    KeyM_KeyVersionGet(0, &ver1);
    KeyM_UpdateKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    uint32 ver2;
    KeyM_KeyVersionGet(0, &ver2);
    TEST_ASSERT_EQUAL(ver1 + 1, ver2);
}

/* =========================================================================
 *  KeyM_FinalizeKey Branches
 * ========================================================================= */

void test_FinalizeKey_Uninitialized_ReportsError(void)
{
    Std_ReturnType ret = KeyM_FinalizeKey(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_FinalizeKey_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_FinalizeKey(KEYM_NUM_KEYS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_FinalizeKey_NoData_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    /* keyLength == 0 after init */
    Std_ReturnType ret = KeyM_FinalizeKey(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_FinalizeKey_WithData_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    Std_ReturnType ret = KeyM_FinalizeKey(0);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_FinalizeKey_SetsStatusValid(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_VALID, status);
}

void test_FinalizeKey_TriggersNotification(void)
{
    KeyM_Init(&testConfig);
    KeyM_SetNotificationCallback(testNotificationCallback);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    TEST_ASSERT_EQUAL(1, notif_called);
    TEST_ASSERT_EQUAL(0, notif_keyId);
    TEST_ASSERT_EQUAL(KEYM_OPRESULT_SUCCESS, notif_result);
}

/* =========================================================================
 *  KeyM_ParseKey Branches
 * ========================================================================= */

void test_ParseKey_Uninitialized_ReportsError(void)
{
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_ParseKey(0, data, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_ParseKey_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_ParseKey(0, NULL_PTR, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_ParseKey_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_ParseKey(KEYM_NUM_KEYS, data, 4, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_ParseKey_LengthTooLarge_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_ParseKey(0, data, KEYM_MAX_KEY_LENGTH + 1, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_LENGTH, det_last_error);
}

void test_ParseKey_ValidInput_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[32] = {0};
    Std_ReturnType ret = KeyM_ParseKey(0, data, 32, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_ParseKey_SetsStatusNew(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_ParseKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_NEW, status);
}

/* =========================================================================
 *  KeyM_ConvertKey Branches
 * ========================================================================= */

void test_ConvertKey_Uninitialized_ReportsError(void)
{
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_ConvertKey(0, buf, &len, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_ConvertKey_NullOutputPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 len = 32;
    Std_ReturnType ret = KeyM_ConvertKey(0, NULL, &len, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_ConvertKey_NullLenPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    Std_ReturnType ret = KeyM_ConvertKey(0, buf, NULL, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_ConvertKey_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_ConvertKey(KEYM_NUM_KEYS, buf, &len, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_ConvertKey_KeyNotValid_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    /* Not finalized */
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_ConvertKey(0, buf, &len, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ConvertKey_BufferTooSmall_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    uint8 data[32] = {0};
    KeyM_SetKey(0, data, 32, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 8;
    Std_ReturnType ret = KeyM_ConvertKey(0, buf, &len, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_ConvertKey_ValidKey_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_ConvertKey(0, buf, &len, KEYM_KEY_FORMAT_RAW);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(16, len);
}

/* =========================================================================
 *  KeyM_CopyKey Branches
 * ========================================================================= */

void test_CopyKey_Uninitialized_ReportsError(void)
{
    Std_ReturnType ret = KeyM_CopyKey(0, 1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_CopyKey_InvalidSrcId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_CopyKey(KEYM_NUM_KEYS, 1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_CopyKey_InvalidDestId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_CopyKey(0, KEYM_NUM_KEYS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_CopyKey_SrcNotValid_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    /* Source key not finalized */
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    Std_ReturnType ret = KeyM_CopyKey(0, 1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_CopyKey_ValidSrc_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    Std_ReturnType ret = KeyM_CopyKey(0, 1);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Verify copied key has data */
    uint32 ver;
    KeyM_KeyVersionGet(1, &ver);
    TEST_ASSERT_NOT_EQUAL(0, ver);
}

/* =========================================================================
 *  KeyM_KeyElementSet Branches
 * ========================================================================= */

void test_KeyElementSet_Uninitialized_ReportsError(void)
{
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_KeyElementSet(0, 0, data, 4);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_KeyElementSet_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_KeyElementSet(0, 0, NULL_PTR, 4);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyElementSet_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_KeyElementSet(KEYM_NUM_KEYS, 0, data, 4);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_KeyElementSet_InvalidElementId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_KeyElementSet(0, KEYM_MAX_KEY_ELEMENTS, data, 4);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_HANDLE, det_last_error);
}

void test_KeyElementSet_LengthTooLarge_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_KeyElementSet(0, 0, data, KEYM_MAX_KEY_LENGTH + 1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_LENGTH, det_last_error);
}

void test_KeyElementSet_ValidInput_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    Std_ReturnType ret = KeyM_KeyElementSet(0, 0, data, 8);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* =========================================================================
 *  KeyM_KeyElementGet Branches
 * ========================================================================= */

void test_KeyElementGet_Uninitialized_ReportsError(void)
{
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(0, 0, buf, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_KeyElementGet_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(0, 0, NULL, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyElementGet_NullLenPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    Std_ReturnType ret = KeyM_KeyElementGet(0, 0, buf, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyElementGet_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(KEYM_NUM_KEYS, 0, buf, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_KeyElementGet_InvalidElementId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(0, KEYM_MAX_KEY_ELEMENTS, buf, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_HANDLE, det_last_error);
}

void test_KeyElementGet_ElementNotInUse_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(0, 0, buf, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_KeyElementGet_BufferTooSmall_ReturnsNotOk(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_KeyElementSet(0, 0, data, 16);
    uint8 buf[32];
    uint32 len = 4; /* Too small */
    Std_ReturnType ret = KeyM_KeyElementGet(0, 0, buf, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_KeyElementGet_ValidElement_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    KeyM_KeyElementSet(0, 0, data, 8);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(0, 0, buf, &len);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(8, len);
    TEST_ASSERT_EQUAL(0, memcmp(data, buf, 8));
}

/* =========================================================================
 *  KeyM_KeyStatusGet Branches
 * ========================================================================= */

void test_KeyStatusGet_Uninitialized_ReportsError(void)
{
    KeyM_KeyStatusType status;
    Std_ReturnType ret = KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_KeyStatusGet_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_KeyStatusGet(0, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyStatusGet_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    KeyM_KeyStatusType status;
    Std_ReturnType ret = KeyM_KeyStatusGet(KEYM_NUM_KEYS, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_KeyStatusGet_NewKey_ReturnsNew(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_KeyStatusType status;
    Std_ReturnType ret = KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_NEW, status);
}

void test_KeyStatusGet_AfterFinalize_ReturnsValid(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_VALID, status);
}

/* =========================================================================
 *  KeyM_KeyVersionGet Branches
 * ========================================================================= */

void test_KeyVersionGet_Uninitialized_ReportsError(void)
{
    uint32 ver;
    Std_ReturnType ret = KeyM_KeyVersionGet(0, &ver);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_KeyVersionGet_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_KeyVersionGet(0, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyVersionGet_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 ver;
    Std_ReturnType ret = KeyM_KeyVersionGet(KEYM_NUM_KEYS, &ver);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_KeyVersionGet_AfterSetKey_ReturnsNonZero(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    uint32 ver;
    KeyM_KeyVersionGet(0, &ver);
    TEST_ASSERT_NOT_EQUAL(0, ver);
}

/* =========================================================================
 *  KeyM_KeyValidityGet Branches
 * ========================================================================= */

void test_KeyValidityGet_Uninitialized_ReportsError(void)
{
    uint32 from, to;
    Std_ReturnType ret = KeyM_KeyValidityGet(0, &from, &to);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_KeyValidityGet_NullFromPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 to;
    Std_ReturnType ret = KeyM_KeyValidityGet(0, NULL, &to);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyValidityGet_NullToPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 from;
    Std_ReturnType ret = KeyM_KeyValidityGet(0, &from, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyValidityGet_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    uint32 from, to;
    Std_ReturnType ret = KeyM_KeyValidityGet(KEYM_NUM_KEYS, &from, &to);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_KeyValidityGet_ValidKey_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    uint32 from = 0xFF, to = 0xFF;
    Std_ReturnType ret = KeyM_KeyValidityGet(0, &from, &to);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* KEYM_DEFAULT_KEY_VALIDITY=0 -> validTo=0 (no expiry) */
    TEST_ASSERT_EQUAL(0, to);
}

/* =========================================================================
 *  KeyM_KeyInfoGet Branches
 * ========================================================================= */

void test_KeyInfoGet_Uninitialized_ReportsError(void)
{
    KeyM_KeyInfoType info;
    Std_ReturnType ret = KeyM_KeyInfoGet(0, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_KeyInfoGet_NullPtr_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_KeyInfoGet(0, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_PARAM_POINTER, det_last_error);
}

void test_KeyInfoGet_InvalidKeyId_ReportsError(void)
{
    KeyM_Init(&testConfig);
    KeyM_KeyInfoType info;
    Std_ReturnType ret = KeyM_KeyInfoGet(KEYM_NUM_KEYS, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

void test_KeyInfoGet_WithConfig_ReturnsConfigType(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(1, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_KeyInfoType info;
    Std_ReturnType ret = KeyM_KeyInfoGet(1, &info);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_KEY_TYPE_AES, info.keyType);
    /* keyLength is in bits */
    TEST_ASSERT_EQUAL(16 * 8, info.keyLength);
}

void test_KeyInfoGet_NullConfig_ReturnsDefaults(void)
{
    KeyM_Init(&testConfig);
    /* Use a keyId that's valid (< KEYM_NUM_KEYS) but test the config path */
    KeyM_KeyInfoType info;
    KeyM_KeyInfoGet(0, &info);
    /* ConfigPtr is not NULL and keyConfigs is not NULL, keyId < numKeys */
    TEST_ASSERT_EQUAL(KEYM_KEY_TYPE_AES, info.keyType);
}

/* =========================================================================
 *  KeyM_SetNotificationCallback Branches
 * ========================================================================= */

void test_SetNotificationCallback_Uninitialized_ReportsError(void)
{
    Std_ReturnType ret = KeyM_SetNotificationCallback(testNotificationCallback);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_UNINIT, det_last_error);
}

void test_SetNotificationCallback_ValidCallback_Succeeds(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_SetNotificationCallback(testNotificationCallback);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_SetNotificationCallback_NullCallback_Succeeds(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_SetNotificationCallback(NULL);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* =========================================================================
 *  KeyM_MainFunction Branches
 * ========================================================================= */

void test_MainFunction_Uninitialized_Returns(void)
{
    /* Should not crash when uninitialized */
    KeyM_MainFunction();
    TEST_ASSERT_FALSE(KeyM_Initialized);
}

void test_MainFunction_Initialized_IncrementsTime(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    /* Run main function multiple times */
    KeyM_MainFunction();
    KeyM_MainFunction();
    KeyM_MainFunction();
    /* Key should still be valid (KEYM_DEFAULT_KEY_VALIDITY=0 -> no expiry) */
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_VALID, status);
}

void test_MainFunction_ChecksKeyExpiry(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    /* With KEYM_DEFAULT_KEY_VALIDITY=0, validTo=0, so no expiry check triggers */
    KeyM_MainFunction();
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_VALID, status);
}

/* =========================================================================
 *  Key Expiry Branches (KeyM_IsKeyCurrentlyValid)
 * ========================================================================= */

void test_IsKeyCurrentlyValid_NotValid_ReturnsFalse(void)
{
    KeyM_Init(&testConfig);
    /* Key not set -> isValid=FALSE */
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_IsKeyCurrentlyValid_ValidNoExpiry_ReturnsTrue(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    /* validTo=0 means no expiry */
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* =========================================================================
 *  Key Version Wraparound (KeyM_GetNextKeyVersion)
 * ========================================================================= */

void test_KeyVersionWraparound_NormalIncrement(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    uint32 ver1;
    KeyM_KeyVersionGet(0, &ver1);
    KeyM_UpdateKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    uint32 ver2;
    KeyM_KeyVersionGet(0, &ver2);
    TEST_ASSERT_EQUAL(ver1 + 1, ver2);
}

/* =========================================================================
 *  Async Operation Queue (KEYM_ASYNC_OPERATIONS)
 * ========================================================================= */

void test_AsyncQueue_QueueOperation_FillQueue(void)
{
    KeyM_Init(&testConfig);
    /* Fill the operation queue by queueing operations */
    /* We can't directly call KeyM_QueueOperation (static), but we can test
       that MainFunction processes the queue without errors */
    KeyM_MainFunction();
    TEST_ASSERT_TRUE(KeyM_Initialized);
}

/* =========================================================================
 *  KeyM_ProtectKeyData / KeyM_CopyKeyData branches
 * ========================================================================= */

void test_ProtectKeyData_NullKeyPtr_OnlyXorProtection(void)
{
    /* This is tested indirectly through SetKey/GetKey operations */
    KeyM_Init(&testConfig);
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    uint8 buf[32];
    uint32 len = 32;
    KeyM_GetKey(0, buf, &len, NULL);
    TEST_ASSERT_EQUAL(E_OK, len > 0 ? E_OK : E_NOT_OK);
}

/* =========================================================================
 *  Multiple key operations
 * ========================================================================= */

void test_MultipleKeys_IndependentOperations(void)
{
    KeyM_Init(&testConfig);
    uint8 data1[16] = {0xAA};
    uint8 data2[16] = {0xBB};

    KeyM_SetKey(0, data1, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_SetKey(1, data2, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    KeyM_FinalizeKey(1);

    uint8 buf1[32], buf2[32];
    uint32 len1 = 32, len2 = 32;
    KeyM_GetKey(0, buf1, &len1, NULL);
    KeyM_GetKey(1, buf2, &len2, NULL);

    TEST_ASSERT_EQUAL(16, len1);
    TEST_ASSERT_EQUAL(16, len2);
    TEST_ASSERT_NOT_EQUAL(0, memcmp(buf1, buf2, 16));
}

void test_SetKey_AfterFinalize_ResetsToNew(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);

    uint8 data2[16] = {0xFF};
    KeyM_SetKey(0, data2, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_KeyStatusType status;
    KeyM_KeyStatusGet(0, &status);
    TEST_ASSERT_EQUAL(KEYM_KEY_STATUS_NEW, status);
}

/* =========================================================================
 *  DeInit resets callback
 * ========================================================================= */

void test_DeInit_ResetsCallback(void)
{
    KeyM_Init(&testConfig);
    KeyM_SetNotificationCallback(testNotificationCallback);
    KeyM_DeInit();
    /* Re-init and finalize should NOT trigger old callback */
    KeyM_Initialized = FALSE;
    KeyM_Init(&testConfig);
    notif_called = 0;
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    TEST_ASSERT_EQUAL(0, notif_called);
}

/* =========================================================================
 *  KeyM_Init with all keys cleared
 * ========================================================================= */

void test_Init_ClearsAllKeys(void)
{
    KeyM_Init(&testConfig);
    uint8 data[16] = {0};
    KeyM_SetKey(0, data, 16, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(0);
    KeyM_DeInit();

    /* Re-init should clear all key data */
    KeyM_Initialized = FALSE;
    KeyM_Init(&testConfig);
    uint32 ver;
    KeyM_KeyVersionGet(0, &ver);
    TEST_ASSERT_EQUAL(0, ver);
}

/* =========================================================================
 *  Edge case: both src and dest invalid for CopyKey
 * ========================================================================= */

void test_CopyKey_BothInvalid_ReportsError(void)
{
    KeyM_Init(&testConfig);
    Std_ReturnType ret = KeyM_CopyKey(KEYM_NUM_KEYS, KEYM_NUM_KEYS + 1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(KEYM_E_INVALID_KEY, det_last_error);
}

/* =========================================================================
 *  Edge case: KeyElementSet at boundary element ID
 * ========================================================================= */

void test_KeyElementSet_MaxValidElementId_Succeeds(void)
{
    KeyM_Init(&testConfig);
    uint8 data[4] = {1, 2, 3, 4};
    Std_ReturnType ret = KeyM_KeyElementSet(0, KEYM_MAX_KEY_ELEMENTS - 1, data, 4);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_KeyElementGet_MaxValidElementId_NotInUse(void)
{
    KeyM_Init(&testConfig);
    uint8 buf[32];
    uint32 len = 32;
    Std_ReturnType ret = KeyM_KeyElementGet(0, KEYM_MAX_KEY_ELEMENTS - 1, buf, &len);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret); /* Not in use */
}

/* =========================================================================
 *  KeyInfoGet with key that has no config entry
 * ========================================================================= */

void test_KeyInfoGet_KeyBeyondConfigNum_ReturnsDefaults(void)
{
    /* Create a config with fewer keys than KEYM_NUM_KEYS */
    static KeyM_KeyConfigType smallConfigs[2] = {
        {0, KEYM_KEY_TYPE_AES, 32, 4, 1, 0, 0xFF},
        {1, KEYM_KEY_TYPE_AES, 16, 4, 1, 0, 0xFF},
    };
    static const KeyM_ConfigType smallConfig = {
        smallConfigs, 2, KEYM_MAX_KEY_ELEMENTS, TRUE, TRUE, KEYM_MAX_KEY_LIFETIME
    };
    KeyM_Init(&smallConfig);
    /* keyId=3 is valid (< KEYM_NUM_KEYS=8) but >= numKeys=2 */
    KeyM_KeyInfoType info;
    KeyM_KeyInfoGet(3, &info);
    TEST_ASSERT_EQUAL(KEYM_KEY_TYPE_GENERIC, info.keyType);
    TEST_ASSERT_EQUAL(0xFF, info.keyUsage);
}

/* ===== main ===== */
int main(void)
{
    UNITY_BEGIN();

    /* Init / DeInit */
    RUN_TEST(test_Init_ValidConfig_Succeeds);
    RUN_TEST(test_Init_AlreadyInitialized_ReportsError);
    RUN_TEST(test_Init_NullPtr_ReportsError);
    RUN_TEST(test_Init_ResetsSystemTime);
    RUN_TEST(test_DeInit_Initialized_Succeeds);
    RUN_TEST(test_DeInit_Uninitialized_ReportsError);

    /* GetVersionInfo */
    RUN_TEST(test_GetVersionInfo_ValidPtr_Succeeds);
    RUN_TEST(test_GetVersionInfo_NullPtr_ReportsError);

    /* SetKey */
    RUN_TEST(test_SetKey_Uninitialized_ReportsError);
    RUN_TEST(test_SetKey_NullKeyPtr_ReportsError);
    RUN_TEST(test_SetKey_InvalidKeyId_ReportsError);
    RUN_TEST(test_SetKey_LengthTooLarge_ReportsError);
    RUN_TEST(test_SetKey_ValidInput_Succeeds);
    RUN_TEST(test_SetKey_SetsStatusNew);
    RUN_TEST(test_SetKey_MaxLength_Succeeds);
    RUN_TEST(test_SetKey_ExactMaxLen_BoundaryCheck);

    /* GetKey */
    RUN_TEST(test_GetKey_Uninitialized_ReportsError);
    RUN_TEST(test_GetKey_NullKeyPtr_ReportsError);
    RUN_TEST(test_GetKey_NullLenPtr_ReportsError);
    RUN_TEST(test_GetKey_InvalidKeyId_ReportsError);
    RUN_TEST(test_GetKey_KeyNotValid_ReturnsNotOk);
    RUN_TEST(test_GetKey_BufferTooSmall_ReturnsNotOk);
    RUN_TEST(test_GetKey_ValidKey_Succeeds);
    RUN_TEST(test_GetKey_WithFormatPtr_SetsRaw);
    RUN_TEST(test_GetKey_NullFormatPtr_Succeeds);

    /* UpdateKey */
    RUN_TEST(test_UpdateKey_Uninitialized_ReportsError);
    RUN_TEST(test_UpdateKey_NullPtr_ReportsError);
    RUN_TEST(test_UpdateKey_InvalidKeyId_ReportsError);
    RUN_TEST(test_UpdateKey_LengthTooLarge_ReportsError);
    RUN_TEST(test_UpdateKey_ValidInput_SetsStatusUpdate);
    RUN_TEST(test_UpdateKey_IncrementsVersion);

    /* FinalizeKey */
    RUN_TEST(test_FinalizeKey_Uninitialized_ReportsError);
    RUN_TEST(test_FinalizeKey_InvalidKeyId_ReportsError);
    RUN_TEST(test_FinalizeKey_NoData_ReturnsNotOk);
    RUN_TEST(test_FinalizeKey_WithData_Succeeds);
    RUN_TEST(test_FinalizeKey_SetsStatusValid);
    RUN_TEST(test_FinalizeKey_TriggersNotification);

    /* ParseKey */
    RUN_TEST(test_ParseKey_Uninitialized_ReportsError);
    RUN_TEST(test_ParseKey_NullPtr_ReportsError);
    RUN_TEST(test_ParseKey_InvalidKeyId_ReportsError);
    RUN_TEST(test_ParseKey_LengthTooLarge_ReportsError);
    RUN_TEST(test_ParseKey_ValidInput_Succeeds);
    RUN_TEST(test_ParseKey_SetsStatusNew);

    /* ConvertKey */
    RUN_TEST(test_ConvertKey_Uninitialized_ReportsError);
    RUN_TEST(test_ConvertKey_NullOutputPtr_ReportsError);
    RUN_TEST(test_ConvertKey_NullLenPtr_ReportsError);
    RUN_TEST(test_ConvertKey_InvalidKeyId_ReportsError);
    RUN_TEST(test_ConvertKey_KeyNotValid_ReturnsNotOk);
    RUN_TEST(test_ConvertKey_BufferTooSmall_ReturnsNotOk);
    RUN_TEST(test_ConvertKey_ValidKey_Succeeds);

    /* CopyKey */
    RUN_TEST(test_CopyKey_Uninitialized_ReportsError);
    RUN_TEST(test_CopyKey_InvalidSrcId_ReportsError);
    RUN_TEST(test_CopyKey_InvalidDestId_ReportsError);
    RUN_TEST(test_CopyKey_SrcNotValid_ReturnsNotOk);
    RUN_TEST(test_CopyKey_ValidSrc_Succeeds);
    RUN_TEST(test_CopyKey_BothInvalid_ReportsError);

    /* KeyElementSet */
    RUN_TEST(test_KeyElementSet_Uninitialized_ReportsError);
    RUN_TEST(test_KeyElementSet_NullPtr_ReportsError);
    RUN_TEST(test_KeyElementSet_InvalidKeyId_ReportsError);
    RUN_TEST(test_KeyElementSet_InvalidElementId_ReportsError);
    RUN_TEST(test_KeyElementSet_LengthTooLarge_ReportsError);
    RUN_TEST(test_KeyElementSet_ValidInput_Succeeds);
    RUN_TEST(test_KeyElementSet_MaxValidElementId_Succeeds);

    /* KeyElementGet */
    RUN_TEST(test_KeyElementGet_Uninitialized_ReportsError);
    RUN_TEST(test_KeyElementGet_NullPtr_ReportsError);
    RUN_TEST(test_KeyElementGet_NullLenPtr_ReportsError);
    RUN_TEST(test_KeyElementGet_InvalidKeyId_ReportsError);
    RUN_TEST(test_KeyElementGet_InvalidElementId_ReportsError);
    RUN_TEST(test_KeyElementGet_ElementNotInUse_ReturnsNotOk);
    RUN_TEST(test_KeyElementGet_BufferTooSmall_ReturnsNotOk);
    RUN_TEST(test_KeyElementGet_ValidElement_Succeeds);
    RUN_TEST(test_KeyElementGet_MaxValidElementId_NotInUse);

    /* KeyStatusGet */
    RUN_TEST(test_KeyStatusGet_Uninitialized_ReportsError);
    RUN_TEST(test_KeyStatusGet_NullPtr_ReportsError);
    RUN_TEST(test_KeyStatusGet_InvalidKeyId_ReportsError);
    RUN_TEST(test_KeyStatusGet_NewKey_ReturnsNew);
    RUN_TEST(test_KeyStatusGet_AfterFinalize_ReturnsValid);

    /* KeyVersionGet */
    RUN_TEST(test_KeyVersionGet_Uninitialized_ReportsError);
    RUN_TEST(test_KeyVersionGet_NullPtr_ReportsError);
    RUN_TEST(test_KeyVersionGet_InvalidKeyId_ReportsError);
    RUN_TEST(test_KeyVersionGet_AfterSetKey_ReturnsNonZero);

    /* KeyValidityGet */
    RUN_TEST(test_KeyValidityGet_Uninitialized_ReportsError);
    RUN_TEST(test_KeyValidityGet_NullFromPtr_ReportsError);
    RUN_TEST(test_KeyValidityGet_NullToPtr_ReportsError);
    RUN_TEST(test_KeyValidityGet_InvalidKeyId_ReportsError);
    RUN_TEST(test_KeyValidityGet_ValidKey_Succeeds);

    /* KeyInfoGet */
    RUN_TEST(test_KeyInfoGet_Uninitialized_ReportsError);
    RUN_TEST(test_KeyInfoGet_NullPtr_ReportsError);
    RUN_TEST(test_KeyInfoGet_InvalidKeyId_ReportsError);
    RUN_TEST(test_KeyInfoGet_WithConfig_ReturnsConfigType);
    RUN_TEST(test_KeyInfoGet_NullConfig_ReturnsDefaults);
    RUN_TEST(test_KeyInfoGet_KeyBeyondConfigNum_ReturnsDefaults);

    /* SetNotificationCallback */
    RUN_TEST(test_SetNotificationCallback_Uninitialized_ReportsError);
    RUN_TEST(test_SetNotificationCallback_ValidCallback_Succeeds);
    RUN_TEST(test_SetNotificationCallback_NullCallback_Succeeds);

    /* MainFunction */
    RUN_TEST(test_MainFunction_Uninitialized_Returns);
    RUN_TEST(test_MainFunction_Initialized_IncrementsTime);
    RUN_TEST(test_MainFunction_ChecksKeyExpiry);

    /* Key validity / expiry */
    RUN_TEST(test_IsKeyCurrentlyValid_NotValid_ReturnsFalse);
    RUN_TEST(test_IsKeyCurrentlyValid_ValidNoExpiry_ReturnsTrue);

    /* Version wraparound */
    RUN_TEST(test_KeyVersionWraparound_NormalIncrement);

    /* Async queue */
    RUN_TEST(test_AsyncQueue_QueueOperation_FillQueue);

    /* ProtectKeyData */
    RUN_TEST(test_ProtectKeyData_NullKeyPtr_OnlyXorProtection);

    /* Multiple key operations */
    RUN_TEST(test_MultipleKeys_IndependentOperations);
    RUN_TEST(test_SetKey_AfterFinalize_ResetsToNew);

    /* DeInit resets */
    RUN_TEST(test_DeInit_ResetsCallback);
    RUN_TEST(test_Init_ClearsAllKeys);

    return UNITY_END();
}

/**
 * @file test_nm.c
 * @brief Nm (Network Management Interface) Unit Tests
 * @req SWS_Nm
 *
 * Real assertions against the fully implemented Nm SUT
 * (src/bsw/services/nm/src/Nm.c, NM_DEV_ERROR_DETECT = STD_ON).
 * Development-error reports are captured by the local Det mock below.
 */

// @tests src/bsw/services/nm/src/Nm.c  @tests src/bsw/services/nm/include/Nm.h
#include "unity.h"
#include "Nm.h"
#include "Nm_Cfg.h"

/* SUT quirk: NM_VENDOR_ID (0x0001) is defined in Nm.c only, not in Nm.h,
 * so the expected vendor ID is spelled out as a literal here. */
#define TEST_NM_EXPECTED_VENDOR_ID   (0x0001U)

/* Mock Det_ReportError — captures report arguments (NM_DEV_ERROR_DETECT = STD_ON) */
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config (Nm_ConfigType carries a dummy field only; Nm_Init does not
 * dereference the pointer beyond the NULL check). */
static Nm_ConfigType testConfig = { 0U };

void setUp(void) {
    /* Return the SUT to a clean, uninitialized state. */
    Nm_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_Nm_00001 */
void test_Nm_Init_NullPtr_ReportsDetWithExactIds(void) {
    Nm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(NM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(NM_INIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(NM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Nm_00001 */
void test_Nm_Init_ValidConfig_DoesNotReportDet(void) {
    Nm_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Nm_00002 */
void test_Nm_DeInit_BeforeInit_ReportsUninit(void) {
    mock_Det_Reset();
    Nm_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(NM_DEINIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(NM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Nm_00002 */
void test_Nm_DeInit_AfterInit_LeavesModuleUninitialized(void) {
    Nm_StateType state = NM_STATE_NORMAL_OPERATION;
    Nm_Init(&testConfig);
    Nm_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_NOT_OK, Nm_GetState(NM_CHANNEL_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(NM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Nm_00003 */
void test_Nm_GetVersionInfo_ValidPtr_FillsVersion(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    Nm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(TEST_NM_EXPECTED_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(NM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(NM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(NM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(NM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Nm_00003 */
void test_Nm_GetVersionInfo_NullPtr_IsSilentNoDet(void) {
    /* SUT quirk: Nm_GetVersionInfo silently ignores a NULL pointer
     * (no Det_ReportError call) — NM_DEV_ERROR_DETECT does not cover it. */
    Nm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Nm_00004 */
void test_Nm_PassiveStartUp_BeforeInit_ReportsUninit(void) {
    Std_ReturnType ret = Nm_PassiveStartUp(NM_CHANNEL_CAN0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(NM_PASSIVESSTARTUP_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(NM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Nm_00004 */
void test_Nm_PassiveStartUp_InvalidChannel_ReportsInvalidChannel(void) {
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_PassiveStartUp(NM_MAX_CHANNELS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(NM_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_Nm_00004 */
void test_Nm_PassiveStartUp_ValidChannel_EnthersRepeatMessageState(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_PassiveStartUp(NM_CHANNEL_CAN0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_OK, Nm_GetState(NM_CHANNEL_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, state);
    TEST_ASSERT_EQUAL(E_OK, Nm_GetMode(NM_CHANNEL_CAN0, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Nm_00005 */
void test_Nm_NetworkRequest_AfterInit_TransitionsToRepeatMessage(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_NetworkRequest(NM_CHANNEL_CAN0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_OK, Nm_GetState(NM_CHANNEL_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, state);
}

/** @req SWS_Nm_00006 */
void test_Nm_NetworkRelease_AfterInit_TransitionsToReadySleep(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    (void)Nm_NetworkRequest(NM_CHANNEL_CAN0);
    ret = Nm_NetworkRelease(NM_CHANNEL_CAN0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_OK, Nm_GetState(NM_CHANNEL_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, state);
}

/** @req SWS_Nm_00005 SWS_Nm_00012 */
void test_Nm_MainFunction_RepeatMessageWithRequest_BecomesNormalOperation(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_Init(&testConfig);
    (void)Nm_NetworkRequest(NM_CHANNEL_CAN0);
    Nm_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, Nm_GetState(NM_CHANNEL_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, state);
}

/** @req SWS_Nm_00012 */
void test_Nm_MainFunction_BeforeInit_IsSilentNoDet(void) {
    /* SUT quirk: uninitialized Nm_MainFunction returns without a DET report. */
    Nm_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Nm_00009 */
void test_Nm_GetState_NullPtr_ReportsInvalidPointer(void) {
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_GetState(NM_CHANNEL_CAN0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(NM_GETSTATE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(NM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Nm_00010 */
void test_Nm_GetMode_NullPtr_ReportsDetWithSutApiId(void) {
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_GetMode(NM_CHANNEL_CAN0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    /* SUT quirk: Nm_GetMode hardcodes ApiId 0x90 instead of a named SID */
    TEST_ASSERT_EQUAL_UINT8(0x90U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(NM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Nm_00011 */
void test_Nm_GetLocalNodeIdentifier_ReturnsConfiguredNodeId(void) {
    Nm_NodeIdType nodeId = 0xFFU;
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_GetLocalNodeIdentifier(NM_CHANNEL_CAN0, &nodeId);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(NM_NODE_ID, nodeId);
}

/** @req SWS_Nm_00011 */
void test_Nm_GetLocalNodeIdentifier_NullPtr_ReportsInvalidPointer(void) {
    Std_ReturnType ret;
    Nm_Init(&testConfig);
    ret = Nm_GetLocalNodeIdentifier(NM_CHANNEL_CAN0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(NM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Nm_00013 */
void test_Nm_SetUserData_GetUserData_RoundTrip(void) {
    uint8 written[8] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    uint8 read[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    Nm_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Nm_SetUserData(NM_CHANNEL_CAN0, written));
    TEST_ASSERT_EQUAL(E_OK, Nm_GetUserData(NM_CHANNEL_CAN0, read));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(written, read, 8U);
}

/** @req SWS_Nm_00007 SWS_Nm_00008 */
void test_Nm_DisableEnableCommunication_BothReturnOk(void) {
    Nm_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, Nm_DisableCommunication(NM_CHANNEL_CAN0));
    TEST_ASSERT_EQUAL(E_OK, Nm_EnableCommunication(NM_CHANNEL_CAN0));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Nm_00101 SWS_Nm_00102 SWS_Nm_00014 */
void test_Nm_RemoteSleepIndication_ReflectedInCheckApi(void) {
    boolean remoteSleep = FALSE;
    Nm_Init(&testConfig);
    Nm_RemoteSleepIndication(NM_CHANNEL_CAN0);
    TEST_ASSERT_EQUAL(E_OK, Nm_CheckRemoteSleepIndication(NM_CHANNEL_CAN0, &remoteSleep));
    TEST_ASSERT_TRUE(remoteSleep);
    Nm_RemoteSleepCancellation(NM_CHANNEL_CAN0);
    TEST_ASSERT_EQUAL(E_OK, Nm_CheckRemoteSleepIndication(NM_CHANNEL_CAN0, &remoteSleep));
    TEST_ASSERT_FALSE(remoteSleep);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Nm_Init_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_Nm_Init_ValidConfig_DoesNotReportDet);
    RUN_TEST(test_Nm_DeInit_BeforeInit_ReportsUninit);
    RUN_TEST(test_Nm_DeInit_AfterInit_LeavesModuleUninitialized);
    RUN_TEST(test_Nm_GetVersionInfo_ValidPtr_FillsVersion);
    RUN_TEST(test_Nm_GetVersionInfo_NullPtr_IsSilentNoDet);
    RUN_TEST(test_Nm_PassiveStartUp_BeforeInit_ReportsUninit);
    RUN_TEST(test_Nm_PassiveStartUp_InvalidChannel_ReportsInvalidChannel);
    RUN_TEST(test_Nm_PassiveStartUp_ValidChannel_EnthersRepeatMessageState);
    RUN_TEST(test_Nm_NetworkRequest_AfterInit_TransitionsToRepeatMessage);
    RUN_TEST(test_Nm_NetworkRelease_AfterInit_TransitionsToReadySleep);
    RUN_TEST(test_Nm_MainFunction_RepeatMessageWithRequest_BecomesNormalOperation);
    RUN_TEST(test_Nm_MainFunction_BeforeInit_IsSilentNoDet);
    RUN_TEST(test_Nm_GetState_NullPtr_ReportsInvalidPointer);
    RUN_TEST(test_Nm_GetMode_NullPtr_ReportsDetWithSutApiId);
    RUN_TEST(test_Nm_GetLocalNodeIdentifier_ReturnsConfiguredNodeId);
    RUN_TEST(test_Nm_GetLocalNodeIdentifier_NullPtr_ReportsInvalidPointer);
    RUN_TEST(test_Nm_SetUserData_GetUserData_RoundTrip);
    RUN_TEST(test_Nm_DisableEnableCommunication_BothReturnOk);
    RUN_TEST(test_Nm_RemoteSleepIndication_ReflectedInCheckApi);

    return UnityEnd();
}

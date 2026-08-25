/**
 * @file test_wdgm.c
 * @brief WdgM (Watchdog Manager) Unit Tests
 * @req SWS_WdgM
 */

// @tests src/bsw/services/wdgm/src/WdgM.c  @tests src/bsw/services/wdgm/include/WdgM.h

#include "unity.h"
#include "WdgM.h"

static uint8 mock_DetApiId = 0xFFU;
static uint8 mock_DetErrorId = 0xFFU;
static uint8 mock_DetCalls = 0;

static void mock_Det_Reset(void) {
    mock_DetApiId = 0xFFU;
    mock_DetErrorId = 0xFFU;
    mock_DetCalls = 0;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetApiId = ApiId;
    mock_DetErrorId = ErrorId;
    mock_DetCalls++;
    return E_OK;
}

static WdgM_ConfigType testConfig;

void setUp(void) { mock_Det_Reset(); }
void tearDown(void) {}

/** @req SWS_WdgM_00001 */
void test_WdgM_Init_NullPtr_ShouldFail(void) {
    Std_ReturnType ret = WdgM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_WdgM_00001 */
void test_WdgM_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.MaxSupervisionEntities = 4U;
    testConfig.SupervisionEntities = NULL_PTR;
    Std_ReturnType ret = WdgM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00002 */
void test_WdgM_DeInit_AfterInit_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    Std_ReturnType ret = WdgM_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00003 */
void test_WdgM_GetState_AfterInit_ShouldReturnActive(void) {
    WdgM_Init(&testConfig);
    WdgM_StateType state = WdgM_GetState();
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, state);
}

/** @req SWS_WdgM_00003 */
void test_WdgM_GetState_BeforeInit_ShouldReturnUninit(void) {
    WdgM_StateType state = WdgM_GetState();
    TEST_ASSERT_EQUAL(WDGM_STATE_UNINIT, state);
}

/** @req SWS_WdgM_00004 */
void test_WdgM_SetMode_ValidMode_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    Std_ReturnType ret = WdgM_SetMode(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00005 */
void test_WdgM_GetMode_AfterSet_ShouldReturnSetMode(void) {
    WdgM_Init(&testConfig);
    WdgM_SetMode(1U);
    uint8 mode = WdgM_GetMode();
    TEST_ASSERT_EQUAL(1U, mode);
}

/** @req SWS_WdgM_00006 */
void test_WdgM_CheckpointReached_ValidId_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    Std_ReturnType ret = WdgM_CheckpointReached(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00006 */
void test_WdgM_CheckpointReached_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = WdgM_CheckpointReached(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_WdgM_00007 */
void test_WdgM_UpdateAliveIndication_ValidId_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    Std_ReturnType ret = WdgM_UpdateAliveIndication(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00008 */
void test_WdgM_GetSEState_ValidId_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    WdgM_SEStateType state;
    Std_ReturnType ret = WdgM_GetSEState(0U, &state);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00009 */
void test_WdgM_GetGlobalStatus_AfterInit_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    WdgM_GlobalStatusType status;
    Std_ReturnType ret = WdgM_GetGlobalStatus(&status);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00010 */
void test_WdgM_MainFunction_AfterInit_ShouldNotCrash(void) {
    WdgM_Init(&testConfig);
    WdgM_MainFunction();
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

/** @req SWS_WdgM_00011 */
void test_WdgM_GetFirstExpiredSEID_NoExpiry_ShouldFail(void) {
    WdgM_Init(&testConfig);
    uint16 seId;
    Std_ReturnType ret = WdgM_GetFirstExpiredSEID(&seId);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_WdgM_00012 */
void test_WdgM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    WdgM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(WDGM_VENDOR_ID, info.vendorID);
}

/** @req SWS_WdgM_00012 */
void test_WdgM_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    WdgM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_WdgM_00013 */
void test_WdgM_DeactivateSE_ValidId_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    Std_ReturnType ret = WdgM_DeactivateSupervisionEntity(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00014 */
void test_WdgM_ActivateSE_ValidId_ShouldSucceed(void) {
    WdgM_Init(&testConfig);
    Std_ReturnType ret = WdgM_ActivateSupervisionEntity(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_WdgM_00015 */
void test_WdgM_IsDisableAllowed_AfterInit_ShouldReturnBoolean(void) {
    WdgM_Init(&testConfig);
    boolean result = WdgM_IsDisableAllowed();
    TEST_ASSERT_TRUE(result == TRUE || result == FALSE);
}

void test_WdgM_HandleLockstepError_ShouldNotCrash(void) {
    WdgM_Init(&testConfig);
    WdgM_HandleLockstepError(0x01U);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

void test_WdgM_HandleRamSafetyError_ShouldNotCrash(void) {
    WdgM_Init(&testConfig);
    WdgM_HandleRamSafetyError(0x01U);
    TEST_ASSERT_EQUAL(WDGM_STATE_ACTIVE, WdgM_GetState());
}

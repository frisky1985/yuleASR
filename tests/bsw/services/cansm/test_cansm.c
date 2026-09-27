/**
 * @file test_cansm.c
 * @brief CanSM (CAN State Manager) Unit Tests
 * @req SWS_CanSM
 *
 * Scope note: the production sources (src/bsw/services/cansm/src/CanSm.c)
 * implement the module lifecycle (CanSM_Init / CanSM_DeInit), the controller
 * mode indication path (CanSM_ControllerModeIndication), the internal state
 * observation hook (CanSM_GetCurrentInternalState), the partial networking
 * services (CanSM_ConfirmPnAvailability / CanSM_ClearTrcvWufFlagIndication),
 * and CanSm_GetVersionInfo (guarded by CANSM_VERSION_INFO_API == STD_ON).
 * CanSM_RequestComMode, CanSM_GetCurrentComMode, CanSM_MainFunction,
 * CanSM_ControllerBusOff, CanSM_SetBaudrate and CanSM_GetBaudrate have no
 * implementation yet, so the substantiated tests below target the real,
 * implemented API. The SUT reports development errors via Det_ReportError
 * (CANSM_DEV_ERROR_DETECT == STD_ON); a local mock captures the report
 * arguments. The CanIf stubs (stubs.c) return E_OK for all controller/PDU
 * mode requests, which lets the mode indication path reach FULLCOM.
 */

// @tests src/bsw/services/cansm/src/CanSm.c  @tests src/bsw/services/cansm/include/CanSm.h
#include "unity.h"
#include "CanSm.h"

/* SUT header/source name mismatch: CanSm.h declares CanSM_GetVersionInfo
 * (capital SM) but CanSm.c defines CanSm_GetVersionInfo. The implemented
 * symbol is what we test; production sources must not be modified. */
extern void CanSm_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Mock Det_ReportError — captures report arguments (CANSM_DEV_ERROR_DETECT = STD_ON) */
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetLastInstanceId = 0xFFU;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetLastInstanceId = 0xFFU;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    mock_DetLastModuleId = ModuleId;
    mock_DetLastInstanceId = InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
}

/* Drives network 0 to CANSM_BSM_S_FULLCOM through the implemented path:
 * CanSM_Init(NULL_PTR) puts the network in CANSM_BSM_S_NOCOM, then a CanIf
 * controller mode indication with CANIF_CS_STARTED is processed by
 * CanSm_HandleModeConfirmation -> CanSm_TransitionToFullCom (the CanIf
 * stubs return E_OK for every mode request). */
static void reach_fullcom(void) {
    CanSM_Init(NULL_PTR);
    CanSM_ControllerModeIndication((uint8)CANSM_CONTROLLER_CAN0, CANIF_CS_STARTED);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_FillsVendorId(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(CANSM_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_FillsModuleId(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(CANSM_MODULE_ID, info.moduleID);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_FillsSwVersion(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_DoesNotReportDet(void) {
    Std_VersionInfoType info;
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_NullPtr_ReportsDetWithExactIds(void) {
    CanSm_GetVersionInfo(NULL_PTR);
    /* SUT hardcodes ApiId 0x02 in the NULL-pointer guard (not CANSM_SID_GETVERSIONINFO) */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANSM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_INSTANCE_ID, mock_DetLastInstanceId);
    TEST_ASSERT_EQUAL_UINT8(0x02U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_NullThenValid_RecoversCleanly(void) {
    CanSm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    /* Valid call after the NULL report: no new report, version still filled */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANSM_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_RepeatedCalls_KeepSilentDet(void) {
    Std_VersionInfoType info;
    CanSm_GetVersionInfo(&info);
    CanSm_GetVersionInfo(&info);
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanSM_00001 */
void test_CanSm_Init_NullPtr_EntersNoComWithoutDet(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOTINITIALIZED;

    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_INT(CANSM_BSM_S_NOCOM, state);
}

/** @req SWS_CanSM_00001 */
void test_CanSm_Init_AfterDeInit_EntersNoComAgain(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOTINITIALIZED;

    CanSM_Init(NULL_PTR);
    CanSM_DeInit();
    mock_Det_Reset();

    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_INT(CANSM_BSM_S_NOCOM, state);
}

/** @req SWS_CanSM_00002 */
void test_CanSm_DeInit_ReportsNetworkNotInitialized(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOCOM;

    CanSM_Init(NULL_PTR);
    mock_Det_Reset();

    CanSM_DeInit();

    /* DeInit leaves no observable state behind except the uninitialized flag */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_GETCURRENTINTERNALSTATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00012 */
void test_CanSm_GetCurrentInternalState_BeforeInit_ReportsNotInitialized(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOCOM;

    CanSM_DeInit();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_GETCURRENTINTERNALSTATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00012 */
void test_CanSm_GetCurrentInternalState_NullPtr_ReportsParamPointer(void) {
    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_GETCURRENTINTERNALSTATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00012 */
void test_CanSm_GetCurrentInternalState_InvalidNetwork_ReportsParamNetwork(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOCOM;

    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NUM_NETWORKS, &state));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_GETCURRENTINTERNALSTATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_NETWORK, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00007 */
void test_CanSm_ControllerModeIndication_BeforeInit_ReportsNotInitialized(void) {
    CanSM_DeInit();

    CanSM_ControllerModeIndication((uint8)CANSM_CONTROLLER_CAN0, CANIF_CS_STARTED);

    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CONTROLLERMODEINDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00007 */
void test_CanSm_ControllerModeIndication_UnknownController_ReportsParamController(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOTINITIALIZED;

    CanSM_Init(NULL_PTR);

    CanSM_ControllerModeIndication((uint8)0x42U, CANIF_CS_STARTED);

    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CONTROLLERMODEINDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_CONTROLLER, mock_DetLastErrorId);
    /* No matching controller: the network state must stay untouched */
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_INT(CANSM_BSM_S_NOCOM, state);
}

/** @req SWS_CanSM_00007 */
void test_CanSm_ControllerModeIndication_StartedFromNoCom_ReachesFullCom(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOTINITIALIZED;

    reach_fullcom();

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_INT(CANSM_BSM_S_FULLCOM, state);
}

/** @req SWS_CanSM_00007 */
void test_CanSm_ControllerModeIndication_StoppedFromFullCom_ReachesSilentCom(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOTINITIALIZED;

    reach_fullcom();

    CanSM_ControllerModeIndication((uint8)CANSM_CONTROLLER_CAN0, CANIF_CS_STOPPED);

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_INT(CANSM_BSM_S_SILENTCOM, state);
}

/** @req SWS_CanSM_00007 */
void test_CanSm_ControllerModeIndication_StoppedThenStarted_RoundTripToFullCom(void) {
    CanSm_BsmStateType state = CANSM_BSM_S_NOTINITIALIZED;

    reach_fullcom();
    CanSM_ControllerModeIndication((uint8)CANSM_CONTROLLER_CAN0, CANIF_CS_STOPPED);
    CanSM_ControllerModeIndication((uint8)CANSM_CONTROLLER_CAN0, CANIF_CS_STARTED);

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_GetCurrentInternalState((uint8)CANSM_NETWORK_CAN0, &state));
    TEST_ASSERT_EQUAL_INT(CANSM_BSM_S_FULLCOM, state);
}

/** @req SWS_CanSM */
void test_CanSm_ConfirmPnAvailability_BeforeInit_ReportsNotInitialized(void) {
    CanSM_DeInit();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_ConfirmPnAvailability((NetworkHandleType)CANSM_NETWORK_CAN0));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANSM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_INSTANCE_ID, mock_DetLastInstanceId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CONFIRMPNAVAILABILITY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_CanSM */
void test_CanSm_ConfirmPnAvailability_InvalidNetwork_ReportsParamNetwork(void) {
    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_ConfirmPnAvailability((NetworkHandleType)CANSM_NUM_NETWORKS));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CONFIRMPNAVAILABILITY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_NETWORK, mock_DetLastErrorId);
}

/** @req SWS_CanSM */
void test_CanSm_ConfirmPnAvailability_NoCom_ReportsInvalidNetworkMode(void) {
    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_ConfirmPnAvailability((NetworkHandleType)CANSM_NETWORK_CAN0));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CONFIRMPNAVAILABILITY, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_INVALID_NETWORK_MODE, mock_DetLastErrorId);
}

/** @req SWS_CanSM */
void test_CanSm_ConfirmPnAvailability_FullCom_ReturnsOkWithoutDet(void) {
    reach_fullcom();
    mock_Det_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_ConfirmPnAvailability((NetworkHandleType)CANSM_NETWORK_CAN0));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanSM */
void test_CanSm_ClearTrcvWufFlagIndication_BeforeInit_ReportsNotInitialized(void) {
    CanSM_DeInit();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_ClearTrcvWufFlagIndication((NetworkHandleType)CANSM_NETWORK_CAN0));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANSM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_INSTANCE_ID, mock_DetLastInstanceId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CLEARTRCVWUFFLAGINDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_CanSM */
void test_CanSm_ClearTrcvWufFlagIndication_InvalidNetwork_ReportsParamNetwork(void) {
    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_ClearTrcvWufFlagIndication((NetworkHandleType)CANSM_NUM_NETWORKS));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CLEARTRCVWUFFLAGINDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_NETWORK, mock_DetLastErrorId);
}

/** @req SWS_CanSM */
void test_CanSm_ClearTrcvWufFlagIndication_NoCom_ReportsInvalidNetworkMode(void) {
    CanSM_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanSM_ClearTrcvWufFlagIndication((NetworkHandleType)CANSM_NETWORK_CAN0));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SID_CLEARTRCVWUFFLAGINDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_INVALID_NETWORK_MODE, mock_DetLastErrorId);
}

/** @req SWS_CanSM */
void test_CanSm_ClearTrcvWufFlagIndication_FullCom_ReturnsOkWithoutDet(void) {
    reach_fullcom();
    mock_Det_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanSM_ClearTrcvWufFlagIndication((NetworkHandleType)CANSM_NETWORK_CAN0));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_FillsVendorId);
    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_FillsModuleId);
    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_FillsSwVersion);
    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_DoesNotReportDet);
    RUN_TEST(test_CanSm_GetVersionInfo_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_CanSm_GetVersionInfo_NullThenValid_RecoversCleanly);
    RUN_TEST(test_CanSm_GetVersionInfo_RepeatedCalls_KeepSilentDet);

    RUN_TEST(test_CanSm_Init_NullPtr_EntersNoComWithoutDet);
    RUN_TEST(test_CanSm_Init_AfterDeInit_EntersNoComAgain);
    RUN_TEST(test_CanSm_DeInit_ReportsNetworkNotInitialized);
    RUN_TEST(test_CanSm_GetCurrentInternalState_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_CanSm_GetCurrentInternalState_NullPtr_ReportsParamPointer);
    RUN_TEST(test_CanSm_GetCurrentInternalState_InvalidNetwork_ReportsParamNetwork);
    RUN_TEST(test_CanSm_ControllerModeIndication_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_CanSm_ControllerModeIndication_UnknownController_ReportsParamController);
    RUN_TEST(test_CanSm_ControllerModeIndication_StartedFromNoCom_ReachesFullCom);
    RUN_TEST(test_CanSm_ControllerModeIndication_StoppedFromFullCom_ReachesSilentCom);
    RUN_TEST(test_CanSm_ControllerModeIndication_StoppedThenStarted_RoundTripToFullCom);
    RUN_TEST(test_CanSm_ConfirmPnAvailability_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_CanSm_ConfirmPnAvailability_InvalidNetwork_ReportsParamNetwork);
    RUN_TEST(test_CanSm_ConfirmPnAvailability_NoCom_ReportsInvalidNetworkMode);
    RUN_TEST(test_CanSm_ConfirmPnAvailability_FullCom_ReturnsOkWithoutDet);
    RUN_TEST(test_CanSm_ClearTrcvWufFlagIndication_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_CanSm_ClearTrcvWufFlagIndication_InvalidNetwork_ReportsParamNetwork);
    RUN_TEST(test_CanSm_ClearTrcvWufFlagIndication_NoCom_ReportsInvalidNetworkMode);
    RUN_TEST(test_CanSm_ClearTrcvWufFlagIndication_FullCom_ReturnsOkWithoutDet);

    return UnityEnd();
}

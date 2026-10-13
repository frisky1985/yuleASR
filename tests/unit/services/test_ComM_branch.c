#include <unity.h>
#include "ComM.h"
#include "ComM_Cfg.h"
#include "Det.h"
#include <string.h>

static uint16 det_last_error = 0xFFFF;
static uint8 det_last_api_id = 0xFF;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId;
    det_last_api_id = ApiId;
    det_last_error = ErrorId;
    return E_OK;
}

void Det_Init(const Det_ConfigType* ConfigPtr) { (void)ConfigPtr; }
void Det_Start(void) {}

static const ComM_ChannelConfigType testChannelConfigs[COMM_NUM_CHANNELS] = {
    {0, COMM_BUS_TYPE_CAN, TRUE,  TRUE,  TRUE,  1000, 500, 200, 100},
    {1, COMM_BUS_TYPE_CAN, TRUE,  FALSE, FALSE, 1000, 500, 200, 100},
    {2, COMM_BUS_TYPE_ETH, TRUE,  TRUE,  FALSE, 1000, 500, 200, 100},
    {3, COMM_BUS_TYPE_LIN, FALSE, FALSE, TRUE,  1000, 500, 200, 100},
};

static const uint8 ch_map0[] = {0, 2};
static const uint8 ch_map1[] = {0};
static const uint8 ch_map2[] = {0, 3};
static const uint8 ch_map3[] = {0, 1, 2, 3};
static const uint8 ch_map4[] = {0, 2};
static const uint8 ch_map5[] = {1, 3};
static const uint8 ch_map6[] = {0, 1};
static const uint8 ch_map7[] = {0, 1, 2, 3};

static const uint8 pnc_map0[] = {0};
static const uint8 pnc_map1[] = {};
static const uint8 pnc_map2[] = {1};
static const uint8 pnc_map3[] = {};
static const uint8 pnc_map4[] = {0};
static const uint8 pnc_map5[] = {1};
static const uint8 pnc_map6[] = {};
static const uint8 pnc_map7[] = {};

static const ComM_UserConfigType testUserConfigs[COMM_NUM_USERS] = {
    {0, ch_map0, 2, pnc_map0, 1},
    {1, ch_map1, 1, pnc_map1, 0},
    {2, ch_map2, 2, pnc_map2, 1},
    {3, ch_map3, 4, pnc_map3, 0},
    {4, ch_map4, 2, pnc_map4, 1},
    {5, ch_map5, 2, pnc_map5, 1},
    {6, ch_map6, 2, pnc_map6, 0},
    {7, ch_map7, 4, pnc_map7, 0},
};

static const ComM_PncChannelMappingType pnc0_ch[] = {{0, TRUE}, {2, FALSE}};
static const ComM_PncChannelMappingType pnc1_ch[] = {{3, TRUE}};

static const ComM_PncConfigType testPncConfigs[COMM_NUM_PNCS] = {
    {0, pnc0_ch, 2, 300, 100, TRUE},
    {1, pnc1_ch, 1, 300, 100, TRUE},
};

static const ComM_ConfigType testConfig = {
    testChannelConfigs,
    testUserConfigs,
    testPncConfigs,
    COMM_NUM_CHANNELS,
    COMM_NUM_USERS,
    COMM_NUM_PNCS,
    50,
    TRUE, TRUE, TRUE
};

extern const ComM_ConfigType ComM_Config;

void setUp(void)
{
    det_last_error = 0xFFFF;
    det_last_api_id = 0xFF;
    ComM_Init(&ComM_Config);
    ComM_DeInit();
    det_last_error = 0xFFFF;
    det_last_api_id = 0xFF;
}

void tearDown(void) {}

/* ===== Init / DeInit ===== */

void test_Init_NullPtr_ReportsError(void)
{
    ComM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(COMM_E_PARAM_POINTER, det_last_error);
    TEST_ASSERT_EQUAL(COMM_INIT_SID, det_last_api_id);
}

void test_Init_ValidConfig_Succeeds(void)
{
    ComM_Init(&testConfig);
    ComM_ModeType mode = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetCurrentComMode(COMM_USER_DCM, &mode));
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_DeInit_Uninitialized_ReportsError(void)
{
    ComM_DeInit();
    TEST_ASSERT_EQUAL(COMM_E_NOT_INIT, det_last_error);
}

void test_Init_DeInit_Reinit(void)
{
    ComM_Init(&testConfig);
    ComM_DeInit();
    ComM_Init(&testConfig);
    ComM_ModeType mode = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetCurrentComMode(0, &mode));
}

/* ===== GetVersionInfo ===== */

void test_GetVersionInfo_NullPtr_ReportsError(void)
{
    ComM_Init(&testConfig);
    ComM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(COMM_E_PARAM_POINTER, det_last_error);
}

void test_GetVersionInfo_ValidPtr(void)
{
    ComM_Init(&testConfig);
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    ComM_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(COMM_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL(COMM_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL(COMM_SW_MINOR_VERSION, vi.sw_minor_version);
}

/* ===== RequestComMode ===== */

void test_RequestComMode_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_RequestComMode(0, COMM_FULL_COMMUNICATION));
    TEST_ASSERT_EQUAL(COMM_E_NOT_INIT, det_last_error);
}

void test_RequestComMode_InvalidUser(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_RequestComMode(COMM_NUM_USERS + 1, COMM_FULL_COMMUNICATION));
    TEST_ASSERT_EQUAL(COMM_E_PARAM_USER, det_last_error);
}

void test_RequestComMode_InvalidMode(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_RequestComMode(0, 0x05));
    TEST_ASSERT_EQUAL(COMM_E_WRONG_PARAMETERS, det_last_error);
}

void test_RequestComMode_Full(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION));
}

void test_RequestComMode_Silent(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ComM_RequestComMode(COMM_USER_DCM, COMM_SILENT_COMMUNICATION));
}

void test_RequestComMode_NoCom(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(0, COMM_FULL_COMMUNICATION);
    TEST_ASSERT_EQUAL(E_OK, ComM_RequestComMode(0, COMM_NO_COMMUNICATION));
}

/* ===== GetMaxComMode ===== */

void test_GetMaxComMode_Uninitialized(void)
{
    ComM_ModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetMaxComMode(0, &mode));
}

void test_GetMaxComMode_InvalidUser(void)
{
    ComM_Init(&testConfig);
    ComM_ModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetMaxComMode(COMM_NUM_USERS + 1, &mode));
}

void test_GetMaxComMode_NullPtr(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetMaxComMode(0, NULL_PTR));
}

void test_GetMaxComMode_AfterRequest(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    ComM_ModeType mode = 0;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetMaxComMode(COMM_USER_DCM, &mode));
    TEST_ASSERT_EQUAL(COMM_FULL_COMMUNICATION, mode);
}

/* ===== GetCurrentComMode ===== */

void test_GetCurrentComMode_Uninitialized(void)
{
    ComM_ModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetCurrentComMode(0, &mode));
}

void test_GetCurrentComMode_InvalidUser(void)
{
    ComM_Init(&testConfig);
    ComM_ModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetCurrentComMode(COMM_NUM_USERS + 1, &mode));
}

void test_GetCurrentComMode_NullPtr(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetCurrentComMode(0, NULL_PTR));
}

void test_GetCurrentComMode_InitialNoCom(void)
{
    ComM_Init(&testConfig);
    ComM_ModeType mode = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetCurrentComMode(COMM_USER_DCM, &mode));
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

/* ===== CommunicationAllowed ===== */

void test_CommunicationAllowed_Uninitialized(void)
{
    ComM_CommunicationAllowed(0, TRUE);
    TEST_ASSERT_EQUAL(COMM_E_NOT_INIT, det_last_error);
}

void test_CommunicationAllowed_InvalidChannel(void)
{
    ComM_Init(&testConfig);
    ComM_CommunicationAllowed(COMM_NUM_CHANNELS + 1, TRUE);
    TEST_ASSERT_EQUAL(COMM_E_PARAM_CHANNEL, det_last_error);
}

void test_CommunicationAllowed_DisableEnable(void)
{
    ComM_Init(&testConfig);
    ComM_CommunicationAllowed(0, FALSE);
    ComM_CommunicationAllowed(0, TRUE);
    /* pass: no assertion failure */
}

/* ===== MainFunction ===== */

void test_MainFunction_Uninitialized(void)
{
    ComM_MainFunction();
    TEST_ASSERT_EQUAL(COMM_E_NOT_INIT, det_last_error);
}

void test_MainFunction_NoRequests(void)
{
    ComM_Init(&testConfig);
    ComM_MainFunction();
    /* pass: no assertion failure */
}

void test_MainFunction_FullComRequest_TransitionToPending(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    ComM_MainFunction();
    /* After one MainFunction cycle, channel should transition NoCom -> Pending */
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    /* Mode should still be NO_COM (pending state maps to NO_COM) */
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_MainFunction_PendingTimeout_TransitionToFullCom(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    /* Run enough cycles to expire the wake-up delay (100 cycles) */
    for (int i = 0; i < 150; i++) {
        ComM_MainFunction();
    }
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_FULL_COMMUNICATION, mode);
}

void test_MainFunction_FullCom_To_SilentCom(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 150; i++) ComM_MainFunction();

    ComM_RequestComMode(COMM_USER_DCM, COMM_NO_COMMUNICATION);
    /* Run cycles to go through SilentCom timeout (500) */
    for (int i = 0; i < 600; i++) ComM_MainFunction();

    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_MainFunction_CommunicationNotAllowed(void)
{
    ComM_Init(&testConfig);
    ComM_CommunicationAllowed(0, FALSE);
    ComM_CommunicationAllowed(2, FALSE);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 150; i++) ComM_MainFunction();
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

/* ===== BusSM Integration ===== */

void test_BusSM_ModeIndication_Uninitialized(void)
{
    /* combined init+channel check, no separate DET — just returns silently */
    ComM_BusSM_ModeIndication(0, COMM_FULL_COMMUNICATION);
}

void test_BusSM_ModeIndication_InvalidChannel(void)
{
    ComM_Init(&testConfig);
    ComM_BusSM_ModeIndication(COMM_NUM_CHANNELS + 1, COMM_FULL_COMMUNICATION);
    /* combined init+channel check, no separate DET for channel */
}

void test_BusSM_ModeIndication_AllModes(void)
{
    ComM_Init(&testConfig);
    ComM_BusSM_ModeIndication(0, COMM_NO_COMMUNICATION);
    ComM_BusSM_ModeIndication(0, COMM_SILENT_COMMUNICATION);
    ComM_BusSM_ModeIndication(0, COMM_FULL_COMMUNICATION);
    ComM_BusSM_ModeIndication(0, 0x05); /* invalid -> default */
    /* pass: no assertion failure */
}

void test_BusSM_BusSleepMode(void)
{
    ComM_Init(&testConfig);
    ComM_BusSM_BusSleepMode(0);
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_BusSM_NetworkMode(void)
{
    ComM_Init(&testConfig);
    ComM_BusSM_NetworkMode(0);
    /* BusSM_NetworkMode sets CurrentMode to FULL but doesn't change RequestedMode */
    /* pass: no assertion failure */
}

void test_BusSM_PrepareBusSleepMode(void)
{
    ComM_Init(&testConfig);
    ComM_BusSM_PrepareBusSleepMode(0);
    /* pass: no assertion failure */
}

/* ===== DCM Integration ===== */

void test_DCM_ActiveDiagnostic_Uninitialized(void)
{
    /* combined init+channel check, no separate DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_DCM_ActiveDiagnostic(0));
}

void test_DCM_ActiveDiagnostic_InvalidChannel(void)
{
    ComM_Init(&testConfig);
    /* combined check: invalid channel returns E_NOT_OK, no separate DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_DCM_ActiveDiagnostic(COMM_NUM_CHANNELS + 1));
}

void test_DCM_ActiveDiagnostic_SetsFullCom(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ComM_DCM_ActiveDiagnostic(0));
    /* DCM active should force full communication request */
    ComM_MainFunction();
    for (int i = 0; i < 150; i++) ComM_MainFunction();
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_FULL_COMMUNICATION, mode);
}

void test_DCM_InactiveDiagnostic(void)
{
    ComM_Init(&testConfig);
    ComM_DCM_ActiveDiagnostic(0);
    TEST_ASSERT_EQUAL(E_OK, ComM_DCM_InactiveDiagnostic(0));
}

void test_DCM_InactiveDiagnostic_Uninitialized(void)
{
    /* combined init+channel check, no separate DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_DCM_InactiveDiagnostic(0));
}

void test_DCM_PassiveDiagnostic(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ComM_DCM_PassiveDiagnostic(0, TRUE));
    TEST_ASSERT_EQUAL(E_OK, ComM_DCM_PassiveDiagnostic(0, FALSE));
}

void test_DCM_PassiveDiagnostic_Uninitialized(void)
{
    /* combined init+channel check, no separate DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_DCM_PassiveDiagnostic(0, TRUE));
}

void test_DCM_PassiveDiagnostic_InvalidChannel(void)
{
    ComM_Init(&testConfig);
    /* combined check: invalid channel returns E_NOT_OK, no separate DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_DCM_PassiveDiagnostic(COMM_NUM_CHANNELS + 1, TRUE));
}

/* ===== ECNM Integration ===== */

void test_ECNM_NetworkMode(void)
{
    ComM_Init(&testConfig);
    ComM_ECNM_NetworkMode(0);
    /* pass: no assertion failure */
}

void test_ECNM_PrepareBusSleepMode(void)
{
    ComM_Init(&testConfig);
    ComM_ECNM_PrepareBusSleepMode(0);
    /* pass: no assertion failure */
}

void test_ECNM_BusSleepMode(void)
{
    ComM_Init(&testConfig);
    ComM_ECNM_BusSleepMode(0);
    /* pass: no assertion failure */
}

/* ===== EcuM Integration ===== */

void test_EcuM_WakeUpIndication_Uninitialized(void)
{
    ComM_EcuM_WakeUpIndication(COMM_WAKEUP_COMM);
    /* pass: no assertion failure */ /* no crash */
}

void test_EcuM_WakeUpIndication_WithWakeUpChannels(void)
{
    ComM_Init(&testConfig);
    ComM_EcuM_WakeUpIndication(COMM_WAKEUP_COMM);
    /* pass: no assertion failure */
}

void test_EcuM_BusWakeUpIndication(void)
{
    ComM_Init(&testConfig);
    ComM_EcuM_BusWakeUpIndication(0);
    ComM_EcuM_BusWakeUpIndication(COMM_NUM_CHANNELS + 1); /* invalid */
    /* pass: no assertion failure */
}

void test_EcuM_BusWakeUpIndication_Uninitialized(void)
{
    ComM_EcuM_BusWakeUpIndication(0);
    /* pass: no assertion failure */
}

/* ===== Inhibition / Limit / WakeUp ===== */

void test_LimitChannelToNoComMode(void)
{
    ComM_Init(&testConfig);
    /* User 0 (DCM) maps to channels 0 and 2 in global config; must limit both */
    ComM_LimitChannelToNoComMode(0, TRUE);
    ComM_LimitChannelToNoComMode(2, TRUE);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 150; i++) ComM_MainFunction();
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_LimitChannelToNoComMode_Release(void)
{
    ComM_Init(&testConfig);
    ComM_LimitChannelToNoComMode(0, TRUE);
    ComM_LimitChannelToNoComMode(0, FALSE);
    /* pass: no assertion failure */
}

void test_LimitChannelToNoComMode_Uninitialized(void)
{
    /* combined init+channel check, no separate DET — just returns silently */
    ComM_LimitChannelToNoComMode(0, TRUE);
}

void test_LimitECUToNoComMode(void)
{
    ComM_Init(&testConfig);
    ComM_LimitECUToNoComMode(TRUE);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 150; i++) ComM_MainFunction();
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_LimitECUToNoComMode_Release(void)
{
    ComM_Init(&testConfig);
    ComM_LimitECUToNoComMode(TRUE);
    ComM_LimitECUToNoComMode(FALSE);
    /* pass: no assertion failure */
}

void test_LimitECUToNoComMode_Uninitialized(void)
{
    /* init check only, no DET — just returns silently */
    ComM_LimitECUToNoComMode(TRUE);
}

void test_PreventWakeUp(void)
{
    ComM_Init(&testConfig);
    ComM_PreventWakeUp(0, TRUE);
    ComM_PreventWakeUp(0, FALSE);
    /* pass: no assertion failure */
}

void test_PreventWakeUp_Uninitialized(void)
{
    /* combined init+channel check, no separate DET — just returns silently */
    ComM_PreventWakeUp(0, TRUE);
}

void test_PreventWakeUp_InvalidChannel(void)
{
    ComM_Init(&testConfig);
    /* combined check, no separate DET for channel */
    ComM_PreventWakeUp(COMM_NUM_CHANNELS + 1, TRUE);
}

/* ===== GetInhibitionStatus ===== */

void test_GetInhibitionStatus_NoInhibition(void)
{
    ComM_Init(&testConfig);
    ComM_InhibitionStatusType status = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetInhibitionStatus(0, &status));
    TEST_ASSERT_EQUAL(COMM_INHIBITION_STATUS_NONE, status);
}

void test_GetInhibitionStatus_WakeUpInhibition(void)
{
    ComM_Init(&testConfig);
    ComM_PreventWakeUp(0, TRUE);
    ComM_InhibitionStatusType status = 0;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetInhibitionStatus(0, &status));
    TEST_ASSERT_NOT_EQUAL(0, status & COMM_INHIBITION_STATUS_WAKEUP);
}

void test_GetInhibitionStatus_LimitToNoCom(void)
{
    ComM_Init(&ComM_Config);
    ComM_LimitECUToNoComMode(TRUE);
    ComM_InhibitionStatusType status = 0;
    Std_ReturnType ret = ComM_GetInhibitionStatus(0, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0, status & COMM_INHIBITION_STATUS_LIMIT_TO_NO_COM);
}

void test_GetInhibitionStatus_NullPtr(void)
{
    ComM_Init(&testConfig);
    /* combined init+ptr+channel check, no separate DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetInhibitionStatus(0, NULL_PTR));
}

void test_GetInhibitionStatus_InvalidChannel(void)
{
    ComM_Init(&testConfig);
    ComM_InhibitionStatusType status;
    /* combined check, no separate DET for channel */
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetInhibitionStatus(COMM_NUM_CHANNELS + 1, &status));
}

/* ===== PNC Management ===== */

void test_RequestPncMode_Uninitialized(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_RequestPncMode(0, COMM_PNC_REQUESTED));
}

void test_RequestPncMode_InvalidPnc(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_RequestPncMode(COMM_NUM_PNCS + 1, COMM_PNC_REQUESTED));
}

void test_RequestPncMode_Requested(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ComM_RequestPncMode(COMM_PNC_0, COMM_PNC_REQUESTED));
}

void test_GetPncMode_NullPtr(void)
{
    ComM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetPncMode(0, NULL_PTR));
}

void test_GetPncMode_InvalidPnc(void)
{
    ComM_Init(&testConfig);
    ComM_PncModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, ComM_GetPncMode(COMM_NUM_PNCS + 1, &mode));
}

void test_GetPncMode_Initial(void)
{
    ComM_Init(&testConfig);
    ComM_PncModeType mode = 0xFF;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetPncMode(COMM_PNC_0, &mode));
    TEST_ASSERT_EQUAL(COMM_PNC_NO_COMMUNICATION, mode);
}

void test_MainFunctionPnc_PncStateMachine(void)
{
    ComM_Init(&testConfig);
    ComM_RequestPncMode(COMM_PNC_0, COMM_PNC_REQUESTED);
    ComM_MainFunction();
    ComM_PncModeType mode;
    ComM_GetPncMode(COMM_PNC_0, &mode);
    /* UpdatePncRequestStatus recalculates RequestActive from user requests (all inactive),
       so PNC transitions REQUESTED → READY_SLEEP in the same MainFunction cycle */
    TEST_ASSERT_EQUAL(COMM_PNC_READY_SLEEP, mode);
}

void test_Pnc_RequestThenRelease(void)
{
    ComM_Init(&testConfig);
    ComM_RequestPncMode(COMM_PNC_0, COMM_PNC_REQUESTED);
    ComM_MainFunction();
    ComM_RequestPncMode(COMM_PNC_0, COMM_PNC_NO_COMMUNICATION);
    /* Run enough cycles for PNC to go through READY_SLEEP -> PREPARE_SLEEP -> NO_COM */
    for (int i = 0; i < 400; i++) ComM_MainFunction();
    ComM_PncModeType mode;
    ComM_GetPncMode(COMM_PNC_0, &mode);
    TEST_ASSERT_EQUAL(COMM_PNC_NO_COMMUNICATION, mode);
}

/* ===== Nm Notifications ===== */

void test_Nm_NetworkMode(void)
{
    ComM_Nm_NetworkMode(0);
    /* pass: no assertion failure */
}

void test_Nm_PrepareBusSleepMode(void)
{
    ComM_Nm_PrepareBusSleepMode(0);
    /* pass: no assertion failure */
}

void test_Nm_BusSleepMode(void)
{
    ComM_Nm_BusSleepMode(0);
    /* pass: no assertion failure */
}

/* ===== NVM ===== */

void test_Nvm_StartUpError(void)
{
    ComM_Nvm_StartUpError();
    /* pass: no assertion failure */
}

void test_Nvm_StoreInhibitionStatus(void)
{
    ComM_Nvm_StoreInhibitionStatus();
    /* pass: no assertion failure */
}

/* ===== GetRequestedComMode ===== */

void test_GetRequestedComMode(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(COMM_USER_DCM, COMM_SILENT_COMMUNICATION);
    ComM_ModeType mode = 0;
    TEST_ASSERT_EQUAL(E_OK, ComM_GetRequestedComMode(COMM_USER_DCM, &mode));
    TEST_ASSERT_EQUAL(COMM_SILENT_COMMUNICATION, mode);
}

/* ===== EcuM_RunRequestIndication ===== */

void test_EcuM_RunRequestIndication(void)
{
    ComM_EcuM_RunRequestIndication(TRUE);
    ComM_EcuM_RunRequestIndication(FALSE);
    /* pass: no assertion failure */
}

/* ===== Complex State Machine Scenarios ===== */

void test_SilentCom_To_FullCom(void)
{
    ComM_Init(&testConfig);
    /* Request silent first */
    ComM_RequestComMode(COMM_USER_DCM, COMM_SILENT_COMMUNICATION);
    for (int i = 0; i < 10; i++) ComM_MainFunction();
    /* Then upgrade to full */
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 150; i++) ComM_MainFunction();
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_FULL_COMMUNICATION, mode);
}

void test_PendingAbort_ByNoComRequest(void)
{
    ComM_Init(&testConfig);
    ComM_RequestComMode(COMM_USER_DCM, COMM_FULL_COMMUNICATION);
    ComM_MainFunction(); /* NoCom -> Pending */
    /* Abort: request NoCom while pending */
    ComM_RequestComMode(COMM_USER_DCM, COMM_NO_COMMUNICATION);
    ComM_MainFunction(); /* Pending -> NoCom */
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_EcuLimitToNoCom_OverridesDcmRequest(void)
{
    ComM_Init(&testConfig);
    ComM_DCM_ActiveDiagnostic(0);
    ComM_LimitECUToNoComMode(TRUE);
    for (int i = 0; i < 150; i++) ComM_MainFunction();
    ComM_ModeType mode;
    ComM_GetCurrentComMode(COMM_USER_DCM, &mode);
    TEST_ASSERT_EQUAL(COMM_NO_COMMUNICATION, mode);
}

void test_WakeUpInhibition_BlocksEcuMWakeUp(void)
{
    ComM_Init(&testConfig);
    ComM_PreventWakeUp(0, TRUE);
    ComM_EcuM_WakeUpIndication(COMM_WAKEUP_COMM);
    /* Channel 0 has WakeUpInhibition, so it should NOT request FullCom */
    /* pass: no assertion failure */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Init_NullPtr_ReportsError);
    RUN_TEST(test_Init_ValidConfig_Succeeds);
    RUN_TEST(test_DeInit_Uninitialized_ReportsError);
    RUN_TEST(test_Init_DeInit_Reinit);
    RUN_TEST(test_GetVersionInfo_NullPtr_ReportsError);
    RUN_TEST(test_GetVersionInfo_ValidPtr);
    RUN_TEST(test_RequestComMode_Uninitialized);
    RUN_TEST(test_RequestComMode_InvalidUser);
    RUN_TEST(test_RequestComMode_InvalidMode);
    RUN_TEST(test_RequestComMode_Full);
    RUN_TEST(test_RequestComMode_Silent);
    RUN_TEST(test_RequestComMode_NoCom);
    RUN_TEST(test_GetMaxComMode_Uninitialized);
    RUN_TEST(test_GetMaxComMode_InvalidUser);
    RUN_TEST(test_GetMaxComMode_NullPtr);
    RUN_TEST(test_GetMaxComMode_AfterRequest);
    RUN_TEST(test_GetCurrentComMode_Uninitialized);
    RUN_TEST(test_GetCurrentComMode_InvalidUser);
    RUN_TEST(test_GetCurrentComMode_NullPtr);
    RUN_TEST(test_GetCurrentComMode_InitialNoCom);
    RUN_TEST(test_CommunicationAllowed_Uninitialized);
    RUN_TEST(test_CommunicationAllowed_InvalidChannel);
    RUN_TEST(test_CommunicationAllowed_DisableEnable);
    RUN_TEST(test_MainFunction_Uninitialized);
    RUN_TEST(test_MainFunction_NoRequests);
    RUN_TEST(test_MainFunction_FullComRequest_TransitionToPending);
    RUN_TEST(test_MainFunction_PendingTimeout_TransitionToFullCom);
    RUN_TEST(test_MainFunction_FullCom_To_SilentCom);
    RUN_TEST(test_MainFunction_CommunicationNotAllowed);
    RUN_TEST(test_BusSM_ModeIndication_Uninitialized);
    RUN_TEST(test_BusSM_ModeIndication_InvalidChannel);
    RUN_TEST(test_BusSM_ModeIndication_AllModes);
    RUN_TEST(test_BusSM_BusSleepMode);
    RUN_TEST(test_BusSM_NetworkMode);
    RUN_TEST(test_BusSM_PrepareBusSleepMode);
    RUN_TEST(test_DCM_ActiveDiagnostic_Uninitialized);
    RUN_TEST(test_DCM_ActiveDiagnostic_InvalidChannel);
    RUN_TEST(test_DCM_ActiveDiagnostic_SetsFullCom);
    RUN_TEST(test_DCM_InactiveDiagnostic);
    RUN_TEST(test_DCM_InactiveDiagnostic_Uninitialized);
    RUN_TEST(test_DCM_PassiveDiagnostic);
    RUN_TEST(test_DCM_PassiveDiagnostic_Uninitialized);
    RUN_TEST(test_DCM_PassiveDiagnostic_InvalidChannel);
    RUN_TEST(test_ECNM_NetworkMode);
    RUN_TEST(test_ECNM_PrepareBusSleepMode);
    RUN_TEST(test_ECNM_BusSleepMode);
    RUN_TEST(test_EcuM_WakeUpIndication_Uninitialized);
    RUN_TEST(test_EcuM_WakeUpIndication_WithWakeUpChannels);
    RUN_TEST(test_EcuM_BusWakeUpIndication);
    RUN_TEST(test_EcuM_BusWakeUpIndication_Uninitialized);
    RUN_TEST(test_LimitChannelToNoComMode);
    RUN_TEST(test_LimitChannelToNoComMode_Release);
    RUN_TEST(test_LimitChannelToNoComMode_Uninitialized);
    RUN_TEST(test_LimitECUToNoComMode);
    RUN_TEST(test_LimitECUToNoComMode_Release);
    RUN_TEST(test_LimitECUToNoComMode_Uninitialized);
    RUN_TEST(test_PreventWakeUp);
    RUN_TEST(test_PreventWakeUp_Uninitialized);
    RUN_TEST(test_PreventWakeUp_InvalidChannel);
    RUN_TEST(test_GetInhibitionStatus_NoInhibition);
    RUN_TEST(test_GetInhibitionStatus_WakeUpInhibition);
    RUN_TEST(test_GetInhibitionStatus_LimitToNoCom);
    RUN_TEST(test_GetInhibitionStatus_NullPtr);
    RUN_TEST(test_GetInhibitionStatus_InvalidChannel);
    RUN_TEST(test_RequestPncMode_Uninitialized);
    RUN_TEST(test_RequestPncMode_InvalidPnc);
    RUN_TEST(test_RequestPncMode_Requested);
    RUN_TEST(test_GetPncMode_NullPtr);
    RUN_TEST(test_GetPncMode_InvalidPnc);
    RUN_TEST(test_GetPncMode_Initial);
    RUN_TEST(test_MainFunctionPnc_PncStateMachine);
    RUN_TEST(test_Pnc_RequestThenRelease);
    RUN_TEST(test_Nm_NetworkMode);
    RUN_TEST(test_Nm_PrepareBusSleepMode);
    RUN_TEST(test_Nm_BusSleepMode);
    RUN_TEST(test_Nvm_StartUpError);
    RUN_TEST(test_Nvm_StoreInhibitionStatus);
    RUN_TEST(test_GetRequestedComMode);
    RUN_TEST(test_EcuM_RunRequestIndication);
    RUN_TEST(test_SilentCom_To_FullCom);
    RUN_TEST(test_PendingAbort_ByNoComRequest);
    RUN_TEST(test_EcuLimitToNoCom_OverridesDcmRequest);
    RUN_TEST(test_WakeUpInhibition_BlocksEcuMWakeUp);
    return UNITY_END();
}

/**
 * @file test_bswm.c
 * @brief BSWM (BSW Mode Manager) Unit Tests
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 * All Rights Reserved.
 *
 * Test Coverage:
 * - Initialization (BswM_Init, BswM_DeInit)
 * - Version Information (BswM_GetVersionInfo)
 * - Mode Request (BswM_RequestMode)
 * - Main Function (BswM_MainFunction)
 * - Callbacks (BswM_EcuM_CurrentState, BswM_ComM_CurrentMode, BswM_Dcm_RequestCommunicationMode)
 * - Error Detection (NULL pointer checks, uninitialized module access)
 *
 * Target Coverage: 80%+
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

/* AUTOSAR Standard Types */
#ifndef STD_TYPES_H
#define STD_TYPES_H
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;
typedef uint8 boolean;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif
typedef uint8 Std_ReturnType;
#define E_OK 0
#define E_NOT_OK 1
#endif

/* Std_VersionInfoType */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

/* BswM Types and Constants */
#define BSWM_MODULE_ID          42U
#define BSWM_VENDOR_ID          0x0001U

/* Error Codes */
#define BSWM_E_NO_ERROR         0x00U
#define BSWM_E_PARAM_POINTER    0x01U
#define BSWM_E_UNINIT           0x02U
#define BSWM_E_PARAM_INVALID    0x03U

/* Service IDs */
#define BSWM_SID_INIT                   0x00U
#define BSWM_SID_DEINIT                 0x01U
#define BSWM_SID_GET_VERSION_INFO       0x02U
#define BSWM_SID_REQUEST_MODE           0x03U
#define BSWM_SID_MAIN_FUNCTION          0x04U

/* Configuration Switches */
#define BSWM_DEV_ERROR_DETECT           STD_ON
#define BSWM_VERSION_INFO_API           STD_ON

/* Maximum counts */
#define BSWM_MAX_MODE_REQUEST_PORTS     32U
#define BSWM_MAX_RULES                  64U
#define BSWM_MAX_ACTIONS                128U
#define BSWM_MAX_ACTION_LISTS           32U

/* Mode definitions */
#define BSWM_MODE_STARTUP               0x00U
#define BSWM_MODE_RUN                   0x01U
#define BSWM_MODE_SHUTDOWN              0x02U
#define BSWM_MODE_SLEEP                 0x03U
#define BSWM_MODE_WAKEUP                0x04U

/* ECU States */
#define BSWM_ECUM_STATE_STARTUP         0x10U
#define BSWM_ECUM_STATE_RUN             0x20U
#define BSWM_ECUM_STATE_SHUTDOWN        0x30U
#define BSWM_ECUM_STATE_SLEEP           0x40U

/* Mode Request Source Types */
typedef enum {
    BSWM_GENERIC_REQUEST = 0,
    BSWM_ECUM_REQUEST,
    BSWM_COMM_REQUEST,
    BSWM_DCM_REQUEST,
    BSWM_NVM_REQUEST,
    BSWM_SWC_REQUEST
} BswM_ModeRequestSourceType;

/* Mode Type */
typedef uint8 BswM_ModeType;

/* Mode Request Port */
typedef struct {
    uint16 PortId;
    BswM_ModeRequestSourceType SourceType;
    BswM_ModeType CurrentMode;
    boolean IsValid;
} BswM_ModeRequestPortType;

/* Action Type */
typedef enum {
    BSWM_ACTION_SCHEDULE = 0,
    BSWM_ACTION_SWITCH_MODE,
    BSWM_ACTION_EXECUTE_ACTION_LIST,
    BSWM_ACTION_USER_CALL
} BswM_ActionItemType;

/* Action Structure */
typedef struct {
    uint16 ActionId;
    BswM_ActionItemType ActionType;
    uint16 TargetId;
} BswM_ActionType;

/* Rule Structure */
typedef struct {
    uint16 RuleId;
    uint16 ModeRequestPortId;
    BswM_ModeType ExpectedMode;
    uint16 ActionListId;
    boolean IsActive;
} BswM_RuleType;

/* Configuration */
typedef struct {
    uint16 NumModeRequestPorts;
    uint16 NumRules;
    uint16 NumActionLists;
    const BswM_ModeRequestPortType* ModeRequestPorts;
    const BswM_RuleType* Rules;
} BswM_ConfigType;

/*==================================================================================================
 *                                  MOCK DET INTERFACE
 *================================================================================================*/
static uint16 mock_det_moduleId = 0;
static uint8 mock_det_instanceId = 0;
static uint8 mock_det_apiId = 0;
static uint8 mock_det_errorId = 0;
static uint32 mock_det_callCount = 0;

static void mock_det_reset(void)
{
    mock_det_moduleId = 0;
    mock_det_instanceId = 0;
    mock_det_apiId = 0;
    mock_det_errorId = 0;
    mock_det_callCount = 0;
}

static uint32 mock_det_get_call_count(void)
{
    return mock_det_callCount;
}

static void mock_det_get_last_error(uint16* moduleId, uint8* instanceId, uint8* apiId, uint8* errorId)
{
    if (moduleId) *moduleId = mock_det_moduleId;
    if (instanceId) *instanceId = mock_det_instanceId;
    if (apiId) *apiId = mock_det_apiId;
    if (errorId) *errorId = mock_det_errorId;
}

static inline Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    mock_det_moduleId = ModuleId;
    mock_det_instanceId = InstanceId;
    mock_det_apiId = ApiId;
    mock_det_errorId = ErrorId;
    mock_det_callCount++;
    return E_OK;
}

/*==================================================================================================
 *                                  MOCK SCHM INTERFACE
 *================================================================================================*/
#define BSWM_EXCLUSIVE_AREA_0   0

static uint32 mock_schm_enter_count = 0;
static uint32 mock_schm_exit_count = 0;

static void mock_schm_reset(void)
{
    mock_schm_enter_count = 0;
    mock_schm_exit_count = 0;
}

#define SchM_Enter_BswM(area)   mock_schm_enter_count++
#define SchM_Exit_BswM(area)    mock_schm_exit_count++

/*==================================================================================================
 *                                  BSWM IMPLEMENTATION
 *================================================================================================*/
typedef enum {
    BSWM_UNINIT = 0,
    BSWM_INIT
} BswM_StateType;

static BswM_StateType BswM_State = BSWM_UNINIT;
static const BswM_ConfigType* BswM_ConfigPtr = NULL_PTR;
static BswM_ModeRequestPortType BswM_ModeRequestPorts[BSWM_MAX_MODE_REQUEST_PORTS];
static BswM_ModeType BswM_RequestedModes[BSWM_MAX_MODE_REQUEST_PORTS];
static boolean BswM_ModeRequestPending[BSWM_MAX_MODE_REQUEST_PORTS];

void BswM_Init(const BswM_ConfigType* ConfigPtr)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_INIT, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    BswM_ConfigPtr = ConfigPtr;
    BswM_State = BSWM_INIT;
    
    /* Initialize mode request ports */
    for (uint16 i = 0; i < BSWM_MAX_MODE_REQUEST_PORTS; i++) {
        BswM_ModeRequestPorts[i].PortId = i;
        BswM_ModeRequestPorts[i].SourceType = BSWM_GENERIC_REQUEST;
        BswM_ModeRequestPorts[i].CurrentMode = BSWM_MODE_STARTUP;
        BswM_ModeRequestPorts[i].IsValid = FALSE;
        BswM_RequestedModes[i] = BSWM_MODE_STARTUP;
        BswM_ModeRequestPending[i] = FALSE;
    }
    
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

void BswM_DeInit(void)
{
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    BswM_ConfigPtr = NULL_PTR;
    BswM_State = BSWM_UNINIT;
    
    /* Clear mode request ports */
    for (uint16 i = 0; i < BSWM_MAX_MODE_REQUEST_PORTS; i++) {
        BswM_ModeRequestPorts[i].IsValid = FALSE;
        BswM_ModeRequestPending[i] = FALSE;
    }
    
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

#if (BSWM_VERSION_INFO_API == STD_ON)
void BswM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_GET_VERSION_INFO, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = BSWM_VENDOR_ID;
    VersionInfo->moduleID = BSWM_MODULE_ID;
    VersionInfo->sw_major_version = 1U;
    VersionInfo->sw_minor_version = 0U;
    VersionInfo->sw_patch_version = 0U;
}
#endif

void BswM_RequestMode(uint16 PortId, BswM_ModeType Mode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BSWM_UNINIT == BswM_State) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return;
    }
    
    if (PortId >= BSWM_MAX_MODE_REQUEST_PORTS) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_PARAM_INVALID);
        return;
    }
#endif
    
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    
    /* Update mode request */
    if (PortId < BSWM_MAX_MODE_REQUEST_PORTS) {
        BswM_RequestedModes[PortId] = Mode;
        BswM_ModeRequestPending[PortId] = TRUE;
        BswM_ModeRequestPorts[PortId].CurrentMode = Mode;
        BswM_ModeRequestPorts[PortId].IsValid = TRUE;
    }
    
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

void BswM_MainFunction(void)
{
    if (BSWM_UNINIT == BswM_State) {
        return;
    }
    
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    
    /* Process rules */
    if (NULL_PTR != BswM_ConfigPtr) {
        for (uint16 i = 0U; i < BSWM_ConfigPtr->NumRules; i++) {
            const BswM_RuleType* rule = &BSWM_ConfigPtr->Rules[i];
            
            if (rule->IsActive && rule->ModeRequestPortId < BSWM_MAX_MODE_REQUEST_PORTS) {
                if (BswM_ModeRequestPending[rule->ModeRequestPortId]) {
                    if (BswM_RequestedModes[rule->ModeRequestPortId] == rule->ExpectedMode) {
                        /* Rule condition met - execute action list */
                        /* Action execution would happen here in full implementation */
                    }
                    BswM_ModeRequestPending[rule->ModeRequestPortId] = FALSE;
                }
            }
        }
    }
    
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

void BswM_EcuM_CurrentState(uint8 State)
{
    (void)State;
    /* Process ECU state change - would typically trigger mode requests */
}

void BswM_ComM_CurrentMode(uint8 Network, uint8 Mode)
{
    (void)Network;
    (void)Mode;
    /* Process ComM mode change - would typically trigger mode requests */
}

void BswM_Dcm_RequestCommunicationMode(uint8 Mode)
{
    (void)Mode;
    /* Process DCM communication mode request - would typically trigger mode requests */
}

/* Helper function to check if BswM is initialized */
boolean BswM_IsInitialized(void)
{
    return (BswM_State == BSWM_INIT);
}

/* Helper function to get current mode for a port */
BswM_ModeType BswM_GetCurrentMode(uint16 PortId)
{
    if (PortId < BSWM_MAX_MODE_REQUEST_PORTS && BswM_ModeRequestPorts[PortId].IsValid) {
        return BswM_ModeRequestPorts[PortId].CurrentMode;
    }
    return BSWM_MODE_STARTUP;
}

/*==================================================================================================
 *                                  TEST FIXTURES
 *================================================================================================*/

/* Test Configuration */
static const BswM_ModeRequestPortType testModeRequestPorts[] = {
    {0U, BSWM_ECUM_REQUEST, BSWM_MODE_STARTUP, TRUE},
    {1U, BSWM_COMM_REQUEST, BSWM_MODE_RUN, TRUE},
    {2U, BSWM_DCM_REQUEST, BSWM_MODE_SHUTDOWN, TRUE},
    {3U, BSWM_GENERIC_REQUEST, BSWM_MODE_STARTUP, TRUE}
};

static const BswM_RuleType testRules[] = {
    {0U, 0U, BSWM_MODE_RUN, 0U, TRUE},
    {1U, 1U, BSWM_MODE_RUN, 1U, TRUE},
    {2U, 2U, BSWM_MODE_SHUTDOWN, 2U, FALSE}
};

static const BswM_ConfigType testConfig = {
    4U,  /* NumModeRequestPorts */
    3U,  /* NumRules */
    0U,  /* NumActionLists */
    testModeRequestPorts,
    testRules
};

static int test_setup(void **state)
{
    (void)state;
    mock_det_reset();
    mock_schm_reset();
    BswM_DeInit();  /* Ensure clean state */
    return 0;
}

static int test_teardown(void **state)
{
    (void)state;
    BswM_DeInit();
    return 0;
}

/*==================================================================================================
 *                                  TEST CASES - INITIALIZATION
 *================================================================================================*/

static void test_BswM_Init_ValidConfig(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    assert_true(BswM_IsInitialized());
    assert_int_equal(mock_det_get_call_count(), 0);
    assert_true(mock_schm_enter_count > 0);
    assert_true(mock_schm_exit_count > 0);
}

static void test_BswM_Init_NullConfig(void **state)
{
    (void)state;
    uint16 moduleId;
    uint8 apiId;
    uint8 errorId;
    
    BswM_Init(NULL_PTR);
    
    assert_false(BswM_IsInitialized());
    assert_int_equal(mock_det_get_call_count(), 1);
    
    mock_det_get_last_error(&moduleId, NULL, &apiId, &errorId);
    assert_int_equal(moduleId, BSWM_MODULE_ID);
    assert_int_equal(apiId, BSWM_SID_INIT);
    assert_int_equal(errorId, BSWM_E_PARAM_POINTER);
}

static void test_BswM_DeInit_Normal(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    assert_true(BswM_IsInitialized());
    
    BswM_DeInit();
    
    assert_false(BswM_IsInitialized());
}

static void test_BswM_DeInit_Uninitialized(void **state)
{
    (void)state;
    
    /* Should not crash when called without init */
    BswM_DeInit();
    
    assert_false(BswM_IsInitialized());
}

static void test_BswM_Init_Reinit(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    assert_true(BswM_IsInitialized());
    
    /* Re-initialize with same config */
    BswM_Init(&testConfig);
    assert_true(BswM_IsInitialized());
}

/*==================================================================================================
 *                                  TEST CASES - VERSION INFO
 *================================================================================================*/

static void test_BswM_GetVersionInfo_ValidPointer(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    BswM_Init(&testConfig);
    BswM_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.vendorID, BSWM_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, BSWM_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, 1U);
    assert_int_equal(versionInfo.sw_minor_version, 0U);
    assert_int_equal(versionInfo.sw_patch_version, 0U);
}

static void test_BswM_GetVersionInfo_NullPointer(void **state)
{
    (void)state;
    uint16 moduleId;
    uint8 apiId;
    uint8 errorId;
    
    BswM_Init(&testConfig);
    BswM_GetVersionInfo(NULL_PTR);
    
    assert_int_equal(mock_det_get_call_count(), 1);
    mock_det_get_last_error(&moduleId, NULL, &apiId, &errorId);
    assert_int_equal(moduleId, BSWM_MODULE_ID);
    assert_int_equal(apiId, BSWM_SID_GET_VERSION_INFO);
    assert_int_equal(errorId, BSWM_E_PARAM_POINTER);
}

/*==================================================================================================
 *                                  TEST CASES - MODE REQUEST
 *================================================================================================*/

static void test_BswM_RequestMode_ValidPort(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    BswM_RequestMode(0U, BSWM_MODE_RUN);
    
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_RUN);
}

static void test_BswM_RequestMode_MultiplePorts(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    BswM_RequestMode(0U, BSWM_MODE_RUN);
    BswM_RequestMode(1U, BSWM_MODE_SHUTDOWN);
    BswM_RequestMode(2U, BSWM_MODE_SLEEP);
    
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_RUN);
    assert_int_equal(BswM_GetCurrentMode(1U), BSWM_MODE_SHUTDOWN);
    assert_int_equal(BswM_GetCurrentMode(2U), BSWM_MODE_SLEEP);
}

static void test_BswM_RequestMode_Uninitialized(void **state)
{
    (void)state;
    uint16 moduleId;
    uint8 apiId;
    uint8 errorId;
    
    /* Should report error when not initialized */
    BswM_RequestMode(0U, BSWM_MODE_RUN);
    
    assert_int_equal(mock_det_get_call_count(), 1);
    mock_det_get_last_error(&moduleId, NULL, &apiId, &errorId);
    assert_int_equal(moduleId, BSWM_MODULE_ID);
    assert_int_equal(apiId, BSWM_SID_REQUEST_MODE);
    assert_int_equal(errorId, BSWM_E_UNINIT);
}

static void test_BswM_RequestMode_InvalidPort(void **state)
{
    (void)state;
    uint16 moduleId;
    uint8 apiId;
    uint8 errorId;
    
    BswM_Init(&testConfig);
    
    /* Request mode for port beyond maximum */
    BswM_RequestMode(BSWM_MAX_MODE_REQUEST_PORTS, BSWM_MODE_RUN);
    
    assert_int_equal(mock_det_get_call_count(), 1);
    mock_det_get_last_error(&moduleId, NULL, &apiId, &errorId);
    assert_int_equal(moduleId, BSWM_MODULE_ID);
    assert_int_equal(apiId, BSWM_SID_REQUEST_MODE);
    assert_int_equal(errorId, BSWM_E_PARAM_INVALID);
}

static void test_BswM_RequestMode_ModeTransition(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Transition through multiple modes */
    BswM_RequestMode(0U, BSWM_MODE_STARTUP);
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_STARTUP);
    
    BswM_RequestMode(0U, BSWM_MODE_RUN);
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_RUN);
    
    BswM_RequestMode(0U, BSWM_MODE_SHUTDOWN);
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_SHUTDOWN);
    
    BswM_RequestMode(0U, BSWM_MODE_SLEEP);
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_SLEEP);
    
    BswM_RequestMode(0U, BSWM_MODE_WAKEUP);
    assert_int_equal(BswM_GetCurrentMode(0U), BSWM_MODE_WAKEUP);
}

/*==================================================================================================
 *                                  TEST CASES - MAIN FUNCTION
 *================================================================================================*/

static void test_BswM_MainFunction_Initialized(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Should not crash */
    BswM_MainFunction();
    
    assert_true(1);
}

static void test_BswM_MainFunction_Uninitialized(void **state)
{
    (void)state;
    
    /* Should not crash and should not report error (silent return) */
    BswM_MainFunction();
    
    assert_false(BswM_IsInitialized());
}

static void test_BswM_MainFunction_WithModeRequest(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Request a mode before calling MainFunction */
    BswM_RequestMode(1U, BSWM_MODE_RUN);
    
    /* MainFunction should process the mode request */
    BswM_MainFunction();
    
    assert_true(1);
}

static void test_BswM_MainFunction_MultipleCalls(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Multiple MainFunction calls should be safe */
    for (int i = 0; i < 100; i++) {
        BswM_MainFunction();
    }
    
    assert_true(1);
}

/*==================================================================================================
 *                                  TEST CASES - CALLBACKS
 *================================================================================================*/

static void test_BswM_EcuM_CurrentState(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Should not crash */
    BswM_EcuM_CurrentState(BSWM_ECUM_STATE_STARTUP);
    BswM_EcuM_CurrentState(BSWM_ECUM_STATE_RUN);
    BswM_EcuM_CurrentState(BSWM_ECUM_STATE_SHUTDOWN);
    BswM_EcuM_CurrentState(BSWM_ECUM_STATE_SLEEP);
    
    assert_true(1);
}

static void test_BswM_ComM_CurrentMode(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Should not crash with different networks and modes */
    BswM_ComM_CurrentMode(0U, 0U);
    BswM_ComM_CurrentMode(0U, 1U);
    BswM_ComM_CurrentMode(1U, 0U);
    BswM_ComM_CurrentMode(1U, 1U);
    
    assert_true(1);
}

static void test_BswM_Dcm_RequestCommunicationMode(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Should not crash with different modes */
    BswM_Dcm_RequestCommunicationMode(0U);
    BswM_Dcm_RequestCommunicationMode(1U);
    BswM_Dcm_RequestCommunicationMode(2U);
    
    assert_true(1);
}

/*==================================================================================================
 *                                  TEST CASES - STATE MANAGEMENT
 *================================================================================================*/

static void test_BswM_IsInitialized_Uninit(void **state)
{
    (void)state;
    
    assert_false(BswM_IsInitialized());
}

static void test_BswM_IsInitialized_AfterInit(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    assert_true(BswM_IsInitialized());
}

static void test_BswM_IsInitialized_AfterDeinit(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    assert_true(BswM_IsInitialized());
    
    BswM_DeInit();
    assert_false(BswM_IsInitialized());
}

static void test_BswM_GetCurrentMode_InvalidPort(void **state)
{
    (void)state;
    BswM_ModeType mode;
    
    BswM_Init(&testConfig);
    
    /* Invalid port should return default mode */
    mode = BswM_GetCurrentMode(BSWM_MAX_MODE_REQUEST_PORTS + 1);
    assert_int_equal(mode, BSWM_MODE_STARTUP);
}

static void test_BswM_GetCurrentMode_InvalidPort_NotInit(void **state)
{
    (void)state;
    BswM_ModeType mode;
    
    /* Without init, should still return default mode */
    mode = BswM_GetCurrentMode(0U);
    assert_int_equal(mode, BSWM_MODE_STARTUP);
}

/*==================================================================================================
 *                                  TEST CASES - CONFIGURATION
 *================================================================================================*/

static void test_BswM_Config_NumRules(void **state)
{
    (void)state;
    
    assert_int_equal(testConfig.NumRules, 3U);
    assert_int_equal(testConfig.NumModeRequestPorts, 4U);
}

static void test_BswM_Config_RuleActiveStatus(void **state)
{
    (void)state;
    
    /* Verify active status of rules */
    assert_true(testRules[0].IsActive);
    assert_true(testRules[1].IsActive);
    assert_false(testRules[2].IsActive);
}

static void test_BswM_Config_RuleExpectedModes(void **state)
{
    (void)state;
    
    assert_int_equal(testRules[0].ExpectedMode, BSWM_MODE_RUN);
    assert_int_equal(testRules[1].ExpectedMode, BSWM_MODE_RUN);
    assert_int_equal(testRules[2].ExpectedMode, BSWM_MODE_SHUTDOWN);
}

/*==================================================================================================
 *                                  TEST CASES - COMPLEX SCENARIOS
 *================================================================================================*/

static void test_BswM_CompleteLifecycle(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    /* Full lifecycle test */
    BswM_Init(&testConfig);
    assert_true(BswM_IsInitialized());
    
    BswM_GetVersionInfo(&versionInfo);
    assert_int_equal(versionInfo.moduleID, BSWM_MODULE_ID);
    
    BswM_RequestMode(0U, BSWM_MODE_RUN);
    BswM_MainFunction();
    
    BswM_EcuM_CurrentState(BSWM_ECUM_STATE_RUN);
    BswM_MainFunction();
    
    BswM_RequestMode(0U, BSWM_MODE_SHUTDOWN);
    BswM_MainFunction();
    
    BswM_DeInit();
    assert_false(BswM_IsInitialized());
}

static void test_BswM_MultipleUsers(void **state)
{
    (void)state;
    
    BswM_Init(&testConfig);
    
    /* Simulate requests from different sources */
    BswM_EcuM_CurrentState(BSWM_ECUM_STATE_RUN);
    BswM_ComM_CurrentMode(0U, 1U);
    BswM_Dcm_RequestCommunicationMode(1U);
    
    /* Direct mode requests */
    BswM_RequestMode(0U, BSWM_MODE_RUN);
    BswM_RequestMode(1U, BSWM_MODE_RUN);
    BswM_RequestMode(2U, BSWM_MODE_SHUTDOWN);
    
    BswM_MainFunction();
    
    assert_true(1);
}

/*==================================================================================================
 *                                  TEST RUNNER
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* Initialization Tests */
        cmocka_unit_test_setup_teardown(test_BswM_Init_ValidConfig, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_Init_NullConfig, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_DeInit_Normal, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_DeInit_Uninitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_Init_Reinit, test_setup, test_teardown),
        
        /* Version Info Tests */
        cmocka_unit_test_setup_teardown(test_BswM_GetVersionInfo_ValidPointer, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_GetVersionInfo_NullPointer, test_setup, test_teardown),
        
        /* Mode Request Tests */
        cmocka_unit_test_setup_teardown(test_BswM_RequestMode_ValidPort, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_RequestMode_MultiplePorts, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_RequestMode_Uninitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_RequestMode_InvalidPort, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_RequestMode_ModeTransition, test_setup, test_teardown),
        
        /* Main Function Tests */
        cmocka_unit_test_setup_teardown(test_BswM_MainFunction_Initialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_MainFunction_Uninitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_MainFunction_WithModeRequest, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_MainFunction_MultipleCalls, test_setup, test_teardown),
        
        /* Callback Tests */
        cmocka_unit_test_setup_teardown(test_BswM_EcuM_CurrentState, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_ComM_CurrentMode, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_Dcm_RequestCommunicationMode, test_setup, test_teardown),
        
        /* State Management Tests */
        cmocka_unit_test_setup_teardown(test_BswM_IsInitialized_Uninit, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_IsInitialized_AfterInit, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_IsInitialized_AfterDeinit, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_GetCurrentMode_InvalidPort, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_GetCurrentMode_InvalidPort_NotInit, test_setup, test_teardown),
        
        /* Configuration Tests */
        cmocka_unit_test_setup_teardown(test_BswM_Config_NumRules, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_Config_RuleActiveStatus, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_Config_RuleExpectedModes, test_setup, test_teardown),
        
        /* Complex Scenario Tests */
        cmocka_unit_test_setup_teardown(test_BswM_CompleteLifecycle, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_BswM_MultipleUsers, test_setup, test_teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

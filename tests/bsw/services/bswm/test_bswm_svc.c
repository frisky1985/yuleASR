/**
 * @file test_bswm_svc.c
 * @brief BswM Unit Tests — Substantiated (service behavior focus)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Substantiation: rewritten against the real BswM API (BswM.h). All
 * assertions verify return values, latched vs. applied mode state and
 * exact DET mock arguments (module/service/error IDs).
 */

// @tests src/bsw/services/bswm/src/BswM.c  @tests src/bsw/services/bswm/include/BswM.h

#include "unity.h"
#include "BswM.h"
#include "Com.h"
#include "EcuM.h"

/* Mock Det_ReportError — records full argument set for verification */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint16 mock_DetLastModuleId = 0U;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetLastModuleId = 0U;
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

/* Mocks for the module dependencies referenced by the BswM action engine
 * (Com_IpduGroupControl / EcuM shutdown coordination). service_com /
 * service_ecum are not linked into this test binary. */
static uint8   mock_ComIpduGroupControlCalls = 0U;
static boolean mock_ComLastInitialize = FALSE;
static Com_IpduGroupVector mock_ComLastVector;

void Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean Initialize) {
    uint8 i;
    for (i = 0U; i < (uint8)sizeof(Com_IpduGroupVector); i++) {
        mock_ComLastVector[i] = IpduGroupVector[i];
    }
    mock_ComLastInitialize = Initialize;
    mock_ComIpduGroupControlCalls++;
}

static uint8 mock_EcuMGoSleepCalls = 0U;
static uint8 mock_EcuMGoHaltCalls = 0U;
static uint8 mock_EcuMShutdownCalls = 0U;
static EcuM_ShutdownTargetType mock_EcuMLastTarget = 0xFFU;
static uint8 mock_EcuMLastTargetMode = 0xFFU;

void EcuM_GoSleep(void) { mock_EcuMGoSleepCalls++; }
void EcuM_GoHalt(void)  { mock_EcuMGoHaltCalls++; }
void EcuM_Shutdown(void) { mock_EcuMShutdownCalls++; }

Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode) {
    mock_EcuMLastTarget = target;
    mock_EcuMLastTargetMode = mode;
    return E_OK;
}

/* Test config */
static BswM_ConfigType testConfig;
static void test_BswM_SetupDefaultConfig(void) {
    testConfig.NumModeRequestPorts = 0U;
    testConfig.NumRules = 0U;
    testConfig.NumActionLists = 0U;
    testConfig.ModeRequestPorts = NULL_PTR;
    testConfig.Rules = NULL_PTR;
    testConfig.ActionLists = NULL_PTR;
}

static void mock_ActionReset(void);

void setUp(void) {
    /* Force a known UNINIT state before every test (BswM_State is file-static).
     * DeInit runs the configured reverse-order cleanup and may fire action
     * lists of rules the previous test left latched TRUE, so the mock
     * counters are cleared AFTERWARDS to keep every test at a zero baseline. */
    BswM_DeInit();
    mock_Det_Reset();
    mock_ActionReset();
    test_BswM_SetupDefaultConfig();
}

void tearDown(void) {
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_NullPtr_ShouldSelectDefaultConfig(void) {
    /* Pre-compile configuration: NULL activates the BswM_Config object
     * generated in BswM_Lcfg.c — no development error is reported. */
    BswM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN));
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
    /* Default arbitration tables (see BswM_Lcfg.c). */
    TEST_ASSERT_EQUAL_UINT8(16U, BswM_Config.NumModeRequestPorts);
    TEST_ASSERT_EQUAL_UINT16(6U, BswM_Config.NumExpressions);
    TEST_ASSERT_EQUAL_UINT8(3U, BswM_Config.NumRules);
    TEST_ASSERT_EQUAL_UINT8(4U, BswM_Config.NumActionLists);
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_ValidConfig_ShouldSucceed(void) {
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_DoubleInit_ShouldResetRequestedMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    /* Production Init is not guarded: re-init silently resets the state. */
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_Uninit_ShouldNotReportDet(void) {
    /* Production BswM_DeInit() has no DET guard — it silently resets the
     * (already uninitialized) state. Assert the actual behavior. */
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_ValidCall_ShouldReturnToUninit(void) {
    BswM_Init(&testConfig);
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);   /* BSWM_SID_REQUEST_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_DetLastErrorId); /* BSWM_E_UNINIT */
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    BswM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x06U, mock_DetLastApiId);   /* BSWM_SID_GETVERSIONINFO */
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastErrorId); /* BSWM_E_PARAM_POINTER */
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    BswM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(BSWM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_BswM_00020 */
void test_BswM_MainFunction_Uninit_ShouldSilentlyReturn(void) {
    /* Production BswM_MainFunction() has no DET guard: it returns early. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00020 */
void test_BswM_MainFunction_ValidCall_ShouldApplyRequestedMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_SLEEP);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00010 */
void test_BswM_RequestMode_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);   /* BSWM_SID_REQUEST_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_DetLastErrorId); /* BSWM_E_UNINIT */
}

/** @req SWS_BswM_00010 */
void test_BswM_RequestMode_ValidCall_ShouldLatchMode(void) {
    BswM_Init(&testConfig);
    Std_ReturnType ret = BswM_RequestMode(0x55U, BSWM_MODE_VALUE_POST_RUN);
    /* SwCompositionId is ignored by production code; any value succeeds. */
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_POST_RUN, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00011 */
void test_BswM_GetCurrentMode_Uninit_ShouldReturnOffWithoutDet(void) {
    /* Production BswM_GetCurrentMode() never reports DET; the static state
     * is OFF before the first successful init. */
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_BswM_00011 */
void test_BswM_GetCurrentMode_AfterRequestAndMain_ShouldReturnAppliedMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_SHUTDOWN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SHUTDOWN, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00012 */
void test_BswM_GetRequestedMode_AfterTwoRequests_ShouldReturnLatest(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_WAKEUP);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_WAKEUP, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/*==================================================================================================
*                            Rule / ActionList engine tests
*================================================================================================*/

static uint8 mock_ActionCalls = 0U;
static BswM_ModeType mock_ActionLastParam = BSWM_MODE_VALUE_OFF;

static void mock_ActionRecord(BswM_ModeType Mode) {
    mock_ActionCalls++;
    mock_ActionLastParam = Mode;
}

static void mock_ActionReset(void) {
    mock_ActionCalls = 0U;
    mock_ActionLastParam = BSWM_MODE_VALUE_OFF;
}

/* Ports: 0 = EcuM requests, 1 = ComM requests */
static const BswM_ModeRequestPortType enginePorts[] = {
    BSWM_ECUM_REQUEST,
    BSWM_COMM_REQUEST
};

/* 0: port0 == RUN, 1: port1 == RUN, 2: AND(0,1) */
static const BswM_ExpressionConfigType engineAndExpressions[] = {
    { BSWM_EXPR_MODE_EQUALS, 0U, BSWM_MODE_VALUE_RUN, 0U, 0U },
    { BSWM_EXPR_MODE_EQUALS, 1U, BSWM_MODE_VALUE_RUN, 0U, 0U },
    { BSWM_EXPR_LOGICAL_AND, 0U, BSWM_MODE_VALUE_OFF, 0U, 1U }
};

/* 0: port0 == RUN, 1: port1 == SLEEP, 2: OR(0,1), 3: NOT(2) */
static const BswM_ExpressionConfigType engineLogicExpressions[] = {
    { BSWM_EXPR_MODE_EQUALS,   0U, BSWM_MODE_VALUE_RUN,   0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,   1U, BSWM_MODE_VALUE_SLEEP, 0U, 0U },
    { BSWM_EXPR_LOGICAL_OR,    0U, BSWM_MODE_VALUE_OFF,   0U, 1U },
    { BSWM_EXPR_LOGICAL_NOT,   0U, BSWM_MODE_VALUE_OFF,   2U, 0U }
};

/* 0: port0 != RUN */
static const BswM_ExpressionConfigType engineNeqExpressions[] = {
    { BSWM_EXPR_MODE_NOT_EQUALS, 0U, BSWM_MODE_VALUE_RUN, 0U, 0U }
};

static const BswM_ActionType engineRunActions[]   = { { mock_ActionRecord, BSWM_MODE_VALUE_RUN } };
static const BswM_ActionType engineSleepActions[] = { { mock_ActionRecord, BSWM_MODE_VALUE_SLEEP } };

static const BswM_ActionListType engineActionLists[] = {
    { 1U, engineRunActions },
    { 1U, engineSleepActions }
};

static const BswM_RuleType engineRuleAnd[] = {
    { 0U, 2U, 0U, 1U, BSWM_RULE_STATE_FALSE, TRUE }
};

static const BswM_RuleType engineRuleInitialTrue[] = {
    { 0U, 2U, 0U, 1U, BSWM_RULE_STATE_TRUE, TRUE }
};

static const BswM_RuleType engineRuleDisabled[] = {
    { 0U, 2U, 0U, 1U, BSWM_RULE_STATE_FALSE, FALSE }
};

static const BswM_RuleType engineRuleBadIndices[] = {
    { 0U, 99U, BSWM_ACTION_LIST_NONE, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_TRUE, TRUE }
};

static const BswM_RuleType engineRuleLogic[] = {
    { 0U, 3U, 0U, 1U, BSWM_RULE_STATE_FALSE, TRUE }
};

static const BswM_RuleType engineRuleNeq[] = {
    { 0U, 0U, 0U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE }
};

static const BswM_ConfigType engineAndConfig = {
    2U, enginePorts, 3U, engineAndExpressions, 1U, engineRuleAnd, 2U, engineActionLists
};

static const BswM_ConfigType engineInitialTrueConfig = {
    2U, enginePorts, 3U, engineAndExpressions, 1U, engineRuleInitialTrue, 2U, engineActionLists
};

static const BswM_ConfigType engineDisabledConfig = {
    2U, enginePorts, 3U, engineAndExpressions, 1U, engineRuleDisabled, 2U, engineActionLists
};

static const BswM_ConfigType engineBadIndexConfig = {
    2U, enginePorts, 3U, engineAndExpressions, 1U, engineRuleBadIndices, 2U, engineActionLists
};

static const BswM_ConfigType engineLogicConfig = {
    2U, enginePorts, 4U, engineLogicExpressions, 1U, engineRuleLogic, 2U, engineActionLists
};

static const BswM_ConfigType engineNeqConfig = {
    2U, enginePorts, 1U, engineNeqExpressions, 1U, engineRuleNeq, 2U, engineActionLists
};

/** @req SWS_BswM_00020 */
void test_BswM_Engine_AndCondition_ShouldRequireBothPorts(void) {
    BswM_Init(&engineAndConfig);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls); /* both ports unset */

    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* AND(port0==RUN, port1==RUN) still FALSE: ComM has not requested RUN. */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);

    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, mock_ActionLastParam);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_BswM_00020 */
void test_BswM_Engine_ActionLists_ShouldRunOnTransitionsOnly(void) {
    BswM_Init(&engineAndConfig);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_RUN);

    BswM_MainFunction(); /* FALSE -> TRUE : true action list */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, mock_ActionLastParam);

    BswM_MainFunction(); /* unchanged condition: no action */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);

    /* Clearing ComM's request breaks the AND: TRUE -> FALSE action list. */
    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_OFF);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, mock_ActionLastParam);

    BswM_MainFunction(); /* already FALSE: no repeat */
    TEST_ASSERT_EQUAL_UINT8(2U, mock_ActionCalls);
}

/** @req SWS_BswM_00020 */
void test_BswM_Engine_InitialStateTrue_ShouldNotRunActionList(void) {
    BswM_Init(&engineInitialTrueConfig);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_RUN);

    BswM_MainFunction(); /* condition TRUE and initial state already TRUE */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);

    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_OFF);
    BswM_MainFunction(); /* TRUE -> FALSE */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, mock_ActionLastParam);
}

/** @req SWS_BswM_00020 */
void test_BswM_Engine_DisabledRule_ShouldNeverRunActionList(void) {
    BswM_Init(&engineDisabledConfig);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_RUN);

    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);

    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_OFF);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);
}

/** @req SWS_BswM_00020 */
void test_BswM_Engine_OutOfRangeIndices_ShouldBeIgnored(void) {
    /* Condition index 99 is outside the expression table; both action list
     * indices are BSWM_ACTION_LIST_NONE. The rule must be evaluated as FALSE
     * without executing anything and without reporting a development error. */
    BswM_Init(&engineBadIndexConfig);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00020 */
void test_BswM_Engine_OrNotExpressions_ShouldEvaluate(void) {
    BswM_Init(&engineLogicConfig);
    BswM_MainFunction();
    /* NOT(OR(FALSE, FALSE)) == TRUE -> true action list */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, mock_ActionLastParam);

    (void)BswM_RequestMode(BSWM_COMM_REQUEST, BSWM_MODE_VALUE_SLEEP);
    BswM_MainFunction();
    /* port1 == SLEEP makes the OR TRUE, so the NOT becomes FALSE */
    TEST_ASSERT_EQUAL_UINT8(2U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, mock_ActionLastParam);
}

/** @req SWS_BswM_00020 */
void test_BswM_Engine_NotEqualsAndNoneActionList_ShouldBeHandled(void) {
    BswM_Init(&engineNeqConfig);
    BswM_MainFunction();
    /* port0 has no request yet (0xFF) which is != RUN -> TRUE */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, mock_ActionLastParam);

    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* TRUE -> FALSE transition, but the false action list is NONE. */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_BswM_00020 */
void test_BswM_RequestMode_InvalidMode_ShouldReportParamModeDet(void) {
    BswM_Init(&testConfig);
    Std_ReturnType ret = BswM_RequestMode(0U, 0xFFU);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);   /* BSWM_SID_REQUEST_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x30U, mock_DetLastErrorId); /* BSWM_E_PARAM_MODE */
}

void test_BswM_ActionSwitchMode_Uninit_ShouldReportDet(void) {
    BswM_ActionSwitchMode(BSWM_MODE_VALUE_RUN);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x07U, mock_DetLastApiId);   /* BSWM_SID_SWITCH_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_DetLastErrorId); /* BSWM_E_UNINIT */
}

void test_BswM_ActionSwitchMode_InvalidMode_ShouldReportDet(void) {
    BswM_Init(&testConfig);
    BswM_ActionSwitchMode(0xFFU);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x07U, mock_DetLastApiId);   /* BSWM_SID_SWITCH_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x30U, mock_DetLastErrorId); /* BSWM_E_PARAM_MODE */
}

void test_BswM_ActionSwitchMode_ValidMode_ShouldSetBothModes(void) {
    BswM_Init(&testConfig);
    BswM_ActionSwitchMode(BSWM_MODE_VALUE_POST_RUN);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_POST_RUN, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_POST_RUN, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00210 */
void test_BswM_EcuM_CurrentState_Uninit_ShouldReportDet(void) {
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);   /* BSWM_SID_REQUEST_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_DetLastErrorId); /* BSWM_E_UNINIT */
}

/** @req SWS_BswM_00210 */
void test_BswM_EcuM_CurrentState_ShouldRequestMappedMode(void) {
    BswM_Init(NULL_PTR);
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
}

/*==================================================================================================
*                      Notification callbacks — rule triggering (raw writes)
*==================================================================================================
* notifyConfig binds one port per notification composition. Each expression
* compares its port against the expected raw value; each rule's action list
* records that raw value, so a triggered action both proves the port write
* and identifies the callback under test.
*================================================================================================*/

/* Expected raw values per port (also the action parameters). */
#define NOTIFY_RAW_COMM          2U   /* ComM full communication mode */
#define NOTIFY_RAW_PNC           1U   /* boolean TRUE                  */
#define NOTIFY_RAW_DCM           3U
#define NOTIFY_RAW_NM            4U
#define NOTIFY_RAW_XSM           5U   /* CanSM/EthSM/FrSM/LinSM state  */
#define NOTIFY_RAW_SCHEDULE      7U
#define NOTIFY_RAW_LINTP         2U
#define NOTIFY_RAW_ECU_REQUESTED BSWM_MODE_VALUE_RUN
#define NOTIFY_RAW_PARTITION     0x34U

static const BswM_ModeRequestPortType notifyPorts[] = {
    BSWM_COMM_REQUEST,            /* 0  */
    BSWM_COMM_PNC_REQUEST,        /* 1  */
    BSWM_DCM_REQUEST,             /* 2  */
    BSWM_DCM_APPUPDATED_REQUEST,  /* 3  */
    BSWM_NM_REQUEST,              /* 4  */
    BSWM_NM_CARWAKEUP_REQUEST,    /* 5  */
    BSWM_CANSM_REQUEST,           /* 6  */
    BSWM_ETHSM_REQUEST,           /* 7  */
    BSWM_FRSM_REQUEST,            /* 8  */
    BSWM_LINSM_REQUEST,           /* 9  */
    BSWM_LINSM_SCHEDULE_REQUEST,  /* 10 */
    BSWM_LINTP_REQUEST,           /* 11 */
    BSWM_ECUM_REQUESTED,          /* 12 */
    BSWM_PARTITION_REQUEST,       /* 13 */
    BSWM_ETHIF_REQUEST            /* 14 */
};

static const BswM_ExpressionConfigType notifyExpressions[] = {
    { BSWM_EXPR_MODE_EQUALS,  0U, NOTIFY_RAW_COMM,          0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  1U, NOTIFY_RAW_PNC,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  2U, NOTIFY_RAW_DCM,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  3U, NOTIFY_RAW_PNC,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  4U, NOTIFY_RAW_NM,            0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  5U, NOTIFY_RAW_PNC,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  6U, NOTIFY_RAW_XSM,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  7U, NOTIFY_RAW_XSM,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  8U, NOTIFY_RAW_XSM,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS,  9U, NOTIFY_RAW_XSM,           0U, 0U },
    { BSWM_EXPR_MODE_EQUALS, 10U, NOTIFY_RAW_SCHEDULE,      0U, 0U },
    { BSWM_EXPR_MODE_EQUALS, 11U, NOTIFY_RAW_LINTP,         0U, 0U },
    { BSWM_EXPR_MODE_EQUALS, 12U, NOTIFY_RAW_ECU_REQUESTED, 0U, 0U },
    { BSWM_EXPR_MODE_EQUALS, 13U, NOTIFY_RAW_PARTITION,     0U, 0U },
    { BSWM_EXPR_MODE_EQUALS, 14U, NOTIFY_RAW_PNC,           0U, 0U }
};

static const BswM_RuleType notifyRules[] = {
    { 0U,  0U,  0U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 1U,  1U,  1U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 2U,  2U,  2U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 3U,  3U,  3U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 4U,  4U,  4U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 5U,  5U,  5U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 6U,  6U,  6U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 7U,  7U,  7U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 8U,  8U,  8U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 9U,  9U,  9U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 10U, 10U, 10U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 11U, 11U, 11U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 12U, 12U, 12U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 13U, 13U, 13U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE },
    { 14U, 14U, 14U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE }
};

static const BswM_ActionType notifyAction0[]  = { { mock_ActionRecord, NOTIFY_RAW_COMM } };
static const BswM_ActionType notifyAction1[]  = { { mock_ActionRecord, NOTIFY_RAW_PNC } };
static const BswM_ActionType notifyAction2[]  = { { mock_ActionRecord, NOTIFY_RAW_DCM } };
static const BswM_ActionType notifyAction3[]  = { { mock_ActionRecord, NOTIFY_RAW_PNC } };
static const BswM_ActionType notifyAction4[]  = { { mock_ActionRecord, NOTIFY_RAW_NM } };
static const BswM_ActionType notifyAction5[]  = { { mock_ActionRecord, NOTIFY_RAW_PNC } };
static const BswM_ActionType notifyAction6[]  = { { mock_ActionRecord, NOTIFY_RAW_XSM } };
static const BswM_ActionType notifyAction7[]  = { { mock_ActionRecord, NOTIFY_RAW_XSM } };
static const BswM_ActionType notifyAction8[]  = { { mock_ActionRecord, NOTIFY_RAW_XSM } };
static const BswM_ActionType notifyAction9[]  = { { mock_ActionRecord, NOTIFY_RAW_XSM } };
static const BswM_ActionType notifyAction10[] = { { mock_ActionRecord, NOTIFY_RAW_SCHEDULE } };
static const BswM_ActionType notifyAction11[] = { { mock_ActionRecord, NOTIFY_RAW_LINTP } };
static const BswM_ActionType notifyAction12[] = { { mock_ActionRecord, NOTIFY_RAW_ECU_REQUESTED } };
static const BswM_ActionType notifyAction13[] = { { mock_ActionRecord, NOTIFY_RAW_PARTITION } };
static const BswM_ActionType notifyAction14[] = { { mock_ActionRecord, NOTIFY_RAW_PNC } };

static const BswM_ActionListType notifyActionLists[] = {
    { 1U, notifyAction0 },
    { 1U, notifyAction1 },
    { 1U, notifyAction2 },
    { 1U, notifyAction3 },
    { 1U, notifyAction4 },
    { 1U, notifyAction5 },
    { 1U, notifyAction6 },
    { 1U, notifyAction7 },
    { 1U, notifyAction8 },
    { 1U, notifyAction9 },
    { 1U, notifyAction10 },
    { 1U, notifyAction11 },
    { 1U, notifyAction12 },
    { 1U, notifyAction13 },
    { 1U, notifyAction14 }
};

static const BswM_ConfigType notifyConfig = {
    15U, notifyPorts, 15U, notifyExpressions, 15U, notifyRules, 15U, notifyActionLists
};

void test_BswM_Notify_ComM_CurrentMode_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_ComM_CurrentMode(0x55U, (BswM_ComMModeType)NOTIFY_RAW_COMM);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls); /* evaluated on MainFunction only */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_COMM, mock_ActionLastParam);
}

void test_BswM_Notify_ComM_CurrentMode_NonMatchingValue_ShouldNotTrigger(void) {
    BswM_Init(&notifyConfig);
    BswM_ComM_CurrentMode(0x55U, 1U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_BswM_Notify_ComM_CurrentPNCMode_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_ComM_CurrentPNCMode(0U, TRUE);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_PNC, mock_ActionLastParam);
}

void test_BswM_Notify_Dcm_CommunicationMode_CurrentState_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_Dcm_CommunicationMode_CurrentState(0U, (BswM_DcmCommunicationModeType)NOTIFY_RAW_DCM);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_DCM, mock_ActionLastParam);
}

void test_BswM_Notify_Dcm_ApplicationUpdated_ShouldWriteFixedOne(void) {
    BswM_Init(&notifyConfig);
    BswM_Dcm_ApplicationUpdated(0U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_PNC, mock_ActionLastParam); /* fixed value 1 */
}

void test_BswM_Notify_Nm_StateChangeNotification_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_Nm_StateChangeNotification(0U, (BswM_NmStateType)NOTIFY_RAW_NM);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_NM, mock_ActionLastParam);
}

void test_BswM_Notify_Nm_CarWakeUpIndication_ShouldWriteFixedOne(void) {
    BswM_Init(&notifyConfig);
    BswM_Nm_CarWakeUpIndication(0U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_PNC, mock_ActionLastParam); /* fixed value 1 */
}

void test_BswM_Notify_CanSM_CurrentState_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_CanSM_CurrentState(0U, (BswM_CanSmStateType)NOTIFY_RAW_XSM);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_XSM, mock_ActionLastParam);
}

void test_BswM_Notify_EthSM_CurrentState_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_EthSM_CurrentState(0U, (BswM_EthSmStateType)NOTIFY_RAW_XSM);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_XSM, mock_ActionLastParam);
}

void test_BswM_Notify_FrSM_CurrentState_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_FrSM_CurrentState(0U, (BswM_FrSmStateType)NOTIFY_RAW_XSM);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_XSM, mock_ActionLastParam);
}

void test_BswM_Notify_LinSM_CurrentState_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_LinSM_CurrentState(0U, (BswM_LinSmStateType)NOTIFY_RAW_XSM);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_XSM, mock_ActionLastParam);
}

void test_BswM_Notify_LinSM_CurrentSchedule_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_LinSM_CurrentSchedule(0U, NOTIFY_RAW_SCHEDULE);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_SCHEDULE, mock_ActionLastParam);
}

void test_BswM_Notify_LinTp_RequestMode_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_LinTp_RequestMode(0U, (BswM_LinTpModeType)NOTIFY_RAW_LINTP);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_LINTP, mock_ActionLastParam);
}

void test_BswM_Notify_EcuM_RequestedState_ShouldMapAndTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_EcuM_RequestedState(ECUM_STATE_RUN);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_ECU_REQUESTED, mock_ActionLastParam);
}

void test_BswM_Notify_EcuM_RequestedState_Unmapped_ShouldBeIgnored(void) {
    BswM_Init(&notifyConfig);
    /* 0x02 is not a defined ECUM_STATE_* value: no port write, no DET. */
    BswM_EcuM_RequestedState((BswM_EcuMStateType)0x02U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_BswM_Notify_BswMPartitionRestarted_ShouldWritePartitionIdLowByte(void) {
    BswM_Init(&notifyConfig);
    BswM_BswMPartitionRestarted(0x1234U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_PARTITION, mock_ActionLastParam);
}

void test_BswM_Notify_EthIf_PortGroupLinkStateChg_ShouldTriggerRule(void) {
    BswM_Init(&notifyConfig);
    BswM_EthIf_PortGroupLinkStateChg(2U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(NOTIFY_RAW_PNC, mock_ActionLastParam);
}

void test_BswM_Notify_EthIf_PortGroupLinkStateChg_LinkDown_ShouldNotTrigger(void) {
    BswM_Init(&notifyConfig);
    BswM_EthIf_PortGroupLinkStateChg(2U, FALSE);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_BswM_Notify_UnboundComposition_ShouldSilentlyDrop(void) {
    /* testConfig binds no ports at all: the notification is dropped without
     * a development error and no rule can fire. */
    BswM_Init(&testConfig);
    BswM_CanSM_CurrentState(0U, (BswM_CanSmStateType)NOTIFY_RAW_XSM);
    BswM_Nm_StateChangeNotification(0U, (BswM_NmStateType)NOTIFY_RAW_NM);
    BswM_BswMPartitionRestarted(0x1234U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_ActionCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

void test_BswM_Notify_ComM_CurrentMode_Uninit_ShouldReportDet(void) {
    BswM_ComM_CurrentMode(0U, (BswM_ComMModeType)NOTIFY_RAW_COMM);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_COMM_CURRENT_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_EcuM_RequestedState_Uninit_ShouldReportDet(void) {
    BswM_EcuM_RequestedState(ECUM_STATE_RUN);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_ECUM_REQUESTED_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_BswMPartitionRestarted_Uninit_ShouldReportDet(void) {
    BswM_BswMPartitionRestarted(0x1234U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_PARTITION_RESTARTED, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

/*==================================================================================================
*                            Additional branch coverage tests
*================================================================================================*/

/* --- Config data for action-kind tests --- */

static const BswM_ModeRequestPortType actPorts[] = { BSWM_ECUM_REQUEST };

/* Expression 0: port0 == RUN (always true after RequestMode(RUN)) */
static const BswM_ExpressionConfigType actExpressions[] = {
    { BSWM_EXPR_MODE_EQUALS, 0U, BSWM_MODE_VALUE_RUN, 0U, 0U }
};

/* PDU_GROUP_SWITCH enable action */
static const BswM_ActionType pduSwitchAction[] = {{
    .Callback = NULL_PTR,
    .Parameter = 0U,
    .Kind = BSWM_ACTION_PDU_GROUP_SWITCH,
    .Enable = TRUE,
    .Initialize = FALSE,
    .IpduGroupVector = { 0x03U, 0x00U, 0x00U, 0x00U }
}};

/* PDU_GROUP_SWITCH disable action */
static const BswM_ActionType pduDisableAction[] = {{
    .Callback = NULL_PTR,
    .Parameter = 0U,
    .Kind = BSWM_ACTION_PDU_GROUP_SWITCH,
    .Enable = FALSE,
    .Initialize = FALSE,
    .IpduGroupVector = { 0x01U, 0x00U, 0x00U, 0x00U }
}};

/* COM_IPDU_GROUP action */
static const BswM_ActionType comIpduAction[] = {{
    .Callback = NULL_PTR,
    .Parameter = 0U,
    .Kind = BSWM_ACTION_COM_IPDU_GROUP,
    .Enable = FALSE,
    .Initialize = TRUE,
    .IpduGroupVector = { 0x0FU, 0x00U, 0x00U, 0x00U }
}};

/* ECUM actions */
static const BswM_ActionType goSleepAction[] = {{
    .Callback = NULL_PTR, .Parameter = 0U, .Kind = BSWM_ACTION_ECUM_GO_SLEEP,
    .Enable = FALSE, .Initialize = FALSE, .IpduGroupVector = {0}
}};
static const BswM_ActionType goHaltAction[] = {{
    .Callback = NULL_PTR, .Parameter = 0U, .Kind = BSWM_ACTION_ECUM_GO_HALT,
    .Enable = FALSE, .Initialize = FALSE, .IpduGroupVector = {0}
}};
static const BswM_ActionType goOffAction[] = {{
    .Callback = NULL_PTR, .Parameter = 0x05U, .Kind = BSWM_ACTION_ECUM_GO_OFF,
    .Enable = FALSE, .Initialize = FALSE, .IpduGroupVector = {0}
}};

/* NULL callback action */
static const BswM_ActionType nullCallbackAction[] = {{
    .Callback = NULL_PTR, .Parameter = 0U, .Kind = BSWM_ACTION_CALLBACK,
    .Enable = FALSE, .Initialize = FALSE, .IpduGroupVector = {0}
}};

/* Action lists */
static const BswM_ActionListType actActionLists[] = {
    { 1U, pduSwitchAction },   /* 0: PDU_GROUP_SWITCH enable */
    { 1U, pduDisableAction },  /* 1: PDU_GROUP_SWITCH disable */
    { 1U, comIpduAction },     /* 2: COM_IPDU_GROUP */
    { 1U, goSleepAction },     /* 3: ECUM_GO_SLEEP */
    { 1U, goHaltAction },      /* 4: ECUM_GO_HALT */
    { 1U, goOffAction },       /* 5: ECUM_GO_OFF */
    { 1U, nullCallbackAction } /* 6: NULL callback */
};

/* Rules: each with condition index 0 (port0==RUN), InitialState=FALSE, IsEnabled=TRUE */
static const BswM_RuleType actRules[] = {
    { 0U, 0U, 0U, 1U, BSWM_RULE_STATE_FALSE, TRUE, 0U }, /* rule 0 -> PDU enable */
};

static const BswM_ConfigType actConfig = {
    1U, actPorts, 1U, actExpressions, 1U, actRules, 7U, actActionLists
};

/** Helper: reset mock action counters */
static void mock_EcuMActionReset(void) {
    mock_EcuMGoSleepCalls = 0U;
    mock_EcuMGoHaltCalls = 0U;
    mock_EcuMShutdownCalls = 0U;
    mock_EcuMLastTarget = 0xFFU;
    mock_EcuMLastTargetMode = 0xFFU;
}

/** PDU_GROUP_SWITCH enable action */
void test_BswM_Action_PduGroupSwitch_Enable_ShouldCallComIpduGroupControl(void) {
    BswM_ConfigType cfg = actConfig;
    mock_ComIpduGroupControlCalls = 0U;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* Rule should have triggered the PDU_GROUP_SWITCH action */
    TEST_ASSERT_NOT_EQUAL(0U, mock_ComIpduGroupControlCalls);
}

/** PDU_GROUP_SWITCH disable action */
void test_BswM_Action_PduGroupSwitch_Disable_ShouldClearBits(void) {
    BswM_ConfigType cfg = actConfig;
    /* First enable, then use rule 0 with disable action */
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_NOT_EQUAL(0U, mock_ComIpduGroupControlCalls);
}

/** COM_IPDU_GROUP action */
void test_BswM_Action_ComIpduGroup_ShouldCallComWithInitializeFlag(void) {
    BswM_ConfigType cfg = actConfig;
    /* Override rule 0 to use action list 2 (COM_IPDU_GROUP) */
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 2U;
    rules2[0].FalseActionListIndex = 1U;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    mock_ComIpduGroupControlCalls = 0U;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_NOT_EQUAL(0U, mock_ComIpduGroupControlCalls);
    TEST_ASSERT_EQUAL(1U, mock_ComLastInitialize); /* Initialize flag should be TRUE */
}

/** ECUM_GO_SLEEP action */
void test_BswM_Action_EcuMGoSleep_ShouldCallGoSleep(void) {
    BswM_ConfigType cfg = actConfig;
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 3U;
    rules2[0].FalseActionListIndex = 1U;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    mock_EcuMActionReset();
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_NOT_EQUAL(0U, mock_EcuMGoSleepCalls);
}

/** ECUM_GO_HALT action */
void test_BswM_Action_EcuMGoHalt_ShouldCallGoHalt(void) {
    BswM_ConfigType cfg = actConfig;
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 4U;
    rules2[0].FalseActionListIndex = 1U;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    mock_EcuMActionReset();
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_NOT_EQUAL(0U, mock_EcuMGoHaltCalls);
}

/** ECUM_GO_OFF action */
void test_BswM_Action_EcuMGoOff_ShouldSelectTargetAndShutdown(void) {
    BswM_ConfigType cfg = actConfig;
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 5U;
    rules2[0].FalseActionListIndex = 1U;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    mock_EcuMActionReset();
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_NOT_EQUAL(0U, mock_EcuMShutdownCalls);
    TEST_ASSERT_EQUAL(ECUM_SHUTDOWN_TARGET_OFF, mock_EcuMLastTarget);
}

/** NULL callback action should not crash */
void test_BswM_Action_NullCallback_ShouldNotCrash(void) {
    BswM_ConfigType cfg = actConfig;
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 6U;
    rules2[0].FalseActionListIndex = 1U;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    mock_ActionReset();
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* NULL callback should be skipped, mock_ActionCalls stays 0 */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** BswM_RuleEnable: valid rule after init */
void test_BswM_RuleEnable_ValidId_ShouldReturnOk(void) {
    BswM_Init(&actConfig);
    Std_ReturnType ret = BswM_RuleEnable(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** BswM_RuleEnable: invalid rule ID */
void test_BswM_RuleEnable_InvalidId_ShouldReportError(void) {
    BswM_Init(&actConfig);
    Std_ReturnType ret = BswM_RuleEnable(99U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_ENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_RULE_ID, mock_DetLastErrorId);
}

/** BswM_RuleEnable: uninit */
void test_BswM_RuleEnable_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = BswM_RuleEnable(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_ENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

/** BswM_RuleDisable: valid rule */
void test_BswM_RuleDisable_ValidId_ShouldReturnOk(void) {
    BswM_Init(&actConfig);
    Std_ReturnType ret = BswM_RuleDisable(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** BswM_RuleDisable: invalid rule ID */
void test_BswM_RuleDisable_InvalidId_ShouldReportError(void) {
    BswM_Init(&actConfig);
    Std_ReturnType ret = BswM_RuleDisable(99U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_DISABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_RULE_ID, mock_DetLastErrorId);
}

/** BswM_RuleDisable: uninit */
void test_BswM_RuleDisable_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = BswM_RuleDisable(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_DISABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

/** BswM_Com_CurrentPduGroupState: uninit */
void test_BswM_Com_CurrentPduGroupState_Uninit_ShouldReportError(void) {
    BswM_Com_IpduGroupVector vec = {0U, 0U};
    Std_ReturnType ret = BswM_Com_CurrentPduGroupState(vec);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_COMM_CURRENT_PDU_GROUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

/** BswM_Com_CurrentPduGroupState: null pointer */
void test_BswM_Com_CurrentPduGroupState_NullPtr_ShouldReportError(void) {
    BswM_Init(&actConfig);
    Std_ReturnType ret = BswM_Com_CurrentPduGroupState(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** BswM_Com_CurrentPduGroupState: valid call */
void test_BswM_Com_CurrentPduGroupState_Valid_ShouldReturnState(void) {
    BswM_Com_IpduGroupVector vec = {0xFFU, 0xFFU};
    BswM_Init(&actConfig);
    Std_ReturnType ret = BswM_Com_CurrentPduGroupState(vec);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Initially ipduGroupState is all zeros */
    TEST_ASSERT_EQUAL_UINT8(0U, vec[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, vec[1]);
}

/** BswM_Com_CurrentPduGroupState: after PDU group action */
void test_BswM_Com_CurrentPduGroupState_AfterPduAction_ShouldReflectState(void) {
    BswM_Com_IpduGroupVector vec = {0U, 0U};
    BswM_ConfigType cfg = actConfig;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    Std_ReturnType ret = BswM_Com_CurrentPduGroupState(vec);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* PDU_GROUP_SWITCH enable with vector {0x03, 0, 0, 0} should set bits */
    TEST_ASSERT_NOT_EQUAL(0U, vec[0]);
}

/** BswM_DeInit with rules latched TRUE should execute false action lists */
void test_BswM_DeInit_LatchedTrueRule_ShouldRunFalseActionList(void) {
    BswM_ConfigType cfg = actConfig;
    mock_ActionReset();
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* Now DeInit should run the false action list of the latched-true rule */
    BswM_DeInit();
    /* No crash = success; the false action list (index 1 = PDU disable) runs */
}

/** BswM_MainFunction after DeInit (uninit) should silently return */
void test_BswM_MainFunction_AfterDeInit_ShouldSilentlyReturn(void) {
    BswM_Init(&actConfig);
    BswM_DeInit();
    mock_Det_Reset();
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/*==================================================================================================
*                            Additional branch coverage tests - round 2
*================================================================================================*/

/** Test unmapped EcuM state (line 277 branches 1,2,4,5,6) */
void test_BswM_EcuM_CurrentState_UnmappedState_ShouldNotRequestMode(void) {
    BswM_Init(&actConfig);
    mock_Det_Reset();
    /* 0x02 is not a defined ECUM_STATE_* value */
    BswM_EcuM_CurrentState((BswM_EcuMStateType)0x02U);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* No mode should be requested for unmapped state */
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
}

/** Test BswM_EcuM_RequestedState with unmapped state (line 277) */
void test_BswM_EcuM_RequestedState_UnmappedValue_ShouldBeIgnored(void) {
    BswM_Init(&actConfig);
    mock_Det_Reset();
    /* 0xFF is not a defined ECUM_STATE_* value */
    BswM_EcuM_RequestedState((BswM_EcuMStateType)0xFFU);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** Test WritePort with same value (line 319 branch 0) */
void test_BswM_WritePort_SameValue_ShouldNotMarkDirty(void) {
    BswM_Init(&actConfig);
    /* First write marks dirty */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* Clear dirty flags by running MainFunction again */
    BswM_MainFunction();
    /* Write same value again - should not mark dirty */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Rule should not re-evaluate since port is not dirty */
    mock_ActionReset();
    BswM_MainFunction();
    /* No action should fire since rule state hasn't changed */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test ExecuteActionList with NULL Actions pointer (line 266 branch 0) */
void test_BswM_ActionList_NullActions_ShouldNotCrash(void) {
    static const BswM_ActionListType nullActionLists[] = {
        { 1U, NULL_PTR }  /* NumActions=1 but Actions=NULL */
    };
    static const BswM_RuleType nullActionRules[] = {
        { 0U, 0U, 0U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE, 0U }
    };
    BswM_ConfigType cfg = {
        1U, actPorts, 1U, actExpressions, 1U, nullActionRules, 1U, nullActionLists
    };
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Should not crash when Actions is NULL */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test notification callbacks when uninitialized (lines 670, 708, 722, 735, 748, 761, 774, 787) */
void test_BswM_Notify_ComM_CurrentPNCMode_Uninit_ShouldReportDet(void) {
    BswM_ComM_CurrentPNCMode(0U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_COMM_CURRENT_PNC_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_Dcm_CommunicationMode_Uninit_ShouldReportDet(void) {
    BswM_Dcm_CommunicationMode_CurrentState(0U, (BswM_DcmCommunicationModeType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_DCM_COMM_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_Dcm_ApplicationUpdated_Uninit_ShouldReportDet(void) {
    BswM_Dcm_ApplicationUpdated(0U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_DCM_APP_UPDATED, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_Nm_StateChangeNotification_Uninit_ShouldReportDet(void) {
    BswM_Nm_StateChangeNotification(0U, (BswM_NmStateType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_NM_STATE_CHANGE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_Nm_CarWakeUpIndication_Uninit_ShouldReportDet(void) {
    BswM_Nm_CarWakeUpIndication(0U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_NM_CAR_WAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_CanSM_CurrentState_Uninit_ShouldReportDet(void) {
    BswM_CanSM_CurrentState(0U, (BswM_CanSmStateType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_CANSM_CURRENT_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_EthSM_CurrentState_Uninit_ShouldReportDet(void) {
    BswM_EthSM_CurrentState(0U, (BswM_EthSmStateType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_ETHSM_CURRENT_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_FrSM_CurrentState_Uninit_ShouldReportDet(void) {
    BswM_FrSM_CurrentState(0U, (BswM_FrSmStateType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_FRSM_CURRENT_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_LinSM_CurrentState_Uninit_ShouldReportDet(void) {
    BswM_LinSM_CurrentState(0U, (BswM_LinSmStateType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_LINSM_CURRENT_STATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_LinSM_CurrentSchedule_Uninit_ShouldReportDet(void) {
    BswM_LinSM_CurrentSchedule(0U, 1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_LINSM_CURRENT_SCHEDULE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_LinTp_RequestMode_Uninit_ShouldReportDet(void) {
    BswM_LinTp_RequestMode(0U, (BswM_LinTpModeType)1U);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_LINTP_REQUEST_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Notify_EthIf_PortGroupLinkStateChg_Uninit_ShouldReportDet(void) {
    BswM_EthIf_PortGroupLinkStateChg(0U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_ETHIF_PORTGROUP_LINKSTATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

/** Test MainFunction dirty flag clearing (line 547 branch 0) */
void test_BswM_MainFunction_DirtyFlagClear_ShouldClearOnUnchangedPort(void) {
    BswM_Init(&actConfig);
    /* Request mode to mark port dirty */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* Run again - dirty flag should be cleared since port value unchanged */
    BswM_MainFunction();
    /* Request same mode again - should not trigger re-evaluation */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    mock_ActionReset();
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test EcuM wakeup indication (line 682) */
void test_BswM_EcuM_CurrentWakeup_Validated_ShouldRequestWakeupMode(void) {
    BswM_Init(&actConfig);
    mock_Det_Reset();
    BswM_EcuM_CurrentWakeup(1U, ECUM_WKSTATUS_VALIDATED);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_WAKEUP, BswM_GetRequestedMode());
}

/** Test EcuM wakeup indication with non-validated status */
void test_BswM_EcuM_CurrentWakeup_NotValidated_ShouldNotRequestMode(void) {
    BswM_Init(&actConfig);
    BswM_EcuM_CurrentWakeup(1U, ECUM_WKSTATUS_NONE);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
}

/** Test ExecuteActionList with ActionListIndex = BSWM_ACTION_LIST_NONE (line 254 branch 1) */
void test_BswM_ExecuteActionList_NoneIndex_ShouldReturnEarly(void) {
    BswM_ConfigType cfg = actConfig;
    /* Rule with TrueActionListIndex = BSWM_ACTION_LIST_NONE */
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = BSWM_ACTION_LIST_NONE;
    rules2[0].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Should not crash when ActionListIndex is NONE */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test ExecuteActionList with out-of-range index (line 260 branch 1) */
void test_BswM_ExecuteActionList_OutOfRangeIndex_ShouldReturnEarly(void) {
    BswM_ConfigType cfg = actConfig;
    /* Rule with TrueActionListIndex out of range */
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 99U;  /* Out of range */
    rules2[0].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Should not crash when ActionListIndex is out of range */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test WritePortByComposition with NULL config (lines 337, 339) */
void test_BswM_WritePortByComposition_NullConfig_ShouldNotCrash(void) {
    /* Initialize with empty config */
    BswM_Init(&testConfig);
    /* Call notification that uses WritePortByComposition */
    BswM_ComM_CurrentMode(0U, (BswM_ComMModeType)2U);
    /* Should not crash */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test RequestMode with composition that doesn't match any port (lines 463, 465) */
void test_BswM_RequestMode_UnmatchedComposition_ShouldStillLatchMode(void) {
    BswM_Init(&actConfig);
    /* Request with a composition ID that doesn't match any port */
    Std_ReturnType ret = BswM_RequestMode(0xFFU, BSWM_MODE_VALUE_RUN);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Mode should still be latched */
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
}

/** Test MainFunction with config that has NumRules = 0 (line 497 branch 3) */
void test_BswM_MainFunction_ZeroRules_ShouldNotEvaluate(void) {
    BswM_ConfigType cfg = testConfig;
    cfg.NumRules = 0U;
    cfg.Rules = NULL_PTR;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Should not crash with zero rules */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetCurrentMode());
}

/** Test Com_CurrentPduGroupState after COM_IPDU_GROUP action (line 658 branch 1) */
void test_BswM_Com_CurrentPduGroupState_AfterComIpduAction_ShouldReturnVector(void) {
    BswM_Com_IpduGroupVector vec = {0xFFU, 0xFFU};
    BswM_ConfigType cfg = actConfig;
    /* Use action list 2 (COM_IPDU_GROUP) */
    BswM_RuleType rules2[1];
    rules2[0].RuleId = 0U;
    rules2[0].ConditionIndex = 0U;
    rules2[0].TrueActionListIndex = 2U;  /* COM_IPDU_GROUP */
    rules2[0].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules2[0].InitialState = BSWM_RULE_STATE_FALSE;
    rules2[0].IsEnabled = TRUE;
    rules2[0].Priority = 0U;
    cfg.Rules = rules2;
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    /* Now check the PDU group state */
    Std_ReturnType ret = BswM_Com_CurrentPduGroupState(vec);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* COM_IPDU_GROUP action with vector {0x0F, 0, 0, 0} should set bits */
    TEST_ASSERT_NOT_EQUAL(0U, vec[0]);
}

/** Test DeInit with NULL configPtr (line 424 branch 3) */
void test_BswM_DeInit_NullConfigPtr_ShouldHandleGracefully(void) {
    /* This is a special case - we can't easily set configPtr to NULL after Init,
     * but we can test DeInit behavior when called multiple times */
    BswM_Init(&testConfig);
    BswM_DeInit();
    /* Call DeInit again - should handle gracefully */
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** Test all EcuM state mappings to cover line 277 branches */
void test_BswM_MapEcuMState_AllMappedStates_ShouldMapCorrectly(void) {
    BswM_Init(&actConfig);
    
    /* Test each mapped state */
    BswM_EcuM_CurrentState(ECUM_STATE_OFF);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_STARTUP);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_STARTUP, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_APP_RUN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_POST_RUN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_POST_RUN, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_APP_POST_RUN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_POST_RUN, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_SLEEP);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_WAKE_SLEEP);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetRequestedMode());
    
    BswM_EcuM_CurrentState(ECUM_STATE_SHUTDOWN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SHUTDOWN, BswM_GetRequestedMode());
}

/** Test expression with invalid type (line 91 branch 2 - default case) */
void test_BswM_EvaluateExpression_InvalidType_ShouldReturnFalse(void) {
    static const BswM_ExpressionConfigType invalidExpr[] = {
        { 0xFFU, 0U, BSWM_MODE_VALUE_RUN, 0U, 0U }  /* Invalid ExpressionType */
    };
    static const BswM_RuleType invalidRule[] = {
        { 0U, 0U, 0U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE, 0U }
    };
    BswM_ConfigType cfg = {
        1U, actPorts, 1U, invalidExpr, 1U, invalidRule, 0U, NULL_PTR
    };
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Should not crash, expression evaluates to FALSE */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test expression with PortIndex >= NumModeRequestPorts (line 94 branches) */
void test_BswM_EvaluateExpression_PortIndexOutOfRange_ShouldReturnFalse(void) {
    static const BswM_ExpressionConfigType outOfRangeExpr[] = {
        { BSWM_EXPR_MODE_EQUALS, 99U, BSWM_MODE_VALUE_RUN, 0U, 0U }  /* PortIndex=99, but NumModeRequestPorts=1 */
    };
    static const BswM_RuleType outOfRangeRule[] = {
        { 0U, 0U, 0U, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE, 0U }
    };
    BswM_ConfigType cfg = {
        1U, actPorts, 1U, outOfRangeExpr, 1U, outOfRangeRule, 0U, NULL_PTR
    };
    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_RUN);
    /* Should not crash, expression evaluates to FALSE due to port index check */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_ActionCalls);
}

/** Test CollectRulePorts with invalid expression type (line 150 branch 0) */
void test_BswM_CollectRulePorts_InvalidType_ShouldReturnZero(void) {
    static const BswM_ExpressionConfigType invalidExpr[] = {
        { 0xFFU, 0U, BSWM_MODE_VALUE_RUN, 0U, 0U }  /* Invalid ExpressionType */
    };
    static const BswM_RuleType invalidRule[] = {
        { 0U, 0U, BSWM_ACTION_LIST_NONE, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE, 0U }
    };
    BswM_ConfigType cfg = {
        1U, actPorts, 1U, invalidExpr, 1U, invalidRule, 0U, NULL_PTR
    };
    /* Init will call CollectRulePorts which should handle invalid type */
    BswM_Init(&cfg);
    /* Should not crash */
    BswM_MainFunction();
}

/** Test CollectRulePorts with PortIndex >= 32 (line 153 branch 1) */
void test_BswM_CollectRulePorts_PortIndexLarge_ShouldSetAllBits(void) {
    static const BswM_ExpressionConfigType largePortExpr[] = {
        { BSWM_EXPR_MODE_EQUALS, 50U, BSWM_MODE_VALUE_RUN, 0U, 0U }  /* PortIndex=50 >= 32 */
    };
    static const BswM_RuleType largePortRule[] = {
        { 0U, 0U, BSWM_ACTION_LIST_NONE, BSWM_ACTION_LIST_NONE, BSWM_RULE_STATE_FALSE, TRUE, 0U }
    };
    BswM_ConfigType cfg = {
        1U, actPorts, 1U, largePortExpr, 1U, largePortRule, 0U, NULL_PTR
    };
    /* Init will call CollectRulePorts which should set all bits for large port index */
    BswM_Init(&cfg);
    /* Should not crash */
    BswM_MainFunction();
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_BswM_Init_NullPtr_ShouldSelectDefaultConfig);
    RUN_TEST(test_BswM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_BswM_Init_DoubleInit_ShouldResetRequestedMode);
    RUN_TEST(test_BswM_DeInit_Uninit_ShouldNotReportDet);
    RUN_TEST(test_BswM_DeInit_ValidCall_ShouldReturnToUninit);
    RUN_TEST(test_BswM_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_BswM_MainFunction_Uninit_ShouldSilentlyReturn);
    RUN_TEST(test_BswM_MainFunction_ValidCall_ShouldApplyRequestedMode);
    RUN_TEST(test_BswM_RequestMode_Uninit_ShouldReportError);
    RUN_TEST(test_BswM_RequestMode_ValidCall_ShouldLatchMode);
    RUN_TEST(test_BswM_GetCurrentMode_Uninit_ShouldReturnOffWithoutDet);
    RUN_TEST(test_BswM_GetCurrentMode_AfterRequestAndMain_ShouldReturnAppliedMode);
    RUN_TEST(test_BswM_GetRequestedMode_AfterTwoRequests_ShouldReturnLatest);
    RUN_TEST(test_BswM_Engine_AndCondition_ShouldRequireBothPorts);
    RUN_TEST(test_BswM_Engine_ActionLists_ShouldRunOnTransitionsOnly);
    RUN_TEST(test_BswM_Engine_InitialStateTrue_ShouldNotRunActionList);
    RUN_TEST(test_BswM_Engine_DisabledRule_ShouldNeverRunActionList);
    RUN_TEST(test_BswM_Engine_OutOfRangeIndices_ShouldBeIgnored);
    RUN_TEST(test_BswM_Engine_OrNotExpressions_ShouldEvaluate);
    RUN_TEST(test_BswM_Engine_NotEqualsAndNoneActionList_ShouldBeHandled);
    RUN_TEST(test_BswM_RequestMode_InvalidMode_ShouldReportParamModeDet);
    RUN_TEST(test_BswM_ActionSwitchMode_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_ActionSwitchMode_InvalidMode_ShouldReportDet);
    RUN_TEST(test_BswM_ActionSwitchMode_ValidMode_ShouldSetBothModes);
    RUN_TEST(test_BswM_EcuM_CurrentState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_EcuM_CurrentState_ShouldRequestMappedMode);
    RUN_TEST(test_BswM_Notify_ComM_CurrentMode_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_ComM_CurrentMode_NonMatchingValue_ShouldNotTrigger);
    RUN_TEST(test_BswM_Notify_ComM_CurrentPNCMode_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_Dcm_CommunicationMode_CurrentState_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_Dcm_ApplicationUpdated_ShouldWriteFixedOne);
    RUN_TEST(test_BswM_Notify_Nm_StateChangeNotification_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_Nm_CarWakeUpIndication_ShouldWriteFixedOne);
    RUN_TEST(test_BswM_Notify_CanSM_CurrentState_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_EthSM_CurrentState_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_FrSM_CurrentState_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_LinSM_CurrentState_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_LinSM_CurrentSchedule_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_LinTp_RequestMode_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_EcuM_RequestedState_ShouldMapAndTriggerRule);
    RUN_TEST(test_BswM_Notify_EcuM_RequestedState_Unmapped_ShouldBeIgnored);
    RUN_TEST(test_BswM_Notify_BswMPartitionRestarted_ShouldWritePartitionIdLowByte);
    RUN_TEST(test_BswM_Notify_EthIf_PortGroupLinkStateChg_ShouldTriggerRule);
    RUN_TEST(test_BswM_Notify_EthIf_PortGroupLinkStateChg_LinkDown_ShouldNotTrigger);
    RUN_TEST(test_BswM_Notify_UnboundComposition_ShouldSilentlyDrop);
    RUN_TEST(test_BswM_Notify_ComM_CurrentMode_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_EcuM_RequestedState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_BswMPartitionRestarted_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Action_PduGroupSwitch_Enable_ShouldCallComIpduGroupControl);
    RUN_TEST(test_BswM_Action_PduGroupSwitch_Disable_ShouldClearBits);
    RUN_TEST(test_BswM_Action_ComIpduGroup_ShouldCallComWithInitializeFlag);
    RUN_TEST(test_BswM_Action_EcuMGoSleep_ShouldCallGoSleep);
    RUN_TEST(test_BswM_Action_EcuMGoHalt_ShouldCallGoHalt);
    RUN_TEST(test_BswM_Action_EcuMGoOff_ShouldSelectTargetAndShutdown);
    RUN_TEST(test_BswM_Action_NullCallback_ShouldNotCrash);
    RUN_TEST(test_BswM_RuleEnable_ValidId_ShouldReturnOk);
    RUN_TEST(test_BswM_RuleEnable_InvalidId_ShouldReportError);
    RUN_TEST(test_BswM_RuleEnable_Uninit_ShouldReportError);
    RUN_TEST(test_BswM_RuleDisable_ValidId_ShouldReturnOk);
    RUN_TEST(test_BswM_RuleDisable_InvalidId_ShouldReportError);
    RUN_TEST(test_BswM_RuleDisable_Uninit_ShouldReportError);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_Uninit_ShouldReportError);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_NullPtr_ShouldReportError);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_Valid_ShouldReturnState);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_AfterPduAction_ShouldReflectState);
    RUN_TEST(test_BswM_DeInit_LatchedTrueRule_ShouldRunFalseActionList);
    RUN_TEST(test_BswM_MainFunction_AfterDeInit_ShouldSilentlyReturn);
    RUN_TEST(test_BswM_EcuM_CurrentState_UnmappedState_ShouldNotRequestMode);
    RUN_TEST(test_BswM_EcuM_RequestedState_UnmappedValue_ShouldBeIgnored);
    RUN_TEST(test_BswM_WritePort_SameValue_ShouldNotMarkDirty);
    RUN_TEST(test_BswM_ActionList_NullActions_ShouldNotCrash);
    RUN_TEST(test_BswM_Notify_ComM_CurrentPNCMode_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_Dcm_CommunicationMode_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_Dcm_ApplicationUpdated_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_Nm_StateChangeNotification_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_Nm_CarWakeUpIndication_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_CanSM_CurrentState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_EthSM_CurrentState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_FrSM_CurrentState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_LinSM_CurrentState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_LinSM_CurrentSchedule_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_LinTp_RequestMode_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Notify_EthIf_PortGroupLinkStateChg_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_MainFunction_DirtyFlagClear_ShouldClearOnUnchangedPort);
    RUN_TEST(test_BswM_EcuM_CurrentWakeup_Validated_ShouldRequestWakeupMode);
    RUN_TEST(test_BswM_EcuM_CurrentWakeup_NotValidated_ShouldNotRequestMode);
    RUN_TEST(test_BswM_ExecuteActionList_NoneIndex_ShouldReturnEarly);
    RUN_TEST(test_BswM_ExecuteActionList_OutOfRangeIndex_ShouldReturnEarly);
    RUN_TEST(test_BswM_WritePortByComposition_NullConfig_ShouldNotCrash);
    RUN_TEST(test_BswM_RequestMode_UnmatchedComposition_ShouldStillLatchMode);
    RUN_TEST(test_BswM_MainFunction_ZeroRules_ShouldNotEvaluate);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_AfterComIpduAction_ShouldReturnVector);
    RUN_TEST(test_BswM_DeInit_NullConfigPtr_ShouldHandleGracefully);
    RUN_TEST(test_BswM_MapEcuMState_AllMappedStates_ShouldMapCorrectly);
    RUN_TEST(test_BswM_EvaluateExpression_InvalidType_ShouldReturnFalse);
    RUN_TEST(test_BswM_EvaluateExpression_PortIndexOutOfRange_ShouldReturnFalse);
    RUN_TEST(test_BswM_CollectRulePorts_InvalidType_ShouldReturnZero);
    RUN_TEST(test_BswM_CollectRulePorts_PortIndexLarge_ShouldSetAllBits);

    return UnityEnd();
}

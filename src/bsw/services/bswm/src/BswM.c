/** @file BswM.c
 *  @brief BSW Mode Manager - Rule/ActionList engine implementation
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_BSWModeManager.pdf
 *
 *  Evaluation model: BswM_RequestMode() only latches requests. Rule
 *  conditions are evaluated in BswM_MainFunction() (10ms task context,
 *  wired via OsAlarm_BswM_MainFunction). Action lists run on rule state
 *  transitions only.
 */

#include "BswM.h"
#include "EcuM.h"
#include "Det.h"

/* Version check */
#if defined(BSWM_AR_RELEASE_MAJOR_VERSION) && (BSWM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "BswM: AR major mismatch"
#endif
#if defined(BSWM_AR_RELEASE_MINOR_VERSION) && (BSWM_AR_RELEASE_MINOR_VERSION != 4u)
#error "BswM: AR minor mismatch"
#endif

/* Sentinel: no mode has been requested on a port yet */
#define BSWM_MODE_VALUE_NONE        0xFFU

/* Expression tree recursion bound (config tables are static and shallow) */
#define BSWM_EXPR_MAX_DEPTH         16U

typedef enum { BSWM_INTERNAL_UNINIT = 0, BSWM_INTERNAL_INIT } BswM_InternalStateType;

typedef struct {
    BswM_InternalStateType  internalState;
    BswM_ModeType           currentMode;
    BswM_ModeType           requestedMode;
    boolean                 requestPending;
    BswM_ModeType           portValue[BSWM_MAX_MODE_REQUEST_PORTS];
    BswM_RuleStateType      ruleState[BSWM_MAX_RULES];
    BswM_EcuMWakeupSourceType wakeupSource;
    BswM_EcuMWakeupStatusType wakeupStatus;
    const BswM_ConfigType*  configPtr;
} BswM_InternalType;

static BswM_InternalType BswM_State = {
    BSWM_INTERNAL_UNINIT,
    BSWM_MODE_VALUE_OFF,
    BSWM_MODE_VALUE_OFF,
    FALSE,
    { 0U },
    { 0U },
    0U,
    ECUM_WKSTATUS_NONE,
    NULL_PTR
};

static void BswM_ExecuteActionList(uint16 ActionListIndex);

static boolean BswM_EvaluateExpression(uint16 ExpressionIndex, uint8 Depth)
{
    const BswM_ExpressionConfigType* expr;
    boolean result = FALSE;

    if ((BswM_State.configPtr == NULL_PTR) || (Depth > BSWM_EXPR_MAX_DEPTH)) {
        return FALSE;
    }
    if ((ExpressionIndex >= BswM_State.configPtr->NumExpressions) ||
        (BswM_State.configPtr->Expressions == NULL_PTR)) {
        return FALSE;
    }

    expr = &BswM_State.configPtr->Expressions[ExpressionIndex];

    switch (expr->ExpressionType) {
        case BSWM_EXPR_MODE_EQUALS:
        case BSWM_EXPR_MODE_NOT_EQUALS:
            if ((expr->PortIndex < BswM_State.configPtr->NumModeRequestPorts) &&
                (BswM_State.configPtr->ModeRequestPorts != NULL_PTR)) {
                boolean matches = (boolean)(BswM_State.portValue[expr->PortIndex] == expr->CompareValue);
                result = (expr->ExpressionType == BSWM_EXPR_MODE_EQUALS)
                             ? matches
                             : (boolean)(matches == FALSE);
            }
            break;

        case BSWM_EXPR_LOGICAL_AND:
            result = (boolean)((BswM_EvaluateExpression(expr->LeftIndex, (uint8)(Depth + 1U)) == TRUE) &&
                               (BswM_EvaluateExpression(expr->RightIndex, (uint8)(Depth + 1U)) == TRUE));
            break;

        case BSWM_EXPR_LOGICAL_OR:
            result = (boolean)((BswM_EvaluateExpression(expr->LeftIndex, (uint8)(Depth + 1U)) == TRUE) ||
                               (BswM_EvaluateExpression(expr->RightIndex, (uint8)(Depth + 1U)) == TRUE));
            break;

        case BSWM_EXPR_LOGICAL_NOT:
            result = (boolean)(BswM_EvaluateExpression(expr->LeftIndex, (uint8)(Depth + 1U)) == FALSE);
            break;

        default:
            result = FALSE;
            break;
    }

    return result;
}

static void BswM_ExecuteActionList(uint16 ActionListIndex)
{
    const BswM_ActionListType* list;
    uint8 i;

    if ((BswM_State.configPtr == NULL_PTR) || (ActionListIndex == BSWM_ACTION_LIST_NONE)) {
        return;
    }
    if (ActionListIndex >= BswM_State.configPtr->NumActionLists) {
        return;
    }
    if (BswM_State.configPtr->ActionLists == NULL_PTR) {
        return;
    }

    list = &BswM_State.configPtr->ActionLists[ActionListIndex];
    for (i = 0U; i < list->NumActions; i++) {
        if ((list->Actions != NULL_PTR) && (list->Actions[i].Callback != NULL_PTR)) {
            list->Actions[i].Callback(list->Actions[i].Parameter);
        }
    }
}

static BswM_ModeType BswM_MapEcuMState(EcuM_StateType EcuMState, boolean* IsMapped)
{
    BswM_ModeType mode = BSWM_MODE_VALUE_OFF;

    *IsMapped = TRUE;
    switch (EcuMState) {
        case ECUM_STATE_OFF:
            mode = BSWM_MODE_VALUE_OFF;
            break;
        case ECUM_STATE_STARTUP:
            mode = BSWM_MODE_VALUE_STARTUP;
            break;
        case ECUM_STATE_RUN:
        case ECUM_STATE_APP_RUN:
            mode = BSWM_MODE_VALUE_RUN;
            break;
        case ECUM_STATE_POST_RUN:
        case ECUM_STATE_APP_POST_RUN:
            mode = BSWM_MODE_VALUE_POST_RUN;
            break;
        case ECUM_STATE_SLEEP:
        case ECUM_STATE_WAKE_SLEEP:
            mode = BSWM_MODE_VALUE_SLEEP;
            break;
        case ECUM_STATE_SHUTDOWN:
            mode = BSWM_MODE_VALUE_SHUTDOWN;
            break;
        default:
            *IsMapped = FALSE;
            break;
    }

    return mode;
}

static void BswM_ResetInternalState(const BswM_ConfigType* ConfigPtr)
{
    uint8 i;

    BswM_State.configPtr = ConfigPtr;
    BswM_State.currentMode = BSWM_MODE_VALUE_OFF;
    BswM_State.requestedMode = BSWM_MODE_VALUE_OFF;
    BswM_State.requestPending = FALSE;
    BswM_State.wakeupSource = 0U;
    BswM_State.wakeupStatus = ECUM_WKSTATUS_NONE;

    for (i = 0U; i < BSWM_MAX_MODE_REQUEST_PORTS; i++) {
        BswM_State.portValue[i] = BSWM_MODE_VALUE_NONE;
    }
    for (i = 0U; i < BSWM_MAX_RULES; i++) {
        if ((ConfigPtr != NULL_PTR) && (ConfigPtr->Rules != NULL_PTR) && (i < ConfigPtr->NumRules)) {
            BswM_State.ruleState[i] = ConfigPtr->Rules[i].InitialState;
        } else {
            BswM_State.ruleState[i] = BSWM_RULE_STATE_FALSE;
        }
    }
}

/** @req SWS_BswM_00001 */
void BswM_Init(const BswM_ConfigType* ConfigPtr)
{
    /* Pre-compile configuration: NULL selects the default config object */
    if (NULL_PTR == ConfigPtr) {
        ConfigPtr = &BswM_Config;
    }

    BswM_ResetInternalState(ConfigPtr);
    BswM_State.internalState = BSWM_INTERNAL_INIT;
}

/** @req SWS_BswM_00002 */
void BswM_DeInit(void)
{
    BswM_ResetInternalState(NULL_PTR);
    BswM_State.internalState = BSWM_INTERNAL_UNINIT;
}

/** @req SWS_BswM_00010 */
Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode)
{
    uint8 i;

#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }
    if (Mode > BSWM_MODE_VALUE_MAX) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_PARAM_MODE);
        return E_NOT_OK;
    }
#endif

    /* Update every port bound to this composition; unmatched compositions
     * are still latched for BswM_GetRequestedMode() compatibility. */
    if ((BswM_State.configPtr != NULL_PTR) && (BswM_State.configPtr->ModeRequestPorts != NULL_PTR)) {
        for (i = 0U; i < BswM_State.configPtr->NumModeRequestPorts; i++) {
            if ((i < BSWM_MAX_MODE_REQUEST_PORTS) &&
                (BswM_State.configPtr->ModeRequestPorts[i] == SwCompositionId)) {
                BswM_State.portValue[i] = Mode;
            }
        }
    }

    BswM_State.requestedMode = Mode;
    BswM_State.requestPending = TRUE;
    return E_OK;
}

/** @req SWS_BswM_00011 */
BswM_ModeType BswM_GetCurrentMode(void)
{
    return BswM_State.currentMode;
}

/** @req SWS_BswM_00012 */
BswM_ModeType BswM_GetRequestedMode(void)
{
    return BswM_State.requestedMode;
}

/** @req SWS_BswM_00020 */
void BswM_MainFunction(void)
{
    uint8 i;

    if ((BswM_State.internalState == BSWM_INTERNAL_UNINIT) || (BswM_State.configPtr == NULL_PTR)) {
        return;
    }

    /* Rule evaluation: action lists execute on state transitions only */
    for (i = 0U; i < BswM_State.configPtr->NumRules; i++) {
        const BswM_RuleType* rule;
        BswM_RuleStateType result;

        if ((i >= BSWM_MAX_RULES) || (BswM_State.configPtr->Rules == NULL_PTR)) {
            break;
        }
        rule = &BswM_State.configPtr->Rules[i];
        if (rule->IsEnabled == FALSE) {
            continue;
        }

        result = (BswM_EvaluateExpression(rule->ConditionIndex, 0U) == TRUE)
                     ? BSWM_RULE_STATE_TRUE
                     : BSWM_RULE_STATE_FALSE;

        if (result != BswM_State.ruleState[i]) {
            BswM_State.ruleState[i] = result;
            BswM_ExecuteActionList((result == BSWM_RULE_STATE_TRUE)
                                       ? rule->TrueActionListIndex
                                       : rule->FalseActionListIndex);
        }
    }

    /* Apply the latched request (legacy direct-apply semantics) */
    if (BswM_State.requestPending == TRUE) {
        BswM_State.currentMode = BswM_State.requestedMode;
        BswM_State.requestPending = FALSE;
    }
}

/** @req SWS_BswM_00030 */
void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_GETVERSIONINFO, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = BSWM_VENDOR_ID;
    versioninfo->moduleID = BSWM_MODULE_ID;
    versioninfo->sw_major_version = BSWM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = BSWM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = BSWM_SW_PATCH_VERSION;
}

void BswM_ActionSwitchMode(BswM_ModeType Mode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_SWITCH_MODE, BSWM_E_UNINIT);
        return;
    }
    if (Mode > BSWM_MODE_VALUE_MAX) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_SWITCH_MODE, BSWM_E_PARAM_MODE);
        return;
    }
#endif
    BswM_State.currentMode = Mode;
    BswM_State.requestedMode = Mode;
}

/** @req SWS_BswM_00210 */
void BswM_EcuM_CurrentState(BswM_EcuMStateType CurrentState)
{
    boolean isMapped = FALSE;
    BswM_ModeType mode = BswM_MapEcuMState((EcuM_StateType)CurrentState, &isMapped);

    if (isMapped == TRUE) {
        (void)BswM_RequestMode(BSWM_ECUM_REQUEST, mode);
    }
}

/** @req SWS_BswM_00211 */
void BswM_EcuM_CurrentWakeup(BswM_EcuMWakeupSourceType WakeupSource,
                             BswM_EcuMWakeupStatusType WakeupStatus)
{
    BswM_State.wakeupSource = WakeupSource;
    BswM_State.wakeupStatus = WakeupStatus;

    if (WakeupStatus == ECUM_WKSTATUS_VALIDATED) {
        (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_WAKEUP);
    }
}

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
 *
 *  Performance model: every mode request port carries a dirty flag, set on
 *  value changes only. Each rule records the bitmask of the ports referenced
 *  by its condition tree, so a cycle evaluates only rules that depend on a
 *  port written since the last cycle. Rules are visited in Priority order
 *  (stable sort, 0 = highest priority first).
 */

#include "BswM.h"
#include "EcuM.h"
#include "Com.h"
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
    boolean                 portDirty[BSWM_MAX_MODE_REQUEST_PORTS];
    BswM_RuleStateType      ruleState[BSWM_MAX_RULES];
    boolean                 ruleEnabled[BSWM_MAX_RULES];
    uint32                  rulePortMask[BSWM_MAX_RULES];
    uint8                   ruleOrder[BSWM_MAX_RULES];
    uint8                   ipduGroupState[BSWM_ACTION_VECTOR_MAX_BYTES];
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
    { FALSE },
    { 0U },
    { FALSE },
    { 0UL },
    { 0U },
    { 0U },
    0U,
    ECUM_WKSTATUS_NONE,
    NULL_PTR
};

void BswM_ExecuteActionList(uint16 ActionListIndex);

/* BswM_EvaluateExpression 显式工作栈帧 (MISRA 17.2 迭代求值) */
typedef struct {
    uint16 index;
    uint8  phase; /* 0 = expand children, 1 = compute result */
    uint8  depth;
} BswM_EvalFrameType;

/* BswM_EvaluateExpression 等价重构助手 */
static void BswM_EvalExpand(const BswM_ExpressionConfigType* expr, const BswM_EvalFrameType* frame,
                            BswM_EvalFrameType* workStack, uint8* wsp, uint8 capacity);
static void BswM_EvalCompute(const BswM_ExpressionConfigType* expr, boolean* valStack, uint8* vsp);

static boolean BswM_EvaluateExpression(uint16 ExpressionIndex, uint8 Depth)
{
    /* Iterative expression-tree evaluation (MISRA 17.2: no recursion).
     * Uses an explicit work stack (index + phase + depth) and a value stack.
     * Depth is bounded by BSWM_EXPR_MAX_DEPTH (16), so stack sizes are safe. */
    BswM_EvalFrameType workStack[(BSWM_EXPR_MAX_DEPTH * 2U) + 2U];
    boolean            valStack[(BSWM_EXPR_MAX_DEPTH * 2U) + 2U];
    uint8              wsp = 0U;
    uint8              vsp = 0U;
    boolean            result = FALSE;

    if ((BswM_State.configPtr == NULL_PTR) || (Depth > BSWM_EXPR_MAX_DEPTH)) {
        return FALSE;
    }
    if ((ExpressionIndex >= BswM_State.configPtr->NumExpressions) ||
        (BswM_State.configPtr->Expressions == NULL_PTR)) {
        return FALSE;
    }

    workStack[wsp].index = ExpressionIndex;
    workStack[wsp].phase = 0U;
    workStack[wsp].depth = Depth;
    wsp++;

    while (wsp > 0U) {
        BswM_EvalFrameType frame = workStack[--wsp];
        const BswM_ExpressionConfigType* expr;

        if ((frame.index >= BswM_State.configPtr->NumExpressions) ||
            (BswM_State.configPtr->Expressions == NULL_PTR) ||
            (frame.depth > BSWM_EXPR_MAX_DEPTH)) {
            valStack[vsp] = FALSE;
            vsp++;
            continue;
        }
        expr = &BswM_State.configPtr->Expressions[frame.index];

        if (frame.phase == 0U) {
            BswM_EvalExpand(expr, &frame, workStack, &wsp,
                            (uint8)((BSWM_EXPR_MAX_DEPTH * 2U) + 2U));
        } else {
            BswM_EvalCompute(expr, valStack, &vsp);
        }
    }

    if (vsp > 0U) {
        result = valStack[vsp - 1U];
    }
    return result;
}

/**
 * @brief Phase 0: 重压自身 (phase 1) 并按表达式类型压入子节点 (等价重构助手)
 * @note  子节点压栈沿用原实现, 不做二次容量检查 (与原代码一致)
 */
static void BswM_EvalExpand(const BswM_ExpressionConfigType* expr, const BswM_EvalFrameType* frame,
                            BswM_EvalFrameType* workStack, uint8* wsp, uint8 capacity)
{
    /* Phase 0: re-push self as phase 1, then push children (right-first so left pops first) */
    if (*wsp < capacity) {
        workStack[*wsp].index = frame->index;
        workStack[*wsp].phase = 1U;
        workStack[*wsp].depth = frame->depth;
        (*wsp)++;
    }
    switch (expr->ExpressionType) {
        case BSWM_EXPR_LOGICAL_AND:
        case BSWM_EXPR_LOGICAL_OR:
            workStack[*wsp].index = expr->RightIndex;
            workStack[*wsp].phase = 0U;
            workStack[*wsp].depth = (uint8)(frame->depth + 1U);
            (*wsp)++;
            workStack[*wsp].index = expr->LeftIndex;
            workStack[*wsp].phase = 0U;
            workStack[*wsp].depth = (uint8)(frame->depth + 1U);
            (*wsp)++;
            break;
        case BSWM_EXPR_LOGICAL_NOT:
            workStack[*wsp].index = expr->LeftIndex;
            workStack[*wsp].phase = 0U;
            workStack[*wsp].depth = (uint8)(frame->depth + 1U);
            (*wsp)++;
            break;
        default:
            break; /* leaf evaluated in phase 1 */
    }
}

/**
 * @brief Phase 1: 从值栈弹出子节点计算本节点值 (等价重构助手)
 */
static void BswM_EvalCompute(const BswM_ExpressionConfigType* expr, boolean* valStack, uint8* vsp)
{
    /* Phase 1: compute node value from children already on valStack */
    boolean val;
    switch (expr->ExpressionType) {
        case BSWM_EXPR_MODE_EQUALS:
        case BSWM_EXPR_MODE_NOT_EQUALS:
            if ((expr->PortIndex < BswM_State.configPtr->NumModeRequestPorts) &&
                (BswM_State.configPtr->ModeRequestPorts != NULL_PTR)) {
                boolean matches = (boolean)(BswM_State.portValue[expr->PortIndex] == expr->CompareValue);
                val = (expr->ExpressionType == BSWM_EXPR_MODE_EQUALS)
                          ? matches
                          : (boolean)(matches == FALSE);
            } else {
                val = FALSE;
            }
            valStack[*vsp] = val;
            (*vsp)++;
            break;

        case BSWM_EXPR_LOGICAL_AND: {
            boolean right;
            boolean left;
            (*vsp)--;
            right = valStack[*vsp];
            (*vsp)--;
            left = valStack[*vsp];
            valStack[*vsp] = (boolean)(left && right);
            (*vsp)++;
            break;
        }

        case BSWM_EXPR_LOGICAL_OR: {
            boolean right;
            boolean left;
            (*vsp)--;
            right = valStack[*vsp];
            (*vsp)--;
            left = valStack[*vsp];
            valStack[*vsp] = (boolean)(left || right);
            (*vsp)++;
            break;
        }

        case BSWM_EXPR_LOGICAL_NOT: {
            boolean child;
            (*vsp)--;
            child = valStack[*vsp];
            valStack[*vsp] = (boolean)(child == FALSE);
            (*vsp)++;
            break;
        }

        default:
            valStack[*vsp] = FALSE;
            (*vsp)++;
            break;
    }
}

/**
 * @brief Bitmask of the mode request ports referenced by a condition tree.
 *
 * Establishes the rule -> port association used by the dirty-flag gate: the
 * port indices are already part of the leaf configuration (PortIndex of the
 * MODE_EQUALS / MODE_NOT_EQUALS expressions). A leaf referencing a port
 * outside the bitmask width marks the rule as always-evaluated (all bits
 * set) so a wider configuration degrades to the legacy full-scan behavior
 * instead of losing the rule.
 */
/* BswM_CollectRulePorts 显式工作栈帧 (MISRA 17.2 迭代遍历) */
typedef struct {
    uint16 index;
    uint8  phase; /* 0 = expand, 1 = combine */
    uint8  depth;
} BswM_ColFrame;

/* BswM_CollectRulePorts 等价重构助手 */
static boolean BswM_ColValidate(uint16 ExpressionIndex, uint8 Depth);
static void BswM_ColExpand(const BswM_ExpressionConfigType* expr, const BswM_ColFrame* frame,
                           BswM_ColFrame* workStack, uint8* wsp);
static void BswM_ColCombine(const BswM_ExpressionConfigType* expr,
                            uint32* maskStack, uint8* msp);

static uint32 BswM_CollectRulePorts(uint16 ExpressionIndex, uint8 Depth)
{
    /* Iterative tree walk (MISRA 17.2: no recursion).
     * Collects the OR of all leaf port-index bitmasks. */
    BswM_ColFrame workStack[(BSWM_EXPR_MAX_DEPTH * 2U) + 2U];
    uint32        maskStack[(BSWM_EXPR_MAX_DEPTH * 2U) + 2U];
    uint8         wsp = 0U;
    uint8         msp = 0U;
    uint32        mask = 0UL;

    if (!BswM_ColValidate(ExpressionIndex, Depth)) {
        return 0UL;
    }

    workStack[wsp].index = ExpressionIndex;
    workStack[wsp].phase = 0U;
    workStack[wsp].depth = Depth;
    wsp++;

    while (wsp > 0U) {
        BswM_ColFrame frame = workStack[--wsp];
        const BswM_ExpressionConfigType* expr;

        if ((frame.index >= BswM_State.configPtr->NumExpressions) ||
            (BswM_State.configPtr->Expressions == NULL_PTR) ||
            (frame.depth > BSWM_EXPR_MAX_DEPTH)) {
            maskStack[msp] = 0UL;
            msp++;
            continue;
        }
        expr = &BswM_State.configPtr->Expressions[frame.index];

        if (frame.phase == 0U) {
            BswM_ColExpand(expr, &frame, workStack, &wsp);
        } else {
            BswM_ColCombine(expr, maskStack, &msp);
        }
    }

    if (msp > 0U) {
        mask = maskStack[msp - 1U];
    }
    return mask;
}

/**
 * @brief BswM_CollectRulePorts 入口校验 (等价重构助手)
 */
static boolean BswM_ColValidate(uint16 ExpressionIndex, uint8 Depth)
{
    if ((BswM_State.configPtr == NULL_PTR) || (Depth > BSWM_EXPR_MAX_DEPTH)) {
        return FALSE;
    }
    if ((ExpressionIndex >= BswM_State.configPtr->NumExpressions) ||
        (BswM_State.configPtr->Expressions == NULL_PTR)) {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Phase 0: 重压自身 (phase 1) 并按表达式类型压入子节点 (等价重构助手)
 * @note  子节点压栈沿用原实现, 不做二次容量检查 (与原代码一致)
 */
static void BswM_ColExpand(const BswM_ExpressionConfigType* expr, const BswM_ColFrame* frame,
                           BswM_ColFrame* workStack, uint8* wsp)
{
    /* Phase 0: re-push self as phase 1, then push children (right-first so left pops first) */
    workStack[*wsp].index = frame->index;
    workStack[*wsp].phase = 1U;
    workStack[*wsp].depth = frame->depth;
    (*wsp)++;
    switch (expr->ExpressionType) {
        case BSWM_EXPR_LOGICAL_AND:
        case BSWM_EXPR_LOGICAL_OR:
            workStack[*wsp].index = expr->RightIndex;
            workStack[*wsp].phase = 0U;
            workStack[*wsp].depth = (uint8)(frame->depth + 1U);
            (*wsp)++;
            workStack[*wsp].index = expr->LeftIndex;
            workStack[*wsp].phase = 0U;
            workStack[*wsp].depth = (uint8)(frame->depth + 1U);
            (*wsp)++;
            break;
        case BSWM_EXPR_LOGICAL_NOT:
            workStack[*wsp].index = expr->LeftIndex;
            workStack[*wsp].phase = 0U;
            workStack[*wsp].depth = (uint8)(frame->depth + 1U);
            (*wsp)++;
            break;
        default:
            break;
    }
}

/**
 * @brief Phase 1: 从掩码栈合并子节点掩码 (等价重构助手)
 */
static void BswM_ColCombine(const BswM_ExpressionConfigType* expr,
                            uint32* maskStack, uint8* msp)
{
    /* Phase 1: combine leaf port masks / child masks from maskStack */
    uint32 m;
    switch (expr->ExpressionType) {
        case BSWM_EXPR_MODE_EQUALS:
        case BSWM_EXPR_MODE_NOT_EQUALS:
            if (expr->PortIndex < BSWM_MAX_MODE_REQUEST_PORTS) {
                m = (1UL << expr->PortIndex);
            } else {
                m = 0xFFFFFFFFUL;
            }
            maskStack[*msp] = m;
            (*msp)++;
            break;

        case BSWM_EXPR_LOGICAL_AND:
        case BSWM_EXPR_LOGICAL_OR: {
            uint32 right = maskStack[--(*msp)];
            uint32 left  = maskStack[--(*msp)];
            maskStack[*msp] = (left | right);
            (*msp)++;
            break;
        }

        case BSWM_EXPR_LOGICAL_NOT: {
            uint32 child = maskStack[--(*msp)];
            maskStack[*msp] = child;
            (*msp)++;
            break;
        }

        default:
            maskStack[*msp] = 0UL;
            (*msp)++;
            break;
    }
}

/**
 * @brief Execute a single action entry according to its Kind.
 */
static void BswM_ExecuteAction(const BswM_ActionType* Action)
{
    uint8 i;
    Com_IpduGroupVector vector;
    const uint8 comVectorBytes = (uint8)sizeof(Com_IpduGroupVector);

    switch (Action->Kind) {
        case BSWM_ACTION_CALLBACK:
            if (Action->Callback != NULL_PTR) {
                Action->Callback(Action->Parameter);
            }
            break;

        case BSWM_ACTION_PDU_GROUP_SWITCH:
            /* Merge semantics: set/clear the configured group bits in the
             * BswM shadow state, then hand the full vector to Com. Bits
             * beyond the Com vector width are ignored. */
            for (i = 0U; i < BSWM_ACTION_VECTOR_MAX_BYTES; i++) {
                if (i < comVectorBytes) {
                    if (Action->Enable == TRUE) {
                        BswM_State.ipduGroupState[i] |= Action->IpduGroupVector[i];
                    } else {
                        BswM_State.ipduGroupState[i] &= (uint8)(~Action->IpduGroupVector[i]);
                    }
                }
            }
            for (i = 0U; i < comVectorBytes; i++) {
                vector[i] = (i < BSWM_ACTION_VECTOR_MAX_BYTES)
                                ? BswM_State.ipduGroupState[i] : 0U;
            }
            Com_IpduGroupControl(vector, FALSE);
            break;

        case BSWM_ACTION_COM_IPDU_GROUP:
            /* Absolute control: the configured vector IS the new state */
            for (i = 0U; i < BSWM_ACTION_VECTOR_MAX_BYTES; i++) {
                BswM_State.ipduGroupState[i] =
                    (i < comVectorBytes) ? Action->IpduGroupVector[i] : 0U;
            }
            for (i = 0U; i < comVectorBytes; i++) {
                vector[i] = (i < BSWM_ACTION_VECTOR_MAX_BYTES)
                                ? BswM_State.ipduGroupState[i] : 0U;
            }
            Com_IpduGroupControl(vector, Action->Initialize);
            break;

        case BSWM_ACTION_ECUM_GO_SLEEP:
            EcuM_GoSleep();
            break;

        case BSWM_ACTION_ECUM_GO_HALT:
            EcuM_GoHalt();
            break;

        case BSWM_ACTION_ECUM_GO_OFF:
            /* This EcuM provides no dedicated GoOff callout: select the OFF
             * shutdown target (shutdown mode carried by Parameter), then
             * trigger the shutdown sequence. */
            (void)EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_OFF, Action->Parameter);
            EcuM_Shutdown();
            break;

        default:
            /* Unknown kind: ignore */
            break;
    }
}

void BswM_ExecuteActionList(uint16 ActionListIndex)
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
        if (list->Actions != NULL_PTR) {
            BswM_ExecuteAction(&list->Actions[i]);
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

/**
 * @brief Write a mode request port and mark it dirty on a real value change.
 *
 * Re-writes of the current value (e.g. repeated notifications of the same
 * state) stay clean, so dependent rules are not re-evaluated needlessly.
 * The very first write is always a change: ports reset to the NONE sentinel.
 */
static void BswM_WritePort(uint8 PortIndex, uint8 RawValue)
{
    if (PortIndex >= BSWM_MAX_MODE_REQUEST_PORTS) {
        return;
    }
    if (BswM_State.portValue[PortIndex] != RawValue) {
        BswM_State.portValue[PortIndex] = RawValue;
        BswM_State.portDirty[PortIndex] = TRUE;
    }
}

/**
 * @brief Write a raw module state to every mode request port bound to a composition.
 *
 * Used by the BswM_* notification callbacks: the value is the module's raw
 * state (e.g. ComM mode 0..2, Nm_StateType) and bypasses the BswM_RequestMode()
 * mode-domain validation/latching. A composition not bound to any port is
 * silently dropped — a notification broadcasts a fact, it does not request.
 */
static void BswM_WritePortByComposition(uint8 CompositionId, uint8 RawValue)
{
    uint8 i;

    if ((BswM_State.configPtr != NULL_PTR) && (BswM_State.configPtr->ModeRequestPorts != NULL_PTR)) {
        for (i = 0U; i < BswM_State.configPtr->NumModeRequestPorts; i++) {
            if ((i < BSWM_MAX_MODE_REQUEST_PORTS) &&
                (BswM_State.configPtr->ModeRequestPorts[i] == CompositionId)) {
                BswM_WritePort(i, RawValue);
            }
        }
    }
}

static void BswM_ResetInternalState(const BswM_ConfigType* ConfigPtr)
{
    uint8 i;
    uint8 k;
    uint8 numRules = 0U;

    BswM_State.configPtr = ConfigPtr;
    BswM_State.currentMode = BSWM_MODE_VALUE_OFF;
    BswM_State.requestedMode = BSWM_MODE_VALUE_OFF;
    BswM_State.requestPending = FALSE;
    BswM_State.wakeupSource = 0U;
    BswM_State.wakeupStatus = ECUM_WKSTATUS_NONE;

    /* All ports start dirty: the first BswM_MainFunction evaluates every
     * rule once to reconcile InitialState with the actual conditions. */
    for (i = 0U; i < BSWM_MAX_MODE_REQUEST_PORTS; i++) {
        BswM_State.portValue[i] = BSWM_MODE_VALUE_NONE;
        BswM_State.portDirty[i] = TRUE;
    }
    for (i = 0U; i < BSWM_ACTION_VECTOR_MAX_BYTES; i++) {
        BswM_State.ipduGroupState[i] = 0U;
    }

    for (i = 0U; i < BSWM_MAX_RULES; i++) {
        if ((ConfigPtr != NULL_PTR) && (ConfigPtr->Rules != NULL_PTR) && (i < ConfigPtr->NumRules)) {
            BswM_State.ruleState[i] = ConfigPtr->Rules[i].InitialState;
            BswM_State.ruleEnabled[i] = ConfigPtr->Rules[i].IsEnabled;
            BswM_State.rulePortMask[i] = BswM_CollectRulePorts(ConfigPtr->Rules[i].ConditionIndex, 0U);
        } else {
            BswM_State.ruleState[i] = BSWM_RULE_STATE_FALSE;
            BswM_State.ruleEnabled[i] = FALSE;
            BswM_State.rulePortMask[i] = 0UL;
        }
        BswM_State.ruleOrder[i] = i;
    }

    /* Build the priority-ordered evaluation sequence: stable insertion sort
     * on Priority (0 = highest, evaluated first). Equal priorities keep the
     * configuration order, preserving the legacy behavior for tables without
     * explicit priorities. */
    if ((ConfigPtr != NULL_PTR) && (ConfigPtr->Rules != NULL_PTR)) {
        numRules = (ConfigPtr->NumRules < BSWM_MAX_RULES) ? ConfigPtr->NumRules : BSWM_MAX_RULES;

        for (k = 1U; k < numRules; k++) {
            uint8 moving = BswM_State.ruleOrder[k];
            uint8 j = k;

            while ((j > 0U) &&
                   (ConfigPtr->Rules[BswM_State.ruleOrder[j - 1U]].Priority >
                    ConfigPtr->Rules[moving].Priority)) {
                BswM_State.ruleOrder[j] = BswM_State.ruleOrder[j - 1U];
                j--;
            }
            BswM_State.ruleOrder[j] = moving;
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
    /* Reverse-order cleanup: walk the configured rules from last to first
     * and run the False action list of every enabled rule latched TRUE.
     * This undoes stateful side effects (e.g. PDU groups enabled by a TRUE
     * transition) in inverse configuration order before the state resets. */
    if ((BswM_State.internalState == BSWM_INTERNAL_INIT) && (BswM_State.configPtr != NULL_PTR) &&
        (BswM_State.configPtr->Rules != NULL_PTR)) {
        uint8 i;

        for (i = BswM_State.configPtr->NumRules; i > 0U; i--) {
            uint8 idx = (uint8)(i - 1U);

            if (idx >= BSWM_MAX_RULES) {
                break;
            }
            if ((BswM_State.ruleEnabled[idx] == TRUE) &&
                (BswM_State.ruleState[idx] == BSWM_RULE_STATE_TRUE)) {
                BswM_ExecuteActionList(BswM_State.configPtr->Rules[idx].FalseActionListIndex);
            }
        }
    }

    BswM_ResetInternalState(NULL_PTR);
    BswM_State.internalState = BSWM_INTERNAL_UNINIT;
}

/** @req SWS_BswM_00010 */
Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode)
{
    uint8 i;

#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }
    if (Mode > BSWM_MODE_VALUE_MAX) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_PARAM_MODE);
        return E_NOT_OK;
    }
#endif

    /* Update every port bound to this composition; unmatched compositions
     * are still latched for BswM_GetRequestedMode() compatibility. */
    if ((BswM_State.configPtr != NULL_PTR) && (BswM_State.configPtr->ModeRequestPorts != NULL_PTR)) {
        for (i = 0U; i < BswM_State.configPtr->NumModeRequestPorts; i++) {
            if ((i < BSWM_MAX_MODE_REQUEST_PORTS) &&
                (BswM_State.configPtr->ModeRequestPorts[i] == SwCompositionId)) {
                BswM_WritePort(i, (uint8)Mode);
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
    uint8 k;
    uint32 dirtyMask = 0UL;
    BswM_ModeType portSnapshot[BSWM_MAX_MODE_REQUEST_PORTS];

    if ((BswM_State.internalState == BSWM_INTERNAL_UNINIT) || (BswM_State.configPtr == NULL_PTR)) {
        return;
    }

    /* Snapshot the ports and collect the dirty mask. Rules whose condition
     * tree does not reference a dirty port skip evaluation entirely. */
    for (i = 0U; i < BSWM_MAX_MODE_REQUEST_PORTS; i++) {
        portSnapshot[i] = BswM_State.portValue[i];
        if (BswM_State.portDirty[i] == TRUE) {
            dirtyMask |= (1UL << i);
        }
    }

    /* Rule evaluation in priority order (ruleOrder: stable sort by Priority,
     * 0 = highest); action lists execute on state transitions only */
    for (k = 0U; k < BswM_State.configPtr->NumRules; k++) {
        const BswM_RuleType* rule;
        BswM_RuleStateType result;
        uint8 idx;

        if (k >= BSWM_MAX_RULES) {
            break;
        }
        idx = BswM_State.ruleOrder[k];
        if ((idx >= BswM_State.configPtr->NumRules) ||
            (BswM_State.configPtr->Rules == NULL_PTR)) {
            break;
        }
        rule = &BswM_State.configPtr->Rules[idx];
        if ((BswM_State.ruleEnabled[idx] == FALSE) ||
            ((BswM_State.rulePortMask[idx] & dirtyMask) == 0UL)) {
            continue;
        }

        result = (BswM_EvaluateExpression(rule->ConditionIndex, 0U) == TRUE)
                     ? BSWM_RULE_STATE_TRUE
                     : BSWM_RULE_STATE_FALSE;

        if (result != BswM_State.ruleState[idx]) {
            BswM_State.ruleState[idx] = result;
            BswM_ExecuteActionList((result == BSWM_RULE_STATE_TRUE)
                                       ? rule->TrueActionListIndex
                                       : rule->FalseActionListIndex);
        }
    }

    /* Clear the dirty flags of ports that were not written again during the
     * evaluation: a request landing mid-cycle keeps its dirty flag and is
     * picked up by the next cycle (no lost edge-triggered rule). */
    for (i = 0U; i < BSWM_MAX_MODE_REQUEST_PORTS; i++) {
        if (BswM_State.portValue[i] == portSnapshot[i]) {
            BswM_State.portDirty[i] = FALSE;
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
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_GETVERSIONINFO, BSWM_E_PARAM_POINTER);
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
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_SWITCH_MODE, BSWM_E_UNINIT);
        return;
    }
    if (Mode > BSWM_MODE_VALUE_MAX) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_SWITCH_MODE, BSWM_E_PARAM_MODE);
        return;
    }
#endif
    BswM_State.currentMode = Mode;
    BswM_State.requestedMode = Mode;
}

/*==================================================================================================
*                                  Runtime rule control APIs
*================================================================================================*/

Std_ReturnType BswM_RuleEnable(uint16 RuleId)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_RULE_ENABLE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if ((BswM_State.configPtr == NULL_PTR) ||
        (RuleId >= (uint16)BswM_State.configPtr->NumRules) ||
        (RuleId >= (uint16)BSWM_MAX_RULES)) {
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_RULE_ENABLE, BSWM_E_PARAM_RULE_ID);
#endif
        return E_NOT_OK;
    }

    BswM_State.ruleEnabled[RuleId] = TRUE;
    return E_OK;
}

Std_ReturnType BswM_RuleDisable(uint16 RuleId)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_RULE_DISABLE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if ((BswM_State.configPtr == NULL_PTR) ||
        (RuleId >= (uint16)BswM_State.configPtr->NumRules) ||
        (RuleId >= (uint16)BSWM_MAX_RULES)) {
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_RULE_DISABLE, BSWM_E_PARAM_RULE_ID);
#endif
        return E_NOT_OK;
    }

    BswM_State.ruleEnabled[RuleId] = FALSE;
    return E_OK;
}

Std_ReturnType BswM_Com_CurrentPduGroupState(BswM_Com_IpduGroupVector IpduGroupVector)
{
    uint8 i;
    uint8 nBytes;
    const uint8 comVectorBytes = (uint8)sizeof(Com_IpduGroupVector);

#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_COMM_CURRENT_PDU_GROUP, BSWM_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == IpduGroupVector) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_COMM_CURRENT_PDU_GROUP, BSWM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    nBytes = (BSWM_ACTION_VECTOR_MAX_BYTES < comVectorBytes)
                 ? BSWM_ACTION_VECTOR_MAX_BYTES
                 : comVectorBytes;
    for (i = 0U; i < comVectorBytes; i++) {
        IpduGroupVector[i] = (i < nBytes) ? BswM_State.ipduGroupState[i] : 0U;
    }

    return E_OK;
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

/*==================================================================================================
*                        Notification callbacks (raw module state semantics)
*================================================================================================*/

/** ComM notification: current communication mode of a network */
void BswM_ComM_CurrentMode(BswM_NetworkHandleType Network, BswM_ComMModeType CurrentMode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_COMM_CURRENT_MODE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_COMM_REQUEST, (uint8)CurrentMode);
}

/** ComM notification: current PNC mode (IsPNCActive) of a network */
void BswM_ComM_CurrentPNCMode(BswM_NetworkHandleType Network, boolean IsPNCActive)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_COMM_CURRENT_PNC_MODE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_COMM_PNC_REQUEST, (uint8)IsPNCActive);
}

/** Dcm notification: current communication mode request state of a network */
void BswM_Dcm_CommunicationMode_CurrentState(BswM_NetworkHandleType Network,
                                             BswM_DcmCommunicationModeType RequestedCommunicationMode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_DCM_COMM_MODE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_DCM_REQUEST, (uint8)RequestedCommunicationMode);
}

/** Dcm notification: application updated indication */
void BswM_Dcm_ApplicationUpdated(BswM_NetworkHandleType Network)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_DCM_APP_UPDATED, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_DCM_APPUPDATED_REQUEST, 1U);
}

/** Nm notification: network management state changed */
void BswM_Nm_StateChangeNotification(BswM_NetworkHandleType Network, BswM_NmStateType State)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_NM_STATE_CHANGE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_NM_REQUEST, (uint8)State);
}

/** Nm notification: car wakeup indication */
void BswM_Nm_CarWakeUpIndication(BswM_NetworkHandleType Network)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_NM_CAR_WAKEUP, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_NM_CARWAKEUP_REQUEST, 1U);
}

/** CanSM notification: current CAN network mode */
void BswM_CanSM_CurrentState(BswM_NetworkHandleType Network, BswM_CanSmStateType CurrentState)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_CANSM_CURRENT_STATE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_CANSM_REQUEST, (uint8)CurrentState);
}

/** EthSM notification: current Ethernet network mode */
void BswM_EthSM_CurrentState(BswM_NetworkHandleType Network, BswM_EthSmStateType CurrentState)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_ETHSM_CURRENT_STATE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_ETHSM_REQUEST, (uint8)CurrentState);
}

/** FrSM notification: current FlexRay network mode */
void BswM_FrSM_CurrentState(BswM_NetworkHandleType Network, BswM_FrSmStateType CurrentState)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_FRSM_CURRENT_STATE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_FRSM_REQUEST, (uint8)CurrentState);
}

/** LinSM notification: current LIN network mode */
void BswM_LinSM_CurrentState(BswM_NetworkHandleType Network, BswM_LinSmStateType CurrentState)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_LINSM_CURRENT_STATE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_LINSM_REQUEST, (uint8)CurrentState);
}

/** LinSM notification: current LIN schedule table */
void BswM_LinSM_CurrentSchedule(BswM_NetworkHandleType Network, uint8 ScheduleIndex)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_LINSM_CURRENT_SCHEDULE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_LINSM_SCHEDULE_REQUEST, ScheduleIndex);
}

/** LinTp notification: requested LIN TP mode */
void BswM_LinTp_RequestMode(BswM_NetworkHandleType Network, BswM_LinTpModeType LinTpMode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_LINTP_REQUEST_MODE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)Network;
    BswM_WritePortByComposition(BSWM_LINTP_REQUEST, (uint8)LinTpMode);
}

/** EcuM notification: requested ECU state, mapped to the BswM mode domain first */
void BswM_EcuM_RequestedState(BswM_EcuMStateType State)
{
    boolean isMapped = FALSE;
    BswM_ModeType mode;

#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_ECUM_REQUESTED_STATE, BSWM_E_UNINIT);
        return;
    }
#endif
    mode = BswM_MapEcuMState((EcuM_StateType)State, &isMapped);
    if (isMapped == TRUE) {
        BswM_WritePortByComposition(BSWM_ECUM_REQUESTED, (uint8)mode);
    }
    /* Unmapped states are ignored (no port write, no DET). */
}

/** BswM notification: partition restarted, writes the partition id low byte */
void BswM_BswMPartitionRestarted(BswM_ApplicationType Application)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_PARTITION_RESTARTED, BSWM_E_UNINIT);
        return;
    }
#endif
    BswM_WritePortByComposition(BSWM_PARTITION_REQUEST, (uint8)(Application & 0x00FFU));
}

/** EthIf notification: port group link state changed */
void BswM_EthIf_PortGroupLinkStateChg(uint8 PortGroup, boolean LinkState)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_State.internalState == BSWM_INTERNAL_UNINIT) {
        (void)Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_ETHIF_PORTGROUP_LINKSTATE, BSWM_E_UNINIT);
        return;
    }
#endif
    (void)PortGroup;
    BswM_WritePortByComposition(BSWM_ETHIF_REQUEST, (uint8)LinkState);
}

/**
 * @file test_bswm.c
 * @brief BswM (BSW Mode Manager) Unit Tests — Phase 3 enhancement coverage
 * @version 2.0.0
 *
 * Substantiated against the production BswM implementation
 * (src/bsw/services/bswm/src/BswM.c). Replaces the former empty-shell
 * assert_true(1) tests with behavioral coverage of:
 *  - rule TRUE->FALSE transition action lists
 *  - expression tree AND/OR/NOT evaluation
 *  - PDU group control actions + BswM_Com_CurrentPduGroupState()
 *  - priority arbitration (high priority rules fire first)
 *  - dirty-flag gating (baseline evaluation, mid-cycle writes,
 *    change-detection on repeated writes)
 *  - BswM_RuleEnable/BswM_RuleDisable runtime control
 *  - EcuM GoSleep/GoHalt/GoOff actions
 *  - BswM_DeInit reverse-order cleanup
 */

// @tests src/bsw/services/bswm/src/BswM.c  @tests src/bsw/services/bswm/include/BswM.h

#include "unity.h"
#include "BswM.h"
#include "Com.h"
#include "EcuM.h"

/*==================================================================================================
*                                  Mocks (Det + Com + EcuM)
*================================================================================================*/

static uint8  mock_DetCalls = 0U;
static uint16 mock_DetLastModuleId = 0U;
static uint8  mock_DetLastApiId = 0xFFU;
static uint8  mock_DetLastErrorId = 0xFFU;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCalls++;
    return E_OK;
}

/* Com_IpduGroupControl mock: records the last vector and Initialize flag */
static uint8   mock_ComCalls = 0U;
static boolean mock_ComLastInitialize = FALSE;
static Com_IpduGroupVector mock_ComLastVector;

void Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean Initialize)
{
    uint8 i;

    for (i = 0U; i < (uint8)sizeof(Com_IpduGroupVector); i++) {
        mock_ComLastVector[i] = IpduGroupVector[i];
    }
    mock_ComLastInitialize = Initialize;
    mock_ComCalls++;
}

/* EcuM mocks: record the shutdown coordination calls of the new actions */
static uint8 mock_EcuMGoSleepCalls = 0U;
static uint8 mock_EcuMGoHaltCalls = 0U;
static uint8 mock_EcuMShutdownCalls = 0U;
static EcuM_ShutdownTargetType mock_EcuMLastTarget = 0xFFU;
static uint8 mock_EcuMLastTargetMode = 0xFFU;

void EcuM_GoSleep(void)
{
    mock_EcuMGoSleepCalls++;
}

void EcuM_GoHalt(void)
{
    mock_EcuMGoHaltCalls++;
}

Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode)
{
    mock_EcuMLastTarget = target;
    mock_EcuMLastTargetMode = mode;
    return E_OK;
}

void EcuM_Shutdown(void)
{
    mock_EcuMShutdownCalls++;
}

/*==================================================================================================
*                                  Action recording helpers
*================================================================================================*/

/* rec_Log records which configured action ran, in call order:
 * 1=A 2=B 3=C 4=D 5=E 6=F 7=Reentrant */
static uint8 rec_Log[32];
static uint8 rec_Count = 0U;

static void rec_Reset(void)
{
    uint8 i;

    for (i = 0U; i < (uint8)sizeof(rec_Log); i++) {
        rec_Log[i] = 0U;
    }
    rec_Count = 0U;
}

static void rec_A(BswM_ModeType Mode)          { (void)Mode; rec_Log[rec_Count++] = 1U; }
static void rec_B(BswM_ModeType Mode)          { (void)Mode; rec_Log[rec_Count++] = 2U; }
static void rec_C(BswM_ModeType Mode)          { (void)Mode; rec_Log[rec_Count++] = 3U; }
static void rec_D(BswM_ModeType Mode)          { (void)Mode; rec_Log[rec_Count++] = 4U; }
static void rec_E(BswM_ModeType Mode)          { (void)Mode; rec_Log[rec_Count++] = 5U; }
static void rec_F(BswM_ModeType Mode)          { (void)Mode; rec_Log[rec_Count++] = 6U; }

/* Re-entrant action: records, then rewrites the SAME port with a new value
 * while the rule evaluation is still running (exercises the dirty-flag
 * snapshot refinement: the port must stay dirty and re-trigger the rule). */
static void rec_Reentrant(BswM_ModeType Mode)
{
    (void)Mode;
    rec_Log[rec_Count++] = 7U;
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, BSWM_MODE_VALUE_POST_RUN);
}

static void mock_ResetAll(void)
{
    mock_DetCalls = 0U;
    mock_DetLastModuleId = 0U;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_ComCalls = 0U;
    mock_ComLastInitialize = FALSE;
    (void)mock_ComLastVector;
    mock_EcuMGoSleepCalls = 0U;
    mock_EcuMGoHaltCalls = 0U;
    mock_EcuMShutdownCalls = 0U;
    mock_EcuMLastTarget = 0xFFU;
    mock_EcuMLastTargetMode = 0xFFU;
    rec_Reset();
}

void setUp(void)
{
    /* Force a known UNINIT state before every test (BswM_State is file-static).
     * DeInit runs the configured reverse-order cleanup and may fire action
     * lists of rules the previous test left latched TRUE, so the mock
     * recorders are cleared AFTERWARDS to keep every test at a zero baseline. */
    BswM_DeInit();
    mock_ResetAll();
}

void tearDown(void)
{
    /* Rebind BswM to the static default config before the test-local config
     * graph goes out of scope, so no later DeInit walks dangling fixtures. */
    BswM_Init(NULL_PTR);
}

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/

/* f1: single port (EcuM), EQ(port0, 1), TrueList=A / FalseList=B */
static BswM_ModeRequestPortType   f1_Ports[1];
static BswM_ExpressionConfigType  f1_Expr[1];
static BswM_ActionType            f1_TrueActs[1];
static BswM_ActionType            f1_FalseActs[1];
static BswM_ActionListType        f1_Lists[2];
static BswM_RuleType              f1_Rule;
static BswM_ConfigType            f1_Cfg;

static void f1_Setup(void)
{
    f1_Ports[0] = BSWM_ECUM_REQUEST;                 /* port 0 <-> composition 1 */

    f1_Expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    f1_Expr[0].PortIndex      = 0U;
    f1_Expr[0].CompareValue   = 1U;
    f1_Expr[0].LeftIndex      = BSWM_EXPR_IDX_NONE;
    f1_Expr[0].RightIndex     = BSWM_EXPR_IDX_NONE;

    f1_TrueActs[0].Callback  = rec_A;
    f1_TrueActs[0].Parameter = 0U;
    f1_TrueActs[0].Kind      = BSWM_ACTION_CALLBACK;
    f1_FalseActs[0].Callback  = rec_B;
    f1_FalseActs[0].Parameter = 0U;
    f1_FalseActs[0].Kind      = BSWM_ACTION_CALLBACK;

    f1_Lists[0].NumActions = 1U;
    f1_Lists[0].Actions    = f1_TrueActs;
    f1_Lists[1].NumActions = 1U;
    f1_Lists[1].Actions    = f1_FalseActs;

    f1_Rule.RuleId             = 0U;
    f1_Rule.ConditionIndex     = 0U;
    f1_Rule.TrueActionListIndex  = 0U;
    f1_Rule.FalseActionListIndex = 1U;
    f1_Rule.InitialState       = BSWM_RULE_STATE_FALSE;
    f1_Rule.IsEnabled          = TRUE;
    f1_Rule.Priority           = 0U;

    f1_Cfg.NumModeRequestPorts = 1U;
    f1_Cfg.ModeRequestPorts    = f1_Ports;
    f1_Cfg.NumExpressions      = 1U;
    f1_Cfg.Expressions         = f1_Expr;
    f1_Cfg.NumRules            = 1U;
    f1_Cfg.Rules               = &f1_Rule;
    f1_Cfg.NumActionLists      = 2U;
    f1_Cfg.ActionLists         = f1_Lists;
}

/* f2: three ports, expression tree AND(OR base) / OR / NOT, three rules */
static BswM_ModeRequestPortType   f2_Ports[3];
static BswM_ExpressionConfigType  f2_Expr[6];
static BswM_ActionType            f2_Acts[6];            /* A..F */
static BswM_ActionListType        f2_Lists[6];
static BswM_RuleType              f2_Rules[3];
static BswM_ConfigType            f2_Cfg;

static void f2_Setup(void)
{
    uint8 i;

    f2_Ports[0] = BSWM_ECUM_REQUEST;   /* port 0 */
    f2_Ports[1] = BSWM_COMM_REQUEST;   /* port 1 */
    f2_Ports[2] = BSWM_NM_REQUEST;     /* port 2 */

    /* 0: EQ(port0, 1) */
    f2_Expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    f2_Expr[0].PortIndex      = 0U;
    f2_Expr[0].CompareValue   = 1U;
    /* 1: EQ(port1, 2) */
    f2_Expr[1].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    f2_Expr[1].PortIndex      = 1U;
    f2_Expr[1].CompareValue   = 2U;
    /* 2: AND(expr0, expr1) */
    f2_Expr[2].ExpressionType = BSWM_EXPR_LOGICAL_AND;
    f2_Expr[2].LeftIndex      = 0U;
    f2_Expr[2].RightIndex     = 1U;
    /* 3: EQ(port2, 3) */
    f2_Expr[3].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    f2_Expr[3].PortIndex      = 2U;
    f2_Expr[3].CompareValue   = 3U;
    /* 4: OR(expr2, expr3) */
    f2_Expr[4].ExpressionType = BSWM_EXPR_LOGICAL_OR;
    f2_Expr[4].LeftIndex      = 2U;
    f2_Expr[4].RightIndex     = 3U;
    /* 5: NOT(expr0) */
    f2_Expr[5].ExpressionType = BSWM_EXPR_LOGICAL_NOT;
    f2_Expr[5].LeftIndex      = 0U;

    for (i = 0U; i < 6U; i++) {
        f2_Expr[i].RightIndex = (i == 5U) ? BSWM_EXPR_IDX_NONE : f2_Expr[i].RightIndex;
        if ((f2_Expr[i].ExpressionType == BSWM_EXPR_MODE_EQUALS) ||
            (f2_Expr[i].ExpressionType == BSWM_EXPR_MODE_NOT_EQUALS)) {
            f2_Expr[i].LeftIndex  = BSWM_EXPR_IDX_NONE;
            f2_Expr[i].RightIndex = BSWM_EXPR_IDX_NONE;
        }
    }

    /* Leaf defaults for logical nodes */
    f2_Expr[2].PortIndex = 0U;
    f2_Expr[2].CompareValue = 0U;
    f2_Expr[4].PortIndex = 0U;
    f2_Expr[4].CompareValue = 0U;
    f2_Expr[5].PortIndex = 0U;
    f2_Expr[5].CompareValue = 0U;
    f2_Expr[5].RightIndex = BSWM_EXPR_IDX_NONE;

    f2_Acts[0].Callback = rec_A;  f2_Acts[0].Parameter = 0U; f2_Acts[0].Kind = BSWM_ACTION_CALLBACK;
    f2_Acts[1].Callback = rec_B;  f2_Acts[1].Parameter = 0U; f2_Acts[1].Kind = BSWM_ACTION_CALLBACK;
    f2_Acts[2].Callback = rec_C;  f2_Acts[2].Parameter = 0U; f2_Acts[2].Kind = BSWM_ACTION_CALLBACK;
    f2_Acts[3].Callback = rec_D;  f2_Acts[3].Parameter = 0U; f2_Acts[3].Kind = BSWM_ACTION_CALLBACK;
    f2_Acts[4].Callback = rec_E;  f2_Acts[4].Parameter = 0U; f2_Acts[4].Kind = BSWM_ACTION_CALLBACK;
    f2_Acts[5].Callback = rec_F;  f2_Acts[5].Parameter = 0U; f2_Acts[5].Kind = BSWM_ACTION_CALLBACK;

    for (i = 0U; i < 6U; i++) {
        f2_Lists[i].NumActions = 1U;
        f2_Lists[i].Actions    = &f2_Acts[i];
    }

    /* R0: AND -> A / B;  R1: OR -> C / D;  R2: NOT -> E / F */
    f2_Rules[0].RuleId = 0U;  f2_Rules[0].ConditionIndex = 2U;
    f2_Rules[0].TrueActionListIndex = 0U;  f2_Rules[0].FalseActionListIndex = 1U;
    f2_Rules[0].InitialState = BSWM_RULE_STATE_FALSE; f2_Rules[0].IsEnabled = TRUE;
    f2_Rules[0].Priority = 0U;

    f2_Rules[1].RuleId = 1U;  f2_Rules[1].ConditionIndex = 4U;
    f2_Rules[1].TrueActionListIndex = 2U;  f2_Rules[1].FalseActionListIndex = 3U;
    f2_Rules[1].InitialState = BSWM_RULE_STATE_FALSE; f2_Rules[1].IsEnabled = TRUE;
    f2_Rules[1].Priority = 0U;

    f2_Rules[2].RuleId = 2U;  f2_Rules[2].ConditionIndex = 5U;
    f2_Rules[2].TrueActionListIndex = 4U;  f2_Rules[2].FalseActionListIndex = 5U;
    f2_Rules[2].InitialState = BSWM_RULE_STATE_FALSE; f2_Rules[2].IsEnabled = TRUE;
    f2_Rules[2].Priority = 0U;

    f2_Cfg.NumModeRequestPorts = 3U;
    f2_Cfg.ModeRequestPorts    = f2_Ports;
    f2_Cfg.NumExpressions      = 6U;
    f2_Cfg.Expressions         = f2_Expr;
    f2_Cfg.NumRules            = 3U;
    f2_Cfg.Rules               = f2_Rules;
    f2_Cfg.NumActionLists      = 6U;
    f2_Cfg.ActionLists         = f2_Lists;
}

/*==================================================================================================
*                       Rule state transitions and expression evaluation
*================================================================================================*/

void test_BswM_Rule_TrueToFalse_TransitionFiresActionLists(void)
{
    f1_Setup();
    BswM_Init(&f1_Cfg);

    /* Baseline cycle (all ports dirty at init): condition FALSE, state stays
     * FALSE, no action list runs. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, rec_Count);

    /* FALSE -> TRUE runs the True action list */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_ECUM_REQUEST, 1U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Log[0]);

    /* TRUE -> FALSE runs the False action list */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_ECUM_REQUEST, 2U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[1]);

    /* No further writes: no further actions */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
}

void test_BswM_ExpressionTree_AndOrNot_Combinations(void)
{
    f2_Setup();
    BswM_Init(&f2_Cfg);

    /* Baseline: AND=F, OR=F, NOT(EQ(port0,1)=F)=T -> R2 fires E */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(5U, rec_Log[0]);

    /* port0=1: NOT flips T->F (F); AND still F (port1 != 2) */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_ECUM_REQUEST, 1U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(6U, rec_Log[1]);

    /* port1=2: AND T (A), OR becomes T (C) — config order R0 before R1 */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_COMM_REQUEST, 2U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(4U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Log[2]);
    TEST_ASSERT_EQUAL_UINT8(3U, rec_Log[3]);

    /* port2=3: OR(T,T) stays TRUE — no transition */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_NM_REQUEST, 3U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(4U, rec_Count);

    /* port1=0: AND T->F (B); OR(AND F, EQ(port2,3)=T) stays TRUE */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_COMM_REQUEST, 0U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(5U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[4]);

    /* port2=0: OR(F,F)=F — R1 fires D */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_NM_REQUEST, 0U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(6U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(4U, rec_Log[5]);
}

void test_BswM_ExpressionTree_OrFalseTransition_FiresFalseList(void)
{
    f2_Setup();
    BswM_Init(&f2_Cfg);

    /* First cycle with both AND leaves satisfied: baseline already sees
     * port0=1 (NOT false, no E), AND fires A, OR fires C. */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    (void)BswM_RequestMode(BSWM_COMM_REQUEST, 2U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Log[0]);         /* A (AND true) */
    TEST_ASSERT_EQUAL_UINT8(3U, rec_Log[1]);         /* C (OR true)  */

    /* Clear the AND branch while port2 stays unequal: AND T->F (B) and
     * OR T->F (D) in the same cycle, config order R0 before R1. */
    (void)BswM_RequestMode(BSWM_COMM_REQUEST, 0U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(4U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[2]);         /* B (AND false) */
    TEST_ASSERT_EQUAL_UINT8(4U, rec_Log[3]);         /* D (OR false)  */
}

/*==================================================================================================
*                                  Priority arbitration
*================================================================================================*/

void test_BswM_PriorityArbitration_HighPriorityRuleFiresFirst(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType acts[2];
    BswM_ActionListType lists[2];
    BswM_RuleType rules[2];
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;

    acts[0].Callback = rec_A; acts[0].Parameter = 0U; acts[0].Kind = BSWM_ACTION_CALLBACK;
    acts[1].Callback = rec_B; acts[1].Parameter = 0U; acts[1].Kind = BSWM_ACTION_CALLBACK;
    lists[0].NumActions = 1U; lists[0].Actions = &acts[0];
    lists[1].NumActions = 1U; lists[1].Actions = &acts[1];

    /* rule0 = lower priority (1), rule1 = higher priority (0) */
    rules[0].RuleId = 0U; rules[0].ConditionIndex = 0U;
    rules[0].TrueActionListIndex = 0U; rules[0].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules[0].InitialState = BSWM_RULE_STATE_FALSE; rules[0].IsEnabled = TRUE;
    rules[0].Priority = 1U;
    rules[1].RuleId = 1U; rules[1].ConditionIndex = 0U;
    rules[1].TrueActionListIndex = 1U; rules[1].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules[1].InitialState = BSWM_RULE_STATE_FALSE; rules[1].IsEnabled = TRUE;
    rules[1].Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 2U;            cfg.Rules = rules;
    cfg.NumActionLists = 2U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();

    /* Both fire in the same cycle — the Priority 0 rule (B) must run first */
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[0]);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Log[1]);
}

void test_BswM_PriorityArbitration_EqualPriorityKeepsConfigOrder(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType acts[2];
    BswM_ActionListType lists[2];
    BswM_RuleType rules[2];
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;

    acts[0].Callback = rec_A; acts[0].Parameter = 0U; acts[0].Kind = BSWM_ACTION_CALLBACK;
    acts[1].Callback = rec_B; acts[1].Parameter = 0U; acts[1].Kind = BSWM_ACTION_CALLBACK;
    lists[0].NumActions = 1U; lists[0].Actions = &acts[0];
    lists[1].NumActions = 1U; lists[1].Actions = &acts[1];

    rules[0].RuleId = 0U; rules[0].ConditionIndex = 0U;
    rules[0].TrueActionListIndex = 0U; rules[0].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules[0].InitialState = BSWM_RULE_STATE_FALSE; rules[0].IsEnabled = TRUE;
    rules[0].Priority = 0U;
    rules[1].RuleId = 1U; rules[1].ConditionIndex = 0U;
    rules[1].TrueActionListIndex = 1U; rules[1].FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rules[1].InitialState = BSWM_RULE_STATE_FALSE; rules[1].IsEnabled = TRUE;
    rules[1].Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 2U;            cfg.Rules = rules;
    cfg.NumActionLists = 2U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();

    /* Equal priorities: stable sort keeps the configuration order A, B */
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Log[0]);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[1]);
}

/*==================================================================================================
*                                  Dirty-flag gating
*================================================================================================*/

void test_BswM_DirtyFlag_BaselineEvaluation_EvaluatesEveryRuleOnce(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType falseActs[1];
    BswM_ActionListType lists[1];
    BswM_RuleType rule;
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;
    falseActs[0].Callback = rec_B; falseActs[0].Parameter = 0U;
    falseActs[0].Kind = BSWM_ACTION_CALLBACK;
    lists[0].NumActions = 1U; lists[0].Actions = falseActs;

    rule.RuleId = 0U; rule.ConditionIndex = 0U;
    rule.TrueActionListIndex = BSWM_ACTION_LIST_NONE;
    rule.FalseActionListIndex = 0U;
    rule.InitialState = BSWM_RULE_STATE_TRUE; rule.IsEnabled = TRUE;
    rule.Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 1U;            cfg.Rules = &rule;
    cfg.NumActionLists = 1U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    /* No request at all: the init-time all-dirty baseline must still evaluate
     * the rule once, reconciling InitialState=TRUE with condition FALSE. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[0]);

    /* Steady state: no further writes, no further evaluation effects */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);
}

void test_BswM_DirtyFlag_MidCycleWrite_KeepsPortDirtyAndRetriggers(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType trueActs[1];
    BswM_ActionType falseActs[1];
    BswM_ActionListType lists[2];
    BswM_RuleType rule;
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;

    trueActs[0].Callback = rec_Reentrant; trueActs[0].Parameter = 0U;
    trueActs[0].Kind = BSWM_ACTION_CALLBACK;
    falseActs[0].Callback = rec_B; falseActs[0].Parameter = 0U;
    falseActs[0].Kind = BSWM_ACTION_CALLBACK;
    lists[0].NumActions = 1U; lists[0].Actions = trueActs;
    lists[1].NumActions = 1U; lists[1].Actions = falseActs;

    rule.RuleId = 0U; rule.ConditionIndex = 0U;
    rule.TrueActionListIndex = 0U; rule.FalseActionListIndex = 1U;
    rule.InitialState = BSWM_RULE_STATE_FALSE; rule.IsEnabled = TRUE;
    rule.Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 1U;            cfg.Rules = &rule;
    cfg.NumActionLists = 2U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);

    /* Cycle 1: rule goes TRUE, its action rewrites port0 to POST_RUN (2)
     * DURING evaluation. The snapshot-based dirty clear must keep port0
     * dirty so the new value is not lost. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(7U, rec_Log[0]);

    /* Cycle 2: the kept-dirty port re-triggers the rule -> FALSE (B). An
     * unconditional dirty clear would have swallowed the write and left
     * the rule latched TRUE forever. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[1]);

    /* Cycle 3: clean port, nothing happens */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
}

void test_BswM_DirtyFlag_SameValueRewrite_DoesNotRetrigger(void)
{
    f1_Setup();
    BswM_Init(&f1_Cfg);

    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);

    /* Repeated write of the SAME value: no value change -> port stays clean
     * -> rule not re-evaluated (and transitions would not re-fire anyway). */
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(BSWM_ECUM_REQUEST, 1U));
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);
}

/*==================================================================================================
*                                  PDU group control actions
*================================================================================================*/

void test_BswM_PduGroupSwitch_EnableThenDisable_UpdatesShadowAndCom(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType trueActs[1];
    BswM_ActionType falseActs[1];
    BswM_ActionListType lists[2];
    BswM_RuleType rule;
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;
    Com_IpduGroupVector queried;
    Std_ReturnType ret;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;

    /* True: enable groups 0 and 2 (0x05); False: disable group 0 (0x01) */
    trueActs[0].Callback = NULL_PTR; trueActs[0].Parameter = 0U;
    trueActs[0].Kind = BSWM_ACTION_PDU_GROUP_SWITCH;
    trueActs[0].Enable = TRUE;
    trueActs[0].Initialize = FALSE;
    trueActs[0].IpduGroupVector[0] = 0x05U;
    trueActs[0].IpduGroupVector[1] = 0x00U;
    trueActs[0].IpduGroupVector[2] = 0x00U;
    trueActs[0].IpduGroupVector[3] = 0x00U;

    falseActs[0].Callback = NULL_PTR; falseActs[0].Parameter = 0U;
    falseActs[0].Kind = BSWM_ACTION_PDU_GROUP_SWITCH;
    falseActs[0].Enable = FALSE;
    falseActs[0].Initialize = FALSE;
    falseActs[0].IpduGroupVector[0] = 0x01U;
    falseActs[0].IpduGroupVector[1] = 0x00U;
    falseActs[0].IpduGroupVector[2] = 0x00U;
    falseActs[0].IpduGroupVector[3] = 0x00U;

    lists[0].NumActions = 1U; lists[0].Actions = trueActs;
    lists[1].NumActions = 1U; lists[1].Actions = falseActs;

    rule.RuleId = 0U; rule.ConditionIndex = 0U;
    rule.TrueActionListIndex = 0U; rule.FalseActionListIndex = 1U;
    rule.InitialState = BSWM_RULE_STATE_FALSE; rule.IsEnabled = TRUE;
    rule.Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 1U;            cfg.Rules = &rule;
    cfg.NumActionLists = 2U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();

    /* Enable path: full shadow vector handed to Com */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ComCalls);
    TEST_ASSERT_EQUAL_UINT8(0x05U, mock_ComLastVector[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, mock_ComLastVector[1]);
    TEST_ASSERT_EQUAL(FALSE, mock_ComLastInitialize);

    /* Query returns the BswM shadow state */
    ret = BswM_Com_CurrentPduGroupState(queried);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0x05U, queried[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, queried[1]);

    /* Disable group 0 only: shadow merges down to 0x04 */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 2U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, mock_ComCalls);
    TEST_ASSERT_EQUAL_UINT8(0x04U, mock_ComLastVector[0]);

    ret = BswM_Com_CurrentPduGroupState(queried);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0x04U, queried[0]);
}

void test_BswM_ComIpduGroup_AbsoluteControl_PassesVectorAndInitialize(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType trueActs[1];
    BswM_ActionListType lists[1];
    BswM_RuleType rule;
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;
    Com_IpduGroupVector queried;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;

    /* Absolute Com IPDU group control: vector IS the new state */
    trueActs[0].Callback = NULL_PTR; trueActs[0].Parameter = 0U;
    trueActs[0].Kind = BSWM_ACTION_COM_IPDU_GROUP;
    trueActs[0].Enable = TRUE;
    trueActs[0].Initialize = TRUE;
    trueActs[0].IpduGroupVector[0] = 0x0FU;
    trueActs[0].IpduGroupVector[1] = 0x00U;
    trueActs[0].IpduGroupVector[2] = 0x00U;
    trueActs[0].IpduGroupVector[3] = 0x00U;

    lists[0].NumActions = 1U; lists[0].Actions = trueActs;

    rule.RuleId = 0U; rule.ConditionIndex = 0U;
    rule.TrueActionListIndex = 0U; rule.FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rule.InitialState = BSWM_RULE_STATE_FALSE; rule.IsEnabled = TRUE;
    rule.Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 1U;            cfg.Rules = &rule;
    cfg.NumActionLists = 1U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(1U, mock_ComCalls);
    TEST_ASSERT_EQUAL_UINT8(0x0FU, mock_ComLastVector[0]);
    TEST_ASSERT_EQUAL(TRUE, mock_ComLastInitialize);

    (void)BswM_Com_CurrentPduGroupState(queried);
    TEST_ASSERT_EQUAL_UINT8(0x0FU, queried[0]);
}

void test_BswM_Com_CurrentPduGroupState_Uninit_ShouldReportDet(void)
{
    Com_IpduGroupVector queried;

    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_Com_CurrentPduGroupState(queried));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_COMM_CURRENT_PDU_GROUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_Com_CurrentPduGroupState_NullPtr_ShouldReportDet(void)
{
    f1_Setup();
    BswM_Init(&f1_Cfg);

    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_Com_CurrentPduGroupState(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_COMM_CURRENT_PDU_GROUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/*==================================================================================================
*                                  Runtime rule enable/disable
*================================================================================================*/

void test_BswM_RuleDisable_SuppressesEvaluation_UntilReEnabled(void)
{
    f1_Setup();
    BswM_Init(&f1_Cfg);

    /* Rule fires TRUE once */
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);

    /* Disabled rule must not react to the port change ... */
    TEST_ASSERT_EQUAL(E_OK, BswM_RuleDisable(0U));
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 2U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);

    /* ... but catches up (stale TRUE vs condition FALSE) once re-enabled and
     * the port changes again. */
    TEST_ASSERT_EQUAL(E_OK, BswM_RuleEnable(0U));
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 3U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[1]);         /* False list ran */
}

void test_BswM_RuleEnable_Uninit_ShouldReportDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RuleEnable(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_ENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_RuleDisable_Uninit_ShouldReportDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RuleDisable(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_DISABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

void test_BswM_RuleEnable_InvalidRuleId_ShouldReportDet(void)
{
    f1_Setup();
    BswM_Init(&f1_Cfg);

    /* Out of range (NumRules == 1) */
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RuleEnable(1U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_ENABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_RULE_ID, mock_DetLastErrorId);

    /* Far out of range */
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RuleDisable(0xFFFFU));
    TEST_ASSERT_EQUAL_UINT8(2U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_RULE_DISABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_RULE_ID, mock_DetLastErrorId);

    /* Valid id succeeds */
    TEST_ASSERT_EQUAL(E_OK, BswM_RuleEnable(0U));
    TEST_ASSERT_EQUAL_UINT8(2U, mock_DetCalls);
}

/*==================================================================================================
*                                  EcuM shutdown coordination actions
*================================================================================================*/

void test_BswM_EcuMActions_GoSleepGoHaltGoOff_AreInvoked(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[1];
    BswM_ActionType trueActs[3];
    BswM_ActionListType lists[1];
    BswM_RuleType rule;
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;

    ports[0] = BSWM_ECUM_REQUEST;
    expr[0].ExpressionType = BSWM_EXPR_MODE_EQUALS;
    expr[0].PortIndex = 0U;
    expr[0].CompareValue = 1U;
    expr[0].LeftIndex = BSWM_EXPR_IDX_NONE;
    expr[0].RightIndex = BSWM_EXPR_IDX_NONE;

    trueActs[0].Callback = NULL_PTR; trueActs[0].Parameter = 0U;
    trueActs[0].Kind = BSWM_ACTION_ECUM_GO_SLEEP;
    trueActs[1].Callback = NULL_PTR; trueActs[1].Parameter = 0U;
    trueActs[1].Kind = BSWM_ACTION_ECUM_GO_HALT;
    /* GO_OFF carries the shutdown mode in Parameter */
    trueActs[2].Callback = NULL_PTR; trueActs[2].Parameter = 7U;
    trueActs[2].Kind = BSWM_ACTION_ECUM_GO_OFF;
    lists[0].NumActions = 3U; lists[0].Actions = trueActs;

    rule.RuleId = 0U; rule.ConditionIndex = 0U;
    rule.TrueActionListIndex = 0U; rule.FalseActionListIndex = BSWM_ACTION_LIST_NONE;
    rule.InitialState = BSWM_RULE_STATE_FALSE; rule.IsEnabled = TRUE;
    rule.Priority = 0U;

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 1U;      cfg.Expressions = expr;
    cfg.NumRules = 1U;            cfg.Rules = &rule;
    cfg.NumActionLists = 1U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 1U);
    BswM_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(1U, mock_EcuMGoSleepCalls);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_EcuMGoHaltCalls);
    /* GO_OFF: select OFF target with the configured mode, then shutdown */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_EcuMShutdownCalls);
    TEST_ASSERT_EQUAL_UINT8(ECUM_SHUTDOWN_TARGET_OFF, mock_EcuMLastTarget);
    TEST_ASSERT_EQUAL_UINT8(7U, mock_EcuMLastTargetMode);
}

void test_BswM_DefaultConfig_ShutdownRule_DoesNotInvokeEcuMActions(void)
{
    /* The default Lcfg shutdown list only switches the BswM mode: no EcuM
     * coordination callouts are configured by default. */
    BswM_Init(NULL_PTR);
    BswM_EcuM_CurrentState(ECUM_STATE_SHUTDOWN);
    BswM_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SHUTDOWN, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EcuMGoSleepCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EcuMGoHaltCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EcuMShutdownCalls);
}

/*==================================================================================================
*                                  DeInit reverse-order cleanup
*================================================================================================*/

void test_BswM_DeInit_LatchedTrueRule_RunsFalseListInReverseOrder(void)
{
    BswM_ModeRequestPortType ports[1];
    BswM_ExpressionConfigType expr[2];
    BswM_ActionType trueActs[1];
    BswM_ActionType falseActs[1];
    BswM_ActionListType lists[2];
    BswM_RuleType rules[2];
    /* Static storage: BswM keeps the config pointer beyond the test scope */
    static BswM_ConfigType cfg;
    uint8 i;

    ports[0] = BSWM_ECUM_REQUEST;
    for (i = 0U; i < 2U; i++) {
        expr[i].ExpressionType = BSWM_EXPR_MODE_EQUALS;
        expr[i].PortIndex = 0U;
        expr[i].LeftIndex = BSWM_EXPR_IDX_NONE;
        expr[i].RightIndex = BSWM_EXPR_IDX_NONE;
    }
    expr[0].CompareValue = 1U;   /* rule0 condition */
    expr[1].CompareValue = 2U;   /* rule1 condition */

    trueActs[0].Callback = rec_A; trueActs[0].Parameter = 0U;
    trueActs[0].Kind = BSWM_ACTION_CALLBACK;
    falseActs[0].Callback = rec_B; falseActs[0].Parameter = 0U;
    falseActs[0].Kind = BSWM_ACTION_CALLBACK;
    lists[0].NumActions = 1U; lists[0].Actions = trueActs;
    lists[1].NumActions = 1U; lists[1].Actions = falseActs;

    /* Two rules sharing the same action lists; both will be TRUE. */
    for (i = 0U; i < 2U; i++) {
        rules[i].RuleId = i;
        rules[i].ConditionIndex = i;
        rules[i].TrueActionListIndex = 0U;
        rules[i].FalseActionListIndex = 1U;
        rules[i].InitialState = BSWM_RULE_STATE_FALSE;
        rules[i].IsEnabled = TRUE;
        rules[i].Priority = 0U;
    }

    cfg.NumModeRequestPorts = 1U; cfg.ModeRequestPorts = ports;
    cfg.NumExpressions = 2U;      cfg.Expressions = expr;
    cfg.NumRules = 2U;            cfg.Rules = rules;
    cfg.NumActionLists = 2U;      cfg.ActionLists = lists;

    BswM_Init(&cfg);
    (void)BswM_RequestMode(BSWM_ECUM_REQUEST, 2U);   /* rule0 F, rule1 TRUE */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Count);          /* A fired once (rule1) */

    /* DeInit runs the False list of every TRUE-latched rule, walking the
     * configuration from last to first: rule1 first, rule0 skipped (FALSE). */
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Count);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_Log[0]);         /* A (rule1, TRUE->cleanup) */
    TEST_ASSERT_EQUAL_UINT8(2U, rec_Log[1]);         /* B (rule1 false list) */

    /* Module is uninitialized afterwards */
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RequestMode(BSWM_ECUM_REQUEST, 1U));
}

void test_BswM_DeInit_Uninit_ShouldNotCrash(void)
{
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, rec_Count);
}

/*==================================================================================================
*                                  Default configuration regression
*================================================================================================*/

void test_BswM_DefaultConfig_Priorities_ShutdownHighest(void)
{
    /* Default Lcfg: Shutdown rule (Priority 0) must precede RunEntry (1)
     * and SleepEntry (2) in the arbitration order. */
    TEST_ASSERT_EQUAL_UINT8(3U, BswM_Config.NumRules);
    TEST_ASSERT_EQUAL_UINT8(0U, BswM_Config.Rules[2].Priority);
    TEST_ASSERT_EQUAL_UINT8(1U, BswM_Config.Rules[0].Priority);
    TEST_ASSERT_EQUAL_UINT8(2U, BswM_Config.Rules[1].Priority);
}

void test_BswM_DefaultConfig_SleepWakePath_StillWorks(void)
{
    BswM_Init(NULL_PTR);

    BswM_Nm_StateChangeNotification(0U, BSWM_MODE_VALUE_SLEEP);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetCurrentMode());

    BswM_Nm_StateChangeNotification(0U, 3U);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

int main(void)
{
    UNITY_BEGIN();

    /* Transitions and expression trees */
    RUN_TEST(test_BswM_Rule_TrueToFalse_TransitionFiresActionLists);
    RUN_TEST(test_BswM_ExpressionTree_AndOrNot_Combinations);
    RUN_TEST(test_BswM_ExpressionTree_OrFalseTransition_FiresFalseList);

    /* Priority arbitration */
    RUN_TEST(test_BswM_PriorityArbitration_HighPriorityRuleFiresFirst);
    RUN_TEST(test_BswM_PriorityArbitration_EqualPriorityKeepsConfigOrder);

    /* Dirty-flag gating */
    RUN_TEST(test_BswM_DirtyFlag_BaselineEvaluation_EvaluatesEveryRuleOnce);
    RUN_TEST(test_BswM_DirtyFlag_MidCycleWrite_KeepsPortDirtyAndRetriggers);
    RUN_TEST(test_BswM_DirtyFlag_SameValueRewrite_DoesNotRetrigger);

    /* PDU group control */
    RUN_TEST(test_BswM_PduGroupSwitch_EnableThenDisable_UpdatesShadowAndCom);
    RUN_TEST(test_BswM_ComIpduGroup_AbsoluteControl_PassesVectorAndInitialize);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_Com_CurrentPduGroupState_NullPtr_ShouldReportDet);

    /* Runtime rule enable/disable */
    RUN_TEST(test_BswM_RuleDisable_SuppressesEvaluation_UntilReEnabled);
    RUN_TEST(test_BswM_RuleEnable_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_RuleDisable_Uninit_ShouldReportDet);
    RUN_TEST(test_BswM_RuleEnable_InvalidRuleId_ShouldReportDet);

    /* EcuM shutdown coordination */
    RUN_TEST(test_BswM_EcuMActions_GoSleepGoHaltGoOff_AreInvoked);
    RUN_TEST(test_BswM_DefaultConfig_ShutdownRule_DoesNotInvokeEcuMActions);

    /* DeInit cleanup */
    RUN_TEST(test_BswM_DeInit_LatchedTrueRule_RunsFalseListInReverseOrder);
    RUN_TEST(test_BswM_DeInit_Uninit_ShouldNotCrash);

    /* Default config regression */
    RUN_TEST(test_BswM_DefaultConfig_Priorities_ShutdownHighest);
    RUN_TEST(test_BswM_DefaultConfig_SleepWakePath_StillWorks);

    return UnityEnd();
}

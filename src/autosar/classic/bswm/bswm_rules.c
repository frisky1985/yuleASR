/******************************************************************************
 * @file    bswm_rules.c
 * @brief   BSW Mode Manager (BswM) Rules Engine Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/classic/bswm/bswm.h"
#include <string.h>

/******************************************************************************
 * Module Internal Constants
 ******************************************************************************/
#define BSWM_MAX_NESTED_EXPRESSIONS     8U

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static BswM_RuleStateType BswM_EvaluateExpression(
    const BswM_ExpressionStructType *expression
);
static BswM_RuleStateType BswM_EvaluateBinaryExpression(
    const BswM_ExpressionStructType *expression
);
static BswM_RuleStateType BswM_EvaluateUnaryExpression(
    const BswM_ExpressionStructType *expression
);
static BswM_RuleStateType BswM_EvaluateModeCondition(
    const BswM_ModeConditionType *condition
);
static BswM_RuleStateType BswM_BoolToRuleState(boolean value);

/******************************************************************************
 * External API Implementations
 ******************************************************************************/

BswM_RuleStateType BswM_EvaluateRule(BswM_RuleIdType ruleId)
{
    BswM_RuleStateType newState = BSWM_RULE_UNDEF;
    BswM_RuleStateType oldState = BSWM_RULE_UNDEF;
    const BswM_StatusType *status = BswM_GetStatus();
    uint16 i;
    const BswM_RuleType *rule = NULL;
    
    if ((!status->initialized) || (BswM_ConfigPtr == NULL)) {
        return BSWM_RULE_UNDEF;
    }
    
    /* Find the rule */
    if (BswM_ConfigPtr->rules != NULL) {
        for (i = 0U; i < BswM_ConfigPtr->numRules; i++) {
            if (BswM_ConfigPtr->rules[i].id == ruleId) {
                rule = &BswM_ConfigPtr->rules[i];
                break;
            }
        }
    }
    
    if (rule == NULL) {
        return BSWM_RULE_UNDEF;
    }
    
    /* Get old state */
    if (ruleId < BSWM_MAX_RULES) {
        oldState = status->ruleStates[ruleId];
    }
    
    /* Evaluate the rule expression */
    if (rule->expression != NULL) {
        newState = BswM_EvaluateExpression(rule->expression);
    }
    
    /* Execute action list if state changed */
    if (newState != oldState) {
        /* Update rule state */
        if (ruleId < BSWM_MAX_RULES) {
            BswM_RuleStates[ruleId] = newState;
        }
        
        /* Execute appropriate action list */
        BswM_ExecuteRuleActionList(ruleId, newState);
        
        /* Notify rule state change */
        BswM_RuleNotification(ruleId, newState);
    }
    
    return newState;
}

void BswM_ExecuteRuleActionList(
    BswM_RuleIdType ruleId,
    BswM_RuleStateType state)
{
    const BswM_StatusType *status = BswM_GetStatus();
    uint16 i;
    const BswM_RuleType *rule = NULL;
    BswM_ActionListIdType actionListId = BSWM_ACTION_LIST_ID_INVALID;
    
    if ((!status->initialized) || (BswM_ConfigPtr == NULL)) {
        return;
    }
    
    /* Find the rule */
    if (BswM_ConfigPtr->rules != NULL) {
        for (i = 0U; i < BswM_ConfigPtr->numRules; i++) {
            if (BswM_ConfigPtr->rules[i].id == ruleId) {
                rule = &BswM_ConfigPtr->rules[i];
                break;
            }
        }
    }
    
    if (rule == NULL) {
        return;
    }
    
    /* Select action list based on state */
    if (state == BSWM_RULE_TRUE) {
        actionListId = rule->trueActionList;
    } else if (state == BSWM_RULE_FALSE) {
        actionListId = rule->falseActionList;
    }
    
    /* Execute action list */
    if (actionListId != BSWM_ACTION_LIST_ID_INVALID) {
        (void)BswM_ExecuteActionList(actionListId);
    }
}

/******************************************************************************
 * Internal Function Implementations
 ******************************************************************************/

/**
 * @brief Evaluate a rule expression
 */
static BswM_RuleStateType BswM_EvaluateExpression(
    const BswM_ExpressionStructType *expression)
{
    BswM_RuleStateType result = BSWM_RULE_UNDEF;
    
    if (expression == NULL) {
        return BSWM_RULE_UNDEF;
    }
    
    switch (expression->type) {
        case BSWM_EXPR_TRUE:
            result = BSWM_RULE_TRUE;
            break;
            
        case BSWM_EXPR_FALSE:
            result = BSWM_RULE_FALSE;
            break;
            
        case BSWM_EXPR_AND:
        case BSWM_EXPR_OR:
            result = BswM_EvaluateBinaryExpression(expression);
            break;
            
        case BSWM_EXPR_NOT:
            result = BswM_EvaluateUnaryExpression(expression);
            break;
            
        case BSWM_EXPR_MODE_REQUEST:
        case BSWM_EXPR_MODE_INDICATION:
            if (expression->data.mode.condition != NULL) {
                result = BswM_EvaluateModeCondition(expression->data.mode.condition);
            }
            break;
            
        case BSWM_EXPR_EVENT:
            /* Event expressions are evaluated based on event state */
            /* For now, return FALSE (event not active) */
            result = BSWM_RULE_FALSE;
            break;
            
        default:
            result = BSWM_RULE_UNDEF;
            break;
    }
    
    return result;
}

/**
 * @brief Evaluate a binary expression (AND/OR)
 */
static BswM_RuleStateType BswM_EvaluateBinaryExpression(
    const BswM_ExpressionStructType *expression)
{
    BswM_RuleStateType leftResult = BSWM_RULE_UNDEF;
    BswM_RuleStateType rightResult = BSWM_RULE_UNDEF;
    BswM_RuleStateType result = BSWM_RULE_UNDEF;
    
    if (expression == NULL) {
        return BSWM_RULE_UNDEF;
    }
    
    /* Evaluate left operand */
    if (expression->data.binary.left != NULL) {
        leftResult = BswM_EvaluateExpression(expression->data.binary.left);
    }
    
    /* Evaluate right operand */
    if (expression->data.binary.right != NULL) {
        rightResult = BswM_EvaluateExpression(expression->data.binary.right);
    }
    
    /* Apply logical operator */
    switch (expression->data.binary.operator) {
        case BSWM_LOGIC_AND:
            /* AND: TRUE if both are TRUE, FALSE if either is FALSE, UNDEF otherwise */
            if ((leftResult == BSWM_RULE_TRUE) && (rightResult == BSWM_RULE_TRUE)) {
                result = BSWM_RULE_TRUE;
            } else if ((leftResult == BSWM_RULE_FALSE) || (rightResult == BSWM_RULE_FALSE)) {
                result = BSWM_RULE_FALSE;
            } else {
                result = BSWM_RULE_UNDEF;
            }
            break;
            
        case BSWM_LOGIC_OR:
            /* OR: TRUE if either is TRUE, FALSE if both are FALSE, UNDEF otherwise */
            if ((leftResult == BSWM_RULE_TRUE) || (rightResult == BSWM_RULE_TRUE)) {
                result = BSWM_RULE_TRUE;
            } else if ((leftResult == BSWM_RULE_FALSE) && (rightResult == BSWM_RULE_FALSE)) {
                result = BSWM_RULE_FALSE;
            } else {
                result = BSWM_RULE_UNDEF;
            }
            break;
            
        default:
            result = BSWM_RULE_UNDEF;
            break;
    }
    
    return result;
}

/**
 * @brief Evaluate a unary expression (NOT)
 */
static BswM_RuleStateType BswM_EvaluateUnaryExpression(
    const BswM_ExpressionStructType *expression)
{
    BswM_RuleStateType operandResult = BSWM_RULE_UNDEF;
    BswM_RuleStateType result = BSWM_RULE_UNDEF;
    
    if (expression == NULL) {
        return BSWM_RULE_UNDEF;
    }
    
    /* Evaluate operand */
    if (expression->data.unary.expr != NULL) {
        operandResult = BswM_EvaluateExpression(expression->data.unary.expr);
    }
    
    /* Apply NOT operator */
    switch (operandResult) {
        case BSWM_RULE_TRUE:
            result = BSWM_RULE_FALSE;
            break;
        case BSWM_RULE_FALSE:
            result = BSWM_RULE_TRUE;
            break;
        default:
            result = BSWM_RULE_UNDEF;
            break;
    }
    
    return result;
}

/**
 * @brief Evaluate a mode condition
 */
static BswM_RuleStateType BswM_EvaluateModeCondition(
    const BswM_ModeConditionType *condition)
{
    BswM_ModeType currentMode = 0U;
    
    if (condition == NULL) {
        return BSWM_RULE_UNDEF;
    }
    
    /* Get current mode from mode request port */
    currentMode = BswM_GetModeRequestPortValue(condition->portId);
    
    /* Compare with expected mode */
    if (currentMode == condition->modeValue) {
        return BSWM_RULE_TRUE;
    } else {
        return BSWM_RULE_FALSE;
    }
}

/**
 * @brief Convert boolean to rule state
 */
static BswM_RuleStateType BswM_BoolToRuleState(boolean value)
{
    return value ? BSWM_RULE_TRUE : BSWM_RULE_FALSE;
}

/******************************************************************************
 * Rule Management Functions
 ******************************************************************************/

Std_ReturnType BswM_AddRule(
    const BswM_RuleType *rule)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (rule == NULL) {
        return E_NOT_OK;
    }
    
    if ((BswM_ConfigPtr == NULL) || (rule->id >= BSWM_MAX_RULES)) {
        return E_NOT_OK;
    }
    
    /* Check if rule ID is valid */
    if (rule->id < BSWM_MAX_RULES) {
        /* Initialize rule state */
        BswM_RuleStates[rule->id] = BSWM_RULE_UNDEF;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType BswM_RemoveRule(BswM_RuleIdType ruleId)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (ruleId < BSWM_MAX_RULES) {
        BswM_RuleStates[ruleId] = BSWM_RULE_UNDEF;
        result = E_OK;
    }
    
    return result;
}

BswM_RuleStateType BswM_GetRuleState(BswM_RuleIdType ruleId)
{
    BswM_RuleStateType state = BSWM_RULE_UNDEF;
    const BswM_StatusType *status = BswM_GetStatus();
    
    if ((status->initialized) && (ruleId < BSWM_MAX_RULES)) {
        state = status->ruleStates[ruleId];
    }
    
    return state;
}

/******************************************************************************
 * Expression Factory Functions
 ******************************************************************************/

BswM_ExpressionStructType* BswM_CreateTrueExpression(void)
{
    /* Static true expression */
    static BswM_ExpressionStructType trueExpr = {
        .type = BSWM_EXPR_TRUE
    };
    return &trueExpr;
}

BswM_ExpressionStructType* BswM_CreateFalseExpression(void)
{
    /* Static false expression */
    static BswM_ExpressionStructType falseExpr = {
        .type = BSWM_EXPR_FALSE
    };
    return &falseExpr;
}

BswM_ExpressionStructType* BswM_CreateAndExpression(
    BswM_ExpressionStructType *left,
    BswM_ExpressionStructType *right)
{
    static BswM_ExpressionStructType andExpr;
    
    andExpr.type = BSWM_EXPR_AND;
    andExpr.data.binary.left = left;
    andExpr.data.binary.right = right;
    andExpr.data.binary.operator = BSWM_LOGIC_AND;
    
    return &andExpr;
}

BswM_ExpressionStructType* BswM_CreateOrExpression(
    BswM_ExpressionStructType *left,
    BswM_ExpressionStructType *right)
{
    static BswM_ExpressionStructType orExpr;
    
    orExpr.type = BSWM_EXPR_OR;
    orExpr.data.binary.left = left;
    orExpr.data.binary.right = right;
    orExpr.data.binary.operator = BSWM_LOGIC_OR;
    
    return &orExpr;
}

BswM_ExpressionStructType* BswM_CreateNotExpression(
    BswM_ExpressionStructType *expr)
{
    static BswM_ExpressionStructType notExpr;
    
    notExpr.type = BSWM_EXPR_NOT;
    notExpr.data.unary.expr = expr;
    
    return &notExpr;
}

BswM_ExpressionStructType* BswM_CreateModeRequestExpression(
    const BswM_ModeConditionType *condition)
{
    static BswM_ExpressionStructType modeExpr;
    
    modeExpr.type = BSWM_EXPR_MODE_REQUEST;
    modeExpr.data.mode.condition = condition;
    
    return &modeExpr;
}

/**
 * @file BswM.c
 * @brief BSW Mode Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/**
     2| * @file BswM.c
     3| * @brief BSW Mode Manager Implementation - Full Rule Engine
     4| * @version 2.0.0
     5| */
     6|
     7|#include "BswM.h"
     8|#include "BswM_Cfg.h"
     9|
    10|/* Forward declarations for external modules to avoid header conflicts */
    11|extern void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
    12|
    13|/* Local definitions for ComM modes */
    14|#ifndef COMM_NO_COMMUNICATION
    15|#define COMM_NO_COMMUNICATION               0x00
    16|#define COMM_SILENT_COMMUNICATION           0x01
    17|#define COMM_FULL_COMMUNICATION             0x02
    18|typedef uint8 ComM_ModeType;
    19|#endif
    20|
    21|/* Forward declarations for external modules */
    22|extern void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
    23|
    24|#define BSWM_MODULE_ID                      0x0B
    25|#define BSWM_INSTANCE_ID                    0x00
    26|
    27|/* Additional Service IDs */
    28|#define BSWM_RULE_EVALUATE_SID              0x10
    29|#define BSWM_RULE_INIT_SID                  0x11
    30|#define BSWM_MODESWITCHNOTIF_SID            0x12
    31|#define BSWM_COMM_CURRENTMODE_SID           0x20
    32|#define BSWM_DCM_APPUPDATED_SID             0x30
    33|#define BSWM_DCM_COMMODE_CURRSTATE_SID      0x31
    34|
    35|/* Module State */
    36|static BswM_InternalStateType BswM_State;
    37|static boolean BswM_IsInitialized = FALSE;
    38|static uint16 BswM_CurrentMode[BSWM_MAX_ACTION_LISTS];
    39|
    40|/* Forward declarations */
    41|static boolean BswM_EvaluateCondition(const BswM_ModeConditionType* condition);
    42|static boolean BswM_EvaluateExpression(const BswM_RuleExpressionType* expression);
    43|static void BswM_ExecuteActionList(uint16 actionListId);
    44|static void BswM_ExecuteAction(const BswM_ActionItem* action);
    45|static void BswM_ProcessModeQueue(void);
    46|static void BswM_EvaluateAllRules(void);
    47|static void BswM_EvaluateDeferredRules(void);
    48|static void BswM_HandleRuleResult(uint16 ruleId, BswM_RuleStateType result);
    49|static boolean BswM_EnqueueModeRequest(BswM_UserType user, uint16 mode);
    50|static boolean BswM_GetModeValue(uint16 modeRequestId, BswM_RequestSourceType source, uint16* modeValue);
    51|
    52|/******************************************************************************
    53| * Standard API Implementations
    54| *****************************************************************************/
    55|
    56|void BswM_Init(const void* ConfigPtr)
    57|{
    58|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    59|    if (BswM_IsInitialized)
    60|    {
    61|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_INIT_SID, BSWM_E_NOT_INITIALIZED);
    62|        return;
    63|    }
    64|    if (ConfigPtr == NULL_PTR)
    65|    {
    66|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_INIT_SID, BSWM_E_NULL_POINTER);
    67|        return;
    68|    }
    69|#endif
    70|    
    71|    (void)ConfigPtr;
    72|    
    73|    /* Initialize internal state */
    74|    BswM_State.isInitialized = FALSE;
    75|    BswM_State.isDeinitRequested = FALSE;
    76|    BswM_State.mainFunctionCounter = 0;
    77|    BswM_State.numRules = 0;
    78|    BswM_State.numActionLists = 0;
    79|    BswM_State.numModeConditions = 0;
    80|    BswM_State.rulesNeedEvaluation = FALSE;
    81|    BswM_State.queueHead = 0;
    82|    BswM_State.queueTail = 0;
    83|    BswM_State.queueCount = 0;
    84|    BswM_State.rulesEvaluated = 0;
    85|    BswM_State.rulesTriggered = 0;
    86|    BswM_State.actionsExecuted = 0;
    87|    BswM_State.queueOverflows = 0;
    88|    
    89|    /* Initialize mode queues */
    90|    for (uint8 i = 0; i < 32; i++)
    91|    {
    92|        BswM_State.modeQueue[i].isPending = FALSE;
    93|        BswM_State.modeQueue[i].timestamp = 0;
    94|    }
    95|    
    96|    /* Initialize all modes to default */
    97|    for (uint16 i = 0; i < BSWM_MAX_ACTION_LISTS; i++)
    98|    {
    99|        BswM_CurrentMode[i] = 0;
   100|        BswM_State.actionLists[i].actionListId = i;
   101|        BswM_State.actionLists[i].numActions = 0;
   102|        BswM_State.actionLists[i].isExecuted = FALSE;
   103|    }
   104|    
   105|    /* Initialize rules */
   106|    for (uint16 i = 0; i < BSWM_MAX_RULES; i++)
   107|    {
   108|        BswM_State.rules[i].ruleId = i;
   109|        BswM_State.rules[i].isEnabled = FALSE;
   110|        BswM_State.rules[i].isDeferred = FALSE;
   111|        BswM_State.rules[i].lastResult = BSWM_RULE_NOT_EVALUATED;
   112|        BswM_State.ruleResults[i] = BSWM_RULE_NOT_EVALUATED;
   113|    }
   114|    
   115|    /* Initialize communication modes */
   116|    for (uint8 i = 0; i < 8; i++)
   117|    {
   118|        BswM_State.commModes[i] = COMM_NO_COMMUNICATION;
   119|        BswM_State.dcmModes[i] = DCM_ENABLE_DEFAULT_RX_TX;
   120|        BswM_State.ecumModes[i] = ECUM_STATE_STARTUP_ONE;
   121|    }
   122|    
   123|    /* Initialize generic modes */
   124|    for (uint8 i = 0; i < 16; i++)
   125|    {
   126|        BswM_State.genericModes[i] = 0;
   127|    }
   128|    
   129|    /* Initialize mode change tracking */
   130|    for (uint8 i = 0; i < BSWM_REQ_SOURCE_MAX; i++)
   131|    {
   132|        BswM_State.modeChanged[i] = FALSE;
   133|        BswM_State.lastEvaluatedMode[i] = 0;
   134|    }
   135|    
   136|    /* Initialize mode conditions */
   137|    for (uint8 i = 0; i < 64; i++)
   138|    {
   139|        BswM_State.modeConditions[i].modeRequestId = 0;
   140|        BswM_State.modeConditions[i].expectedMode = 0;
   141|        BswM_State.modeConditions[i].condition = BSWM_COND_EQUALS;
   142|        BswM_State.modeConditions[i].source = BSWM_REQ_SOURCE_GENERIC;
   143|        BswM_State.modeConditions[i].isAvailable = FALSE;
   144|    }
   145|    
   146|    BswM_State.isInitialized = TRUE;
   147|    BswM_IsInitialized = TRUE;
   148|}
   149|
   150|void BswM_Deinit(void)
   151|{
   152|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   153|    if (!BswM_IsInitialized)
   154|    {
   155|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_DEINIT_SID, BSWM_E_NOT_INITIALIZED);
   156|        return;
   157|    }
   158|#endif
   159|    
   160|    /* Mark all rules as not evaluated */
   161|    for (uint16 i = 0; i < BSWM_MAX_RULES; i++)
   162|    {
   163|        BswM_State.rules[i].lastResult = BSWM_RULE_NOT_EVALUATED;
   164|        BswM_State.rules[i].isEnabled = FALSE;
   165|    }
   166|    
   167|    /* Clear any pending mode requests */
   168|    BswM_State.queueHead = 0;
   169|    BswM_State.queueTail = 0;
   170|    BswM_State.queueCount = 0;
   171|    for (uint8 i = 0; i < 32; i++)
   172|    {
   173|        BswM_State.modeQueue[i].isPending = FALSE;
   174|    }
   175|    
   176|    BswM_State.isInitialized = FALSE;
   177|    BswM_IsInitialized = FALSE;
   178|}
   179|
   180|void BswM_RequestMode(BswM_UserType requesting_user, uint16 requested_mode)
   181|{
   182|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   183|    if (!BswM_IsInitialized)
   184|    {
   185|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_REQUESTMODE_SID, BSWM_E_NOT_INITIALIZED);
   186|        return;
   187|    }
   188|    if (requesting_user >= BSWM_MAX_ACTION_LISTS)
   189|    {
   190|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_REQUESTMODE_SID, BSWM_E_INVALID_PAR);
   191|        return;
   192|    }
   193|#endif
   194|    
   195|    /* Queue the mode request */
   196|    if (BswM_EnqueueModeRequest(requesting_user, requested_mode))
   197|    {
   198|        /* Mark rules for evaluation */
   199|        BswM_State.rulesNeedEvaluation = TRUE;
   200|        BswM_CurrentMode[requesting_user] = requested_mode;
   201|    }
   202|}
   203|
   204|void BswM_MainFunction(void)
   205|{
   206|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   207|    if (!BswM_IsInitialized)
   208|    {
   209|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_MAINFUNCTION_SID, BSWM_E_NOT_INITIALIZED);
   210|        return;
   211|    }
   212|#endif
   213|    
   214|    BswM_State.mainFunctionCounter++;
   215|    
   216|    /* Process any pending mode requests from queue */
   217|    BswM_ProcessModeQueue();
   218|    
   219|    /* Evaluate deferred rules */
   220|    BswM_EvaluateDeferredRules();
   221|    
   222|    /* If mode changes detected, evaluate all rules */
   223|    if (BswM_State.rulesNeedEvaluation)
   224|    {
   225|        BswM_EvaluateAllRules();
   226|        BswM_State.rulesNeedEvaluation = FALSE;
   227|    }
   228|    
   229|    /* Check for any pending actions that need to be executed */
   230|    for (uint8 i = 0; i < BswM_State.numActionLists; i++)
   231|    {
   232|        if (!BswM_State.actionLists[i].isExecuted && BswM_State.actionLists[i].numActions > 0)
   233|        {
   234|            /* Action list needs execution */
   235|            ; /* Handled by rule evaluation */
   236|        }
   237|    }
   238|}
   239|
   240|/******************************************************************************
   241| * New API Implementations
   242| *****************************************************************************/
   243|
   244|/**
   245| * @brief Initialize a specific rule
   246| */
   247|void BswM_Rule_Init(uint16 ruleId)
   248|{
   249|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   250|    if (!BswM_IsInitialized)
   251|    {
   252|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_RULE_INIT_SID, BSWM_E_NOT_INITIALIZED);
   253|        return;
   254|    }
   255|    if (ruleId >= BSWM_MAX_RULES)
   256|    {
   257|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_RULE_INIT_SID, BSWM_E_INVALID_PAR);
   258|        return;
   259|    }
   260|#endif
   261|
   262|    if (ruleId < BSWM_MAX_RULES)
   263|    {
   264|        BswM_State.rules[ruleId].lastResult = BSWM_RULE_NOT_EVALUATED;
   265|        BswM_State.rules[ruleId].isEnabled = TRUE;
   266|        
   267|        /* Initialize expression */
   268|        BswM_State.rules[ruleId].expression.numConditions = 0;
   269|        BswM_State.rules[ruleId].expression.logicalOp = BSWM_LOGIC_AND;
   270|        
   271|        for (uint8 i = 0; i < 8; i++)
   272|        {
   273|            BswM_State.rules[ruleId].expression.conditionIds[i] = 0xFFFF;
   274|        }
   275|        
   276|        BswM_State.ruleResults[ruleId] = BSWM_RULE_NOT_EVALUATED;
   277|    }
   278|}
   279|
   280|/**
   281| * @brief Evaluate a specific rule
   282| */
   283|BswM_RuleStateType BswM_Rule_Evaluate(uint16 ruleId)
   284|{
   285|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   286|    if (!BswM_IsInitialized)
   287|    {
   288|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_RULE_EVALUATE_SID, BSWM_E_NOT_INITIALIZED);
   289|        return BSWM_RULE_NOT_EVALUATED;
   290|    }
   291|    if (ruleId >= BSWM_MAX_RULES)
   292|    {
   293|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_RULE_EVALUATE_SID, BSWM_E_INVALID_PAR);
   294|        return BSWM_RULE_NOT_EVALUATED;
   295|    }
   296|#endif
   297|
   298|    BswM_RuleStateType result = BSWM_RULE_NOT_EVALUATED;
   299|    
   300|    if (ruleId < BSWM_MAX_RULES && BswM_State.rules[ruleId].isEnabled)
   301|    {
   302|        boolean evalResult = BswM_EvaluateExpression(&BswM_State.rules[ruleId].expression);
   303|        result = evalResult ? BSWM_RULE_TRUE : BSWM_RULE_FALSE;
   304|        
   305|        BswM_State.rules[ruleId].lastResult = result;
   306|        BswM_State.ruleResults[ruleId] = result;
   307|        BswM_State.rulesEvaluated++;
   308|        
   309|        /* Handle rule result - execute appropriate action list */
   310|        BswM_HandleRuleResult(ruleId, result);
   311|    }
   312|    
   313|    return result;
   314|}
   315|
   316|/**
   317| * @brief Handle mode switch notification from RTE
   318| */
   319|void BswM_ModeSwitchNotification(uint16 modeGroupId, uint16 newMode, uint8 instanceId)
   320|{
   321|    (void)instanceId;
   322|    
   323|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   324|    if (!BswM_IsInitialized)
   325|    {
   326|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_MODESWITCHNOTIF_SID, BSWM_E_NOT_INITIALIZED);
   327|        return;
   328|    }
   329|#endif
   330|
   331|    /* Update the mode value */
   332|    if (modeGroupId < 16)
   333|    {
   334|        BswM_State.genericModes[modeGroupId] = newMode;
   335|        BswM_State.modeChanged[BSWM_REQ_SOURCE_SWC] = TRUE;
   336|        
   337|        /* Trigger rule evaluation */
   338|        BswM_State.rulesNeedEvaluation = TRUE;
   339|        
   340|        /* Process immediately if not deferred */
   341|        BswM_EvaluateAllRules();
   342|    }
   343|}
   344|
   345|/**
   346| * @brief Handle ComM current mode indication
   347| */
   348|void BswM_ComM_CurrentMode(uint8 networkId, ComM_ModeType mode)
   349|{
   350|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   351|    if (!BswM_IsInitialized)
   352|    {
   353|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_COMM_CURRENTMODE_SID, BSWM_E_NOT_INITIALIZED);
   354|        return;
   355|    }
   356|#endif
   357|
   358|    if (networkId < 8)
   359|    {
   360|        /* Update stored ComM mode */
   361|        BswM_State.commModes[networkId] = (uint16)mode;
   362|        BswM_State.modeChanged[BSWM_REQ_SOURCE_COMM] = TRUE;
   363|        
   364|        /* Mark rules for evaluation */
   365|        BswM_State.rulesNeedEvaluation = TRUE;
   366|        
   367|        /* Find and evaluate rules that depend on ComM modes */
   368|        for (uint8 i = 0; i < BswM_State.numRules; i++)
   369|        {
   370|            if (BswM_State.rules[i].isEnabled)
   371|            {
   372|                /* Check if rule has ComM conditions */
   373|                for (uint8 j = 0; j < BswM_State.rules[i].expression.numConditions; j++)
   374|                {
   375|                    uint16 condId = BswM_State.rules[i].expression.conditionIds[j];
   376|                    if (condId < 64 && BswM_State.modeConditions[condId].source == BSWM_REQ_SOURCE_COMM)
   377|                    {
   378|                        if (!BswM_State.rules[i].isDeferred)
   379|                        {
   380|                            BswM_Rule_Evaluate(i);
   381|                        }
   382|                        break;
   383|                    }
   384|                }
   385|            }
   386|        }
   387|    }
   388|}
   389|
   390|/**
   391| * @brief Handle DCM application updated notification
   392| */
   393|void BswM_Dcm_ApplicationUpdated(void)
   394|{
   395|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   396|    if (!BswM_IsInitialized)
   397|    {
   398|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_DCM_APPUPDATED_SID, BSWM_E_NOT_INITIALIZED);
   399|        return;
   400|    }
   401|#endif
   402|
   403|    /* Trigger evaluation of rules that depend on DCM application update */
   404|    BswM_State.rulesNeedEvaluation = TRUE;
   405|    
   406|    /* Evaluate all enabled rules */
   407|    for (uint8 i = 0; i < BswM_State.numRules; i++)
   408|    {
   409|        if (BswM_State.rules[i].isEnabled)
   410|        {
   411|            BswM_Rule_Evaluate(i);
   412|        }
   413|    }
   414|}
   415|
   416|/**
   417| * @brief Handle DCM communication mode current state
   418| */
   419|void BswM_Dcm_CommunicationMode_CurrentState(uint8 channelId, Dcm_CommunicationModeType mode)
   420|{
   421|#if (BSWM_DEV_ERROR_DETECT == STD_ON)
   422|    if (!BswM_IsInitialized)
   423|    {
   424|        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_DCM_COMMODE_CURRSTATE_SID, BSWM_E_NOT_INITIALIZED);
   425|        return;
   426|    }
   427|#endif
   428|
   429|    if (channelId < 8)
   430|    {
   431|        /* Update stored DCM mode */
   432|        BswM_State.dcmModes[channelId] = (uint16)mode;
   433|        BswM_State.modeChanged[BSWM_REQ_SOURCE_DCM] = TRUE;
   434|        
   435|        /* Mark rules for evaluation */
   436|        BswM_State.rulesNeedEvaluation = TRUE;
   437|        
   438|        /* Evaluate rules with DCM conditions */
   439|        for (uint8 i = 0; i < BswM_State.numRules; i++)
   440|        {
   441|            if (BswM_State.rules[i].isEnabled)
   442|            {
   443|                for (uint8 j = 0; j < BswM_State.rules[i].expression.numConditions; j++)
   444|                {
   445|                    uint16 condId = BswM_State.rules[i].expression.conditionIds[j];
   446|                    if (condId < 64 && BswM_State.modeConditions[condId].source == BSWM_REQ_SOURCE_DCM)
   447|                    {
   448|                        BswM_Rule_Evaluate(i);
   449|                        break;
   450|                    }
   451|                }
   452|            }
   453|        }
   454|    }
   455|}
   456|
   457|/******************************************************************************
   458| * Private Helper Functions
   459| *****************************************************************************/
   460|
   461|/**
   462| * @brief Evaluate a single mode condition
   463| */
   464|static boolean BswM_EvaluateCondition(const BswM_ModeConditionType* condition)
   465|{
   466|    if (condition == NULL_PTR || !condition->isAvailable)
   467|    {
   468|        return FALSE;
   469|    }
   470|    
   471|    uint16 currentMode = 0;
   472|    if (!BswM_GetModeValue(condition->modeRequestId, condition->source, &currentMode))
   473|    {
   474|        return FALSE;
   475|    }
   476|    
   477|    boolean result = FALSE;
   478|    
   479|    switch (condition->condition)
   480|    {
   481|        case BSWM_COND_EQUALS:
   482|            result = (currentMode == condition->expectedMode);
   483|            break;
   484|        case BSWM_COND_NOT_EQUALS:
   485|            result = (currentMode != condition->expectedMode);
   486|            break;
   487|        case BSWM_COND_GREATER_THAN:
   488|            result = (currentMode > condition->expectedMode);
   489|            break;
   490|        case BSWM_COND_LESS_THAN:
   491|            result = (currentMode < condition->expectedMode);
   492|            break;
   493|        case BSWM_COND_GREATER_EQUAL:
   494|            result = (currentMode >= condition->expectedMode);
   495|            break;
   496|        case BSWM_COND_LESS_EQUAL:
   497|            result = (currentMode <= condition->expectedMode);
   498|            break;
   499|        default:
   500|            result = FALSE;
   501|
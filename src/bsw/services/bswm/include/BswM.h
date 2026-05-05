/**
 * @file BswM.h
 * @brief BSW Mode Manager
 * @version 2.0.0
 */

#ifndef BSWM_H
#define BSWM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define BSWM_AR_RELEASE_MAJOR_VERSION       4
#define BSWM_AR_RELEASE_MINOR_VERSION       0
#define BSWM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define BSWM_SW_MAJOR_VERSION               2
#define BSWM_SW_MINOR_VERSION               0
#define BSWM_SW_PATCH_VERSION               0

/* Service IDs */
#define BSWM_INIT_SID                       0x00
#define BSWM_DEINIT_SID                     0x01
#define BSWM_REQUESTMODE_SID                0x02
#define BSWM_MAINFUNCTION_SID               0x03

/* New Service IDs */
#define BSWM_RULE_EVALUATE_SID              0x10
#define BSWM_RULE_INIT_SID                  0x11
#define BSWM_MODESWITCHNOTIF_SID            0x12
#define BSWM_COMM_CURRENTMODE_SID           0x20
#define BSWM_DCM_APPUPDATED_SID             0x30
#define BSWM_DCM_COMMODE_CURRSTATE_SID      0x31

/* Error Codes */
#define BSWM_E_NO_ERROR                     0x00
#define BSWM_E_NULL_POINTER                 0x01
#define BSWM_E_INVALID_PAR                  0x02
#define BSWM_E_NOT_INITIALIZED              0x03

/* Types */
typedef uint8 BswM_UserType;

typedef enum {
    BSWM_GENERICVALUE_MIN = 0,
    BSWM_GENERICVALUE_MAX = 255
} BswM_GenericValueType;

typedef struct {
    uint16 mode;
    uint16 user;
} BswM_ModeDeclarationType;

/* Rule State Type */
typedef enum {
    BSWM_RULE_NOT_EVALUATED = 0,
    BSWM_RULE_TRUE,
    BSWM_RULE_FALSE,
    BSWM_RULE_MAX_STATE
} BswM_RuleStateType;

/* Action Types */
typedef enum {
    BSWM_ACTION_ECU_STATE = 0,
    BSWM_ACTION_COMM_MODE,
    BSWM_ACTION_DCM_RSP,
    BSWM_ACTION_RTE_SWITCH,
    BSWM_ACTION_RTE_EVENT,
    BSWM_ACTION_NVM_READ_REQ,
    BSWM_ACTION_NVM_WRITE_REQ,
    BSWM_ACTION_TIMER_CONTROL,
    BSWM_ACTION_LIN_SCHEDULE,
    BSWM_ACTION_MAX
} BswM_ActionItemType;

/* Logical Operation Types */
typedef enum {
    BSWM_LOGIC_AND = 0,
    BSWM_LOGIC_OR,
    BSWM_LOGIC_NAND,
    BSWM_LOGIC_NOR
} BswM_LogicalOpType;

/* Mode Condition Types */
typedef enum {
    BSWM_COND_EQUALS = 0,
    BSWM_COND_NOT_EQUALS,
    BSWM_COND_GREATER_THAN,
    BSWM_COND_LESS_THAN,
    BSWM_COND_GREATER_EQUAL,
    BSWM_COND_LESS_EQUAL
} BswM_ConditionType;

/* Mode Request Source Types */
typedef enum {
    BSWM_REQ_SOURCE_COMM = 0,
    BSWM_REQ_SOURCE_DCM,
    BSWM_REQ_SOURCE_ECUM,
    BSWM_REQ_SOURCE_GENERIC,
    BSWM_REQ_SOURCE_SWC,
    BSWM_REQ_SOURCE_MAX
} BswM_RequestSourceType;

/* Mode Condition Structure */
typedef struct {
    uint16 modeRequestId;
    uint16 expectedMode;
    BswM_ConditionType condition;
    BswM_RequestSourceType source;
    boolean isAvailable;
} BswM_ModeConditionType;

/* Rule Expression (combines multiple conditions) */
typedef struct {
    uint16 conditionIds[8];
    uint8 numConditions;
    BswM_LogicalOpType logicalOp;
} BswM_RuleExpressionType;

/* Action Item Structure */
typedef struct {
    BswM_ActionItemType actionType;
    uint16 actionId;
    uint16 parameter;
    boolean isAbortOnFail;
} BswM_ActionItem;

/* Action List Structure */
typedef struct {
    uint16 actionListId;
    BswM_ActionItem actions[16];
    uint8 numActions;
    boolean isExecuted;
} BswM_ActionListType;

/* Rule Definition Structure */
typedef struct {
    uint16 ruleId;
    BswM_RuleExpressionType expression;
    uint16 trueActionListId;
    uint16 falseActionListId;
    BswM_RuleStateType lastResult;
    boolean isDeferred;
    boolean isEnabled;
} BswM_RuleType;

/* Mode Request Queue Entry */
typedef struct {
    BswM_UserType user;
    uint16 requestedMode;
    boolean isPending;
    uint8 timestamp;
} BswM_ModeRequestQueueEntry;

/* Internal State Structure */
typedef struct {
    boolean isInitialized;
    boolean isDeinitRequested;
    uint8 mainFunctionCounter;
    
    /* Rule Engine State */
    BswM_RuleType rules[32];  /* BSWM_MAX_RULES */
    BswM_ActionListType actionLists[16];  /* BSWM_MAX_ACTION_LISTS */
    BswM_ModeConditionType modeConditions[64];
    
    uint8 numRules;
    uint8 numActionLists;
    uint8 numModeConditions;
    
    /* Rule Evaluation State */
    BswM_RuleStateType ruleResults[32];
    boolean rulesNeedEvaluation;
    
    /* Mode Request Queue */
    BswM_ModeRequestQueueEntry modeQueue[32];
    uint8 queueHead;
    uint8 queueTail;
    uint8 queueCount;
    
    /* Current Modes for various sources */
    uint16 commModes[8];
    uint16 dcmModes[8];
    uint16 ecumModes[8];
    uint16 genericModes[16];
    
    /* Mode Change Tracking */
    boolean modeChanged[BSWM_REQ_SOURCE_MAX];
    uint16 lastEvaluatedMode[BSWM_REQ_SOURCE_MAX];
    
    /* Statistics */
    uint32 rulesEvaluated;
    uint32 rulesTriggered;
    uint32 actionsExecuted;
    uint32 queueOverflows;
    
} BswM_InternalStateType;

/* DCM Communication Mode Type (for BswM interface) */
typedef enum {
    DCM_ENABLE_DEFAULT_RX_TX = 0x00,
    DCM_ENABLE_RX_TX_NORM = 0x01,
    DCM_ENABLE_RX_TX_NM = 0x02,
    DCM_ENABLE_RX_TX_NORM_NM = 0x03,
    DCM_DISABLE_DEFAULT_RX_TX = 0x04,
    DCM_DISABLE_RX_TX_NORM = 0x05,
    DCM_DISABLE_RX_TX_NM = 0x06,
    DCM_DISABLE_RX_TX_NORM_NM = 0x07
} Dcm_CommunicationModeType;

/* ComM Mode Type (for BswM interface) */
typedef uint8 ComM_ModeType;
#define COMM_NO_COMMUNICATION               0x00
#define COMM_SILENT_COMMUNICATION           0x01
#define COMM_FULL_COMMUNICATION             0x02

/* EcuM state constants */
#define ECUM_STATE_STARTUP_ONE    0x01
#define ECUM_STATE_STARTUP_TWO    0x02
#define ECUM_STATE_RUN            0x03
#define ECUM_STATE_SLEEP          0x04
#define ECUM_STATE_SHUTDOWN       0x05

/* Function Prototypes - Standard APIs */
extern void BswM_Init(const void* ConfigPtr);
extern void BswM_Deinit(void);
extern void BswM_RequestMode(BswM_UserType requesting_user, uint16 requested_mode);
extern void BswM_MainFunction(void);

/* Function Prototypes - Rule Engine APIs */
extern void BswM_Rule_Init(uint16 ruleId);
extern BswM_RuleStateType BswM_Rule_Evaluate(uint16 ruleId);

/* Function Prototypes - Mode Notification APIs */
extern void BswM_ModeSwitchNotification(uint16 modeGroupId, uint16 newMode, uint8 instanceId);
extern void BswM_ComM_CurrentMode(uint8 networkId, ComM_ModeType mode);
extern void BswM_Dcm_ApplicationUpdated(void);
extern void BswM_Dcm_CommunicationMode_CurrentState(uint8 channelId, Dcm_CommunicationModeType mode);

/* Function Prototypes - Configuration APIs */
extern void BswM_SetModeCondition(uint16 conditionId, const BswM_ModeConditionType* condition);
extern void BswM_SetRule(uint16 ruleId, const BswM_RuleType* rule);
extern void BswM_SetActionList(uint16 actionListId, const BswM_ActionListType* actionList);

/* Function Prototypes - Diagnostic APIs */
extern void BswM_GetStatistics(uint32* rulesEvaluated, uint32* rulesTriggered, uint32* actionsExecuted);
extern void BswM_ClearStatistics(void);
extern BswM_RuleStateType BswM_GetRuleState(uint16 ruleId);
extern void BswM_SetRuleState(uint16 ruleId, boolean enabled);

#endif /* BSWM_H */

/******************************************************************************
 * @file    bswm.h
 * @brief   BSW Mode Manager (BswM) Interface
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef BSWM_H
#define BSWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include "../ecum/ecum.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * BswM Version Information
 ******************************************************************************/
#define BSWM_VENDOR_ID                  0x01U
#define BSWM_MODULE_ID                  0x0DU  /* BswM module ID per AUTOSAR */
#define BSWM_SW_MAJOR_VERSION           1U
#define BSWM_SW_MINOR_VERSION           0U
#define BSWM_SW_PATCH_VERSION           0U

/******************************************************************************
 * BswM Configuration Constants
 ******************************************************************************/
#define BSWM_MAX_RULES                  64U
#define BSWM_MAX_ACTION_LISTS           32U
#define BSWM_MAX_MODES                  32U
#define BSWM_MAX_MODE_REQUEST_PORTS     32U
#define BSWM_MAX_ACTIONS_PER_LIST       16U

/******************************************************************************
 * BswM Return Types
 ******************************************************************************/
typedef enum {
    BSWM_E_OK = 0,
    BSWM_E_NOT_OK = 1,
    BSWM_E_NO_RULE = 2,
    BSWM_E_NULL_POINTER = 3,
    BSWM_E_INVALID_MODE = 4,
    BSWM_E_ACTION_FAILED = 5
} BswM_ReturnType;

/******************************************************************************
 * BswM Handle Types
 ******************************************************************************/
typedef uint16 BswM_RuleIdType;
typedef uint16 BswM_ActionListIdType;
typedef uint16 BswM_ModeIdType;
typedef uint16 BswM_UserType;
typedef uint32 BswM_ModeType;

#define BSWM_RULE_ID_INVALID            ((BswM_RuleIdType)0xFFFFU)
#define BSWM_ACTION_LIST_ID_INVALID     ((BswM_ActionListIdType)0xFFFFU)
#define BSWM_MODE_ID_INVALID            ((BswM_ModeIdType)0xFFFFU)
#define BSWM_USER_ID_INVALID            ((BswM_UserType)0xFFFFU)

/******************************************************************************
 * BswM Rule Expression Types
 ******************************************************************************/
typedef enum {
    BSWM_EXPR_AND = 0,
    BSWM_EXPR_OR,
    BSWM_EXPR_NOT,
    BSWM_EXPR_MODE_REQUEST,
    BSWM_EXPR_MODE_INDICATION,
    BSWM_EXPR_EVENT,
    BSWM_EXPR_TRUE,
    BSWM_EXPR_FALSE
} BswM_ExpressionType;

/******************************************************************************
 * BswM Rule State Types
 ******************************************************************************/
typedef enum {
    BSWM_RULE_FALSE = 0,
    BSWM_RULE_TRUE,
    BSWM_RULE_UNDEF
} BswM_RuleStateType;

/******************************************************************************
 * BswM Action Types
 ******************************************************************************/
typedef enum {
    /* BswM specific actions */
    BSWM_ACTION_REQUEST_MODE = 0,
    BSWM_ACTION_SWITCH_MODE,
    BSWM_ACTION_EXECUTE_ACTION_LIST,
    
    /* ComM actions */
    BSWM_ACTION_COMM_ALLOW_COM,
    BSWM_ACTION_COMM_MODE_LIMITATION,
    BSWM_ACTION_COMM_MODE_SWITCH,
    
    /* Dcm actions */
    BSWM_ACTION_DCM_ENABLE_RESET,
    BSWM_ACTION_DCM_DISABLE_RESET,
    BSWM_ACTION_DCM_ENABLE_DTC_SETTING,
    BSWM_ACTION_DCM_DISABLE_DTC_SETTING,
    BSWM_ACTION_DCM_REQUEST_SESSION_MODE,
    
    /* EcuM actions */
    BSWM_ACTION_ECUM_GO_DOWN,
    BSWM_ACTION_ECUM_SELECT_SHUTDOWN_TARGET,
    BSWM_ACTION_ECUM_RELEASE_RUN_REQUEST,
    
    /* Other BSW actions */
    BSWM_ACTION_DEADLINE_MONITORING_CONTROL,
    BSWM_ACTION_PDUR_DISABLE_ROUTING,
    BSWM_ACTION_PDUR_ENABLE_ROUTING,
    
    /* Custom actions */
    BSWM_ACTION_USER_CALLBACK,
    BSWM_ACTION_SCHEDULER_REFERENCE
} BswM_ActionType;

/******************************************************************************
 * BswM Mode Request Port Types
 ******************************************************************************/
typedef enum {
    BSWM_MRP_BSW_MODE_NOTIFICATION = 0,
    BSWM_MRP_CAN_SM_INDICATION,
    BSWM_MRP_COMM_INDICATION,
    BSWM_MRP_COMM_PNC_REQUEST,
    BSWM_MRP_COMM_REQUEST,
    BSWM_MRP_DCM_APPLICATION_UPDATE,
    BSWM_MRP_DCM_COM_MODE_CURRENT,
    BSWM_MRP_DCM_COM_MODE_REQUEST,
    BSWM_MRP_DCM_RESET_MODE_REQUEST,
    BSWM_MRP_DCM_SESSION_MODE_REQUEST,
    BSWM_MRP_ECUM_INDICATION,
    BSWM_MRP_ECUM_WK_STATUS,
    BSWM_MRP_ETH_SM_INDICATION,
    BSWM_MRP_FR_SM_INDICATION,
    BSWM_MRP_GENERIC_REQUEST,
    BSWM_MRP_LIN_SCHEDULE_INDICATION,
    BSWM_MRP_LIN_SM_INDICATION,
    BSWM_MRP_LINSM_SCHEDULE,
    BSWM_MRP_NM_IF_CAR_WK_ECU,
    BSWM_MRP_NVM_JOB_MODE_INDICATION,
    BSWM_MRP_SD_CLIENT_SERVICE_MODE,
    BSWM_MRP_SD_CONSUMED_EVENT_GROUP_MODE,
    BSWM_MRP_SD_EVENT_HANDLER_MODE,
    BSWM_MRP_SWC_MODE_REQUEST,
    BSWM_MRP_SWC_MODE_NOTIFICATION,
    BSWM_MRP_TIMER
} BswM_ModeRequestPortType;

/******************************************************************************
 * BswM Logical Operator Types
 ******************************************************************************/
typedef enum {
    BSWM_LOGIC_AND = 0,
    BSWM_LOGIC_OR,
    BSWM_LOGIC_NOT
} BswM_LogicalOperatorType;

/******************************************************************************
 * BswM Configuration Types
 ******************************************************************************/

/* Forward declarations */
struct BswM_RuleStruct;
struct BswM_ActionListStruct;
struct BswM_ActionItemStruct;
struct BswM_ModeConditionStruct;
struct BswM_ExpressionStruct;

/* Mode Condition */
typedef struct BswM_ModeConditionStruct {
    BswM_ModeRequestPortType portType;
    uint16 portId;
    BswM_ModeType modeValue;
    BswM_ModeType initialMode;
    boolean isInitialized;
} BswM_ModeConditionType;

/* Expression Structure */
typedef struct BswM_ExpressionStruct {
    BswM_ExpressionType type;
    union {
        struct {
            const struct BswM_ExpressionStruct *left;
            const struct BswM_ExpressionStruct *right;
            BswM_LogicalOperatorType operator;
        } binary;
        struct {
            const struct BswM_ExpressionStruct *expr;
        } unary;
        struct {
            const BswM_ModeConditionType *condition;
        } mode;
    } data;
} BswM_ExpressionStructType;

/* Action Item */
typedef struct BswM_ActionItemStruct {
    BswM_ActionType actionType;
    union {
        struct {
            BswM_UserType user;
            BswM_ModeType mode;
        } requestMode;
        struct {
            BswM_ModeIdType modeId;
            BswM_ModeType mode;
        } switchMode;
        struct {
            BswM_ActionListIdType actionListId;
        } executeList;
        struct {
            uint8 channel;
            ComM_ModeType comMode;
        } comMMode;
        struct {
            uint8 sessionMode;
        } dcmSession;
        struct {
            EcuM_ShutdownTargetType target;
            uint8 mode;
        } shutdownTarget;
        struct {
            EcuM_UserType user;
        } ecumUser;
        struct {
            void (*callback)(void);
        } userCallback;
    } parameters;
    boolean abortOnFail;
    uint8 retryCount;
} BswM_ActionItemType;

/* Action List */
typedef struct BswM_ActionListStruct {
    BswM_ActionListIdType id;
    boolean executeOnlyIf;
    BswM_RuleStateType condition;
    uint8 numActions;
    const BswM_ActionItemType *actions;
} BswM_ActionListType;

/* Rule */
typedef struct BswM_RuleStruct {
    BswM_RuleIdType id;
    const BswM_ExpressionStructType *expression;
    BswM_ActionListIdType trueActionList;
    BswM_ActionListIdType falseActionList;
    BswM_RuleStateType lastState;
    boolean isInitialized;
} BswM_RuleType;

/* Mode Request Port */
typedef struct {
    BswM_ModeRequestPortType type;
    uint16 id;
    BswM_ModeType currentMode;
    BswM_ModeType requestedMode;
    boolean requestPending;
    boolean isInitialized;
} BswM_ModeRequestPortStructType;

/* BswM Configuration */
typedef struct {
    /* Rules */
    const BswM_RuleType *rules;
    uint16 numRules;
    
    /* Action Lists */
    const BswM_ActionListType *actionLists;
    uint16 numActionLists;
    
    /* Mode Request Ports */
    BswM_ModeRequestPortStructType *modeRequestPorts;
    uint16 numModeRequestPorts;
    
    /* Mode Conditions */
    const BswM_ModeConditionType *modeConditions;
    uint16 numModeConditions;
    
    /* Expressions */
    const BswM_ExpressionStructType *expressions;
    uint16 numExpressions;
    
    /* General */
    boolean modeRequestProcessingEnabled;
    boolean rulesProcessingEnabled;
} BswM_ConfigType;

/******************************************************************************
 * BswM Status Type
 ******************************************************************************/
typedef struct {
    bool initialized;
    BswM_RuleStateType *ruleStates;
    uint16 numActiveRules;
    boolean deferredProcessingPending;
} BswM_StatusType;

/******************************************************************************
 * BswM Callback Function Types
 ******************************************************************************/
typedef void (*BswM_RuleNotificationCallback)(
    BswM_RuleIdType ruleId,
    BswM_RuleStateType state
);

typedef void (*BswM_ModeSwitchCallback)(
    BswM_ModeIdType modeId,
    BswM_ModeType mode
);

/******************************************************************************
 * External Variables
 ******************************************************************************/
extern const BswM_ConfigType *BswM_ConfigPtr;
extern BswM_StatusType BswM_Status;
extern BswM_RuleStateType BswM_RuleStates[];
extern BswM_ModeType BswM_ModeValues[];

/******************************************************************************
 * BswM Core API Functions
 ******************************************************************************/

/**
 * @brief Initialize BswM module
 * @param config Pointer to BswM configuration
 */
void BswM_Init(const BswM_ConfigType *config);

/**
 * @brief Deinitialize BswM module
 */
void BswM_Deinit(void);

/**
 * @brief Main function - process rules and modes
 */
void BswM_MainFunction(void);

/******************************************************************************
 * Mode Request Functions
 ******************************************************************************/

/**
 * @brief Request mode from mode request port
 * @param portType Type of mode request port
 * @param mode Requested mode
 */
void BswM_RequestMode(
    BswM_ModeRequestPortType portType,
    BswM_ModeType mode
);

/**
 * @brief Generic mode request
 * @param requestingModule Module requesting mode
 * @param mode Requested mode
 */
void BswM_ModeRequest(
    BswM_UserType requestingModule,
    BswM_ModeType mode
);

/******************************************************************************
 * Mode Switch Functions
 ******************************************************************************/

/**
 * @brief Switch mode
 * @param modeId Mode ID to switch
 * @param mode New mode value
 */
void BswM_SwitchMode(
    BswM_ModeIdType modeId,
    BswM_ModeType mode
);

/**
 * @brief Get current mode
 * @param modeId Mode ID to query
 * @return Current mode value
 */
BswM_ModeType BswM_GetMode(BswM_ModeIdType modeId);

/******************************************************************************
 * Rule Functions
 ******************************************************************************/

/**
 * @brief Evaluate specific rule
 * @param ruleId Rule ID to evaluate
 * @return Rule state after evaluation
 */
BswM_RuleStateType BswM_EvaluateRule(BswM_RuleIdType ruleId);

/**
 * @brief Execute rule's action list
 * @param ruleId Rule ID
 * @param state State triggering execution (TRUE/FALSE)
 */
void BswM_ExecuteRuleActionList(
    BswM_RuleIdType ruleId,
    BswM_RuleStateType state
);

/**
 * @brief Rule notification callback
 * @param ruleId Rule ID
 * @param state New rule state
 */
void BswM_RuleNotification(
    BswM_RuleIdType ruleId,
    BswM_RuleStateType state
);

/******************************************************************************
 * Action List Functions
 ******************************************************************************/

/**
 * @brief Execute action list
 * @param actionListId Action list ID to execute
 * @return BSWM_E_OK if successful
 */
BswM_ReturnType BswM_ExecuteActionList(BswM_ActionListIdType actionListId);

/**
 * @brief Execute specific action
 * @param action Pointer to action item
 * @return BSWM_E_OK if successful
 */
BswM_ReturnType BswM_ExecuteAction(const BswM_ActionItemType *action);

/******************************************************************************
 * Mode Request Port Functions
 ******************************************************************************/

/**
 * @brief Update mode request port
 * @param portId Port ID to update
 * @param mode New mode value
 */
void BswM_UpdateModeRequestPort(
    uint16 portId,
    BswM_ModeType mode
);

/**
 * @brief Get mode request port value
 * @param portId Port ID to query
 * @return Current mode value
 */
BswM_ModeType BswM_GetModeRequestPortValue(uint16 portId);

/******************************************************************************
 * DCM Integration Functions
 ******************************************************************************/

/**
 * @brief DCM session mode request
 * @param sessionMode Requested DCM session mode
 */
void BswM_Dcm_RequestSessionMode(uint8 sessionMode);

/**
 * @brief DCM reset mode request
 * @param resetMode Requested reset mode
 */
void BswM_Dcm_RequestResetMode(uint8 resetMode);

/**
 * @brief DCM communication mode current state
 * @param channel Network channel
 * @param comMode Communication mode
 */
void BswM_Dcm_CommunicationMode_CurrentState(
    uint8 channel,
    uint8 comMode
);

/**
 * @brief DCM application update
 * @param updateState Update state
 */
void BswM_Dcm_ApplicationUpdated(boolean updateState);

/******************************************************************************
 * ComM Integration Functions
 ******************************************************************************/

/**
 * @brief ComM current mode indication
 * @param channel Network channel
 * @param comMode Current communication mode
 */
void BswM_ComM_CurrentMode(
    uint8 channel,
    ComM_ModeType comMode
);

/**
 * @brief ComM PNC request
 * @param pnc PNC handle
 * @param comMode Requested mode
 */
void BswM_ComM_RequestMode(
    PNCHandleType pnc,
    ComM_ModeType comMode
);

/******************************************************************************
 * EcuM Integration Functions
 ******************************************************************************/

/**
 * @brief EcuM current state indication
 * @param state Current ECU state
 */
void BswM_EcuM_CurrentState(EcuM_StateType state);

/**
 * @brief EcuM wakeup status
 * @param source Wakeup source
 * @param status Wakeup status
 */
void BswM_EcuM_WakeupStatus(
    EcuM_WakeupSourceType source,
    EcuM_WakeupStatusType status
);

/******************************************************************************
 * NVM Integration Functions
 ******************************************************************************/

/**
 * @brief NVM job mode indication
 * @param blockId NVM block ID
 * @param jobMode NVM job mode
 */
void BswM_NvM_CurrentJobMode(
    uint16 blockId,
    uint8 jobMode
);

/******************************************************************************
 * Bus Manager Integration Functions
 ******************************************************************************/

/**
 * @brief CanSM current state
 * @param channel CanSM channel
 * @param state Current state
 */
void BswM_CanSM_CurrentState(
    uint8 channel,
    uint8 state
);

/**
 * @brief LinSM current state
 * @param channel LinSM channel
 * @param state Current state
 */
void BswM_LinSM_CurrentState(
    uint8 channel,
    uint8 state
);

/**
 * @brief LinSM schedule indication
 * @param channel LinSM channel
 * @param schedule Schedule ID
 */
void BswM_LinSM_CurrentSchedule(
    uint8 channel,
    uint8 schedule
);

/**
 * @brief FrSM current state
 * @param channel FrSM channel
 * @param state Current state
 */
void BswM_FrSM_CurrentState(
    uint8 channel,
    uint8 state
);

/**
 * @brief EthSM current state
 * @param channel EthSM channel
 * @param state Current state
 */
void BswM_EthSM_CurrentState(
    uint8 channel,
    uint8 state
);

/******************************************************************************
 * Service Discovery Functions
 ******************************************************************************/

/**
 * @brief SD client service mode
 * @param serviceHandleId Service handle ID
 * @param mode Service mode
 */
void BswM_SD_ClientServiceMode(
    uint16 serviceHandleId,
    uint8 mode
);

/**
 * @brief SD consumed event group mode
 * @param consumedEventGroupHandleId Event group handle ID
 * @param mode Event group mode
 */
void BswM_SD_ConsumedEventGroupMode(
    uint16 consumedEventGroupHandleId,
    uint8 mode
);

/**
 * @brief SD event handler mode
 * @param eventHandlerHandleId Event handler ID
 * @param mode Event handler mode
 */
void BswM_SD_EventHandlerMode(
    uint16 eventHandlerHandleId,
    uint8 mode
);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get module status
 * @return Pointer to status structure
 */
const BswM_StatusType* BswM_GetStatus(void);

/**
 * @brief Check if module is initialized
 * @return TRUE if initialized
 */
boolean BswM_IsInitialized(void);

/**
 * @brief Get version information
 * @param version Pointer to version structure
 */
void BswM_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* BSWM_H */

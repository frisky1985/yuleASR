/**
 * @file BswM.h
 * @brief BSW Mode Manager - Rule/ActionList Engine - AUTOSAR Service Module
 * @version 2.0.0
 * @date 2026-09-26
 * @author YuleTech
 *
 * @implements AUTOSAR_SWS_BSWModeManager.pdf
 *
 * Pre-compile configuration: BswM_Init(NULL_PTR) selects the default
 * configuration object (BswM_Config) provided by BswM_Lcfg.c.
 */

#ifndef BSWM_H
#define BSWM_H

#include "Std_Types.h"
#include "BswM_Cfg.h"
#include "ModuleId.h"

#define BSWM_AR_RELEASE_MAJOR_VERSION   4U
#define BSWM_AR_RELEASE_MINOR_VERSION   4U
#define BSWM_AR_RELEASE_REVISION_VERSION 0U
#define BSWM_SW_MAJOR_VERSION           2U
#define BSWM_SW_MINOR_VERSION           0U
#define BSWM_SW_PATCH_VERSION           0U
#define BSWM_MODULE_ID              MODULE_ID_BSWM   /* Fixed: was 0x12, conflict with ComM — see ModuleId.h */
#define BSWM_VENDOR_ID              0x0055U

/* Service IDs (DET) */
#define BSWM_SID_INIT               0x00U
#define BSWM_SID_DEINIT             0x01U
#define BSWM_SID_MAINFUNCTION       0x02U
#define BSWM_SID_REQUEST_MODE       0x03U
#define BSWM_SID_GET_CURRENT_MODE   0x04U
#define BSWM_SID_GET_REQUESTED_MODE 0x05U
#define BSWM_SID_GETVERSIONINFO     0x06U
#define BSWM_SID_SWITCH_MODE        0x07U

/* Development error codes */
#define BSWM_E_PARAM_POINTER        0x10U
#define BSWM_E_UNINIT               0x20U
#define BSWM_E_PARAM_MODE           0x30U
#define BSWM_E_MODE_REQUEST_REJECT  0x40U

/* SwCompositionId of the mode request ports fed by EcuM notifications */
#define BSWM_ECUM_REQUEST           0x01U
#define BSWM_COMM_REQUEST           0x02U
#define BSWM_DCM_REQUEST            0x03U
#define BSWM_NM_REQUEST             0x04U
#define BSWM_SCHM_REQUEST           0x05U

/* Mode values */
#define BSWM_MODE_VALUE_OFF         0U
#define BSWM_MODE_VALUE_START       1U
#define BSWM_MODE_VALUE_RUN         2U
#define BSWM_MODE_VALUE_POST_RUN    3U
#define BSWM_MODE_VALUE_SLEEP       4U
#define BSWM_MODE_VALUE_SHUTDOWN    5U
#define BSWM_MODE_VALUE_WAKEUP      6U
#define BSWM_MODE_VALUE_STARTUP     7U
#define BSWM_MODE_VALUE_MAX         BSWM_MODE_VALUE_STARTUP

typedef uint8 BswM_ModeType;

/* Rule evaluation result */
#define BSWM_RULE_STATE_FALSE       0U
#define BSWM_RULE_STATE_TRUE        1U
typedef uint8 BswM_RuleStateType;

/* Expression types */
#define BSWM_EXPR_MODE_EQUALS       0U
#define BSWM_EXPR_MODE_NOT_EQUALS   1U
#define BSWM_EXPR_LOGICAL_AND       2U
#define BSWM_EXPR_LOGICAL_OR        3U
#define BSWM_EXPR_LOGICAL_NOT       4U
#define BSWM_EXPR_IDX_NONE          0xFFFFU
#define BSWM_ACTION_LIST_NONE       0xFFFFU

/**
 * @brief Mode request port: source of a requested mode for rule conditions.
 *
 * The port value is the latest mode requested by its SwCompositionId (via
 * BswM_RequestMode or the BswM_EcuM_* notifications). Expressions compare
 * that value against BswM_ExpressionConfigType::CompareValue.
 */
typedef uint8 BswM_ModeRequestPortType;

/**
 * @brief Expression tree node (flat table, referenced by index).
 *
 * Leaf nodes (MODE_EQUALS / MODE_NOT_EQUALS) use PortIndex + CompareValue.
 * Logical nodes use LeftIndex/RightIndex (NOT uses LeftIndex only).
 */
typedef struct {
    uint8          ExpressionType;
    uint8          PortIndex;
    BswM_ModeType  CompareValue;
    uint16         LeftIndex;
    uint16         RightIndex;
} BswM_ExpressionConfigType;

/**
 * @brief Rule: condition expression + true/false transition action lists.
 *
 * Action lists execute on state transitions only (FALSE->TRUE runs
 * TrueActionListIndex, TRUE->FALSE runs FalseActionListIndex).
 */
typedef struct {
    uint8    RuleId;
    uint16   ConditionIndex;
    uint16   TrueActionListIndex;
    uint16   FalseActionListIndex;
    uint8    InitialState;
    boolean  IsEnabled;
} BswM_RuleType;

/**
 * @brief Action: callback invoked with a mode parameter.
 */
typedef void (*BswM_ActionCallback)(BswM_ModeType Mode);

typedef struct {
    BswM_ActionCallback Callback;
    BswM_ModeType       Parameter;
} BswM_ActionType;

typedef struct {
    uint8                 NumActions;
    const BswM_ActionType* Actions;
} BswM_ActionListType;

typedef struct {
    uint8          NumModeRequestPorts;
    const BswM_ModeRequestPortType* ModeRequestPorts;
    uint16         NumExpressions;
    const BswM_ExpressionConfigType* Expressions;
    uint8          NumRules;
    const BswM_RuleType* Rules;
    uint8          NumActionLists;
    const BswM_ActionListType* ActionLists;
} BswM_ConfigType;

/** Default (pre-compile) configuration object, provided by BswM_Lcfg.c */
extern const BswM_ConfigType BswM_Config;

/** @req SWS_BswM_00001 */
void BswM_Init(const BswM_ConfigType* ConfigPtr);
/** @req SWS_BswM_00002 */
void BswM_DeInit(void);
/** @req SWS_BswM_00010 */
Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode);
/** @req SWS_BswM_00011 */
BswM_ModeType BswM_GetCurrentMode(void);
/** @req SWS_BswM_00012 */
BswM_ModeType BswM_GetRequestedMode(void);
/** @req SWS_BswM_00020 */
void BswM_MainFunction(void);
/** @req SWS_BswM_00030 */
void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Built-in action: switch the arbitrated mode (usable in action lists).
 * @param Mode Target mode; applied to current mode immediately.
 */
void BswM_ActionSwitchMode(BswM_ModeType Mode);

/**
 * @brief Parameter types of the EcuM notifications.
 *
 * Interchangeable with EcuM_StateType / EcuM_WakeupSourceType /
 * EcuM_WakeupStatusType; mirrored here so that including BswM.h does not
 * require EcuM's include path (same convention as CanIf.h).
 */
typedef uint8  BswM_EcuMStateType;
typedef uint32 BswM_EcuMWakeupSourceType;
typedef uint8  BswM_EcuMWakeupStatusType;

/** @req SWS_BswM_00210 EcuM notification: ECU state changed */
void BswM_EcuM_CurrentState(BswM_EcuMStateType CurrentState);
/** @req SWS_BswM_00211 EcuM notification: wakeup source status changed */
void BswM_EcuM_CurrentWakeup(BswM_EcuMWakeupSourceType WakeupSource,
                             BswM_EcuMWakeupStatusType WakeupStatus);

#endif /* BSWM_H */

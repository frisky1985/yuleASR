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
#define BSWM_SW_MINOR_VERSION           1U
#define BSWM_SW_PATCH_VERSION           0U
#define BSWM_MODULE_ID              MODULE_ID_BSWM   /* Fixed: was 0x12, conflict with ComM — see ModuleId.h */
#define BSWM_VENDOR_ID              0x0055U

/* Service IDs (DET) */
#define BSWM_SID_INIT                    0x00U
#define BSWM_SID_DEINIT                  0x01U
#define BSWM_SID_MAINFUNCTION            0x02U
#define BSWM_SID_REQUEST_MODE            0x03U
#define BSWM_SID_GET_CURRENT_MODE        0x04U
#define BSWM_SID_GET_REQUESTED_MODE      0x05U
#define BSWM_SID_GETVERSIONINFO          0x06U
#define BSWM_SID_SWITCH_MODE             0x07U
#define BSWM_SID_COMM_CURRENT_MODE       0x08U
#define BSWM_SID_COMM_CURRENT_PNC_MODE   0x09U
#define BSWM_SID_DCM_COMM_MODE           0x0AU
#define BSWM_SID_DCM_APP_UPDATED         0x0BU
#define BSWM_SID_NM_STATE_CHANGE         0x0CU
#define BSWM_SID_NM_CAR_WAKEUP           0x0DU
#define BSWM_SID_CANSM_CURRENT_STATE     0x0EU
#define BSWM_SID_ETHSM_CURRENT_STATE     0x0FU
#define BSWM_SID_FRSM_CURRENT_STATE      0x10U
#define BSWM_SID_LINSM_CURRENT_STATE     0x11U
#define BSWM_SID_LINSM_CURRENT_SCHEDULE  0x12U
#define BSWM_SID_LINTP_REQUEST_MODE      0x13U
#define BSWM_SID_ECUM_REQUESTED_STATE    0x14U
#define BSWM_SID_PARTITION_RESTARTED     0x15U
#define BSWM_SID_ETHIF_PORTGROUP_LINKSTATE 0x16U
#define BSWM_SID_COMM_CURRENT_PDU_GROUP 0x17U
#define BSWM_SID_RULE_ENABLE            0x18U
#define BSWM_SID_RULE_DISABLE           0x19U

/* Development error codes */
#define BSWM_E_PARAM_POINTER        0x10U
#define BSWM_E_UNINIT               0x20U
#define BSWM_E_PARAM_MODE           0x30U
#define BSWM_E_MODE_REQUEST_REJECT  0x40U
#define BSWM_E_PARAM_RULE_ID        0x50U

/* SwCompositionId of the mode request ports fed by BSW module notifications */
#define BSWM_ECUM_REQUEST           0x01U
#define BSWM_COMM_REQUEST           0x02U
#define BSWM_DCM_REQUEST            0x03U
#define BSWM_NM_REQUEST             0x04U
#define BSWM_SCHM_REQUEST           0x05U
#define BSWM_CANSM_REQUEST          0x06U
#define BSWM_ETHSM_REQUEST          0x07U
#define BSWM_FRSM_REQUEST           0x08U
#define BSWM_LINSM_REQUEST          0x09U
#define BSWM_COMM_PNC_REQUEST       0x0AU
#define BSWM_ECUM_REQUESTED         0x0BU
#define BSWM_PARTITION_REQUEST      0x0CU
#define BSWM_ETHIF_REQUEST          0x0DU
#define BSWM_NM_CARWAKEUP_REQUEST   0x0EU
#define BSWM_DCM_APPUPDATED_REQUEST 0x0FU
#define BSWM_LINSM_SCHEDULE_REQUEST 0x10U
#define BSWM_LINTP_REQUEST          0x11U

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
 *
 * Priority arbitrates the per-cycle evaluation order in BswM_MainFunction:
 * rules with a lower Priority value are evaluated (and their action lists
 * fired) first, 0 = highest priority. Equal priorities keep the table order.
 * The field is appended for backward compatibility: configurations that do
 * not set it default to the highest priority and keep the legacy order.
 */
typedef struct {
    uint8    RuleId;
    uint16   ConditionIndex;
    uint16   TrueActionListIndex;
    uint16   FalseActionListIndex;
    uint8    InitialState;
    boolean  IsEnabled;
    uint8    Priority;      /**< 0 = highest, evaluated first */
} BswM_RuleType;

/**
 * @brief Action: callback invoked with a mode parameter.
 */
typedef void (*BswM_ActionCallback)(BswM_ModeType Mode);

/**
 * @brief Extended action kinds dispatched by BswM_ExecuteActionList().
 *
 * BSWM_ACTION_CALLBACK is the legacy behavior (invoke Callback with
 * Parameter) and is the default for every configuration that leaves Kind
 * zero-initialized, keeping existing tables source-compatible.
 */
typedef enum {
    BSWM_ACTION_CALLBACK         = 0U,  /**< Legacy callback action          */
    BSWM_ACTION_PDU_GROUP_SWITCH = 1U,  /**< Enable/disable PDU group bits   */
    BSWM_ACTION_COM_IPDU_GROUP   = 2U,  /**< Com IPDU group absolute control */
    BSWM_ACTION_ECUM_GO_SLEEP    = 3U,  /**< EcuM_GoSleep                    */
    BSWM_ACTION_ECUM_GO_HALT     = 4U,  /**< EcuM_GoHalt                     */
    BSWM_ACTION_ECUM_GO_OFF      = 5U   /**< Shutdown target OFF + EcuM_Shutdown */
} BswM_ActionKindType;

/**
 * @brief Mirrored PDU group vector of an action (bit i = IPDU group i).
 *
 * Sized independently of Com_Cfg (supports up to 32 groups) so BswM config
 * tables do not depend on the Com group count; BswM clamps the width to
 * sizeof(Com_IpduGroupVector) when calling Com_IpduGroupControl().
 */
#define BSWM_ACTION_VECTOR_MAX_BYTES    4U

/**
 * @brief Mirror of Com's Com_IpduGroupVector (byte count for
 * COM_NUM_IPDU_GROUPS = 16), used by BswM_Com_CurrentPduGroupState().
 *
 * Mirrored instead of included because this tree carries a second,
 * API-less Com.h stub (include/autosar/classic/com) whose COM_H guard
 * shadows the real service header depending on target include order. The
 * type is binary-compatible with the real Com_IpduGroupVector; BswM.c
 * clamps the width at the Com_IpduGroupControl() call boundary.
 */
typedef uint8 BswM_Com_IpduGroupVector[(16U + 7U) / 8U];

/**
 * @brief Action entry of an action list.
 *
 * The extended payload fields are appended after the legacy pair so existing
 * positional initializers ( { Callback, Parameter } ) keep compiling with
 * Kind = BSWM_ACTION_CALLBACK.
 */
typedef struct {
    BswM_ActionCallback Callback;   /**< Kind == BSWM_ACTION_CALLBACK            */
    BswM_ModeType       Parameter;  /**< Callback mode / GO_OFF shutdown mode    */
    BswM_ActionKindType Kind;       /**< Action kind selector                    */
    boolean             Enable;     /**< PDU_GROUP_SWITCH: TRUE = enable groups  */
    boolean             Initialize; /**< COM_IPDU_GROUP: Initialize flag for Com */
    uint8               IpduGroupVector[BSWM_ACTION_VECTOR_MAX_BYTES]; /**< group bitmask, LSB = group 0 */
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
/**
 * @brief Initialize the module
 * @param[in] ConfigPtr Configuration reference
 */
void BswM_Init(const BswM_ConfigType* ConfigPtr);
/** @req SWS_BswM_00002 */
/**
 * @brief De-initialize the module
 * @param[in] ConfigPtr Configuration reference
 */
void BswM_DeInit(void);
/** @req SWS_BswM_00010 */
/**
 * @brief Request operation
 * @param[in] SwCompositionId Identifier
 * @param[in] Mode Operation mode
 * @return Operation status
 */
Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode);
/** @req SWS_BswM_00011 */
/**
 * @brief Get requested information
 * @param[in] SwCompositionId Identifier
 * @param[in] Mode Operation mode
 * @return Operation result
 */
BswM_ModeType BswM_GetCurrentMode(void);
/** @req SWS_BswM_00012 */
/**
 * @brief Get requested information
 * @param[in] SwCompositionId Identifier
 * @param[in] Mode Operation mode
 * @return Operation result
 */
BswM_ModeType BswM_GetRequestedMode(void);
/** @req SWS_BswM_00020 */
/**
 * @brief Process periodic tasks
 * @param[in] SwCompositionId Identifier
 * @param[in] Mode Operation mode
 */
void BswM_MainFunction(void);
/** @req SWS_BswM_00030 */
/**
 * @brief Get module version information
 * @param[in] versioninfo Version info
 */
void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Built-in action: switch the arbitrated mode (usable in action lists).
 * @param Mode Target mode; applied to current mode immediately.
 */
void BswM_ActionSwitchMode(BswM_ModeType Mode);

/**
 * @brief Enable a rule at runtime (undoes BswM_RuleDisable).
 * @param RuleId Index of the rule in the configuration rule table.
 * @return E_OK on success, E_NOT_OK when uninitialized or RuleId is invalid
 *         (DET: BSWM_E_UNINIT / BSWM_E_PARAM_RULE_ID).
 */
Std_ReturnType BswM_RuleEnable(uint16 RuleId);

/**
 * @brief Disable a rule at runtime; disabled rules are skipped by
 *        BswM_MainFunction until re-enabled.
 * @param RuleId Index of the rule in the configuration rule table.
 * @return E_OK on success, E_NOT_OK when uninitialized or RuleId is invalid
 *         (DET: BSWM_E_UNINIT / BSWM_E_PARAM_RULE_ID).
 */
/**
 * @brief rule disable
 * @param[in] RuleId Identifier
 * @return Operation status
 */
Std_ReturnType BswM_RuleDisable(uint16 RuleId);

/**
 * @brief Query the PDU group state last applied by BswM PDU group actions.
 * @param IpduGroupVector Out: bit i = group i enabled state (LSB = group 0).
 * @return E_OK on success, E_NOT_OK when uninitialized or the pointer is
 *         NULL (DET: BSWM_E_UNINIT / BSWM_E_PARAM_POINTER).
 */
Std_ReturnType BswM_Com_CurrentPduGroupState(BswM_Com_IpduGroupVector IpduGroupVector);

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

/**
 * @brief Parameter types of the mode manager notification callbacks.
 *
 * Mirrored from ComM / Dcm / Nm / CanSM / EthSM / FrSM / LinSM / LinTp and
 * the OS-Application domain so that including BswM.h does not require those
 * modules' include paths (same convention as BswM_EcuMStateType). Values are
 * written to the mode request ports unchanged (raw module state semantics);
 * they are NOT restricted to the BswM_RequestMode() arbitration mode domain.
 */
typedef uint8  BswM_ComMModeType;
typedef uint8  BswM_DcmCommunicationModeType;
typedef uint8  BswM_NmStateType;
typedef uint8  BswM_CanSmStateType;
typedef uint8  BswM_EthSmStateType;
typedef uint8  BswM_FrSmStateType;
typedef uint8  BswM_LinSmStateType;
typedef uint8  BswM_LinTpModeType;
typedef uint16 BswM_ApplicationType;

/*
 * NetworkHandleType: the full AUTOSAR ComStack_Types.h (RTE variant) provides
 * it; the stub variant reachable in this build tree does not. Mirror the type
 * locally so BswM.h stays self-contained and binary-compatible either way.
 * (The Com_IpduGroupVector mirror lives next to the action payload types.)
 */
#include "ComStack_Types.h"
typedef uint8 BswM_NetworkHandleType;

/** @req SWS_BswM_00210 EcuM notification: ECU state changed */
/**
 * @brief ecu m_ current state
 * @param[in] CurrentState State value
 */
void BswM_EcuM_CurrentState(BswM_EcuMStateType CurrentState);
/** @req SWS_BswM_00211 EcuM notification: wakeup source status changed */
/**
 * @brief ecu m_ current wakeup
 * @param[in] WakeupSource WakeupSource value
 * @param[in] WakeupStatus State value
 */
void BswM_EcuM_CurrentWakeup(BswM_EcuMWakeupSourceType WakeupSource,
                             BswM_EcuMWakeupStatusType WakeupStatus);

/** ComM notification: current communication mode of a network */
/**
 * @brief com m_ current mode
 * @param[in] Network Network value
 * @param[in] CurrentMode Operation mode
 */
void BswM_ComM_CurrentMode(BswM_NetworkHandleType Network, BswM_ComMModeType CurrentMode);
/** ComM notification: current PNC mode (IsPNCActive) of a network */
/**
 * @brief com m_ current p n c mode
 * @param[in] Network Network value
 * @param[in] IsPNCActive IsPNCActive value
 */
void BswM_ComM_CurrentPNCMode(BswM_NetworkHandleType Network, boolean IsPNCActive);
/** Dcm notification: current communication mode request state of a network */
/**
 * @brief dcm_ communication mode_ current state
 * @param[in] Network Network value
 * @param[in] RequestedCommunicationMode Operation mode
 */
void BswM_Dcm_CommunicationMode_CurrentState(BswM_NetworkHandleType Network,
                                             BswM_DcmCommunicationModeType RequestedCommunicationMode);
/** Dcm notification: application updated indication (fires BswM rules with fixed value 1) */
/**
 * @brief dcm_ application updated
 * @param[in] Network Network value
 */
void BswM_Dcm_ApplicationUpdated(BswM_NetworkHandleType Network);
/** Nm notification: network management state changed */
/**
 * @brief nm_ state change notification
 * @param[in] Network Network value
 * @param[in] State State value
 */
void BswM_Nm_StateChangeNotification(BswM_NetworkHandleType Network, BswM_NmStateType State);
/** Nm notification: car wakeup indication (fires BswM rules with fixed value 1) */
/**
 * @brief nm_ car wake up indication
 * @param[in] Network Network value
 */
void BswM_Nm_CarWakeUpIndication(BswM_NetworkHandleType Network);
/** CanSM notification: current CAN network mode */
/**
 * @brief can s m_ current state
 * @param[in] Network Network value
 * @param[in] CurrentState State value
 */
void BswM_CanSM_CurrentState(BswM_NetworkHandleType Network, BswM_CanSmStateType CurrentState);
/** EthSM notification: current Ethernet network mode */
/**
 * @brief eth s m_ current state
 * @param[in] Network Network value
 * @param[in] CurrentState State value
 */
void BswM_EthSM_CurrentState(BswM_NetworkHandleType Network, BswM_EthSmStateType CurrentState);
/** FrSM notification: current FlexRay network mode */
/**
 * @brief fr s m_ current state
 * @param[in] Network Network value
 * @param[in] CurrentState State value
 */
void BswM_FrSM_CurrentState(BswM_NetworkHandleType Network, BswM_FrSmStateType CurrentState);
/** LinSM notification: current LIN network mode */
/**
 * @brief lin s m_ current state
 * @param[in] Network Network value
 * @param[in] CurrentState State value
 */
void BswM_LinSM_CurrentState(BswM_NetworkHandleType Network, BswM_LinSmStateType CurrentState);
/** LinSM notification: current LIN schedule table */
/**
 * @brief lin s m_ current schedule
 * @param[in] Network Network value
 * @param[in] ScheduleIndex Index value
 */
void BswM_LinSM_CurrentSchedule(BswM_NetworkHandleType Network, uint8 ScheduleIndex);
/** LinTp notification: requested LIN TP mode */
/**
 * @brief lin tp_ request mode
 * @param[in] Network Network value
 * @param[in] LinTpMode Operation mode
 */
void BswM_LinTp_RequestMode(BswM_NetworkHandleType Network, BswM_LinTpModeType LinTpMode);
/** EcuM notification: requested ECU state (mapped to the BswM mode domain first) */
/**
 * @brief ecu m_ requested state
 * @param[in] State State value
 */
void BswM_EcuM_RequestedState(BswM_EcuMStateType State);
/** BswM notification: partition restarted (writes the partition id low byte) */
/**
 * @brief bsw m partition restarted
 * @param[in] Application Application value
 */
void BswM_BswMPartitionRestarted(BswM_ApplicationType Application);
/** EthIf notification: port group link state changed */
/**
 * @brief eth if_ port group link state chg
 * @param[in] PortGroup Group identifier
 * @param[in] LinkState State value
 */
void BswM_EthIf_PortGroupLinkStateChg(uint8 PortGroup, boolean LinkState);


#endif /* BSWM_H */

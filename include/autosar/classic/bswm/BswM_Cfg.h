/******************************************************************************
 * @file    BswM_Cfg.h
 * @brief   BSW Mode Manager (BswM) Configuration Header
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef BSWM_CFG_H
#define BSWM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bswm.h"

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define BSWM_CFG_VENDOR_ID              0x01U
#define BSWM_CFG_MODULE_ID              0x0DU
#define BSWM_CFG_SW_MAJOR_VERSION       1U
#define BSWM_CFG_SW_MINOR_VERSION       0U
#define BSWM_CFG_SW_PATCH_VERSION       0U

/******************************************************************************
 * DDS Integration Feature Switches
 ******************************************************************************/
#define BSWM_DDS_ENABLED                STD_ON
#define BSWM_DDS_RULES_ENABLED          STD_ON
#define BSWM_DDS_MODE_REQUEST_ENABLED   STD_ON
#define BSWM_DDS_NETWORK_CONTROL        STD_ON
#define BSWM_DDS_PUBLISHER_CONTROL      STD_ON
#define BSWM_DDS_SUBSCRIBER_CONTROL     STD_ON

/******************************************************************************
 * EcuM Integration Configuration
 ******************************************************************************/
#define BSWM_ECUM_ENABLED               STD_ON
#define BSWM_ECUM_STATE_RULES_ENABLED   STD_ON
#define BSWM_ECUM_WKUP_RULES_ENABLED    STD_ON

/******************************************************************************
 * WdgM Integration Configuration
 ******************************************************************************/
#define BSWM_WDGM_ENABLED               STD_ON
#define BSWM_WDGM_MODE_SWITCH_ENABLED   STD_ON
#define BSWM_WDGM_SAFE_STATE_ENABLED    STD_ON

/******************************************************************************
 * SoAd/PduR Integration Configuration
 ******************************************************************************/
#define BSWM_SOAD_ENABLED               STD_ON
#define BSWM_SOAD_ROUTING_CONTROL       STD_ON
#define BSWM_PDUR_ENABLED               STD_ON
#define BSWM_PDUR_ROUTING_CONTROL       STD_ON

/******************************************************************************
 * DCM Integration Configuration
 ******************************************************************************/
#define BSWM_DCM_ENABLED                STD_ON
#define BSWM_DCM_SESSION_RULES          STD_ON
#define BSWM_DCM_RESET_RULES            STD_ON

/******************************************************************************
 * Mode Request Port IDs for DDS
 ******************************************************************************/
/* DDS Related Mode Request Ports */
#define BSWM_MRP_DDS_COMMUNICATION_STATE    0x10U
#define BSWM_MRP_DDS_NETWORK_STATUS         0x11U
#define BSWM_MRP_DDS_DISCOVERY_STATE        0x12U
#define BSWM_MRP_DDS_PUBLISHER_STATUS       0x13U
#define BSWM_MRP_DDS_SUBSCRIBER_STATUS      0x14U
#define BSWM_MRP_DDS_SECURITY_STATUS        0x15U
#define BSWM_MRP_DDS_QOS_VIOLATION          0x16U
#define BSWM_MRP_DDS_RELIABILITY_STATUS     0x17U

/* Extended Mode Request Ports */
#define BSWM_MRP_SOAD_SOCKET_STATE          0x20U
#define BSWM_MRP_SOAD_ROUTING_GROUP_STATE   0x21U
#define BSWM_MRP_PDUR_ROUTING_STATE         0x22U

/******************************************************************************
 * DDS Communication States
 ******************************************************************************/
typedef enum {
    DDS_COMM_STATE_INACTIVE = 0,
    DDS_COMM_STATE_INITIALIZING,
    DDS_COMM_STATE_ACTIVE,
    DDS_COMM_STATE_DEGRADED,
    DDS_COMM_STATE_ERROR,
    DDS_COMM_STATE_SHUTDOWN
} BswM_DdsCommunicationStateType;

/******************************************************************************
 * DDS Network Status
 ******************************************************************************/
typedef enum {
    DDS_NETWORK_DISCONNECTED = 0,
    DDS_NETWORK_CONNECTING,
    DDS_NETWORK_CONNECTED,
    DDS_NETWORK_RECONNECTING,
    DDS_NETWORK_ERROR
} BswM_DdsNetworkStatusType;

/******************************************************************************
 * DDS Discovery States
 ******************************************************************************/
typedef enum {
    DDS_DISCOVERY_INACTIVE = 0,
    DDS_DISCOVERY_PENDING,
    DDS_DISCOVERY_COMPLETE,
    DDS_DISCOVERY_FAILED,
    DDS_DISCOVERY_REFRESHING
} BswM_DdsDiscoveryStateType;

/******************************************************************************
 * DDS Publisher/Subscriber Status
 ******************************************************************************/
typedef enum {
    DDS_ENTITY_INACTIVE = 0,
    DDS_ENTITY_ACTIVE,
    DDS_ENTITY_SUSPENDED,
    DDS_ENTITY_ERROR
} BswM_DdsEntityStatusType;

/******************************************************************************
 * DDS Security Status
 ******************************************************************************/
typedef enum {
    DDS_SECURITY_UNENCRYPTED = 0,
    DDS_SECURITY_AUTHENTICATING,
    DDS_SECURITY_ENCRYPTED,
    DDS_SECURITY_VIOLATION,
    DDS_SECURITY_FAILED
} BswM_DdsSecurityStatusType;

/******************************************************************************
 * Rule IDs for DDS Scenarios
 ******************************************************************************/
typedef enum {
    /* System Startup Rules */
    BSWM_RULE_SYSTEM_STARTUP = 0,
    BSWM_RULE_SOAD_ROUTING_ENABLE,
    BSWM_RULE_DDS_INIT_COMPLETE,
    
    /* Network Connection Rules */
    BSWM_RULE_NETWORK_CONNECTED,
    BSWM_RULE_NETWORK_DISCONNECTED,
    BSWM_RULE_DDS_RECONNECT,
    
    /* WdgM Safety Rules */
    BSWM_RULE_WDGM_FAILURE,
    BSWM_RULE_WDGM_SAFE_MODE_ENTRY,
    BSWM_RULE_WDGM_SAFE_MODE_EXIT,
    
    /* DDS Communication Rules */
    BSWM_RULE_DDS_COMM_ACTIVATE,
    BSWM_RULE_DDS_COMM_DEACTIVATE,
    BSWM_RULE_DDS_PUBLISHER_ENABLE,
    BSWM_RULE_DDS_PUBLISHER_DISABLE,
    BSWM_RULE_DDS_SUBSCRIBER_ENABLE,
    BSWM_RULE_DDS_SUBSCRIBER_DISABLE,
    
    /* Discovery Management Rules */
    BSWM_RULE_DDS_DISCOVERY_START,
    BSWM_RULE_DDS_DISCOVERY_COMPLETE,
    BSWM_RULE_DDS_DISCOVERY_REFRESH,
    
    /* Security Rules */
    BSWM_RULE_DDS_SECURITY_VIOLATION,
    BSWM_RULE_DDS_SECURITY_RESTORE,
    
    /* Diagnostic Session Rules */
    BSWM_RULE_DCM_SESSION_DEFAULT,
    BSWM_RULE_DCM_SESSION_EXTENDED,
    BSWM_RULE_DCM_SESSION_PROGRAMMING,
    BSWM_RULE_DCM_SESSION_SAFE,
    
    /* SoAd/PduR Routing Rules */
    BSWM_RULE_SOAD_ENABLE_ROUTING,
    BSWM_RULE_SOAD_DISABLE_ROUTING,
    BSWM_RULE_PDUR_ENABLE_ROUTING,
    BSWM_RULE_PDUR_DISABLE_ROUTING,
    
    /* Shutdown Rules */
    BSWM_RULE_SYSTEM_SHUTDOWN_PREPARE,
    BSWM_RULE_SYSTEM_SHUTDOWN_EXECUTE,
    BSWM_RULE_DDS_SHUTDOWN,
    
    /* EcuM State Rules */
    BSWM_RULE_ECUM_STATE_STARTUP,
    BSWM_RULE_ECUM_STATE_RUN,
    BSWM_RULE_ECUM_STATE_SLEEP_PREP,
    BSWM_RULE_ECUM_STATE_SHUTDOWN,
    
    BSWM_MAX_DDS_RULES
} BswM_DdsRuleIdType;

/******************************************************************************
 * Action List IDs for DDS Scenarios
 ******************************************************************************/
typedef enum {
    /* Startup Action Lists */
    BSWM_ALIST_SYSTEM_STARTUP = 0,
    BSWM_ALIST_SOAD_ACTIVATE,
    BSWM_ALIST_DDS_INITIALIZE,
    
    /* Network Action Lists */
    BSWM_ALIST_NETWORK_CONNECT,
    BSWM_ALIST_NETWORK_DISCONNECT,
    BSWM_ALIST_DDS_RECONNECT,
    
    /* Safety Action Lists */
    BSWM_ALIST_ENTER_SAFE_MODE,
    BSWM_ALIST_EXIT_SAFE_MODE,
    BSWM_ALIST_WDGM_FAILURE_HANDLER,
    
    /* DDS Communication Action Lists */
    BSWM_ALIST_DDS_COMM_START,
    BSWM_ALIST_DDS_COMM_STOP,
    BSWM_ALIST_DDS_PUB_ENABLE,
    BSWM_ALIST_DDS_PUB_DISABLE,
    BSWM_ALIST_DDS_SUB_ENABLE,
    BSWM_ALIST_DDS_SUB_DISABLE,
    
    /* Discovery Action Lists */
    BSWM_ALIST_DISCOVERY_START,
    BSWM_ALIST_DISCOVERY_COMPLETE,
    BSWM_ALIST_DISCOVERY_REFRESH,
    
    /* Security Action Lists */
    BSWM_ALIST_SECURITY_VIOLATION_HANDLER,
    BSWM_ALIST_SECURITY_RESTORE,
    
    /* Diagnostic Action Lists */
    BSWM_ALIST_DCM_DEFAULT_SESSION,
    BSWM_ALIST_DCM_EXTENDED_SESSION,
    BSWM_ALIST_DCM_PROGRAMMING_SESSION,
    BSWM_ALIST_DCM_SAFE_SESSION,
    
    /* Routing Action Lists */
    BSWM_ALIST_SOAD_ENABLE_ROUTING,
    BSWM_ALIST_SOAD_DISABLE_ROUTING,
    BSWM_ALIST_PDUR_ENABLE_ROUTING,
    BSWM_ALIST_PDUR_DISABLE_ROUTING,
    
    /* Shutdown Action Lists */
    BSWM_ALIST_SYSTEM_SHUTDOWN,
    BSWM_ALIST_DDS_SHUTDOWN,
    
    BSWM_MAX_DDS_ACTION_LISTS
} BswM_DdsActionListIdType;

/******************************************************************************
 * Mode IDs for DDS
 ******************************************************************************/
typedef enum {
    BSWM_MODE_DDS_COMMUNICATION = 0,
    BSWM_MODE_DDS_NETWORK,
    BSWM_MODE_DDS_DISCOVERY,
    BSWM_MODE_DDS_PUBLISHER,
    BSWM_MODE_DDS_SUBSCRIBER,
    BSWM_MODE_DDS_SECURITY,
    BSWM_MODE_SYSTEM_STATE,
    BSWM_MODE_DIAGNOSTIC_SESSION,
    BSWM_MAX_DDS_MODES
} BswM_DdsModeIdType;

/******************************************************************************
 * System State Modes
 ******************************************************************************/
typedef enum {
    BSWM_SYSTEM_STATE_INIT = 0,
    BSWM_SYSTEM_STATE_STARTUP,
    BSWM_SYSTEM_STATE_NORMAL,
    BSWM_SYSTEM_STATE_DEGRADED,
    BSWM_SYSTEM_STATE_SAFE,
    BSWM_SYSTEM_STATE_SHUTDOWN
} BswM_SystemStateModeType;

/******************************************************************************
 * Configuration Structure Declaration
 ******************************************************************************/
extern const BswM_ConfigType BswM_DdsConfig;

/******************************************************************************
 * Mode Request Port Configuration
 ******************************************************************************/
extern BswM_ModeRequestPortStructType BswM_DdsModeRequestPorts[];
extern const uint16 BswM_NumDdsModeRequestPorts;

/******************************************************************************
 * Rule Configuration
 ******************************************************************************/
extern const BswM_RuleType BswM_DdsRules[];
extern const uint16 BswM_NumDdsRules;

/******************************************************************************
 * Action List Configuration
 ******************************************************************************/
extern const BswM_ActionListType BswM_DdsActionLists[];
extern const uint16 BswM_NumDdsActionLists;

/******************************************************************************
 * Mode Condition Configuration
 ******************************************************************************/
extern const BswM_ModeConditionType BswM_DdsModeConditions[];
extern const uint16 BswM_NumDdsModeConditions;

/******************************************************************************
 * Expression Configuration
 ******************************************************************************/
extern const BswM_ExpressionStructType BswM_DdsExpressions[];
extern const uint16 BswM_NumDdsExpressions;

/******************************************************************************
 * Configuration Sets
 ******************************************************************************/
#define BSWM_CONFIG_DDS_DEFAULT     (&BswM_DdsConfig)

/******************************************************************************
 * Callback Function Declarations for DDS Integration
 ******************************************************************************/

/* DDS Communication Callbacks */
extern void BswM_Dds_CommunicationStateChange(BswM_DdsCommunicationStateType newState);
extern void BswM_Dds_NetworkStatusChange(BswM_DdsNetworkStatusType newStatus);
extern void BswM_Dds_DiscoveryStateChange(BswM_DdsDiscoveryStateType newState);
extern void BswM_Dds_PublisherStatusChange(BswM_DdsEntityStatusType newStatus);
extern void BswM_Dds_SubscriberStatusChange(BswM_DdsEntityStatusType newStatus);
extern void BswM_Dds_SecurityStatusChange(BswM_DdsSecurityStatusType newStatus);

/* System State Callbacks */
extern void BswM_System_StateChange(BswM_SystemStateModeType newState);
extern void BswM_System_EnterSafeMode(void);
extern void BswM_System_ExitSafeMode(void);

/* SoAd/PduR Routing Control Callbacks */
extern void BswM_SoAd_EnableRouting(uint16 routingGroup);
extern void BswM_SoAd_DisableRouting(uint16 routingGroup);
extern void BswM_PduR_EnableRouting(uint16 routingPath);
extern void BswM_PduR_DisableRouting(uint16 routingPath);

/******************************************************************************
 * Utility Macros
 ******************************************************************************/
#define BSWM_IS_DDS_COMM_ACTIVE() \
    (BswM_GetModeRequestPortValue(BSWM_MRP_DDS_COMMUNICATION_STATE) == DDS_COMM_STATE_ACTIVE)

#define BSWM_IS_DDS_NETWORK_CONNECTED() \
    (BswM_GetModeRequestPortValue(BSWM_MRP_DDS_NETWORK_STATUS) == DDS_NETWORK_CONNECTED)

#define BSWM_IS_SYSTEM_SAFE_MODE() \
    (BswM_GetMode(BSWM_MODE_SYSTEM_STATE) == BSWM_SYSTEM_STATE_SAFE)

#define BSWM_TRIGGER_DDS_RECONNECT() \
    BswM_RequestMode(BSWM_MRP_DDS_NETWORK_STATUS, DDS_NETWORK_RECONNECTING)

#define BSWM_SET_DDS_COMM_STATE(state) \
    BswM_UpdateModeRequestPort(BSWM_MRP_DDS_COMMUNICATION_STATE, (BswM_ModeType)(state))

#ifdef __cplusplus
}
#endif

#endif /* BSWM_CFG_H */

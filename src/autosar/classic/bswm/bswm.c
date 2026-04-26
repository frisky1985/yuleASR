/******************************************************************************
 * @file    bswm.c
 * @brief   BSW Mode Manager (BswM) Implementation
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
#define BSWM_MAX_CALLBACKS              8U
#define BSWM_MAINFUNCTION_PERIOD_MS     10U

/******************************************************************************
 * Module Variables (extern for use by other BswM files)
 ******************************************************************************/
const BswM_ConfigType *BswM_ConfigPtr = NULL;
BswM_StatusType BswM_Status;
BswM_RuleStateType BswM_RuleStates[BSWM_MAX_RULES];
BswM_ModeType BswM_ModeValues[BSWM_MAX_MODES];

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static void BswM_ProcessModeRequests(void);
static void BswM_ProcessRules(void);
static void BswM_ProcessDeferredActions(void);

/******************************************************************************
 * External API Implementations
 ******************************************************************************/

void BswM_Init(const BswM_ConfigType *config)
{
    uint16 i;
    
    if (config != NULL) {
        /* Store configuration */
        BswM_ConfigPtr = config;
        
        /* Initialize status */
        (void)memset(&BswM_Status, 0, sizeof(BswM_StatusType));
        BswM_Status.ruleStates = BswM_RuleStates;
        
        /* Initialize rule states */
        for (i = 0U; i < BSWM_MAX_RULES; i++) {
            BswM_RuleStates[i] = BSWM_RULE_UNDEF;
        }
        
        /* Initialize rule states from configuration */
        if (config->rules != NULL) {
            for (i = 0U; i < config->numRules; i++) {
                if (config->rules[i].isInitialized) {
                    BswM_RuleStates[config->rules[i].id] = config->rules[i].lastState;
                }
            }
        }
        
        /* Initialize mode request ports */
        if (config->modeRequestPorts != NULL) {
            for (i = 0U; i < config->numModeRequestPorts; i++) {
                if (config->modeRequestPorts[i].isInitialized) {
                    /* Initialize with configured mode */
                    config->modeRequestPorts[i].currentMode = 
                        config->modeRequestPorts[i].requestedMode;
                }
            }
        }
        
        /* Initialize mode values */
        for (i = 0U; i < BSWM_MAX_MODES; i++) {
            BswM_ModeValues[i] = 0U;
        }
        
        BswM_Status.initialized = TRUE;
    }
}

void BswM_Deinit(void)
{
    if (BswM_Status.initialized) {
        BswM_ConfigPtr = NULL;
        (void)memset(&BswM_Status, 0, sizeof(BswM_StatusType));
        (void)memset(BswM_RuleStates, 0, sizeof(BswM_RuleStates));
        (void)memset(BswM_ModeValues, 0, sizeof(BswM_ModeValues));
    }
}

void BswM_MainFunction(void)
{
    if (BswM_Status.initialized) {
        /* Process mode requests */
        if ((BswM_ConfigPtr != NULL) &&
            (BswM_ConfigPtr->modeRequestProcessingEnabled)) {
            BswM_ProcessModeRequests();
        }
        
        /* Evaluate and execute rules */
        if ((BswM_ConfigPtr != NULL) &&
            (BswM_ConfigPtr->rulesProcessingEnabled)) {
            BswM_ProcessRules();
        }
        
        /* Process deferred actions */
        BswM_ProcessDeferredActions();
    }
}

void BswM_RequestMode(
    BswM_ModeRequestPortType portType,
    BswM_ModeType mode)
{
    uint16 i;
    
    if ((!BswM_Status.initialized) || (BswM_ConfigPtr == NULL)) {
        return;
    }
    
    /* Find matching mode request port */
    if (BswM_ConfigPtr->modeRequestPorts != NULL) {
        for (i = 0U; i < BswM_ConfigPtr->numModeRequestPorts; i++) {
            if (BswM_ConfigPtr->modeRequestPorts[i].type == (uint8)portType) {
                /* Update requested mode */
                BswM_ConfigPtr->modeRequestPorts[i].requestedMode = mode;
                BswM_ConfigPtr->modeRequestPorts[i].requestPending = TRUE;
                break;
            }
        }
    }
}

void BswM_ModeRequest(
    BswM_UserType requestingModule,
    BswM_ModeType mode)
{
    /* Generic mode request - map to generic request port type */
    if (BswM_Status.initialized) {
        BswM_RequestMode(BSWM_MRP_GENERIC_REQUEST, mode);
    }
    
    (void)requestingModule;
}

void BswM_SwitchMode(
    BswM_ModeIdType modeId,
    BswM_ModeType mode)
{
    if ((!BswM_Status.initialized) || (modeId >= BSWM_MAX_MODES)) {
        return;
    }
    
    /* Update mode value */
    BswM_ModeValues[modeId] = mode;
    
    /* Trigger rule evaluation (deferred) */
    BswM_Status.deferredProcessingPending = TRUE;
}

BswM_ModeType BswM_GetMode(BswM_ModeIdType modeId)
{
    BswM_ModeType mode = 0U;
    
    if ((BswM_Status.initialized) && (modeId < BSWM_MAX_MODES)) {
        mode = BswM_ModeValues[modeId];
    }
    
    return mode;
}

void BswM_RuleNotification(
    BswM_RuleIdType ruleId,
    BswM_RuleStateType state)
{
    /* Update rule state */
    if ((BswM_Status.initialized) && (ruleId < BSWM_MAX_RULES)) {
        BswM_RuleStates[ruleId] = state;
    }
}

void BswM_UpdateModeRequestPort(
    uint16 portId,
    BswM_ModeType mode)
{
    if ((!BswM_Status.initialized) || (BswM_ConfigPtr == NULL)) {
        return;
    }
    
    if ((BswM_ConfigPtr->modeRequestPorts != NULL) &&
        (portId < BswM_ConfigPtr->numModeRequestPorts)) {
        BswM_ConfigPtr->modeRequestPorts[portId].currentMode = mode;
        BswM_ConfigPtr->modeRequestPorts[portId].requestPending = FALSE;
    }
}

BswM_ModeType BswM_GetModeRequestPortValue(uint16 portId)
{
    BswM_ModeType mode = 0U;
    
    if ((BswM_Status.initialized) && (BswM_ConfigPtr != NULL)) {
        if ((BswM_ConfigPtr->modeRequestPorts != NULL) &&
            (portId < BswM_ConfigPtr->numModeRequestPorts)) {
            mode = BswM_ConfigPtr->modeRequestPorts[portId].currentMode;
        }
    }
    
    return mode;
}

const BswM_StatusType* BswM_GetStatus(void)
{
    return &BswM_Status;
}

boolean BswM_IsInitialized(void)
{
    return BswM_Status.initialized;
}

void BswM_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = BSWM_VENDOR_ID;
        version->moduleID = BSWM_MODULE_ID;
        version->sw_major_version = BSWM_SW_MAJOR_VERSION;
        version->sw_minor_version = BSWM_SW_MINOR_VERSION;
        version->sw_patch_version = BSWM_SW_PATCH_VERSION;
    }
}

/******************************************************************************
 * DCM Integration Functions
 ******************************************************************************/
void BswM_Dcm_RequestSessionMode(uint8 sessionMode)
{
    /* Map DCM session mode to BswM mode request port */
    BswM_RequestMode(BSWM_MRP_DCM_SESSION_MODE_REQUEST, (BswM_ModeType)sessionMode);
}

void BswM_Dcm_RequestResetMode(uint8 resetMode)
{
    /* Map DCM reset mode to BswM mode request port */
    BswM_RequestMode(BSWM_MRP_DCM_RESET_MODE_REQUEST, (BswM_ModeType)resetMode);
}

void BswM_Dcm_CommunicationMode_CurrentState(
    uint8 channel,
    uint8 comMode)
{
    /* Update DCM communication mode */
    BswM_RequestMode(BSWM_MRP_DCM_COM_MODE_CURRENT, (BswM_ModeType)comMode);
    
    (void)channel;
}

void BswM_Dcm_ApplicationUpdated(boolean updateState)
{
    /* Notify BswM about application update */
    BswM_RequestMode(BSWM_MRP_DCM_APPLICATION_UPDATE, 
                     updateState ? 1U : 0U);
}

/******************************************************************************
 * ComM Integration Functions
 ******************************************************************************/
void BswM_ComM_CurrentMode(
    uint8 channel,
    ComM_ModeType comMode)
{
    /* Update ComM mode indication */
    BswM_RequestMode(BSWM_MRP_COMM_INDICATION, (BswM_ModeType)comMode);
    
    (void)channel;
}

void BswM_ComM_RequestMode(
    PNCHandleType pnc,
    ComM_ModeType comMode)
{
    /* Handle ComM PNC request */
    BswM_RequestMode(BSWM_MRP_COMM_PNC_REQUEST, (BswM_ModeType)comMode);
    
    (void)pnc;
}

/******************************************************************************
 * EcuM Integration Functions
 ******************************************************************************/
void BswM_EcuM_CurrentState(EcuM_StateType state)
{
    /* Map EcuM state to BswM mode request port */
    BswM_RequestMode(BSWM_MRP_ECUM_INDICATION, (BswM_ModeType)state);
}

void BswM_EcuM_WakeupStatus(
    EcuM_WakeupSourceType source,
    EcuM_WakeupStatusType status)
{
    /* Map EcuM wakeup status to BswM mode request port */
    /* Encode both source and status in mode value */
    BswM_ModeType mode = ((BswM_ModeType)source << 8U) | (BswM_ModeType)status;
    BswM_RequestMode(BSWM_MRP_ECUM_WK_STATUS, mode);
}

/******************************************************************************
 * NVM Integration Functions
 ******************************************************************************/
void BswM_NvM_CurrentJobMode(
    uint16 blockId,
    uint8 jobMode)
{
    /* Update NVM job mode */
    BswM_RequestMode(BSWM_MRP_NVM_JOB_MODE_INDICATION, (BswM_ModeType)jobMode);
    
    (void)blockId;
}

/******************************************************************************
 * Bus Manager Integration Functions
 ******************************************************************************/
void BswM_CanSM_CurrentState(
    uint8 channel,
    uint8 state)
{
    BswM_RequestMode(BSWM_MRP_CAN_SM_INDICATION, (BswM_ModeType)state);
    (void)channel;
}

void BswM_LinSM_CurrentState(
    uint8 channel,
    uint8 state)
{
    BswM_RequestMode(BSWM_MRP_LIN_SM_INDICATION, (BswM_ModeType)state);
    (void)channel;
}

void BswM_LinSM_CurrentSchedule(
    uint8 channel,
    uint8 schedule)
{
    BswM_RequestMode(BSWM_MRP_LINSM_SCHEDULE, (BswM_ModeType)schedule);
    (void)channel;
}

void BswM_FrSM_CurrentState(
    uint8 channel,
    uint8 state)
{
    BswM_RequestMode(BSWM_MRP_FR_SM_INDICATION, (BswM_ModeType)state);
    (void)channel;
}

void BswM_EthSM_CurrentState(
    uint8 channel,
    uint8 state)
{
    BswM_RequestMode(BSWM_MRP_ETH_SM_INDICATION, (BswM_ModeType)state);
    (void)channel;
}

/******************************************************************************
 * Service Discovery Functions
 ******************************************************************************/
void BswM_SD_ClientServiceMode(
    uint16 serviceHandleId,
    uint8 mode)
{
    BswM_RequestMode(BSWM_MRP_SD_CLIENT_SERVICE_MODE, (BswM_ModeType)mode);
    (void)serviceHandleId;
}

void BswM_SD_ConsumedEventGroupMode(
    uint16 consumedEventGroupHandleId,
    uint8 mode)
{
    BswM_RequestMode(BSWM_MRP_SD_CONSUMED_EVENT_GROUP_MODE, (BswM_ModeType)mode);
    (void)consumedEventGroupHandleId;
}

void BswM_SD_EventHandlerMode(
    uint16 eventHandlerHandleId,
    uint8 mode)
{
    BswM_RequestMode(BSWM_MRP_SD_EVENT_HANDLER_MODE, (BswM_ModeType)mode);
    (void)eventHandlerHandleId;
}

/******************************************************************************
 * Internal Function Implementations
 ******************************************************************************/

/**
 * @brief Process pending mode requests
 */
static void BswM_ProcessModeRequests(void)
{
    uint16 i;
    
    if (BswM_ConfigPtr == NULL) {
        return;
    }
    
    if (BswM_ConfigPtr->modeRequestPorts == NULL) {
        return;
    }
    
    /* Process each pending mode request */
    for (i = 0U; i < BswM_ConfigPtr->numModeRequestPorts; i++) {
        if (BswM_ConfigPtr->modeRequestPorts[i].requestPending) {
            /* Update current mode */
            BswM_ConfigPtr->modeRequestPorts[i].currentMode = 
                BswM_ConfigPtr->modeRequestPorts[i].requestedMode;
            BswM_ConfigPtr->modeRequestPorts[i].requestPending = FALSE;
            
            /* Trigger rule evaluation */
            BswM_Status.deferredProcessingPending = TRUE;
        }
    }
}

/**
 * @brief Process all rules
 */
static void BswM_ProcessRules(void)
{
    uint16 i;
    
    if (BswM_ConfigPtr == NULL) {
        return;
    }
    
    if (BswM_ConfigPtr->rules == NULL) {
        return;
    }
    
    /* Evaluate each rule */
    for (i = 0U; i < BswM_ConfigPtr->numRules; i++) {
        (void)BswM_EvaluateRule(BswM_ConfigPtr->rules[i].id);
    }
}

/**
 * @brief Process deferred actions
 */
static void BswM_ProcessDeferredActions(void)
{
    if (BswM_Status.deferredProcessingPending) {
        /* Process any deferred actions */
        BswM_Status.deferredProcessingPending = FALSE;
    }
}

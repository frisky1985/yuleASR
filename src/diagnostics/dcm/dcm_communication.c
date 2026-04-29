/******************************************************************************
 * @file    dcm_communication.c
 * @brief   DCM Communication Control Service (0x28) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_communication.h"
#include "dcm_session.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_COMM_MAGIC_INIT             (0x434F4D4DU)  /* "COMM" */
#define DCM_MAX_SUBNETS                 8U
#define DCM_ALL_SUBNETS                 0U

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;                                 /* Initialization marker */
    const Dcm_CommunicationControlConfigType *config; /* Configuration pointer */
    Dcm_CommunicationStatusType status;             /* Communication status */
    bool initialized;                               /* Initialization flag */
} Dcm_CommunicationInternalStateType;

/******************************************************************************
 * Static Data
 ******************************************************************************/
static Dcm_CommunicationInternalStateType s_commState;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Send negative response
 */
static Dcm_ReturnType sendNegativeResponse(
    Dcm_ResponseType *response,
    uint8_t nrc
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (response != NULL) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        response->length = 0U;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Send positive response
 */
static Dcm_ReturnType sendPositiveResponse(
    Dcm_ResponseType *response,
    uint8_t controlType
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= DCM_COMM_CTRL_RESPONSE_LENGTH)) {
        response->data[0U] = (uint8_t)(UDS_SVC_COMMUNICATION_CONTROL + 0x40U);
        response->data[1U] = controlType;
        response->length = DCM_COMM_CTRL_RESPONSE_LENGTH;
        response->isNegativeResponse = false;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Get subnet state structure
 */
static Dcm_SubnetStateType* getSubnetState(uint8_t subnetId)
{
    Dcm_SubnetStateType *state = NULL;
    
    if (s_commState.initialized) {
        for (uint8_t i = 0U; i < s_commState.status.numSubnets; i++) {
            if (s_commState.status.subnetStates[i].subnetId == subnetId) {
                state = &s_commState.status.subnetStates[i];
                break;
            }
        }
    }
    
    return state;
}

/**
 * @brief Initialize subnet state
 */
static void initSubnetState(Dcm_SubnetStateType *subnet, uint8_t subnetId)
{
    if (subnet != NULL) {
        subnet->subnetId = subnetId;
        subnet->state = DCM_COMM_STATE_ENABLED;
        subnet->normalRxEnabled = true;
        subnet->normalTxEnabled = true;
        subnet->nmRxEnabled = true;
        subnet->nmTxEnabled = true;
        subnet->ddsEnabled = true;
    }
}

/**
 * @brief Apply control to subnet
 */
static Dcm_ReturnType applyControlToSubnet(
    uint8_t subnetId,
    uint8_t controlType,
    uint8_t commType
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    Dcm_SubnetStateType *subnet = getSubnetState(subnetId);
    
    if (subnet != NULL) {
        bool rxEnable = false;
        bool txEnable = false;
        bool normalAffected = false;
        bool nmAffected = false;
        
        /* Determine RX/TX settings based on control type */
        switch (controlType) {
            case DCM_COMM_CTRL_ENABLE_RX_TX:
                rxEnable = true;
                txEnable = true;
                break;
            case DCM_COMM_CTRL_ENABLE_RX_DISABLE_TX:
                rxEnable = true;
                txEnable = false;
                break;
            case DCM_COMM_CTRL_DISABLE_RX_ENABLE_TX:
                rxEnable = false;
                txEnable = true;
                break;
            case DCM_COMM_CTRL_DISABLE_RX_TX:
                rxEnable = false;
                txEnable = false;
                break;
            default:
                /* Invalid control type - handled by caller */
                break;
        }
        
        /* Determine which communication types are affected */
        switch (commType) {
            case DCM_COMM_TYPE_NORMAL:
                normalAffected = true;
                break;
            case DCM_COMM_TYPE_NM:
                nmAffected = true;
                break;
            case DCM_COMM_TYPE_NORMAL_AND_NM:
                normalAffected = true;
                nmAffected = true;
                break;
            default:
                /* Invalid type - handled by caller */
                break;
        }
        
        /* Apply settings */
        if (normalAffected) {
            subnet->normalRxEnabled = rxEnable;
            subnet->normalTxEnabled = txEnable;
        }
        
        if (nmAffected) {
            subnet->nmRxEnabled = rxEnable;
            subnet->nmTxEnabled = txEnable;
        }
        
        /* Update overall state */
        if (subnet->normalRxEnabled && subnet->nmRxEnabled &&
            subnet->normalTxEnabled && subnet->nmTxEnabled) {
            subnet->state = DCM_COMM_STATE_ENABLED;
        } else if (!subnet->normalRxEnabled && !subnet->nmRxEnabled &&
                   !subnet->normalTxEnabled && !subnet->nmTxEnabled) {
            subnet->state = DCM_COMM_STATE_RX_TX_DISABLED;
        } else if (!subnet->normalRxEnabled && !subnet->nmRxEnabled) {
            subnet->state = DCM_COMM_STATE_RX_DISABLED;
        } else if (!subnet->normalTxEnabled && !subnet->nmTxEnabled) {
            subnet->state = DCM_COMM_STATE_TX_DISABLED;
        } else {
            /* Mixed state - use disabled as default */
            subnet->state = DCM_COMM_STATE_RX_TX_DISABLED;
        }
        
        /* Notify network callback if registered */
        if ((s_commState.config != NULL) && 
            (s_commState.config->networkCallback != NULL)) {
            result = s_commState.config->networkCallback(
                subnetId,
                (subnet->normalRxEnabled || subnet->nmRxEnabled),
                (subnet->normalTxEnabled || subnet->nmTxEnabled)
            );
        } else {
            result = DCM_E_OK;
        }
        
        /* Notify change callback */
        if ((result == DCM_E_OK) && (s_commState.config != NULL) &&
            (s_commState.config->changeCallback != NULL)) {
            Dcm_CommunicationTypeEnum type = DCM_COMM_BOTH_MESSAGES;
            if (normalAffected && !nmAffected) {
                type = DCM_COMM_NORMAL_MESSAGES;
            } else if (!normalAffected && nmAffected) {
                type = DCM_COMM_NM_MESSAGES;
            } else {
                /* Both affected or unknown */
            }
            
            s_commState.config->changeCallback(
                type,
                rxEnable,
                txEnable,
                subnetId
            );
        }
    }
    
    return result;
}

/**
 * @brief Check session requirements
 */
static bool checkSessionRequirements(void)
{
    bool allowed = true;
    Dcm_SessionType currentSession = Dcm_GetCurrentSession();
    
    if (s_commState.config != NULL) {
        if (s_commState.config->requireExtendedSession && 
            (currentSession != DCM_SESSION_EXTENDED)) {
            allowed = false;
        }
        
        if (s_commState.config->requireProgrammingSession && 
            (currentSession != DCM_SESSION_PROGRAMMING)) {
            allowed = false;
        }
    }
    
    return allowed;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_CommunicationControlInit(const Dcm_CommunicationControlConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL) {
        /* Initialize state */
        (void)memset(&s_commState, 0, sizeof(s_commState));
        
        s_commState.magic = DCM_COMM_MAGIC_INIT;
        s_commState.config = config;
        s_commState.initialized = true;
        
        /* Initialize default subnet (0 = all networks) */
        s_commState.status.numSubnets = 1U;
        initSubnetState(&s_commState.status.subnetStates[0U], DCM_ALL_SUBNETS);
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_CommunicationControl(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    /* Check initialization */
    if (!s_commState.initialized) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL)) {
        return result;
    }
    
    /* Check request length */
    if (request->length < DCM_COMM_CTRL_MIN_LENGTH) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check session requirements */
    if (!checkSessionRequirements()) {
        nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED_IN_SESSION;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    const uint8_t controlType = request->data[1U] & DCM_SUBFUNCTION_MASK;
    const uint8_t commType = request->data[2U];
    uint8_t subnetId = DCM_ALL_SUBNETS;
    
    /* Check for optional node identification */
    if (request->length > DCM_COMM_CTRL_MIN_LENGTH) {
        subnetId = request->data[3U];
    }
    
    /* Check for suppress positive response */
    if ((request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0U) {
        response->suppressPositiveResponse = true;
    }
    
    /* Validate control type */
    if (!Dcm_IsControlTypeValid(controlType)) {
        nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate communication type */
    if (!Dcm_IsCommunicationTypeValid(commType)) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check if communication type is enabled in config */
    if ((s_commState.config != NULL) && !s_commState.config->enableNormalCommControl &&
        ((commType == DCM_COMM_TYPE_NORMAL) || (commType == DCM_COMM_TYPE_NORMAL_AND_NM))) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    if ((s_commState.config != NULL) && !s_commState.config->enableNmCommControl &&
        ((commType == DCM_COMM_TYPE_NM) || (commType == DCM_COMM_TYPE_NORMAL_AND_NM))) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate node identification */
    if (!Dcm_IsNodeIdentificationValid(subnetId)) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Apply control to all subnets or specific subnet */
    if (subnetId == DCM_ALL_SUBNETS) {
        for (uint8_t i = 0U; i < s_commState.status.numSubnets; i++) {
            result = applyControlToSubnet(
                s_commState.status.subnetStates[i].subnetId,
                controlType,
                commType
            );
            
            if (result != DCM_E_OK) {
                break;
            }
        }
    } else {
        result = applyControlToSubnet(subnetId, controlType, commType);
    }
    
    if (result == DCM_E_OK) {
        /* Update status */
        s_commState.status.activeControlType = controlType;
        s_commState.status.activeCommunicationType = commType;
        s_commState.status.controlRequestCount++;
        
        result = sendPositiveResponse(response, controlType);
    } else {
        nrc = UDS_NRC_GENERAL_REJECT;
        (void)sendNegativeResponse(response, nrc);
    }
    
    return result;
}

Dcm_ReturnType Dcm_EnableCommunication(uint8_t subnetId)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_commState.initialized) {
        if (subnetId == DCM_ALL_SUBNETS) {
            for (uint8_t i = 0U; i < s_commState.status.numSubnets; i++) {
                result = applyControlToSubnet(
                    s_commState.status.subnetStates[i].subnetId,
                    DCM_COMM_CTRL_ENABLE_RX_TX,
                    DCM_COMM_TYPE_NORMAL_AND_NM
                );
                
                if (result != DCM_E_OK) {
                    break;
                }
            }
        } else {
            result = applyControlToSubnet(subnetId, DCM_COMM_CTRL_ENABLE_RX_TX, DCM_COMM_TYPE_NORMAL_AND_NM);
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_DisableCommunication(uint8_t subnetId)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_commState.initialized) {
        if (subnetId == DCM_ALL_SUBNETS) {
            for (uint8_t i = 0U; i < s_commState.status.numSubnets; i++) {
                result = applyControlToSubnet(
                    s_commState.status.subnetStates[i].subnetId,
                    DCM_COMM_CTRL_DISABLE_RX_TX,
                    DCM_COMM_TYPE_NORMAL_AND_NM
                );
                
                if (result != DCM_E_OK) {
                    break;
                }
            }
        } else {
            result = applyControlToSubnet(subnetId, DCM_COMM_CTRL_DISABLE_RX_TX, DCM_COMM_TYPE_NORMAL_AND_NM);
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_GetCommunicationState(
    uint8_t subnetId,
    Dcm_CommunicationStateType *state
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((state != NULL) && s_commState.initialized) {
        Dcm_SubnetStateType *subnet = getSubnetState(subnetId);
        
        if (subnet != NULL) {
            *state = subnet->state;
            result = DCM_E_OK;
        }
    }
    
    return result;
}

bool Dcm_IsNormalCommunicationEnabled(uint8_t subnetId)
{
    bool enabled = false;
    
    if (s_commState.initialized) {
        Dcm_SubnetStateType *subnet = getSubnetState(subnetId);
        
        if (subnet != NULL) {
            enabled = (subnet->normalRxEnabled && subnet->normalTxEnabled);
        }
    }
    
    return enabled;
}

bool Dcm_IsNmCommunicationEnabled(uint8_t subnetId)
{
    bool enabled = false;
    
    if (s_commState.initialized) {
        Dcm_SubnetStateType *subnet = getSubnetState(subnetId);
        
        if (subnet != NULL) {
            enabled = (subnet->nmRxEnabled && subnet->nmTxEnabled);
        }
    }
    
    return enabled;
}

Dcm_ReturnType Dcm_GetCommunicationStatus(Dcm_CommunicationStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((status != NULL) && s_commState.initialized) {
        *status = s_commState.status;
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_ResetCommunicationState(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_commState.initialized) {
        /* Reset all subnets to enabled state */
        for (uint8_t i = 0U; i < s_commState.status.numSubnets; i++) {
            initSubnetState(&s_commState.status.subnetStates[i], 
                          s_commState.status.subnetStates[i].subnetId);
        }
        
        s_commState.status.activeControlType = DCM_COMM_CTRL_ENABLE_RX_TX;
        s_commState.status.activeCommunicationType = DCM_COMM_TYPE_NORMAL_AND_NM;
        
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsControlTypeValid(uint8_t controlType)
{
    bool valid = false;
    
    if ((controlType == DCM_COMM_CTRL_ENABLE_RX_TX) ||
        (controlType == DCM_COMM_CTRL_ENABLE_RX_DISABLE_TX) ||
        (controlType == DCM_COMM_CTRL_DISABLE_RX_ENABLE_TX) ||
        (controlType == DCM_COMM_CTRL_DISABLE_RX_TX)) {
        valid = true;
    }
    
    return valid;
}

bool Dcm_IsCommunicationTypeValid(uint8_t commType)
{
    bool valid = false;
    
    if ((commType == DCM_COMM_TYPE_NORMAL) ||
        (commType == DCM_COMM_TYPE_NM) ||
        (commType == DCM_COMM_TYPE_NORMAL_AND_NM)) {
        valid = true;
    }
    
    return valid;
}

bool Dcm_IsNodeIdentificationValid(uint8_t nodeId)
{
    bool valid = false;
    
    if ((nodeId == DCM_COMM_NODE_ECU) ||
        ((nodeId >= DCM_COMM_NODE_SPECIFIC_MIN) && 
         (nodeId <= DCM_COMM_NODE_SPECIFIC_MAX))) {
        valid = true;
    }
    
    return valid;
}

Dcm_ReturnType Dcm_ControlDdsTopics(bool enable, uint8_t topicMask)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_commState.initialized && (s_commState.config != NULL) &&
        s_commState.config->enableDdsControl) {
        if (s_commState.config->ddsCallback != NULL) {
            s_commState.config->ddsCallback(enable, topicMask);
        }
        
        /* Update state for all subnets */
        for (uint8_t i = 0U; i < s_commState.status.numSubnets; i++) {
            s_commState.status.subnetStates[i].ddsEnabled = enable;
        }
        
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_AreDdsTopicsEnabled(uint8_t topicMask)
{
    bool enabled = false;
    (void)topicMask;  /* Unused in basic implementation */
    
    if (s_commState.initialized && (s_commState.status.numSubnets > 0U)) {
        /* Check first subnet as representative */
        enabled = s_commState.status.subnetStates[0U].ddsEnabled;
    }
    
    return enabled;
}

/******************************************************************************
 * @file    dcm.c
 * @brief   DCM (Diagnostic Communication Manager) Main Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_MAGIC_INIT                  (0x44434D30U)  /* "DCM0" */

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_ConfigType *config;
    Dcm_StateType state;
    bool initialized;
} Dcm_MainStateType;

static Dcm_MainStateType s_dcmState;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Send negative response
 */
static Dcm_ReturnType sendNegativeResponse(Dcm_ResponseType *response, 
                                            uint8_t sid, 
                                            uint8_t nrc)
{
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= 3U)) {
        response->data[0U] = DCM_SID_NEGATIVE_RESPONSE;
        response->data[1U] = sid;
        response->data[2U] = nrc;
        response->length = 3U;
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_Init(const Dcm_ConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL) {
        /* Clear state */
        (void)memset(&s_dcmState, 0, sizeof(s_dcmState));
        
        s_dcmState.magic = DCM_MAGIC_INIT;
        s_dcmState.config = config;
        s_dcmState.state = DCM_STATE_INIT;
        
        /* Initialize session control */
        if (config->sessionConfig != NULL) {
            result = Dcm_SessionInit(config->sessionConfig);
        }
        
        /* Initialize ECU reset service */
        if ((result == DCM_E_OK) && (config->ecuResetConfig != NULL)) {
            result = Dcm_EcuResetInit(config->ecuResetConfig);
        }
        
        /* Initialize security access service */
        if ((result == DCM_E_OK) && (config->securityConfig != NULL)) {
            result = Dcm_SecurityAccessInit(config->securityConfig);
        }
        
        /* Initialize communication control service */
        if ((result == DCM_E_OK) && (config->commControlConfig != NULL)) {
            result = Dcm_CommunicationControlInit(config->commControlConfig);
        }
        
        /* Initialize dynamic DID service */
        if ((result == DCM_E_OK) && (config->dynamicDidConfig != NULL)) {
            result = Dcm_DynamicDidInit(config->dynamicDidConfig);
        }
        
        /* Initialize memory write service */
        if ((result == DCM_E_OK) && (config->memoryWriteConfig != NULL)) {
            result = Dcm_MemoryWriteInit(config->memoryWriteConfig);
        }
        
        /* Initialize routine control service */
        if ((result == DCM_E_OK) && (config->routineConfigs != NULL)) {
            result = Dcm_RoutineControlInit(config->routineConfigs, 
                                            config->numRoutines);
        }
        
        if (result == DCM_E_OK) {
            s_dcmState.initialized = true;
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_DeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_dcmState.initialized) {
        /* Reset all sub-modules */
        s_dcmState.initialized = false;
        s_dcmState.state = DCM_STATE_UNINIT;
        s_dcmState.magic = 0U;
        s_dcmState.config = NULL;
        
        result = DCM_E_OK;
    }
    
    return result;
}

void Dcm_MainFunction(uint32_t elapsedTimeMs)
{
    if (s_dcmState.initialized) {
        /* Update session timers */
        (void)Dcm_SessionTimerUpdate(elapsedTimeMs);
        
        /* Update ECU reset timer */
        (void)Dcm_EcuResetTimerUpdate(elapsedTimeMs);
        
        /* Update security timers */
        (void)Dcm_SecurityTimerUpdate(elapsedTimeMs);
        
        /* Check for ECU reset execution */
        if (Dcm_GetEcuResetState() == DCM_ECU_RESET_STATE_PENDING) {
            /* Execute reset after positive response sent */
            /* This would be handled by the protocol layer */
        }
    }
}

Dcm_ReturnType Dcm_ProcessRequest(const Dcm_RequestType *request,
                                  Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    /* Check initialization */
    if (!s_dcmState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL) || 
        (request->data == NULL) || (request->length == 0U)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check minimum request length */
    if (request->length < 1U) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, request->data[0U], nrc);
        return DCM_E_NOT_OK;
    }
    
    const uint8_t serviceId = request->data[0U];
    s_dcmState.state = DCM_STATE_PROCESSING;
    
    /* Route to appropriate service handler */
    switch (serviceId) {
        case UDS_SVC_DIAGNOSTIC_SESSION_CONTROL:
            result = Dcm_DiagnosticSessionControl(request, response);
            break;
            
        case UDS_SVC_ECU_RESET:
            result = Dcm_EcuReset(request, response);
            break;
            
        case UDS_SVC_SECURITY_ACCESS:
            result = Dcm_SecurityAccess(request, response);
            break;
            
        case UDS_SVC_COMMUNICATION_CONTROL:
            result = Dcm_CommunicationControl(request, response);
            break;
            
        case UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER:
            result = Dcm_DynamicallyDefineDataIdentifier(request, response);
            break;
            
        case UDS_SVC_WRITE_MEMORY_BY_ADDRESS:
            result = Dcm_WriteMemoryByAddress(request, response);
            break;
            
        case UDS_SVC_ROUTINE_CONTROL:
            result = Dcm_RoutineControl(request, response);
            break;
            
        default:
            /* Service not supported */
            nrc = UDS_NRC_SERVICE_NOT_SUPPORTED;
            result = sendNegativeResponse(response, serviceId, nrc);
            break;
    }
    
    s_dcmState.state = DCM_STATE_INIT;
    return result;
}

bool Dcm_IsInitialized(void)
{
    return s_dcmState.initialized;
}

Dcm_StateType Dcm_GetState(void)
{
    return s_dcmState.state;
}

void Dcm_GetVersionInfo(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if (major != NULL) {
        *major = DCM_MAJOR_VERSION;
    }
    if (minor != NULL) {
        *minor = DCM_MINOR_VERSION;
    }
    if (patch != NULL) {
        *patch = DCM_PATCH_VERSION;
    }
}

/******************************************************************************
 * Service-specific process functions for external use
 ******************************************************************************/

Dcm_ReturnType Dcm_ProcessSecurityAccess(const Dcm_RequestType *request,
                                         Dcm_ResponseType *response)
{
    return Dcm_SecurityAccess(request, response);
}

Dcm_ReturnType Dcm_ProcessCommunicationControl(const Dcm_RequestType *request,
                                               Dcm_ResponseType *response)
{
    return Dcm_CommunicationControl(request, response);
}

Dcm_ReturnType Dcm_ProcessDynamicDid(const Dcm_RequestType *request,
                                     Dcm_ResponseType *response)
{
    return Dcm_DynamicallyDefineDataIdentifier(request, response);
}

Dcm_ReturnType Dcm_ProcessWriteMemory(const Dcm_RequestType *request,
                                      Dcm_ResponseType *response)
{
    return Dcm_WriteMemoryByAddress(request, response);
}

Dcm_ReturnType Dcm_ProcessRoutineControl(const Dcm_RequestType *request,
                                         Dcm_ResponseType *response)
{
    return Dcm_RoutineControl(request, response);
}

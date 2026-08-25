/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

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
#include "dcm_dem_integration.h"
#include "dcm_did.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_MAGIC_INIT                  (0x44434D30U)  /* "DCM0" */

/******************************************************************************
 * Module State
 ******************************************************************************/
Dcm_ReturnType Dcm_ProcessRoutineControl(const Dcm_RequestType *request,                                         Dcm_ResponseType *response);
Dcm_ReturnType Dcm_ProcessWriteMemory(const Dcm_RequestType *request,                                      Dcm_ResponseType *response);
Dcm_ReturnType Dcm_ProcessDynamicDid(const Dcm_RequestType *request,                                     Dcm_ResponseType *response);
Dcm_ReturnType Dcm_ProcessCommunicationControl(const Dcm_RequestType *request,                                               Dcm_ResponseType *response);
Dcm_ReturnType Dcm_ProcessSecurityAccess(const Dcm_RequestType *request,                                         Dcm_ResponseType *response);
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
 * @req SWS_Dcm_00101
 */
static Dcm_ReturnType sendNegativeResponse(Dcm_ResponseType *response, 
                                            uint8_t sid, 
                                            uint8_t nrc)
{
    if ((response != NULL_PTR) && (response->data != NULL_PTR) && 
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

/**
 * @brief Initialize the DCM module
 * @req SWS_Dcm_00001
 */
Dcm_ReturnType Dcm_Init(const Dcm_ConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL_PTR) {
        /* Clear state */
        (void)memset(&s_dcmState, 0, sizeof(s_dcmState));
        
        s_dcmState.magic = DCM_MAGIC_INIT;
        s_dcmState.config = config;
        s_dcmState.state = DCM_STATE_INIT;
        
        /* Initialize session control */
        if (config->sessionConfig != NULL_PTR) {
            result = Dcm_SessionInit(config->sessionConfig);
        }
        
        /* Initialize ECU reset service */
        if ((result == DCM_E_OK) && (config->ecuResetConfig != NULL_PTR)) {
            result = Dcm_EcuResetInit(config->ecuResetConfig);
        }
        
        /* Initialize security access service */
        if ((result == DCM_E_OK) && (config->securityConfig != NULL_PTR)) {
            result = Dcm_SecurityAccessInit(config->securityConfig);
        }
        
        /* Initialize communication control service */
        if ((result == DCM_E_OK) && (config->commControlConfig != NULL_PTR)) {
            result = Dcm_CommunicationControlInit(config->commControlConfig);
        }
        
        /* Initialize dynamic DID service */
        if ((result == DCM_E_OK) && (config->dynamicDidConfig != NULL_PTR)) {
            result = Dcm_DynamicDidInit(config->dynamicDidConfig);
        }
        
        /* Initialize memory write service */
        if ((result == DCM_E_OK) && (config->memoryWriteConfig != NULL_PTR)) {
            result = Dcm_MemoryWriteInit(config->memoryWriteConfig);
        }
        
        /* Initialize routine control service */
        if ((result == DCM_E_OK) && (config->routineConfigs != NULL_PTR)) {
            result = Dcm_RoutineControlInit(config->routineConfigs, 
                                            config->numRoutines);
        }

        /* Initialize DCM-DEM integration */
        if ((result == DCM_E_OK) && (config->demIntegrationConfig != NULL_PTR)) {
            result = Dcm_DemIntegration_Init(config->demIntegrationConfig);
        }

        /* Initialize DID service */
        if ((result == DCM_E_OK) && (config->didConfig != NULL_PTR)) {
            result = Dcm_DidInit(config->didConfig);
        }

        /* Initialize IO Control service */
        if ((result == DCM_E_OK) && (config->ioControlConfigs != NULL_PTR)) {
            result = Dcm_IoControlInit(config->ioControlConfigs, config->numIoControls);
        }
        
        if (result == DCM_E_OK) {
            s_dcmState.initialized = true;
        }
    }
    
    return result;
}

/**
 * @brief Deinitialize the DCM module
 * @req SWS_Dcm_00002
 */
Dcm_ReturnType Dcm_DeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_dcmState.initialized) {
        /* Deinitialize DCM-DEM integration */
        (void)Dcm_DemIntegration_DeInit();

        /* Reset all sub-modules */
        s_dcmState.initialized = false;
        s_dcmState.state = DCM_STATE_UNINIT;
        s_dcmState.magic = 0U;
        s_dcmState.config = NULL_PTR;
        
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Main function for periodic DCM processing
 * @req SWS_Dcm_00003
 */
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

/**
 * @brief Process a diagnostic request
 * @req SWS_Dcm_00004
 */
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
    if ((request == NULL_PTR) || (response == NULL_PTR) || 
        (request->data == NULL_PTR) || (request->length == 0U)) {
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
            
        case UDS_SVC_TESTER_PRESENT:
            result = Dcm_TesterPresent(request, response);
            break;
            
        case UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER:
            result = Dcm_DynamicallyDefineDataIdentifier(request, response);
            break;
            
        case UDS_SVC_WRITE_MEMORY_BY_ADDRESS:
            result = Dcm_WriteMemoryByAddress(request, response);
            break;

        case UDS_SVC_READ_MEMORY_BY_ADDRESS:
            result = Dcm_ReadMemoryByAddress(request, response);
            break;
            
        case UDS_SVC_ROUTINE_CONTROL:
            result = Dcm_RoutineControl(request, response);
            break;

        case UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION:
            result = Dcm_DemIntegration_ClearDiagnosticInformation(request, response);
            break;

        case UDS_SVC_READ_DTC_INFORMATION:
            result = Dcm_DemIntegration_ReadDTCInformation(request, response);
            break;

        case UDS_SVC_CONTROL_DTC_SETTING:
            result = Dcm_DemIntegration_ControlDTCSetting(request, response);
            break;

        case UDS_SVC_READ_DATA_BY_IDENTIFIER:
            result = Dcm_ReadDataByIdentifier(request, response);
            break;

        case UDS_SVC_WRITE_DATA_BY_IDENTIFIER:
            result = Dcm_WriteDataByIdentifier(request, response);
            break;

        case UDS_SVC_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER:
            result = Dcm_InputOutputControlByIdentifier(request, response);
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

/**
 * @brief Check if DCM is initialized
 * @req SWS_Dcm_00005
 */
bool Dcm_IsInitialized(void)
{
    return s_dcmState.initialized;
}

/**
 * @brief Get the current DCM state
 * @req SWS_Dcm_00006
 */
Dcm_StateType Dcm_GetState(void)
{
    return s_dcmState.state;
}

/**
 * @brief Get DCM version information
 * @req SWS_Dcm_00010
 */
void Dcm_GetVersionInfo(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if (major != NULL_PTR) {
        *major = DCM_MAJOR_VERSION;
    }
    if (minor != NULL_PTR) {
        *minor = DCM_MINOR_VERSION;
    }
    if (patch != NULL_PTR) {
        *patch = DCM_PATCH_VERSION;
    }
}

/******************************************************************************
 * Service-specific process functions for external use
 ******************************************************************************/

/**
 * @brief Process security access service
 * @req SWS_Dcm_00102
 */
Dcm_ReturnType Dcm_ProcessSecurityAccess(const Dcm_RequestType *request,
                                         Dcm_ResponseType *response)
{
    return Dcm_SecurityAccess(request, response);
}

/**
 * @brief Process communication control service
 * @req SWS_Dcm_00103
 */
Dcm_ReturnType Dcm_ProcessCommunicationControl(const Dcm_RequestType *request,
                                               Dcm_ResponseType *response)
{
    return Dcm_CommunicationControl(request, response);
}

/**
 * @brief Process dynamic DID service
 * @req SWS_Dcm_00104
 */
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

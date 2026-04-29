/******************************************************************************
 * @file    dcm_routine.c
 * @brief   DCM Routine Control Service (0x31) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_routine.h"
#include "dcm_security.h"
#include "dcm_session.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_ROUTINE_MAGIC_INIT          (0x52544430U)  /* "RTD0" */
#define DCM_MAX_ROUTINES                32U

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_RoutineConfigType *routines;
    uint8_t numRoutines;
    Dcm_RoutineStatusType status;
    bool initialized;
} Dcm_RoutineInternalStateType;

static Dcm_RoutineInternalStateType s_routineState;

/******************************************************************************
 * Default Routine Configurations
 ******************************************************************************/
static const Dcm_RoutineConfigType s_defaultRoutines[] = {
    {
        .routineId = DCM_ROUTINE_ID_ERASE_MEMORY,
        .startSupported = true,
        .stopSupported = false,
        .resultsSupported = true,
        .requiredSecurityLevel = 3U,
        .requiredSession = DCM_SESSION_PROGRAMMING,
        .startFunc = NULL,
        .stopFunc = NULL,
        .resultsFunc = NULL,
        .description = "Erase Memory"
    },
    {
        .routineId = DCM_ROUTINE_ID_CHECK_PROG_DEPENDENCIES,
        .startSupported = true,
        .stopSupported = false,
        .resultsSupported = true,
        .requiredSecurityLevel = 1U,
        .requiredSession = DCM_SESSION_EXTENDED,
        .startFunc = NULL,
        .stopFunc = NULL,
        .resultsFunc = NULL,
        .description = "Check Programming Dependencies"
    },
    {
        .routineId = DCM_ROUTINE_ID_SELF_TEST,
        .startSupported = true,
        .stopSupported = true,
        .resultsSupported = true,
        .requiredSecurityLevel = 1U,
        .requiredSession = DCM_SESSION_EXTENDED,
        .startFunc = NULL,
        .stopFunc = NULL,
        .resultsFunc = NULL,
        .description = "Self Test"
    }
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

static Dcm_ReturnType sendNegativeResponse(Dcm_ResponseType *response, uint8_t nrc)
{
    if (response != NULL) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        response->length = 0U;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

static Dcm_ReturnType sendPositiveResponse(Dcm_ResponseType *response,
                                           uint8_t controlType,
                                           uint16_t routineId,
                                           const uint8_t *statusRecord,
                                           uint16_t statusLength)
{
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= (uint32_t)(4U + statusLength))) {
        response->data[0U] = (uint8_t)(UDS_SVC_ROUTINE_CONTROL + 0x40U);
        response->data[1U] = controlType;
        response->data[2U] = (uint8_t)((routineId >> 8) & 0xFFU);
        response->data[3U] = (uint8_t)(routineId & 0xFFU);
        
        if ((statusRecord != NULL) && (statusLength > 0U)) {
            (void)memcpy(&response->data[4U], statusRecord, statusLength);
        }
        
        response->length = (uint32_t)(4U + statusLength);
        response->isNegativeResponse = false;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

static const Dcm_RoutineConfigType* findRoutineConfig(uint16_t routineId)
{
    if (s_routineState.initialized && (s_routineState.routines != NULL)) {
        for (uint8_t i = 0U; i < s_routineState.numRoutines; i++) {
            if (s_routineState.routines[i].routineId == routineId) {
                return &s_routineState.routines[i];
            }
        }
    }
    return NULL;
}

static bool checkRoutineAccess(const Dcm_RoutineConfigType *config)
{
    if (config == NULL) {
        return false;
    }
    
    /* Check security level */
    if (config->requiredSecurityLevel > 0U) {
        if (!Dcm_IsSecurityLevelUnlocked(config->requiredSecurityLevel)) {
            return false;
        }
    }
    
    /* Check session */
    Dcm_SessionType currentSession = Dcm_GetCurrentSession();
    if ((config->requiredSession != DCM_SESSION_DEFAULT) && 
        (currentSession != config->requiredSession)) {
        return false;
    }
    
    return true;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_RoutineControlInit(const Dcm_RoutineConfigType *routines,
                                      uint8_t numRoutines)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    (void)memset(&s_routineState, 0, sizeof(s_routineState));
    
    s_routineState.magic = DCM_ROUTINE_MAGIC_INIT;
    s_routineState.initialized = true;
    s_routineState.status.state = DCM_ROUTINE_STATE_IDLE;
    
    if ((routines != NULL) && (numRoutines > 0U)) {
        s_routineState.routines = routines;
        s_routineState.numRoutines = numRoutines;
    } else {
        /* Use default routines */
        s_routineState.routines = s_defaultRoutines;
        s_routineState.numRoutines = (uint8_t)(sizeof(s_defaultRoutines) / sizeof(s_defaultRoutines[0U]));
    }
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_RoutineControl(const Dcm_RequestType *request,
                                  Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    if (!s_routineState.initialized) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    if ((request == NULL) || (response == NULL)) {
        return result;
    }
    
    if (request->length < DCM_ROUTINE_CTRL_MIN_LENGTH) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    const uint8_t controlType = request->data[1U] & DCM_SUBFUNCTION_MASK;
    const uint16_t routineId = ((uint16_t)request->data[2U] << 8) | request->data[3U];
    
    if ((request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0U) {
        response->suppressPositiveResponse = true;
    }
    
    /* Validate control type */
    if (!Dcm_IsRoutineControlTypeValid(controlType)) {
        nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Find routine configuration */
    const Dcm_RoutineConfigType *routineConfig = findRoutineConfig(routineId);
    
    if (routineConfig == NULL) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check access requirements */
    if (!checkRoutineAccess(routineConfig)) {
        nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Process based on control type */
    switch (controlType) {
        case DCM_ROUTINE_CTRL_START: {
            if (!routineConfig->startSupported) {
                nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
                break;
            }
            
            /* Check if already running */
            if (s_routineState.status.state == DCM_ROUTINE_STATE_RUNNING) {
                nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
                break;
            }
            
            const uint8_t *optionRecord = NULL;
            uint16_t optionLength = 0U;
            
            if (request->length > DCM_ROUTINE_CTRL_MIN_LENGTH) {
                optionRecord = &request->data[4U];
                optionLength = (uint16_t)(request->length - DCM_ROUTINE_CTRL_MIN_LENGTH);
            }
            
            uint8_t statusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
            uint16_t statusLength = 0U;
            
            result = Dcm_StartRoutine(routineId, optionRecord, optionLength,
                                      statusRecord, &statusLength);
            
            if (result == DCM_E_OK) {
                s_routineState.status.routineId = routineId;
                s_routineState.status.state = DCM_ROUTINE_STATE_RUNNING;
                s_routineState.status.executionCount++;
                s_routineState.status.startTime = 0U;  /* TODO: Get timestamp */
                
                result = sendPositiveResponse(response, controlType, routineId,
                                              statusRecord, statusLength);
            } else {
                nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
            }
            break;
        }
        
        case DCM_ROUTINE_CTRL_STOP: {
            if (!routineConfig->stopSupported) {
                nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
                break;
            }
            
            /* Check if correct routine is running */
            if ((s_routineState.status.state != DCM_ROUTINE_STATE_RUNNING) ||
                (s_routineState.status.routineId != routineId)) {
                nrc = UDS_NRC_REQUEST_SEQUENCE_ERROR;
                break;
            }
            
            uint8_t statusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
            uint16_t statusLength = 0U;
            
            result = Dcm_StopRoutine(routineId, statusRecord, &statusLength);
            
            if (result == DCM_E_OK) {
                s_routineState.status.state = DCM_ROUTINE_STATE_STOPPED;
                s_routineState.status.endTime = 0U;
                
                result = sendPositiveResponse(response, controlType, routineId,
                                              statusRecord, statusLength);
            } else {
                nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
            }
            break;
        }
        
        case DCM_ROUTINE_CTRL_REQUEST_RESULTS: {
            if (!routineConfig->resultsSupported) {
                nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
                break;
            }
            
            /* Check if routine was started */
            if (s_routineState.status.routineId != routineId) {
                nrc = UDS_NRC_REQUEST_SEQUENCE_ERROR;
                break;
            }
            
            uint8_t statusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
            uint16_t statusLength = 0U;
            uint8_t dataRecord[DCM_MAX_ROUTINE_INFO_RECORD];
            uint16_t dataLength = 0U;
            
            result = Dcm_RequestRoutineResults(routineId, statusRecord, &statusLength,
                                               dataRecord, &dataLength);
            
            if (result == DCM_E_OK) {
                /* Combine status and data for response */
                uint8_t combinedRecord[DCM_MAX_ROUTINE_STATUS_RECORD + DCM_MAX_ROUTINE_INFO_RECORD];
                if (statusLength > 0U) {
                    (void)memcpy(combinedRecord, statusRecord, statusLength);
                }
                if (dataLength > 0U) {
                    (void)memcpy(&combinedRecord[statusLength], dataRecord, dataLength);
                }
                
                result = sendPositiveResponse(response, controlType, routineId,
                                              combinedRecord, statusLength + dataLength);
            } else {
                nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
            }
            break;
        }
        
        default:
            nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            break;
    }
    
    if ((result != DCM_E_OK) && (nrc != UDS_NRC_GENERAL_REJECT)) {
        (void)sendNegativeResponse(response, nrc);
    }
    
    return result;
}

Dcm_ReturnType Dcm_StartRoutine(uint16_t routineId,
                                const uint8_t *optionRecord,
                                uint16_t optionLength,
                                uint8_t *statusRecord,
                                uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL) && (statusLength != NULL) && s_routineState.initialized) {
        const Dcm_RoutineConfigType *config = findRoutineConfig(routineId);
        
        if ((config != NULL) && config->startSupported) {
            if (config->startFunc != NULL) {
                result = config->startFunc(routineId, optionRecord, optionLength,
                                           statusRecord, statusLength);
            } else {
                /* Default implementation */
                *statusLength = 1U;
                statusRecord[0U] = 0x00U;  /* Routine started successfully */
                
                /* Simulate routine completion for predefined routines */
                if (routineId == DCM_ROUTINE_ID_ERASE_MEMORY) {
                    s_routineState.status.progress = 100U;
                    s_routineState.status.state = DCM_ROUTINE_STATE_COMPLETED;
                } else if (routineId == DCM_ROUTINE_ID_CHECK_PROG_DEPENDENCIES) {
                    s_routineState.status.progress = 100U;
                    s_routineState.status.state = DCM_ROUTINE_STATE_COMPLETED;
                    statusRecord[0U] = 0x00U;  /* Dependencies OK */
                } else if (routineId == DCM_ROUTINE_ID_SELF_TEST) {
                    s_routineState.status.progress = 0U;
                    /* Keep running state for self test */
                }
                
                result = DCM_E_OK;
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_StopRoutine(uint16_t routineId,
                               uint8_t *statusRecord,
                               uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL) && (statusLength != NULL) && s_routineState.initialized) {
        const Dcm_RoutineConfigType *config = findRoutineConfig(routineId);
        
        if ((config != NULL) && config->stopSupported) {
            if (config->stopFunc != NULL) {
                result = config->stopFunc(routineId, statusRecord, statusLength);
            } else {
                /* Default implementation */
                *statusLength = 1U;
                statusRecord[0U] = 0x00U;  /* Routine stopped */
                s_routineState.status.state = DCM_ROUTINE_STATE_STOPPED;
                result = DCM_E_OK;
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_RequestRoutineResults(uint16_t routineId,
                                         uint8_t *statusRecord,
                                         uint16_t *statusLength,
                                         uint8_t *dataRecord,
                                         uint16_t *dataLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL) && (statusLength != NULL) && s_routineState.initialized) {
        const Dcm_RoutineConfigType *config = findRoutineConfig(routineId);
        
        if ((config != NULL) && config->resultsSupported) {
            if (config->resultsFunc != NULL) {
                result = config->resultsFunc(routineId, statusRecord, statusLength,
                                             dataRecord, dataLength);
            } else {
                /* Default implementation */
                *statusLength = 1U;
                statusRecord[0U] = (uint8_t)s_routineState.status.state;
                
                if (dataRecord != NULL && dataLength != NULL) {
                    *dataLength = 1U;
                    dataRecord[0U] = s_routineState.status.progress;
                }
                
                result = DCM_E_OK;
            }
        }
    }
    
    return result;
}

bool Dcm_IsRoutineSupported(uint16_t routineId)
{
    return (findRoutineConfig(routineId) != NULL);
}

const Dcm_RoutineConfigType* Dcm_GetRoutineConfig(uint16_t routineId)
{
    return findRoutineConfig(routineId);
}

Dcm_ReturnType Dcm_GetRoutineStatus(Dcm_RoutineStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((status != NULL) && s_routineState.initialized) {
        *status = s_routineState.status;
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsRoutineControlTypeValid(uint8_t controlType)
{
    return ((controlType == DCM_ROUTINE_CTRL_START) ||
            (controlType == DCM_ROUTINE_CTRL_STOP) ||
            (controlType == DCM_ROUTINE_CTRL_REQUEST_RESULTS));
}

Dcm_RoutineStateType Dcm_GetRoutineState(void)
{
    Dcm_RoutineStateType state = DCM_ROUTINE_STATE_IDLE;
    
    if (s_routineState.initialized) {
        state = s_routineState.status.state;
    }
    
    return state;
}

bool Dcm_IsRoutineRunning(void)
{
    return (Dcm_GetRoutineState() == DCM_ROUTINE_STATE_RUNNING);
}

/******************************************************************************
 * Predefined Routine Implementations
 ******************************************************************************/

Dcm_ReturnType Dcm_Routine_EraseMemory(uint32_t memoryAddress, uint32_t memorySize)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    /* Validate parameters */
    if ((memorySize > 0U) && s_routineState.initialized) {
        /* Start the erase memory routine */
        uint8_t optionRecord[8U];
        optionRecord[0U] = (uint8_t)((memoryAddress >> 24) & 0xFFU);
        optionRecord[1U] = (uint8_t)((memoryAddress >> 16) & 0xFFU);
        optionRecord[2U] = (uint8_t)((memoryAddress >> 8) & 0xFFU);
        optionRecord[3U] = (uint8_t)(memoryAddress & 0xFFU);
        optionRecord[4U] = (uint8_t)((memorySize >> 24) & 0xFFU);
        optionRecord[5U] = (uint8_t)((memorySize >> 16) & 0xFFU);
        optionRecord[6U] = (uint8_t)((memorySize >> 8) & 0xFFU);
        optionRecord[7U] = (uint8_t)(memorySize & 0xFFU);
        
        uint8_t statusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
        uint16_t statusLength = 0U;
        
        result = Dcm_StartRoutine(DCM_ROUTINE_ID_ERASE_MEMORY, optionRecord, 8U,
                                  statusRecord, &statusLength);
    }
    
    return result;
}

Dcm_ReturnType Dcm_Routine_CheckProgrammingDependencies(bool *result)
{
    Dcm_ReturnType retVal = DCM_E_NOT_OK;
    
    if ((result != NULL) && s_routineState.initialized) {
        uint8_t statusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
        uint16_t statusLength = 0U;
        
        retVal = Dcm_StartRoutine(DCM_ROUTINE_ID_CHECK_PROG_DEPENDENCIES, NULL, 0U,
                                  statusRecord, &statusLength);
        
        if (retVal == DCM_E_OK) {
            *result = (statusRecord[0U] == 0x00U);
        }
    }
    
    return retVal;
}

Dcm_ReturnType Dcm_Routine_SelfTest(uint8_t testId, bool *result)
{
    Dcm_ReturnType retVal = DCM_E_NOT_OK;
    (void)testId;  /* Unused in default implementation */
    
    if ((result != NULL) && s_routineState.initialized) {
        uint8_t optionRecord[1U] = {testId};
        uint8_t statusRecord[DCM_MAX_ROUTINE_STATUS_RECORD];
        uint16_t statusLength = 0U;
        
        retVal = Dcm_StartRoutine(DCM_ROUTINE_ID_SELF_TEST, optionRecord, 1U,
                                  statusRecord, &statusLength);
        
        if (retVal == DCM_E_OK) {
            *result = true;  /* Assume success */
        }
    }
    
    return retVal;
}

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
 * @file    dcm_io_control.c
 * @brief   DCM Input Output Control By Identifier Service (0x2F) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant (Section 10.7)
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_io_control.h"
#include "dcm_security.h"
#include "dcm_session.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_IO_CONTROL_MAGIC_INIT       (0x494F4330U)  /* "IOC0" */
#define DCM_MAX_IO_CONTROLS             32U

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_IoControlConfigType *ioControls;
    uint8_t numIoControls;
    Dcm_IoControlStatusType status;
    bool initialized;
} Dcm_IoControlInternalStateType;

static Dcm_IoControlInternalStateType s_ioControlState;

/******************************************************************************
 * Default IO Control Configurations
 ******************************************************************************/
static const Dcm_IoControlConfigType s_defaultIoControls[] = {
    {
        .dataIdentifier = DCM_IO_CTRL_DID_ECU_VOLTAGE,
        .returnToEcuSupported = true,
        .resetToDefaultSupported = true,
        .freezeStateSupported = true,
        .shortTermAdjustmentSupported = true,
        .controlEnableMaskSupported = false,
        .requiredSecurityLevel = 1U,
        .requiredSession = DCM_SESSION_EXTENDED,
        .returnToEcuFunc = NULL_PTR,
        .resetToDefaultFunc = NULL_PTR,
        .freezeStateFunc = NULL_PTR,
        .shortTermFunc = NULL_PTR,
        .conditionCheckFunc = NULL_PTR,
        .controlStateSize = 2U,
        .description = "ECU Supply Voltage Control"
    },
    {
        .dataIdentifier = DCM_IO_CTRL_DID_ENGINE_RPM,
        .returnToEcuSupported = true,
        .resetToDefaultSupported = true,
        .freezeStateSupported = true,
        .shortTermAdjustmentSupported = true,
        .controlEnableMaskSupported = true,
        .requiredSecurityLevel = 1U,
        .requiredSession = DCM_SESSION_EXTENDED,
        .returnToEcuFunc = NULL_PTR,
        .resetToDefaultFunc = NULL_PTR,
        .freezeStateFunc = NULL_PTR,
        .shortTermFunc = NULL_PTR,
        .conditionCheckFunc = NULL_PTR,
        .controlStateSize = 2U,
        .description = "Engine RPM Control"
    },
    {
        .dataIdentifier = DCM_IO_CTRL_DID_FUEL_PUMP,
        .returnToEcuSupported = true,
        .resetToDefaultSupported = true,
        .freezeStateSupported = true,
        .shortTermAdjustmentSupported = true,
        .controlEnableMaskSupported = false,
        .requiredSecurityLevel = 1U,
        .requiredSession = DCM_SESSION_EXTENDED,
        .returnToEcuFunc = NULL_PTR,
        .resetToDefaultFunc = NULL_PTR,
        .freezeStateFunc = NULL_PTR,
        .shortTermFunc = NULL_PTR,
        .conditionCheckFunc = NULL_PTR,
        .controlStateSize = 1U,
        .description = "Fuel Pump Control"
    },
    {
        .dataIdentifier = DCM_IO_CTRL_DID_THROTTLE_POSITION,
        .returnToEcuSupported = true,
        .resetToDefaultSupported = true,
        .freezeStateSupported = true,
        .shortTermAdjustmentSupported = true,
        .controlEnableMaskSupported = true,
        .requiredSecurityLevel = 1U,
        .requiredSession = DCM_SESSION_EXTENDED,
        .returnToEcuFunc = NULL_PTR,
        .resetToDefaultFunc = NULL_PTR,
        .freezeStateFunc = NULL_PTR,
        .shortTermFunc = NULL_PTR,
        .conditionCheckFunc = NULL_PTR,
        .controlStateSize = 2U,
        .description = "Throttle Position Control"
    }
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Send negative response
 */
static Dcm_ReturnType sendNegativeResponse(Dcm_ResponseType *response, uint8_t nrc)
{
    if (response != NULL_PTR) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        response->length = 0U;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

/**
 * @brief Send positive response
 */
static Dcm_ReturnType sendPositiveResponse(Dcm_ResponseType *response,
                                           uint16_t dataIdentifier,
                                           const uint8_t *statusRecord,
                                           uint16_t statusLength)
{
    if ((response != NULL_PTR) && (response->data != NULL_PTR) && 
        (response->maxLength >= (uint32_t)(3U + statusLength))) {
        response->data[0U] = (uint8_t)(UDS_SVC_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER + 0x40U);
        response->data[1U] = (uint8_t)((dataIdentifier >> 8) & 0xFFU);
        response->data[2U] = (uint8_t)(dataIdentifier & 0xFFU);
        
        if ((statusRecord != NULL_PTR) && (statusLength > 0U)) {
            (void)memcpy(&response->data[3U], statusRecord, statusLength);
        }
        
        response->length = (uint32_t)(3U + statusLength);
        response->isNegativeResponse = false;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

/**
 * @brief Find IO control configuration
 */
static const Dcm_IoControlConfigType* findIoControlConfig(uint16_t dataIdentifier)
{
    if (s_ioControlState.initialized && (s_ioControlState.ioControls != NULL_PTR)) {
        for (uint8_t i = 0U; i < s_ioControlState.numIoControls; i++) {
            if (s_ioControlState.ioControls[i].dataIdentifier == dataIdentifier) {
                return &s_ioControlState.ioControls[i];
            }
        }
    }
    return NULL_PTR;
}

/**
 * @brief Check IO control access requirements
 */
static bool checkIoControlAccess(const Dcm_IoControlConfigType *config)
{
    if (config == NULL_PTR) {
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

/**
 * @brief Check conditions for IO control
 */
static bool checkIoControlConditions(uint16_t dataIdentifier, uint8_t controlType)
{
    const Dcm_IoControlConfigType *config = findIoControlConfig(dataIdentifier);
    
    if ((config != NULL_PTR) && (config->conditionCheckFunc != NULL_PTR)) {
        bool conditionsOk = false;
        if (config->conditionCheckFunc(dataIdentifier, controlType, &conditionsOk) == DCM_E_OK) {
            return conditionsOk;
        }
    }
    
    /* Default: conditions are OK */
    return true;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_IoControlInit(const Dcm_IoControlConfigType *ioControls,
                                 uint8_t numIoControls)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    (void)memset(&s_ioControlState, 0, sizeof(s_ioControlState));
    
    s_ioControlState.magic = DCM_IO_CONTROL_MAGIC_INIT;
    s_ioControlState.initialized = true;
    s_ioControlState.status.state = DCM_IO_STATE_ECU_CONTROL;
    s_ioControlState.status.isActive = false;
    
    if ((ioControls != NULL_PTR) && (numIoControls > 0U)) {
        s_ioControlState.ioControls = ioControls;
        s_ioControlState.numIoControls = numIoControls;
    } else {
        /* Use default IO controls */
        s_ioControlState.ioControls = s_defaultIoControls;
        s_ioControlState.numIoControls = (uint8_t)(sizeof(s_defaultIoControls) / sizeof(s_defaultIoControls[0U]));
    }
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_InputOutputControlByIdentifier(const Dcm_RequestType *request,
                                                  Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    if (!s_ioControlState.initialized) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    if ((request == NULL_PTR) || (response == NULL_PTR)) {
        return result;
    }
    
    /* Check minimum request length: SID + DID (2 bytes) + controlOptionRecord (at least 1 byte) */
    if (request->length < (DCM_IO_CONTROL_MIN_LENGTH + DCM_IO_CONTROL_OPTION_RECORD_MIN_LENGTH)) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Parse data identifier (big endian) */
    const uint16_t dataIdentifier = ((uint16_t)request->data[1U] << 8) | request->data[2U];
    
    /* Parse control option record */
    const uint8_t controlType = request->data[3U];
    
    /* Find IO control configuration */
    const Dcm_IoControlConfigType *ioConfig = findIoControlConfig(dataIdentifier);
    
    if (ioConfig == NULL_PTR) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check access requirements (security and session) */
    if (!checkIoControlAccess(ioConfig)) {
        nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate control type */
    if (!Dcm_IsIoControlTypeValid(controlType)) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check if control type is supported for this DID */
    if (!Dcm_IsIoControlTypeSupported(dataIdentifier, controlType)) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check conditions */
    if (!checkIoControlConditions(dataIdentifier, controlType)) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Process based on control type */
    uint8_t statusRecord[DCM_MAX_CONTROL_STATUS_RECORD];
    uint16_t statusLength = 0U;
    
    switch (controlType) {
        case DCM_IO_CTRL_RETURN_TO_ECU: {
            result = Dcm_IoControlReturnToEcu(dataIdentifier, statusRecord, &statusLength);
            if (result == DCM_E_OK) {
                s_ioControlState.status.dataIdentifier = dataIdentifier;
                s_ioControlState.status.controlType = controlType;
                s_ioControlState.status.state = DCM_IO_STATE_ECU_CONTROL;
                s_ioControlState.status.isActive = false;
            } else {
                nrc = UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
            }
            break;
        }
        
        case DCM_IO_CTRL_RESET_TO_DEFAULT: {
            result = Dcm_IoControlResetToDefault(dataIdentifier, statusRecord, &statusLength);
            if (result == DCM_E_OK) {
                s_ioControlState.status.dataIdentifier = dataIdentifier;
                s_ioControlState.status.controlType = controlType;
                s_ioControlState.status.state = DCM_IO_STATE_DEFAULT;
                s_ioControlState.status.isActive = true;
            } else {
                nrc = UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
            }
            break;
        }
        
        case DCM_IO_CTRL_FREEZE_CURRENT_STATE: {
            result = Dcm_IoControlFreezeCurrentState(dataIdentifier, statusRecord, &statusLength);
            if (result == DCM_E_OK) {
                s_ioControlState.status.dataIdentifier = dataIdentifier;
                s_ioControlState.status.controlType = controlType;
                s_ioControlState.status.state = DCM_IO_STATE_FROZEN;
                s_ioControlState.status.isActive = true;
            } else {
                nrc = UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
            }
            break;
        }
        
        case DCM_IO_CTRL_SHORT_TERM_ADJUSTMENT: {
            /* Check minimum length for short term adjustment */
            /* Need at least: SID(1) + DID(2) + controlType(1) + controlState(1+) */
            if (request->length < (DCM_IO_CONTROL_MIN_LENGTH + 2U)) {
                nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
                (void)sendNegativeResponse(response, nrc);
                return result;
            }
            
            /* Parse control state and optional enable mask */
            const uint8_t *controlState = &request->data[4U];
            uint16_t controlStateLength = ioConfig->controlStateSize;
            
            /* Adjust if request is shorter than expected */
            if (request->length < (uint32_t)(4U + controlStateLength)) {
                controlStateLength = (uint16_t)(request->length - 4U);
            }
            
            /* Check for control enable mask */
            const uint8_t *controlEnableMask = NULL_PTR;
            uint16_t maskLength = 0U;
            
            if (ioConfig->controlEnableMaskSupported) {
                uint16_t expectedLength = (uint16_t)(4U + controlStateLength);
                if (request->length > (uint32_t)expectedLength) {
                    controlEnableMask = &request->data[expectedLength];
                    maskLength = (uint16_t)(request->length - expectedLength);
                    if (maskLength > DCM_MAX_CONTROL_ENABLE_MASK) {
                        maskLength = DCM_MAX_CONTROL_ENABLE_MASK;
                    }
                }
            }
            
            result = Dcm_IoControlShortTermAdjustment(dataIdentifier,
                                                       controlState,
                                                       controlStateLength,
                                                       controlEnableMask,
                                                       maskLength,
                                                       statusRecord,
                                                       &statusLength);
            if (result == DCM_E_OK) {
                s_ioControlState.status.dataIdentifier = dataIdentifier;
                s_ioControlState.status.controlType = controlType;
                s_ioControlState.status.state = DCM_IO_STATE_UNDER_DIAGNOSTIC_CONTROL;
                s_ioControlState.status.isActive = true;
            } else {
                nrc = UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
            }
            break;
        }
        
        default:
            nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
            break;
    }
    
    if (result == DCM_E_OK) {
        /* Store last status */
        if (statusLength > 0U) {
            (void)memcpy(s_ioControlState.status.lastControlStatus, statusRecord, 
                        (statusLength > DCM_MAX_CONTROL_STATUS_RECORD) ? DCM_MAX_CONTROL_STATUS_RECORD : statusLength);
            s_ioControlState.status.lastStatusLength = statusLength;
        }
        
        result = sendPositiveResponse(response, dataIdentifier, statusRecord, statusLength);
    } else if (nrc != UDS_NRC_GENERAL_REJECT) {
        (void)sendNegativeResponse(response, nrc);
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoControlReturnToEcu(uint16_t dataIdentifier,
                                        uint8_t *controlStatusRecord,
                                        uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((controlStatusRecord != NULL_PTR) && (statusLength != NULL_PTR) && s_ioControlState.initialized) {
        const Dcm_IoControlConfigType *config = findIoControlConfig(dataIdentifier);
        
        if ((config != NULL_PTR) && config->returnToEcuSupported) {
            if (config->returnToEcuFunc != NULL_PTR) {
                result = config->returnToEcuFunc(dataIdentifier, controlStatusRecord, statusLength);
            } else {
                /* Default implementation */
                result = Dcm_IoCtrl_DefaultReturnToEcu(dataIdentifier, controlStatusRecord, statusLength);
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoControlResetToDefault(uint16_t dataIdentifier,
                                           uint8_t *controlStatusRecord,
                                           uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((controlStatusRecord != NULL_PTR) && (statusLength != NULL_PTR) && s_ioControlState.initialized) {
        const Dcm_IoControlConfigType *config = findIoControlConfig(dataIdentifier);
        
        if ((config != NULL_PTR) && config->resetToDefaultSupported) {
            if (config->resetToDefaultFunc != NULL_PTR) {
                result = config->resetToDefaultFunc(dataIdentifier, controlStatusRecord, statusLength);
            } else {
                /* Default implementation */
                result = Dcm_IoCtrl_DefaultResetToDefault(dataIdentifier, controlStatusRecord, statusLength);
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoControlFreezeCurrentState(uint16_t dataIdentifier,
                                               uint8_t *controlStatusRecord,
                                               uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((controlStatusRecord != NULL_PTR) && (statusLength != NULL_PTR) && s_ioControlState.initialized) {
        const Dcm_IoControlConfigType *config = findIoControlConfig(dataIdentifier);
        
        if ((config != NULL_PTR) && config->freezeStateSupported) {
            if (config->freezeStateFunc != NULL_PTR) {
                result = config->freezeStateFunc(dataIdentifier, controlStatusRecord, statusLength);
            } else {
                /* Default implementation */
                result = Dcm_IoCtrl_DefaultFreezeState(dataIdentifier, controlStatusRecord, statusLength);
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoControlShortTermAdjustment(uint16_t dataIdentifier,
                                                const uint8_t *controlState,
                                                uint16_t controlStateLength,
                                                const uint8_t *controlEnableMask,
                                                uint16_t maskLength,
                                                uint8_t *controlStatusRecord,
                                                uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((controlStatusRecord != NULL_PTR) && (statusLength != NULL_PTR) && 
        s_ioControlState.initialized) {
        const Dcm_IoControlConfigType *config = findIoControlConfig(dataIdentifier);
        
        if ((config != NULL_PTR) && config->shortTermAdjustmentSupported) {
            if (config->shortTermFunc != NULL_PTR) {
                result = config->shortTermFunc(dataIdentifier, controlState, controlStateLength,
                                               controlEnableMask, maskLength,
                                               controlStatusRecord, statusLength);
            } else {
                /* Default implementation */
                result = Dcm_IoCtrl_DefaultShortTermAdjustment(dataIdentifier, controlState, 
                                                                controlStateLength,
                                                                controlEnableMask, maskLength,
                                                                controlStatusRecord, statusLength);
            }
        }
    }
    
    return result;
}

bool Dcm_IsIoControlSupported(uint16_t dataIdentifier)
{
    return (findIoControlConfig(dataIdentifier) != NULL_PTR);
}

const Dcm_IoControlConfigType* Dcm_GetIoControlConfig(uint16_t dataIdentifier)
{
    return findIoControlConfig(dataIdentifier);
}

Dcm_ReturnType Dcm_GetIoControlStatus(Dcm_IoControlStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((status != NULL_PTR) && s_ioControlState.initialized) {
        *status = s_ioControlState.status;
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsIoControlTypeValid(uint8_t controlType)
{
    return ((controlType == DCM_IO_CTRL_RETURN_TO_ECU) ||
            (controlType == DCM_IO_CTRL_RESET_TO_DEFAULT) ||
            (controlType == DCM_IO_CTRL_FREEZE_CURRENT_STATE) ||
            (controlType == DCM_IO_CTRL_SHORT_TERM_ADJUSTMENT));
}

Dcm_IoControlStateType Dcm_GetIoControlState(void)
{
    Dcm_IoControlStateType state = DCM_IO_STATE_ECU_CONTROL;
    
    if (s_ioControlState.initialized) {
        state = s_ioControlState.status.state;
    }
    
    return state;
}

bool Dcm_IsIoUnderDiagnosticControl(void)
{
    return (Dcm_GetIoControlState() == DCM_IO_STATE_UNDER_DIAGNOSTIC_CONTROL);
}

bool Dcm_IsIoControlTypeSupported(uint16_t dataIdentifier, uint8_t controlType)
{
    const Dcm_IoControlConfigType *config = findIoControlConfig(dataIdentifier);
    bool supported = false;
    
    if (config != NULL_PTR) {
        switch (controlType) {
            case DCM_IO_CTRL_RETURN_TO_ECU:
                supported = config->returnToEcuSupported;
                break;
            case DCM_IO_CTRL_RESET_TO_DEFAULT:
                supported = config->resetToDefaultSupported;
                break;
            case DCM_IO_CTRL_FREEZE_CURRENT_STATE:
                supported = config->freezeStateSupported;
                break;
            case DCM_IO_CTRL_SHORT_TERM_ADJUSTMENT:
                supported = config->shortTermAdjustmentSupported;
                break;
            default:
                supported = false;
                break;
        }
    }
    
    return supported;
}

/******************************************************************************
 * Default IO Control Handler Implementations
 ******************************************************************************/

Dcm_ReturnType Dcm_IoCtrl_DefaultReturnToEcu(uint16_t dataIdentifier,
                                             uint8_t *statusRecord,
                                             uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL_PTR) && (statusLength != NULL_PTR)) {
        /* Return control to ECU - status shows ECU is in control */
        *statusLength = 1U;
        statusRecord[0U] = DCM_IO_CTRL_RETURN_TO_ECU;
        
        /* Store the IO control state */
        s_ioControlState.status.state = DCM_IO_STATE_ECU_CONTROL;
        s_ioControlState.status.isActive = false;
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoCtrl_DefaultResetToDefault(uint16_t dataIdentifier,
                                                uint8_t *statusRecord,
                                                uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL_PTR) && (statusLength != NULL_PTR)) {
        /* Reset to default value - return current state */
        *statusLength = 3U;
        statusRecord[0U] = DCM_IO_CTRL_RESET_TO_DEFAULT;
        statusRecord[1U] = (uint8_t)((dataIdentifier >> 8) & 0xFFU);
        statusRecord[2U] = (uint8_t)(dataIdentifier & 0xFFU);
        
        /* Store the IO control state */
        s_ioControlState.status.state = DCM_IO_STATE_DEFAULT;
        s_ioControlState.status.isActive = true;
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoCtrl_DefaultFreezeState(uint16_t dataIdentifier,
                                             uint8_t *statusRecord,
                                             uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL_PTR) && (statusLength != NULL_PTR)) {
        /* Freeze current state - return frozen state indication */
        *statusLength = 3U;
        statusRecord[0U] = DCM_IO_CTRL_FREEZE_CURRENT_STATE;
        statusRecord[1U] = (uint8_t)((dataIdentifier >> 8) & 0xFFU);
        statusRecord[2U] = (uint8_t)(dataIdentifier & 0xFFU);
        
        /* Store the IO control state */
        s_ioControlState.status.state = DCM_IO_STATE_FROZEN;
        s_ioControlState.status.isActive = true;
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_IoCtrl_DefaultShortTermAdjustment(uint16_t dataIdentifier,
                                                     const uint8_t *controlState,
                                                     uint16_t controlStateLength,
                                                     const uint8_t *controlEnableMask,
                                                     uint16_t maskLength,
                                                     uint8_t *statusRecord,
                                                     uint16_t *statusLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((statusRecord != NULL_PTR) && (statusLength != NULL_PTR) && 
        (controlState != NULL_PTR) && (controlStateLength > 0U)) {
        /* Apply control state with optional enable mask */
        *statusLength = (uint16_t)(1U + controlStateLength);
        if (*statusLength > DCM_MAX_CONTROL_STATUS_RECORD) {
            *statusLength = DCM_MAX_CONTROL_STATUS_RECORD;
        }
        
        statusRecord[0U] = DCM_IO_CTRL_SHORT_TERM_ADJUSTMENT;
        
        /* Copy control state to status record (echo back with mask applied) */
        uint16_t copyLength = controlStateLength;
        if (copyLength > (DCM_MAX_CONTROL_STATUS_RECORD - 1U)) {
            copyLength = DCM_MAX_CONTROL_STATUS_RECORD - 1U;
        }
        
        /* Apply control enable mask if provided */
        if ((controlEnableMask != NULL_PTR) && (maskLength > 0U)) {
            for (uint16_t i = 0U; i < copyLength; i++) {
                uint8_t mask = (i < maskLength) ? controlEnableMask[i] : 0xFFU;
                statusRecord[1U + i] = controlState[i] & mask;
            }
        } else {
            (void)memcpy(&statusRecord[1U], controlState, copyLength);
        }
        
        /* Store the IO control state */
        s_ioControlState.status.state = DCM_IO_STATE_UNDER_DIAGNOSTIC_CONTROL;
        s_ioControlState.status.isActive = true;
        
        result = DCM_E_OK;
    }
    
    return result;
}

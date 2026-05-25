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
 * @file    dcm_did.c
 * @brief   DCM Read Data By Identifier Service (0x22) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant (Section 10.3)
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_did.h"
#include "dcm_session.h"
#include "dcm_security.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_DID_MAGIC_INIT                  (0x44494430U)  /* "DID0" */
#define DCM_DID_MAX_DATABASE_SIZE           256U
#define DCM_DID_INVALID_INDEX               0xFFFFU

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_DidConfigType *config;
    bool initialized;
    Dcm_DidStatusType status;
    /* Runtime DID database */
    Dcm_DidDatabaseEntryType runtimeDatabase[DCM_DID_MAX_DATABASE_SIZE];
    uint16_t runtimeDbCount;
} Dcm_DidStateType;

static Dcm_DidStateType s_didState;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/
static Dcm_ReturnType sendNegativeResponse(
    Dcm_ResponseType *response,
    uint8_t sid,
    uint8_t nrc
);

static Dcm_ReturnType readSingleDid(
    uint16_t did,
    uint8_t *responseData,
    uint32_t responseMaxLength,
    uint32_t *responseLength,
    uint8_t *nrc
);

static int16_t findDidInDatabase(uint16_t did);

static bool checkDidAccess(
    uint16_t did,
    uint8_t *nrc
);

static Dcm_ReturnType buildPositiveResponse(
    const uint8_t *didData,
    uint32_t didDataLength,
    uint16_t did,
    Dcm_ResponseType *response
);

/******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Send negative response
 */
static Dcm_ReturnType sendNegativeResponse(
    Dcm_ResponseType *response,
    uint8_t sid,
    uint8_t nrc)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= 3U)) {
        response->data[0U] = DCM_SID_NEGATIVE_RESPONSE;
        response->data[1U] = sid;
        response->data[2U] = nrc;
        response->length = 3U;
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Find DID in database
 */
static int16_t findDidInDatabase(uint16_t did)
{
    int16_t result = (int16_t)(-1);
    uint16_t i;
    
    /* Search runtime database first */
    for (i = 0U; i < s_didState.runtimeDbCount; i++) {
        if (s_didState.runtimeDatabase[i].did == did) {
            result = (int16_t)i;
            break;
        }
    }
    
    /* If not found in runtime, search static configuration */
    if ((result < 0) && (s_didState.config != NULL) && 
        (s_didState.config->didTable != NULL)) {
        for (i = 0U; i < s_didState.config->numDids; i++) {
            if (s_didState.config->didTable[i].did == did) {
                /* Return index as negative to indicate static table */
                result = (int16_t)(-((int16_t)i + 2));
                break;
            }
        }
    }
    
    return result;
}

/**
 * @brief Check DID access permissions
 */
static bool checkDidAccess(uint16_t did, uint8_t *nrc)
{
    bool accessAllowed = false;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    Dcm_SessionType currentSession;
    uint8_t currentSecurityLevel;
    
    /* Check if DID exists */
    didIndex = findDidInDatabase(did);
    if (didIndex < 0) {
        /* Check for special case: dynamic DIDs */
        if ((did >= DCM_DID_MIN_IDENT_OPTION_LEGACY) && 
            (did <= DCM_DID_MAX_IDENT_OPTION_LEGACY)) {
            /* Dynamic DIDs handled separately */
            accessAllowed = true;
        } else {
            *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
            s_didState.status.outOfRangeCount++;
        }
    } else {
        /* Get DID info */
        if (didIndex >= 0) {
            didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
        } else {
            /* Static table entry */
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            didInfo = s_didState.config->didTable[staticIndex].info;
        }
        
        if (didInfo != NULL) {
            /* Check if read is enabled */
            if (!didInfo->readEnabled) {
                *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
            } else {
                /* Check session support */
                currentSession = Dcm_GetCurrentSession();
                if (!Dcm_CheckDidSession(did, currentSession)) {
                    *nrc = UDS_NRC_REQUEST_SEQUENCE_ERROR;
                } else {
                    /* Check security level */
                    currentSecurityLevel = Dcm_GetSecurityLevel();
                    if (!Dcm_CheckDidSecurity(did, currentSecurityLevel)) {
                        *nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
                        s_didState.status.securityDeniedCount++;
                    } else {
                        accessAllowed = true;
                    }
                }
            }
        } else {
            *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        }
    }
    
    return accessAllowed;
}

/**
 * @brief Read single DID data
 */
static Dcm_ReturnType readSingleDid(
    uint16_t did,
    uint8_t *responseData,
    uint32_t responseMaxLength,
    uint32_t *responseLength,
    uint8_t *nrc)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    uint16_t dataLength = 0U;
    
    /* Check access permissions */
    if (!checkDidAccess(did, nrc)) {
        return DCM_E_NOT_OK;
    }
    
    /* Find DID in database */
    didIndex = findDidInDatabase(did);
    
    if (didIndex >= 0) {
        /* Runtime database entry */
        didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
    } else if (didIndex < (-1)) {
        /* Static table entry */
        uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
        didInfo = s_didState.config->didTable[staticIndex].info;
    } else {
        /* DID not found in database - check for standard DIDs */
        /* This would be handled by default handlers */
        didInfo = NULL;
    }
    
    /* Check buffer size for DID + data */
    if (responseMaxLength < 2U) {
        *nrc = UDS_NRC_RESPONSE_TOO_LONG;
        return DCM_E_NOT_OK;
    }
    
    /* Add DID to response */
    responseData[0U] = (uint8_t)((did >> 8U) & 0xFFU);
    responseData[1U] = (uint8_t)(did & 0xFFU);
    *responseLength = 2U;
    
    if (didInfo != NULL) {
        /* Use registered callback */
        if (didInfo->readCallback != NULL) {
            result = didInfo->readCallback(
                did,
                &responseData[2U],
                (uint16_t)(responseMaxLength - 2U),
                &dataLength
            );
            
            if (result == DCM_E_OK) {
                *responseLength += dataLength;
            } else {
                *nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
            }
        } else {
            *nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
            result = DCM_E_NOT_OK;
        }
    } else {
        /* Handle standard DIDs with default handlers */
        switch (did) {
            case DCM_DID_ACTIVE_DIAGNOSTIC_SESSION:
                result = Dcm_ReadDid_ActiveSession(
                    did,
                    &responseData[2U],
                    (uint16_t)(responseMaxLength - 2U),
                    &dataLength
                );
                break;
                
            case DCM_DID_VIN:
                result = Dcm_ReadDid_VIN(
                    did,
                    &responseData[2U],
                    (uint16_t)(responseMaxLength - 2U),
                    &dataLength
                );
                break;
                
            case DCM_DID_BOOT_SOFTWARE_IDENTIFICATION:
            case DCM_DID_APPLICATION_SOFTWARE_IDENT:
            case DCM_DID_APPLICATION_DATA_IDENT:
            case DCM_DID_BOOT_SOFTWARE_IDENTIFICATION_2:
            case DCM_DID_APPLICATION_SOFTWARE_IDENT_2:
                result = Dcm_ReadDid_SoftwareIdentification(
                    did,
                    &responseData[2U],
                    (uint16_t)(responseMaxLength - 2U),
                    &dataLength
                );
                break;
                
            default:
                /* DID not supported */
                *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                result = DCM_E_NOT_OK;
                break;
        }
        
        if (result == DCM_E_OK) {
            *responseLength += dataLength;
        }
    }
    
    return result;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_DidInit(const Dcm_DidConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL) {
        /* Clear state */
        (void)memset(&s_didState, 0, sizeof(s_didState));
        
        s_didState.magic = DCM_DID_MAGIC_INIT;
        s_didState.config = config;
        s_didState.runtimeDbCount = 0U;
        s_didState.initialized = true;
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_ReadDataByIdentifier(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    uint32_t reqIndex;
    uint8_t numDids;
    uint32_t responsePos;
    uint16_t i;
    
    /* Check initialization */
    if (!s_didState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL) || 
        (request->data == NULL) || (response->data == NULL)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check minimum request length (SID + at least one DID) */
    if (request->length < DCM_DID_MIN_REQUEST_LENGTH) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, UDS_SVC_READ_DATA_BY_IDENTIFIER, nrc);
        s_didState.status.readErrorCount++;
        return DCM_E_NOT_OK;
    }
    
    /* Check if request length is valid (SID + N*2 bytes for DIDs) */
    if (((request->length - 1U) % DCM_DID_RECORD_SIZE) != 0U) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, UDS_SVC_READ_DATA_BY_IDENTIFIER, nrc);
        s_didState.status.readErrorCount++;
        return DCM_E_NOT_OK;
    }
    
    /* Calculate number of DIDs */
    numDids = (uint8_t)((request->length - 1U) / DCM_DID_RECORD_SIZE);
    
    /* Check if multiple DIDs are supported */
    if ((numDids > 1U) && 
        ((s_didState.config == NULL) || (!s_didState.config->supportMultipleDids))) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, UDS_SVC_READ_DATA_BY_IDENTIFIER, nrc);
        s_didState.status.readErrorCount++;
        return DCM_E_NOT_OK;
    }
    
    /* Check max DIDs per request */
    if ((s_didState.config != NULL) && 
        (numDids > s_didState.config->maxDidsPerRequest)) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, UDS_SVC_READ_DATA_BY_IDENTIFIER, nrc);
        s_didState.status.readErrorCount++;
        return DCM_E_NOT_OK;
    }
    
    /* Start building response */
    response->data[0U] = DCM_DID_RESPONSE_SID;
    responsePos = 1U;
    
    /* Process each DID */
    reqIndex = 1U; /* Skip SID */
    for (i = 0U; i < numDids; i++) {
        uint16_t did;
        uint32_t didResponseLength = 0U;
        
        /* Extract DID from request */
        did = (uint16_t)(((uint16_t)request->data[reqIndex] << 8U) | 
                         (uint16_t)request->data[reqIndex + 1U]);
        reqIndex += 2U;
        
        /* Update status */
        s_didState.status.lastAccessedDid = did;
        s_didState.status.readRequestCount++;
        
        /* Check if response buffer has enough space */
        if ((responsePos + 2U) > response->maxLength) {
            nrc = UDS_NRC_RESPONSE_TOO_LONG;
            (void)sendNegativeResponse(response, UDS_SVC_READ_DATA_BY_IDENTIFIER, nrc);
            s_didState.status.readErrorCount++;
            return DCM_E_NOT_OK;
        }
        
        /* Read DID data */
        result = readSingleDid(
            did,
            &response->data[responsePos],
            response->maxLength - responsePos,
            &didResponseLength,
            &nrc
        );
        
        if (result != DCM_E_OK) {
            /* Send negative response */
            s_didState.status.lastNrc = nrc;
            s_didState.status.readErrorCount++;
            (void)sendNegativeResponse(response, UDS_SVC_READ_DATA_BY_IDENTIFIER, nrc);
            return DCM_E_NOT_OK;
        }
        
        responsePos += didResponseLength;
    }
    
    /* Set final response length */
    response->length = responsePos;
    response->isNegativeResponse = false;
    response->negativeResponseCode = 0U;
    
    /* Update success count */
    s_didState.status.readSuccessCount++;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_RegisterDid(uint16_t did, const Dcm_DidInfoType *info)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((!s_didState.initialized) || (info == NULL)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check if DID already exists */
    if (findDidInDatabase(did) != (int16_t)(-1)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check runtime database capacity */
    if (s_didState.runtimeDbCount < DCM_DID_MAX_DATABASE_SIZE) {
        s_didState.runtimeDatabase[s_didState.runtimeDbCount].did = did;
        s_didState.runtimeDatabase[s_didState.runtimeDbCount].info = info;
        s_didState.runtimeDatabase[s_didState.runtimeDbCount].context = NULL;
        s_didState.runtimeDbCount++;
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_UnregisterDid(uint16_t did)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint16_t i;
    int16_t foundIndex = (int16_t)(-1);
    
    if (!s_didState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Find in runtime database only */
    for (i = 0U; i < s_didState.runtimeDbCount; i++) {
        if (s_didState.runtimeDatabase[i].did == did) {
            foundIndex = (int16_t)i;
            break;
        }
    }
    
    if (foundIndex >= 0) {
        /* Remove by shifting entries */
        for (i = (uint16_t)foundIndex; i < (s_didState.runtimeDbCount - 1U); i++) {
            s_didState.runtimeDatabase[i] = s_didState.runtimeDatabase[i + 1U];
        }
        s_didState.runtimeDbCount--;
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsDidSupported(uint16_t did)
{
    bool supported = false;
    
    if (s_didState.initialized) {
        if (findDidInDatabase(did) != (int16_t)(-1)) {
            supported = true;
        } else if (Dcm_IsValidDidRange(did)) {
            /* Check for standard DIDs that have default handlers */
            if ((did == DCM_DID_ACTIVE_DIAGNOSTIC_SESSION) ||
                (did == DCM_DID_VIN) ||
                ((did >= DCM_DID_BOOT_SOFTWARE_IDENTIFICATION) && 
                 (did <= DCM_DID_APPLICATION_SOFTWARE_IDENT_2))) {
                supported = true;
            }
        }
    }
    
    return supported;
}

bool Dcm_IsDidReadable(uint16_t did)
{
    bool readable = false;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    if (s_didState.initialized) {
        didIndex = findDidInDatabase(did);
        
        if (didIndex >= 0) {
            didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
        } else if (didIndex < (-1)) {
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            didInfo = s_didState.config->didTable[staticIndex].info;
        }
        
        if (didInfo != NULL) {
            readable = didInfo->readEnabled;
        }
    }
    
    return readable;
}

Dcm_ReturnType Dcm_GetDidInfo(uint16_t did, const Dcm_DidInfoType **info)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    int16_t didIndex;
    
    if ((s_didState.initialized) && (info != NULL)) {
        didIndex = findDidInDatabase(did);
        
        if (didIndex >= 0) {
            *info = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
            result = DCM_E_OK;
        } else if (didIndex < (-1)) {
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            *info = s_didState.config->didTable[staticIndex].info;
            result = DCM_E_OK;
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_ReadDidData(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    if ((!s_didState.initialized) || (dataBuffer == NULL) || 
        (dataLength == NULL) || (bufferSize == 0U)) {
        return DCM_E_NOT_OK;
    }
    
    didIndex = findDidInDatabase(did);
    
    if (didIndex >= 0) {
        didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
    } else if (didIndex < (-1)) {
        uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
        didInfo = s_didState.config->didTable[staticIndex].info;
    }
    
    if ((didInfo != NULL) && (didInfo->readCallback != NULL)) {
        result = didInfo->readCallback(did, dataBuffer, bufferSize, dataLength);
    }
    
    return result;
}

bool Dcm_CheckDidSecurity(uint16_t did, uint8_t currentSecurityLevel)
{
    bool accessGranted = false;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    if (s_didState.initialized) {
        didIndex = findDidInDatabase(did);
        
        if (didIndex >= 0) {
            didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
        } else if (didIndex < (-1)) {
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            didInfo = s_didState.config->didTable[staticIndex].info;
        }
        
        if (didInfo != NULL) {
            if (didInfo->requiredSecurityLevel == 0U) {
                accessGranted = true;
            } else {
                accessGranted = (currentSecurityLevel >= didInfo->requiredSecurityLevel);
            }
        } else {
            /* DID not in database - allow access for standard DIDs */
            accessGranted = true;
        }
    }
    
    return accessGranted;
}

bool Dcm_CheckDidSession(uint16_t did, Dcm_SessionType currentSession)
{
    bool accessGranted = false;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    (void)did; /* Prevent unused parameter warning when not used */
    
    if (s_didState.initialized) {
        didIndex = findDidInDatabase(did);
        
        if (didIndex >= 0) {
            didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
        } else if (didIndex < (-1)) {
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            didInfo = s_didState.config->didTable[staticIndex].info;
        }
        
        if (didInfo != NULL) {
            /* Check if current session is in supported sessions bitmask */
            uint8_t sessionMask = (uint8_t)(1U << ((uint8_t)currentSession - 1U));
            accessGranted = ((didInfo->supportedSessions & sessionMask) != 0U);
        } else {
            /* DID not in database - allow in all sessions */
            accessGranted = true;
        }
    }
    
    return accessGranted;
}

bool Dcm_IsValidDidRange(uint16_t did)
{
    bool valid = false;
    
    /* Vehicle Manufacturer Specific: 0x0100 - 0xEFFF */
    if ((did >= DCM_DID_MIN_VEHICLE_MANUFACTURER) && 
        (did <= DCM_DID_MAX_VEHICLE_MANUFACTURER)) {
        valid = true;
    }
    /* Identification Option Vehicle Manufacturer: 0xF100 - 0xF1FF */
    else if ((did >= DCM_DID_MIN_IDENT_OPTION_VM) && 
             (did <= DCM_DID_MAX_IDENT_OPTION_VM)) {
        valid = true;
    }
    /* Identification Option System Supplier: 0xF200 - 0xF2FF */
    else if ((did >= DCM_DID_MIN_IDENT_OPTION_SS) && 
             (did <= DCM_DID_MAX_IDENT_OPTION_SS)) {
        valid = true;
    }
    /* Identification Option Legacy: 0xF300 - 0xF3FF */
    else if ((did >= DCM_DID_MIN_IDENT_OPTION_LEGACY) && 
             (did <= DCM_DID_MAX_IDENT_OPTION_LEGACY)) {
        valid = true;
    }
    /* Identification Option SAE: 0xF500 - 0xF5FF */
    else if ((did >= DCM_DID_MIN_IDENT_OPTION_SAE) && 
             (did <= DCM_DID_MAX_IDENT_OPTION_SAE)) {
        valid = true;
    }
    /* Identification Option ISO: 0xF600 - 0xF6FF */
    else if ((did >= DCM_DID_MIN_IDENT_OPTION_ISO) && 
             (did <= DCM_DID_MAX_IDENT_OPTION_ISO)) {
        valid = true;
    }
    /* Standardized DIDs: 0xFF00 - 0xFFFF */
    else if (did >= DCM_DID_MIN_STANDARDIZED) {
        valid = true;
    }
    
    return valid;
}

Dcm_ReturnType Dcm_GetDidStatus(Dcm_DidStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((s_didState.initialized) && (status != NULL)) {
        (void)memcpy(status, &s_didState.status, sizeof(Dcm_DidStatusType));
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsMultipleDidSupported(void)
{
    bool supported = false;
    
    if ((s_didState.initialized) && (s_didState.config != NULL)) {
        supported = s_didState.config->supportMultipleDids;
    }
    
    return supported;
}

bool Dcm_IsDidWritable(uint16_t did)
{
    bool writable = false;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    if (s_didState.initialized) {
        didIndex = findDidInDatabase(did);
        
        if (didIndex >= 0) {
            didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
        } else if (didIndex < (-1)) {
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            didInfo = s_didState.config->didTable[staticIndex].info;
        }
        
        if (didInfo != NULL) {
            writable = didInfo->writeEnabled;
        }
    }
    
    return writable;
}

/**
 * @brief Check DID write access permissions
 */
static bool checkDidWriteAccess(uint16_t did, uint8_t *nrc)
{
    bool accessAllowed = false;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    Dcm_SessionType currentSession;
    uint8_t currentSecurityLevel;
    
    /* Check if DID exists */
    didIndex = findDidInDatabase(did);
    if (didIndex < 0) {
        *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        s_didState.status.outOfRangeCount++;
    } else {
        /* Get DID info */
        if (didIndex >= 0) {
            didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
        } else {
            /* Static table entry */
            uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
            didInfo = s_didState.config->didTable[staticIndex].info;
        }
        
        if (didInfo != NULL) {
            /* Check if write is enabled */
            if (!didInfo->writeEnabled) {
                *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
            } else {
                /* Check session support */
                currentSession = Dcm_GetCurrentSession();
                if (!Dcm_CheckDidSession(did, currentSession)) {
                    *nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
                } else {
                    /* Check security level */
                    currentSecurityLevel = Dcm_GetSecurityLevel();
                    if (!Dcm_CheckDidSecurity(did, currentSecurityLevel)) {
                        *nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
                        s_didState.status.securityDeniedCount++;
                    } else {
                        accessAllowed = true;
                    }
                }
            }
        } else {
            *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        }
    }
    
    return accessAllowed;
}

/**
 * @brief Write single DID data
 */
static Dcm_ReturnType writeSingleDid(
    uint16_t did,
    const uint8_t *writeData,
    uint16_t writeDataLength,
    uint8_t *nrc)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    /* Check access permissions */
    if (!checkDidWriteAccess(did, nrc)) {
        return DCM_E_NOT_OK;
    }
    
    /* Find DID in database */
    didIndex = findDidInDatabase(did);
    
    if (didIndex >= 0) {
        /* Runtime database entry */
        didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
    } else if (didIndex < (-1)) {
        /* Static table entry */
        uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
        didInfo = s_didState.config->didTable[staticIndex].info;
    } else {
        /* DID not found - should not happen after access check */
        *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        return DCM_E_NOT_OK;
    }
    
    if (didInfo != NULL) {
        /* Check data length */
        if (didInfo->dataLength > 0U) {
            /* Fixed length DID */
            if (writeDataLength != didInfo->dataLength) {
                *nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
                return DCM_E_NOT_OK;
            }
        } else {
            /* Variable length DID - check against max */
            if (writeDataLength > didInfo->maxDataLength) {
                *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                return DCM_E_NOT_OK;
            }
        }
        
        /* Use registered callback */
        if (didInfo->writeCallback != NULL) {
            result = didInfo->writeCallback(did, writeData, writeDataLength);
            
            if (result != DCM_E_OK) {
                /* Check if the callback returned a specific error */
                *nrc = UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
            }
        } else {
            *nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
            result = DCM_E_NOT_OK;
        }
    } else {
        *nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
    }
    
    return result;
}

Dcm_ReturnType Dcm_WriteDataByIdentifier(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    uint16_t did;
    uint16_t writeDataLength;
    
    /* Check initialization */
    if (!s_didState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL) || 
        (request->data == NULL) || (response->data == NULL)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check minimum request length (SID + DID + at least 1 byte data) */
    if (request->length < DCM_WRITE_DID_MIN_REQUEST_LENGTH) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, UDS_SVC_WRITE_DATA_BY_IDENTIFIER, nrc);
        s_didState.status.writeErrorCount++;
        return DCM_E_NOT_OK;
    }
    
    /* Extract DID from request */
    did = (uint16_t)(((uint16_t)request->data[1U] << 8U) | 
                     (uint16_t)request->data[2U]);
    
    /* Calculate data length (request length - SID - DID) */
    writeDataLength = (uint16_t)(request->length - 3U);
    
    /* Update status */
    s_didState.status.lastAccessedDid = did;
    s_didState.status.writeRequestCount++;
    
    /* Write DID data */
    result = writeSingleDid(
        did,
        &request->data[3U],  /* Data starts after SID + DID */
        writeDataLength,
        &nrc
    );
    
    if (result != DCM_E_OK) {
        /* Send negative response */
        s_didState.status.lastNrc = nrc;
        s_didState.status.writeErrorCount++;
        (void)sendNegativeResponse(response, UDS_SVC_WRITE_DATA_BY_IDENTIFIER, nrc);
        return DCM_E_NOT_OK;
    }
    
    /* Build positive response */
    if (response->maxLength >= DCM_WRITE_DID_RESPONSE_LENGTH) {
        response->data[0U] = DCM_DID_WRITE_RESPONSE_SID;
        response->data[1U] = (uint8_t)((did >> 8U) & 0xFFU);
        response->data[2U] = (uint8_t)(did & 0xFFU);
        response->length = DCM_WRITE_DID_RESPONSE_LENGTH;
        response->isNegativeResponse = false;
        response->negativeResponseCode = 0U;
        
        /* Update success count */
        s_didState.status.writeSuccessCount++;
        
        result = DCM_E_OK;
    } else {
        nrc = UDS_NRC_RESPONSE_TOO_LONG;
        (void)sendNegativeResponse(response, UDS_SVC_WRITE_DATA_BY_IDENTIFIER, nrc);
        s_didState.status.writeErrorCount++;
        result = DCM_E_NOT_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_WriteDidData(
    uint16_t did,
    const uint8_t *data,
    uint16_t dataLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    int16_t didIndex;
    const Dcm_DidInfoType *didInfo = NULL;
    
    if ((!s_didState.initialized) || (data == NULL) || (dataLength == 0U)) {
        return DCM_E_NOT_OK;
    }
    
    didIndex = findDidInDatabase(did);
    
    if (didIndex >= 0) {
        didInfo = s_didState.runtimeDatabase[(uint16_t)didIndex].info;
    } else if (didIndex < (-1)) {
        uint16_t staticIndex = (uint16_t)((-didIndex) - 2U);
        didInfo = s_didState.config->didTable[staticIndex].info;
    }
    
    if ((didInfo != NULL) && (didInfo->writeCallback != NULL) && didInfo->writeEnabled) {
        result = didInfo->writeCallback(did, data, dataLength);
    }
    
    return result;
}

uint8_t Dcm_GetDidCountFromRequest(uint32_t requestLength)
{
    uint8_t count = 0U;
    
    if (requestLength > 1U) {
        count = (uint8_t)((requestLength - 1U) / DCM_DID_RECORD_SIZE);
    }
    
    return count;
}

Dcm_ReturnType Dcm_ExtractDidFromRequest(
    const uint8_t *requestData,
    uint8_t didIndex,
    uint16_t *did)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint32_t offset;
    
    if ((requestData != NULL) && (did != NULL)) {
        offset = 1U + ((uint32_t)didIndex * DCM_DID_RECORD_SIZE);
        *did = (uint16_t)(((uint16_t)requestData[offset] << 8U) | 
                          (uint16_t)requestData[offset + 1U]);
        result = DCM_E_OK;
    }
    
    return result;
}

/******************************************************************************
 * Default Standard DID Handlers (Weak implementations)
 ******************************************************************************/

__attribute__((weak)) Dcm_ReturnType Dcm_ReadDid_VIN(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength)
{
    (void)did;
    (void)dataBuffer;
    (void)bufferSize;
    (void)dataLength;
    return DCM_E_NOT_OK;
}

__attribute__((weak)) Dcm_ReturnType Dcm_ReadDid_ActiveSession(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    Dcm_SessionType currentSession;
    
    (void)did;
    
    if ((dataBuffer != NULL) && (dataLength != NULL) && (bufferSize >= 1U)) {
        currentSession = Dcm_GetCurrentSession();
        dataBuffer[0U] = (uint8_t)currentSession;
        *dataLength = 1U;
        result = DCM_E_OK;
    }
    
    return result;
}

__attribute__((weak)) Dcm_ReturnType Dcm_ReadDid_SoftwareIdentification(
    uint16_t did,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength)
{
    (void)did;
    (void)dataBuffer;
    (void)bufferSize;
    (void)dataLength;
    return DCM_E_NOT_OK;
}

/******************************************************************************
 * @file    dcm_dynamic_did.c
 * @brief   DCM Dynamically Define Data Identifier Service (0x2C) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_dynamic_did.h"
#include "dcm_security.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_DYN_DID_MAGIC_INIT          (0x44444944U)  /* "DDID" */
#define DCM_ALL_DYNAMIC_DIDS            0xFFFFU

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;                                 /* Initialization marker */
    const Dcm_DynamicDidConfigType *config;         /* Configuration pointer */
    Dcm_DynamicDidDefinitionType definitions[DCM_MAX_DYNAMIC_DIDS];
    uint8_t numDefinedDids;                         /* Number of defined DIDs */
    bool initialized;                               /* Initialization flag */
} Dcm_DynamicDidStateType;

/******************************************************************************
 * Static Data
 ******************************************************************************/
static Dcm_DynamicDidStateType s_dynDidState;

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
    uint8_t definitionMode,
    uint16_t dynamicDid
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= DCM_DYN_DID_RESPONSE_MIN_LENGTH)) {
        response->data[0U] = (uint8_t)(UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER + 0x40U);
        response->data[1U] = definitionMode;
        
        /* Add dynamic DID for clear operation */
        if (definitionMode == DCM_DYN_DID_CLEAR_DYN_DID) {
            if (response->maxLength >= 4U) {
                response->data[2U] = (uint8_t)((dynamicDid >> 8) & 0xFFU);
                response->data[3U] = (uint8_t)(dynamicDid & 0xFFU);
                response->length = 4U;
            }
        } else {
            response->length = DCM_DYN_DID_RESPONSE_MIN_LENGTH;
        }
        
        response->isNegativeResponse = false;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Find dynamic DID definition
 */
static Dcm_DynamicDidDefinitionType* findDynamicDidDefinition(uint16_t dynamicDid)
{
    Dcm_DynamicDidDefinitionType *definition = NULL;
    
    if (s_dynDidState.initialized) {
        for (uint8_t i = 0U; i < s_dynDidState.numDefinedDids; i++) {
            if (s_dynDidState.definitions[i].dynamicDid == dynamicDid) {
                definition = &s_dynDidState.definitions[i];
                break;
            }
        }
    }
    
    return definition;
}

/**
 * @brief Find free definition slot
 */
static Dcm_DynamicDidDefinitionType* findFreeDefinitionSlot(void)
{
    Dcm_DynamicDidDefinitionType *slot = NULL;
    
    if (s_dynDidState.initialized && 
        (s_dynDidState.numDefinedDids < DCM_MAX_DYNAMIC_DIDS)) {
        slot = &s_dynDidState.definitions[s_dynDidState.numDefinedDids];
    }
    
    return slot;
}

/**
 * @brief Check security access
 */
static bool checkSecurityAccess(void)
{
    bool allowed = true;
    
    if ((s_dynDidState.config != NULL) && 
        (s_dynDidState.config->requiredSecurityLevel > 0U)) {
        allowed = Dcm_IsSecurityLevelUnlocked(s_dynDidState.config->requiredSecurityLevel);
    }
    
    return allowed;
}

/**
 * @brief Parse memory address from request
 */
static uint32_t parseMemoryAddress(const uint8_t *data, uint8_t length)
{
    uint32_t address = 0U;
    
    if (data != NULL) {
        for (uint8_t i = 0U; i < length; i++) {
            address = (address << 8U) | (uint32_t)data[i];
        }
    }
    
    return address;
}

/**
 * @brief Parse memory size from request
 */
static uint32_t parseMemorySize(const uint8_t *data, uint8_t length)
{
    uint32_t size = 0U;
    
    if (data != NULL) {
        for (uint8_t i = 0U; i < length; i++) {
            size = (size << 8U) | (uint32_t)data[i];
        }
    }
    
    return size;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_DynamicDidInit(const Dcm_DynamicDidConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL) {
        /* Initialize state */
        (void)memset(&s_dynDidState, 0, sizeof(s_dynDidState));
        
        s_dynDidState.magic = DCM_DYN_DID_MAGIC_INIT;
        s_dynDidState.config = config;
        s_dynDidState.initialized = true;
        s_dynDidState.numDefinedDids = 0U;
        
        /* Initialize all definition slots */
        for (uint8_t i = 0U; i < DCM_MAX_DYNAMIC_DIDS; i++) {
            s_dynDidState.definitions[i].defined = false;
            s_dynDidState.definitions[i].dynamicDid = 0U;
            s_dynDidState.definitions[i].numSourceElements = 0U;
            s_dynDidState.definitions[i].totalSize = 0U;
        }
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_DynamicallyDefineDataIdentifier(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    /* Check initialization */
    if (!s_dynDidState.initialized) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL)) {
        return result;
    }
    
    /* Check request length */
    if (request->length < DCM_DYN_DID_MIN_LENGTH) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Check security access */
    if (!checkSecurityAccess()) {
        nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    const uint8_t definitionMode = request->data[1U] & DCM_SUBFUNCTION_MASK;
    
    /* Check for suppress positive response */
    if ((request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0U) {
        response->suppressPositiveResponse = true;
    }
    
    /* Validate definition mode */
    if (!Dcm_IsDefinitionModeSupported(definitionMode)) {
        nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Process based on definition mode */
    switch (definitionMode) {
        case DCM_DYN_DID_DEFINE_BY_IDENTIFIER: {
            /* Check minimum length for define by identifier */
            if (request->length < 7U) {
                nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
                break;
            }
            
            /* Extract dynamic DID */
            uint16_t dynamicDid = ((uint16_t)request->data[2U] << 8) | request->data[3U];
            
            /* Validate dynamic DID range */
            if (!Dcm_IsValidDynamicDidRange(dynamicDid)) {
                nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                break;
            }
            
            /* Extract source DID */
            uint16_t sourceDid = ((uint16_t)request->data[4U] << 8) | request->data[5U];
            uint8_t position = request->data[6U];
            uint8_t size = request->data[7U];
            
            /* Validate size */
            if ((size == 0U) || (size > DCM_MAX_MEMORY_SIZE)) {
                nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                break;
            }
            
            result = Dcm_DefineDynamicDidByIdentifier(dynamicDid, sourceDid, position, size);
            
            if (result == DCM_E_OK) {
                result = sendPositiveResponse(response, definitionMode, 0U);
            } else {
                nrc = UDS_NRC_GENERAL_REJECT;
            }
            break;
        }
        
        case DCM_DYN_DID_DEFINE_BY_MEMORY_ADDRESS: {
            /* Check minimum length for define by memory address */
            if (request->length < 6U) {
                nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
                break;
            }
            
            /* Extract dynamic DID */
            uint16_t dynamicDid = ((uint16_t)request->data[2U] << 8) | request->data[3U];
            
            /* Validate dynamic DID range */
            if (!Dcm_IsValidDynamicDidRange(dynamicDid)) {
                nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                break;
            }
            
            /* Parse address and length format identifier */
            uint8_t addressLength;
            uint8_t sizeLength;
            result = Dcm_ParseAddressLengthFormat(request->data[4U], &addressLength, &sizeLength);
            
            if (result != DCM_E_OK) {
                nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                break;
            }
            
            /* Check request length */
            uint8_t expectedLength = 5U + addressLength + sizeLength;
            if (request->length < expectedLength) {
                nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
                break;
            }
            
            /* Parse memory address and size */
            uint32_t memoryAddress = parseMemoryAddress(&request->data[5U], addressLength);
            uint32_t memorySize = parseMemorySize(&request->data[5U + addressLength], sizeLength);
            
            /* Validate size */
            if ((memorySize == 0U) || (memorySize > DCM_MAX_MEMORY_SIZE)) {
                nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
                break;
            }
            
            result = Dcm_DefineDynamicDidByMemoryAddress(dynamicDid, memoryAddress, memorySize);
            
            if (result == DCM_E_OK) {
                result = sendPositiveResponse(response, definitionMode, 0U);
            } else {
                nrc = UDS_NRC_GENERAL_REJECT;
            }
            break;
        }
        
        case DCM_DYN_DID_CLEAR_DYN_DID: {
            uint16_t dynamicDid;
            
            /* Check if clearing all or specific DID */
            if (request->length >= 4U) {
                dynamicDid = ((uint16_t)request->data[2U] << 8) | request->data[3U];
            } else {
                /* If no DID specified, clear all */
                dynamicDid = DCM_ALL_DYNAMIC_DIDS;
            }
            
            result = Dcm_ClearDynamicDid(dynamicDid);
            
            if (result == DCM_E_OK) {
                result = sendPositiveResponse(response, definitionMode, dynamicDid);
            } else {
                nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
            }
            break;
        }
        
        default:
            nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            break;
    }
    
    /* Send negative response if needed */
    if ((result != DCM_E_OK) && (nrc != UDS_NRC_GENERAL_REJECT)) {
        (void)sendNegativeResponse(response, nrc);
    }
    
    return result;
}

Dcm_ReturnType Dcm_DefineDynamicDidByIdentifier(
    uint16_t dynamicDid,
    uint16_t sourceDid,
    uint16_t position,
    uint16_t size
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_dynDidState.initialized && Dcm_IsValidDynamicDidRange(dynamicDid)) {
        /* Check if DID already exists */
        Dcm_DynamicDidDefinitionType *definition = findDynamicDidDefinition(dynamicDid);
        
        if (definition == NULL) {
            /* Find free slot */
            definition = findFreeDefinitionSlot();
        }
        
        if (definition != NULL) {
            /* Initialize or update definition */
            if (!definition->defined) {
                s_dynDidState.numDefinedDids++;
            }
            
            definition->dynamicDid = dynamicDid;
            definition->defined = true;
            definition->numSourceElements = 1U;
            definition->sources[0U].type = DCM_DYN_SRC_TYPE_DID;
            definition->sources[0U].source.did.sourceDid = sourceDid;
            definition->sources[0U].source.did.position = position;
            definition->sources[0U].source.did.size = size;
            definition->totalSize = size;
            definition->definitionTime = 0U;  /* TODO: Get timestamp */
            
            result = DCM_E_OK;
        } else {
            /* No free slot */
            result = DCM_E_NOT_OK;
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_DefineDynamicDidByMemoryAddress(
    uint16_t dynamicDid,
    uint32_t memoryAddress,
    uint32_t memorySize
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_dynDidState.initialized && Dcm_IsValidDynamicDidRange(dynamicDid)) {
        /* Check if DID already exists */
        Dcm_DynamicDidDefinitionType *definition = findDynamicDidDefinition(dynamicDid);
        
        if (definition == NULL) {
            /* Find free slot */
            definition = findFreeDefinitionSlot();
        }
        
        if (definition != NULL) {
            /* Initialize or update definition */
            if (!definition->defined) {
                s_dynDidState.numDefinedDids++;
            }
            
            definition->dynamicDid = dynamicDid;
            definition->defined = true;
            definition->numSourceElements = 1U;
            definition->sources[0U].type = DCM_DYN_SRC_TYPE_MEMORY;
            definition->sources[0U].source.mem.memoryAddress = memoryAddress;
            definition->sources[0U].source.mem.memorySize = memorySize;
            definition->totalSize = (uint16_t)memorySize;
            definition->definitionTime = 0U;  /* TODO: Get timestamp */
            
            result = DCM_E_OK;
        } else {
            /* No free slot */
            result = DCM_E_NOT_OK;
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_ClearDynamicDid(uint16_t dynamicDid)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_dynDidState.initialized) {
        if (dynamicDid == DCM_ALL_DYNAMIC_DIDS) {
            /* Clear all dynamic DIDs */
            for (uint8_t i = 0U; i < DCM_MAX_DYNAMIC_DIDS; i++) {
                s_dynDidState.definitions[i].defined = false;
                s_dynDidState.definitions[i].dynamicDid = 0U;
                s_dynDidState.definitions[i].numSourceElements = 0U;
            }
            s_dynDidState.numDefinedDids = 0U;
            result = DCM_E_OK;
        } else {
            /* Clear specific DID */
            Dcm_DynamicDidDefinitionType *definition = findDynamicDidDefinition(dynamicDid);
            
            if (definition != NULL) {
                definition->defined = false;
                definition->dynamicDid = 0U;
                definition->numSourceElements = 0U;
                
                if (s_dynDidState.numDefinedDids > 0U) {
                    s_dynDidState.numDefinedDids--;
                }
                
                result = DCM_E_OK;
            } else {
                /* DID not found */
                result = DCM_E_REQUEST_OUT_OF_RANGE;
            }
        }
    }
    
    return result;
}

bool Dcm_IsDynamicDidDefined(uint16_t dynamicDid)
{
    bool defined = false;
    
    if (s_dynDidState.initialized) {
        Dcm_DynamicDidDefinitionType *definition = findDynamicDidDefinition(dynamicDid);
        defined = (definition != NULL) && definition->defined;
    }
    
    return defined;
}

Dcm_ReturnType Dcm_GetDynamicDidDefinition(
    uint16_t dynamicDid,
    Dcm_DynamicDidDefinitionType **definition
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((definition != NULL) && s_dynDidState.initialized) {
        *definition = findDynamicDidDefinition(dynamicDid);
        
        if ((*definition != NULL) && (*definition)->defined) {
            result = DCM_E_OK;
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_ReadDynamicDidData(
    uint16_t dynamicDid,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((dataBuffer != NULL) && (dataLength != NULL) && s_dynDidState.initialized) {
        Dcm_DynamicDidDefinitionType *definition = findDynamicDidDefinition(dynamicDid);
        
        if ((definition != NULL) && definition->defined) {
            if (bufferSize >= definition->totalSize) {
                /* For now, return placeholder data */
                /* In full implementation, this would read from source DID or memory */
                (void)memset(dataBuffer, 0, definition->totalSize);
                *dataLength = definition->totalSize;
                result = DCM_E_OK;
            } else {
                /* Buffer too small */
                result = DCM_E_RESPONSE_BUFFER_TOO_SMALL;
            }
        } else {
            result = DCM_E_REQUEST_OUT_OF_RANGE;
        }
    }
    
    return result;
}

bool Dcm_IsValidDynamicDidRange(uint16_t did)
{
    bool valid = false;
    
    if (s_dynDidState.config != NULL) {
        if ((did >= s_dynDidState.config->minDynamicDid) &&
            (did <= s_dynDidState.config->maxDynamicDid)) {
            valid = true;
        }
    } else {
        /* Use default range */
        if ((did >= DCM_DYN_DID_MIN_DID) && (did <= DCM_DYN_DID_MAX_DID)) {
            valid = true;
        }
    }
    
    return valid;
}

Dcm_ReturnType Dcm_GetDynamicDidStatus(Dcm_DynamicDidStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((status != NULL) && s_dynDidState.initialized) {
        status->numDefinedDids = s_dynDidState.numDefinedDids;
        
        uint8_t index = 0U;
        for (uint8_t i = 0U; (i < DCM_MAX_DYNAMIC_DIDS) && (index < DCM_MAX_DYNAMIC_DIDS); i++) {
            if (s_dynDidState.definitions[i].defined) {
                status->definedDids[index] = s_dynDidState.definitions[i].dynamicDid;
                index++;
            }
        }
        
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsDefinitionModeSupported(uint8_t definitionMode)
{
    bool supported = false;
    
    if (s_dynDidState.config != NULL) {
        switch (definitionMode) {
            case DCM_DYN_DID_DEFINE_BY_IDENTIFIER:
                supported = s_dynDidState.config->enableDidBasedDefinition;
                break;
            case DCM_DYN_DID_DEFINE_BY_MEMORY_ADDRESS:
                supported = s_dynDidState.config->enableMemoryBasedDefinition;
                break;
            case DCM_DYN_DID_CLEAR_DYN_DID:
                supported = s_dynDidState.config->enableClearDynamicDid;
                break;
            default:
                supported = false;
                break;
        }
    } else {
        /* Default: all modes supported */
        if ((definitionMode >= DCM_DYN_DID_DEFINE_BY_IDENTIFIER) &&
            (definitionMode <= DCM_DYN_DID_CLEAR_DYN_DID)) {
            supported = true;
        }
    }
    
    return supported;
}

Dcm_ReturnType Dcm_ParseAddressLengthFormat(
    uint8_t formatId,
    uint8_t *addressLength,
    uint8_t *sizeLength
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((addressLength != NULL) && (sizeLength != NULL)) {
        uint8_t addrLen = (formatId & DCM_DYN_DID_ADDR_LEN_MASK) >> DCM_DYN_DID_ADDR_LEN_SHIFT;
        uint8_t szLen = formatId & DCM_DYN_DID_SIZE_LEN_MASK;
        
        /* Convert to actual bytes (1, 2, or 4) */
        if (addrLen == 0x1U) {
            *addressLength = 1U;
        } else if (addrLen == 0x2U) {
            *addressLength = 2U;
        } else if (addrLen == 0x4U) {
            *addressLength = 4U;
        } else {
            *addressLength = 0U;
        }
        
        if (szLen == 0x1U) {
            *sizeLength = 1U;
        } else if (szLen == 0x2U) {
            *sizeLength = 2U;
        } else if (szLen == 0x4U) {
            *sizeLength = 4U;
        } else {
            *sizeLength = 0U;
        }
        
        /* Validate */
        if ((*addressLength != 0U) && (*sizeLength != 0U)) {
            result = DCM_E_OK;
        }
    }
    
    return result;
}

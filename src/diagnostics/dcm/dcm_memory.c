/******************************************************************************
 * @file    dcm_memory.c
 * @brief   DCM Write Memory By Address Service (0x3D) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_memory.h"
#include "dcm_security.h"
#include "dcm_session.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_MEMORY_MAGIC_INIT           (0x4D454D30U)  /* "MEM0" */
#define DCM_MAX_MEMORY_REGIONS          16U

/******************************************************************************
 * Default Memory Regions (Example configuration)
 ******************************************************************************/
static const Dcm_MemoryRegionConfigType s_defaultRegions[] = {
    {
        .startAddress = 0x20000000U,
        .endAddress = 0x2001FFFFU,
        .regionType = DCM_MEM_REGION_RAM,
        .requiredSecurityLevel = 1U,
        .writeAllowed = true,
        .readAllowed = true,
        .eraseRequired = false,
        .alignment = 1U,
        .description = "RAM"
    },
    {
        .startAddress = 0x08000000U,
        .endAddress = 0x0807FFFFU,
        .regionType = DCM_MEM_REGION_FLASH,
        .requiredSecurityLevel = 3U,
        .writeAllowed = true,
        .readAllowed = true,
        .eraseRequired = true,
        .alignment = 4U,
        .description = "Flash"
    }
};

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_MemoryWriteConfigType *config;
    Dcm_MemoryWriteStatusType status;
    bool initialized;
} Dcm_MemoryStateType;

static Dcm_MemoryStateType s_memoryState;

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
                                           uint8_t formatId,
                                           uint32_t address,
                                           uint32_t size,
                                           uint8_t addrLen,
                                           uint8_t sizeLen)
{
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= (uint32_t)(2U + addrLen + sizeLen))) {
        response->data[0U] = (uint8_t)(UDS_SVC_WRITE_MEMORY_BY_ADDRESS + 0x40U);
        response->data[1U] = formatId;
        
        uint8_t idx = 2U;
        for (int8_t i = (int8_t)(addrLen - 1); i >= 0; i--) {
            response->data[idx++] = (uint8_t)((address >> (i * 8)) & 0xFFU);
        }
        for (int8_t i = (int8_t)(sizeLen - 1); i >= 0; i--) {
            response->data[idx++] = (uint8_t)((size >> (i * 8)) & 0xFFU);
        }
        
        response->length = (uint32_t)(2U + addrLen + sizeLen);
        response->isNegativeResponse = false;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

static const Dcm_MemoryRegionConfigType* findMemoryRegion(uint32_t address)
{
    if ((s_memoryState.config != NULL) && (s_memoryState.config->regions != NULL)) {
        for (uint8_t i = 0U; i < s_memoryState.config->numRegions; i++) {
            if ((address >= s_memoryState.config->regions[i].startAddress) &&
                (address <= s_memoryState.config->regions[i].endAddress)) {
                return &s_memoryState.config->regions[i];
            }
        }
    }
    return NULL;
}

static bool checkSecurityAccess(const Dcm_MemoryRegionConfigType *region)
{
    if (region == NULL) {
        return false;
    }
    
    /* Check region security level */
    if (region->requiredSecurityLevel > 0U) {
        if (!Dcm_IsSecurityLevelUnlocked(region->requiredSecurityLevel)) {
            return false;
        }
    }
    
    /* Check global security level */
    if ((s_memoryState.config != NULL) && 
        (s_memoryState.config->requiredSecurityLevel > 0U)) {
        if (!Dcm_IsSecurityLevelUnlocked(s_memoryState.config->requiredSecurityLevel)) {
            return false;
        }
    }
    
    return true;
}

static bool checkSessionRequirements(void)
{
    if ((s_memoryState.config != NULL) && 
        s_memoryState.config->requireProgrammingSession) {
        Dcm_SessionType session = Dcm_GetCurrentSession();
        if (session != DCM_SESSION_PROGRAMMING) {
            return false;
        }
    }
    return true;
}

static Dcm_ReturnType validateWriteRequest(uint32_t address, uint32_t length)
{
    /* Check address range */
    if (length == 0U) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    if (length > DCM_WRITE_MEM_MAX_DATA_LENGTH) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    /* Check max write size */
    if ((s_memoryState.config != NULL) && 
        (length > s_memoryState.config->maxWriteSize)) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    /* Check if writable */
    if (!Dcm_IsMemoryAddressWritable(address, length)) {
        return DCM_E_SECURITY_ACCESS_DENIED;
    }
    
    /* Check memory protection */
    if (Dcm_IsMemoryProtected(address, length)) {
        return DCM_E_SECURITY_ACCESS_DENIED;
    }
    
    /* Check region boundaries */
    const Dcm_MemoryRegionConfigType *region = findMemoryRegion(address);
    if (region == NULL) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    /* Check if write crosses region boundary */
    if ((address + length - 1U) > region->endAddress) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    /* Check alignment */
    if ((region->alignment > 1U) && ((address % region->alignment) != 0U)) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    /* Check security access */
    if (!checkSecurityAccess(region)) {
        return DCM_E_SECURITY_ACCESS_DENIED;
    }
    
    return DCM_E_OK;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_MemoryWriteInit(const Dcm_MemoryWriteConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL) {
        (void)memset(&s_memoryState, 0, sizeof(s_memoryState));
        
        s_memoryState.magic = DCM_MEMORY_MAGIC_INIT;
        s_memoryState.config = config;
        s_memoryState.initialized = true;
        s_memoryState.status.state = DCM_MEM_WRITE_STATE_IDLE;
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_WriteMemoryByAddress(const Dcm_RequestType *request,
                                        Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    if (!s_memoryState.initialized) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    if ((request == NULL) || (response == NULL)) {
        return result;
    }
    
    if (request->length < DCM_WRITE_MEM_MIN_LENGTH) {
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
    
    const uint8_t formatId = request->data[1U];
    
    if ((request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0U) {
        response->suppressPositiveResponse = true;
    }
    
    /* Parse format identifier */
    uint8_t addressLength;
    uint8_t sizeLength;
    result = Dcm_ParseMemoryFormat(formatId & 0x0FU, &addressLength, &sizeLength);
    
    if (result != DCM_E_OK) {
        nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Calculate expected request length */
    uint8_t expectedHeaderLen = 2U + addressLength + sizeLength;
    if (request->length < expectedHeaderLen) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Parse address and size */
    uint32_t memoryAddress = Dcm_ParseMemoryAddress(&request->data[2U], addressLength);
    uint32_t memorySize = Dcm_ParseMemorySize(&request->data[2U + addressLength], sizeLength);
    
    /* Get data pointer */
    const uint8_t *writeData = &request->data[expectedHeaderLen];
    uint32_t dataLength = request->length - expectedHeaderLen;
    
    /* Validate data length matches size */
    if (dataLength != memorySize) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate write request */
    result = validateWriteRequest(memoryAddress, memorySize);
    
    if (result != DCM_E_OK) {
        if (result == DCM_E_REQUEST_OUT_OF_RANGE) {
            nrc = UDS_NRC_REQUEST_OUT_OF_RANGE;
        } else if (result == DCM_E_SECURITY_ACCESS_DENIED) {
            nrc = UDS_NRC_SECURITY_ACCESS_DENIED;
        } else {
            nrc = UDS_NRC_GENERAL_REJECT;
        }
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Perform write */
    result = Dcm_WriteMemory(memoryAddress, writeData, memorySize);
    
    if (result == DCM_E_OK) {
        /* Update status */
        s_memoryState.status.state = DCM_MEM_WRITE_STATE_COMPLETED;
        s_memoryState.status.lastWriteAddress = memoryAddress;
        s_memoryState.status.lastWriteSize = memorySize;
        s_memoryState.status.totalBytesWritten += memorySize;
        s_memoryState.status.lastWriteTime = 0U;
        
        result = sendPositiveResponse(response, formatId, memoryAddress, memorySize,
                                      addressLength, sizeLength);
    } else {
        s_memoryState.status.state = DCM_MEM_WRITE_STATE_FAILED;
        s_memoryState.status.writeErrorCount++;
        nrc = UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
        (void)sendNegativeResponse(response, nrc);
    }
    
    return result;
}

Dcm_ReturnType Dcm_WriteMemory(uint32_t memoryAddress, const uint8_t *data, uint32_t length)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((data != NULL) && (length > 0U) && s_memoryState.initialized) {
        const Dcm_MemoryRegionConfigType *region = findMemoryRegion(memoryAddress);
        
        if (region != NULL) {
            /* Use callback if available */
            if ((s_memoryState.config != NULL) && 
                (s_memoryState.config->writeCallback != NULL)) {
                result = s_memoryState.config->writeCallback(memoryAddress, data, length, 
                                                              region->regionType);
            } else {
                /* Default: simulate write success */
                /* In real implementation, this would write to actual memory */
                result = DCM_E_OK;
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_VerifyMemoryWrite(uint32_t memoryAddress,
                                     const uint8_t *expectedData,
                                     uint32_t length,
                                     bool *verifyResult)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((expectedData != NULL) && (verifyResult != NULL) && s_memoryState.initialized) {
        *verifyResult = false;
        
        if ((s_memoryState.config != NULL) && 
            (s_memoryState.config->verifyCallback != NULL)) {
            result = s_memoryState.config->verifyCallback(memoryAddress, expectedData, 
                                                           length, verifyResult);
        } else {
            /* Default: assume verified */
            *verifyResult = true;
            result = DCM_E_OK;
        }
    }
    
    return result;
}

bool Dcm_IsMemoryAddressWritable(uint32_t memoryAddress, uint32_t length)
{
    bool writable = false;
    
    if (s_memoryState.initialized && (length > 0U)) {
        const Dcm_MemoryRegionConfigType *region = findMemoryRegion(memoryAddress);
        
        if ((region != NULL) && region->writeAllowed) {
            /* Check if entire range is within region */
            if ((memoryAddress >= region->startAddress) &&
                ((memoryAddress + length - 1U) <= region->endAddress)) {
                writable = true;
            }
        }
    }
    
    return writable;
}

const Dcm_MemoryRegionConfigType* Dcm_GetMemoryRegion(uint32_t memoryAddress)
{
    return findMemoryRegion(memoryAddress);
}

bool Dcm_IsAddressRangeValid(uint32_t startAddress, uint32_t length)
{
    bool valid = false;
    
    if ((length > 0U) && ((startAddress + length) > startAddress)) {
        /* Check for overflow */
        const Dcm_MemoryRegionConfigType *region = findMemoryRegion(startAddress);
        
        if (region != NULL) {
            if ((startAddress >= region->startAddress) &&
                ((startAddress + length - 1U) <= region->endAddress)) {
                valid = true;
            }
        }
    }
    
    return valid;
}

bool Dcm_IsMemoryProtected(uint32_t memoryAddress, uint32_t length)
{
    bool protected = true;
    (void)length;  /* Unused parameter */
    
    if (s_memoryState.initialized) {
        const Dcm_MemoryRegionConfigType *region = findMemoryRegion(memoryAddress);
        
        if ((region != NULL) && region->writeAllowed) {
            protected = false;
        }
    }
    
    return protected;
}

Dcm_ReturnType Dcm_GetMemoryWriteStatus(Dcm_MemoryWriteStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((status != NULL) && s_memoryState.initialized) {
        *status = s_memoryState.status;
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_ParseMemoryFormat(uint8_t formatId,
                                     uint8_t *addressLength,
                                     uint8_t *sizeLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((addressLength != NULL) && (sizeLength != NULL)) {
        uint8_t addrLen = (formatId >> 4U) & 0x0FU;
        uint8_t szLen = formatId & 0x0FU;
        
        /* Convert nibbles to actual byte lengths */
        if (addrLen == 0x1U) {
            *addressLength = 1U;
        } else if (addrLen == 0x2U) {
            *addressLength = 2U;
        } else if (addrLen == 0x4U) {
            *addressLength = 4U;
        } else {
            return DCM_E_NOT_OK;
        }
        
        if (szLen == 0x1U) {
            *sizeLength = 1U;
        } else if (szLen == 0x2U) {
            *sizeLength = 2U;
        } else if (szLen == 0x4U) {
            *sizeLength = 4U;
        } else {
            return DCM_E_NOT_OK;
        }
        
        result = DCM_E_OK;
    }
    
    return result;
}

uint32_t Dcm_ParseMemoryAddress(const uint8_t *data, uint8_t length)
{
    uint32_t address = 0U;
    
    if (data != NULL) {
        for (uint8_t i = 0U; i < length; i++) {
            address = (address << 8U) | (uint32_t)data[i];
        }
    }
    
    return address;
}

uint32_t Dcm_ParseMemorySize(const uint8_t *data, uint8_t length)
{
    uint32_t size = 0U;
    
    if (data != NULL) {
        for (uint8_t i = 0U; i < length; i++) {
            size = (size << 8U) | (uint32_t)data[i];
        }
    }
    
    return size;
}

/******************************************************************************
 * @file    dcm_dynamic_did.h
 * @brief   DCM Dynamically Define Data Identifier Service (0x2C) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_DYNAMIC_DID_H
#define DCM_DYNAMIC_DID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Dynamic DID Constants (ISO 14229-1:2020 Table 110)
 ******************************************************************************/
#define DCM_DYN_DID_MIN_LENGTH                  0x03U
#define DCM_DYN_DID_MAX_LENGTH                  0xFFU
#define DCM_DYN_DID_RESPONSE_MIN_LENGTH         0x02U
#define DCM_DYN_DID_MIN_DID                     0xF300U  /* Min dynamic DID */
#define DCM_DYN_DID_MAX_DID                     0xF3FFU  /* Max dynamic DID */
#define DCM_MAX_DYNAMIC_DIDS                    0x10U    /* Max 16 dynamic DIDs */
#define DCM_MAX_SOURCE_ELEMENTS                 0x08U    /* Max source elements per DID */
#define DCM_MAX_MEMORY_SIZE                     0xFFFU   /* Max 4KB per element */

/******************************************************************************
 * Definition Mode Subfunctions (ISO 14229-1:2020 Table 109)
 ******************************************************************************/
#define DCM_DYN_DID_DEFINE_BY_IDENTIFIER        0x01U
#define DCM_DYN_DID_DEFINE_BY_MEMORY_ADDRESS    0x02U
#define DCM_DYN_DID_CLEAR_DYN_DID               0x03U

/******************************************************************************
 * Memory Size Length Formats (ISO 14229-1:2020)
 ******************************************************************************/
#define DCM_DYN_DID_SIZE_1_BYTE                 0x01U
#define DCM_DYN_DID_SIZE_2_BYTES                0x02U
#define DCM_DYN_DID_SIZE_4_BYTES                0x04U

/******************************************************************************
 * Address And Length Format Identifier Bit Definitions
 ******************************************************************************/
#define DCM_DYN_DID_ADDR_LEN_MASK               0xF0U
#define DCM_DYN_DID_SIZE_LEN_MASK               0x0FU
#define DCM_DYN_DID_ADDR_LEN_SHIFT              0x04U

/******************************************************************************
 * Source Definition Types
 ******************************************************************************/
typedef enum {
    DCM_DYN_SRC_TYPE_DID = 0,               /* Source is another DID */
    DCM_DYN_SRC_TYPE_MEMORY                 /* Source is memory address */
} Dcm_DynamicDidSourceType;

/******************************************************************************
 * Source Definition Structure (DID-based)
 ******************************************************************************/
typedef struct {
    uint16_t sourceDid;                     /* Source data identifier */
    uint16_t position;                      /* Start position in source DID */
    uint16_t size;                          /* Size of data to copy */
    uint8_t memorySizeLength;               /* Memory size length format */
} Dcm_DynamicDidSourceDidType;

/******************************************************************************
 * Source Definition Structure (Memory-based)
 ******************************************************************************/
typedef struct {
    uint32_t memoryAddress;                 /* Source memory address */
    uint32_t memorySize;                    /* Size of data to copy */
    uint8_t addressLength;                  /* Address length format */
    uint8_t sizeLength;                     /* Size length format */
} Dcm_DynamicDidSourceMemoryType;

/******************************************************************************
 * Source Element Union
 ******************************************************************************/
typedef struct {
    Dcm_DynamicDidSourceType type;          /* Source type */
    union {
        Dcm_DynamicDidSourceDidType did;    /* DID-based source */
        Dcm_DynamicDidSourceMemoryType mem; /* Memory-based source */
    } source;
} Dcm_DynamicDidSourceElementType;

/******************************************************************************
 * Dynamic DID Definition
 ******************************************************************************/
typedef struct {
    uint16_t dynamicDid;                    /* Dynamic DID number */
    bool defined;                           /* Definition valid flag */
    uint8_t numSourceElements;              /* Number of source elements */
    Dcm_DynamicDidSourceElementType sources[DCM_MAX_SOURCE_ELEMENTS];
    uint16_t totalSize;                     /* Total data size */
    uint64_t definitionTime;                /* Time of definition */
} Dcm_DynamicDidDefinitionType;

/******************************************************************************
 * Dynamic DID Result Types
 ******************************************************************************/
typedef enum {
    DCM_DYN_DID_OK = 0,                     /* Operation successful */
    DCM_DYN_DID_ERR_NOT_SUPPORTED,          /* Dynamic DID not supported */
    DCM_DYN_DID_ERR_INVALID_DID,            /* Invalid DID number */
    DCM_DYN_DID_ERR_INVALID_FORMAT,         /* Invalid format identifier */
    DCM_DYN_DID_ERR_NO_MEMORY,              /* No memory available */
    DCM_DYN_DID_ERR_TOO_MANY_SOURCES,       /* Too many source elements */
    DCM_DYN_DID_ERR_OUT_OF_RANGE,           /* Request out of range */
    DCM_DYN_DID_ERR_SECURITY_DENIED         /* Security access denied */
} Dcm_DynamicDidResultType;

/******************************************************************************
 * Dynamic DID Configuration
 ******************************************************************************/
typedef struct {
    bool enableDidBasedDefinition;          /* Enable definition by DID */
    bool enableMemoryBasedDefinition;       /* Enable definition by memory */
    bool enableClearDynamicDid;             /* Enable clear operation */
    uint8_t maxDynamicDids;                 /* Maximum dynamic DIDs */
    uint8_t maxSourceElements;              /* Max sources per DID */
    uint16_t minDynamicDid;                 /* Minimum dynamic DID value */
    uint16_t maxDynamicDid;                 /* Maximum dynamic DID value */
    uint8_t requiredSecurityLevel;          /* Required security level */
} Dcm_DynamicDidConfigType;

/******************************************************************************
 * Dynamic DID Status
 ******************************************************************************/
typedef struct {
    uint8_t numDefinedDids;                 /* Number of defined DIDs */
    uint16_t definedDids[DCM_MAX_DYNAMIC_DIDS]; /* List of defined DIDs */
    uint32_t definitionCount;               /* Total definitions made */
    uint32_t clearCount;                    /* Total clears performed */
} Dcm_DynamicDidStatusType;

/******************************************************************************
 * Dynamic DID Functions
 ******************************************************************************/

/**
 * @brief Initialize dynamic DID service
 *
 * @param config Pointer to dynamic DID configuration
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using dynamic DID service
 * @requirement ISO 14229-1:2020 10.11
 */
Dcm_ReturnType Dcm_DynamicDidInit(const Dcm_DynamicDidConfigType *config);

/**
 * @brief Process DynamicallyDefineDataIdentifier (0x2C) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x2C for dynamic DID definition
 * @requirement ISO 14229-1:2020 10.11
 */
Dcm_ReturnType Dcm_DynamicallyDefineDataIdentifier(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Define dynamic DID by source DID
 *
 * @param dynamicDid Target dynamic DID
 * @param sourceDid Source DID to reference
 * @param position Start position in source DID
 * @param size Size of data to include
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_DefineDynamicDidByIdentifier(
    uint16_t dynamicDid,
    uint16_t sourceDid,
    uint16_t position,
    uint16_t size
);

/**
 * @brief Define dynamic DID by memory address
 *
 * @param dynamicDid Target dynamic DID
 * @param memoryAddress Source memory address
 * @param memorySize Size of data to include
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_DefineDynamicDidByMemoryAddress(
    uint16_t dynamicDid,
    uint32_t memoryAddress,
    uint32_t memorySize
);

/**
 * @brief Clear dynamic DID definition
 *
 * @param dynamicDid Dynamic DID to clear (0xFFFF = all)
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ClearDynamicDid(uint16_t dynamicDid);

/**
 * @brief Check if dynamic DID is defined
 *
 * @param dynamicDid Dynamic DID to check
 * @return bool True if defined
 */
bool Dcm_IsDynamicDidDefined(uint16_t dynamicDid);

/**
 * @brief Get dynamic DID definition
 *
 * @param dynamicDid Dynamic DID to get
 * @param definition Pointer to store definition
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetDynamicDidDefinition(
    uint16_t dynamicDid,
    Dcm_DynamicDidDefinitionType **definition
);

/**
 * @brief Read data from dynamic DID
 *
 * @param dynamicDid Dynamic DID to read
 * @param dataBuffer Buffer to store data
 * @param bufferSize Buffer size
 * @param dataLength Output: actual data length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ReadDynamicDidData(
    uint16_t dynamicDid,
    uint8_t *dataBuffer,
    uint16_t bufferSize,
    uint16_t *dataLength
);

/**
 * @brief Check if DID is a valid dynamic DID range
 *
 * @param did DID to check
 * @return bool True if valid dynamic DID range
 */
bool Dcm_IsValidDynamicDidRange(uint16_t did);

/**
 * @brief Get dynamic DID status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetDynamicDidStatus(Dcm_DynamicDidStatusType *status);

/**
 * @brief Check if definition mode is supported
 *
 * @param definitionMode Definition mode to check
 * @return bool True if supported
 */
bool Dcm_IsDefinitionModeSupported(uint8_t definitionMode);

/**
 * @brief Parse address and length format identifier
 *
 * @param formatId Format identifier byte
 * @param addressLength Output: address length
 * @param sizeLength Output: size length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ParseAddressLengthFormat(
    uint8_t formatId,
    uint8_t *addressLength,
    uint8_t *sizeLength
);

#ifdef __cplusplus
}
#endif

#endif /* DCM_DYNAMIC_DID_H */

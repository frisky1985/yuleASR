/******************************************************************************
 * @file    dcm_memory.h
 * @brief   DCM Write Memory By Address Service (0x3D) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_MEMORY_H
#define DCM_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Write Memory Constants (ISO 14229-1:2020 Table 168)
 ******************************************************************************/
#define DCM_WRITE_MEM_MIN_LENGTH                0x04U
#define DCM_WRITE_MEM_RESPONSE_LENGTH           0x04U
#define DCM_WRITE_MEM_MAX_DATA_LENGTH           0xFFFU  /* Max 4KB per write */

/******************************************************************************
 * Address And Length Format Identifier Constants
 ******************************************************************************/
#define DCM_MEM_ADDR_LEN_1_BYTE                 0x01U
#define DCM_MEM_ADDR_LEN_2_BYTES                0x02U
#define DCM_MEM_ADDR_LEN_4_BYTES                0x04U
#define DCM_MEM_SIZE_LEN_1_BYTE                 0x01U
#define DCM_MEM_SIZE_LEN_2_BYTES                0x02U
#define DCM_MEM_SIZE_LEN_4_BYTES                0x04U

/******************************************************************************
 * Memory Write States
 ******************************************************************************/
typedef enum {
    DCM_MEM_WRITE_STATE_IDLE = 0,           /* No write in progress */
    DCM_MEM_WRITE_STATE_PENDING,            /* Write pending verification */
    DCM_MEM_WRITE_STATE_COMPLETED,          /* Write completed */
    DCM_MEM_WRITE_STATE_FAILED              /* Write failed */
} Dcm_MemoryWriteStateType;

/******************************************************************************
 * Memory Write Result Types
 ******************************************************************************/
typedef enum {
    DCM_MEM_OK = 0,                         /* Operation successful */
    DCM_MEM_ERR_NOT_SUPPORTED,              /* Memory write not supported */
    DCM_MEM_ERR_INVALID_FORMAT,             /* Invalid format identifier */
    DCM_MEM_ERR_OUT_OF_RANGE,               /* Address or size out of range */
    DCM_MEM_ERR_SECURITY_DENIED,            /* Security access denied */
    DCM_MEM_ERR_PROTECTION,                 /* Memory protection violation */
    DCM_MEM_ERR_WRITE_FAILED,               /* Memory write failed */
    DCM_MEM_ERR_INVALID_LENGTH              /* Invalid data length */
} Dcm_MemoryWriteResultType;

/******************************************************************************
 * Memory Region Types
 ******************************************************************************/
typedef enum {
    DCM_MEM_REGION_RAM = 0,                 /* RAM region */
    DCM_MEM_REGION_FLASH,                   /* Flash/EEPROM region */
    DCM_MEM_REGION_REGISTER,                /* Hardware registers */
    DCM_MEM_REGION_RESERVED                 /* Reserved/protected */
} Dcm_MemoryRegionEnum;

/******************************************************************************
 * Memory Region Definition
 ******************************************************************************/
typedef struct {
    uint32_t startAddress;                  /* Region start address */
    uint32_t endAddress;                    /* Region end address */
    Dcm_MemoryRegionEnum regionType;        /* Region type */
    uint8_t requiredSecurityLevel;          /* Required security level */
    bool writeAllowed;                      /* Write access allowed */
    bool readAllowed;                       /* Read access allowed */
    bool eraseRequired;                     /* Erase before write required */
    uint32_t alignment;                     /* Write alignment requirement */
    const char *description;                /* Region description */
} Dcm_MemoryRegionConfigType;

/******************************************************************************
 * Memory Write Callback Type
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_MemoryWriteCallback)(
    uint32_t memoryAddress,
    const uint8_t *data,
    uint32_t length,
    Dcm_MemoryRegionEnum regionType
);

/******************************************************************************
 * Memory Verification Callback Type
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_MemoryVerifyCallback)(
    uint32_t memoryAddress,
    const uint8_t *expectedData,
    uint32_t length,
    bool *verifyResult
);

/******************************************************************************
 * Memory Write Configuration
 ******************************************************************************/
typedef struct {
    const Dcm_MemoryRegionConfigType *regions;    /* Memory regions array */
    uint8_t numRegions;                     /* Number of regions */
    uint32_t maxWriteSize;                  /* Maximum write size */
    bool enableVerification;                /* Enable write verification */
    bool requireProgrammingSession;         /* Require programming session */
    uint8_t requiredSecurityLevel;          /* Required security level */
    Dcm_MemoryWriteCallback writeCallback;  /* Write callback function */
    Dcm_MemoryVerifyCallback verifyCallback; /* Verify callback function */
} Dcm_MemoryWriteConfigType;

/******************************************************************************
 * Memory Write Request
 ******************************************************************************/
typedef struct {
    uint32_t memoryAddress;                 /* Target memory address */
    uint32_t memorySize;                    /* Write size */
    uint8_t addressLength;                  /* Address format length */
    uint8_t sizeLength;                     /* Size format length */
    const uint8_t *data;                    /* Data to write */
    uint32_t dataLength;                    /* Data length */
} Dcm_MemoryWriteRequestType;

/******************************************************************************
 * Memory Write Status
 ******************************************************************************/
typedef struct {
    Dcm_MemoryWriteStateType state;         /* Write state */
    uint32_t lastWriteAddress;              /* Last write address */
    uint32_t lastWriteSize;                 /* Last write size */
    uint32_t totalBytesWritten;             /* Total bytes written */
    uint32_t writeErrorCount;               /* Write error count */
    uint64_t lastWriteTime;                 /* Last write timestamp */
} Dcm_MemoryWriteStatusType;

/******************************************************************************
 * Memory Write Functions
 ******************************************************************************/

/**
 * @brief Initialize memory write service
 *
 * @param config Pointer to memory write configuration
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using memory write service
 * @requirement ISO 14229-1:2020 10.18
 */
Dcm_ReturnType Dcm_MemoryWriteInit(const Dcm_MemoryWriteConfigType *config);

/**
 * @brief Process WriteMemoryByAddress (0x3D) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x3D for memory writing
 * @requirement ISO 14229-1:2020 10.18
 */
Dcm_ReturnType Dcm_WriteMemoryByAddress(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Write data to memory address
 *
 * @param memoryAddress Target memory address
 * @param data Data to write
 * @param length Data length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_WriteMemory(
    uint32_t memoryAddress,
    const uint8_t *data,
    uint32_t length
);

/**
 * @brief Verify memory write
 *
 * @param memoryAddress Memory address to verify
 * @param expectedData Expected data
 * @param length Data length
 * @param verifyResult Output: verification result
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_VerifyMemoryWrite(
    uint32_t memoryAddress,
    const uint8_t *expectedData,
    uint32_t length,
    bool *verifyResult
);

/**
 * @brief Check if memory address is writable
 *
 * @param memoryAddress Memory address to check
 * @param length Write length
 * @return bool True if writable
 */
bool Dcm_IsMemoryAddressWritable(uint32_t memoryAddress, uint32_t length);

/**
 * @brief Get memory region for address
 *
 * @param memoryAddress Memory address
 * @return const Dcm_MemoryRegionConfigType* Region definition or NULL
 */
const Dcm_MemoryRegionConfigType* Dcm_GetMemoryRegion(uint32_t memoryAddress);

/**
 * @brief Check if address range is valid
 *
 * @param startAddress Start address
 * @param length Length to check
 * @return bool True if valid
 */
bool Dcm_IsAddressRangeValid(uint32_t startAddress, uint32_t length);

/**
 * @brief Check memory protection
 *
 * @param memoryAddress Memory address
 * @param length Write length
 * @return bool True if protected (not writable)
 */
bool Dcm_IsMemoryProtected(uint32_t memoryAddress, uint32_t length);

/**
 * @brief Get memory write status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetMemoryWriteStatus(Dcm_MemoryWriteStatusType *status);

/**
 * @brief Parse address and length format identifier
 *
 * @param formatId Format identifier byte
 * @param addressLength Output: address length
 * @param sizeLength Output: size length
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ParseMemoryFormat(
    uint8_t formatId,
    uint8_t *addressLength,
    uint8_t *sizeLength
);

/**
 * @brief Parse memory address from byte array
 *
 * @param data Byte array
 * @param length Array length
 * @return uint32_t Parsed address
 */
uint32_t Dcm_ParseMemoryAddress(const uint8_t *data, uint8_t length);

/**
 * @brief Parse memory size from byte array
 *
 * @param data Byte array
 * @param length Array length
 * @return uint32_t Parsed size
 */
uint32_t Dcm_ParseMemorySize(const uint8_t *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /* DCM_MEMORY_H */

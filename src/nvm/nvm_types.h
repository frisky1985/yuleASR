/**
 * @file nvm_types.h
 * @brief NvM (Non-Volatile Memory) Types Definition
 * @version 1.0.0
 * @date 2025
 * 
 * AUTOSAR NvM Module - ASIL-D Compliant
 * Copyright (c) 2025
 * 
 * This module provides non-volatile storage management following AUTOSAR specs.
 * Designed for automotive safety-critical applications (ASIL-D).
 */

#ifndef NVM_TYPES_H
#define NVM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*
 * Version Information
 *============================================================================*/
#define NVM_MAJOR_VERSION       1u
#define NVM_MINOR_VERSION       0u
#define NVM_PATCH_VERSION       0u

/*============================================================================*
 * Configuration Constants
 *============================================================================*/
#define NVM_MAX_BLOCK_COUNT     64u     /* Maximum number of NVM blocks */
#define NVM_MAX_JOB_QUEUE       32u     /* Maximum job queue size */
#define NVM_BLOCK_NAME_LEN      32u     /* Maximum block name length */
#define NVM_MAX_WRITE_RETRIES   3u      /* Maximum write retry attempts */
#define NVM_CRC32_POLYNOMIAL    0xEDB88320u  /* CRC32 IEEE polynomial */
#define NVM_MAGIC_NUMBER        0x4E564D55u  /* "NVMU" in hex */
#define NVM_VERSION_CURRENT     0x0100u /* Current NVM format version */

/* Page sizes for different storage types */
#define NVM_FLASH_PAGE_SIZE     256u    /* Flash page size */
#define NVM_EEPROM_PAGE_SIZE    8u      /* EEPROM page size */
#define NVM_MAX_PAGE_SIZE       256u    /* Maximum page size */

/*============================================================================*
 * Error Codes (ASIL-D Compliant)
 *============================================================================*/
typedef enum {
    NVM_OK                      = 0x00u,    /* Operation successful */
    NVM_E_NOT_OK                = 0x01u,    /* General error */
    NVM_E_BLOCK_INVALID         = 0x02u,    /* Invalid block ID */
    NVM_E_BLOCK_LOCKED          = 0x03u,    /* Block is locked/protected */
    NVM_E_BLOCK_CORRUPTED       = 0x04u,    /* Block data corrupted */
    NVM_E_WRITE_FAILED          = 0x05u,    /* Write operation failed */
    NVM_E_READ_FAILED           = 0x06u,    /* Read operation failed */
    NVM_E_ERASE_FAILED          = 0x07u,    /* Erase operation failed */
    NVM_E_VERIFY_FAILED         = 0x08u,    /* Write verification failed */
    NVM_E_CRC_FAILED            = 0x09u,    /* CRC check failed */
    NVM_E_MAGIC_FAILED          = 0x0Au,    /* Magic number mismatch */
    NVM_E_VERSION_MISMATCH      = 0x0Bu,    /* Version mismatch */
    NVM_E_QUEUE_FULL            = 0x0Cu,    /* Job queue full */
    NVM_E_QUEUE_EMPTY           = 0x0Du,    /* Job queue empty */
    NVM_E_TIMEOUT               = 0x0Eu,    /* Operation timeout */
    NVM_E_REDUNDANCY_FAILED     = 0x0Fu,    /* Redundancy check failed */
    NVM_E_RECOVERY_FAILED       = 0x10u,    /* Data recovery failed */
    NVM_E_STORAGE_NOT_READY     = 0x11u,    /* Storage not initialized */
    NVM_E_POWER_LOSS_DETECTED   = 0x12u,    /* Power loss detected */
    NVM_E_WEAR_LEVEL_CRITICAL   = 0x13u,    /* Wear level critical */
    NVM_E_OUT_OF_MEMORY         = 0x14u,    /* Out of memory */
    NVM_E_NOT_INITIALIZED       = 0x15u,    /* Module not initialized */
    NVM_E_ALREADY_INITIALIZED   = 0x16u,    /* Module already initialized */
    NVM_E_PARAM_POINTER         = 0x17u,    /* NULL pointer parameter */
    NVM_E_PARAM_RANGE           = 0x18u,    /* Parameter out of range */
    NVM_E_ASIL_VIOLATION        = 0x19u    /* ASIL safety violation */
} Nvm_ErrorCode_t;

/*============================================================================*
 * Block Types
 *============================================================================*/
typedef enum {
    NVM_BLOCK_TYPE_DATA         = 0x00u,    /* Regular data block */
    NVM_BLOCK_TYPE_CONFIG       = 0x01u,    /* Configuration block */
    NVM_BLOCK_TYPE_CALIBRATION  = 0x02u,    /* Calibration data */
    NVM_BLOCK_TYPE_DIAGNOSTIC   = 0x03u,    /* Diagnostic data */
    NVM_BLOCK_TYPE_SECURITY     = 0x04u,    /* Security data */
    NVM_BLOCK_TYPE_FACTORY      = 0x05u     /* Factory settings (read-only) */
} Nvm_BlockType_t;

/*============================================================================*
 * Protection Levels
 *============================================================================*/
typedef enum {
    NVM_PROT_NONE               = 0x00u,    /* No protection */
    NVM_PROT_WRITE              = 0x01u,    /* Write protected */
    NVM_PROT_ERASE              = 0x02u,    /* Erase protected */
    NVM_PROT_ALL                = 0x03u    /* Full protection */
} Nvm_Protection_t;

/*============================================================================*
 * Job Types
 *============================================================================*/
typedef enum {
    NVM_JOB_READ                = 0x01u,
    NVM_JOB_WRITE               = 0x02u,
    NVM_JOB_ERASE               = 0x04u,
    NVM_JOB_VERIFY              = 0x08u,
    NVM_JOB_RESTORE             = 0x10u,
    NVM_JOB_VALIDATE            = 0x20u,
    NVM_JOB_COMPACT             = 0x40u,
    NVM_JOB_RECOVER             = 0x80u
} Nvm_JobType_t;

/*============================================================================*
 * Job Priorities
 *============================================================================*/
typedef enum {
    NVM_PRIO_LOW                = 0u,       /* Background tasks */
    NVM_PRIO_NORMAL             = 1u,       /* Normal operations */
    NVM_PRIO_HIGH               = 2u,       /* Critical operations */
    NVM_PRIO_CRITICAL           = 3u        /* Safety-critical (ASIL-D) */
} Nvm_Priority_t;

/*============================================================================*
 * Storage Types
 *============================================================================*/
typedef enum {
    NVM_STORAGE_FLASH           = 0x00u,    /* Flash memory */
    NVM_STORAGE_EEPROM          = 0x01u,    /* EEPROM */
    NVM_STORAGE_EMULATED        = 0x02u,    /* Emulated EEPROM on Flash */
    NVM_STORAGE_EXTERNAL        = 0x03u     /* External storage */
} Nvm_StorageType_t;

/*============================================================================*
 * Block States
 *============================================================================*/
typedef enum {
    NVM_BLOCK_STATE_EMPTY       = 0x00u,    /* Block is empty */
    NVM_BLOCK_STATE_VALID       = 0x01u,    /* Block contains valid data */
    NVM_BLOCK_STATE_INVALID     = 0x02u,    /* Block data is invalid */
    NVM_BLOCK_STATE_CORRUPTED   = 0x03u,    /* Block data is corrupted */
    NVM_BLOCK_STATE_WRITING     = 0x04u,    /* Write in progress */
    NVM_BLOCK_STATE_ERASING     = 0x05u,    /* Erase in progress */
    NVM_BLOCK_STATE_RECOVERING  = 0x06u    /* Recovery in progress */
} Nvm_BlockState_t;

/*============================================================================*
 * Forward Declarations
 *============================================================================*/
struct Nvm_Block_s;
struct Nvm_Job_s;
struct Nvm_Context_s;

/*============================================================================*
 * Callback Types
 *============================================================================*/
typedef void (*Nvm_JobCallback_t)(
    uint16_t blockId,
    Nvm_JobType_t jobType,
    Nvm_ErrorCode_t result,
    void* userData
);

typedef Nvm_ErrorCode_t (*Nvm_StorageRead_t)(
    uint32_t address,
    uint8_t* data,
    uint32_t length
);

typedef Nvm_ErrorCode_t (*Nvm_StorageWrite_t)(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
);

typedef Nvm_ErrorCode_t (*Nvm_StorageErase_t)(
    uint32_t address,
    uint32_t length
);

typedef Nvm_ErrorCode_t (*Nvm_WearLevelGet_t)(
    uint32_t address,
    uint32_t* eraseCount
);

/*============================================================================*
 * Block Header (Stored in NVM)
 *============================================================================*/
typedef struct {
    uint32_t magic;                         /* Magic number for validation */
    uint16_t version;                       /* Data format version */
    uint16_t blockId;                       /* Block identifier */
    uint32_t dataLength;                    /* Actual data length */
    uint32_t sequence;                      /* Write sequence number */
    uint32_t crc32;                         /* CRC32 of header + data */
    uint32_t timestamp;                     /* Last modification timestamp */
    uint8_t  reserved[8];                   /* Reserved for future use */
} Nvm_BlockHeader_t;

#define NVM_BLOCK_HEADER_SIZE   (sizeof(Nvm_BlockHeader_t))

/*============================================================================*
 * Safety Monitors (ASIL-D)
 *============================================================================*/
typedef struct {
    uint32_t readCount;                     /* Number of read operations */
    uint32_t writeCount;                    /* Number of write operations */
    uint32_t errorCount;                    /* Cumulative error count */
    uint32_t retryCount;                    /* Retry count */
    uint32_t lastError;                     /* Last error code */
    uint32_t powerLossCount;                /* Power loss events detected */
    uint32_t recoveryCount;                 /* Recovery operations count */
} Nvm_SafetyMonitor_t;

#ifdef __cplusplus
}
#endif

#endif /* NVM_TYPES_H */

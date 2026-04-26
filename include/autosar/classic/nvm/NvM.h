/**
 * @file NvM.h
 * @brief AUTOSAR NvM (Non-Volatile Memory) Module Header
 * @version 4.4.0
 * @date 2025
 * 
 * AUTOSAR Classic Platform - NvM Module (Module ID: 0x0E)
 * 
 * The NvM module provides services to the application to let it
 * store data blocks in non-volatile memory (NVRAM).
 * 
 * Key Features:
 * - Block-based NVRAM management
 * - Task queue with priority scheduling
 * - State machine: IDLE -> BUSY -> PENDING -> IDLE
 * - Write protection with configurable windows
 * - Write retry mechanism (configurable retries)
 * - Read-after-write verification
 * - RAM block and ROM block management
 * - Incremental write protection (SetRamBlockStatus)
 * - CRC data integrity checks
 * - Module identification with Block ID + Checksum
 * 
 * Copyright (c) 2025
 */

#ifndef NVM_H
#define NVM_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 * Includes
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "NvM_Cfg.h"

/*============================================================================*
 * Module Version Information
 *============================================================================*/
#define NVM_MODULE_ID                   0x0Eu
#define NVM_VENDOR_ID                   0x00u
#define NVM_INSTANCE_ID                 0x00u

#define NVM_AR_RELEASE_MAJOR_VERSION    4u
#define NVM_AR_RELEASE_MINOR_VERSION    4u
#define NVM_AR_RELEASE_REVISION_VERSION 0u

#define NVM_SW_MAJOR_VERSION            1u
#define NVM_SW_MINOR_VERSION            0u
#define NVM_SW_PATCH_VERSION            0u

/*============================================================================*
 * Service IDs for Error Reporting
 *============================================================================*/
#define NVM_SID_INIT                    0x00u
#define NVM_SID_GETVERSIONINFO          0x0Fu
#define NVM_SID_READ_BLOCK              0x06u
#define NVM_SID_WRITE_BLOCK             0x07u
#define NVM_SID_RESTORE_BLOCK_DEFAULTS  0x08u
#define NVM_SID_ERASE_NV_BLOCK          0x09u
#define NVM_SID_INVALIDATE_NV_BLOCK     0x0Bu
#define NVM_SID_CANCEL_WRITE_ALL        0x0Cu
#define NVM_SID_READ_ALL                0x0Du
#define NVM_SID_WRITE_ALL               0x0Eu
#define NVM_SID_SET_RAM_BLOCK_STATUS    0x05u
#define NVM_SID_SET_BLOCK_PROTECTION    0x03u
#define NVM_SID_GET_ERROR_STATUS        0x04u
#define NVM_SID_SET_DATA_INDEX          0x01u
#define NVM_SID_GET_DATA_INDEX          0x02u

/*============================================================================*
 * Error Codes
 *============================================================================*/
#define NVM_E_NO_ERROR                  0x00u
#define NVM_E_NOT_INITIALIZED           0x01u
#define NVM_E_BLOCK_PENDING             0x02u
#define NVM_E_BLOCK_CONFIG              0x03u
#define NVM_E_PARAM_BLOCK_ID            0x04u
#define NVM_E_PARAM_BLOCK_TYPE          0x05u
#define NVM_E_PARAM_DATA_INDEX          0x06u
#define NVM_E_PARAM_POINTER             0x07u
#define NVM_E_BLOCK_LOCKED              0x08u
#define NVM_E_WRITE_PROTECTED           0x09u
#define NVM_E_VERIFY_FAILED             0x0Au
#define NVM_E_WRITE_TO_ROM_BLOCK        0x0Bu
#define NVM_E_QUEUE_FULL                0x0Cu
#define NVM_E_QUEUE_OVERFLOW            0x0Du
#define NVM_E_REQ_FAILED                0x0Eu

/*============================================================================*
 * Type Definitions
 *============================================================================*/

/**
 * @brief NvM Request Result Type
 * @details Provides the result status of the last asynchronous request
 */
typedef enum {
    NVM_REQ_OK = 0,                 /*!< Last request successful */
    NVM_REQ_NOT_OK = 1,             /*!< Last request failed */
    NVM_REQ_PENDING = 2,            /*!< Request still pending */
    NVM_REQ_INTEGRITY_FAILED = 3,   /*!< Data integrity check failed */
    NVM_REQ_BLOCK_SKIPPED = 4,      /*!< Block skipped (config) */
    NVM_REQ_NV_INVALIDATED = 5,     /*!< NV block invalidated */
    NVM_REQ_CANCELLED = 6,          /*!< Request cancelled */
    NVM_REQ_RESTORED_FROM_ROM = 7,  /*!< Restored from ROM defaults */
    NVM_REQ_REDUNDANCY_FAILED = 8,  /*!< Redundancy check failed */
    NVM_REQ_POWER_LOSS_DETECTED = 9 /*!< Power loss during operation */
} NvM_RequestResultType;

/**
 * @brief NvM Block Management Type
 * @details Defines the management type for a NVRAM block
 */
typedef enum {
    NVM_BLOCK_NATIVE = 0,           /*!< Native NVRAM block */
    NVM_BLOCK_REDUNDANT = 1,        /*!< Redundant NVRAM block */
    NVM_BLOCK_DATASET = 2           /*!< Dataset NVRAM block */
} NvM_BlockManagementType;

/**
 * @brief NvM Block State Type
 * @details Internal state machine states
 */
typedef enum {
    NVM_STATE_UNINIT = 0,           /*!< Module uninitialized */
    NVM_STATE_IDLE = 1,             /*!< Idle state */
    NVM_STATE_BUSY = 2,             /*!< Processing request */
    NVM_STATE_PENDING = 3,          /*!< Waiting for MemIf */
    NVM_STATE_WAIT_FOR_CALLBACK = 4 /*!< Waiting for callback completion */
} NvM_StateType;

/**
 * @brief NvM Job Type
 * @details Types of jobs in the queue
 */
typedef enum {
    NVM_JOB_TYPE_NONE = 0,          /*!< No job */
    NVM_JOB_TYPE_READ = 1,          /*!< Read job */
    NVM_JOB_TYPE_WRITE = 2,         /*!< Write job */
    NVM_JOB_TYPE_ERASE = 3,         /*!< Erase job */
    NVM_JOB_TYPE_VALIDATE = 4,      /*!< Validation job */
    NVM_JOB_TYPE_RESTORE = 5,       /*!< Restore defaults job */
    NVM_JOB_TYPE_READ_ALL = 6,      /*!< Read all job */
    NVM_JOB_TYPE_WRITE_ALL = 7      /*!< Write all job */
} NvM_JobTypeType;

/**
 * @brief NvM Job Priority Type
 */
typedef enum {
    NVM_PRIORITY_LOW = 0,           /*!< Low priority (background) */
    NVM_PRIORITY_NORMAL = 1,        /*!< Normal priority */
    NVM_PRIORITY_HIGH = 2,          /*!< High priority (immediate) */
    NVM_PRIORITY_CRITICAL = 3       /*!< Critical priority (ASIL) */
} NvM_PriorityType;

/**
 * @brief NvM CRC Type
 */
typedef enum {
    NVM_CRC_NONE = 0,               /*!< No CRC */
    NVM_CRC_8 = 1,                  /*!< CRC-8 */
    NVM_CRC_16 = 2,                 /*!< CRC-16 */
    NVM_CRC_32 = 3                  /*!< CRC-32 */
} NvM_CrcType;

/*============================================================================*
 * Block Descriptor Type
 *============================================================================*/
typedef struct {
    uint16_t                NvBlockBaseNumber;      /*!< Block base number */
    uint16_t                NvBlockLength;          /*!< Block length in bytes */
    uint8_t                 NvBlockNum;             /*!< Number of NV blocks */
    uint8_t                 RomBlockNum;            /*!< Number of ROM blocks */
    NvM_BlockManagementType BlockManagementType;    /*!< Management type */
    boolean                 BlockWriteProt;         /*!< Write protection flag */
    boolean                 WriteBlockOnce;         /*!< Write once flag */
    boolean                 SelectBlockForReadall;  /*!< Include in ReadAll */
    boolean                 SelectBlockForWriteall; /*!< Include in WriteAll */
    boolean                 BswMBlockStatusInformation; /*!< Report to BswM */
    boolean                 CalcRamBlockCrc;        /*!< Calculate RAM CRC */
    boolean                 WriteVerification;      /*!< Enable write verification */
    boolean                 StaticBlockIDCheck;     /*!< Enable block ID check */
    NvM_CrcType             CrcType;                /*!< CRC type */
    uint8_t                 NvramDeviceId;          /*!< Memory device ID */
    uint8_t                 MaxNumOfWriteRetries;   /*!< Max write retries */
    const void*             RomBlockDataAddr;       /*!< ROM block data address */
    void*                   RamBlockDataAddr;       /*!< Permanent RAM address */
    void (*NvMBlockCallback)(uint8_t ServiceId, NvM_RequestResultType JobResult);
} NvM_BlockDescriptorType;

/*============================================================================*
 * Configuration Type
 *============================================================================*/
typedef struct {
    const NvM_BlockDescriptorType*  BlockDescriptorTable;
    uint16_t                        NumOfBlocks;
    uint16_t                        CommonCrcBlockBaseNumber;
    uint8_t                         MaxNumOfWriteRetries;
    uint16_t                        SizeOfJobQueue;
    boolean                         SetRamBlockStatusApi;
    boolean                         EnableWriteProtection;
    uint32_t                        WriteProtectionWindow; /* in ms */
    uint8_t                         MainFunctionCycleTime; /* in ms */
} NvM_ConfigType;

/*============================================================================*
 * External Configuration
 *============================================================================*/
extern const NvM_ConfigType NvM_Config;

/*============================================================================*
 * Public API Functions
 *============================================================================*/

/**
 * @brief Initializes the NvM module
 * @param ConfigPtr Pointer to configuration (NULL if using generated config)
 * @pre None
 * @post Module initialized and ready for operation
 * @note Must be called during EcuM Init One phase
 */
extern void NvM_Init(const NvM_ConfigType* ConfigPtr);

/**
 * @brief Gets the version information of the NvM module
 * @param Versioninfo Pointer to version info structure
 */
extern void NvM_GetVersionInfo(Std_VersionInfoType* Versioninfo);

/**
 * @brief Reads data from NV block into RAM block
 * @param BlockId Block identifier (0 to configured max)
 * @param NvM_DstPtr Pointer to destination buffer (NULL = permanent RAM)
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 * @note Operation is asynchronous, check status via callback
 */
extern Std_ReturnType NvM_ReadBlock(
    NvM_BlockIdType BlockId,
    void* NvM_DstPtr
);

/**
 * @brief Writes data from RAM block to NV block
 * @param BlockId Block identifier
 * @param NvM_SrcPtr Pointer to source buffer (NULL = permanent RAM)
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 */
extern Std_ReturnType NvM_WriteBlock(
    NvM_BlockIdType BlockId,
    const void* NvM_SrcPtr
);

/**
 * @brief Restores the default data from ROM to RAM
 * @param BlockId Block identifier
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 */
extern Std_ReturnType NvM_RestoreBlockDefaults(
    NvM_BlockIdType BlockId,
    void* NvM_DstPtr
);

/**
 * @brief Erases a NV block
 * @param BlockId Block identifier
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 */
extern Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId);

/**
 * @brief Invalidates a NV block
 * @param BlockId Block identifier
 * @return E_OK: Request accepted, E_NOT_OK: Request rejected
 */
extern Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId);

/**
 * @brief Cancels an ongoing WriteAll request
 * @return None
 */
extern void NvM_CancelWriteAll(void);

/**
 * @brief Initiates a multi-block read request
 * @return None
 * @note Reads all blocks with SelectBlockForReadall = TRUE
 */
extern void NvM_ReadAll(void);

/**
 * @brief Initiates a multi-block write request
 * @return None
 * @note Writes all blocks with SelectBlockForWriteall = TRUE
 */
extern void NvM_WriteAll(void);

/**
 * @brief Sets the RAM block status for incremental write protection
 * @param BlockId Block identifier
 * @param BlockChanged TRUE if RAM block data has changed
 * @return E_OK: Success, E_NOT_OK: Failure
 * @note Used for incremental storage of modified data
 */
extern Std_ReturnType NvM_SetRamBlockStatus(
    NvM_BlockIdType BlockId,
    boolean BlockChanged
);

/**
 * @brief Sets the write protection for a NV block
 * @param BlockId Block identifier
 * @param ProtectionEnabled TRUE to enable protection
 * @return E_OK: Success, E_NOT_OK: Failure
 */
extern Std_ReturnType NvM_SetBlockProtection(
    NvM_BlockIdType BlockId,
    boolean ProtectionEnabled
);

/**
 * @brief Gets the error status of the last operation
 * @param BlockId Block identifier
 * @param RequestResultPtr Pointer to store result
 * @return E_OK: Success, E_NOT_OK: Failure
 */
extern Std_ReturnType NvM_GetErrorStatus(
    NvM_BlockIdType BlockId,
    NvM_RequestResultType* RequestResultPtr
);

/**
 * @brief Sets the data index for dataset blocks
 * @param BlockId Block identifier (must be dataset block)
 * @param DataIndex Data index to select
 * @return E_OK: Success, E_NOT_OK: Failure
 */
extern Std_ReturnType NvM_SetDataIndex(
    NvM_BlockIdType BlockId,
    uint8_t DataIndex
);

/**
 * @brief Gets the current data index for dataset blocks
 * @param BlockId Block identifier (must be dataset block)
 * @param DataIndexPtr Pointer to store current index
 * @return E_OK: Success, E_NOT_OK: Failure
 */
extern Std_ReturnType NvM_GetDataIndex(
    NvM_BlockIdType BlockId,
    uint8_t* DataIndexPtr
);

/**
 * @brief Main function - called cyclically by BSW Scheduler
 * @details Processes pending jobs and handles state machine
 * @note Call period configured in NvM_Cfg.h (NVM_MAIN_FUNCTION_PERIOD_MS)
 */
extern void NvM_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* NVM_H */

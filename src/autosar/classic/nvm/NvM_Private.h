/**
 * @file NvM_Private.h
 * @brief AUTOSAR NvM Module Private Header
 * @version 4.4.0
 * @date 2025
 * 
 * AUTOSAR Classic Platform - NvM Internal Definitions (Module ID: 0x0E)
 * 
 * This file contains internal types and definitions not exposed to the API.
 * 
 * Copyright (c) 2025
 */

#ifndef NVM_PRIVATE_H
#define NVM_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "NvM.h"
#include "MemIf.h"
#include <string.h>

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

/*============================================================================*
 * Magic Numbers and Identifiers
 *============================================================================*/
#define NVM_MAGIC_VALID                 0xA5B6C7D8u
#define NVM_MAGIC_INVALIDATED           0x00000000u
#define NVM_MAGIC_ERASED                0xFFFFFFFFu

#define NVM_STATE_UNINIT                0x00u
#define NVM_STATE_IDLE                  0x01u
#define NVM_STATE_BUSY                  0x02u
#define NVM_STATE_PENDING               0x03u

/*============================================================================*
 * Service IDs for Error Reporting
 *============================================================================*/
#define NVM_SID_INIT                    0x00u
#define NVM_SID_SET_DATA_INDEX          0x01u
#define NVM_SID_GET_DATA_INDEX          0x02u
#define NVM_SID_SET_BLOCK_PROTECTION    0x03u
#define NVM_SID_GET_ERROR_STATUS        0x04u
#define NVM_SID_SET_RAM_BLOCK_STATUS    0x05u
#define NVM_SID_READ_BLOCK              0x06u
#define NVM_SID_WRITE_BLOCK             0x07u
#define NVM_SID_RESTORE_BLOCK_DEFAULTS  0x08u
#define NVM_SID_ERASE_NV_BLOCK          0x09u
#define NVM_SID_CANCEL_WRITE_ALL        0x0Cu
#define NVM_SID_READ_ALL                0x0Du
#define NVM_SID_WRITE_ALL               0x0Eu
#define NVM_SID_GET_VERSION_INFO        0x0Fu
#define NVM_SID_INVALIDATE_NV_BLOCK     0x0Bu
#define NVM_SID_MAIN_FUNCTION           0x10u

/*============================================================================*
 * Internal Type Definitions
 *============================================================================*/

/**
 * @brief Internal Block State
 */
typedef enum {
    NVM_BLOCK_STATE_IDLE = 0,           /*!< Block idle */
    NVM_BLOCK_STATE_READ_PENDING,       /*!< Read operation pending */
    NVM_BLOCK_STATE_WRITE_PENDING,      /*!< Write operation pending */
    NVM_BLOCK_STATE_ERASE_PENDING,      /*!< Erase operation pending */
    NVM_BLOCK_STATE_VALIDATE_PENDING,   /*!< Validation pending */
    NVM_BLOCK_STATE_RESTORE_PENDING     /*!< Restore pending */
} NvM_InternalBlockStateType;

/**
 * @brief Job Queue Entry
 */
typedef struct NvM_JobQueueEntry_s {
    NvM_JobTypeType         JobType;        /*!< Type of job */
    NvM_BlockIdType         BlockId;        /*!< Target block ID */
    void*                   DataPtr;        /*!< Data pointer */
    uint8_t                 DataIndex;      /*!< Dataset index */
    NvM_PriorityType        Priority;       /*!< Job priority */
    struct NvM_JobQueueEntry_s* Next;       /*!< Next in queue */
    boolean                 InProgress;     /*!< Job processing flag */
} NvM_JobQueueEntryType;

/**
 * @brief RAM Block Status
 */
typedef struct {
    boolean DataChanged;                    /*!< Data has been modified */
    boolean WriteProtected;                 /*!< Block is write protected */
    uint8_t DataIndex;                      /*!< Current dataset index */
    NvM_RequestResultType LastResult;       /*!< Last operation result */
    NvM_InternalBlockStateType State;       /*!< Current block state */
    uint32_t LastWriteTime;                 /*!< Timestamp of last write */
    uint8_t WriteRetryCount;                /*!< Current retry count */
} NvM_RamBlockStatusType;

/**
 * @brief Internal Block Descriptor (runtime)
 */
typedef struct {
    const NvM_BlockDescriptorType* Config;  /*!< Pointer to configuration */
    NvM_RamBlockStatusType Status;          /*!< Runtime status */
    uint32_t CurrentCrc;                    /*!< Current CRC value */
    boolean Invalidated;                    /*!< Block invalidated flag */
} NvM_InternalBlockType;

/**
 * @brief Module Global State
 */
typedef struct {
    boolean Initialized;                    /*!< Module initialized flag */
    NvM_StateType State;                    /*!< Current state machine state */
    uint32_t CurrentJobId;                  /*!< Current job ID counter */
    NvM_JobQueueEntryType* QueueHead;       /*!< Job queue head */
    NvM_JobQueueEntryType* QueueTail;       /*!< Job queue tail */
    uint16_t QueueSize;                     /*!< Current queue size */
    boolean WriteAllActive;                 /*!< WriteAll in progress */
    boolean ReadAllActive;                  /*!< ReadAll in progress */
    boolean CancelWriteAll;                 /*!< Cancel WriteAll request */
    uint32_t CurrentTimeMs;                 /*!< Current time in ms */
    uint8_t WriteRetryCounter;              /*!< Current write retry count */
    NvM_InternalBlockType Blocks[NVM_MAX_NUMBER_OF_BLOCKS + 1]; /*!< Block table */
    NvM_JobQueueEntryType JobQueue[NVM_SIZE_OF_JOB_QUEUE];      /*!< Job pool */
} NvM_GlobalType;

/*============================================================================*
 * Block Header Structure (for storage)
 *============================================================================*/
typedef struct {
    uint32_t Magic;                 /*!< Magic number for validation */
    uint16_t BlockId;               /*!< Block identifier */
    uint16_t DataLength;            /*!< Data length */
    uint32_t SequenceNumber;        /*!< Write sequence number */
    uint32_t DataCrc;               /*!< CRC of data */
    uint32_t HeaderCrc;             /*!< CRC of header (excluding this field) */
} NvM_BlockHeaderType;

/**
 * @brief Maximum block length (configurable)
 */
#define NVM_MAX_BLOCK_LENGTH        2048u

/*============================================================================*
 * External Global Variables
 *============================================================================*/
extern NvM_GlobalType NvM_Global;

/*============================================================================*
 * Internal Function Prototypes
 *============================================================================*/

/* State Machine Functions */
void NvM_StateMachine_Process(void);
void NvM_StateMachine_EnterIdle(void);
void NvM_StateMachine_EnterBusy(void);
void NvM_StateMachine_EnterPending(void);

/* Job Queue Functions */
Std_ReturnType NvM_Queue_Init(void);
Std_ReturnType NvM_Queue_AddJob(
    NvM_JobTypeType JobType,
    NvM_BlockIdType BlockId,
    void* DataPtr,
    NvM_PriorityType Priority
);
Std_ReturnType NvM_Queue_GetNextJob(NvM_JobQueueEntryType** JobPtr);
void NvM_Queue_JobComplete(NvM_JobQueueEntryType* Job);
void NvM_Queue_Clear(void);
boolean NvM_Queue_IsEmpty(void);
boolean NvM_Queue_IsFull(void);

/* Block Management Functions */
Std_ReturnType NvM_Block_Init(NvM_BlockIdType BlockId);
Std_ReturnType NvM_Block_Read(NvM_BlockIdType BlockId, void* DataPtr);
Std_ReturnType NvM_Block_Write(NvM_BlockIdType BlockId, const void* DataPtr);
Std_ReturnType NvM_Block_Erase(NvM_BlockIdType BlockId);
Std_ReturnType NvM_Block_Restore(NvM_BlockIdType BlockId, void* DataPtr);
Std_ReturnType NvM_Block_Validate(NvM_BlockIdType BlockId);
void NvM_Block_SetResult(NvM_BlockIdType BlockId, NvM_RequestResultType Result);

/* CRC Functions */
uint8_t NvM_Crc8_Calculate(const uint8_t* DataPtr, uint32_t Length);
uint16_t NvM_Crc16_Calculate(const uint8_t* DataPtr, uint32_t Length);
uint32_t NvM_Crc32_Calculate(const uint8_t* DataPtr, uint32_t Length);
uint32_t NvM_Crc_Calculate(NvM_CrcType CrcType, const uint8_t* DataPtr, uint32_t Length);
boolean NvM_Crc_Verify(NvM_CrcType CrcType, const uint8_t* DataPtr, uint32_t Length, uint32_t ExpectedCrc);

/* Write Verification Functions */
Std_ReturnType NvM_Verify_Write(NvM_BlockIdType BlockId, const void* DataPtr, uint32_t Length);

/* Write Protection Functions */
boolean NvM_WriteProtection_IsActive(NvM_BlockIdType BlockId);
void NvM_WriteProtection_Enable(NvM_BlockIdType BlockId);
void NvM_WriteProtection_Disable(NvM_BlockIdType BlockId);

/* Utility Functions */
boolean NvM_IsBlockIdValid(NvM_BlockIdType BlockId);
boolean NvM_IsBlockConfigured(NvM_BlockIdType BlockId);
boolean NvM_IsQueueEntryAvailable(void);
NvM_InternalBlockType* NvM_GetInternalBlock(NvM_BlockIdType BlockId);
void NvM_ReportError(uint8_t ApiId, uint8_t ErrorId);

/* MemIf Interface Functions */
Std_ReturnType NvM_MemIf_Read(
    uint8_t DeviceId,
    uint16_t BlockNumber,
    uint16_t BlockOffset,
    uint8_t* DataPtr,
    uint16_t Length
);
Std_ReturnType NvM_MemIf_Write(
    uint8_t DeviceId,
    uint16_t BlockNumber,
    uint8_t* DataPtr,
    uint16_t Length
);
Std_ReturnType NvM_MemIf_Erase(
    uint8_t DeviceId,
    uint16_t BlockNumber
);
Std_ReturnType NvM_MemIf_Invalidate(
    uint8_t DeviceId,
    uint16_t BlockNumber
);
MemIf_StatusType NvM_MemIf_GetStatus(uint8_t DeviceId);

/*============================================================================*
 * Macros
 *============================================================================*/
#define NVM_CHECK_INITIALIZED() \
    do { \
        if (NvM_Global.Initialized == FALSE) { \
            NvM_ReportError(NVM_SID_MAIN_FUNCTION, NVM_E_NOT_INITIALIZED); \
            return; \
        } \
    } while(0)

#define NVM_CHECK_INITIALIZED_RET(EOK) \
    do { \
        if (NvM_Global.Initialized == FALSE) { \
            NvM_ReportError(NVM_SID_MAIN_FUNCTION, NVM_E_NOT_INITIALIZED); \
            return (EOK); \
        } \
    } while(0)

#define NVM_MIN(a, b) ((a) < (b) ? (a) : (b))
#define NVM_MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
}
#endif

#endif /* NVM_PRIVATE_H */

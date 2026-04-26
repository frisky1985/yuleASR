/**
 * @file NvM.c
 * @brief AUTOSAR NvM (Non-Volatile Memory) Module Implementation
 * @version 4.4.0
 * @date 2025
 *
 * AUTOSAR Classic Platform - NvM Implementation (Module ID: 0x0E)
 *
 * Key Features:
 * - Task queue management (single linked list)
 * - State machine: IDLE -> BUSY -> PENDING -> IDLE
 * - Write protection mechanism with configurable window
 * - Write retry mechanism (configurable 3 retries)
 * - Write verification (Read-After-Write)
 * - RAM Block and ROM Block management
 * - Incremental write protection (SetRamBlockStatus)
 * - Module identification (Block ID + Checksum)
 * - CRC data integrity (CRC-8/16/32)
 *
 * Copyright (c) 2025
 */

#include "NvM_Private.h"
#include "MemIf.h"
#include <string.h>

/*============================================================================*
 * Module Global Variables
 *============================================================================*/
NvM_GlobalType NvM_Global;

/* Development Error Reporting */
#if (NVM_DEV_ERROR_DETECT == STD_ON)
    /* Det.h not available in this environment - error reporting disabled */
    #define NVM_REPORT_ERROR(ApiId, ErrorId) /* Empty */
#else
    #define NVM_REPORT_ERROR(ApiId, ErrorId)
#endif

/*============================================================================*
 * Static Function Prototypes
 *============================================================================*/
static Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId, uint8_t ApiId);
static void NvM_ProcessJob(NvM_JobQueueEntryType* Job);
static void NvM_HandleWriteJob(NvM_JobQueueEntryType* Job);
static void NvM_HandleReadJob(NvM_JobQueueEntryType* Job);
static void NvM_HandleEraseJob(NvM_JobQueueEntryType* Job);
static void NvM_HandleRestoreJob(NvM_JobQueueEntryType* Job);
static Std_ReturnType NvM_PerformWrite(NvM_BlockIdType BlockId, const void* DataPtr);
static Std_ReturnType NvM_PerformRead(NvM_BlockIdType BlockId, void* DataPtr);
static Std_ReturnType NvM_PerformErase(NvM_BlockIdType BlockId);
static void NvM_CopyToInternalBuffer(NvM_BlockIdType BlockId, const void* DataPtr);

/*============================================================================*
 * Module Initialization
 *============================================================================*/

/**
 * @brief Initializes the NvM module
 */
void NvM_Init(const NvM_ConfigType* ConfigPtr)
{
    NvM_BlockIdType blockId;
    
    /* Check if already initialized */
    if (NvM_Global.Initialized == TRUE) {
        NVM_REPORT_ERROR(NVM_SID_INIT, NVM_E_ALREADY_INITIALIZED);
        return;
    }
    
    /* Initialize global state */
    NvM_Global.Initialized = FALSE;
    NvM_Global.State = NVM_STATE_UNINIT;
    NvM_Global.CurrentJobId = 0u;
    NvM_Global.QueueHead = NULL_PTR;
    NvM_Global.QueueTail = NULL_PTR;
    NvM_Global.QueueSize = 0u;
    NvM_Global.WriteAllActive = FALSE;
    NvM_Global.ReadAllActive = FALSE;
    NvM_Global.CancelWriteAll = FALSE;
    NvM_Global.CurrentTimeMs = 0u;
    NvM_Global.WriteRetryCounter = 0u;
    
    /* Clear job queue pool */
    for (blockId = 0u; blockId < NVM_SIZE_OF_JOB_QUEUE; blockId++) {
        NvM_Global.JobQueue[blockId].JobType = NVM_JOB_TYPE_NONE;
        NvM_Global.JobQueue[blockId].BlockId = 0u;
        NvM_Global.JobQueue[blockId].DataPtr = NULL_PTR;
        NvM_Global.JobQueue[blockId].Next = NULL_PTR;
        NvM_Global.JobQueue[blockId].InProgress = FALSE;
    }
    
    /* Initialize all configured blocks */
    for (blockId = 0u; blockId <= NVM_MAX_NUMBER_OF_BLOCKS; blockId++) {
        NvM_Block_Init(blockId);
    }
    
    /* Initialize MemIf module */
    MemIf_Init();
    
    /* Initialize job queue */
    (void)NvM_Queue_Init();
    
    /* Mark module as initialized */
    NvM_Global.Initialized = TRUE;
    NvM_Global.State = NVM_STATE_IDLE;
}

/**
 * @brief Gets the version information
 */
void NvM_GetVersionInfo(Std_VersionInfoType* Versioninfo)
{
    if (Versioninfo == NULL_PTR) {
        NVM_REPORT_ERROR(NVM_SID_GET_VERSION_INFO, NVM_E_PARAM_POINTER);
        return;
    }
    
    Versioninfo->vendorID = NVM_VENDOR_ID;
    Versioninfo->moduleID = NVM_MODULE_ID;
    Versioninfo->sw_major_version = NVM_SW_MAJOR_VERSION;
    Versioninfo->sw_minor_version = NVM_SW_MINOR_VERSION;
    Versioninfo->sw_patch_version = NVM_SW_PATCH_VERSION;
}

/*============================================================================*
 * Block Operations API
 *============================================================================*/

/**
 * @brief Reads a block from NVRAM
 */
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    Std_ReturnType result;
    
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_READ_BLOCK) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if block is pending */
    if (NvM_Global.Blocks[BlockId].Status.State != NVM_BLOCK_STATE_IDLE) {
        NVM_REPORT_ERROR(NVM_SID_READ_BLOCK, NVM_E_BLOCK_PENDING);
        return E_NOT_OK;
    }
    
    /* Add read job to queue */
    result = NvM_Queue_AddJob(NVM_JOB_TYPE_READ, BlockId, NvM_DstPtr, NVM_PRIORITY_NORMAL);
    
    if (result == E_OK) {
        NvM_Global.Blocks[BlockId].Status.State = NVM_BLOCK_STATE_READ_PENDING;
        NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_PENDING;
    } else {
        NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_NOT_OK;
    }
    
    return result;
}

/**
 * @brief Writes a block to NVRAM
 */
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    Std_ReturnType result;
    
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_WRITE_BLOCK) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if block is pending */
    if (NvM_Global.Blocks[BlockId].Status.State != NVM_BLOCK_STATE_IDLE) {
        NVM_REPORT_ERROR(NVM_SID_WRITE_BLOCK, NVM_E_BLOCK_PENDING);
        return E_NOT_OK;
    }
    
    /* Check write protection */
    if (NvM_WriteProtection_IsActive(BlockId) == TRUE) {
        NVM_REPORT_ERROR(NVM_SID_WRITE_BLOCK, NVM_E_WRITE_PROTECTED);
        return E_NOT_OK;
    }
    
    /* Add write job to queue */
    result = NvM_Queue_AddJob(NVM_JOB_TYPE_WRITE, BlockId, (void*)NvM_SrcPtr, NVM_PRIORITY_NORMAL);
    
    if (result == E_OK) {
        NvM_Global.Blocks[BlockId].Status.State = NVM_BLOCK_STATE_WRITE_PENDING;
        NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_PENDING;
    } else {
        NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_NOT_OK;
    }
    
    return result;
}

/**
 * @brief Restores block defaults from ROM
 */
Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    Std_ReturnType result;
    
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_RESTORE_BLOCK_DEFAULTS) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if block is pending */
    if (NvM_Global.Blocks[BlockId].Status.State != NVM_BLOCK_STATE_IDLE) {
        NVM_REPORT_ERROR(NVM_SID_RESTORE_BLOCK_DEFAULTS, NVM_E_BLOCK_PENDING);
        return E_NOT_OK;
    }
    
    /* Add restore job to queue */
    result = NvM_Queue_AddJob(NVM_JOB_TYPE_RESTORE, BlockId, NvM_DstPtr, NVM_PRIORITY_HIGH);
    
    if (result == E_OK) {
        NvM_Global.Blocks[BlockId].Status.State = NVM_BLOCK_STATE_RESTORE_PENDING;
        NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_PENDING;
    }
    
    return result;
}

/**
 * @brief Erases a NV block
 */
Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId)
{
    Std_ReturnType result;
    
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_ERASE_NV_BLOCK) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if block is pending */
    if (NvM_Global.Blocks[BlockId].Status.State != NVM_BLOCK_STATE_IDLE) {
        NVM_REPORT_ERROR(NVM_SID_ERASE_NV_BLOCK, NVM_E_BLOCK_PENDING);
        return E_NOT_OK;
    }
    
    /* Check write protection */
    if (NvM_WriteProtection_IsActive(BlockId) == TRUE) {
        NVM_REPORT_ERROR(NVM_SID_ERASE_NV_BLOCK, NVM_E_WRITE_PROTECTED);
        return E_NOT_OK;
    }
    
    /* Add erase job to queue */
    result = NvM_Queue_AddJob(NVM_JOB_TYPE_ERASE, BlockId, NULL_PTR, NVM_PRIORITY_HIGH);
    
    if (result == E_OK) {
        NvM_Global.Blocks[BlockId].Status.State = NVM_BLOCK_STATE_ERASE_PENDING;
        NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_PENDING;
    }
    
    return result;
}

/**
 * @brief Invalidates a NV block
 */
Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)
{
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_INVALIDATE_NV_BLOCK) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check write protection */
    if (NvM_WriteProtection_IsActive(BlockId) == TRUE) {
        NVM_REPORT_ERROR(NVM_SID_INVALIDATE_NV_BLOCK, NVM_E_WRITE_PROTECTED);
        return E_NOT_OK;
    }
    
    /* Mark block as invalidated */
    NvM_Global.Blocks[BlockId].Invalidated = TRUE;
    NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_NV_INVALIDATED;
    
    return E_OK;
}

/*============================================================================*
 * Multi-Block Operations API
 *============================================================================*/

/**
 * @brief Cancels an ongoing WriteAll operation
 */
void NvM_CancelWriteAll(void)
{
    NVM_CHECK_INITIALIZED();
    
    if (NvM_Global.WriteAllActive == TRUE) {
        NvM_Global.CancelWriteAll = TRUE;
    }
}

/**
 * @brief Initiates a multi-block read request
 */
void NvM_ReadAll(void)
{
    NvM_BlockIdType blockId;
    
    NVM_CHECK_INITIALIZED();
    
    if (NvM_Global.ReadAllActive == TRUE) {
        return;
    }
    
    NvM_Global.ReadAllActive = TRUE;
    
    /* Queue read jobs for all blocks with SelectBlockForReadall = TRUE */
    for (blockId = 1u; blockId <= NVM_MAX_NUMBER_OF_BLOCKS; blockId++) {
        if ((NvM_Global.Blocks[blockId].Config != NULL_PTR) &&
            (NvM_Global.Blocks[blockId].Config->SelectBlockForReadall == TRUE)) {
            (void)NvM_Queue_AddJob(NVM_JOB_TYPE_READ, blockId, NULL_PTR, NVM_PRIORITY_LOW);
        }
    }
}

/**
 * @brief Initiates a multi-block write request
 */
void NvM_WriteAll(void)
{
    NvM_BlockIdType blockId;
    
    NVM_CHECK_INITIALIZED();
    
    if (NvM_Global.WriteAllActive == TRUE) {
        return;
    }
    
    NvM_Global.WriteAllActive = TRUE;
    NvM_Global.CancelWriteAll = FALSE;
    
    /* Queue write jobs for all blocks with SelectBlockForWriteall = TRUE and data changed */
    for (blockId = 1u; blockId <= NVM_MAX_NUMBER_OF_BLOCKS; blockId++) {
        if ((NvM_Global.Blocks[blockId].Config != NULL_PTR) &&
            (NvM_Global.Blocks[blockId].Config->SelectBlockForWriteall == TRUE) &&
            (NvM_Global.Blocks[blockId].Status.DataChanged == TRUE)) {
            (void)NvM_Queue_AddJob(NVM_JOB_TYPE_WRITE, blockId, NULL_PTR, NVM_PRIORITY_LOW);
        }
    }
}

/*============================================================================*
 * Status and Control API
 *============================================================================*/

/**
 * @brief Sets the RAM block status for incremental write protection
 */
Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)
{
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
#if (NVM_SET_RAM_BLOCK_STATUS_API == STD_ON)
    if (NvM_ValidateBlockId(BlockId, NVM_SID_SET_RAM_BLOCK_STATUS) != E_OK) {
        return E_NOT_OK;
    }
    
    NvM_Global.Blocks[BlockId].Status.DataChanged = BlockChanged;
    
    return E_OK;
#else
    (void)BlockId;
    (void)BlockChanged;
    return E_NOT_OK;
#endif
}

/**
 * @brief Sets the write protection for a NV block
 */
Std_ReturnType NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled)
{
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
#if (NVM_ENABLE_WRITE_PROTECTION == STD_ON)
    if (NvM_ValidateBlockId(BlockId, NVM_SID_SET_BLOCK_PROTECTION) != E_OK) {
        return E_NOT_OK;
    }
    
    if (ProtectionEnabled == TRUE) {
        NvM_WriteProtection_Enable(BlockId);
    } else {
        NvM_WriteProtection_Disable(BlockId);
    }
    
    return E_OK;
#else
    (void)BlockId;
    (void)ProtectionEnabled;
    return E_NOT_OK;
#endif
}

/**
 * @brief Gets the error status of the last operation
 */
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr)
{
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (RequestResultPtr == NULL_PTR) {
        NVM_REPORT_ERROR(NVM_SID_GET_ERROR_STATUS, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_GET_ERROR_STATUS) != E_OK) {
        return E_NOT_OK;
    }
    
    *RequestResultPtr = NvM_Global.Blocks[BlockId].Status.LastResult;
    
    return E_OK;
}

/**
 * @brief Sets the data index for dataset blocks
 */
Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8_t DataIndex)
{
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_SET_DATA_INDEX) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if block is a dataset block */
    if (NvM_Global.Blocks[BlockId].Config->BlockManagementType != NVM_BLOCK_DATASET) {
        NVM_REPORT_ERROR(NVM_SID_SET_DATA_INDEX, NVM_E_PARAM_BLOCK_TYPE);
        return E_NOT_OK;
    }
    
    /* Validate data index range */
    if (DataIndex >= NvM_Global.Blocks[BlockId].Config->NvBlockNum) {
        NVM_REPORT_ERROR(NVM_SID_SET_DATA_INDEX, NVM_E_PARAM_DATA_INDEX);
        return E_NOT_OK;
    }
    
    NvM_Global.Blocks[BlockId].Status.DataIndex = DataIndex;
    
    return E_OK;
}

/**
 * @brief Gets the current data index for dataset blocks
 */
Std_ReturnType NvM_GetDataIndex(NvM_BlockIdType BlockId, uint8_t* DataIndexPtr)
{
    NVM_CHECK_INITIALIZED_RET(E_NOT_OK);
    
    if (DataIndexPtr == NULL_PTR) {
        NVM_REPORT_ERROR(NVM_SID_GET_DATA_INDEX, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (NvM_ValidateBlockId(BlockId, NVM_SID_GET_DATA_INDEX) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if block is a dataset block */
    if (NvM_Global.Blocks[BlockId].Config->BlockManagementType != NVM_BLOCK_DATASET) {
        NVM_REPORT_ERROR(NVM_SID_GET_DATA_INDEX, NVM_E_PARAM_BLOCK_TYPE);
        return E_NOT_OK;
    }
    
    *DataIndexPtr = NvM_Global.Blocks[BlockId].Status.DataIndex;
    
    return E_OK;
}

/*============================================================================*
 * Main Function - State Machine
 *============================================================================*/

/**
 * @brief Main function - called cyclically
 */
void NvM_MainFunction(void)
{
    NvM_JobQueueEntryType* currentJob;
    
    NVM_CHECK_INITIALIZED();
    
    /* Update current time */
    NvM_Global.CurrentTimeMs += NVM_MAIN_FUNCTION_PERIOD_MS;
    
    /* Call MemIf main function */
    MemIf_MainFunction();
    
    /* Process state machine */
    NvM_StateMachine_Process();
    
    /* Get next job from queue */
    if (NvM_Queue_GetNextJob(&currentJob) == E_OK) {
        NvM_ProcessJob(currentJob);
    }
    
    /* Check for WriteAll cancellation */
    if ((NvM_Global.CancelWriteAll == TRUE) && (NvM_Global.WriteAllActive == TRUE)) {
        /* Clear pending write jobs */
        NvM_Queue_Clear();
        NvM_Global.WriteAllActive = FALSE;
        NvM_Global.CancelWriteAll = FALSE;
    }
}

/*============================================================================*
 * Internal Functions
 *============================================================================*/

/**
 * @brief Validates block ID
 */
static Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId, uint8_t ApiId)
{
    if (BlockId == 0u) {
        /* Block 0 is reserved for multi-block requests */
        NVM_REPORT_ERROR(ApiId, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
    
    if (BlockId > NVM_MAX_NUMBER_OF_BLOCKS) {
        NVM_REPORT_ERROR(ApiId, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
    
    if (NvM_Global.Blocks[BlockId].Config == NULL_PTR) {
        NVM_REPORT_ERROR(ApiId, NVM_E_BLOCK_CONFIG);
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Processes a job from the queue
 */
static void NvM_ProcessJob(NvM_JobQueueEntryType* Job)
{
    if (Job == NULL_PTR) {
        return;
    }
    
    /* Mark job as in progress */
    Job->InProgress = TRUE;
    
    /* Process based on job type */
    switch (Job->JobType) {
        case NVM_JOB_TYPE_READ:
            NvM_HandleReadJob(Job);
            break;
            
        case NVM_JOB_TYPE_WRITE:
            NvM_HandleWriteJob(Job);
            break;
            
        case NVM_JOB_TYPE_ERASE:
            NvM_HandleEraseJob(Job);
            break;
            
        case NVM_JOB_TYPE_RESTORE:
            NvM_HandleRestoreJob(Job);
            break;
            
        default:
            /* Unknown job type */
            NvM_Block_SetResult(Job->BlockId, NVM_REQ_NOT_OK);
            NvM_Queue_JobComplete(Job);
            break;
    }
}

/**
 * @brief Handles a write job with retry mechanism
 */
static void NvM_HandleWriteJob(NvM_JobQueueEntryType* Job)
{
    Std_ReturnType result;
    NvM_BlockIdType blockId = Job->BlockId;
    const void* dataPtr = Job->DataPtr;
    uint8_t retryCount = 0;
    
    /* Get data pointer (use permanent RAM if NULL) */
    if (dataPtr == NULL_PTR) {
        dataPtr = NvM_Global.Blocks[blockId].Config->RamBlockDataAddr;
    }
    
    /* Attempt write with retry */
    do {
        result = NvM_PerformWrite(blockId, dataPtr);
        
        if (result == E_OK) {
            /* Verify write if enabled */
            #if (NVM_WRITE_VERIFICATION == STD_ON)
            if (NvM_Global.Blocks[blockId].Config->WriteVerification == TRUE) {
                result = NvM_Verify_Write(blockId, dataPtr,
                    NvM_Global.Blocks[blockId].Config->NvBlockLength);
            }
            #endif
            
            if (result == E_OK) {
                /* Success - clear changed flag */
                NvM_Global.Blocks[blockId].Status.DataChanged = FALSE;
                NvM_Global.Blocks[blockId].Status.WriteRetryCount = 0;
                NvM_Block_SetResult(blockId, NVM_REQ_OK);
                NvM_Queue_JobComplete(Job);
                return;
            }
        }
        
        retryCount++;
        NvM_Global.Blocks[blockId].Status.WriteRetryCount = retryCount;
        
    } while (retryCount < NvM_Global.Blocks[blockId].Config->MaxNumOfWriteRetries);
    
    /* All retries failed */
    NvM_Block_SetResult(blockId, NVM_REQ_NOT_OK);
    NvM_Queue_JobComplete(Job);
}

/**
 * @brief Handles a read job
 */
static void NvM_HandleReadJob(NvM_JobQueueEntryType* Job)
{
    Std_ReturnType result;
    NvM_BlockIdType blockId = Job->BlockId;
    void* dataPtr = Job->DataPtr;
    
    /* Get data pointer (use permanent RAM if NULL) */
    if (dataPtr == NULL_PTR) {
        dataPtr = NvM_Global.Blocks[blockId].Config->RamBlockDataAddr;
    }
    
    result = NvM_PerformRead(blockId, dataPtr);
    
    if (result == E_OK) {
        NvM_Block_SetResult(blockId, NVM_REQ_OK);
    } else {
        /* Try to restore from ROM */
        #if (NVM_RESTORE_ROM_ON_FAILURE == STD_ON)
        result = NvM_Block_Restore(blockId, dataPtr);
        if (result == E_OK) {
            NvM_Block_SetResult(blockId, NVM_REQ_RESTORED_FROM_ROM);
        } else
        #endif
        {
            NvM_Block_SetResult(blockId, NVM_REQ_NOT_OK);
        }
    }
    
    NvM_Queue_JobComplete(Job);
}

/**
 * @brief Handles an erase job
 */
static void NvM_HandleEraseJob(NvM_JobQueueEntryType* Job)
{
    Std_ReturnType result;
    
    result = NvM_PerformErase(Job->BlockId);
    
    if (result == E_OK) {
        NvM_Block_SetResult(Job->BlockId, NVM_REQ_OK);
    } else {
        NvM_Block_SetResult(Job->BlockId, NVM_REQ_NOT_OK);
    }
    
    NvM_Queue_JobComplete(Job);
}

/**
 * @brief Handles a restore job
 */
static void NvM_HandleRestoreJob(NvM_JobQueueEntryType* Job)
{
    Std_ReturnType result;
    void* dataPtr = Job->DataPtr;
    
    /* Get data pointer (use permanent RAM if NULL) */
    if (dataPtr == NULL_PTR) {
        dataPtr = NvM_Global.Blocks[Job->BlockId].Config->RamBlockDataAddr;
    }
    
    result = NvM_Block_Restore(Job->BlockId, dataPtr);
    
    if (result == E_OK) {
        NvM_Block_SetResult(Job->BlockId, NVM_REQ_RESTORED_FROM_ROM);
    } else {
        NvM_Block_SetResult(Job->BlockId, NVM_REQ_NOT_OK);
    }
    
    NvM_Queue_JobComplete(Job);
}

/**
 * @brief Performs the actual write operation via MemIf
 */
static Std_ReturnType NvM_PerformWrite(NvM_BlockIdType BlockId, const void* DataPtr)
{
    const NvM_BlockDescriptorType* config = NvM_Global.Blocks[BlockId].Config;
    uint16_t blockNumber;
    uint8_t* dataBuffer;
    
    /* Calculate block number with dataset offset */
    blockNumber = config->NvBlockBaseNumber + NvM_Global.Blocks[BlockId].Status.DataIndex;
    
    /* Allocate temporary buffer with header */
    dataBuffer = (uint8_t*)DataPtr; /* Simplified - should include header in real impl */
    
    /* Write via MemIf */
    return MemIf_Write(
        config->NvramDeviceId,
        blockNumber,
        dataBuffer
    );
}

/**
 * @brief Performs the actual read operation via MemIf
 */
static Std_ReturnType NvM_PerformRead(NvM_BlockIdType BlockId, void* DataPtr)
{
    const NvM_BlockDescriptorType* config = NvM_Global.Blocks[BlockId].Config;
    uint16_t blockNumber;
    
    /* Calculate block number with dataset offset */
    blockNumber = config->NvBlockBaseNumber + NvM_Global.Blocks[BlockId].Status.DataIndex;
    
    /* Read via MemIf */
    return MemIf_Read(
        config->NvramDeviceId,
        blockNumber,
        0u, /* Block offset */
        (uint8_t*)DataPtr,
        config->NvBlockLength
    );
}

/**
 * @brief Performs the actual erase operation via MemIf
 */
static Std_ReturnType NvM_PerformErase(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* config = NvM_Global.Blocks[BlockId].Config;
    uint16_t blockNumber;
    
    blockNumber = config->NvBlockBaseNumber + NvM_Global.Blocks[BlockId].Status.DataIndex;
    
    return MemIf_Erase(config->NvramDeviceId, blockNumber);
}

/*============================================================================*
 * External Configuration
 *============================================================================*/

/* Default configuration - should be replaced by generated configuration */
static const NvM_BlockDescriptorType NvM_BlockDescriptors[NVM_MAX_NUMBER_OF_BLOCKS + 1] = {
    /* Block 0 - Reserved for multi-block requests */
    {
        .NvBlockBaseNumber = 0u,
        .NvBlockLength = 0u,
        .NvBlockNum = 0u,
        .RomBlockNum = 0u,
        .BlockManagementType = NVM_BLOCK_NATIVE,
        .BlockWriteProt = FALSE,
        .WriteBlockOnce = FALSE,
        .SelectBlockForReadall = FALSE,
        .SelectBlockForWriteall = FALSE,
        .BswMBlockStatusInformation = FALSE,
        .CalcRamBlockCrc = FALSE,
        .WriteVerification = FALSE,
        .StaticBlockIDCheck = FALSE,
        .CrcType = NVM_CRC_NONE,
        .NvramDeviceId = 0u,
        .MaxNumOfWriteRetries = NVM_MAX_NUM_OF_WRITE_RETRIES,
        .RomBlockDataAddr = NULL_PTR,
        .RamBlockDataAddr = NULL_PTR,
        .NvMBlockCallback = NULL_PTR
    }
    /* Blocks 1-31 should be configured by user */
};

const NvM_ConfigType NvM_Config = {
    .BlockDescriptorTable = NvM_BlockDescriptors,
    .NumOfBlocks = NVM_MAX_NUMBER_OF_BLOCKS,
    .CommonCrcBlockBaseNumber = 0xFF00u,
    .MaxNumOfWriteRetries = NVM_MAX_NUM_OF_WRITE_RETRIES,
    .SizeOfJobQueue = NVM_SIZE_OF_JOB_QUEUE,
    .SetRamBlockStatusApi = (boolean)NVM_SET_RAM_BLOCK_STATUS_API,
    .EnableWriteProtection = (boolean)NVM_ENABLE_WRITE_PROTECTION,
    .WriteProtectionWindow = NVM_WRITE_PROTECTION_WINDOW_MS,
    .MainFunctionCycleTime = NVM_MAIN_FUNCTION_PERIOD_MS
};

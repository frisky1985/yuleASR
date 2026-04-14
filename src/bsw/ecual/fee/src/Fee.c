/**
 * @file Fee.c
 * @brief Flash EEPROM Emulation implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Fee.h"
#include "Fee_Cfg.h"
#include "MemIf.h"
#include "Det.h"

#define FEE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Fee_Initialized = FALSE;
static const Fee_ConfigType* Fee_ConfigPtr = NULL_PTR;
static Fee_StatusType Fee_Status = FEE_IDLE;
static Fee_JobResultType Fee_JobResult = FEE_JOB_OK;
static Fee_ModeType Fee_CurrentMode = FEE_MODE_FAST;

/* Block management */
typedef struct {
    uint32 BlockAddress;
    uint16 BlockSize;
    boolean IsValid;
    boolean IsInvalidated;
    uint32 WriteCycleCounter;
} Fee_BlockInfoType;

static Fee_BlockInfoType Fee_BlockInfo[FEE_NUM_BLOCKS];

/* Job management */
typedef enum {
    FEE_JOB_NONE = 0,
    FEE_JOB_READ,
    FEE_JOB_WRITE,
    FEE_JOB_INVALIDATE,
    FEE_JOB_ERASE_IMMEDIATE,
    FEE_JOB_GC
} Fee_JobType;

static Fee_JobType Fee_CurrentJob = FEE_JOB_NONE;
static Fee_BlockIdType Fee_CurrentBlock = 0U;
static uint16 Fee_CurrentOffset = 0U;
static uint8* Fee_DataBufferPtr = NULL_PTR;
static uint16 Fee_DataLength = 0U;

/* Garbage collection state */
typedef enum {
    FEE_GC_IDLE = 0,
    FEE_GC_COPY,
    FEE_GC_ERASE
} Fee_GcStateType;

static Fee_GcStateType Fee_GcState = FEE_GC_IDLE;
static uint32 Fee_EraseCycleCounter = 0U;
static uint32 Fee_WriteCycleCounter = 0U;

#define FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define FEE_START_SEC_CODE
#include "MemMap.h"

/* Forward declarations */
static void Fee_ProcessJob(void);
static void Fee_ProcessGc(void);
static Std_ReturnType Fee_FindBlockAddress(Fee_BlockIdType BlockNumber, uint32* BlockAddress);
static Std_ReturnType Fee_WriteBlockToFlash(Fee_BlockIdType BlockNumber, const uint8* DataBufferPtr);
static void Fee_TriggerGarbageCollection(void);

void Fee_Init(const Fee_ConfigType* ConfigPtr)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_INIT, FEE_E_INVALID_CFG);
        return;
    }
    #endif
    
    Fee_ConfigPtr = ConfigPtr;
    
    /* Initialize block info */
    for (uint16 i = 0U; i < FEE_NUM_BLOCKS; i++) {
        Fee_BlockInfo[i].BlockAddress = 0U;
        Fee_BlockInfo[i].BlockSize = 0U;
        Fee_BlockInfo[i].IsValid = FALSE;
        Fee_BlockInfo[i].IsInvalidated = FALSE;
        Fee_BlockInfo[i].WriteCycleCounter = 0U;
    }
    
    /* Scan flash to build block table */
    /* In a real implementation, this would scan the flash sectors
     * to find valid blocks and build the block management table */
    
    Fee_Status = FEE_IDLE;
    Fee_JobResult = FEE_JOB_OK;
    Fee_CurrentMode = FEE_MODE_FAST;
    Fee_CurrentJob = FEE_JOB_NONE;
    Fee_GcState = FEE_GC_IDLE;
    
    Fee_Initialized = TRUE;
}

void Fee_SetMode(Fee_ModeType Mode)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_SETMODE, FEE_E_UNINIT);
        return;
    }
    #if (FEE_SET_MODE_SUPPORTED == STD_ON)
    if ((Mode != FEE_MODE_SLOW) && (Mode != FEE_MODE_FAST)) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_SETMODE, FEE_E_INVALID_MODE);
        return;
    }
    #endif
    #endif
    
    #if (FEE_SET_MODE_SUPPORTED == STD_ON)
    Fee_CurrentMode = Mode;
    #else
    (void)Mode;
    #endif
}

Std_ReturnType Fee_Read(Fee_BlockIdType BlockNumber,
                         uint16 BlockOffset,
                         uint8* DataBufferPtr,
                         uint16 Length)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_READ, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= FEE_NUM_BLOCKS) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_READ, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    if (BlockOffset >= FEE_MAX_BLOCK_SIZE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_READ, FEE_E_INVALID_BLOCK_OFS);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_READ, FEE_E_INVALID_DATA_PTR);
        return E_NOT_OK;
    }
    if ((Length == 0U) || (Length > FEE_MAX_BLOCK_SIZE)) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_READ, FEE_E_INVALID_BLOCK_LEN);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if block is valid */
    if (Fee_BlockInfo[BlockNumber].IsValid == FALSE) {
        Fee_JobResult = FEE_BLOCK_INVALID;
        return E_NOT_OK;
    }
    
    if (Fee_BlockInfo[BlockNumber].IsInvalidated == TRUE) {
        Fee_JobResult = FEE_BLOCK_INVALID;
        return E_NOT_OK;
    }
    
    /* Check if another job is pending */
    if (Fee_Status == FEE_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup read job */
    Fee_CurrentJob = FEE_JOB_READ;
    Fee_CurrentBlock = BlockNumber;
    Fee_CurrentOffset = BlockOffset;
    Fee_DataBufferPtr = DataBufferPtr;
    Fee_DataLength = Length;
    Fee_Status = FEE_BUSY;
    Fee_JobResult = FEE_JOB_PENDING;
    
    /* In a real implementation, this would start the flash read operation */
    /* For now, simulate successful read */
    
    return E_OK;
}

Std_ReturnType Fee_Write(Fee_BlockIdType BlockNumber, const uint8* DataBufferPtr)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_WRITE, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= FEE_NUM_BLOCKS) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_WRITE, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_WRITE, FEE_E_INVALID_DATA_PTR);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if another job is pending */
    if (Fee_Status == FEE_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup write job */
    Fee_CurrentJob = FEE_JOB_WRITE;
    Fee_CurrentBlock = BlockNumber;
    Fee_CurrentOffset = 0U;
    Fee_DataBufferPtr = (uint8*)DataBufferPtr;
    Fee_DataLength = Fee_BlockInfo[BlockNumber].BlockSize;
    Fee_Status = FEE_BUSY;
    Fee_JobResult = FEE_JOB_PENDING;
    
    /* In a real implementation, this would:
     * 1. Find next free flash location
     * 2. Write block header + data
     * 3. Mark old block as obsolete
     * 4. Trigger garbage collection if needed
     */
    
    return E_OK;
}

void Fee_Cancel(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_CANCEL, FEE_E_UNINIT);
        return;
    }
    #endif
    
    if (Fee_Status == FEE_BUSY) {
        /* Cancel current job */
        Fee_Status = FEE_IDLE;
        Fee_JobResult = FEE_JOB_CANCELLED;
        Fee_CurrentJob = FEE_JOB_NONE;
        
        /* In a real implementation, this would cancel the underlying flash operation */
    }
}

Fee_StatusType Fee_GetStatus(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_GETSTATUS, FEE_E_UNINIT);
        return FEE_IDLE;
    }
    #endif
    
    return Fee_Status;
}

Fee_JobResultType Fee_GetJobResult(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_GETJOBRESULT, FEE_E_UNINIT);
        return FEE_JOB_FAILED;
    }
    #endif
    
    return Fee_JobResult;
}

Std_ReturnType Fee_InvalidateBlock(Fee_BlockIdType BlockNumber)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_INVALIDATEBLOCK, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= FEE_NUM_BLOCKS) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_INVALIDATEBLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if another job is pending */
    if (Fee_Status == FEE_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup invalidate job */
    Fee_CurrentJob = FEE_JOB_INVALIDATE;
    Fee_CurrentBlock = BlockNumber;
    Fee_Status = FEE_BUSY;
    Fee_JobResult = FEE_JOB_PENDING;
    
    return E_OK;
}

Std_ReturnType Fee_EraseImmediateBlock(Fee_BlockIdType BlockNumber)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_ERASEIMMEDIATEBLOCK, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= FEE_NUM_BLOCKS) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_ERASEIMMEDIATEBLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if another job is pending */
    if (Fee_Status == FEE_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup erase immediate job */
    Fee_CurrentJob = FEE_JOB_ERASE_IMMEDIATE;
    Fee_CurrentBlock = BlockNumber;
    Fee_Status = FEE_BUSY;
    Fee_JobResult = FEE_JOB_PENDING;
    
    return E_OK;
}

void Fee_JobEndNotification(void)
{
    #if (FEE_NVM_JOB_END_NOTIFICATION == STD_ON)
    /* Call upper layer notification */
    /* This would typically call NvM_JobEndNotification */
    #endif
}

void Fee_JobErrorNotification(void)
{
    #if (FEE_NVM_JOB_ERROR_NOTIFICATION == STD_ON)
    /* Call upper layer notification */
    /* This would typically call NvM_JobErrorNotification */
    #endif
}

void Fee_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_GETVERSIONINFO, FEE_E_INVALID_DATA_PTR);
        return;
    }
    #endif
    
    versioninfo->vendorID = FEE_VENDOR_ID;
    versioninfo->moduleID = FEE_MODULE_ID;
    versioninfo->sw_major_version = FEE_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FEE_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FEE_SW_PATCH_VERSION;
}

uint32 Fee_GetCycleCount(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_GETCYCLECOUNT, FEE_E_UNINIT);
        return 0U;
    }
    #endif
    
    return Fee_WriteCycleCounter;
}

uint32 Fee_GetEraseCycleCount(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_GETERASECYCLECOUNT, FEE_E_UNINIT);
        return 0U;
    }
    #endif
    
    return Fee_EraseCycleCounter;
}

uint32 Fee_GetWriteCycleCount(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE) {
        Det_ReportError(FEE_MODULE_ID, 0U, FEE_SID_GETWRITE_CYCLE_COUNT, FEE_E_UNINIT);
        return 0U;
    }
    #endif
    
    return Fee_WriteCycleCounter;
}

void Fee_MainFunction(void)
{
    if (Fee_Initialized == FALSE) {
        return;
    }
    
    /* Process current job */
    if (Fee_Status == FEE_BUSY) {
        Fee_ProcessJob();
    }
    
    /* Process garbage collection */
    if (Fee_GcState != FEE_GC_IDLE) {
        Fee_ProcessGc();
    }
    
    /* Check if garbage collection is needed */
    /* In a real implementation, this would check free space
     * and trigger GC when necessary */
}

static void Fee_ProcessJob(void)
{
    switch (Fee_CurrentJob) {
        case FEE_JOB_READ:
            /* Process read job */
            /* In a real implementation, this would:
             * 1. Check flash operation status
             * 2. Copy data to buffer
             * 3. Complete job
             */
            Fee_Status = FEE_IDLE;
            Fee_JobResult = FEE_JOB_OK;
            Fee_CurrentJob = FEE_JOB_NONE;
            Fee_JobEndNotification();
            break;
            
        case FEE_JOB_WRITE:
            /* Process write job */
            /* In a real implementation, this would:
             * 1. Program flash page by page
             * 2. Verify written data
             * 3. Update block table
             * 4. Complete job
             */
            Fee_WriteCycleCounter++;
            Fee_BlockInfo[Fee_CurrentBlock].WriteCycleCounter++;
            Fee_Status = FEE_IDLE;
            Fee_JobResult = FEE_JOB_OK;
            Fee_CurrentJob = FEE_JOB_NONE;
            Fee_JobEndNotification();
            break;
            
        case FEE_JOB_INVALIDATE:
            /* Process invalidate job */
            Fee_BlockInfo[Fee_CurrentBlock].IsInvalidated = TRUE;
            Fee_Status = FEE_IDLE;
            Fee_JobResult = FEE_JOB_OK;
            Fee_CurrentJob = FEE_JOB_NONE;
            Fee_JobEndNotification();
            break;
            
        case FEE_JOB_ERASE_IMMEDIATE:
            /* Process erase immediate job */
            Fee_BlockInfo[Fee_CurrentBlock].IsValid = FALSE;
            Fee_BlockInfo[Fee_CurrentBlock].IsInvalidated = FALSE;
            Fee_Status = FEE_IDLE;
            Fee_JobResult = FEE_JOB_OK;
            Fee_CurrentJob = FEE_JOB_NONE;
            Fee_JobEndNotification();
            break;
            
        default:
            Fee_Status = FEE_IDLE;
            Fee_CurrentJob = FEE_JOB_NONE;
            break;
    }
}

static void Fee_ProcessGc(void)
{
    switch (Fee_GcState) {
        case FEE_GC_COPY:
            /* Copy valid blocks to new sector */
            break;
            
        case FEE_GC_ERASE:
            /* Erase old sector */
            Fee_EraseCycleCounter++;
            Fee_GcState = FEE_GC_IDLE;
            break;
            
        default:
            Fee_GcState = FEE_GC_IDLE;
            break;
    }
}

static Std_ReturnType Fee_FindBlockAddress(Fee_BlockIdType BlockNumber, uint32* BlockAddress)
{
    if (BlockNumber >= FEE_NUM_BLOCKS) {
        return E_NOT_OK;
    }
    
    if (Fee_BlockInfo[BlockNumber].IsValid == FALSE) {
        return E_NOT_OK;
    }
    
    *BlockAddress = Fee_BlockInfo[BlockNumber].BlockAddress;
    return E_OK;
}

static Std_ReturnType Fee_WriteBlockToFlash(Fee_BlockIdType BlockNumber, const uint8* DataBufferPtr)
{
    (void)BlockNumber;
    (void)DataBufferPtr;
    
    /* In a real implementation, this would write the block to flash
     * with proper header information */
    
    return E_OK;
}

static void Fee_TriggerGarbageCollection(void)
{
    if (Fee_GcState == FEE_GC_IDLE) {
        Fee_GcState = FEE_GC_COPY;
    }
}

#define FEE_STOP_SEC_CODE
#include "MemMap.h"

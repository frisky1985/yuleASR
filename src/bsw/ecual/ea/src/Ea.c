/**
 * @file Ea.c
 * @brief EEPROM Abstraction implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Ea.h"
#include "Ea_Cfg.h"
#include "MemIf.h"
#include "Det.h"

/* Forward declaration for EEPROM driver access */
extern Std_ReturnType Eep_Read(uint32 EepromAddress, uint8* DataBufferPtr, uint16 Length);
extern Std_ReturnType Eep_Write(uint32 EepromAddress, const uint8* DataBufferPtr, uint16 Length);
extern Std_ReturnType Eep_Erase(uint32 EepromAddress, uint16 Length);
extern MemIf_StatusType Eep_GetStatus(void);
extern MemIf_JobResultType Eep_GetJobResult(void);
extern void Eep_Cancel(void);

#define EA_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Ea_Initialized = FALSE;
static const Ea_ConfigType* Ea_ConfigPtr = NULL_PTR;
static Ea_StatusType Ea_Status = EA_IDLE;
static Ea_JobResultType Ea_JobResult = EA_JOB_OK;
static Ea_ModeType Ea_CurrentMode = EA_MODE_FAST;

/* Block management */
typedef struct {
    uint32 EepromAddress;
    uint16 BlockSize;
    boolean IsValid;
    boolean IsInvalidated;
    uint32 WriteCycleCounter;
} Ea_BlockInfoType;

static Ea_BlockInfoType Ea_BlockInfo[EA_NUM_BLOCKS];

/* Job management */
typedef enum {
    EA_JOB_NONE = 0,
    EA_JOB_READ,
    EA_JOB_WRITE,
    EA_JOB_INVALIDATE,
    EA_JOB_ERASE_IMMEDIATE
} Ea_JobType;

static Ea_JobType Ea_CurrentJob = EA_JOB_NONE;
static Ea_BlockIdType Ea_CurrentBlock = 0U;
static uint16 Ea_CurrentOffset = 0U;
static uint8* Ea_DataBufferPtr = NULL_PTR;
static uint16 Ea_DataLength = 0U;

static uint32 Ea_EraseCycleCounter = 0U;

#define EA_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define EA_START_SEC_CODE
#include "MemMap.h"

/* Forward declarations */
static void Ea_ProcessJob(void);
static Std_ReturnType Ea_FindBlockAddress(Ea_BlockIdType BlockNumber, uint32* BlockAddress);
static uint32 Ea_CalculateBlockAddress(Ea_BlockIdType BlockNumber);

void Ea_Init(const Ea_ConfigType* ConfigPtr)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_INIT, EA_E_INVALID_CFG);
        return;
    }
    #endif
    
    Ea_ConfigPtr = ConfigPtr;
    
    /* Initialize block info */
    for (uint16 i = 0U; i < EA_NUM_BLOCKS; i++) {
        Ea_BlockInfo[i].EepromAddress = Ea_CalculateBlockAddress((Ea_BlockIdType)i);
        Ea_BlockInfo[i].BlockSize = 0U;
        Ea_BlockInfo[i].IsValid = FALSE;
        Ea_BlockInfo[i].IsInvalidated = FALSE;
        Ea_BlockInfo[i].WriteCycleCounter = 0U;
    }
    
    /* Load block configuration */
    if (ConfigPtr->BlockConfig != NULL_PTR) {
        for (uint16 i = 0U; i < ConfigPtr->NumBlocks; i++) {
            Ea_BlockIdType blockId = ConfigPtr->BlockConfig[i].BlockId;
            if (blockId < EA_NUM_BLOCKS) {
                Ea_BlockInfo[blockId].BlockSize = ConfigPtr->BlockConfig[i].BlockSize;
                Ea_BlockInfo[blockId].IsValid = TRUE;
            }
        }
    }
    
    Ea_Status = EA_IDLE;
    Ea_JobResult = EA_JOB_OK;
    Ea_CurrentMode = EA_MODE_FAST;
    Ea_CurrentJob = EA_JOB_NONE;
    
    Ea_Initialized = TRUE;
}

void Ea_SetMode(Ea_ModeType Mode)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_SETMODE, EA_E_UNINIT);
        return;
    }
    #if (EA_SET_MODE_SUPPORTED == STD_ON)
    if ((Mode != EA_MODE_SLOW) && (Mode != EA_MODE_FAST)) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_SETMODE, EA_E_INVALID_MODE);
        return;
    }
    #endif
    #endif
    
    #if (EA_SET_MODE_SUPPORTED == STD_ON)
    Ea_CurrentMode = Mode;
    /* Propagate mode to EEPROM driver */
    /* Eep_SetMode(Mode); */
    #else
    (void)Mode;
    #endif
}

Std_ReturnType Ea_Read(Ea_BlockIdType BlockNumber,
                        uint16 BlockOffset,
                        uint8* DataBufferPtr,
                        uint16 Length)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_READ, EA_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= EA_NUM_BLOCKS) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_READ, EA_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    if (BlockOffset >= EA_MAX_BLOCK_SIZE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_READ, EA_E_INVALID_BLOCK_OFS);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_READ, EA_E_INVALID_DATA_PTR);
        return E_NOT_OK;
    }
    if ((Length == 0U) || (Length > EA_MAX_BLOCK_SIZE)) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_READ, EA_E_INVALID_BLOCK_LEN);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if block is valid */
    if (Ea_BlockInfo[BlockNumber].IsValid == FALSE) {
        Ea_JobResult = EA_BLOCK_INVALID;
        return E_NOT_OK;
    }
    
    if (Ea_BlockInfo[BlockNumber].IsInvalidated == TRUE) {
        Ea_JobResult = EA_BLOCK_INVALID;
        return E_NOT_OK;
    }
    
    /* Check if another job is pending */
    if (Ea_Status == EA_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup read job */
    Ea_CurrentJob = EA_JOB_READ;
    Ea_CurrentBlock = BlockNumber;
    Ea_CurrentOffset = BlockOffset;
    Ea_DataBufferPtr = DataBufferPtr;
    Ea_DataLength = Length;
    Ea_Status = EA_BUSY;
    Ea_JobResult = EA_JOB_PENDING;
    
    /* Start EEPROM read operation */
    uint32 eepromAddress = Ea_BlockInfo[BlockNumber].EepromAddress + BlockOffset;
    Std_ReturnType result = Eep_Read(eepromAddress, DataBufferPtr, Length);
    
    if (result != E_OK) {
        Ea_Status = EA_IDLE;
        Ea_JobResult = EA_JOB_FAILED;
        Ea_CurrentJob = EA_JOB_NONE;
        return E_NOT_OK;
    }
    
    return E_OK;
}

Std_ReturnType Ea_Write(Ea_BlockIdType BlockNumber, const uint8* DataBufferPtr)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_WRITE, EA_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= EA_NUM_BLOCKS) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_WRITE, EA_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_WRITE, EA_E_INVALID_DATA_PTR);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if block is valid */
    if (Ea_BlockInfo[BlockNumber].IsValid == FALSE) {
        return E_NOT_OK;
    }
    
    /* Check if another job is pending */
    if (Ea_Status == EA_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup write job */
    Ea_CurrentJob = EA_JOB_WRITE;
    Ea_CurrentBlock = BlockNumber;
    Ea_CurrentOffset = 0U;
    Ea_DataBufferPtr = (uint8*)DataBufferPtr;
    Ea_DataLength = Ea_BlockInfo[BlockNumber].BlockSize;
    Ea_Status = EA_BUSY;
    Ea_JobResult = EA_JOB_PENDING;
    
    /* Start EEPROM write operation */
    uint32 eepromAddress = Ea_BlockInfo[BlockNumber].EepromAddress;
    Std_ReturnType result = Eep_Write(eepromAddress, DataBufferPtr, Ea_DataLength);
    
    if (result != E_OK) {
        Ea_Status = EA_IDLE;
        Ea_JobResult = EA_JOB_FAILED;
        Ea_CurrentJob = EA_JOB_NONE;
        return E_NOT_OK;
    }
    
    return E_OK;
}

void Ea_Cancel(void)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_CANCEL, EA_E_UNINIT);
        return;
    }
    #endif
    
    if (Ea_Status == EA_BUSY) {
        /* Cancel current job */
        Eep_Cancel();
        Ea_Status = EA_IDLE;
        Ea_JobResult = EA_JOB_CANCELLED;
        Ea_CurrentJob = EA_JOB_NONE;
    }
}

Ea_StatusType Ea_GetStatus(void)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_GETSTATUS, EA_E_UNINIT);
        return EA_IDLE;
    }
    #endif
    
    return Ea_Status;
}

Ea_JobResultType Ea_GetJobResult(void)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_GETJOBRESULT, EA_E_UNINIT);
        return EA_JOB_FAILED;
    }
    #endif
    
    return Ea_JobResult;
}

Std_ReturnType Ea_InvalidateBlock(Ea_BlockIdType BlockNumber)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_INVALIDATEBLOCK, EA_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= EA_NUM_BLOCKS) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_INVALIDATEBLOCK, EA_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if another job is pending */
    if (Ea_Status == EA_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup invalidate job */
    Ea_CurrentJob = EA_JOB_INVALIDATE;
    Ea_CurrentBlock = BlockNumber;
    Ea_Status = EA_BUSY;
    Ea_JobResult = EA_JOB_PENDING;
    
    /* Mark block as invalidated */
    Ea_BlockInfo[BlockNumber].IsInvalidated = TRUE;
    
    /* Complete immediately for now */
    Ea_Status = EA_IDLE;
    Ea_JobResult = EA_JOB_OK;
    Ea_CurrentJob = EA_JOB_NONE;
    
    #if (EA_NVM_JOB_END_NOTIFICATION == STD_ON)
    Ea_JobEndNotification();
    #endif
    
    return E_OK;
}

Std_ReturnType Ea_EraseImmediateBlock(Ea_BlockIdType BlockNumber)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_ERASEIMMEDIATEBLOCK, EA_E_UNINIT);
        return E_NOT_OK;
    }
    if (BlockNumber >= EA_NUM_BLOCKS) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_ERASEIMMEDIATEBLOCK, EA_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if another job is pending */
    if (Ea_Status == EA_BUSY) {
        return E_NOT_OK;
    }
    
    /* Setup erase immediate job */
    Ea_CurrentJob = EA_JOB_ERASE_IMMEDIATE;
    Ea_CurrentBlock = BlockNumber;
    Ea_Status = EA_BUSY;
    Ea_JobResult = EA_JOB_PENDING;
    
    /* Erase the block in EEPROM */
    uint32 eepromAddress = Ea_BlockInfo[BlockNumber].EepromAddress;
    uint16 blockSize = Ea_BlockInfo[BlockNumber].BlockSize;
    
    Std_ReturnType result = Eep_Erase(eepromAddress, blockSize);
    
    if (result != E_OK) {
        Ea_Status = EA_IDLE;
        Ea_JobResult = EA_JOB_FAILED;
        Ea_CurrentJob = EA_JOB_NONE;
        return E_NOT_OK;
    }
    
    Ea_EraseCycleCounter++;
    
    return E_OK;
}

void Ea_JobEndNotification(void)
{
    #if (EA_NVM_JOB_END_NOTIFICATION == STD_ON)
    /* Call upper layer notification */
    /* This would typically call NvM_JobEndNotification */
    #endif
}

void Ea_JobErrorNotification(void)
{
    #if (EA_NVM_JOB_ERROR_NOTIFICATION == STD_ON)
    /* Call upper layer notification */
    /* This would typically call NvM_JobErrorNotification */
    #endif
}

void Ea_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_GETVERSIONINFO, EA_E_INVALID_DATA_PTR);
        return;
    }
    #endif
    
    versioninfo->vendorID = EA_VENDOR_ID;
    versioninfo->moduleID = EA_MODULE_ID;
    versioninfo->sw_major_version = EA_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = EA_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = EA_SW_PATCH_VERSION;
}

uint32 Ea_GetEraseCycleCount(void)
{
    #if (EA_DEV_ERROR_DETECT == STD_ON)
    if (Ea_Initialized == FALSE) {
        Det_ReportError(EA_MODULE_ID, 0U, EA_SID_GETERASECYCLECOUNT, EA_E_UNINIT);
        return 0U;
    }
    #endif
    
    return Ea_EraseCycleCounter;
}

void Ea_MainFunction(void)
{
    if (Ea_Initialized == FALSE) {
        return;
    }
    
    /* Process current job */
    if (Ea_Status == EA_BUSY) {
        Ea_ProcessJob();
    }
}

static void Ea_ProcessJob(void)
{
    MemIf_JobResultType eepResult = Eep_GetJobResult();
    
    if (eepResult == MEMIF_JOB_PENDING) {
        /* Job still pending */
        return;
    }
    
    if (eepResult == MEMIF_JOB_OK) {
        /* Job completed successfully */
        switch (Ea_CurrentJob) {
            case EA_JOB_READ:
                Ea_JobResult = EA_JOB_OK;
                break;
                
            case EA_JOB_WRITE:
                Ea_BlockInfo[Ea_CurrentBlock].WriteCycleCounter++;
                Ea_JobResult = EA_JOB_OK;
                break;
                
            case EA_JOB_INVALIDATE:
                Ea_JobResult = EA_JOB_OK;
                break;
                
            case EA_JOB_ERASE_IMMEDIATE:
                Ea_JobResult = EA_JOB_OK;
                break;
                
            default:
                Ea_JobResult = EA_JOB_OK;
                break;
        }
        
        Ea_Status = EA_IDLE;
        Ea_CurrentJob = EA_JOB_NONE;
        
        #if (EA_NVM_JOB_END_NOTIFICATION == STD_ON)
        Ea_JobEndNotification();
        #endif
    } else {
        /* Job failed */
        Ea_JobResult = EA_JOB_FAILED;
        Ea_Status = EA_IDLE;
        Ea_CurrentJob = EA_JOB_NONE;
        
        #if (EA_NVM_JOB_ERROR_NOTIFICATION == STD_ON)
        Ea_JobErrorNotification();
        #endif
    }
}

static Std_ReturnType Ea_FindBlockAddress(Ea_BlockIdType BlockNumber, uint32* BlockAddress)
{
    if (BlockNumber >= EA_NUM_BLOCKS) {
        return E_NOT_OK;
    }
    
    if (Ea_BlockInfo[BlockNumber].IsValid == FALSE) {
        return E_NOT_OK;
    }
    
    *BlockAddress = Ea_BlockInfo[BlockNumber].EepromAddress;
    return E_OK;
}

static uint32 Ea_CalculateBlockAddress(Ea_BlockIdType BlockNumber)
{
    /* Calculate EEPROM address based on block number and sector size */
    /* Each block gets a fixed offset in the EEPROM address space */
    return (uint32)(BlockNumber * EA_MAX_BLOCK_SIZE);
}

#define EA_STOP_SEC_CODE
#include "MemMap.h"

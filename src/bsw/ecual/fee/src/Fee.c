/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*==================================================================================================
 *                                      FEE DRIVER
 *==================================================================================================
 * FILENAME: Fee.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashEEPROMEmulation.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Flash EEPROM Emulation module
 *              with Fls integration, block management, wear leveling,
 *              and garbage collection
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Fee.h"
#include "Fee_Cfg.h"
#include "Fls.h"
#include "Det.h"
#include <string.h>

/* Version checks */
#if defined(FEE_AR_RELEASE_MAJOR_VERSION) && (FEE_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Fee.c: Mismatch in AUTOSAR major version"
#endif

#if defined(FEE_SW_MAJOR_VERSION) && (FEE_SW_MAJOR_VERSION != 1u)
    #error "Fee.c: Mismatch in software major version"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
/* Block header magic numbers */
#define FEE_BLOCK_HEADER_MAGIC_VALID    (0x46454556u)   /* "FEEV" */
#define FEE_BLOCK_HEADER_MAGIC_INVALID  (0x46454549u)   /* "FEEI" */
#define FEE_BLOCK_HEADER_MAGIC_ERASED   (0xFFFFFFFFu)

/* Block states */
#define FEE_BLOCK_STATE_EMPTY           (0x00u)
#define FEE_BLOCK_STATE_VALID           (0x01u)
#define FEE_BLOCK_STATE_INVALIDATED     (0x02u)
#define FEE_BLOCK_STATE_OBSOLETE        (0x03u)

/* Job types */
#define FEE_JOB_NONE                    (0u)
#define FEE_JOB_READ                    (1u)
#define FEE_JOB_WRITE                   (2u)
#define FEE_JOB_INVALIDATE              (3u)
#define FEE_JOB_ERASE_IMMEDIATE         (4u)
#define FEE_JOB_GC                      (5u)
#define FEE_JOB_SECTOR_ERASE            (6u)

/* FLS job states */
#define FEE_FLS_JOB_IDLE                (0u)
#define FEE_FLS_JOB_PENDING             (1u)
#define FEE_FLS_JOB_COMPLETED           (2u)
#define FEE_FLS_JOB_FAILED              (3u)

/* GC states */
#define FEE_GC_STATE_IDLE               (0u)
#define FEE_GC_STATE_SCAN               (1u)
#define FEE_GC_STATE_COPY               (2u)
#define FEE_GC_STATE_ERASE              (3u)
#define FEE_GC_STATE_FINALIZE           (4u)

/* CRC polynomial */
#define FEE_CRC16_POLYNOMIAL            (0x1021u)
#define FEE_CRC16_INITIAL               (0xFFFFu)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#define FEE_ENTER_CRITICAL_SECTION()    /* OS integration point - disable interrupts */
#define FEE_EXIT_CRITICAL_SECTION()     /* OS integration point - enable interrupts */

#define FEE_ALIGN_TO_PAGE(size)         (((size) + FEE_VIRTUAL_PAGE_SIZE - 1u) & \
                                         ~(FEE_VIRTUAL_PAGE_SIZE - 1u))

#define FEE_MIN(a, b)                   ((a) < (b) ? (a) : (b))
#define FEE_MAX(a, b)                   ((a) > (b) ? (a) : (b))

/*==================================================================================================
 *                                    LOCAL TYPEDEFS
 *==================================================================================================*/
/* Block header structure - stored in flash */
typedef struct {
    uint32 MagicNumber;         /* Magic number for validation */
    uint16 BlockId;             /* Block ID */
    uint16 BlockSize;           /* Block data size */
    uint32 WriteCycleCount;     /* Write cycle counter */
    uint16 BlockCrc;            /* CRC of block data */
    uint16 HeaderCrc;           /* CRC of header (excluding HeaderCrc) */
    uint8 BlockState;           /* Block state */
    uint8 Reserved[3];          /* Padding to 8-byte boundary */
} Fee_BlockHeaderType;

/* Block runtime information */
typedef struct {
    Fee_AddressType BlockAddress;       /* Current flash address */
    uint16 BlockSize;                   /* Block size */
    uint8 BlockState;                   /* Current state */
    boolean IsValid;                    /* Block is valid */
    boolean IsInvalidated;              /* Block is invalidated */
    uint32 WriteCycleCounter;           /* Write cycle count for this block */
} Fee_BlockInfoType;

/* Sector runtime information */
typedef struct {
    Fee_AddressType SectorStartAddress; /* Start address */
    Fee_LengthType SectorSize;          /* Sector size */
    Fee_AddressType NextWriteAddress;   /* Next free address for writing */
    Fee_LengthType FreeSpace;           /* Available free space */
    uint32 EraseCycleCount;             /* Erase cycle count */
    uint8 SectorState;                  /* Sector state (active/full/erasing) */
    boolean IsActive;                   /* Sector is active for writing */
    boolean IsValid;                    /* Sector is valid */
} Fee_SectorInfoType;

/* Job control structure */
typedef struct {
    uint8 JobType;                      /* Current job type */
    Fee_BlockIdType BlockId;            /* Block ID for current job */
    uint16 BlockOffset;                 /* Offset within block */
    uint8* DataBufferPtr;               /* Data buffer pointer */
    uint16 DataLength;                  /* Data length */
    Fee_JobResultType JobResult;        /* Job result */
    uint8 JobState;                     /* Job state */
    uint8 RetryCount;                   /* Retry counter */
} Fee_JobControlType;

/* FLS job control */
typedef struct {
    uint8 JobState;                     /* FLS job state */
    Fls_AddressType Address;            /* Flash address */
    uint8* DataPtr;                     /* Data pointer */
    uint32 Length;                      /* Data length */
    uint8 JobType;                      /* Job type (read/write/erase) */
} Fee_FlsJobControlType;

/* GC control structure */
typedef struct {
    uint8 GcState;                      /* Current GC state */
    uint8 SourceSectorIndex;            /* Source sector for GC */
    uint8 TargetSectorIndex;            /* Target sector for GC */
    uint16 BlockIndex;                  /* Current block being processed */
    Fee_AddressType SourceAddress;      /* Current source address */
    Fee_AddressType TargetAddress;      /* Current target address */
    uint32 BlocksCopied;                /* Number of blocks copied */
    uint32 BlocksErased;                /* Number of blocks erased */
} Fee_GcControlType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define FEE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Module state */
static boolean Fee_Initialized = FALSE;
static const Fee_ConfigType* Fee_ConfigPtr = NULL_PTR;
static Fee_StatusType Fee_ModuleStatus = FEE_IDLE;
static Fee_ModeType Fee_CurrentMode = FEE_MODE_FAST;

/* Block management */
static Fee_BlockInfoType Fee_BlockInfo[FEE_NUM_BLOCKS];
static Fee_SectorInfoType Fee_SectorInfo[FEE_NUM_SECTORS];

/* Job control */
static Fee_JobControlType Fee_CurrentJob;
static Fee_FlsJobControlType Fee_FlsJob;
static Fee_GcControlType Fee_GcControl;

/* Global counters */
static uint32 Fee_EraseCycleCounter = 0u;
static uint32 Fee_WriteCycleCounter = 0u;
static uint32 Fee_GcCycleCounter = 0u;

/* Working buffer for flash operations */
static uint8 Fee_WorkBuffer[FEE_MAX_BLOCK_SIZE + sizeof(Fee_BlockHeaderType)];

#define FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
#define FEE_START_SEC_CODE
#include "MemMap.h"

/* Initialization functions */
static void Fee_ScanSectors(void);
static void Fee_BuildBlockTable(void);
static Std_ReturnType Fee_ValidateBlockHeader(Fee_AddressType Address, Fee_BlockHeaderType* HeaderPtr);

/* Block management functions */
static Fee_AddressType Fee_FindNextFreeAddress(uint8 SectorIndex);
static Std_ReturnType Fee_FindBlockAddress(Fee_BlockIdType BlockId, Fee_AddressType* AddressPtr);
static uint8 Fee_FindActiveSector(void);
static uint8 Fee_FindFreeSector(void);

/* CRC functions */
static uint16 Fee_CalculateCrc16(const uint8* DataPtr, uint32 Length);
static uint16 Fee_CalculateHeaderCrc(const Fee_BlockHeaderType* HeaderPtr);

/* Job processing functions */
static void Fee_ProcessJob(void);
static void Fee_ProcessReadJob(void);
static void Fee_ProcessWriteJob(void);
static void Fee_ProcessInvalidateJob(void);
static void Fee_ProcessEraseImmediateJob(void);
static void Fee_FinishJob(Fee_JobResultType Result);

/* FLS integration functions */
static Std_ReturnType Fee_StartFlsRead(Fls_AddressType Address, uint8* DataPtr, uint32 Length);
static Std_ReturnType Fee_StartFlsWrite(Fls_AddressType Address, const uint8* DataPtr, uint32 Length);
static Std_ReturnType Fee_StartFlsErase(Fls_AddressType Address, uint32 Length);
static void Fee_CheckFlsJobStatus(void);

/* GC functions */
static void Fee_ProcessGc(void);
static void Fee_StartGarbageCollection(void);
static void Fee_GcScanSector(void);
static void Fee_GcCopyBlocks(void);
static void Fee_GcEraseSector(void);
static void Fee_GcFinalize(void);
static boolean Fee_IsGcNeeded(void);

/* Wear leveling */
static uint8 Fee_SelectSectorForWrite(uint16 BlockSize);
static void Fee_UpdateWearLeveling(void);

/* Utility functions */
static Fee_LengthType Fee_GetBlockTotalSize(uint16 DataSize);
static void Fee_Memcpy(uint8* Dest, const uint8* Src, uint32 Length);
static boolean Fee_IsAddressInSector(Fee_AddressType Address, uint8 SectorIndex);

/*==================================================================================================
 *                                    API IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Flash EEPROM Emulation module
 * @param ConfigPtr Pointer to configuration structure
 * @req SWS_Fee_00153
 */
void Fee_Init(const Fee_ConfigType* ConfigPtr)
{
    uint8 i;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INIT, FEE_E_PARAM_CONFIG);
        return;
    }

    if (Fee_Initialized == TRUE)
    {
        /* Already initialized - report error but continue */
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INIT, FEE_E_INVALID_CFG);
        return;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    /* Store configuration */
    Fee_ConfigPtr = ConfigPtr;

    /* Initialize block info */
    for (i = 0u; i < FEE_NUM_BLOCKS; i++)
    {
        Fee_BlockInfo[i].BlockAddress = 0u;
        Fee_BlockInfo[i].BlockSize = 0u;
        Fee_BlockInfo[i].BlockState = FEE_BLOCK_STATE_EMPTY;
        Fee_BlockInfo[i].IsValid = FALSE;
        Fee_BlockInfo[i].IsInvalidated = FALSE;
        Fee_BlockInfo[i].WriteCycleCounter = 0u;
    }

    /* Initialize sector info */
    for (i = 0u; i < FEE_NUM_SECTORS; i++)
    {
        if (i < ConfigPtr->NumSectors)
        {
            Fee_SectorInfo[i].SectorStartAddress = ConfigPtr->SectorConfig[i].SectorStartAddress;
            Fee_SectorInfo[i].SectorSize = ConfigPtr->SectorConfig[i].SectorSize;
            Fee_SectorInfo[i].NextWriteAddress = ConfigPtr->SectorConfig[i].SectorStartAddress;
            Fee_SectorInfo[i].FreeSpace = ConfigPtr->SectorConfig[i].SectorSize;
            Fee_SectorInfo[i].EraseCycleCount = ConfigPtr->SectorConfig[i].SectorEraseCycleCount;
            Fee_SectorInfo[i].SectorState = FEE_BLOCK_STATE_EMPTY;
            Fee_SectorInfo[i].IsActive = FALSE;
            Fee_SectorInfo[i].IsValid = ConfigPtr->SectorConfig[i].SectorIsValid;
        }
    }

    /* Initialize job control */
    Fee_CurrentJob.JobType = FEE_JOB_NONE;
    Fee_CurrentJob.BlockId = 0u;
    Fee_CurrentJob.BlockOffset = 0u;
    Fee_CurrentJob.DataBufferPtr = NULL_PTR;
    Fee_CurrentJob.DataLength = 0u;
    Fee_CurrentJob.JobResult = FEE_JOB_OK;
    Fee_CurrentJob.JobState = FEE_FLS_JOB_IDLE;
    Fee_CurrentJob.RetryCount = 0u;

    /* Initialize FLS job control */
    Fee_FlsJob.JobState = FEE_FLS_JOB_IDLE;
    Fee_FlsJob.Address = 0u;
    Fee_FlsJob.DataPtr = NULL_PTR;
    Fee_FlsJob.Length = 0u;
    Fee_FlsJob.JobType = FLS_JOB_NONE;

    /* Initialize GC control */
    Fee_GcControl.GcState = FEE_GC_STATE_IDLE;
    Fee_GcControl.SourceSectorIndex = 0u;
    Fee_GcControl.TargetSectorIndex = 0u;
    Fee_GcControl.BlockIndex = 0u;
    Fee_GcControl.SourceAddress = 0u;
    Fee_GcControl.TargetAddress = 0u;
    Fee_GcControl.BlocksCopied = 0u;
    Fee_GcControl.BlocksErased = 0u;

    /* Scan flash to build block table */
    Fee_ScanSectors();
    Fee_BuildBlockTable();

    /* Set initial state */
    Fee_ModuleStatus = FEE_IDLE;
    Fee_CurrentMode = FEE_MODE_FAST;
    Fee_Initialized = TRUE;

    FEE_EXIT_CRITICAL_SECTION();
}

/**
 * @brief De-initializes the Flash EEPROM Emulation module
 */
void Fee_DeInit(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INIT, FEE_E_UNINIT);
        return;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    /* Cancel any ongoing job */
    if (Fee_ModuleStatus == FEE_BUSY)
    {
        Fee_Cancel();
    }

    Fee_Initialized = FALSE;
    Fee_ConfigPtr = NULL_PTR;
    Fee_ModuleStatus = FEE_IDLE;

    FEE_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (SLOW/FAST)
 * @req SWS_Fee_00155
 */
void Fee_SetMode(Fee_ModeType Mode)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SETMODE, FEE_E_UNINIT);
        return;
    }

#if (FEE_SET_MODE_SUPPORTED == STD_ON)
    if ((Mode != FEE_MODE_SLOW) && (Mode != FEE_MODE_FAST))
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SETMODE, FEE_E_INVALID_MODE);
        return;
    }
#endif
#endif

#if (FEE_SET_MODE_SUPPORTED == STD_ON)
    FEE_ENTER_CRITICAL_SECTION();
    Fee_CurrentMode = Mode;

    /* Propagate to Fls */
    if (Mode == FEE_MODE_FAST)
    {
        Fls_SetMode(MEMIF_MODE_FAST);
    }
    else
    {
        Fls_SetMode(MEMIF_MODE_SLOW);
    }

    FEE_EXIT_CRITICAL_SECTION();
#else
    (void)Mode;
#endif
}

/**
 * @brief Reads data from a block
 * @param BlockNumber Block number
 * @param BlockOffset Block offset
 * @param DataBufferPtr Data buffer pointer
 * @param Length Data length
 * @return Result of operation
 * @req SWS_Fee_00156
 */
Std_ReturnType Fee_Read(Fee_BlockIdType BlockNumber,
                         uint16 BlockOffset,
                         uint8* DataBufferPtr,
                         uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    if (BlockNumber >= FEE_NUM_BLOCKS)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }

    if (BlockOffset >= FEE_MAX_BLOCK_SIZE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_BLOCK_OFS);
        return E_NOT_OK;
    }

    if (DataBufferPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_DATA_PTR);
        return E_NOT_OK;
    }

    if ((Length == 0u) || (Length > FEE_MAX_BLOCK_SIZE))
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_BLOCK_LEN);
        return E_NOT_OK;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    /* Check if block is valid */
    if (Fee_BlockInfo[BlockNumber].IsValid == FALSE)
    {
        Fee_CurrentJob.JobResult = FEE_BLOCK_INVALID;
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    if (Fee_BlockInfo[BlockNumber].IsInvalidated == TRUE)
    {
        Fee_CurrentJob.JobResult = FEE_BLOCK_INVALID;
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    /* Check if another job is pending */
    if (Fee_ModuleStatus == FEE_BUSY)
    {
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    /* Setup read job */
    Fee_CurrentJob.JobType = FEE_JOB_READ;
    Fee_CurrentJob.BlockId = BlockNumber;
    Fee_CurrentJob.BlockOffset = BlockOffset;
    Fee_CurrentJob.DataBufferPtr = DataBufferPtr;
    Fee_CurrentJob.DataLength = Length;
    Fee_CurrentJob.JobResult = FEE_JOB_PENDING;
    Fee_CurrentJob.JobState = FEE_FLS_JOB_PENDING;
    Fee_CurrentJob.RetryCount = 0u;

    Fee_ModuleStatus = FEE_BUSY;
    result = E_OK;

    FEE_EXIT_CRITICAL_SECTION();

    return result;
}

/**
 * @brief Writes data to a block
 * @param BlockNumber Block number
 * @param DataBufferPtr Data buffer pointer
 * @return Result of operation
 * @req SWS_Fee_00157
 */
Std_ReturnType Fee_Write(Fee_BlockIdType BlockNumber, const uint8* DataBufferPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 blockSize;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    if (BlockNumber >= FEE_NUM_BLOCKS)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }

    if (DataBufferPtr == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_INVALID_DATA_PTR);
        return E_NOT_OK;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    /* Check if another job is pending */
    if (Fee_ModuleStatus == FEE_BUSY)
    {
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    /* Get block size from config */
    if (Fee_ConfigPtr != NULL_PTR)
    {
        blockSize = Fee_ConfigPtr->BlockConfig[BlockNumber].BlockSize;
    }
    else
    {
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    /* Check if GC is needed before write */
    if (Fee_IsGcNeeded())
    {
        /* Trigger GC - write will be processed after GC completes */
        Fee_StartGarbageCollection();
        /* Still accept the write job, it will be processed after GC */
    }

    /* Setup write job */
    Fee_CurrentJob.JobType = FEE_JOB_WRITE;
    Fee_CurrentJob.BlockId = BlockNumber;
    Fee_CurrentJob.BlockOffset = 0u;
    Fee_CurrentJob.DataBufferPtr = (uint8*)DataBufferPtr;
    Fee_CurrentJob.DataLength = blockSize;
    Fee_CurrentJob.JobResult = FEE_JOB_PENDING;
    Fee_CurrentJob.JobState = FEE_FLS_JOB_PENDING;
    Fee_CurrentJob.RetryCount = 0u;

    Fee_ModuleStatus = FEE_BUSY;
    result = E_OK;

    FEE_EXIT_CRITICAL_SECTION();

    return result;
}

/**
 * @brief Cancels ongoing operation
 * @req SWS_Fee_00158
 */
void Fee_Cancel(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_CANCEL, FEE_E_UNINIT);
        return;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    if (Fee_ModuleStatus == FEE_BUSY)
    {
        /* Cancel FLS job if active */
        if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
        {
            Fls_Cancel();
        }

        /* Cancel current job */
        Fee_ModuleStatus = FEE_IDLE;
        Fee_CurrentJob.JobResult = FEE_JOB_CANCELLED;
        Fee_CurrentJob.JobType = FEE_JOB_NONE;
        Fee_CurrentJob.JobState = FEE_FLS_JOB_IDLE;
    }

    FEE_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Gets module status
 * @return Module status
 * @req SWS_Fee_00159
 */
Fee_StatusType Fee_GetStatus(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETSTATUS, FEE_E_UNINIT);
        return FEE_IDLE;
    }
#endif

    return Fee_ModuleStatus;
}

/**
 * @brief Gets job result
 * @return Job result
 * @req SWS_Fee_00160
 */
Fee_JobResultType Fee_GetJobResult(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETJOBRESULT, FEE_E_UNINIT);
        return FEE_JOB_FAILED;
    }
#endif

    return Fee_CurrentJob.JobResult;
}

/**
 * @brief Invalidates a block
 * @param BlockNumber Block number
 * @return Result of operation
 * @req SWS_Fee_00161
 */
Std_ReturnType Fee_InvalidateBlock(Fee_BlockIdType BlockNumber)
{
    Std_ReturnType result = E_NOT_OK;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INVALIDATEBLOCK, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    if (BlockNumber >= FEE_NUM_BLOCKS)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INVALIDATEBLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    /* Check if another job is pending */
    if (Fee_ModuleStatus == FEE_BUSY)
    {
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    /* Setup invalidate job */
    Fee_CurrentJob.JobType = FEE_JOB_INVALIDATE;
    Fee_CurrentJob.BlockId = BlockNumber;
    Fee_CurrentJob.JobResult = FEE_JOB_PENDING;
    Fee_CurrentJob.JobState = FEE_FLS_JOB_PENDING;
    Fee_CurrentJob.RetryCount = 0u;

    Fee_ModuleStatus = FEE_BUSY;
    result = E_OK;

    FEE_EXIT_CRITICAL_SECTION();

    return result;
}

/**
 * @brief Erases immediate block
 * @param BlockNumber Block number
 * @return Result of operation
 * @req SWS_Fee_00162
 */
Std_ReturnType Fee_EraseImmediateBlock(Fee_BlockIdType BlockNumber)
{
    Std_ReturnType result = E_NOT_OK;

#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASEIMMEDIATEBLOCK, FEE_E_UNINIT);
        return E_NOT_OK;
    }

    if (BlockNumber >= FEE_NUM_BLOCKS)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASEIMMEDIATEBLOCK, FEE_E_INVALID_BLOCK_NO);
        return E_NOT_OK;
    }
#endif

    FEE_ENTER_CRITICAL_SECTION();

    /* Check if another job is pending */
    if (Fee_ModuleStatus == FEE_BUSY)
    {
        FEE_EXIT_CRITICAL_SECTION();
        return E_NOT_OK;
    }

    /* Setup erase immediate job */
    Fee_CurrentJob.JobType = FEE_JOB_ERASE_IMMEDIATE;
    Fee_CurrentJob.BlockId = BlockNumber;
    Fee_CurrentJob.JobResult = FEE_JOB_PENDING;
    Fee_CurrentJob.JobState = FEE_FLS_JOB_PENDING;
    Fee_CurrentJob.RetryCount = 0u;

    Fee_ModuleStatus = FEE_BUSY;
    result = E_OK;

    FEE_EXIT_CRITICAL_SECTION();

    return result;
}

/**
 * @brief Job end notification callback
 * @req SWS_Fee_00163
 */
void Fee_JobEndNotification(void)
{
#if (FEE_NVM_JOB_END_NOTIFICATION == STD_ON)
    /* Call upper layer notification (NvM) */
    extern void NvM_JobEndNotification(void);
    NvM_JobEndNotification();
#endif
}

/**
 * @brief Job error notification callback
 * @req SWS_Fee_00164
 */
void Fee_JobErrorNotification(void)
{
#if (FEE_NVM_JOB_ERROR_NOTIFICATION == STD_ON)
    /* Call upper layer notification (NvM) */
    extern void NvM_JobErrorNotification(void);
    NvM_JobErrorNotification();
#endif
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @req SWS_Fee_00165
 */
#if (FEE_VERSION_INFO_API == STD_ON)
void Fee_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETVERSIONINFO, FEE_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = FEE_VENDOR_ID;
    versioninfo->moduleID = FEE_MODULE_ID;
    versioninfo->sw_major_version = FEE_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FEE_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FEE_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Gets cycle count
 * @return Cycle count
 */
uint32 Fee_GetCycleCount(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETCYCLECOUNT, FEE_E_UNINIT);
        return 0u;
    }
#endif

    return Fee_WriteCycleCounter;
}

/**
 * @brief Gets erase cycle count
 * @return Erase cycle count
 */
uint32 Fee_GetEraseCycleCount(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETERASECYCLECOUNT, FEE_E_UNINIT);
        return 0u;
    }
#endif

    return Fee_EraseCycleCounter;
}

/**
 * @brief Gets write cycle count
 * @return Write cycle count
 */
uint32 Fee_GetWriteCycleCount(void)
{
#if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (Fee_Initialized == FALSE)
    {
        (void)Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETWRITECYCLECOUNT, FEE_E_UNINIT);
        return 0u;
    }
#endif

    return Fee_WriteCycleCounter;
}

/**
 * @brief Main function for periodic processing
 * @req SWS_Fee_00169
 */
void Fee_MainFunction(void)
{
    if (Fee_Initialized == FALSE)
    {
        return;
    }

    /* Check FLS job status */
    Fee_CheckFlsJobStatus();

    /* Process GC if active */
    if (Fee_GcControl.GcState != FEE_GC_STATE_IDLE)
    {
        Fee_ProcessGc();
        return;  /* GC has priority over normal jobs */
    }

    /* Process current job */
    if (Fee_ModuleStatus == FEE_BUSY)
    {
        Fee_ProcessJob();
    }

    /* Check if garbage collection is needed */
    if ((Fee_ModuleStatus != FEE_BUSY) && (Fee_IsGcNeeded()))
    {
        Fee_StartGarbageCollection();
    }
}

/**
 * @brief Internal function to process Fee jobs via Fls
 * @note This function is called by Fee_MainFunction to execute flash operations
 */
void Fee_ProcessFlsJob(void)
{
    /* This function is used internally by Fee_MainFunction */
    /* The actual FLS operations are started by the specific job processing functions */
}

/**
 * @brief Callback function for Fls job end notification
 * @note This function is called by Fls when a job completes successfully
 */
void Fee_FlsJobEndNotification(void)
{
    FEE_ENTER_CRITICAL_SECTION();

    if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
    {
        Fee_FlsJob.JobState = FEE_FLS_JOB_COMPLETED;
    }

    FEE_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Callback function for Fls job error notification
 * @note This function is called by Fls when a job fails
 */
void Fee_FlsJobErrorNotification(void)
{
    FEE_ENTER_CRITICAL_SECTION();

    if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
    {
        Fee_FlsJob.JobState = FEE_FLS_JOB_FAILED;
    }

    FEE_EXIT_CRITICAL_SECTION();
}

/*==================================================================================================
 *                                    LOCAL FUNCTION IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Scans flash sectors to find valid blocks
 */
static void Fee_ScanSectors(void)
{
    uint8 sectorIndex;
    Fee_AddressType address;
    Fee_BlockHeaderType header;

    for (sectorIndex = 0u; sectorIndex < FEE_NUM_SECTORS; sectorIndex++)
    {
        if (Fee_SectorInfo[sectorIndex].IsValid == FALSE)
        {
            continue;
        }

        address = Fee_SectorInfo[sectorIndex].SectorStartAddress;

        /* Scan sector for block headers */
        while (address < (Fee_SectorInfo[sectorIndex].SectorStartAddress + Fee_SectorInfo[sectorIndex].SectorSize))
        {
            if (Fee_ValidateBlockHeader(address, &header) == E_OK)
            {
                /* Valid block header found */
                if (header.BlockId < FEE_NUM_BLOCKS)
                {
                    /* Update block info if this is the latest copy */
                    if ((Fee_BlockInfo[header.BlockId].IsValid == FALSE) ||
                        (header.WriteCycleCount > Fee_BlockInfo[header.BlockId].WriteCycleCounter))
                    {
                        Fee_BlockInfo[header.BlockId].BlockAddress = address;
                        Fee_BlockInfo[header.BlockId].BlockSize = header.BlockSize;
                        Fee_BlockInfo[header.BlockId].BlockState = header.BlockState;
                        Fee_BlockInfo[header.BlockId].IsValid = TRUE;
                        Fee_BlockInfo[header.BlockId].IsInvalidated = (header.BlockState == FEE_BLOCK_STATE_INVALIDATED);
                        Fee_BlockInfo[header.BlockId].WriteCycleCounter = header.WriteCycleCount;
                    }
                }

                /* Move to next block */
                address += Fee_GetBlockTotalSize(header.BlockSize);
            }
            else
            {
                /* Empty or corrupted area, assume rest of sector is empty */
                Fee_SectorInfo[sectorIndex].NextWriteAddress = address;
                Fee_SectorInfo[sectorIndex].FreeSpace = Fee_SectorInfo[sectorIndex].SectorSize -
                                                        (address - Fee_SectorInfo[sectorIndex].SectorStartAddress);
                break;
            }
        }

        /* If we reached the end of sector */
        if (address >= (Fee_SectorInfo[sectorIndex].SectorStartAddress + Fee_SectorInfo[sectorIndex].SectorSize))
        {
            Fee_SectorInfo[sectorIndex].NextWriteAddress = address;
            Fee_SectorInfo[sectorIndex].FreeSpace = 0u;
            Fee_SectorInfo[sectorIndex].SectorState = FEE_BLOCK_STATE_OBSOLETE;
        }
    }
}

/**
 * @brief Builds the block table after scanning
 */
static void Fee_BuildBlockTable(void)
{
    uint8 i;
    uint8 activeSectorFound = FALSE;

    /* Find an active sector for writing */
    for (i = 0u; i < FEE_NUM_SECTORS; i++)
    {
        if ((Fee_SectorInfo[i].IsValid == TRUE) &&
            (Fee_SectorInfo[i].FreeSpace > 0u) &&
            (activeSectorFound == FALSE))
        {
            Fee_SectorInfo[i].IsActive = TRUE;
            Fee_SectorInfo[i].SectorState = FEE_BLOCK_STATE_VALID;
            activeSectorFound = TRUE;
        }
    }

    /* If no active sector found, mark first valid sector as active */
    if (activeSectorFound == FALSE)
    {
        for (i = 0u; i < FEE_NUM_SECTORS; i++)
        {
            if (Fee_SectorInfo[i].IsValid == TRUE)
            {
                Fee_SectorInfo[i].IsActive = TRUE;
                Fee_SectorInfo[i].NextWriteAddress = Fee_SectorInfo[i].SectorStartAddress;
                Fee_SectorInfo[i].FreeSpace = Fee_SectorInfo[i].SectorSize;
                Fee_SectorInfo[i].SectorState = FEE_BLOCK_STATE_EMPTY;
                break;
            }
        }
    }
}

/**
 * @brief Validates a block header at the given address
 */
static Std_ReturnType Fee_ValidateBlockHeader(Fee_AddressType Address, Fee_BlockHeaderType* HeaderPtr)
{
    /* In a real implementation, this would read from flash via Fls */
    /* For now, return not OK to indicate empty flash */
    (void)Address;
    (void)HeaderPtr;
    return E_NOT_OK;
}

/**
 * @brief Calculates CRC16 for data
 */
static uint16 Fee_CalculateCrc16(const uint8* DataPtr, uint32 Length)
{
    uint16 crc = FEE_CRC16_INITIAL;
    uint32 i;
    uint8 bit;

    for (i = 0u; i < Length; i++)
    {
        crc ^= ((uint16)DataPtr[i] << 8u);
        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (crc << 1u) ^ FEE_CRC16_POLYNOMIAL;
            }
            else
            {
                crc = crc << 1u;
            }
        }
    }

    return crc;
}

/**
 * @brief Calculates header CRC
 */
static uint16 Fee_CalculateHeaderCrc(const Fee_BlockHeaderType* HeaderPtr)
{
    /* Calculate CRC of header excluding the HeaderCrc field */
    const uint8* dataPtr = (const uint8*)HeaderPtr;
    /* Skip the last 2 bytes which are HeaderCrc */
    return Fee_CalculateCrc16(dataPtr, sizeof(Fee_BlockHeaderType) - 2u);
}

/**
 * @brief Finds the flash address for a block
 */
static Std_ReturnType Fee_FindBlockAddress(Fee_BlockIdType BlockId, Fee_AddressType* AddressPtr)
{
    if (BlockId >= FEE_NUM_BLOCKS)
    {
        return E_NOT_OK;
    }

    if (Fee_BlockInfo[BlockId].IsValid == FALSE)
    {
        return E_NOT_OK;
    }

    *AddressPtr = Fee_BlockInfo[BlockId].BlockAddress + sizeof(Fee_BlockHeaderType);
    return E_OK;
}

/**
 * @brief Gets total block size including header
 */
static Fee_LengthType Fee_GetBlockTotalSize(uint16 DataSize)
{
    return FEE_ALIGN_TO_PAGE(sizeof(Fee_BlockHeaderType) + DataSize);
}

/**
 * @brief Selects a sector for writing based on wear leveling
 */
static uint8 Fee_SelectSectorForWrite(uint16 BlockSize)
{
    uint8 i;
    uint8 selectedSector = 0xFFu;
    uint32 minEraseCount = 0xFFFFFFFFu;

    /* Find sector with enough space and lowest erase count */
    for (i = 0u; i < FEE_NUM_SECTORS; i++)
    {
        if ((Fee_SectorInfo[i].IsValid == TRUE) &&
            (Fee_SectorInfo[i].FreeSpace >= Fee_GetBlockTotalSize(BlockSize)))
        {
            if (Fee_SectorInfo[i].EraseCycleCount < minEraseCount)
            {
                minEraseCount = Fee_SectorInfo[i].EraseCycleCount;
                selectedSector = i;
            }
        }
    }

    return selectedSector;
}

/**
 * @brief Finds the active sector
 */
static uint8 Fee_FindActiveSector(void)
{
    uint8 i;

    for (i = 0u; i < FEE_NUM_SECTORS; i++)
    {
        if (Fee_SectorInfo[i].IsActive == TRUE)
        {
            return i;
        }
    }

    return 0xFFu;  /* No active sector */
}

/**
 * @brief Finds a free sector for GC target
 */
static uint8 Fee_FindFreeSector(void)
{
    uint8 i;

    for (i = 0u; i < FEE_NUM_SECTORS; i++)
    {
        if ((Fee_SectorInfo[i].IsValid == TRUE) &&
            (Fee_SectorInfo[i].IsActive == FALSE) &&
            (Fee_SectorInfo[i].SectorState == FEE_BLOCK_STATE_EMPTY))
        {
            return i;
        }
    }

    return 0xFFu;  /* No free sector */
}

/**
 * @brief Checks if GC is needed
 */
static boolean Fee_IsGcNeeded(void)
{
    uint8 i;
    Fee_LengthType totalFreeSpace = 0u;
    Fee_LengthType totalSize = 0u;

    for (i = 0u; i < FEE_NUM_SECTORS; i++)
    {
        if (Fee_SectorInfo[i].IsValid == TRUE)
        {
            totalFreeSpace += Fee_SectorInfo[i].FreeSpace;
            totalSize += Fee_SectorInfo[i].SectorSize;
        }
    }

    /* GC needed if free space is below threshold */
    if (totalSize > 0u)
    {
        uint8 freePercent = (uint8)((totalFreeSpace * 100u) / totalSize);
        return (freePercent < FEE_GC_THRESHOLD_PERCENT);
    }

    return FALSE;
}

/**
 * @brief Starts garbage collection
 */
static void Fee_StartGarbageCollection(void)
{
    uint8 sourceSector;
    uint8 targetSector;

    /* Find source sector (oldest/active with obsolete data) */
    sourceSector = Fee_FindActiveSector();
    if (sourceSector == 0xFFu)
    {
        return;  /* No source sector */
    }

    /* Find target sector */
    targetSector = Fee_FindFreeSector();
    if (targetSector == 0xFFu)
    {
        return;  /* No target sector available */
    }

    /* Initialize GC control */
    Fee_GcControl.GcState = FEE_GC_STATE_SCAN;
    Fee_GcControl.SourceSectorIndex = sourceSector;
    Fee_GcControl.TargetSectorIndex = targetSector;
    Fee_GcControl.BlockIndex = 0u;
    Fee_GcControl.SourceAddress = Fee_SectorInfo[sourceSector].SectorStartAddress;
    Fee_GcControl.TargetAddress = Fee_SectorInfo[targetSector].SectorStartAddress;
    Fee_GcControl.BlocksCopied = 0u;
    Fee_GcControl.BlocksErased = 0u;

    /* Mark target sector as active for GC */
    Fee_SectorInfo[targetSector].SectorState = FEE_BLOCK_STATE_VALID;
}

/**
 * @brief Processes garbage collection
 */
static void Fee_ProcessGc(void)
{
    switch (Fee_GcControl.GcState)
    {
        case FEE_GC_STATE_SCAN:
            Fee_GcScanSector();
            break;

        case FEE_GC_STATE_COPY:
            Fee_GcCopyBlocks();
            break;

        case FEE_GC_STATE_ERASE:
            Fee_GcEraseSector();
            break;

        case FEE_GC_STATE_FINALIZE:
            Fee_GcFinalize();
            break;

        default:
            Fee_GcControl.GcState = FEE_GC_STATE_IDLE;
            break;
    }
}

/**
 * @brief GC: Scan sector for valid blocks
 */
static void Fee_GcScanSector(void)
{
    /* Move to copy state - actual scanning happens during copy */
    Fee_GcControl.GcState = FEE_GC_STATE_COPY;
}

/**
 * @brief GC: Copy valid blocks to target sector
 */
static void Fee_GcCopyBlocks(void)
{
    /* In a real implementation, this would:
     * 1. Read block from source sector
     * 2. Write to target sector
     * 3. Update block table
     * 4. Continue until all blocks are copied
     */

    /* For now, simulate completion */
    Fee_GcControl.GcState = FEE_GC_STATE_ERASE;
}

/**
 * @brief GC: Erase source sector
 */
static void Fee_GcEraseSector(void)
{
    Std_ReturnType result;

    /* Start FLS erase job */
    result = Fee_StartFlsErase(Fee_SectorInfo[Fee_GcControl.SourceSectorIndex].SectorStartAddress,
                                Fee_SectorInfo[Fee_GcControl.SourceSectorIndex].SectorSize);

    if (result == E_OK)
    {
        /* Wait for erase to complete - check in next MainFunction cycle */
        if (Fee_FlsJob.JobState == FEE_FLS_JOB_COMPLETED)
        {
            Fee_EraseCycleCounter++;
            Fee_SectorInfo[Fee_GcControl.SourceSectorIndex].EraseCycleCount++;
            Fee_GcControl.GcState = FEE_GC_STATE_FINALIZE;
        }
        else if (Fee_FlsJob.JobState == FEE_FLS_JOB_FAILED)
        {
            /* Erase failed, abort GC */
            Fee_GcControl.GcState = FEE_GC_STATE_IDLE;
        }
        /* else: still pending, wait */
    }
    else
    {
        /* Failed to start erase */
        Fee_GcControl.GcState = FEE_GC_STATE_IDLE;
    }
}

/**
 * @brief GC: Finalize garbage collection
 */
static void Fee_GcFinalize(void)
{
    uint8 sourceSector = Fee_GcControl.SourceSectorIndex;
    uint8 targetSector = Fee_GcControl.TargetSectorIndex;

    /* Reset source sector */
    Fee_SectorInfo[sourceSector].IsActive = FALSE;
    Fee_SectorInfo[sourceSector].SectorState = FEE_BLOCK_STATE_EMPTY;
    Fee_SectorInfo[sourceSector].NextWriteAddress = Fee_SectorInfo[sourceSector].SectorStartAddress;
    Fee_SectorInfo[sourceSector].FreeSpace = Fee_SectorInfo[sourceSector].SectorSize;

    /* Mark target sector as active */
    Fee_SectorInfo[targetSector].IsActive = TRUE;
    Fee_SectorInfo[targetSector].SectorState = FEE_BLOCK_STATE_VALID;

    /* Update GC counter */
    Fee_GcCycleCounter++;

    /* Clear GC state */
    Fee_GcControl.GcState = FEE_GC_STATE_IDLE;
    Fee_GcControl.SourceSectorIndex = 0u;
    Fee_GcControl.TargetSectorIndex = 0u;
}

/**
 * @brief Starts FLS read operation
 */
static Std_ReturnType Fee_StartFlsRead(Fls_AddressType Address, uint8* DataPtr, uint32 Length)
{
    if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
    {
        return E_NOT_OK;  /* Another FLS job is active */
    }

    Fee_FlsJob.JobType = FLS_JOB_READ;
    Fee_FlsJob.Address = Address;
    Fee_FlsJob.DataPtr = DataPtr;
    Fee_FlsJob.Length = Length;
    Fee_FlsJob.JobState = FEE_FLS_JOB_PENDING;

    Fls_Read(Address, DataPtr, Length);

    return E_OK;
}

/**
 * @brief Starts FLS write operation
 */
static Std_ReturnType Fee_StartFlsWrite(Fls_AddressType Address, const uint8* DataPtr, uint32 Length)
{
    if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
    {
        return E_NOT_OK;  /* Another FLS job is active */
    }

    Fee_FlsJob.JobType = FLS_JOB_WRITE;
    Fee_FlsJob.Address = Address;
    Fee_FlsJob.DataPtr = (uint8*)DataPtr;
    Fee_FlsJob.Length = Length;
    Fee_FlsJob.JobState = FEE_FLS_JOB_PENDING;

    return Fls_Write(Address, DataPtr, Length);
}

/**
 * @brief Starts FLS erase operation
 */
static Std_ReturnType Fee_StartFlsErase(Fls_AddressType Address, uint32 Length)
{
    if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
    {
        return E_NOT_OK;  /* Another FLS job is active */
    }

    Fee_FlsJob.JobType = FLS_JOB_ERASE;
    Fee_FlsJob.Address = Address;
    Fee_FlsJob.DataPtr = NULL_PTR;
    Fee_FlsJob.Length = Length;
    Fee_FlsJob.JobState = FEE_FLS_JOB_PENDING;

    return Fls_Erase(Address, Length);
}

/**
 * @brief Checks FLS job status
 */
static void Fee_CheckFlsJobStatus(void)
{
    MemIf_JobResultType flsResult;

    if (Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING)
    {
        flsResult = Fls_GetJobResult();

        if (flsResult == MEMIF_JOB_OK)
        {
            Fee_FlsJob.JobState = FEE_FLS_JOB_COMPLETED;
        }
        else if (flsResult == MEMIF_JOB_FAILED)
        {
            Fee_FlsJob.JobState = FEE_FLS_JOB_FAILED;
        }
        /* else: still pending */
    }
}

/**
 * @brief Processes the current job
 */
static void Fee_ProcessJob(void)
{
    switch (Fee_CurrentJob.JobType)
    {
        case FEE_JOB_READ:
            Fee_ProcessReadJob();
            break;

        case FEE_JOB_WRITE:
            Fee_ProcessWriteJob();
            break;

        case FEE_JOB_INVALIDATE:
            Fee_ProcessInvalidateJob();
            break;

        case FEE_JOB_ERASE_IMMEDIATE:
            Fee_ProcessEraseImmediateJob();
            break;

        default:
            Fee_FinishJob(FEE_JOB_FAILED);
            break;
    }
}

/**
 * @brief Processes a read job
 */
static void Fee_ProcessReadJob(void)
{
    Std_ReturnType result;
    Fee_AddressType blockAddress;
    uint32 headerSize = sizeof(Fee_BlockHeaderType);

    switch (Fee_CurrentJob.JobState)
    {
        case FEE_FLS_JOB_PENDING:
            /* Start reading block header */
            result = Fee_FindBlockAddress(Fee_CurrentJob.BlockId, &blockAddress);
            if (result != E_OK)
            {
                Fee_FinishJob(FEE_BLOCK_INVALID);
                return;
            }

            /* Start FLS read */
            result = Fee_StartFlsRead(blockAddress - headerSize + Fee_CurrentJob.BlockOffset,
                                       Fee_WorkBuffer,
                                       Fee_CurrentJob.DataLength);
            if (result != E_OK)
            {
                Fee_FinishJob(FEE_JOB_FAILED);
                return;
            }

            Fee_CurrentJob.JobState = FEE_FLS_JOB_PENDING + 1u;  /* Move to next state */
            break;

        case FEE_FLS_JOB_PENDING + 1u:
            /* Wait for FLS read to complete */
            if (Fee_FlsJob.JobState == FEE_FLS_JOB_COMPLETED)
            {
                /* Copy data to user buffer */
                Fee_Memcpy(Fee_CurrentJob.DataBufferPtr, Fee_WorkBuffer, Fee_CurrentJob.DataLength);
                Fee_FinishJob(FEE_JOB_OK);
            }
            else if (Fee_FlsJob.JobState == FEE_FLS_JOB_FAILED)
            {
                Fee_FinishJob(FEE_JOB_FAILED);
            }
            break;

        default:
            Fee_FinishJob(FEE_JOB_FAILED);
            break;
    }
}

/**
 * @brief Processes a write job
 */
static void Fee_ProcessWriteJob(void)
{
    Fee_BlockHeaderType header;
    uint8 sectorIndex;
    Fee_AddressType writeAddress;
    Std_ReturnType result;
    uint32 totalSize;

    switch (Fee_CurrentJob.JobState)
    {
        case FEE_FLS_JOB_PENDING:
            /* Select sector for write */
            sectorIndex = Fee_SelectSectorForWrite(Fee_CurrentJob.DataLength);
            if (sectorIndex == 0xFFu)
            {
                Fee_FinishJob(FEE_JOB_FAILED);
                return;
            }

            /* Build block header */
            header.MagicNumber = FEE_BLOCK_HEADER_MAGIC_VALID;
            header.BlockId = Fee_CurrentJob.BlockId;
            header.BlockSize = Fee_CurrentJob.DataLength;
            header.WriteCycleCount = Fee_BlockInfo[Fee_CurrentJob.BlockId].WriteCycleCounter + 1u;
            header.BlockState = FEE_BLOCK_STATE_VALID;
            header.Reserved[0] = 0u;
            header.Reserved[1] = 0u;
            header.Reserved[2] = 0u;

            /* Calculate CRCs */
            header.BlockCrc = Fee_CalculateCrc16(Fee_CurrentJob.DataBufferPtr, Fee_CurrentJob.DataLength);
            header.HeaderCrc = Fee_CalculateHeaderCrc(&header);

            /* Prepare write buffer */
            totalSize = Fee_GetBlockTotalSize(Fee_CurrentJob.DataLength);
            Fee_Memcpy(Fee_WorkBuffer, (uint8*)&header, sizeof(Fee_BlockHeaderType));
            Fee_Memcpy(&Fee_WorkBuffer[sizeof(Fee_BlockHeaderType)],
                       Fee_CurrentJob.DataBufferPtr,
                       Fee_CurrentJob.DataLength);

            /* Pad to page boundary */
            if (totalSize > (sizeof(Fee_BlockHeaderType) + Fee_CurrentJob.DataLength))
            {
                uint32 padSize = totalSize - sizeof(Fee_BlockHeaderType) - Fee_CurrentJob.DataLength;
                uint32 offset = sizeof(Fee_BlockHeaderType) + Fee_CurrentJob.DataLength;
                uint32 i;
                for (i = 0u; i < padSize; i++)
                {
                    Fee_WorkBuffer[offset + i] = 0xFFu;
                }
            }

            /* Start FLS write */
            writeAddress = Fee_SectorInfo[sectorIndex].NextWriteAddress;
            result = Fee_StartFlsWrite(writeAddress, Fee_WorkBuffer, totalSize);
            if (result != E_OK)
            {
                Fee_FinishJob(FEE_JOB_FAILED);
                return;
            }

            /* Store write address for later update */
            Fee_CurrentJob.BlockOffset = (uint16)sectorIndex;  /* Store sector index */
            Fee_CurrentJob.JobState = FEE_FLS_JOB_PENDING + 1u;
            break;

        case FEE_FLS_JOB_PENDING + 1u:
            /* Wait for FLS write to complete */
            if (Fee_FlsJob.JobState == FEE_FLS_JOB_COMPLETED)
            {
                /* Update block info */
                sectorIndex = (uint8)Fee_CurrentJob.BlockOffset;
                Fee_BlockInfo[Fee_CurrentJob.BlockId].BlockAddress =
                    Fee_SectorInfo[sectorIndex].NextWriteAddress;
                Fee_BlockInfo[Fee_CurrentJob.BlockId].BlockSize = Fee_CurrentJob.DataLength;
                Fee_BlockInfo[Fee_CurrentJob.BlockId].IsValid = TRUE;
                Fee_BlockInfo[Fee_CurrentJob.BlockId].IsInvalidated = FALSE;
                Fee_BlockInfo[Fee_CurrentJob.BlockId].WriteCycleCounter++;
                Fee_BlockInfo[Fee_CurrentJob.BlockId].BlockState = FEE_BLOCK_STATE_VALID;

                /* Update sector info */
                totalSize = Fee_GetBlockTotalSize(Fee_CurrentJob.DataLength);
                Fee_SectorInfo[sectorIndex].NextWriteAddress += totalSize;
                Fee_SectorInfo[sectorIndex].FreeSpace -= totalSize;

                /* Update global counters */
                Fee_WriteCycleCounter++;

                Fee_FinishJob(FEE_JOB_OK);
            }
            else if (Fee_FlsJob.JobState == FEE_FLS_JOB_FAILED)
            {
                Fee_FinishJob(FEE_JOB_FAILED);
            }
            break;

        default:
            Fee_FinishJob(FEE_JOB_FAILED);
            break;
    }
}

/**
 * @brief Processes an invalidate job
 */
static void Fee_ProcessInvalidateJob(void)
{
    /* Mark block as invalidated in runtime */
    Fee_BlockInfo[Fee_CurrentJob.BlockId].IsInvalidated = TRUE;
    Fee_BlockInfo[Fee_CurrentJob.BlockId].BlockState = FEE_BLOCK_STATE_INVALIDATED;

    /* In a full implementation, this would write an invalidation marker to flash */

    Fee_FinishJob(FEE_JOB_OK);
}

/**
 * @brief Processes an erase immediate job
 */
static void Fee_ProcessEraseImmediateJob(void)
{
    /* Mark block as erased in runtime */
    Fee_BlockInfo[Fee_CurrentJob.BlockId].IsValid = FALSE;
    Fee_BlockInfo[Fee_CurrentJob.BlockId].IsInvalidated = FALSE;
    Fee_BlockInfo[Fee_CurrentJob.BlockId].BlockState = FEE_BLOCK_STATE_EMPTY;

    /* In a full implementation, this would trigger sector erase if needed */

    Fee_FinishJob(FEE_JOB_OK);
}

/**
 * @brief Finishes the current job
 */
static void Fee_FinishJob(Fee_JobResultType Result)
{
    Fee_CurrentJob.JobResult = Result;
    Fee_CurrentJob.JobType = FEE_JOB_NONE;
    Fee_CurrentJob.JobState = FEE_FLS_JOB_IDLE;
    Fee_ModuleStatus = FEE_IDLE;

    /* Clear FLS job */
    Fee_FlsJob.JobState = FEE_FLS_JOB_IDLE;

    /* Notify upper layer */
    if (Result == FEE_JOB_OK)
    {
        Fee_JobEndNotification();
    }
    else
    {
        Fee_JobErrorNotification();
    }
}

/**
 * @brief Updates wear leveling information
 */
static void Fee_UpdateWearLeveling(void)
{
    /* This function would be called periodically to balance wear across sectors */
    /* For now, it's a placeholder for future implementation */
}

/**
 * @brief Safe memory copy
 */
static void Fee_Memcpy(uint8* Dest, const uint8* Src, uint32 Length)
{
    uint32 i;
    for (i = 0u; i < Length; i++)
    {
        Dest[i] = Src[i];
    }
}

/**
 * @brief Checks if address is within a sector
 */
static boolean Fee_IsAddressInSector(Fee_AddressType Address, uint8 SectorIndex)
{
    if (SectorIndex >= FEE_NUM_SECTORS)
    {
        return FALSE;
    }

    return ((Address >= Fee_SectorInfo[SectorIndex].SectorStartAddress) &&
            (Address < (Fee_SectorInfo[SectorIndex].SectorStartAddress +
                       Fee_SectorInfo[SectorIndex].SectorSize)));
}

#define FEE_STOP_SEC_CODE
#include "MemMap.h"

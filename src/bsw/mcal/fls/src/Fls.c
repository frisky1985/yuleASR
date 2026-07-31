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
 *                                      FLASH DRIVER
 *==================================================================================================
 * FILENAME: Fls.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashDriver.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Flash Driver module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Fls.h"
#include "Det.h"

/* Version checks */
#if defined(FLS_AR_RELEASE_MAJOR_VERSION) && (FLS_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Fls.c: Mismatch in AUTOSAR major version"
#endif

#if defined(FLS_SW_MAJOR_VERSION) && (FLS_SW_MAJOR_VERSION != 1u)
    #error "Fls.c: Mismatch in software major version"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
/* Job types */
#define FLS_JOB_NONE                    (0u)
#define FLS_JOB_ERASE                   (1u)
#define FLS_JOB_WRITE                   (2u)
#define FLS_JOB_READ                    (3u)
#define FLS_JOB_COMPARE                 (4u)

/* Internal state machine states */
#define FLS_STATE_IDLE                  (0u)
#define FLS_STATE_ERASING               (1u)
#define FLS_STATE_WRITING               (2u)
#define FLS_STATE_READING               (3u)
#define FLS_STATE_COMPARING             (4u)

/* Flash control register bits (example for ARM Cortex-M) */
#define FLS_CR_PG                       (0x00000001u)  /* Programming */
#define FLS_CR_PER                      (0x00000002u)  /* Page erase */
#define FLS_CR_MER                      (0x00000004u)  /* Mass erase */
#define FLS_CR_STRT                     (0x00010000u)  /* Start */
#define FLS_CR_LOCK                     (0x80000000u)  /* Lock */

/* Flash status register bits */
#define FLS_SR_EOP                      (0x00000001u)  /* End of operation */
#define FLS_SR_WRPRTERR                 (0x00000010u)  /* Write protection error */
#define FLS_SR_PGERR                    (0x00000004u)  /* Programming error */
#define FLS_SR_BSY                      (0x00000001u)  /* Busy */

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#define FLS_ENTER_CRITICAL_SECTION()    /* Disable interrupts - OS integration point */
#define FLS_EXIT_CRITICAL_SECTION()     /* Enable interrupts - OS integration point */

#define FLS_IS_ADDRESS_VALID(addr)      (((addr) >= FLS_BASE_ADDRESS) && \
                                         ((addr) < (FLS_BASE_ADDRESS + FLS_TOTAL_SIZE)))

#define FLS_IS_SECTOR_ALIGNED(addr)     (((addr) & (FLS_SECTOR_0_SIZE - 1u)) == 0u)

/*==================================================================================================
 *                                    LOCAL TYPEDEFS
 *==================================================================================================*/
typedef uint8 Fls_JobType;
typedef uint8 Fls_StateType;

/* Job control structure */
typedef struct {
    Fls_JobType jobType;                /* Current job type */
    Fls_AddressType address;            /* Current address */
    const uint8* writePtr;              /* Write data pointer */
    const uint8* comparePtr;            /* Compare data pointer */
    uint8* readPtr;                     /* Read data pointer */
    Fls_LengthType length;              /* Remaining length */
    Fls_LengthType processed;           /* Processed bytes */
    Fls_JobResultType result;           /* Job result */
} Fls_JobControlType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define FLS_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fls_MemMap.h"

/* Module configuration pointer */
const Fls_ConfigType* Fls_ConfigPtr = NULL_PTR;

/* Driver status */
Fls_StatusType Fls_Status = FLS_UNINIT;

/* Current operation mode */
static MemIf_ModeType Fls_CurrentMode = MEMIF_MODE_SLOW;

/* Internal state machine state */
static Fls_StateType Fls_State = FLS_STATE_IDLE;

/* Job control structure */
static Fls_JobControlType Fls_JobControl;

/* Timeout counter */
static uint32 Fls_TimeoutCounter = 0u;

#define FLS_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fls_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
#define FLS_START_SEC_CODE
#include "Fls_MemMap.h"

static void Fls_SetJobResult(Fls_JobResultType result);
static Std_ReturnType Fls_ValidateAddress(Fls_AddressType address, Fls_LengthType length);
static const Fls_SectorType* Fls_GetSector(Fls_AddressType address);
static void Fls_UnlockFlash(void);
static void Fls_LockFlash(void);
static Std_ReturnType Fls_EraseSector(Fls_AddressType address);
static Std_ReturnType Fls_WritePage(Fls_AddressType address, const uint8* data);
static void Fls_ReadData(Fls_AddressType address, uint8* data, Fls_LengthType length);
static void Fls_ProcessErase(void);
static void Fls_ProcessWrite(void);
static void Fls_ProcessRead(void);
static void Fls_ProcessCompare(void);
static void Fls_FinishJob(void);

/*==================================================================================================
 *                                       API IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Flash driver
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_Fls_00153
 */
void Fls_Init(const Fls_ConfigType* ConfigPtr)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check if already initialized */
    if (Fls_Status != FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_INIT, FLS_E_ALREADY_INITIALIZED);
        return;
    }
    
    /* Check parameter pointer */
    if (ConfigPtr == NULL_PTR)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_INIT, FLS_E_PARAM_CONFIG);
        return;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    
    /* Store configuration */
    Fls_ConfigPtr = ConfigPtr;
    
    /* Initialize state */
    Fls_Status = FLS_IDLE;
    Fls_State = FLS_STATE_IDLE;
    Fls_CurrentMode = MEMIF_MODE_SLOW;
    
    /* Clear job control */
    Fls_JobControl.jobType = FLS_JOB_NONE;
    Fls_JobControl.address = 0u;
    Fls_JobControl.writePtr = NULL_PTR;
    Fls_JobControl.readPtr = NULL_PTR;
    Fls_JobControl.comparePtr = NULL_PTR;
    Fls_JobControl.length = 0u;
    Fls_JobControl.processed = 0u;
    Fls_JobControl.result = MEMIF_JOB_OK;
    
    Fls_TimeoutCounter = 0u;
    
    FLS_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Erases a flash sector range
 * @param TargetAddress Target address in flash memory
 * @param Length Number of bytes to erase
 * @return E_OK: Job accepted, E_NOT_OK: Job rejected
 * @req SWS_Fls_00154
 */
Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_ERASE, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check if busy */
    if (Fls_Status == FLS_BUSY)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_ERASE, FLS_E_BUSY);
        return E_NOT_OK;
    }
    
    /* Validate address and length */
    if (Fls_ValidateAddress(TargetAddress, Length) != E_OK)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_ERASE, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
    
    /* Check sector alignment */
    if (!FLS_IS_SECTOR_ALIGNED(TargetAddress))
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_ERASE, FLS_E_INVALID_ADDRESS);
        return E_NOT_OK;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    
    if (Fls_Status == FLS_IDLE)
    {
        /* Setup job */
        Fls_JobControl.jobType = FLS_JOB_ERASE;
        Fls_JobControl.address = TargetAddress;
        Fls_JobControl.length = Length;
        Fls_JobControl.processed = 0u;
        Fls_JobControl.result = MEMIF_JOB_PENDING;
        
        Fls_Status = FLS_BUSY;
        Fls_State = FLS_STATE_ERASING;
        Fls_TimeoutCounter = FLS_TIMEOUT_VALUE;
        
        result = E_OK;
    }
    
    FLS_EXIT_CRITICAL_SECTION();
    
    return result;
}

/**
 * @brief Writes data to flash memory
 * @param TargetAddress Target address in flash memory
 * @param SourceAddress Pointer to source data buffer
 * @param Length Number of bytes to write
 * @return E_OK: Job accepted, E_NOT_OK: Job rejected
 * @req SWS_Fls_00155
 */
Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddress, Fls_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_WRITE, FLS_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* Check if busy */
    if (Fls_Status == FLS_BUSY)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_WRITE, FLS_E_BUSY);
        return E_NOT_OK;
    }
    
    /* Validate parameters */
    if (SourceAddress == NULL_PTR)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_WRITE, FLS_E_PARAM_DATA);
        return E_NOT_OK;
    }
    
    if (Fls_ValidateAddress(TargetAddress, Length) != E_OK)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_WRITE, FLS_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    
    if (Fls_Status == FLS_IDLE)
    {
        /* Setup job */
        Fls_JobControl.jobType = FLS_JOB_WRITE;
        Fls_JobControl.address = TargetAddress;
        Fls_JobControl.writePtr = SourceAddress;
        Fls_JobControl.length = Length;
        Fls_JobControl.processed = 0u;
        Fls_JobControl.result = MEMIF_JOB_PENDING;
        
        Fls_Status = FLS_BUSY;
        Fls_State = FLS_STATE_WRITING;
        Fls_TimeoutCounter = FLS_TIMEOUT_VALUE;
        
        result = E_OK;
    }
    
    FLS_EXIT_CRITICAL_SECTION();
    
    return result;
}

/**
 * @brief Reads data from flash memory
 * @param SourceAddress Source address in flash memory
 * @param TargetAddressPtr Pointer to target data buffer
 * @param Length Number of bytes to read
 * @return None
 * @req SWS_Fls_00156
 */
void Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_READ, FLS_E_UNINIT);
        return;
    }
    
    /* Check parameters */
    if (TargetAddressPtr == NULL_PTR)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_READ, FLS_E_PARAM_DATA);
        return;
    }
    
    if (Fls_ValidateAddress(SourceAddress, Length) != E_OK)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_READ, FLS_E_PARAM_ADDRESS);
        return;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    
    if (Fls_Status == FLS_IDLE)
    {
        /* Setup job */
        Fls_JobControl.jobType = FLS_JOB_READ;
        Fls_JobControl.address = SourceAddress;
        Fls_JobControl.readPtr = TargetAddressPtr;
        Fls_JobControl.length = Length;
        Fls_JobControl.processed = 0u;
        Fls_JobControl.result = MEMIF_JOB_PENDING;
        
        Fls_Status = FLS_BUSY;
        Fls_State = FLS_STATE_READING;
        Fls_TimeoutCounter = FLS_TIMEOUT_VALUE;
    }
    
    FLS_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Compares flash memory with data buffer
 * @param SourceAddress Source address in flash memory
 * @param TargetAddressPtr Pointer to data buffer to compare
 * @param Length Number of bytes to compare
 * @return None
 * @req SWS_Fls_00157
 */
void Fls_Compare(Fls_AddressType SourceAddress, const uint8* TargetAddressPtr, Fls_LengthType Length)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_COMPARE, FLS_E_UNINIT);
        return;
    }
    
    /* Check parameters */
    if (TargetAddressPtr == NULL_PTR)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_COMPARE, FLS_E_PARAM_DATA);
        return;
    }
    
    if (Fls_ValidateAddress(SourceAddress, Length) != E_OK)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_COMPARE, FLS_E_PARAM_ADDRESS);
        return;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    
    if (Fls_Status == FLS_IDLE)
    {
        /* Setup job */
        Fls_JobControl.jobType = FLS_JOB_COMPARE;
        Fls_JobControl.address = SourceAddress;
        Fls_JobControl.comparePtr = TargetAddressPtr;
        Fls_JobControl.length = Length;
        Fls_JobControl.processed = 0u;
        Fls_JobControl.result = MEMIF_JOB_PENDING;
        
        Fls_Status = FLS_BUSY;
        Fls_State = FLS_STATE_COMPARING;
        Fls_TimeoutCounter = FLS_TIMEOUT_VALUE;
    }
    
    FLS_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Sets the flash driver's operation mode
 * @param Mode Desired operation mode
 * @return None
 * @req SWS_Fls_00158
 */
void Fls_SetMode(MemIf_ModeType Mode)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_SETMODE, FLS_E_UNINIT);
        return;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    Fls_CurrentMode = Mode;
    FLS_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Gets the current driver status
 * @return Fls_StatusType: Current driver status
 * @req SWS_Fls_00159
 */
Fls_StatusType Fls_GetStatus(void)
{
    return Fls_Status;
}

/**
 * @brief Gets the result of the last job
 * @return Fls_JobResultType: Result of last job
 * @req SWS_Fls_00160
 */
Fls_JobResultType Fls_GetJobResult(void)
{
    Fls_JobResultType result;
    
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_GETJOBRESULT, FLS_E_UNINIT);
        return MEMIF_JOB_FAILED;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    result = Fls_JobControl.result;
    FLS_EXIT_CRITICAL_SECTION();
    
    return result;
}

/**
 * @brief Cancels an ongoing flash job
 * @return None
 * @req SWS_Fls_00161
 */
void Fls_Cancel(void)
{
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Fls_Status == FLS_UNINIT)
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_CANCEL, FLS_E_UNINIT);
        return;
    }
#endif
    
    FLS_ENTER_CRITICAL_SECTION();
    
    if (Fls_Status == FLS_BUSY)
    {
        /* Cancel the job */
        Fls_SetJobResult(MEMIF_JOB_CANCELED);
        Fls_Status = FLS_IDLE;
        Fls_State = FLS_STATE_IDLE;
    }
    
    FLS_EXIT_CRITICAL_SECTION();
}

/**
 * @brief Main function for processing flash jobs
 * @return None
 * @req SWS_Fls_00162
 */
void Fls_MainFunction(void)
{
    if (Fls_Status != FLS_BUSY)
    {
        return;
    }
    
    /* Process current job */
    switch (Fls_State)
    {
        case FLS_STATE_ERASING:
            Fls_ProcessErase();
            break;
            
        case FLS_STATE_WRITING:
            Fls_ProcessWrite();
            break;
            
        case FLS_STATE_READING:
            Fls_ProcessRead();
            break;
            
        case FLS_STATE_COMPARING:
            Fls_ProcessCompare();
            break;
            
        default:
            /* Invalid state */
            Fls_SetJobResult(MEMIF_JOB_FAILED);
            Fls_Status = FLS_IDLE;
            Fls_State = FLS_STATE_IDLE;
            break;
    }
    
    /* Check timeout */
    if (Fls_TimeoutCounter > 0u)
    {
        Fls_TimeoutCounter--;
        if (Fls_TimeoutCounter == 0u)
        {
            Fls_SetJobResult(MEMIF_JOB_FAILED);
            Fls_Status = FLS_IDLE;
            Fls_State = FLS_STATE_IDLE;
            
#if (FLS_RUNTIME_ERROR_DETECT == STD_ON)
            (void)Det_ReportRuntimeError(FLS_MODULE_ID, FLS_INSTANCE_ID, 0u, FLS_E_ERASE_FAILED);
#endif
        }
    }
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Fls_00163
 */
#if (FLS_VERSION_INFO_API == STD_ON)
void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = FLS_VENDOR_ID;
        versioninfo->moduleID = FLS_MODULE_ID;
        versioninfo->sw_major_version = FLS_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = FLS_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = FLS_SW_PATCH_VERSION;
    }
#if (FLS_DEV_ERROR_DETECT == STD_ON)
    else
    {
        (void)Det_ReportError(FLS_MODULE_ID, FLS_INSTANCE_ID, FLS_SID_GETVERSIONINFO, FLS_E_PARAM_POINTER);
    }
#endif
}
#endif

/*==================================================================================================
 *                                    LOCAL FUNCTION IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Sets job result and triggers notification
 */
static void Fls_SetJobResult(Fls_JobResultType result)
{
    Fls_JobControl.result = result;
    
    if (result == MEMIF_JOB_OK)
    {
#if (FLS_JOB_END_NOTIFICATION == STD_ON)
        Fls_JobEndNotification();
#endif
    }
    else if (result != MEMIF_JOB_PENDING && result != MEMIF_JOB_CANCELED)
    {
#if (FLS_JOB_ERROR_NOTIFICATION == STD_ON)
        Fls_JobErrorNotification();
#endif
    }
}

/**
 * @brief Validates flash address and length
 */
static Std_ReturnType Fls_ValidateAddress(Fls_AddressType address, Fls_LengthType length)
{
    if (!FLS_IS_ADDRESS_VALID(address))
    {
        return E_NOT_OK;
    }
    
    if (length == 0u)
    {
        return E_NOT_OK;
    }
    
    if ((address + length) > (FLS_BASE_ADDRESS + FLS_TOTAL_SIZE))
    {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Gets sector configuration for given address
 */
static const Fls_SectorType* Fls_GetSector(Fls_AddressType address)
{
    const Fls_SectorType* sector = NULL_PTR;
    
    if (Fls_ConfigPtr != NULL_PTR)
    {
        uint32 i;
        for (i = 0u; i < Fls_ConfigPtr->sectorCount; i++)
        {
            const Fls_SectorType* s = &Fls_ConfigPtr->sectorList[i];
            if ((address >= s->sectorStartAddr) && 
                (address < (s->sectorStartAddr + s->sectorSize)))
            {
                sector = s;
                break;
            }
        }
    }
    
    return sector;
}

/**
 * @brief Unlocks flash for programming/erasing
 */
static void Fls_UnlockFlash(void)
{
    /* Hardware-specific unlock sequence */
    /* Example: Write KEY1 and KEY2 to FLASH_KEYR register */
}

/**
 * @brief Locks flash after programming/erasing
 */
static void Fls_LockFlash(void)
{
    /* Hardware-specific lock sequence */
    /* Example: Set LOCK bit in FLASH_CR register */
}

/**
 * @brief Erases a single flash sector
 */
static Std_ReturnType Fls_EraseSector(Fls_AddressType address)
{
    /* Hardware-specific sector erase */
    /* This is a simplified implementation */
    
    /* 1. Unlock flash */
    Fls_UnlockFlash();
    
    /* 2. Select sector erase */
    /* FLASH->CR |= FLS_CR_PER; */
    
    /* 3. Set sector address */
    /* FLASH->AR = address; */
    
    /* 4. Start erase */
    /* FLASH->CR |= FLS_CR_STRT; */
    
    /* 5. Wait for completion (in real implementation) */
    /* while (FLASH->SR & FLS_SR_BSY); */
    
    /* 6. Lock flash */
    Fls_LockFlash();
    
    return E_OK;
}

/**
 * @brief Writes a single page to flash
 */
static Std_ReturnType Fls_WritePage(Fls_AddressType address, const uint8* data)
{
    /* Hardware-specific page write */
    /* This is a simplified implementation */
    
    /* 1. Unlock flash */
    Fls_UnlockFlash();
    
    /* 2. Enable programming */
    /* FLASH->CR |= FLS_CR_PG; */
    
    /* 3. Write data (32-bit words for ARM) */
    /* *((volatile uint32*)address) = *((const uint32*)data); */
    
    /* 4. Wait for completion */
    /* while (FLASH->SR & FLS_SR_BSY); */
    
    /* 5. Lock flash */
    Fls_LockFlash();
    
    return E_OK;
}

/**
 * @brief Reads data from flash
 */
static void Fls_ReadData(Fls_AddressType address, uint8* data, Fls_LengthType length)
{
    /* Flash can be read directly like RAM */
    Fls_LengthType i;
    for (i = 0u; i < length; i++)
    {
        data[i] = ((const uint8*)(uintptr)address)[i];
    }
}

/**
 * @brief Processes erase job
 */
static void Fls_ProcessErase(void)
{
    const Fls_SectorType* sector;
    Fls_AddressType currentAddr;
    
    /* Calculate current address */
    currentAddr = Fls_JobControl.address + Fls_JobControl.processed;
    
    /* Get sector for current address */
    sector = Fls_GetSector(currentAddr);
    
    if (sector != NULL_PTR)
    {
        /* Erase the sector */
        if (Fls_EraseSector(currentAddr) == E_OK)
        {
            /* Move to next sector */
            Fls_JobControl.processed += sector->sectorSize;
            
            /* Check if done */
            if (Fls_JobControl.processed >= Fls_JobControl.length)
            {
                Fls_SetJobResult(MEMIF_JOB_OK);
                Fls_FinishJob();
            }
        }
        else
        {
            Fls_SetJobResult(MEMIF_JOB_FAILED);
            Fls_FinishJob();
        }
    }
    else
    {
        Fls_SetJobResult(MEMIF_JOB_FAILED);
        Fls_FinishJob();
    }
}

/**
 * @brief Processes write job
 */
static void Fls_ProcessWrite(void)
{
    const Fls_SectorType* sector;
    Fls_AddressType currentAddr;
    Fls_LengthType chunkSize;
    
    /* Calculate current address */
    currentAddr = Fls_JobControl.address + Fls_JobControl.processed;
    
    /* Get sector for current address */
    sector = Fls_GetSector(currentAddr);
    
    if (sector != NULL_PTR)
    {
        /* Determine chunk size based on mode */
        if (Fls_CurrentMode == MEMIF_MODE_FAST)
        {
            chunkSize = (Fls_ConfigPtr != NULL_PTR) ? Fls_ConfigPtr->maxWriteFastMode : FLS_MAX_WRITE_FAST_MODE;
        }
        else
        {
            chunkSize = (Fls_ConfigPtr != NULL_PTR) ? Fls_ConfigPtr->maxWriteNormalMode : FLS_MAX_WRITE_NORMAL_MODE;
        }
        
        /* Limit chunk to remaining length */
        if (chunkSize > (Fls_JobControl.length - Fls_JobControl.processed))
        {
            chunkSize = Fls_JobControl.length - Fls_JobControl.processed;
        }
        
        /* Write chunk */
        if (Fls_WritePage(currentAddr, &Fls_JobControl.writePtr[Fls_JobControl.processed]) == E_OK)
        {
            Fls_JobControl.processed += chunkSize;
            
            /* Check if done */
            if (Fls_JobControl.processed >= Fls_JobControl.length)
            {
                Fls_SetJobResult(MEMIF_JOB_OK);
                Fls_FinishJob();
            }
        }
        else
        {
            Fls_SetJobResult(MEMIF_JOB_FAILED);
            Fls_FinishJob();
        }
    }
    else
    {
        Fls_SetJobResult(MEMIF_JOB_FAILED);
        Fls_FinishJob();
    }
}

/**
 * @brief Processes read job
 */
static void Fls_ProcessRead(void)
{
    Fls_AddressType currentAddr;
    Fls_LengthType chunkSize;
    
    /* Calculate current address */
    currentAddr = Fls_JobControl.address + Fls_JobControl.processed;
    
    /* Determine chunk size based on mode */
    if (Fls_CurrentMode == MEMIF_MODE_FAST)
    {
        chunkSize = (Fls_ConfigPtr != NULL_PTR) ? Fls_ConfigPtr->maxReadFastMode : FLS_MAX_READ_FAST_MODE;
    }
    else
    {
        chunkSize = (Fls_ConfigPtr != NULL_PTR) ? Fls_ConfigPtr->maxReadNormalMode : FLS_MAX_READ_NORMAL_MODE;
    }
    
    /* Limit chunk to remaining length */
    if (chunkSize > (Fls_JobControl.length - Fls_JobControl.processed))
    {
        chunkSize = Fls_JobControl.length - Fls_JobControl.processed;
    }
    
    /* Read chunk */
    Fls_ReadData(currentAddr, &Fls_JobControl.readPtr[Fls_JobControl.processed], chunkSize);
    
    Fls_JobControl.processed += chunkSize;
    
    /* Check if done */
    if (Fls_JobControl.processed >= Fls_JobControl.length)
    {
        Fls_SetJobResult(MEMIF_JOB_OK);
        Fls_FinishJob();
    }
}

/**
 * @brief Processes compare job
 */
static void Fls_ProcessCompare(void)
{
    Fls_AddressType currentAddr;
    Fls_LengthType chunkSize;
    uint8 readBuffer[256];
    Fls_LengthType i;
    boolean match;
    
    /* Calculate current address */
    currentAddr = Fls_JobControl.address + Fls_JobControl.processed;
    
    /* Determine chunk size */
    chunkSize = 256u;
    if (chunkSize > (Fls_JobControl.length - Fls_JobControl.processed))
    {
        chunkSize = Fls_JobControl.length - Fls_JobControl.processed;
    }
    
    /* Read chunk from flash */
    Fls_ReadData(currentAddr, readBuffer, chunkSize);
    
    /* Compare */
    match = TRUE;
    for (i = 0u; i < chunkSize; i++)
    {
        if (readBuffer[i] != Fls_JobControl.comparePtr[Fls_JobControl.processed + i])
        {
            match = FALSE;
            break;
        }
    }
    
    if (match)
    {
        Fls_JobControl.processed += chunkSize;
        
        /* Check if done */
        if (Fls_JobControl.processed >= Fls_JobControl.length)
        {
            Fls_SetJobResult(MEMIF_JOB_OK);
            Fls_FinishJob();
        }
    }
    else
    {
        Fls_SetJobResult(MEMIF_BLOCK_INCONSISTENT);
        Fls_FinishJob();
    }
}

/**
 * @brief Finishes current job
 */
static void Fls_FinishJob(void)
{
    Fls_Status = FLS_IDLE;
    Fls_State = FLS_STATE_IDLE;
    Fls_JobControl.jobType = FLS_JOB_NONE;
}

#define FLS_STOP_SEC_CODE
#include "Fls_MemMap.h"

/*==================================================================================================
 *                                      END OF FILE
 *==================================================================================================*/

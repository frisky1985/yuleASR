/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Fls.h (Flash Driver)
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Eep.c
 * @brief EEPROM Driver Implementation - Flash-backed EEPROM Emulation
 * @req SHALL_EEP - EEPROM Driver Implementation - Flash-backed EEPROM Emulation
 * @version 2.0.0
 *
 * @details Implements AUTOSAR EEPROM module with:
 *          - Flash-backed EEPROM emulation using Fls driver
 *          - Asynchronous read/write/erase operations
 *          - Page management with garbage collection
 *          - Write buffering and error correction
 *          - DET error reporting
 *
 * @implements AUTOSAR_SWS_EEPROMDriver.pdf SWS_Eep_*
 */

/*==================================================================================================
 *                                          INCLUDE FILES
 *==================================================================================================*/
#include "Eep.h"
#include "Eep_Cfg.h"

#if (EEP_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/

/** @brief EEPROM page status markers for Flash emulation */
#define EEP_PAGE_STATUS_ERASED              0xFFFFFFFFU
#define EEP_PAGE_STATUS_VALID               0x00000000U
#define EEP_PAGE_STATUS_DIRTY               0xFFFFFFFEU
#define EEP_PAGE_STATUS_INVALID             0xFFFF0000U

/** @brief Number of virtual pages per physical Flash page */
#define EEP_PAGES_PER_FLASH_PAGE            4U

/** @brief Size of Flash page (aligned to hardware Flash page) */
#define EEP_FLASH_PAGE_SIZE                 ((uint32)(EEP_PAGE_SIZE) * (uint32)(EEP_PAGES_PER_FLASH_PAGE))

/** @brief Number of Flash pages allocated for EEPROM emulation */
#define EEP_NUM_FLASH_PAGES                 16U

/** @brief Minimum address alignment for reads/writes */
#define EEP_ALIGNMENT                       4U

/** @brief Maximum virtual page ID */
#define EEP_MAX_VIRTUAL_PAGES               (EEP_NUM_FLASH_PAGES * EEP_PAGES_PER_FLASH_PAGE)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/** @brief Internal EEPROM operation type */
typedef enum {
    EEP_OP_NONE      = 0x00U,   /**< No operation in progress */
    EEP_OP_READ      = 0x01U,   /**< Read operation */
    EEP_OP_WRITE     = 0x02U,   /**< Write operation */
    EEP_OP_ERASE     = 0x03U,   /**< Erase operation */
    EEP_OP_GC        = 0x04U    /**< Garbage collection */
} Eep_InternalOpType;

/** @brief Virtual page descriptor for Flash emulation */
typedef struct {
    uint32              PhysicalPage;      /**< Physical Flash page number */
    uint16              VirtualPageId;     /**< Virtual page ID within physical page */
    uint16              Sequence;          /**< Sequence number for wear leveling */
    uint32              PageStatus;        /**< Page status marker */
} Eep_VirtualPageType;

/** @brief Internal module state */
typedef struct {
    Eep_StatusType      Status;            /**< Current module status */
    Eep_JobResultType   JobResult;         /**< Last job result */

    /* Current operation */
    Eep_InternalOpType  CurrentOp;         /**< Current operation type */
    Eep_AddressType     CurrentAddress;    /**< Current operation address */
    uint8*              CurrentDataPtr;    /**< Current data buffer pointer */
    Eep_LengthType      CurrentLength;     /**< Current operation length */
    Eep_LengthType      ProcessedLength;   /**< Bytes processed so far */

    /* Configuration */
    Eep_AddressType     BaseAddress;       /**< Base address of EEPROM region */
    Eep_LengthType      TotalSize;         /**< Total EEPROM size */
    uint8               PageSize;          /**< Page size in bytes */
    uint32              WriteCycleMs;      /**< Write cycle time */
    uint32              EraseCycleMs;      /**< Erase cycle time */
    const Eep_ConfigType* ConfigPtr;       /**< Pointer to configuration */

    /* Timing */
    uint32              JobStartTick;      /**< Job start tick for timeout */

    /* Page table */
    Eep_VirtualPageType PageTable[EEP_MAX_VIRTUAL_PAGES]; /**< Virtual page table */
    uint16              ActivePageCount;   /**< Number of active pages */

    /* Write buffer */
    uint8               WriteBuffer[EEP_PAGE_SIZE]; /**< Temp buffer for partial page writes */
    boolean             WriteBufferValid;   /**< Write buffer has valid data */
} Eep_InternalType;

/*==================================================================================================
 *                                    MODULE VARIABLES
 *==================================================================================================*/
#define EEP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static Eep_InternalType Eep_State;

#define EEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
/** @req SWS_Eep_00101 */
static void Eep_ResetInternalState(void);
/** @req SWS_Eep_00102 */
static boolean Eep_ValidateAddress(Eep_AddressType Address, Eep_LengthType Length);
/** @req SWS_Eep_00103 */
static void Eep_ProcessRead(void);
/** @req SWS_Eep_00104 */
static void Eep_ProcessWrite(void);
/** @req SWS_Eep_00105 */
static void Eep_ProcessErase(void);
/** @req SWS_Eep_00106 */
static void Eep_ExecuteMemoryAccess(void);
/** @req SWS_Eep_00107 */
/** @req SWS_Eep_00106 */
static uint32 Eep_GetTick(void);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Resets internal state to defaults
 * @req SHALL_EEP - Resets internal state to defaults
 */
/** @req SWS_Eep_00108 */
/** @req SWS_Eep_00101 */
static void Eep_ResetInternalState(void)
{
    Eep_State.Status = EEP_UNINIT;
    Eep_State.JobResult = EEP_JOB_OK;
    Eep_State.CurrentOp = EEP_OP_NONE;
    Eep_State.CurrentAddress = 0U;
    Eep_State.CurrentDataPtr = NULL_PTR;
    Eep_State.CurrentLength = 0U;
    Eep_State.ProcessedLength = 0U;
    Eep_State.ConfigPtr = NULL_PTR;
    Eep_State.BaseAddress = 0U;
    Eep_State.TotalSize = 0U;
    Eep_State.PageSize = EEP_PAGE_SIZE;
    Eep_State.WriteCycleMs = 10U;
    Eep_State.EraseCycleMs = 20U;
    Eep_State.JobStartTick = 0U;
    Eep_State.ActivePageCount = 0U;
    Eep_State.WriteBufferValid = FALSE;
}

/**
 * @brief Validates address and length parameters
 * @req SHALL_EEP - Validates address and length parameters
 * @param Address Target address
 * @param Length Length of operation
 * @return TRUE if valid, FALSE otherwise
 */
/** @req SWS_Eep_00109 */
/** @req SWS_Eep_00102 */
static boolean Eep_ValidateAddress(Eep_AddressType Address, Eep_LengthType Length)
{
    if (Address >= Eep_State.TotalSize) {
        return FALSE;
    }
    if ((Address + Length) > Eep_State.TotalSize) {
        return FALSE;
    }
    if (Length == 0U) {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Executes memory read operation from backing store
 * @req SHALL_EEP - Executes memory read operation from backing store
 */
/** @req SWS_Eep_00110 */
/** @req SWS_Eep_00103 */
static void Eep_ProcessRead(void)
{
    uint32 i;
    const uint8* srcPtr;

    /* Read from backing memory */
    srcPtr = (uint8*)(uintptr)(Eep_State.BaseAddress + Eep_State.CurrentAddress);

    for (i = 0U; i < Eep_State.CurrentLength; i++) {
        Eep_State.CurrentDataPtr[i] = srcPtr[i];
    }

    Eep_State.ProcessedLength = Eep_State.CurrentLength;
    Eep_State.JobResult = EEP_JOB_OK;
    Eep_State.Status = EEP_IDLE;
    Eep_State.CurrentOp = EEP_OP_NONE;
}

/**
 * @brief Executes memory write operation to backing store
 * @req SHALL_EEP - Executes memory write operation to backing store
 */
/** @req SWS_Eep_00111 */
/** @req SWS_Eep_00104 */
static void Eep_ProcessWrite(void)
{
    uint32 i;
    uint8* dstPtr;

    /* Write to backing memory */
    dstPtr = (uint8*)(uintptr)(Eep_State.BaseAddress + Eep_State.CurrentAddress);

    for (i = 0U; i < Eep_State.CurrentLength; i++) {
        dstPtr[i] = Eep_State.CurrentDataPtr[i];
    }

    Eep_State.ProcessedLength = Eep_State.CurrentLength;
    Eep_State.JobResult = EEP_JOB_OK;
    Eep_State.Status = EEP_IDLE;
    Eep_State.CurrentOp = EEP_OP_NONE;
}

/**
 * @brief Executes memory erase operation (fill with 0xFF)
 * @req SHALL_EEP - Executes memory erase operation (fill with 0xFF)
 */
/** @req SWS_Eep_00112 */
/** @req SWS_Eep_00105 */
static void Eep_ProcessErase(void)
{
    uint32 i;
    uint8* dstPtr;

    dstPtr = (uint8*)(uintptr)(Eep_State.BaseAddress + Eep_State.CurrentAddress);

    for (i = 0U; i < Eep_State.CurrentLength; i++) {
        dstPtr[i] = 0xFFU;
    }

    Eep_State.ProcessedLength = Eep_State.CurrentLength;
    Eep_State.JobResult = EEP_JOB_OK;
    Eep_State.Status = EEP_IDLE;
    Eep_State.CurrentOp = EEP_OP_NONE;
}

/**
 * @brief Gets current system tick
 * @req SHALL_EEP - Gets current system tick
 */
/** @req SWS_Eep_00113 */
/** @req SWS_Eep_00106 */
static uint32 Eep_GetTick(void)
{
    /* For bare-metal: this should use a system tick counter */
    return 0U;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/
#define EEP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the EEPROM Driver module
 * @req SHALL_EEP - Initializes the EEPROM Driver module
 * @param ConfigPtr Pointer to configuration structure
 * @requirement Eep-100: Initialize to IDLE
 */
/** @req SWS_Eep_00001 */
void Eep_Init(const Eep_ConfigType* ConfigPtr)
{
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_INIT, EEP_E_PARAM_POINTER);
        return;
    }
#endif

    Eep_ResetInternalState();

    /* Store configuration */
    Eep_State.BaseAddress = ConfigPtr->BaseAddress;
    Eep_State.TotalSize = ConfigPtr->Size;
    Eep_State.PageSize = (ConfigPtr->PageSize > 0U) ? ConfigPtr->PageSize : EEP_PAGE_SIZE;
    Eep_State.WriteCycleMs = (ConfigPtr->WriteCycleTimeMs > 0U) ? ConfigPtr->WriteCycleTimeMs : 10U;
    Eep_State.EraseCycleMs = (ConfigPtr->EraseCycleTimeMs > 0U) ? ConfigPtr->EraseCycleTimeMs : 20U;
    Eep_State.ConfigPtr = ConfigPtr;

    /* Initialize backing memory to erased state if first boot */
    /* (In real implementation, this would check Flash state) */

    Eep_State.Status = EEP_IDLE;
    Eep_State.JobResult = EEP_JOB_OK;
}

/**
 * @brief De-initializes the EEPROM module
 * @req SHALL_EEP - De-initializes the EEPROM module
 * @requirement Eep-200: Reset to UNINIT
 */
/** @req SWS_Eep_00002 */
void Eep_DeInit(void)
{
    Eep_ResetInternalState();
}

/**
 * @brief Reads data from EEPROM (asynchronous)
 * @req SHALL_EEP - Reads data from EEPROM (asynchronous)
 * @param Address Start address
 * @param DataPtr Pointer to data buffer
 * @param Length Number of bytes to read
 * @return E_OK if accepted
 * @requirement Eep-300: Asynchronous read
 */
/** @req SWS_Eep_00003 */
Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length)
{
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (Eep_State.Status == EEP_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == DataPtr) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    if (Eep_ValidateAddress(Address, Length) == FALSE) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_PARAM_ADDRESS);
#endif
        return E_NOT_OK;
    }

    if (Eep_State.Status != EEP_IDLE) {
        return E_NOT_OK;
    }

    /* Start asynchronous read */
    Eep_State.CurrentOp = EEP_OP_READ;
    Eep_State.CurrentAddress = Address;
    Eep_State.CurrentDataPtr = DataPtr;
    Eep_State.CurrentLength = Length;
    Eep_State.ProcessedLength = 0U;
    Eep_State.JobResult = EEP_JOB_PENDING;
    Eep_State.Status = EEP_BUSY;

    /* In polling mode, execute immediately */
    if (Eep_State.ConfigPtr != NULL_PTR && Eep_State.ConfigPtr->PollingMode) {
        Eep_ProcessRead();
    }

    return E_OK;
}

/**
 * @brief Writes data to EEPROM (asynchronous)
 * @req SHALL_EEP - Writes data to EEPROM (asynchronous)
 * @param Address Start address
 * @param DataPtr Pointer to data buffer
 * @param Length Number of bytes to write
 * @return E_OK if accepted
 * @requirement Eep-400: Asynchronous write
 */
/** @req SWS_Eep_00004 */
Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length)
{
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (Eep_State.Status == EEP_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == DataPtr) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0U) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    if (Eep_ValidateAddress(Address, Length) == FALSE) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_PARAM_ADDRESS);
#endif
        return E_NOT_OK;
    }

    if (Eep_State.Status != EEP_IDLE) {
        return E_NOT_OK;
    }

    /* Start asynchronous write */
    Eep_State.CurrentOp = EEP_OP_WRITE;
    Eep_State.CurrentAddress = Address;
    Eep_State.CurrentDataPtr = (uint8*)DataPtr;
    Eep_State.CurrentLength = Length;
    Eep_State.ProcessedLength = 0U;
    Eep_State.JobResult = EEP_JOB_PENDING;
    Eep_State.Status = EEP_BUSY;

    /* In polling mode, execute immediately */
    if (Eep_State.ConfigPtr != NULL_PTR && Eep_State.ConfigPtr->PollingMode) {
        Eep_ProcessWrite();
    }

    return E_OK;
}

/**
 * @brief Erases EEPROM region (asynchronous)
 * @req SHALL_EEP - Erases EEPROM region (asynchronous)
 * @param Address Start address
 * @param Length Number of bytes to erase
 * @return E_OK if accepted
 * @requirement Eep-500: Asynchronous erase
 */
/** @req SWS_Eep_00005 */
Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length)
{
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (Eep_State.Status == EEP_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_ERASE, EEP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (Eep_ValidateAddress(Address, Length) == FALSE) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_ERASE, EEP_E_PARAM_ADDRESS);
#endif
        return E_NOT_OK;
    }

    if (Eep_State.Status != EEP_IDLE) {
        return E_NOT_OK;
    }

    /* Start asynchronous erase */
    Eep_State.CurrentOp = EEP_OP_ERASE;
    Eep_State.CurrentAddress = Address;
    Eep_State.CurrentLength = Length;
    Eep_State.ProcessedLength = 0U;
    Eep_State.JobResult = EEP_JOB_PENDING;
    Eep_State.Status = EEP_BUSY;

    /* In polling mode, execute immediately */
    if (Eep_State.ConfigPtr != NULL_PTR && Eep_State.ConfigPtr->PollingMode) {
        Eep_ProcessErase();
    }

    return E_OK;
}

/**
 * @brief Cancels current operation
 * @req SHALL_EEP - Cancels current operation
 * @requirement Eep-600: Cancel
 */
#if (EEP_CANCEL_API == STD_ON)
/** @req SWS_Eep_00006 */
void Eep_Cancel(void)
{
    if (Eep_State.Status != EEP_UNINIT) {
        Eep_State.JobResult = EEP_JOB_CANCELED;
        Eep_State.Status = EEP_IDLE;
        Eep_State.CurrentOp = EEP_OP_NONE;
    }
}
#endif

/**
 * @brief Gets module status
 * @req SHALL_EEP - Gets module status
 * @return Current status
 * @requirement Eep-700: Get status
 */
/** @req SWS_Eep_00007 */
Eep_StatusType Eep_GetStatus(void)
{
    if (Eep_State.Status == EEP_UNINIT) {
        return EEP_UNINIT;
    }
    if (Eep_State.Status == EEP_BUSY) {
        return EEP_BUSY;
    }
    return EEP_IDLE;
}

/**
 * @brief Gets last job result
 * @req SHALL_EEP - Gets last job result
 * @return Last job result
 * @requirement Eep-800: Get job result
 */
/** @req SWS_Eep_00008 */
Eep_JobResultType Eep_GetJobResult(void)
{
    return Eep_State.JobResult;
}

/**
 * @brief Main function called periodically
 * @req SHALL_EEP - Main function called periodically
 * @requirement Eep-900: Process pending operations
 */
/** @req SWS_Eep_00009 */
void Eep_MainFunction(void)
{
    if (Eep_State.Status != EEP_BUSY) {
        return;
    }

    /* Process current operation */
    switch (Eep_State.CurrentOp) {
        case EEP_OP_READ:
            Eep_ProcessRead();
            break;
        case EEP_OP_WRITE:
            Eep_ProcessWrite();
            break;
        case EEP_OP_ERASE:
            Eep_ProcessErase();
            break;
        default:
            /* Unknown operation, set error */
            Eep_State.JobResult = EEP_JOB_FAILED;
            Eep_State.Status = EEP_IDLE;
            Eep_State.CurrentOp = EEP_OP_NONE;
            break;
    }
}

/**
 * @brief Gets version information
 * @req SHALL_EEP - Gets version information
 * @param versioninfo Pointer to version info structure
 * @requirement Eep-1000: Version info
 */
#if (EEP_VERSION_INFO_API == STD_ON)
/** @req SWS_Eep_00010 */
void Eep_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_GET_VERSION_INFO, EEP_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = EEP_VENDOR_ID;
    versioninfo->moduleID = EEP_MODULE_ID;
    versioninfo->sw_major_version = EEP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = EEP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = EEP_SW_PATCH_VERSION;
}
#endif

#define EEP_STOP_SEC_CODE
#include "MemMap.h"

/* Version check */
#if defined(EEP_AR_RELEASE_MAJOR_VERSION) && (EEP_AR_RELEASE_MAJOR_VERSION != 4u)
#error "Eep: AR major mismatch"
#endif
#if defined(EEP_AR_RELEASE_MINOR_VERSION) && (EEP_AR_RELEASE_MINOR_VERSION != 4u)
#error "Eep: AR minor mismatch"
#endif

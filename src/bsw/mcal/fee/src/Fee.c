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
 *                                      FLASH EEPROM EMULATION DRIVER
 *                                      (MCAL LAYER)
 *==================================================================================================
 * FILENAME: Fee.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: MCAL layer Fee driver implementation
 *              Provides flash access services for EEPROM emulation
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Fee.h"
#include "Fee_Cfg.h"
#include "Det.h"
#include "SchM_Fee.h"

/*==================================================================================================
 *                                    VERSION CHECK
 *==================================================================================================*/
#if (FEE_SW_MAJOR_VERSION != 1)
    #error "Fee.c: Mismatch in software major version"
#endif

#if (FEE_SW_MINOR_VERSION != 0U )
    #error "Fee.c: Mismatch in software minor version"
#endif

#if (FEE_SW_PATCH_VERSION != 0U )
    #error "Fee.c: Mismatch in software patch version"
#endif

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#define FEE_SECTOR_STATE_FREE           (0x00U)
#define FEE_SECTOR_STATE_ACTIVE         (0x01U)
#define FEE_SECTOR_STATE_FULL           (0x02U)
#define FEE_SECTOR_STATE_ERASING        (0x03U)

#define FEE_JOB_NONE                    (0x00U)
#define FEE_JOB_READ                    (0x01U)
#define FEE_JOB_WRITE                   (0x02U)
#define FEE_JOB_ERASE                   (0x03U)
#define FEE_JOB_COMPARE                 (0x04U)
#define FEE_JOB_BLANKCHECK              (0x05U)

#define FEE_FLAG_INITIALIZED            (0x01U)
#define FEE_FLAG_BUSY                   (0x02U)
#define FEE_FLAG_SUSPENDED              (0x04U)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

typedef struct {
    uint8 flags;
    Fee_ModeType currentMode;
    Fee_JobResultType jobResult;
    Fee_StateType state;
} Fee_DriverStateType;

typedef struct {
    uint8 jobType;
    Fee_AddressType address;
    Fee_LengthType length;
    const uint8* dataPtr;
    Fee_LengthType bytesProcessed;
} Fee_JobInfoType;

typedef struct {
    uint8 state;
    uint32 writeCount;
    uint32 eraseCount;
    Fee_AddressType nextWriteAddr;
} Fee_SectorStateType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define FEE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static Fee_DriverStateType Fee_DriverState = {
    .flags = 0U,
    .currentMode = FEE_MODE_NORMAL,
    .jobResult = FEE_JOB_OK,
    .state = FEE_UNINIT
};

static Fee_JobInfoType Fee_CurrentJob = {
    .jobType = FEE_JOB_NONE,
    .address = 0U,
    .length = 0U,
    .dataPtr = NULL_PTR,
    .bytesProcessed = 0U
};

static Fee_SectorStateType Fee_SectorStates[FEE_NUM_SECTORS];

static const Fee_ConfigType* Fee_ConfigPtr = NULL_PTR;

#define FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static void Fee_InitSectors(void);
static Std_ReturnType Fee_ValidateAddress(Fee_AddressType Address, Fee_LengthType Length);
static Std_ReturnType Fee_ValidateLength(Fee_LengthType Length);
static Std_ReturnType Fee_ValidateDataPtr(const uint8* DataPtr);
static void Fee_SetJobResult(Fee_JobResultType Result);
static void Fee_ProcessRead(void);
static void Fee_ProcessWrite(void);
static void Fee_ProcessErase(void);
static void Fee_ProcessCompare(void);
static void Fee_ProcessBlankCheck(void);
static Std_ReturnType Fee_FlashRead(Fee_AddressType Address, uint8* DestPtr, Fee_LengthType Length);
static Std_ReturnType Fee_FlashWrite(Fee_AddressType Address, const uint8* SourcePtr, Fee_LengthType Length);
static Std_ReturnType Fee_FlashErase(Fee_AddressType Address, Fee_LengthType Length);
static boolean Fee_IsAddressInSector(Fee_AddressType Address, uint8 SectorIdx);
static uint8 Fee_FindSectorForAddress(Fee_AddressType Address);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initialize sector states
 */
static void Fee_InitSectors(void)
{
    uint8 i;
    
    for (i = 0U; i < FEE_NUM_SECTORS; i++) {
        Fee_SectorStates[i].state = FEE_SECTOR_STATE_FREE;
        Fee_SectorStates[i].writeCount = 0U;
        Fee_SectorStates[i].eraseCount = 0U;
        
        if (Fee_ConfigPtr != NULL_PTR && Fee_ConfigPtr->sectorList != NULL_PTR) {
            Fee_SectorStates[i].nextWriteAddr = Fee_ConfigPtr->sectorList[i].sectorStartAddr;
        } else {
            Fee_SectorStates[i].nextWriteAddr = 0U;
        }
    }
}

/**
 * @brief Validate flash address
 */
static Std_ReturnType Fee_ValidateAddress(Fee_AddressType Address, Fee_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;
    Fee_AddressType endAddress;
    
    if (Fee_ConfigPtr == NULL_PTR || Fee_ConfigPtr->sectorList == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Check for overflow */
    if (Address > (0xFFFFFFFFU - Length)) {
        return E_NOT_OK;
    }
    endAddress = Address + Length;
    
    /* Check if address range is within configured sectors */
    for (uint8 i = 0U; i < Fee_ConfigPtr->sectorCount; i++) {
        Fee_AddressType sectorStart = Fee_ConfigPtr->sectorList[i].sectorStartAddr;
        Fee_AddressType sectorEnd = sectorStart + Fee_ConfigPtr->sectorList[i].sectorSize;
        
        if ((Address >= sectorStart) && (endAddress <= sectorEnd)) {
            result = E_OK;
            break;
        }
    }
    
    return result;
}

/**
 * @brief Validate length parameter
 */
static Std_ReturnType Fee_ValidateLength(Fee_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Length > 0U) {
        /* Check alignment to virtual page size */
        if ((Length % FEE_VIRTUAL_PAGE_SIZE) == 0U) {
            result = E_OK;
        }
    }
    
    return result;
}

/**
 * @brief Validate data pointer
 */
static Std_ReturnType Fee_ValidateDataPtr(const uint8* DataPtr)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (DataPtr != NULL_PTR) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Set job result
 */
static void Fee_SetJobResult(Fee_JobResultType Result)
{
    Fee_DriverState.jobResult = Result;
}

/**
 * @brief Process read job
 */
static void Fee_ProcessRead(void)
{
    Fee_LengthType bytesToRead;
    Fee_LengthType maxBytes;
    
    if (Fee_DriverState.currentMode == FEE_MODE_FAST) {
        maxBytes = (Fee_ConfigPtr != NULL_PTR) ? Fee_ConfigPtr->maxReadFastMode : FEE_MAX_READ_FAST_MODE;
    } else {
        maxBytes = (Fee_ConfigPtr != NULL_PTR) ? Fee_ConfigPtr->maxReadNormalMode : FEE_MAX_READ_NORMAL_MODE;
    }
    
    bytesToRead = Fee_CurrentJob.length - Fee_CurrentJob.bytesProcessed;
    if (bytesToRead > maxBytes) {
        bytesToRead = maxBytes;
    }
    
    /* Perform flash read - hardware abstraction would go here */
    /* For now, simulate successful read */
    Fee_CurrentJob.bytesProcessed += bytesToRead;
    
    if (Fee_CurrentJob.bytesProcessed >= Fee_CurrentJob.length) {
        /* Read complete */
        Fee_CurrentJob.jobType = FEE_JOB_NONE;
        Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
        Fee_SetJobResult(FEE_JOB_OK);
        
        #if (FEE_JOB_END_NOTIFICATION_ENABLED == STD_ON)
        Fee_JobEndNotification();
        #endif
    }
    /* else: more bytes to read in next MainFunction cycle */
}

/**
 * @brief Process write job
 */
static void Fee_ProcessWrite(void)
{
    Fee_LengthType bytesToWrite;
    Fee_LengthType maxBytes;
    Std_ReturnType status;
    
    if (Fee_DriverState.currentMode == FEE_MODE_FAST) {
        maxBytes = (Fee_ConfigPtr != NULL_PTR) ? Fee_ConfigPtr->maxWriteFastMode : FEE_MAX_WRITE_FAST_MODE;
    } else {
        maxBytes = (Fee_ConfigPtr != NULL_PTR) ? Fee_ConfigPtr->maxWriteNormalMode : FEE_MAX_WRITE_NORMAL_MODE;
    }
    
    bytesToWrite = Fee_CurrentJob.length - Fee_CurrentJob.bytesProcessed;
    if (bytesToWrite > maxBytes) {
        bytesToWrite = maxBytes;
    }
    
    /* Perform flash write - hardware abstraction would go here */
    status = Fee_FlashWrite(
        Fee_CurrentJob.address + Fee_CurrentJob.bytesProcessed,
        &Fee_CurrentJob.dataPtr[Fee_CurrentJob.bytesProcessed],
        bytesToWrite
    );
    
    if (status == E_OK) {
        Fee_CurrentJob.bytesProcessed += bytesToWrite;
        
        if (Fee_CurrentJob.bytesProcessed >= Fee_CurrentJob.length) {
            /* Write complete */
            Fee_CurrentJob.jobType = FEE_JOB_NONE;
            Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
            Fee_SetJobResult(FEE_JOB_OK);
            
            #if (FEE_JOB_END_NOTIFICATION_ENABLED == STD_ON)
            Fee_JobEndNotification();
            #endif
        }
    } else {
        /* Write failed */
        Fee_CurrentJob.jobType = FEE_JOB_NONE;
        Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
        Fee_SetJobResult(FEE_JOB_FAILED);
        
        #if (FEE_JOB_ERROR_NOTIFICATION_ENABLED == STD_ON)
        Fee_JobErrorNotification();
        #endif
    }
}

/**
 * @brief Process erase job
 */
static void Fee_ProcessErase(void)
{
    Std_ReturnType status;
    
    /* Perform flash erase - hardware abstraction would go here */
    status = Fee_FlashErase(Fee_CurrentJob.address, Fee_CurrentJob.length);
    
    if (status == E_OK) {
        Fee_CurrentJob.jobType = FEE_JOB_NONE;
        Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
        Fee_SetJobResult(FEE_JOB_OK);
        
        #if (FEE_JOB_END_NOTIFICATION_ENABLED == STD_ON)
        Fee_JobEndNotification();
        #endif
    } else {
        Fee_CurrentJob.jobType = FEE_JOB_NONE;
        Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
        Fee_SetJobResult(FEE_JOB_FAILED);
        
        #if (FEE_JOB_ERROR_NOTIFICATION_ENABLED == STD_ON)
        Fee_JobErrorNotification();
        #endif
    }
}

/**
 * @brief Process compare job
 */
static void Fee_ProcessCompare(void)
{
    /* Compare operation - to be implemented with hardware abstraction */
    Fee_CurrentJob.jobType = FEE_JOB_NONE;
    Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
    Fee_SetJobResult(FEE_JOB_OK);
    
    #if (FEE_JOB_END_NOTIFICATION_ENABLED == STD_ON)
    Fee_JobEndNotification();
    #endif
}

/**
 * @brief Process blank check job
 */
static void Fee_ProcessBlankCheck(void)
{
    /* Blank check operation - to be implemented with hardware abstraction */
    Fee_CurrentJob.jobType = FEE_JOB_NONE;
    Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
    Fee_SetJobResult(FEE_JOB_OK);
    
    #if (FEE_JOB_END_NOTIFICATION_ENABLED == STD_ON)
    Fee_JobEndNotification();
    #endif
}

/**
 * @brief Low-level flash read (hardware abstraction)
 */
static Std_ReturnType Fee_FlashRead(Fee_AddressType Address, uint8* DestPtr, Fee_LengthType Length)
{
    /* Hardware-specific flash read implementation */
    /* This would interface with the actual flash controller */
    (void)Address;
    (void)DestPtr;
    (void)Length;
    return E_OK;
}

/**
 * @brief Low-level flash write (hardware abstraction)
 */
static Std_ReturnType Fee_FlashWrite(Fee_AddressType Address, const uint8* SourcePtr, Fee_LengthType Length)
{
    /* Hardware-specific flash write implementation */
    /* This would interface with the actual flash controller */
    (void)Address;
    (void)SourcePtr;
    (void)Length;
    return E_OK;
}

/**
 * @brief Low-level flash erase (hardware abstraction)
 */
static Std_ReturnType Fee_FlashErase(Fee_AddressType Address, Fee_LengthType Length)
{
    /* Hardware-specific flash erase implementation */
    /* This would interface with the actual flash controller */
    (void)Address;
    (void)Length;
    return E_OK;
}

/**
 * @brief Check if address is within a sector
 */
static boolean Fee_IsAddressInSector(Fee_AddressType Address, uint8 SectorIdx)
{
    boolean result = FALSE;
    
    if ((Fee_ConfigPtr != NULL_PTR) && 
        (Fee_ConfigPtr->sectorList != NULL_PTR) &&
        (SectorIdx < Fee_ConfigPtr->sectorCount)) {
        Fee_AddressType sectorStart = Fee_ConfigPtr->sectorList[SectorIdx].sectorStartAddr;
        Fee_AddressType sectorEnd = sectorStart + Fee_ConfigPtr->sectorList[SectorIdx].sectorSize;
        
        if ((Address >= sectorStart) && (Address < sectorEnd)) {
            result = TRUE;
        }
    }
    
    return result;
}

/**
 * @brief Find sector index for given address
 */
static uint8 Fee_FindSectorForAddress(Fee_AddressType Address)
{
    uint8 sectorIdx = FEE_NUM_SECTORS;  /* Invalid sector */
    
    if (Fee_ConfigPtr != NULL_PTR && Fee_ConfigPtr->sectorList != NULL_PTR) {
        for (uint8 i = 0U; i < Fee_ConfigPtr->sectorCount; i++) {
            if (Fee_IsAddressInSector(Address, i)) {
                sectorIdx = i;
                break;
            }
        }
    }
    
    return sectorIdx;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Fee driver
 */
Std_ReturnType Fee_Init(const Fee_ConfigType* ConfigPtr)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INIT, FEE_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_INIT, FEE_E_ALREADY_INITIALIZED);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_ConfigPtr = ConfigPtr;
    Fee_DriverState.flags = FEE_FLAG_INITIALIZED;
    Fee_DriverState.currentMode = ConfigPtr->defaultMode;
    Fee_DriverState.state = FEE_IDLE;
    Fee_DriverState.jobResult = FEE_JOB_OK;
    
    Fee_InitSectors();
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief De-initializes the Fee driver
 */
Std_ReturnType Fee_DeInit(void)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_DEINIT, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_DEINIT, FEE_E_BUSY);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_ConfigPtr = NULL_PTR;
    Fee_DriverState.flags = 0U;
    Fee_DriverState.currentMode = FEE_MODE_NORMAL;
    Fee_DriverState.state = FEE_UNINIT;
    Fee_DriverState.jobResult = FEE_JOB_OK;
    
    Fee_CurrentJob.jobType = FEE_JOB_NONE;
    Fee_CurrentJob.address = 0U;
    Fee_CurrentJob.length = 0U;
    Fee_CurrentJob.dataPtr = NULL_PTR;
    Fee_CurrentJob.bytesProcessed = 0U;
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Sets the operation mode
 */
Std_ReturnType Fee_SetMode(Fee_ModeType Mode)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SETMODE, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((Mode != FEE_MODE_NORMAL) && (Mode != FEE_MODE_FAST)) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SETMODE, FEE_E_INVALID_MODE);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    Fee_DriverState.currentMode = Mode;
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Reads data from flash
 */
Std_ReturnType Fee_Read(Fee_AddressType SourceAddress,
                         Fee_LengthType Length,
                         uint8* DestPtr)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateDataPtr(DestPtr) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateLength(Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_LENGTH);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateAddress(SourceAddress, Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_INVALID_ADDRESS);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_READ, FEE_E_BUSY);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_CurrentJob.jobType = FEE_JOB_READ;
    Fee_CurrentJob.address = SourceAddress;
    Fee_CurrentJob.length = Length;
    Fee_CurrentJob.dataPtr = DestPtr;
    Fee_CurrentJob.bytesProcessed = 0U;
    
    Fee_DriverState.flags |= FEE_FLAG_BUSY;
    Fee_DriverState.state = FEE_BUSY;
    Fee_SetJobResult(FEE_JOB_PENDING);
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Writes data to flash
 */
Std_ReturnType Fee_Write(Fee_AddressType TargetAddress,
                          Fee_LengthType Length,
                          const uint8* SourcePtr)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateDataPtr(SourcePtr) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateLength(Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_INVALID_LENGTH);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateAddress(TargetAddress, Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_INVALID_ADDRESS);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_WRITE, FEE_E_BUSY);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_CurrentJob.jobType = FEE_JOB_WRITE;
    Fee_CurrentJob.address = TargetAddress;
    Fee_CurrentJob.length = Length;
    Fee_CurrentJob.dataPtr = SourcePtr;
    Fee_CurrentJob.bytesProcessed = 0U;
    
    Fee_DriverState.flags |= FEE_FLAG_BUSY;
    Fee_DriverState.state = FEE_BUSY;
    Fee_SetJobResult(FEE_JOB_PENDING);
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Erases flash sector(s)
 */
Std_ReturnType Fee_Erase(Fee_AddressType TargetAddress,
                          Fee_LengthType Length)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASE, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateLength(Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASE, FEE_E_INVALID_LENGTH);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateAddress(TargetAddress, Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASE, FEE_E_INVALID_ADDRESS);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_ERASE, FEE_E_BUSY);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_CurrentJob.jobType = FEE_JOB_ERASE;
    Fee_CurrentJob.address = TargetAddress;
    Fee_CurrentJob.length = Length;
    Fee_CurrentJob.dataPtr = NULL_PTR;
    Fee_CurrentJob.bytesProcessed = 0U;
    
    Fee_DriverState.flags |= FEE_FLAG_BUSY;
    Fee_DriverState.state = FEE_BUSY;
    Fee_SetJobResult(FEE_JOB_PENDING);
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Compares flash data with buffer
 */
Std_ReturnType Fee_Compare(Fee_AddressType SourceAddress,
                            Fee_LengthType Length,
                            const uint8* DataPtr)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_COMPARE, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateDataPtr(DataPtr) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_COMPARE, FEE_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateLength(Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_COMPARE, FEE_E_INVALID_LENGTH);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateAddress(SourceAddress, Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_COMPARE, FEE_E_INVALID_ADDRESS);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_COMPARE, FEE_E_BUSY);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_CurrentJob.jobType = FEE_JOB_COMPARE;
    Fee_CurrentJob.address = SourceAddress;
    Fee_CurrentJob.length = Length;
    Fee_CurrentJob.dataPtr = DataPtr;
    Fee_CurrentJob.bytesProcessed = 0U;
    
    Fee_DriverState.flags |= FEE_FLAG_BUSY;
    Fee_DriverState.state = FEE_BUSY;
    Fee_SetJobResult(FEE_JOB_PENDING);
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Checks if flash area is blank
 */
Std_ReturnType Fee_BlankCheck(Fee_AddressType TargetAddress,
                               Fee_LengthType Length)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_BLANKCHECK, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateLength(Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_BLANKCHECK, FEE_E_INVALID_LENGTH);
        return E_NOT_OK;
    }
    
    if (Fee_ValidateAddress(TargetAddress, Length) != E_OK) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_BLANKCHECK, FEE_E_INVALID_ADDRESS);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_BLANKCHECK, FEE_E_BUSY);
        return E_NOT_OK;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_CurrentJob.jobType = FEE_JOB_BLANKCHECK;
    Fee_CurrentJob.address = TargetAddress;
    Fee_CurrentJob.length = Length;
    Fee_CurrentJob.dataPtr = NULL_PTR;
    Fee_CurrentJob.bytesProcessed = 0U;
    
    Fee_DriverState.flags |= FEE_FLAG_BUSY;
    Fee_DriverState.state = FEE_BUSY;
    Fee_SetJobResult(FEE_JOB_PENDING);
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Gets the driver status
 */
Fee_StateType Fee_GetStatus(void)
{
    Fee_StateType status = FEE_UNINIT;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETSTATUS, FEE_E_UNINIT);
        return FEE_UNINIT;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    status = Fee_DriverState.state;
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return status;
}

/**
 * @brief Gets the result of the last job
 */
Fee_JobResultType Fee_GetJobResult(void)
{
    Fee_JobResultType result = FEE_JOB_FAILED;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETJOBRESULT, FEE_E_UNINIT);
        return FEE_JOB_FAILED;
    }
    #endif
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    result = Fee_DriverState.jobResult;
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Cancels the ongoing job
 */
Std_ReturnType Fee_Cancel(void)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_CANCEL, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_CANCEL, FEE_E_INVALID_CANCEL);
        return E_NOT_OK;
    }
    #endif
    
    #if (FEE_CANCEL_SUPPORT == STD_ON)
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    Fee_CurrentJob.jobType = FEE_JOB_NONE;
    Fee_DriverState.flags &= ~FEE_FLAG_BUSY;
    Fee_DriverState.state = FEE_IDLE;
    Fee_SetJobResult(FEE_JOB_CANCELLED);
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    #else
    result = E_NOT_OK;
    #endif
    
    return result;
}

/**
 * @brief Suspends the ongoing erase operation
 */
Std_ReturnType Fee_Suspend(void)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SUSPEND, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_SUSPEND, FEE_E_INVALID_SUSPEND);
        return E_NOT_OK;
    }
    #endif
    
    #if (FEE_ERASE_SUSPEND_SUPPORT == STD_ON)
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    Fee_DriverState.flags |= FEE_FLAG_SUSPENDED;
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    #else
    result = E_NOT_OK;
    #endif
    
    return result;
}

/**
 * @brief Resumes a suspended erase operation
 */
Std_ReturnType Fee_Resume(void)
{
    Std_ReturnType result = E_OK;
    
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_RESUME, FEE_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((Fee_DriverState.flags & FEE_FLAG_SUSPENDED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_RESUME, FEE_E_INVALID_RESUME);
        return E_NOT_OK;
    }
    #endif
    
    #if (FEE_ERASE_SUSPEND_SUPPORT == STD_ON)
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    Fee_DriverState.flags &= ~FEE_FLAG_SUSPENDED;
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
    #else
    result = E_NOT_OK;
    #endif
    
    return result;
}

/**
 * @brief Gets version information
 */
#if (FEE_VERSION_INFO_API == STD_ON)
void Fee_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_GETVERSIONINFO, FEE_E_PARAM_POINTER);
        return;
    }
    #endif
    
    VersionInfoPtr->vendorID = FEE_VENDOR_ID;
    VersionInfoPtr->moduleID = FEE_MODULE_ID;
    VersionInfoPtr->sw_major_version = FEE_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = FEE_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = FEE_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Main function for processing asynchronous jobs
 */
void Fee_MainFunction(void)
{
    #if (FEE_DEV_ERROR_DETECT == STD_ON)
    if ((Fee_DriverState.flags & FEE_FLAG_INITIALIZED) == 0U) {
        Det_ReportError(FEE_MODULE_ID, FEE_INSTANCE_ID, FEE_SID_MAINFUNCTION, FEE_E_UNINIT);
        return;
    }
    #endif
    
    /* Check if suspended */
    if ((Fee_DriverState.flags & FEE_FLAG_SUSPENDED) != 0U) {
        return;
    }
    
    SchM_Enter_Fee_FEE_EXCLUSIVE_AREA_0();
    
    /* Process current job */
    if ((Fee_DriverState.flags & FEE_FLAG_BUSY) != 0U) {
        switch (Fee_CurrentJob.jobType) {
            case FEE_JOB_READ:
                Fee_ProcessRead();
                break;
            case FEE_JOB_WRITE:
                Fee_ProcessWrite();
                break;
            case FEE_JOB_ERASE:
                Fee_ProcessErase();
                break;
            case FEE_JOB_COMPARE:
                Fee_ProcessCompare();
                break;
            case FEE_JOB_BLANKCHECK:
                Fee_ProcessBlankCheck();
                break;
            default:
                /* No job or unknown job type */
                break;
        }
    }
    
    SchM_Exit_Fee_FEE_EXCLUSIVE_AREA_0();
}

/**
 * @brief Job end notification callback
 */
void Fee_JobEndNotification(void)
{
    /* Callback function - can be configured for upper layer notification */
}

/**
 * @brief Job error notification callback
 */
void Fee_JobErrorNotification(void)
{
    /* Callback function - can be configured for upper layer notification */
}

/*==================================================================================================
 *                                    HELPER FUNCTION IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Get next state based on current state and job type
 */
Fee_InternalStateType Fee_GetNextState(Fee_InternalStateType CurrentState, Fee_JobType JobType)
{
    Fee_InternalStateType nextState = CurrentState;
    
    /* Simple state transition logic */
    switch (CurrentState) {
        case FEE_STATE_IDLE:
            switch (JobType) {
                case FEE_JOB_READ:
                    nextState = FEE_STATE_READ_HEADER;
                    break;
                case FEE_JOB_WRITE:
                    nextState = FEE_STATE_WRITE_HEADER;
                    break;
                case FEE_JOB_ERASE_IMMEDIATE:
                    nextState = FEE_STATE_ERASE_IMMEDIATE;
                    break;
                case FEE_JOB_GC_PAGE:
                    nextState = FEE_STATE_GC_COPY;
                    break;
                default:
                    break;
            }
            break;
        case FEE_STATE_READ_HEADER:
            if (JobType == FEE_JOB_READ) {
                nextState = FEE_STATE_READ_DATA;
            }
            break;
        case FEE_STATE_READ_DATA:
            if (JobType == FEE_JOB_READ) {
                nextState = FEE_STATE_IDLE;
            }
            break;
        case FEE_STATE_WRITE_HEADER:
            if (JobType == FEE_JOB_WRITE) {
                nextState = FEE_STATE_WRITE_DATA;
            }
            break;
        case FEE_STATE_WRITE_DATA:
            if (JobType == FEE_JOB_WRITE) {
                nextState = FEE_STATE_IDLE;
            }
            break;
        case FEE_STATE_ERASE_IMMEDIATE:
            if (JobType == FEE_JOB_ERASE_IMMEDIATE) {
                nextState = FEE_STATE_IDLE;
            }
            break;
        case FEE_STATE_GC_COPY:
            if (JobType == FEE_JOB_GC_PAGE) {
                nextState = FEE_STATE_GC_ERASE;
            }
            break;
        case FEE_STATE_GC_ERASE:
            if (JobType == FEE_JOB_GC_PAGE) {
                nextState = FEE_STATE_IDLE;
            }
            break;
        default:
            break;
    }
    
    return nextState;
}

/**
 * @brief Check if state transition is valid
 */
boolean Fee_IsStateTransitionValid(Fee_InternalStateType CurrentState, Fee_JobType JobType)
{
    boolean valid = FALSE;
    
    switch (CurrentState) {
        case FEE_STATE_IDLE:
            valid = (JobType == FEE_JOB_READ || 
                     JobType == FEE_JOB_WRITE || 
                     JobType == FEE_JOB_ERASE_IMMEDIATE || 
                     JobType == FEE_JOB_GC_PAGE);
            break;
        case FEE_STATE_READ_HEADER:
            valid = (JobType == FEE_JOB_READ);
            break;
        case FEE_STATE_READ_DATA:
            valid = (JobType == FEE_JOB_READ);
            break;
        case FEE_STATE_WRITE_HEADER:
            valid = (JobType == FEE_JOB_WRITE);
            break;
        case FEE_STATE_WRITE_DATA:
            valid = (JobType == FEE_JOB_WRITE);
            break;
        case FEE_STATE_ERASE_IMMEDIATE:
            valid = (JobType == FEE_JOB_ERASE_IMMEDIATE);
            break;
        case FEE_STATE_GC_COPY:
            valid = (JobType == FEE_JOB_GC_PAGE);
            break;
        case FEE_STATE_GC_ERASE:
            valid = (JobType == FEE_JOB_GC_PAGE);
            break;
        default:
            break;
    }
    
    return valid;
}

/**
 * @brief Update wear leveling counters
 */
void Fee_UpdateWearLeveling(uint8 PageIndex, uint8 Operation)
{
    (void)PageIndex;
    (void)Operation;
    /* Implementation would update wear leveling tracking structures */
}

/**
 * @brief Get preferred page for garbage collection
 */
uint8 Fee_GetPreferredPageForGc(void)
{
    /* Return page 0 as default - real implementation would check erase counts */
    return 0U;
}

/**
 * @brief Get block configuration by block number
 */
const Fee_BlockConfigType* Fee_GetBlockConfig(uint16 BlockNumber)
{
    const Fee_BlockConfigType* blockConfig = NULL_PTR;
    
    if (Fee_ConfigPtr != NULL_PTR && Fee_ConfigPtr->FeeBlockConfig != NULL_PTR) {
        for (uint16 i = 0U; i < Fee_ConfigPtr->FeeNumberOfBlocks; i++) {
            if (Fee_ConfigPtr->FeeBlockConfig[i].FeeBlockNumber == BlockNumber) {
                blockConfig = &Fee_ConfigPtr->FeeBlockConfig[i];
                break;
            }
        }
    }
    
    return blockConfig;
}

/**
 * @brief Get page configuration by page number
 */
const Fee_PageConfigType* Fee_GetPageConfig(uint8 PageNumber)
{
    const Fee_PageConfigType* pageConfig = NULL_PTR;
    
    if (Fee_ConfigPtr != NULL_PTR && Fee_ConfigPtr->FeePageConfig != NULL_PTR) {
        for (uint8 i = 0U; i < Fee_ConfigPtr->FeeNumberOfPages; i++) {
            if (Fee_ConfigPtr->FeePageConfig[i].PageNumber == PageNumber) {
                pageConfig = &Fee_ConfigPtr->FeePageConfig[i];
                break;
            }
        }
    }
    
    return pageConfig;
}

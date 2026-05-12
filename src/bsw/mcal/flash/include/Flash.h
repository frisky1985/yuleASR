/************************************************************************************
 * File:       Flash.h
 * Description: AUTOSAR Flash Driver (Fls) API Header
 * Author:      YuleTech AutoSAR Team
 * Version:     1.0.0
 * Date:        2025
 *
 * AUTOSAR Version: 4.4.0
 * Target:        ARM Cortex-M4/M7 (STM32H743)
 ************************************************************************************/

#ifndef FLASH_H
#define FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************
 *                                   Includes
 ************************************************************************************/
#include "Std_Types.h"
#include "Flash_Cfg.h"

/************************************************************************************
 *                              Version Information
 ************************************************************************************/
#define FLS_VENDOR_ID                   (uint16)0x0001U
#define FLS_MODULE_ID                   (uint16)0x005CU
#define FLS_INSTANCE_ID                 (uint8)0x00U

#define FLS_SW_MAJOR_VERSION            (uint8)1U
#define FLS_SW_MINOR_VERSION            (uint8)0U
#define FLS_SW_PATCH_VERSION            (uint8)0U

#define FLS_AR_MAJOR_VERSION            (uint8)4U
#define FLS_AR_MINOR_VERSION            (uint8)4U
#define FLS_AR_PATCH_VERSION            (uint8)0U

/************************************************************************************
 *                              Service IDs
 ************************************************************************************/
#define FLS_INIT_SID                    (uint8)0x00U
#define FLS_ERASE_SID                   (uint8)0x01U
#define FLS_WRITE_SID                   (uint8)0x02U
#define FLS_READ_SID                    (uint8)0x07U
#define FLS_CANCEL_SID                  (uint8)0x03U
#define FLS_GETSTATUS_SID               (uint8)0x04U
#define FLS_GETJOBRESULT_SID            (uint8)0x05U
#define FLS_COMPARE_SID                 (uint8)0x08U
#define FLS_BLANKCHECK_SID              (uint8)0x0AU
#define FLS_SETMODE_SID                 (uint8)0x09U
#define FLS_GETVERSIONINFO_SID          (uint8)0x10U

/************************************************************************************
 *                              Error Codes
 ************************************************************************************/
#define FLS_E_PARAM_CONFIG              (uint8)0x01U
#define FLS_E_PARAM_ADDRESS             (uint8)0x02U
#define FLS_E_PARAM_LENGTH              (uint8)0x03U
#define FLS_E_PARAM_DATA                (uint8)0x04U
#define FLS_E_UNINIT                    (uint8)0x05U
#define FLS_E_BUSY                      (uint8)0x06U
#define FLS_E_VERIFY_ERASED_FAILED      (uint8)0x07U
#define FLS_E_VERIFY_WRITE_FAILED       (uint8)0x08U
#define FLS_E_TIMEOUT                   (uint8)0x09U
#define FLS_E_PARAM_POINTER             (uint8)0x0AU
#define FLS_E_ERASE_FAILED              (uint8)0x0BU
#define FLS_E_WRITE_FAILED              (uint8)0x0CU
#define FLS_E_READ_FAILED               (uint8)0x0DU
#define FLS_E_COMPARE_FAILED            (uint8)0x0EU
#define FLS_E_UNEXPECTED_FLASH_ID       (uint8)0x0FU
#define FLS_E_SECTOR_PROTECTED          (uint8)0x10U

/************************************************************************************
 *                              Types Definitions
 ************************************************************************************/

/* Flash Driver State */
typedef enum
{
    MEMIF_UNINIT = 0,
    MEMIF_IDLE,
    MEMIF_BUSY
} MemIf_StatusType;

/* Flash Job Result */
typedef enum
{
    MEMIF_JOB_OK = 0,
    MEMIF_JOB_FAILED,
    MEMIF_JOB_PENDING,
    MEMIF_JOB_CANCELED
} MemIf_JobResultType;

/* Flash Programming Type */
typedef enum
{
    FLS_PROGRAM_TYPE_NORMAL = 0,
    FLS_PROGRAM_TYPE_FAST
} Fls_ProgramType;

/* Flash Access Mode */
typedef enum
{
    MEMIF_MODE_SLOW = 0,
    MEMIF_MODE_FAST
} MemIf_ModeType;

/* Flash Sector Size Type */
typedef enum
{
    FLS_SECTOR_SIZE_4KB = 0,
    FLS_SECTOR_SIZE_32KB,
    FLS_SECTOR_SIZE_64KB,
    FLS_SECTOR_SIZE_128KB
} Fls_SectorSizeType;

/* Flash Sector Info */
typedef struct
{
    uint32                SectorStartAddress;
    uint32                SectorSize;
    Fls_SectorSizeType    SectorSizeType;
    boolean               SectorProtected;
    uint8                 SectorBank;
} Fls_SectorInfoType;

/* Flash Configuration Type */
typedef struct
{
    uint32                BaseAddress;
    uint32                TotalSize;
    const Fls_SectorInfoType* SectorInfo;
    uint32                SectorCount;
    uint32                PageSize;
    uint32                ProgrammingUnit;
    uint32                MaxReadFastMode;
    uint32                MaxReadNormalMode;
    uint32                MaxWriteFastMode;
    uint32                MaxWriteNormalMode;
    MemIf_ModeType        DefaultMode;
    uint32                CallCycle;
    boolean               UseInterrupts;
    void (*JobEndNotification)(void);
    void (*JobErrorNotification)(void);
} Fls_ConfigType;

/* Flash Protection Type */
typedef enum
{
    FLS_PROTECTION_NONE = 0,
    FLS_PROTECTION_READ,
    FLS_PROTECTION_WRITE,
    FLS_PROTECTION_READ_WRITE
} Fls_ProtectionType;

/************************************************************************************
 *                          Function Prototypes
 ************************************************************************************/

/* Initialization and De-initialization */
void Fls_Init(const Fls_ConfigType* ConfigPtr);
void Fls_DeInit(void);

/* Flash Operations */
Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, 
                         Fls_LengthType Length);
Std_ReturnType Fls_Write(Fls_AddressType TargetAddress,
                         const uint8* SourceAddressPtr,
                         Fls_LengthType Length);
Std_ReturnType Fls_Read(Fls_AddressType SourceAddress,
                        uint8* TargetAddressPtr,
                        Fls_LengthType Length);

/* Control Functions */
void Fls_Cancel(void);
MemIf_StatusType Fls_GetStatus(void);
MemIf_JobResultType Fls_GetJobResult(void);
Std_ReturnType Fls_Compare(Fls_AddressType SourceAddress,
                           const uint8* TargetAddressPtr,
                           Fls_LengthType Length);
Std_ReturnType Fls_BlankCheck(Fls_AddressType TargetAddress,
                              Fls_LengthType Length);
void Fls_SetMode(MemIf_ModeType Mode);

/* Version Info */
#if (FLS_VERSION_INFO_API == STD_ON)
void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/* Main Function for asynchronous operations */
void Fls_MainFunction(void);

/* Flash Controller Interface */
Std_ReturnType Fls_UnlockControlRegisters(void);
Std_ReturnType Fls_LockControlRegisters(void);
Std_ReturnType Fls_ClearErrorFlags(void);
Std_ReturnType Fls_WaitForOperation(uint32 Timeout);

/* Protection Functions */
Std_ReturnType Fls_ConfigureReadProtection(Fls_ProtectionType Protection);
Std_ReturnType Fls_ConfigureWriteProtection(uint32 SectorMask, boolean Enable);

/* Utility Functions */
uint32 Fls_GetSectorIndex(Fls_AddressType Address);
Std_ReturnType Fls_VerifySectorErased(uint32 SectorIndex);
Std_ReturnType Fls_VerifyWrittenData(Fls_AddressType Address,
                                     const uint8* DataPtr,
                                     Fls_LengthType Length);

/************************************************************************************
 *                          Extern Declarations
 ************************************************************************************/
extern const Fls_ConfigType Fls_Config;

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H */

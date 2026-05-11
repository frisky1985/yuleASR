/*==================================================================================================
 *                                      FLASH DRIVER
 *==================================================================================================
 * FILENAME: Fls.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashDriver.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Flash Driver module
 *==================================================================================================
 */

#ifndef FLS_H
#define FLS_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Fls_Cfg.h"
#include "MemIf_Types.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define FLS_VENDOR_ID                   (100u)
#define FLS_MODULE_ID                   (92u)
#define FLS_INSTANCE_ID                 (0u)

#define FLS_AR_RELEASE_MAJOR_VERSION    (4u)
#define FLS_AR_RELEASE_MINOR_VERSION    (7u)
#define FLS_AR_RELEASE_REVISION_VERSION (0u)

#define FLS_SW_MAJOR_VERSION            (1u)
#define FLS_SW_MINOR_VERSION            (0u)
#define FLS_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((FLS_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (FLS_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of Fls.h and Std_Types.h are different"
    #endif
    
    #if ((FLS_AR_RELEASE_MAJOR_VERSION != MEMIF_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (FLS_AR_RELEASE_MINOR_VERSION != MEMIF_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of Fls.h and MemIf_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define FLS_SID_INIT                    (0x00u)
#define FLS_SID_ERASE                   (0x01u)
#define FLS_SID_WRITE                   (0x02u)
#define FLS_SID_READ                    (0x03u)
#define FLS_SID_COMPARE                 (0x04u)
#define FLS_SID_SETMODE                 (0x05u)
#define FLS_SID_CANCEL                  (0x06u)
#define FLS_SID_GETSTATUS               (0x07u)
#define FLS_SID_GETJOBRESULT            (0x08u)
#define FLS_SID_GETVERSIONINFO          (0x09u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define FLS_E_PARAM_CONFIG              (0x01u)
#define FLS_E_PARAM_ADDRESS             (0x02u)
#define FLS_E_PARAM_LENGTH              (0x03u)
#define FLS_E_PARAM_DATA                (0x04u)
#define FLS_E_UNINIT                    (0x05u)
#define FLS_E_BUSY                      (0x06u)
#define FLS_E_INVALID_LENGTH            (0x07u)
#define FLS_E_INVALID_ADDRESS           (0x08u)
#define FLS_E_PARAM_POINTER             (0x09u)
#define FLS_E_ALREADY_INITIALIZED       (0x0Au)

/* Runtime error codes */
#define FLS_E_ERASE_FAILED              (0x01u)
#define FLS_E_WRITE_FAILED              (0x02u)
#define FLS_E_READ_FAILED               (0x03u)
#define FLS_E_COMPARE_FAILED            (0x04u)
#define FLS_E_UNEXPECTED_FLASH_ID       (0x05u)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief Flash address type
 */
typedef uint32 Fls_AddressType;

/**
 * @brief Flash length type
 */
typedef uint32 Fls_LengthType;

/**
 * @brief Flash driver state type
 */
typedef enum {
    FLS_UNINIT = 0,         /* Driver not initialized */
    FLS_IDLE,               /* Driver initialized, no job running */
    FLS_BUSY                /* Job currently processing */
} Fls_StatusType;

/**
 * @brief Flash job result type
 */
typedef MemIf_JobResultType Fls_JobResultType;

/**
 * @brief Flash operation mode type
 */
typedef enum {
    FLS_MODE_NORMAL = 0,    /* Normal operation mode */
    FLS_MODE_FAST           /* Fast operation mode (if supported) */
} Fls_OpModeType;

/**
 * @brief Flash sector type
 */
typedef struct {
    Fls_AddressType sectorStartAddr;    /* Sector start address */
    Fls_LengthType sectorSize;          /* Sector size in bytes */
    uint32 sectorPageSize;              /* Page size for write operations */
    uint32 sectorUnlockMask;            /* Unlock mask (if applicable) */
    boolean sectorWritable;             /* Sector can be written */
    boolean sectorErasable;             /* Sector can be erased */
} Fls_SectorType;

/**
 * @brief Flash configuration type
 */
typedef struct {
    const Fls_SectorType* sectorList;   /* Pointer to sector configuration array */
    uint32 sectorCount;                 /* Number of configured sectors */
    uint32 defaultMode;                 /* Default operation mode */
    uint32 maxReadFastMode;             /* Max read bytes in fast mode */
    uint32 maxReadNormalMode;           /* Max read bytes in normal mode */
    uint32 maxWriteFastMode;            /* Max write bytes in fast mode */
    uint32 maxWriteNormalMode;          /* Max write bytes in normal mode */
    boolean jobEndNotificationEnabled;  /* Job end notification enabled */
    boolean jobErrorNotificationEnabled;/* Job error notification enabled */
} Fls_ConfigType;

/*==================================================================================================
 *                                    CALLBACK TYPES
 *==================================================================================================*/
/**
 * @brief Job end notification callback type
 */
typedef void (*Fls_JobEndNotificationType)(void);

/**
 * @brief Job error notification callback type
 */
typedef void (*Fls_JobErrorNotificationType)(void);

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define FLS_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fls_MemMap.h"

extern const Fls_ConfigType* Fls_ConfigPtr;
extern Fls_StatusType Fls_Status;

#define FLS_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Fls_MemMap.h"

/*==================================================================================================
 *                                     API DECLARATIONS
 *==================================================================================================*/
#define FLS_START_SEC_CODE
#include "Fls_MemMap.h"

/**
 * @brief Initializes the Flash driver
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_Fls_00153
 */
extern void Fls_Init(const Fls_ConfigType* ConfigPtr);

/**
 * @brief Erases a flash sector range
 * @param TargetAddress Target address in flash memory
 * @param Length Number of bytes to erase
 * @return E_OK: Job accepted, E_NOT_OK: Job rejected
 * @req SWS_Fls_00154
 */
extern Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length);

/**
 * @brief Writes data to flash memory
 * @param TargetAddress Target address in flash memory
 * @param SourceAddress Pointer to source data buffer
 * @param Length Number of bytes to write
 * @return E_OK: Job accepted, E_NOT_OK: Job rejected
 * @req SWS_Fls_00155
 */
extern Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddress, Fls_LengthType Length);

/**
 * @brief Reads data from flash memory
 * @param SourceAddress Source address in flash memory
 * @param TargetAddressPtr Pointer to target data buffer
 * @param Length Number of bytes to read
 * @return None
 * @req SWS_Fls_00156
 */
extern void Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length);

/**
 * @brief Compares flash memory with data buffer
 * @param SourceAddress Source address in flash memory
 * @param TargetAddressPtr Pointer to data buffer to compare
 * @param Length Number of bytes to compare
 * @return None
 * @req SWS_Fls_00157
 */
extern void Fls_Compare(Fls_AddressType SourceAddress, const uint8* TargetAddressPtr, Fls_LengthType Length);

/**
 * @brief Sets the flash driver's operation mode
 * @param Mode Desired operation mode (NORMAL or FAST)
 * @return None
 * @req SWS_Fls_00158
 */
extern void Fls_SetMode(MemIf_ModeType Mode);

/**
 * @brief Gets the current driver status
 * @return Fls_StatusType: Current driver status
 * @req SWS_Fls_00159
 */
extern Fls_StatusType Fls_GetStatus(void);

/**
 * @brief Gets the result of the last job
 * @return Fls_JobResultType: Result of last job
 * @req SWS_Fls_00160
 */
extern Fls_JobResultType Fls_GetJobResult(void);

/**
 * @brief Cancels an ongoing flash job
 * @return None
 * @req SWS_Fls_00161
 */
extern void Fls_Cancel(void);

/**
 * @brief Main function for processing flash jobs
 * @return None
 * @req SWS_Fls_00162
 */
extern void Fls_MainFunction(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Fls_00163
 */
#if (FLS_VERSION_INFO_API == STD_ON)
extern void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Reads flash memory synchronously (if supported)
 * @param SourceAddress Source address in flash memory
 * @param TargetAddressPtr Pointer to target data buffer
 * @param Length Number of bytes to read
 * @return E_OK: Success, E_NOT_OK: Failed
 * @req SWS_Fls_00300
 */
#if (FLS_USE_ISR == STD_OFF)
extern Std_ReturnType Fls_ReadSync(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length);
#endif

#define FLS_STOP_SEC_CODE
#include "Fls_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* FLS_H */

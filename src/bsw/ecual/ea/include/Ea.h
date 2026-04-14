/**
 * @file Ea.h
 * @brief EEPROM Abstraction module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: EEPROM Abstraction (EA)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Purpose: Abstract interface for EEPROM drivers
 */

#ifndef EA_H
#define EA_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Ea_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define EA_VENDOR_ID                    (0x01U) /* YuleTech Vendor ID */
#define EA_MODULE_ID                    (0x31U) /* EA Module ID */
#define EA_AR_RELEASE_MAJOR_VERSION     (0x04U)
#define EA_AR_RELEASE_MINOR_VERSION     (0x04U)
#define EA_AR_RELEASE_REVISION_VERSION  (0x00U)
#define EA_SW_MAJOR_VERSION             (0x01U)
#define EA_SW_MINOR_VERSION             (0x00U)
#define EA_SW_PATCH_VERSION             (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define EA_SID_INIT                     (0x00U)
#define EA_SID_SETMODE                  (0x01U)
#define EA_SID_READ                     (0x02U)
#define EA_SID_WRITE                    (0x03U)
#define EA_SID_CANCEL                   (0x04U)
#define EA_SID_GETSTATUS                (0x05U)
#define EA_SID_GETJOBRESULT             (0x06U)
#define EA_SID_INVALIDATEBLOCK          (0x07U)
#define EA_SID_ERASEIMMEDIATEBLOCK      (0x08U)
#define EA_SID_JOBENDNOTIFICATION       (0x09U)
#define EA_SID_JOBERRORNOTIFICATION     (0x0AU)
#define EA_SID_GETVERSIONINFO           (0x0BU)
#define EA_SID_GETERASECYCLECOUNT       (0x0CU)
#define EA_SID_MAINFUNCTION             (0x0DU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define EA_E_UNINIT                     (0x01U)
#define EA_E_INVALID_BLOCK_NO           (0x02U)
#define EA_E_INVALID_BLOCK_OFS          (0x03U)
#define EA_E_INVALID_DATA_PTR           (0x04U)
#define EA_E_INVALID_BLOCK_LEN          (0x05U)
#define EA_E_BUSY                       (0x06U)
#define EA_E_BUSY_INTERNAL              (0x07U)
#define EA_E_INVALID_CANCEL             (0x08U)
#define EA_E_INVALID_MODE               (0x09U)
#define EA_E_INVALID_CFG                (0x0AU)

/*==================================================================================================
*                                    EA STATUS TYPE
==================================================================================================*/
typedef enum {
    EA_IDLE = 0,
    EA_BUSY,
    EA_BUSY_INTERNAL
} Ea_StatusType;

/*==================================================================================================
*                                    EA JOB RESULT TYPE
==================================================================================================*/
typedef enum {
    EA_JOB_OK = 0,
    EA_JOB_FAILED,
    EA_JOB_PENDING,
    EA_JOB_CANCELLED,
    EA_BLOCK_INCONSISTENT,
    EA_BLOCK_INVALID
} Ea_JobResultType;

/*==================================================================================================
*                                    EA MODE TYPE
==================================================================================================*/
typedef enum {
    EA_MODE_SLOW = 0,
    EA_MODE_FAST
} Ea_ModeType;

/*==================================================================================================
*                                    EA BLOCK ID TYPE
==================================================================================================*/
typedef uint16 Ea_BlockIdType;

/*==================================================================================================
*                                    EA BLOCK CONFIG TYPE
==================================================================================================*/
typedef struct {
    Ea_BlockIdType BlockId;
    uint16 BlockSize;
    uint16 ImmediateData;
    uint32 NumberOfWriteCycles;
    boolean DeviceIndex;
    boolean BlockCrc;
} Ea_BlockConfigType;

/*==================================================================================================
*                                    EA CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Ea_BlockConfigType* BlockConfig;
    uint16 NumBlocks;
    uint32 EaSectorSize;
    uint32 EaNumberOfSectors;
    uint32 EaIndexSize;
    boolean EaNvmJobEndNotification;
    boolean EaNvmJobErrorNotification;
    boolean EaDevErrorDetect;
    boolean EaPollMode;
    boolean EaSetModeSupported;
    boolean EaVersionInfoApi;
} Ea_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define EA_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Ea_ConfigType Ea_Config;

#define EA_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define EA_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the EEPROM Abstraction module
 * @param ConfigPtr Pointer to configuration structure
 */
void Ea_Init(const Ea_ConfigType* ConfigPtr);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (SLOW/FAST)
 */
void Ea_SetMode(Ea_ModeType Mode);

/**
 * @brief Reads data from a block
 * @param BlockNumber Block number
 * @param BlockOffset Block offset
 * @param DataBufferPtr Data buffer pointer
 * @param Length Data length
 * @return Result of operation
 */
Std_ReturnType Ea_Read(Ea_BlockIdType BlockNumber,
                        uint16 BlockOffset,
                        uint8* DataBufferPtr,
                        uint16 Length);

/**
 * @brief Writes data to a block
 * @param BlockNumber Block number
 * @param DataBufferPtr Data buffer pointer
 * @return Result of operation
 */
Std_ReturnType Ea_Write(Ea_BlockIdType BlockNumber, const uint8* DataBufferPtr);

/**
 * @brief Cancels ongoing operation
 */
void Ea_Cancel(void);

/**
 * @brief Gets module status
 * @return Module status
 */
Ea_StatusType Ea_GetStatus(void);

/**
 * @brief Gets job result
 * @return Job result
 */
Ea_JobResultType Ea_GetJobResult(void);

/**
 * @brief Invalidates a block
 * @param BlockNumber Block number
 * @return Result of operation
 */
Std_ReturnType Ea_InvalidateBlock(Ea_BlockIdType BlockNumber);

/**
 * @brief Erases immediate block
 * @param BlockNumber Block number
 * @return Result of operation
 */
Std_ReturnType Ea_EraseImmediateBlock(Ea_BlockIdType BlockNumber);

/**
 * @brief Job end notification callback
 */
void Ea_JobEndNotification(void);

/**
 * @brief Job error notification callback
 */
void Ea_JobErrorNotification(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Ea_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Gets erase cycle count
 * @return Erase cycle count
 */
uint32 Ea_GetEraseCycleCount(void);

/**
 * @brief Main function for periodic processing
 */
void Ea_MainFunction(void);

#define EA_STOP_SEC_CODE
#include "MemMap.h"

#endif /* EA_H */

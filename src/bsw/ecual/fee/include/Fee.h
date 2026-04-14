/**
 * @file Fee.h
 * @brief Flash EEPROM Emulation module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Flash EEPROM Emulation (FEE)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Purpose: Emulate EEPROM functionality using Flash memory
 */

#ifndef FEE_H
#define FEE_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Fee_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define FEE_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define FEE_MODULE_ID                   (0x30U) /* FEE Module ID */
#define FEE_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define FEE_AR_RELEASE_MINOR_VERSION    (0x04U)
#define FEE_AR_RELEASE_REVISION_VERSION (0x00U)
#define FEE_SW_MAJOR_VERSION            (0x01U)
#define FEE_SW_MINOR_VERSION            (0x00U)
#define FEE_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define FEE_SID_INIT                    (0x00U)
#define FEE_SID_SETMODE                 (0x01U)
#define FEE_SID_READ                    (0x02U)
#define FEE_SID_WRITE                   (0x03U)
#define FEE_SID_CANCEL                  (0x04U)
#define FEE_SID_GETSTATUS               (0x05U)
#define FEE_SID_GETJOBRESULT            (0x06U)
#define FEE_SID_INVALIDATEBLOCK         (0x07U)
#define FEE_SID_ERASEIMMEDIATEBLOCK     (0x08U)
#define FEE_SID_JOBENDNOTIFICATION      (0x09U)
#define FEE_SID_JOBERRORNOTIFICATION    (0x0AU)
#define FEE_SID_GETVERSIONINFO          (0x0BU)
#define FEE_SID_GETCYCLECOUNT           (0x0CU)
#define FEE_SID_GETERASECYCLECOUNT      (0x0DU)
#define FEE_SID_GETWRITE_CYCLECOUNT     (0x0EU)
#define FEE_SID_GETVENDORINFO           (0x0FU)
#define FEE_SID_MAINFUNCTION            (0x12U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define FEE_E_UNINIT                    (0x01U)
#define FEE_E_INVALID_BLOCK_NO          (0x02U)
#define FEE_E_INVALID_BLOCK_OFS         (0x03U)
#define FEE_E_INVALID_DATA_PTR          (0x04U)
#define FEE_E_INVALID_BLOCK_LEN         (0x05U)
#define FEE_E_BUSY                      (0x06U)
#define FEE_E_BUSY_INTERNAL             (0x07U)
#define FEE_E_INVALID_CANCEL            (0x08U)
#define FEE_E_GC_BUSY                   (0x09U)
#define FEE_E_GC_READ                   (0x0AU)
#define FEE_E_GC_WRITE                  (0x0BU)
#define FEE_E_GC_ERASE                  (0x0CU)
#define FEE_E_INVALID_SUSPEND           (0x0DU)
#define FEE_E_INVALID_RESUME            (0x0EU)
#define FEE_E_INVALID_MODE              (0x0FU)
#define FEE_E_INVALID_CFG               (0x10U)
#define FEE_E_NOTIFICATION              (0x11U)
#define FEE_E_INVALID_POLLING           (0x12U)

/*==================================================================================================
*                                    FEE STATUS TYPE
==================================================================================================*/
typedef enum {
    FEE_IDLE = 0,
    FEE_BUSY,
    FEE_BUSY_INTERNAL,
    FEE_CANCELLED
} Fee_StatusType;

/*==================================================================================================
*                                    FEE JOB RESULT TYPE
==================================================================================================*/
typedef enum {
    FEE_JOB_OK = 0,
    FEE_JOB_FAILED,
    FEE_JOB_PENDING,
    FEE_JOB_CANCELLED,
    FEE_BLOCK_INCONSISTENT,
    FEE_BLOCK_INVALID
} Fee_JobResultType;

/*==================================================================================================
*                                    FEE MODE TYPE
==================================================================================================*/
typedef enum {
    FEE_MODE_SLOW = 0,
    FEE_MODE_FAST
} Fee_ModeType;

/*==================================================================================================
*                                    FEE BLOCK ID TYPE
==================================================================================================*/
typedef uint16 Fee_BlockIdType;

/*==================================================================================================
*                                    FEE BLOCK CONFIG TYPE
==================================================================================================*/
typedef struct {
    Fee_BlockIdType BlockId;
    uint16 BlockSize;
    uint16 ImmediateData;
    uint32 NumberOfWriteCycles;
    boolean BlockCrc;
    boolean BlockCrcType;
    boolean BlockCrcChecksum;
    boolean BlockCrcChecksumType;
} Fee_BlockConfigType;

/*==================================================================================================
*                                    FEE CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Fee_BlockConfigType* BlockConfig;
    uint16 NumBlocks;
    uint32 FeeSectorSize;
    uint32 FeeNumberOfSectors;
    uint32 FeeVirtualPageSize;
    uint32 FeeMaximumBlockingTime;
    uint32 FeeMaxGcCycles;
    uint32 FeeMaxGcErases;
    uint32 FeeMaxWriteCycles;
    boolean FeeNvmJobEndNotification;
    boolean FeeNvmJobErrorNotification;
    boolean FeeUseEraseSuspend;
    boolean FeePollMode;
    boolean FeeSetModeSupported;
    boolean FeeVersionInfoApi;
    boolean FeeDevErrorDetect;
} Fee_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define FEE_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Fee_ConfigType Fee_Config;

#define FEE_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define FEE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Flash EEPROM Emulation module
 * @param ConfigPtr Pointer to configuration structure
 */
void Fee_Init(const Fee_ConfigType* ConfigPtr);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (SLOW/FAST)
 */
void Fee_SetMode(Fee_ModeType Mode);

/**
 * @brief Reads data from a block
 * @param BlockNumber Block number
 * @param BlockOffset Block offset
 * @param DataBufferPtr Data buffer pointer
 * @param Length Data length
 * @return Result of operation
 */
Std_ReturnType Fee_Read(Fee_BlockIdType BlockNumber,
                         uint16 BlockOffset,
                         uint8* DataBufferPtr,
                         uint16 Length);

/**
 * @brief Writes data to a block
 * @param BlockNumber Block number
 * @param DataBufferPtr Data buffer pointer
 * @return Result of operation
 */
Std_ReturnType Fee_Write(Fee_BlockIdType BlockNumber, const uint8* DataBufferPtr);

/**
 * @brief Cancels ongoing operation
 */
void Fee_Cancel(void);

/**
 * @brief Gets module status
 * @return Module status
 */
Fee_StatusType Fee_GetStatus(void);

/**
 * @brief Gets job result
 * @return Job result
 */
Fee_JobResultType Fee_GetJobResult(void);

/**
 * @brief Invalidates a block
 * @param BlockNumber Block number
 * @return Result of operation
 */
Std_ReturnType Fee_InvalidateBlock(Fee_BlockIdType BlockNumber);

/**
 * @brief Erases immediate block
 * @param BlockNumber Block number
 * @return Result of operation
 */
Std_ReturnType Fee_EraseImmediateBlock(Fee_BlockIdType BlockNumber);

/**
 * @brief Job end notification callback
 */
void Fee_JobEndNotification(void);

/**
 * @brief Job error notification callback
 */
void Fee_JobErrorNotification(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Fee_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Gets cycle count
 * @return Cycle count
 */
uint32 Fee_GetCycleCount(void);

/**
 * @brief Gets erase cycle count
 * @return Erase cycle count
 */
uint32 Fee_GetEraseCycleCount(void);

/**
 * @brief Gets write cycle count
 * @return Write cycle count
 */
uint32 Fee_GetWriteCycleCount(void);

/**
 * @brief Main function for periodic processing
 */
void Fee_MainFunction(void);

#define FEE_STOP_SEC_CODE
#include "MemMap.h"

#endif /* FEE_H */

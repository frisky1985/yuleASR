/**
 * @file MemIf.h
 * @brief Memory Interface module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Memory Interface (MEMIF)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Purpose: Abstract interface for EEPROM and Flash drivers
 */

#ifndef MEMIF_H
#define MEMIF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "MemIf_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define MEMIF_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define MEMIF_MODULE_ID                 (0x16U) /* MEMIF Module ID */
#define MEMIF_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define MEMIF_AR_RELEASE_MINOR_VERSION  (0x04U)
#define MEMIF_AR_RELEASE_REVISION_VERSION (0x00U)
#define MEMIF_SW_MAJOR_VERSION          (0x01U)
#define MEMIF_SW_MINOR_VERSION          (0x00U)
#define MEMIF_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define MEMIF_SID_READ                  (0x01U)
#define MEMIF_SID_WRITE                 (0x02U)
#define MEMIF_SID_CANCEL                (0x03U)
#define MEMIF_SID_GETSTATUS             (0x04U)
#define MEMIF_SID_GETJOBRESULT          (0x05U)
#define MEMIF_SID_INVALIDATEBLOCK       (0x06U)
#define MEMIF_SID_ERASEIMMEDIATEBLOCK   (0x07U)
#define MEMIF_SID_GETVERSIONINFO        (0x08U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define MEMIF_E_PARAM_DEVICE            (0x01U)
#define MEMIF_E_PARAM_BLOCK             (0x02U)
#define MEMIF_E_PARAM_POINTER           (0x03U)
#define MEMIF_E_PARAM_LENGTH            (0x04U)
#define MEMIF_E_PARAM_DATA              (0x05U)
#define MEMIF_E_UNINIT                  (0x06U)

/*==================================================================================================
*                                    MEMIF STATUS TYPE
==================================================================================================*/
typedef enum {
    MEMIF_IDLE = 0,
    MEMIF_BUSY,
    MEMIF_BUSY_INTERNAL
} MemIf_StatusType;

/*==================================================================================================
*                                    MEMIF JOB RESULT TYPE
==================================================================================================*/
typedef enum {
    MEMIF_JOB_OK = 0,
    MEMIF_JOB_FAILED,
    MEMIF_JOB_PENDING,
    MEMIF_JOB_CANCELED,
    MEMIF_BLOCK_INCONSISTENT,
    MEMIF_BLOCK_INVALID
} MemIf_JobResultType;

/*==================================================================================================
*                                    MEMIF MODE TYPE
==================================================================================================*/
typedef enum {
    MEMIF_MODE_SLOW = 0,
    MEMIF_MODE_FAST
} MemIf_ModeType;

/*==================================================================================================
*                                    MEMIF DEVICE ID TYPE
==================================================================================================*/
typedef uint8 MemIf_DeviceIdType;

/*==================================================================================================
*                                    MEMIF BLOCK ID TYPE
==================================================================================================*/
typedef uint16 MemIf_BlockIdType;

/*==================================================================================================
*                                    MEMIF ADDRESS TYPE
==================================================================================================*/
typedef uint32 MemIf_AddressType;

/*==================================================================================================
*                                    MEMIF LENGTH TYPE
==================================================================================================*/
typedef uint32 MemIf_LengthType;

/*==================================================================================================
*                                    MEMIF DEVICE CONFIG TYPE
==================================================================================================*/
typedef struct {
    MemIf_DeviceIdType DeviceId;
    uint8 UnderlyingDriver;         /* 0=EEP, 1=FEE, 2=EA */
    uint8 UnderlyingDeviceId;
    uint32 TotalSize;
    uint32 BlockSize;
    MemIf_ModeType DefaultMode;
} MemIf_DeviceConfigType;

/*==================================================================================================
*                                    MEMIF CONFIG TYPE
==================================================================================================*/
typedef struct {
    const MemIf_DeviceConfigType* Devices;
    uint8 NumDevices;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} MemIf_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define MEMIF_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const MemIf_ConfigType MemIf_Config;

#define MEMIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define MEMIF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Memory Interface
 * @param ConfigPtr Pointer to configuration structure
 */
void MemIf_Init(const MemIf_ConfigType* ConfigPtr);

/**
 * @brief Reads data from memory device
 * @param DeviceIndex Device index
 * @param BlockNumber Block number
 * @param BlockOffset Block offset
 * @param DataBufferPtr Data buffer pointer
 * @param Length Data length
 * @return Result of operation
 */
Std_ReturnType MemIf_Read(MemIf_DeviceIdType DeviceIndex,
                           MemIf_BlockIdType BlockNumber,
                           uint16 BlockOffset,
                           uint8* DataBufferPtr,
                           uint16 Length);

/**
 * @brief Writes data to memory device
 * @param DeviceIndex Device index
 * @param BlockNumber Block number
 * @param DataBufferPtr Data buffer pointer
 * @return Result of operation
 */
Std_ReturnType MemIf_Write(MemIf_DeviceIdType DeviceIndex,
                            MemIf_BlockIdType BlockNumber,
                            const uint8* DataBufferPtr);

/**
 * @brief Cancels ongoing operation
 * @param DeviceIndex Device index
 */
void MemIf_Cancel(MemIf_DeviceIdType DeviceIndex);

/**
 * @brief Gets device status
 * @param DeviceIndex Device index
 * @return Device status
 */
MemIf_StatusType MemIf_GetStatus(MemIf_DeviceIdType DeviceIndex);

/**
 * @brief Gets job result
 * @param DeviceIndex Device index
 * @return Job result
 */
MemIf_JobResultType MemIf_GetJobResult(MemIf_DeviceIdType DeviceIndex);

/**
 * @brief Invalidates a block
 * @param DeviceIndex Device index
 * @param BlockNumber Block number
 * @return Result of operation
 */
Std_ReturnType MemIf_InvalidateBlock(MemIf_DeviceIdType DeviceIndex,
                                      MemIf_BlockIdType BlockNumber);

/**
 * @brief Erases immediate block
 * @param DeviceIndex Device index
 * @param BlockNumber Block number
 * @return Result of operation
 */
Std_ReturnType MemIf_EraseImmediateBlock(MemIf_DeviceIdType DeviceIndex,
                                          MemIf_BlockIdType BlockNumber);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void MemIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets device mode
 * @param DeviceIndex Device index
 * @param Mode Mode to set
 */
void MemIf_SetMode(MemIf_DeviceIdType DeviceIndex, MemIf_ModeType Mode);

#define MEMIF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* MEMIF_H */

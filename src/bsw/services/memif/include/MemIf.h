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

/*******************************************************************************
 * @file        MemIf.h
 * @brief       Memory Interface (MemIf) public header
 * @module      SERVICES
 * @author      AutoSAR Generator
 * @version     1.0.0
 * @date        2026-05-06
 * @copyright   上海予乐电子科技有限公司
 * 
 * @description
 * MemIf provides uniform access to memory hardware devices (Flash/EEPROM).
 * It abstracts multiple underlying memory drivers (Fee, Ea) and provides
 * device index based routing for read/write/erase operations.
 ******************************************************************************/

#ifndef MEMIF_H
#define MEMIF_H

/*******************************************************************************
 *                              Includes
 ******************************************************************************/
#include "Std_Types.h"
#include "MemIf_Cfg.h"

/*******************************************************************************
 *                              Version Info
 ******************************************************************************/
#define MEMIF_VENDOR_ID                     (0x01u)
#define MEMIF_MODULE_ID                     (0x16u)
#define MEMIF_AR_MAJOR_VERSION              (4u)
#define MEMIF_AR_MINOR_VERSION              (4u)
#define MEMIF_AR_PATCH_VERSION              (0u)
#define MEMIF_SW_MAJOR_VERSION              (1u)
#define MEMIF_SW_MINOR_VERSION              (0u)
#define MEMIF_SW_PATCH_VERSION              (0u)

/*******************************************************************************
 *                              Type Definitions
 ******************************************************************************/

/** @brief MemIf device index type */
#ifndef MEMIF_DEVICEMODE_TYPE
    #define MEMIF_DEVICEMODE_TYPE       uint8
#endif

typedef uint8 MemIf_DeviceIndexType;

/** @brief MemIf mode type */
typedef enum {
    MEMIF_MODE_UNINIT = 0,      /**< Uninitialized state */
    MEMIF_MODE_SLOW,            /**< Slow read/write mode */
    MEMIF_MODE_FAST             /**< Fast read/write mode */
} MemIf_ModeType;

/** @brief MemIf job result type */
typedef enum {
    MEMIF_JOB_OK = 0,           /**< Job completed successfully */
    MEMIF_JOB_PENDING,          /**< Job is still pending */
    MEMIF_JOB_CANCELED,         /**< Job was canceled */
    MEMIF_JOB_FAILED            /**< Job failed */
} MemIf_JobResultType;

/** @brief MemIf status type */
#undef MEMIF_UNINIT
#undef MEMIF_IDLE
#undef MEMIF_BUSY
typedef enum {
    MEMIF_UNINIT = 0,           /**< Module not initialized */
    MEMIF_IDLE,                 /**< Module initialized and idle */
    MEMIF_BUSY                  /**< Module busy with operation */
} MemIf_StatusType;
#ifndef MEMIF_UNINIT
#define MEMIF_UNINIT                        (0x00U)
#endif
#ifndef MEMIF_IDLE
#define MEMIF_IDLE                          (0x01U)
#endif
#ifndef MEMIF_BUSY
#define MEMIF_BUSY                          (0x02U)
#endif

/** @brief Device abstraction structure */
typedef struct {
    uint32 DeviceIndex;
    uint32 DeviceType;          /**< 0=Fee, 1=Ea */
    void (*Read)(void);
    void (*Write)(void);
    void (*Erase)(void);
    void (*Cancel)(void);
    MemIf_StatusType (*GetStatus)(void);
    MemIf_JobResultType (*GetJobResult)(void);
} MemIf_DeviceAbstractionType;

/** @brief MemIf device state type (internal tracking) */
typedef struct {
    MemIf_StatusType status;
    MemIf_JobResultType jobResult;
    boolean isInitialized;
} MemIf_DeviceStateType;

/** @brief MemIf device configuration type */
typedef struct {
    MemIf_DeviceIndexType DeviceId;
    uint8 DeviceType;      /* 0=FEE, 1=EA */
    uint32 BlockSize;
    uint32 NumberOfBlocks;
    uint32 numBlocks;
} MemIf_DeviceConfigType;

/** @brief MemIf global configuration type */
typedef struct {
    const MemIf_DeviceConfigType* Devices;
    uint8 NumDevices;
} MemIf_ConfigType;

/*******************************************************************************
 *                         Default Configuration
 ******************************************************************************/

#ifndef MEMIF_NUMBER_OF_DEVICES
#define MEMIF_NUMBER_OF_DEVICES             (1U)
#endif

#ifndef MEMIF_TOTAL_NUM_DEVICES
#define MEMIF_TOTAL_NUM_DEVICES             (1U)
#endif

#ifndef MEMIF_INSTANCE_ID
#define MEMIF_INSTANCE_ID                   (0U)
#endif

#ifndef MEMIF_SID_INIT
#define MEMIF_SID_INIT                      (0x01U)
#endif

#ifndef MEMIF_UNINIT
#define MEMIF_UNINIT                        (0x00U)
#endif

#ifndef MEMIF_BUSY_INTERNAL
#define MEMIF_BUSY_INTERNAL                 (0x02U)
#endif

#ifndef MEMIF_E_UNINIT
#define MEMIF_E_UNINIT                      (0x03U)
#endif

#ifndef MEMIF_SID_READ
#define MEMIF_SID_READ                      (0x03U)
#endif

#ifndef MEMIF_SID_WRITE
#define MEMIF_SID_WRITE                     (0x04U)
#endif

#ifndef MEMIF_MODE_UNINIT
#define MEMIF_MODE_UNINIT                   (0x00U)
#endif

#ifndef MEMIF_E_PARAM_DEVICE_INDEX
#define MEMIF_E_PARAM_DEVICE_INDEX          (0x04U)
#endif

#ifndef MEMIF_E_ALREADY_INITIALIZED
#define MEMIF_E_ALREADY_INITIALIZED          (0x05U)
#endif

#ifndef MEMIF_DEVICE_INDEX_FEE
#define MEMIF_DEVICE_INDEX_FEE              (0x00U)
#endif

#ifndef MEMIF_DEVICE_INDEX_EA
#define MEMIF_DEVICE_INDEX_EA               (0x01U)
#endif

#ifndef MEMIF_SID_CANCEL
#define MEMIF_SID_CANCEL                    (0x05U)
#endif

#ifndef MEMIF_SID_DEINIT
#define MEMIF_SID_DEINIT                    (0x02U)
#endif

#ifndef MEMIF_SID_MAINFUNCTION
#define MEMIF_SID_MAINFUNCTION              (0x06U)
#endif

#ifndef MEMIF_SID_GETSTATUS
#define MEMIF_SID_GETSTATUS                 (0x07U)
#endif

#ifndef MEMIF_SID_GETJOBRESULT
#define MEMIF_SID_GETJOBRESULT              (0x08U)
#endif

#ifndef MEMIF_SID_ERASEBLOCK
#define MEMIF_SID_ERASEBLOCK                (0x09U)
#endif

#ifndef MEMIF_SID_INVALIDATEBLOCK
#define MEMIF_SID_INVALIDATEBLOCK           (0x0AU)
#endif

/*******************************************************************************
 *                         External Declarations
 ******************************************************************************/

/* Device abstraction table from Lcfg */
extern const MemIf_DeviceAbstractionType MemIf_Devices[MEMIF_NUMBER_OF_DEVICES];

/*******************************************************************************
 *                          Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize MemIf module
 * @return None
 */
extern void MemIf_Init(const MemIf_ConfigType* ConfigPtr);

/**
 * @brief Cancel ongoing operation for a device
 * @param DeviceIndex Index of the device
 * @return None
 */
extern void MemIf_Cancel(uint8 DeviceIndex);

/**
 * @brief Read data from memory device
 * @param DeviceIndex Index of the device
 * @param BlockNumber Block number to read
 * @param BlockOffset Offset within the block
 * @param Length Number of bytes to read
 * @param DataPtr Pointer to data buffer
 * @return Std_ReturnType E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType MemIf_Read(uint8 DeviceIndex, 
                                  uint16 BlockNumber, 
                                  uint16 BlockOffset, 
                                  uint8* DataPtr, 
                                  uint16 Length);

/**
 * @brief Write data to memory device
 * @param DeviceIndex Index of the device
 * @param BlockNumber Block number to write
 * @param DataPtr Pointer to data buffer
 * @return Std_ReturnType E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType MemIf_Write(uint8 DeviceIndex, 
                                   uint16 BlockNumber, 
                                   const uint8* DataPtr);

/**
 * @brief Invalidate a block in memory device
 * @param DeviceIndex Index of the device
 * @param BlockNumber Block number to invalidate
 * @return Std_ReturnType E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, 
                                             uint16 BlockNumber);

/**
 * @brief Erase immediate block in memory device
 * @param DeviceIndex Index of the device
 * @param BlockNumber Block number to erase
 * @return Std_ReturnType E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType MemIf_EraseImmediateBlock(uint8 DeviceIndex, 
                                                 uint16 BlockNumber);

/**
 * @brief Get the status of a device
 * @param DeviceIndex Index of the device
 * @return MemIf_StatusType Current status of the device
 */
extern MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex);

/**
 * @brief Get the job result of a device
 * @param DeviceIndex Index of the device
 * @return MemIf_JobResultType Job result
 */
extern MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex);

/**
 * @brief Set the mode for a device
 * @param DeviceIndex Index of the device
 * @param Mode Mode to set (MEMIF_MODE_SLOW or MEMIF_MODE_FAST)
 * @return None
 */
extern void MemIf_SetMode(uint8 DeviceIndex, MemIf_ModeType Mode);

/*******************************************************************************
 *                          Inline Functions (Optional)
 ******************************************************************************/

#if (MEMIF_VERSION_INFO_API == STD_ON)
/**
 * @brief Get version information
 * @param VersionInfo Pointer to version info structure
 * @return None
 */
#define MemIf_GetVersionInfo(VersionInfoPtr) \
    do { \
        if ((VersionInfoPtr) != NULL_PTR) { \
            (VersionInfoPtr)->vendorID = MEMIF_VENDOR_ID; \
            (VersionInfoPtr)->moduleID = MEMIF_MODULE_ID; \
            (VersionInfoPtr)->sw_major_version = MEMIF_SW_MAJOR_VERSION; \
            (VersionInfoPtr)->sw_minor_version = MEMIF_SW_MINOR_VERSION; \
            (VersionInfoPtr)->sw_patch_version = MEMIF_SW_PATCH_VERSION; \
        } \
    } while(0)
#endif

/*******************************************************************************
 *                          Development Error Tracing
 ******************************************************************************/
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    #define MEMIF_E_PARAM_DEVICE    (0x01u)  /**< Invalid device index */
    #define MEMIF_E_PARAM_POINTER   (0x02u)  /**< NULL pointer */
    #define MEMIF_E_PARAM_BLOCK     (0x03u)  /**< Invalid block number */
    #define MEMIF_E_NOT_INITIALIZED (0x04u)  /**< Module not initialized */
    #define MEMIF_E_PARAM_MODE      (0x05u)  /**< Invalid mode */
    
    /** @brief Report development error */
    void MemIf_ReportError(uint8 ApiId, uint8 ErrorId);
#endif

/*******************************************************************************
 *                          RTE Interface (Optional)
 ******************************************************************************/

#ifdef MEMIF_USE_RTE
    #include "Rte_MemIf.h"
#endif

#endif /* MEMIF_H */

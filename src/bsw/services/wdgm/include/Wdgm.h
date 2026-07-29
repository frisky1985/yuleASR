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

/** @file Wdgm.h
 * @brief Watchdog Manager header file
 * 
 * AUTOSAR R22-11 compliant Wdgm module
 * Service layer - Watchdog Management and Supervision
 */

#ifndef WDGM_H
#define WDGM_H

/*============================================================================
 *  AUTOSAR VERSION INFORMATION
 *===========================================================================*/
#define WDGM_AR_RELEASE_MAJOR_VERSION       4
#define WDGM_AR_RELEASE_MINOR_VERSION       7
#define WDGM_AR_RELEASE_REVISION_VERSION    0

#define WDGM_SW_MAJOR_VERSION               1
#define WDGM_SW_MINOR_VERSION               0
#define WDGM_SW_PATCH_VERSION               0

/*============================================================================
 *  INCLUDES
 *===========================================================================*/
#include "Std_Types.h"
#include "WdgIf.h"

/*============================================================================
 *  VERSION CHECK
 *===========================================================================*/
#if (STD_TYPES_AR_RELEASE_MAJOR_VERSION != WDGM_AR_RELEASE_MAJOR_VERSION)
    #error "Wdgm: AR major version mismatch with Std_Types.h"
#endif

/*============================================================================
 *  MODULE INFORMATION
 *===========================================================================*/
#define WDGM_MODULE_ID                      13  /* AUTOSAR module ID */
#define WDGM_VENDOR_ID                      0x5C  /* Vendor ID */

/*============================================================================
 *  DEVELOPMENT ERROR CODES
 *===========================================================================*/
#define WDGM_E_NOT_INITIALIZED              0x10  /* Module not initialized */
#define WDGM_E_PARAM_CONFIG                 0x11  /* Invalid configuration */
#define WDGM_E_PARAM_POINTER                0x12  /* Invalid pointer */
#define WDGM_E_CPID                         0x13  /* Invalid checkpoint ID */
#define WDGM_E_PARAM_SEID                   0x14  /* Invalid supervision entity ID */
#define WDGM_E_NO_DEINIT                    0x15  /* Deinit not allowed */

/*============================================================================
 *  RUNTIME ERROR CODES
 *===========================================================================*/
#define WDGM_E_SET_MODE                     0x20  /* Set mode failed */
#define WDGM_E_DATA_CORRUPT                 0x21  /* Data corruption detected */
#define WDGM_E_MEMORY_FAILURE               0x22  /* Memory failure detected */

/*============================================================================
 *  API SERVICE IDs
 *===========================================================================*/
#define WDGM_SID_INIT                       0x00
#define WDGM_SID_DEINIT                     0x01
#define WDGM_SID_GETVERSIONINFO             0x02
#define WDGM_SID_SETMODE                    0x03
#define WDGM_SID_GETMODE                    0x0B
#define WDGM_SID_CHECKPOINTREACHED          0x0E
#define WDGM_SID_GETLOCALSTATUS             0x0C
#define WDGM_SID_GETGLOBALSTATUS            0x0D
#define WDGM_SID_PERFORMRESET               0x0F
#define WDGM_SID_GETFIRSTEXPIREDSEID        0x10
#define WDGM_SID_MAINFUNCTION               0x08

/*============================================================================
 *  DATA TYPES
 *===========================================================================*/

/** @brief Watchdog Manager supervision status */
typedef enum {
    WDGM_LOCAL_STATUS_DEACTIVATED = 0,   /* Supervision deactivated */
    WDGM_LOCAL_STATUS_OK,                 /* Supervision OK */
    WDGM_LOCAL_STATUS_FAILED,             /* Supervision failed */
    WDGM_LOCAL_STATUS_EXPIRED             /* Supervision expired */
} Wdgm_LocalStatusType;

/** @brief Watchdog Manager global status */
typedef enum {
    WDGM_GLOBAL_STATUS_DEACTIVATED = 0,  /* Global supervision deactivated */
    WDGM_GLOBAL_STATUS_OK,                /* Global supervision OK */
    WDGM_GLOBAL_STATUS_FAILED,            /* Global supervision failed */
    WDGM_GLOBAL_STATUS_EXPIRED            /* Global supervision expired */
} Wdgm_GlobalStatusType;

/** @brief Checkpoint ID type */
typedef uint16 Wdgm_CheckpointIdType;

/** @brief Supervision entity ID type */
typedef uint16 Wdgm_SupervisedEntityIdType;

/** @brief Alive counter type */
typedef uint16 Wdgm_AliveCounterType;

/** @brief Deadline time type (in ticks) */
typedef uint32 Wdgm_DeadlineTimeType;

/** @brief Supervision entity configuration */
typedef struct {
    Wdgm_SupervisedEntityIdType SEId;
    boolean IsActive;
    Wdgm_CheckpointIdType CheckpointCount;
} Wdgm_SupervisedEntityConfigType;

/** @brief Checkpoint configuration */
typedef struct {
    Wdgm_CheckpointIdType CheckpointId;
    Wdgm_SupervisedEntityIdType SEId;
    uint8 SupervisionType;  /* Bitmask: Alive/Deadline/Logical */
} Wdgm_CheckpointConfigType;

/** @brief Supervision entity runtime data */
typedef struct {
    Wdgm_SupervisedEntityIdType SEId;
    Wdgm_LocalStatusType LocalStatus;
    Wdgm_AliveCounterType AliveCounter;
    Wdgm_DeadlineTimeType DeadlineStartTime;
    boolean IsInitialized;
} Wdgm_SupervisedEntityType;

/** @brief Module configuration */
typedef struct {
    const Wdgm_SupervisedEntityConfigType* SEConfigs;
    const WdgIf_ModeType* InitialMode;
    uint16 SupervisionCycleMs;
    uint8 ExpirationTolerance;
} Wdgm_ConfigType;

/*============================================================================
 *  FUNCTION PROTOTYPES
 *===========================================================================*/

/**
 * @brief Initialize Watchdog Manager
 * @param ConfigPtr Pointer to configuration
 */
void Wdgm_Init(const Wdgm_ConfigType* ConfigPtr);

/**
 * @brief Deinitialize Watchdog Manager
 */
void Wdgm_DeInit(void);

/**
 * @brief Get version information
 * @param VersionInfo Pointer to version info
 */
#if (WDGM_VERSION_INFO_API == STD_ON)
void Wdgm_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/**
 * @brief Set Watchdog Manager mode
 * @param Mode Mode to set (WDGIF_OFF_MODE, WDGIF_SLOW_MODE, WDGIF_FAST_MODE)
 * @return E_OK if successful
 */
Std_ReturnType Wdgm_SetMode(WdgIf_ModeType Mode);

/**
 * @brief Get current Watchdog Manager mode
 * @return Current mode
 */
WdgIf_ModeType Wdgm_GetMode(void);

/**
 * @brief Report checkpoint reached
 * @param SEID Supervision Entity ID
 * @param CheckpointID Checkpoint ID
 * @return E_OK if successful
 */
Std_ReturnType Wdgm_CheckpointReached(Wdgm_SupervisedEntityIdType SEID,
                                       Wdgm_CheckpointIdType CheckpointID);

/**
 * @brief Get local supervision status
 * @param SEID Supervision Entity ID
 * @return Local status
 */
Wdgm_LocalStatusType Wdgm_GetLocalStatus(Wdgm_SupervisedEntityIdType SEID);

/**
 * @brief Get global supervision status
 * @return Global status
 */
Wdgm_GlobalStatusType Wdgm_GetGlobalStatus(void);

/**
 * @brief Perform reset
 */
void Wdgm_PerformReset(void);

/**
 * @brief Get first expired SEID
 * @return SEID of first expired supervision entity
 */
Wdgm_SupervisedEntityIdType Wdgm_GetFirstExpiredSEID(void);

/**
 * @brief Main function - cyclic supervision
 */
void Wdgm_MainFunction(void);

#endif /* WDGM_H */

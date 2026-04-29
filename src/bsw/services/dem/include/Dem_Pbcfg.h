/**
 * @file Dem_Pbcfg.h
 * @brief Diagnostic Event Manager - Post-Build Configuration Header
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Post-build configuration header for Dem module
 * Contains post-build configurable parameters and NvM integration
 */

#ifndef DEM_PBCFG_H
#define DEM_PBCFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Dem_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DEM_PBCFG_VENDOR_ID                   (0x01U)
#define DEM_PBCFG_MODULE_ID                   (0x54U)
#define DEM_PBCFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DEM_PBCFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DEM_PBCFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define DEM_PBCFG_SW_MAJOR_VERSION            (0x01U)
#define DEM_PBCFG_SW_MINOR_VERSION            (0x01U)
#define DEM_PBCFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    POST-BUILD CONFIGURATION
==================================================================================================*/
/**
 * @brief Post-build configuration type
 * Contains data that can be configured after compilation
 */
typedef struct {
    uint16 NvMBlockIdEventStatus;
    uint16 NvMBlockIdDTCData;
    uint16 NvMBlockIdFreezeFrame;
    uint16 NvMBlockIdExtendedData;
    boolean NvMStorageEnabled;
    uint8 NvMImmediateStorageThreshold;
} Dem_PostBuildConfigType;

/*==================================================================================================
*                                    NvM INTEGRATION
==================================================================================================*/
/* NvM block IDs for DEM persistent storage */
#define DEM_NVM_BLOCK_ID_EVENT_STATUS   (1U)
#define DEM_NVM_BLOCK_ID_DTC_DATA       (2U)
#define DEM_NVM_BLOCK_ID_FREEZE_FRAME   (3U)
#define DEM_NVM_BLOCK_ID_EXTENDED_DATA  (4U)
#define DEM_NVM_BLOCK_ID_OBD_DATA       (5U)

/*==================================================================================================
*                                    POST-BUILD CONFIGURATION DATA
==================================================================================================*/
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* Default post-build configuration */
extern CONST(Dem_PostBuildConfigType, DEM_CONST) Dem_PostBuildConfig;

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Get post-build configuration
 * @return Pointer to post-build configuration
 */
extern const Dem_PostBuildConfigType* Dem_GetPostBuildConfig(void);

/**
 * @brief Initialize with post-build configuration
 * @param PostBuildConfigPtr Pointer to post-build configuration
 * @return Result of operation
 */
extern Std_ReturnType Dem_InitWithPostBuildConfig(const Dem_PostBuildConfigType* PostBuildConfigPtr);

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DEM_PBCFG_H */

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

/**
 * @file Dem_Pbcfg.c
 * @brief Diagnostic Event Manager - Post-Build Configuration
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Post-build configuration implementation
 */

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Dem_Pbcfg.h"
#include "MemMap.h"

/*==================================================================================================
*                                    POST-BUILD CONFIGURATION DATA
==================================================================================================*/
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Default post-build configuration
 * 
 * This configuration can be modified at runtime or linked from
 * a different object file for post-build configuration support.
 */
CONST(Dem_PostBuildConfigType, DEM_CONST) Dem_PostBuildConfig = {
    DEM_NVM_BLOCK_ID_EVENT_STATUS,   /* NvMBlockIdEventStatus */
    DEM_NVM_BLOCK_ID_DTC_DATA,       /* NvMBlockIdDTCData */
    DEM_NVM_BLOCK_ID_FREEZE_FRAME,   /* NvMBlockIdFreezeFrame */
    DEM_NVM_BLOCK_ID_EXTENDED_DATA,  /* NvMBlockIdExtendedData */
    TRUE,                            /* NvMStorageEnabled */
    2U                               /* NvMImmediateStorageThreshold - store after 2 occurrences */
};

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Get post-build configuration
 * @return Pointer to post-build configuration
 */
const Dem_PostBuildConfigType* Dem_GetPostBuildConfig(void)
{
    return &Dem_PostBuildConfig;
}

/**
 * @brief Initialize with post-build configuration
 * @param PostBuildConfigPtr Pointer to post-build configuration
 * @return Result of operation
 */
Std_ReturnType Dem_InitWithPostBuildConfig(const Dem_PostBuildConfigType* PostBuildConfigPtr)
{
    if (PostBuildConfigPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Validate configuration parameters */
    if (PostBuildConfigPtr->NvMImmediateStorageThreshold == 0U)
    {
        return E_NOT_OK;
    }

    /* Configuration is valid - in a full implementation,
     * this would store the pointer or copy the configuration */
    (void)PostBuildConfigPtr;

    return E_OK;
}

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

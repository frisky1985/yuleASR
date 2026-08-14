#ifndef NVM_REDUNDANT_H
#define NVM_REDUNDANT_H

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file NvM_Redundant.h
 * @brief NVM Redundant Storage public API
 * @details Redundant (primary/mirror) storage for critical NvM blocks with
 *          CRC validation and automatic recovery. Enabled by
 *          NVM_REDUNDANT_STORAGE_ENABLED in NvM_Cfg.h.
 */

#include "Std_Types.h"
#include "NvM.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write block to both primary and mirror instances with CRC
 * @param BlockId Redundant block group index (0 .. NVM_NUM_REDUNDANT_BLOCKS-1)
 * @param SrcPtr  Source data buffer (size = NvBlockLength of primary descriptor)
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType NvM_RedundantWrite(NvM_BlockIdType BlockId, const uint8* SrcPtr);

/**
 * @brief Read block with automatic recovery from the healthy instance
 * @param BlockId Redundant block group index
 * @param DestPtr Destination buffer
 * @return E_OK on success (single-instance corruption is auto-recovered)
 */
Std_ReturnType NvM_RedundantRead(NvM_BlockIdType BlockId, uint8* DestPtr);

/**
 * @brief Check whether primary and mirror instances hold identical data
 * @param BlockId Redundant block group index
 * @return E_OK if consistent, E_NOT_OK if inconsistent or unreadable
 */
Std_ReturnType NvM_RedundantCheckConsistency(NvM_BlockIdType BlockId);

/**
 * @brief Repair an inconsistent block from the healthy instance
 * @param BlockId Redundant block group index
 * @return E_OK if repaired, E_NOT_OK if neither instance is valid
 */
Std_ReturnType NvM_RedundantRepair(NvM_BlockIdType BlockId);

#ifdef __cplusplus
}
#endif

#endif /* NVM_REDUNDANT_H */

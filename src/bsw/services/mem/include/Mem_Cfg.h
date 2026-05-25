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

/*==================================================================================================
 *                                      MEMORY SERVICE (Mem)
 *==================================================================================================
 * FILENAME: Mem_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Configuration header file for Memory Service module
 *==================================================================================================
 */

#ifndef MEM_CFG_H
#define MEM_CFG_H

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define MEM_CFG_AR_RELEASE_MAJOR_VERSION    (4u)
#define MEM_CFG_AR_RELEASE_MINOR_VERSION    (7u)
#define MEM_CFG_AR_RELEASE_REVISION_VERSION (0u)

#define MEM_CFG_SW_MAJOR_VERSION            (1u)
#define MEM_CFG_SW_MINOR_VERSION            (0u)
#define MEM_CFG_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    PRE-COMPILE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 */
#define MEM_DEV_ERROR_DETECT                (STD_ON)

/**
 * @brief Version info API enable/disable
 */
#define MEM_VERSION_INFO_API                (STD_ON)

/**
 * @brief Main function period in milliseconds
 */
#define MEM_MAIN_FUNCTION_PERIOD_MS         (10u)

/*==================================================================================================
 *                                    MEMORY POOL CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Number of memory pools
 */
#define MEM_NUM_POOLS                       (3u)

/**
 * @brief Default pool index
 */
#define MEM_DEFAULT_POOL_INDEX              (0u)

/**
 * @brief Fast pool size (4KB) - for small allocations
 */
#define MEM_FAST_POOL_SIZE                  (4096u)

/**
 * @brief Standard pool size (16KB) - for medium allocations
 */
#define MEM_STANDARD_POOL_SIZE              (16384u)

/**
 * @brief Large pool size (64KB) - for large allocations
 */
#define MEM_LARGE_POOL_SIZE                 (65536u)

/**
 * @brief Minimum block size (16 bytes)
 */
#define MEM_MIN_BLOCK_SIZE                  (16u)

/**
 * @brief Maximum block size in fast pool
 */
#define MEM_FAST_POOL_MAX_BLOCK             (256u)

/**
 * @brief Maximum block size in standard pool
 */
#define MEM_STANDARD_POOL_MAX_BLOCK         (2048u)

/**
 * @brief Maximum block size in large pool
 */
#define MEM_LARGE_POOL_MAX_BLOCK            (65536u)

/**
 * @brief Default alignment
 */
#define MEM_DEFAULT_ALIGNMENT               (4u)

/**
 * @brief Enable memory integrity checking
 */
#define MEM_ENABLE_CHECKSUM                 (STD_ON)

/**
 * @brief Enable memory monitoring
 */
#define MEM_ENABLE_MONITORING               (STD_ON)

/**
 * @brief Defragmentation threshold (%)
 */
#define MEM_DEFRAG_THRESHOLD                (30u)

/**
 * @brief Maximum number of allocations
 */
#define MEM_MAX_ALLOCATIONS                 (256u)

/**
 * @brief Memory corruption detection magic number
 */
#define MEM_MAGIC_NUMBER_CFG                (0x4D454D21u)

#endif /* MEM_CFG_H */

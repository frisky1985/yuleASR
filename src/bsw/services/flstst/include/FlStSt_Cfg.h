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
 * @file FlStSt_Cfg.h
 * @brief Flash Test Pre-Compile Configuration
 * @version 1.0.0
 */

#ifndef FLSTST_CFG_H
#define FLSTST_CFG_H

/*==================================================================================================
 *                                    DEVELOPMENT ERROR DETECT
 *==================================================================================================*/
#define FLSTST_DEV_ERROR_DETECT                 (STD_ON)
#define FLSTST_VERSION_INFO_API                 (STD_ON)

/*==================================================================================================
 *                                    SECTOR CONFIGURATION
 *==================================================================================================*/
#define FLSTST_MAX_SECTORS                      (8U)

/*==================================================================================================
 *                                    MARCH C PARAMETERS
 *==================================================================================================*/
#define FLSTST_MARCH_BACKGROUND_PATTERN         (0x55U)   /* 01010101 */
#define FLSTST_MARCH_BACKGROUND_INVERT          (0xAAU)   /* 10101010 */
#define FLSTST_MARCH_CHECKERBOARD_A             (0xAAU)
#define FLSTST_MARCH_CHECKERBOARD_B             (0x55U)

/*==================================================================================================
 *                                    TIMING
 *==================================================================================================*/
#define FLSTST_MAIN_FUNCTION_PERIOD_MS          (10U)
#define FLSTST_BYTES_PER_CYCLE                  (4U)      /* Words per MainFunction call (deferred) */

/*==================================================================================================
 *                                    ERASE / PROGRAM VERIFY PARAMETERS
 *==================================================================================================*/
#define FLSTST_ERASE_VALUE                      (0xFFU)   /* Expected value after erase */
#define FLSTST_VERIFY_CHUNK_SIZE                (256U)

#endif /* FLSTST_CFG_H */

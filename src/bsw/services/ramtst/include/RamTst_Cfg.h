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
 * @file RamTst_Cfg.h
 * @brief RAM Test Pre-Compile Configuration
 * @version 1.0.0
 */

#ifndef RAMTST_CFG_H
#define RAMTST_CFG_H

/*==================================================================================================
 *                                    DEVELOPMENT ERROR DETECT
 *==================================================================================================*/
#define RAMTST_DEV_ERROR_DETECT                 (STD_ON)
#define RAMTST_VERSION_INFO_API                 (STD_ON)

/*==================================================================================================
 *                                    REGION CONFIGURATION
 *==================================================================================================*/
#define RAMTST_MAX_REGIONS                      (8U)

/*==================================================================================================
 *                                    MARCH C PARAMETERS
 *==================================================================================================*/
#define RAMTST_MARCH_BACKGROUND                 (0x55U)   /* 01010101 */
#define RAMTST_MARCH_BACKGROUND_INVERT          (0xAAU)   /* 10101010 */
#define RAMTST_CHECKERBOARD_A                   (0xAAU)
#define RAMTST_CHECKERBOARD_B                   (0x55U)

/*==================================================================================================
 *                                    TIMING
 *==================================================================================================*/
#define RAMTST_MAIN_FUNCTION_PERIOD_MS          (10U)
#define RAMTST_WORDS_PER_CYCLE                  (8U)      /* 32-bit words per MainFunction call */

/*==================================================================================================
 *                                    FEATURE
 *==================================================================================================*/
#define RAMTST_RUN_ON_STARTUP                   (STD_OFF)

#endif /* RAMTST_CFG_H */

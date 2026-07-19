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
 * @file RamTst_Cfg.h
 * @brief RAM Test Driver Configuration Header
 * @version 2.0.0
 */

#ifndef RAMTST_CFG_H
#define RAMTST_CFG_H

#include "Std_Types.h"

/*==================================================================================================
 *                                    SWITCHES
 *==================================================================================================*/
#define RAMTST_DEV_ERROR_DETECT             STD_ON
#define RAMTST_VERSION_INFO_API             STD_ON
#define RAMTST_SET_MODE_API                 STD_OFF
#define RAMTST_GET_MODE_API                 STD_OFF

/*==================================================================================================
 *                                    MODULE INSTANCE
 *==================================================================================================*/
#define RAMTST_INSTANCE_ID                  0x00U

/*==================================================================================================
 *                                    CONFIGURATION VALUES
 *==================================================================================================*/

/** @brief Default RAM start address (ITCM/DTCM or SRAM) */
#define RAMTST_START_ADDRESS                0x20000000U

/** @brief Default RAM size (128KB) */
#define RAMTST_SIZE                         0x00020000U

/** @brief Default test algorithm */
#define RAMTST_ALGORITHM                    RAMTST_ALGORITHM_MARCH_C

/** @brief MainFunction call cycle in ms */
#define RAMTST_CALL_CYCLE                   10U

/** @brief Default test timeout in ms */
#define RAMTST_TIMEOUT_MS                   5000U

/** @brief Stop on first error */
#define RAMTST_STOP_ON_ERROR                FALSE

/** @brief Pattern seed */
#define RAMTST_PATTERN_SEED                 0xA5A5A5A5U

#endif /* RAMTST_CFG_H */

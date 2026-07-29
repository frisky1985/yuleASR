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
 * @file Eep_Cfg.h
 * @brief EEPROM Driver Configuration Header
 * @version 2.0.0
 */

#ifndef EEP_CFG_H
#define EEP_CFG_H

#include "Std_Types.h"

/*==================================================================================================
 *                                    SWITCHES
 *==================================================================================================*/
#define EEP_DEV_ERROR_DETECT                STD_ON
#define EEP_VERSION_INFO_API                STD_ON
#define EEP_CANCEL_API                      STD_ON

/*==================================================================================================
 *                                    CONFIGURATION VALUES
 *==================================================================================================*/

/** @brief EEPROM backing store base address (RAM or Flash area) */
#ifdef S32K312
#include "S32K312.h"
#define EEP_BASE_ADDRESS                    (S32K312_FLASH_BASE_ALIAS + 0x80000U)
#else
#define EEP_BASE_ADDRESS                    0x08080000U
#endif

/** @brief Total EEPROM size in bytes (64KB) */
#define EEP_SIZE                            0x00010000U

/** @brief Page size for write/erase operations */
#define EEP_PAGE_SIZE                       8U

/** @brief Write cycle time in ms */
#define EEP_WRITE_CYCLE_TIME                10U

/** @brief Erase cycle time in ms */
#define EEP_ERASE_CYCLE_TIME                20U

/** @brief Main function call cycle in ms */
#define EEP_JOB_CALL_CYCLE                  10U

/** @brief Operating mode: FALSE = interrupt, TRUE = polling */
#define EEP_POLLING_MODE                    TRUE

#endif /* EEP_CFG_H */

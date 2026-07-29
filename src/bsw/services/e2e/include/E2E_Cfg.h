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
 * @file    E2E_Cfg.h
 * @brief   E2E Configuration Header
 * @details Pre-compile configuration for E2E library
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#ifndef E2E_CFG_H
#define E2E_CFG_H

/*=============================================================================*
 * Profile Enable Configuration
 *=============================================================================*/
#define E2E_PROFILE_01_ENABLED    STD_ON
#define E2E_PROFILE_02_ENABLED    STD_ON
#define E2E_PROFILE_04_ENABLED    STD_ON
#define E2E_PROFILE_05_ENABLED    STD_ON
#define E2E_PROFILE_06_ENABLED    STD_ON
#define E2E_PROFILE_07_ENABLED    STD_ON

/*=============================================================================*
 * CRC Configuration
 *=============================================================================*/
#define E2E_USE_CRC_HARDWARE      STD_OFF
#define E2E_USE_CRC_SOFTWARE      STD_ON
#define E2E_CRC_TABLE_OPTIMIZED   STD_ON

/*=============================================================================*
 * Development Error Detection
 *=============================================================================*/
#define E2E_DEV_ERROR_DETECT      STD_ON

/*=============================================================================*
 * Version Check API
 *=============================================================================*/
#define E2E_VERSION_INFO_API      STD_ON

/*=============================================================================*
 * Max Data Length Configuration
 *=============================================================================*/
#define E2E_MAX_DATA_LENGTH_P01   30U    /* Max payload for Profile 1 */
#define E2E_MAX_DATA_LENGTH_P02   256U   /* Max payload for Profile 2 */
#define E2E_MAX_DATA_LENGTH_P04   4096U  /* Max payload for Profile 4 */
#define E2E_MAX_DATA_LENGTH_P05   4096U  /* Max payload for Profile 5 */
#define E2E_MAX_DATA_LENGTH_P06   4096U  /* Max payload for Profile 6 */
#define E2E_MAX_DATA_LENGTH_P07   4096U  /* Max payload for Profile 7 */

/*=============================================================================*
 * State Machine Configuration
 *=============================================================================*/
#define E2E_SM_MAX_ERROR_WINDOW   15U
#define E2E_SM_MAX_SYNC_STEPS     2U
#define E2E_SM_MIN_OK_COUNT       2U

#endif /* E2E_CFG_H */

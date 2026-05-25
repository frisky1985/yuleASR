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
 * @brief RAM Test Configuration
 */

#ifndef RAMTST_CFG_H
#define RAMTST_CFG_H

#define RAMTST_DEV_ERROR_DETECT     STD_ON
#define RAMTST_VERSION_INFO_API     STD_ON

/* RAM Test Configuration */
#define RAMTST_START_ADDRESS        0x20000000U
#define RAMTST_SIZE                 0x00020000U  /* 128KB */
#define RAMTST_ALGORITHM            RAMTST_ALGORITHM_MARCH
#define RAMTST_CALL_CYCLE           10U

#endif

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
/* @req SHALL_RAMTST */


/**
 * @file RamTst_Lcfg.c
 * @brief RAM Test Link-Time Configuration
 */

#include "RamTst.h"
#include "RamTst_Cfg.h"

const RamTst_ConfigType RamTst_Config = {
    .StartAddress = RAMTST_START_ADDRESS,
    .Size = RAMTST_SIZE,
    .Algorithm = RAMTST_ALGORITHM,
    .CallCycle = RAMTST_CALL_CYCLE
};

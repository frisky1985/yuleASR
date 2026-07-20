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
 * @file Eep_Lcfg.c
 * @brief EEPROM Link-Time Configuration
 */

#include "Eep.h"
#include "Eep_Cfg.h"

static const Eep_ConfigType Eep_Config = {
    .BaseAddress = EEP_BASE_ADDRESS,
    .Size = EEP_SIZE,
    .JobCallCycle = EEP_JOB_CALL_CYCLE
};

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
 * @file E2E_Lcfg.c
 * @brief E2E Link-time Configuration
 */

#include "E2E.h"
#include "E2E_Cfg.h"

#if (E2E_ENABLED == STD_ON)

/* Profile 1 Configuration */
#if (E2E_PROFILE_1 == STD_ON)
const E2E_P01ConfigType E2E_P01Config[E2E_P01_NUM_CONFIGS] = {
    {
        .CounterOffset = 0U,
        .CRCOffset = 8U,
        .DataID = 0x1234U,
        .DataIDMode = E2E_P01_DATAID_BOTH,
        .DataLength = 64U
    }
};
#endif

/* Profile 2 Configuration */
#if (E2E_PROFILE_2 == STD_ON)
const E2E_P02ConfigType E2E_P02Config[E2E_P02_NUM_CONFIGS] = {
    {
        .CounterOffset = 0U,
        .CRCOffset = 8U,
        .DataID = 0x12345678U,
        .DataLength = 64U
    }
};
#endif

/* Profile 4 Configuration */
#if (E2E_PROFILE_4 == STD_ON)
const E2E_P04ConfigType E2E_P04Config[E2E_P04_NUM_CONFIGS] = {
    {
        .CounterOffset = 0U,
        .CRCOffset = 0U,
        .DataID = 0x12345678U,
        .DataLength = 256U,
        .Offset = 64U
    }
};
#endif

/* Profile 5 Configuration */
#if (E2E_PROFILE_5 == STD_ON)
const E2E_P05ConfigType E2E_P05Config[E2E_P05_NUM_CONFIGS] = {
    {
        .CounterOffset = 0U,
        .CRCOffset = 64U,
        .DataID = 0x12345678U,
        .DataLength = 256U,
        .Offset = 128U
    }
};
#endif

#endif /* E2E_ENABLED */

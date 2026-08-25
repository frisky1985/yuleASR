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
/* @req SHALL_IPDUM */


/**
 * @file IpduM_Lcfg.c
 * @brief IPDU Multiplexer Link-Time Configuration
 */

#include "IpduM.h"
#include "IpduM_Cfg.h"

extern const IpduM_ConfigType IpduM_Config;
static const IpduM_StaticPartType IpduM_StaticParts[IPDUM_MAX_STATIC_PARTS] = {
    { 0U, 0U, 0U },
    { 1U, 1U, 1U },
    { 0xFFFFU, 0xFFFFU, 0U }  /* End marker */
};

const IpduM_ConfigType IpduM_Config = {
    .NumStaticParts = 2U,
    .StaticParts = IpduM_StaticParts
};

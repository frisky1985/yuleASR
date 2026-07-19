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
 * @file SecOc_Lcfg.c
 * @brief SecOc Configuration Tables
 */

#include "SecOc.h"
#include "SecOc_Cfg.h"

/* Security Profiles */
static const SecOC_SecurityProfileType SecOc_Profiles[] = {
    {
        .ProfileId = 0U,
        .AuthenticatorLength = SECOC_AUTH_LENGTH_4,
        .FreshnessLength = SECOC_FRESHNESS_LENGTH_3,
        .UseTrippleFreshness = TRUE
    },
    {
        .ProfileId = 1U,
        .AuthenticatorLength = SECOC_AUTH_LENGTH_8,
        .FreshnessLength = SECOC_FRESHNESS_LENGTH_4,
        .UseTrippleFreshness = TRUE
    }
};

/* PDU Configurations */
static const SecOC_PduConfigType SecOc_Pdus[SECOC_MAX_PDUS] = {
    {
        .PduId = 0U,
        .DataId = 0x0001U,
        .Profile = &SecOc_Profiles[0],
        .FreshnessValueId = 0U
    },
    {
        .PduId = 1U,
        .DataId = 0x0002U,
        .Profile = &SecOc_Profiles[1],
        .FreshnessValueId = 1U
    }
};

/* Configuration */
const SecOC_ConfigType SecOc_Config = {
    .NumPdus = 2U,
    .Pdus = SecOc_Pdus
};

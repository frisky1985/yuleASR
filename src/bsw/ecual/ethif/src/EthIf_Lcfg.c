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
/* @req SWS_EthIf_00001 @req SWS_EthIf_00002 @req SWS_EthIf_00003 */


/**
 * @file EthIf_Lcfg.c
 * @brief Ethernet Interface Configuration Tables
 */

#include "EthIf.h"
#include "EthIf_Cfg.h"

/* Controller Configurations */
static const EthIf_ControllerConfigType EthIf_Controllers[ETHIF_MAX_CONTROLLERS] = {
    {
        .CtrlIdx = 0U,
        .VlanId = 1U,
        .CtrlMode = ETH_MODE_DOWN
    },
    {
        .CtrlIdx = 1U,
        .VlanId = 2U,
        .CtrlMode = ETH_MODE_DOWN
    }
};

/* VLAN Configurations */
static const EthIf_VlanConfigType EthIf_Vlans[ETHIF_MAX_VLANS] = {
    {
        .VlanId = 1U,
        .Priority = 0U
    },
    {
        .VlanId = 2U,
        .Priority = 7U
    }
};

/* Configuration */
static const EthIf_ConfigType EthIf_Config = {
    .Controllers = EthIf_Controllers,
    .NumControllers = 2U,
    .Vlans = EthIf_Vlans,
    .NumVlans = 2U
};

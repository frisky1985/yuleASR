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

/* CanTSyn Link-time Configuration */
#include "CanTSyn.h"
#include "CanTSyn_Cfg.h"

const CanTSyn_TimeDomainConfigType CanTSyn_TimeDomainConfig[CANTSYN_NUMBER_OF_TIME_DOMAINS] = {
    {
        .TimeDomainId = 0,
        .TimeBaseId = 0,
        .IsTimeMaster = TRUE,
        .TxPduId = 0,
        .RxPduId = 1
    }
};

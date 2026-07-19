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

/* Compatibility alias */
typedef CanTSyn_TimeBaseConfigType CanTSyn_TimeDomainConfigType;

const CanTSyn_TimeDomainConfigType CanTSyn_TimeDomainConfig[CANTSYN_NUMBER_OF_TIME_DOMAINS] = {
    {
        .timeBaseId = 0U,
        .domainId = 0U,
        .masterConfig = 2U,  /* Master */
        .IsTimeMaster = TRUE,
        .TxPduId = 0U,
        .syncPeriodMs = 10U,
        .debounceTimeMs = 5U,
        .syncTimeoutMs = 100U,
        .crcSecured = FALSE,
        .useImmediateTransmission = FALSE,
        .syncCanId = 0x180U,
        .fupCanId = 0x280U,
        .ocsCanId = 0x380U,
        .syncTxPduId = 0U,
        .fupTxPduId = 1U,
        .ocsTxPduId = 2U,
        .syncRxPduId = 3U,
        .fupRxPduId = 4U,
        .ocsRxPduId = 5U
    }
};

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
/* @req SHALL_J1939TP */


/* J1939Tp Link-time Configuration */
#include "J1939Tp.h"
#include "J1939Tp_Cfg.h"

const J1939Tp_NSduConfigType J1939Tp_NSduConfig[J1939TP_NUM_NSDUS] = {
    {
        .NSduId = 0,
        .ConnectionIdx = 0,
        .Protocol = J1939TP_PROTOCOL_BAM,
        .TxPduId = 0,
        .RxPduId = 1
    }
};

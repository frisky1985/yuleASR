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


/**
 * @file J1939Tp_Lcfg.c
 * @brief J1939Tp Link-time Configuration
 *
 * @copyright Copyright (c) 2026
 */

#include "J1939Tp.h"

/*==================================================================================================
 *                               Connection Configurations
 *================================================================================================*/
static const J1939Tp_ConnectionConfigType J1939Tp_Connections[J1939TP_MAX_CONNECTIONS] = {
    /* Connection 0 - Standard CMDT */
    {
        .SduId = 0,
        .ComType = J1939TP_CTS,
        .BlockSize = 8U,
        .T1Timeout = 750U,
        .T2Timeout = 1250U,
        .T3Timeout = 1250U,
        .T4Timeout = 1050U,
        .TxPduId = 100U,    /* TP.CM transmission */
        .TxDtPduId = 101U,  /* TP.DT transmission */
        .RxPduId = 102U     /* Reception PDU */
    },
    /* Connection 1 - BAM Broadcast */
    {
        .SduId = 1,
        .ComType = J1939TP_BAM,
        .BlockSize = 0U,
        .T1Timeout = 0U,
        .T2Timeout = 0U,
        .T3Timeout = 0U,
        .T4Timeout = 0U,
        .TxPduId = 103U,
        .TxDtPduId = 104U,
        .RxPduId = 105U
    },
    /* Connection 2 - Direct NPDU */
    {
        .SduId = 2,
        .ComType = J1939TP_DIRECT,
        .BlockSize = 0U,
        .T1Timeout = 0U,
        .T2Timeout = 0U,
        .T3Timeout = 0U,
        .T4Timeout = 0U,
        .TxPduId = 106U,
        .TxDtPduId = 107U,
        .RxPduId = 108U
    },
    /* Additional connections - placeholder */
    {3, J1939TP_CTS, 8U, 750U, 1250U, 1250U, 1050U, 109U, 110U, 111U},
    {4, J1939TP_BAM, 0U, 0U, 0U, 0U, 0U, 112U, 113U, 114U},
    {5, J1939TP_DIRECT, 0U, 0U, 0U, 0U, 0U, 115U, 116U, 117U},
    {6, J1939TP_CTS, 8U, 750U, 1250U, 1250U, 1050U, 118U, 119U, 120U},
    {7, J1939TP_BAM, 0U, 0U, 0U, 0U, 0U, 121U, 122U, 123U}
};

/*==================================================================================================
 *                                 PG Configurations
 *================================================================================================*/
static const J1939Tp_PgConfigType J1939Tp_PgConfigs[J1939TP_MAX_PG] = {
    /* PG 0 - Engine Temperature */
    {
        .PgId = 0,
        .PduId = 200U,
        .DirectNPdu = FALSE,
        .PgIsVariable = FALSE,
        .PgLength = 8U,
        .DirectSdu = 0U,
        .MetaDataLength = 8U
    },
    /* PG 1 - Vehicle Speed */
    {
        .PgId = 1,
        .PduId = 201U,
        .DirectNPdu = FALSE,
        .PgIsVariable = FALSE,
        .PgLength = 8U,
        .DirectSdu = 0U,
        .MetaDataLength = 8U
    },
    /* PG 2 - Diagnostic Multi-packet */
    {
        .PgId = 2,
        .PduId = 202U,
        .DirectNPdu = FALSE,
        .PgIsVariable = TRUE,
        .PgLength = 1785U,
        .DirectSdu = 0U,
        .MetaDataLength = 8U
    },
    /* PG 3 - Software Download */
    {
        .PgId = 3,
        .PduId = 203U,
        .DirectNPdu = FALSE,
        .PgIsVariable = TRUE,
        .PgLength = 1785U,
        .DirectSdu = 0U,
        .MetaDataLength = 8U
    },
    /* Additional PGs - placeholders */
    {4, 204U, FALSE, FALSE, 8U, 0U, 8U},
    {5, 205U, FALSE, FALSE, 8U, 0U, 8U},
    {6, 206U, TRUE, FALSE, 8U, 1U, 8U},
    {7, 207U, TRUE, FALSE, 8U, 1U, 8U},
    {8, 208U, FALSE, TRUE, 100U, 0U, 8U},
    {9, 209U, FALSE, TRUE, 200U, 0U, 8U},
    {10, 210U, FALSE, FALSE, 8U, 0U, 8U},
    {11, 211U, FALSE, FALSE, 8U, 0U, 8U},
    {12, 212U, TRUE, FALSE, 8U, 2U, 8U},
    {13, 213U, TRUE, FALSE, 8U, 2U, 8U},
    {14, 214U, FALSE, TRUE, 500U, 0U, 8U},
    {15, 215U, FALSE, TRUE, 1000U, 0U, 8U},
    {16, 216U, FALSE, FALSE, 8U, 0U, 8U},
    {17, 217U, FALSE, FALSE, 8U, 0U, 8U},
    {18, 218U, TRUE, FALSE, 8U, 3U, 8U},
    {19, 219U, TRUE, FALSE, 8U, 3U, 8U},
    {20, 220U, FALSE, TRUE, 1500U, 0U, 8U},
    {21, 221U, FALSE, FALSE, 8U, 0U, 8U},
    {22, 222U, FALSE, FALSE, 8U, 0U, 8U},
    {23, 223U, TRUE, FALSE, 8U, 4U, 8U},
    {24, 224U, FALSE, TRUE, 800U, 0U, 8U},
    {25, 225U, FALSE, FALSE, 8U, 0U, 8U},
    {26, 226U, TRUE, FALSE, 8U, 5U, 8U},
    {27, 227U, FALSE, TRUE, 1200U, 0U, 8U},
    {28, 228U, FALSE, FALSE, 8U, 0U, 8U},
    {29, 229U, TRUE, FALSE, 8U, 6U, 8U},
    {30, 230U, FALSE, TRUE, 600U, 0U, 8U},
    {31, 231U, FALSE, FALSE, 8U, 0U, 8U}
};

/*==================================================================================================
 *                              Module Configuration
 *================================================================================================*/
const J1939Tp_ConfigType J1939Tp_Config = {
    .ConnectionCount = J1939TP_MAX_CONNECTIONS,
    .Connections = J1939Tp_Connections,
    .PgCount = J1939TP_MAX_PG,
    .PgConfigs = J1939Tp_PgConfigs
};

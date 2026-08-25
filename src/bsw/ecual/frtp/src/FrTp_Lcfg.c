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
/* @req SWS_FrTp_00001 @req SWS_FrTp_00002 @req SWS_FrTp_00005 */


/**
 * @file FrTp_Lcfg.c
 * @brief FlexRay Transport Protocol link-time configuration
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"
#include "FrTp_Lcfg.h"
#include "FrTp_Private.h"

/*==================================================================================================
*                                    CONNECTION CONFIGURATION
==================================================================================================*/
#define FRTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief FrTp connection configurations
 */
const FrTp_ConnectionConfigType FrTp_ConnectionConfigs[FRTP_MAX_CONNECTIONS] =
{
    /* Connection 0: Diagnostic */
    {
        /* connIdx */               0U,
        /* txPduId */               0U,     /* FrIf Tx PDU ID for diagnostic */
        /* rxPduId */               1U,     /* FrIf Rx PDU ID for diagnostic */
        /* maxPayload */            254U,   /* Maximum FlexRay payload */
        /* maxRetries */            3U,     /* Max retry count */
        /* timeoutAs */             100U,   /* N_As: Tx confirmation timeout */
        /* timeoutBs */             100U,   /* N_Bs: FC reception timeout */
        /* timeoutCs */             100U,   /* N_Cs: CF transmission timeout */
        /* timeoutAr */             100U,   /* N_Ar: Rx indication timeout */
        /* timeoutBr */             100U,   /* N_Br: Buffer request timeout */
        /* timeoutCr */             100U,   /* N_Cr: CF reception timeout */
        /* flowControlEnabled */    TRUE,   /* Flow control enabled */
        /* defaultBlockSize */      8U,     /* Default block size */
        /* defaultSTmin */          10U     /* Default STmin (10ms) */
    },
    /* Connection 1: Application */
    {
        /* connIdx */               1U,
        /* txPduId */               2U,
        /* rxPduId */               3U,
        /* maxPayload */            254U,
        /* maxRetries */            3U,
        /* timeoutAs */             100U,
        /* timeoutBs */             100U,
        /* timeoutCs */             100U,
        /* timeoutAr */             100U,
        /* timeoutBr */             100U,
        /* timeoutCr */             100U,
        /* flowControlEnabled */    TRUE,
        /* defaultBlockSize */      8U,
        /* defaultSTmin */          10U
    },
    /* Connection 2: Test */
    {
        /* connIdx */               2U,
        /* txPduId */               4U,
        /* rxPduId */               5U,
        /* maxPayload */            254U,
        /* maxRetries */            3U,
        /* timeoutAs */             100U,
        /* timeoutBs */             100U,
        /* timeoutCs */             100U,
        /* timeoutAr */             100U,
        /* timeoutBr */             100U,
        /* timeoutCr */             100U,
        /* flowControlEnabled */    TRUE,
        /* defaultBlockSize */      8U,
        /* defaultSTmin */          10U
    },
    /* Connection 3: Debug */
    {
        /* connIdx */               3U,
        /* txPduId */               6U,
        /* rxPduId */               7U,
        /* maxPayload */            254U,
        /* maxRetries */            3U,
        /* timeoutAs */             100U,
        /* timeoutBs */             100U,
        /* timeoutCs */             100U,
        /* timeoutAr */             100U,
        /* timeoutBr */             100U,
        /* timeoutCr */             100U,
        /* flowControlEnabled */    TRUE,
        /* defaultBlockSize */      8U,
        /* defaultSTmin */          10U
    }
};

/**
 * @brief FrTp global configuration
 */
const FrTp_ConfigType FrTp_Config =
{
    /* connections */           FrTp_ConnectionConfigs,
    /* numConnections */        FRTP_MAX_CONNECTIONS,
    /* devErrorDetect */        FRTP_DEV_ERROR_DETECT,
    /* versionInfoApi */        FRTP_VERSION_INFO_API
};

#define FRTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    RUNTIME DATA
==================================================================================================*/
#define FRTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief FrTp runtime data structure
 */
FrTp_RuntimeType FrTp_Runtime;

#define FRTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

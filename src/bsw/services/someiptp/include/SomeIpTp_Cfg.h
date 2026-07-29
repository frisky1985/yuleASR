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
 * @file SomeIpTp_Cfg.h
 * @brief SOME/IP Transport Protocol configuration header - AutoSAR R22-11
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef SOMEIPTP_CFG_H
#define SOMEIPTP_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define SOMEIPTP_DEV_ERROR_DETECT               (STD_ON)
#define SOMEIPTP_VERSION_INFO_API               (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define SOMEIPTP_NUMBER_OF_CHANNELS             (4U)

/*==================================================================================================
*                                    BUFFER CONFIGURATION
==================================================================================================*/
#define SOMEIPTP_MAX_PDU_LENGTH                 (65536U)  /* 64KB max SOME/IP message */
#define SOMEIPTP_MAX_SEGMENT_SIZE               (1392U)   /* 1400 - 8 bytes SOME/IP header */
#define SOMEIPTP_RX_BUFFER_SIZE                 (65536U)
#define SOMEIPTP_TX_BUFFER_SIZE                 (65536U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/
#define SOMEIPTP_TX_TIMEOUT_MS                  (1000U)
#define SOMEIPTP_RX_TIMEOUT_MS                  (1000U)
#define SOMEIPTP_RETRANSMIT_TIMEOUT_MS          (500U)

/*==================================================================================================
*                                    RETRY CONFIGURATION
==================================================================================================*/
#define SOMEIPTP_MAX_RETRIES                    (3U)

/*==================================================================================================
*                                    CHANNEL IDs
==================================================================================================*/
#define SOMEIPTP_CHANNEL_ID_0                   (0U)
#define SOMEIPTP_CHANNEL_ID_1                   (1U)
#define SOMEIPTP_CHANNEL_ID_2                   (2U)
#define SOMEIPTP_CHANNEL_ID_3                   (3U)

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define SOMEIPTP_PDU_ID_CHANNEL_0_TX            (0U)
#define SOMEIPTP_PDU_ID_CHANNEL_0_RX            (1U)
#define SOMEIPTP_PDU_ID_CHANNEL_1_TX            (2U)
#define SOMEIPTP_PDU_ID_CHANNEL_1_RX            (3U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define SOMEIPTP_MAIN_FUNCTION_PERIOD_MS        (10U)

#endif /* SOMEIPTP_CFG_H */

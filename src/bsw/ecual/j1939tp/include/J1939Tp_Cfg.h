/**
 * @file J1939Tp_Cfg.h
 * @brief J1939 Transport Protocol configuration header
 * @version 1.0.0
 * @date 2026-05-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * Pre-compile configuration for J1939Tp module
 */

#ifndef J1939TP_CFG_H
#define J1939TP_CFG_H

/*==================================================================================================
*                                    INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "J1939.h"

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/* Development Error Detection */
#define J1939TP_DEV_ERROR_DETECT                (STD_ON)

/* Version Info API */
#define J1939TP_VERSION_INFO_API                (STD_ON)

/* Dynamic channel allocation */
#define J1939TP_DYNAMIC_CHANNEL_ALLOCATION      (STD_OFF)

/* BAM (Broadcast Announce Message) support */
#define J1939TP_BAM_SUPPORT                     (STD_ON)

/* RTS/CTS (Request to Send/Clear to Send) support */
#define J1939TP_RTS_CTS_SUPPORT                 (STD_ON)

/* Change Parameter API */
#define J1939TP_CHANGE_PARAMETER_API            (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define J1939TP_MAX_CHANNEL_CNT                 (2U)
#define J1939TP_MAX_TX_SDU_PER_CHANNEL          (4U)
#define J1939TP_MAX_RX_SDU_PER_CHANNEL          (4U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION (ms)
==================================================================================================*/
#define J1939TP_DEFAULT_TIMEOUT                 (1250U)    /* Default timeout: 1.25s */
#define J1939TP_T1_TIMEOUT                      (750U)     /* T1: Response time */
#define J1939TP_T2_TIMEOUT                      (1250U)    /* T2: Sender response time */
#define J1939TP_T3_TIMEOUT                      (1250U)    /* T3: Receiver response time */
#define J1939TP_T4_TIMEOUT                      (1050U)    /* T4: BAM receive timeout */
#define J1939TP_TH_TIMEOUT                      (500U)     /* TH: Hold timeout */

/*==================================================================================================
*                                    BUFFER CONFIGURATION
==================================================================================================*/
#define J1939TP_MAX_MESSAGE_LENGTH              (1785U)    /* Max J1939 TP message: 255 packets * 7 bytes */
#define J1939TP_MAX_NUM_PACKETS                 (255U)     /* Maximum number of packets */
#define J1939TP_MAX_DT_DATA_LEN                 (7U)       /* DT packet data length */
#define J1939TP_MAX_PACKETS_PER_CTS             (16U)      /* Default max packets per CTS */

/*==================================================================================================
*                                    CAN ID CONFIGURATION
==================================================================================================*/
/* TP.CM CAN ID: PDU1 format, PS = Destination Address */
#define J1939TP_CM_PGN                          (0xEC00U)  /* Connection Management PGN */
#define J1939TP_CM_DA_GLOBAL                    (0xFFU)    /* Global destination address */

/* TP.DT CAN ID: PDU1 format, PS = Destination Address */
#define J1939TP_DT_PGN                          (0xEB00U)  /* Data Transfer PGN */
#define J1939TP_DT_DA_GLOBAL                    (0xFFU)    /* Global destination address */

/* Priority for TP messages */
#define J1939TP_CM_PRIORITY                     (7U)
#define J1939TP_DT_PRIORITY                     (7U)
#define J1939TP_BAM_PRIORITY                    (7U)

/*==================================================================================================
*                                    PDU ID CONFIGURATION
==================================================================================================*/
/* Tx PDU IDs */
#define J1939TP_TX_CM_PDU_ID                    (0U)
#define J1939TP_TX_DT_PDU_ID                    (1U)
#define J1939TP_TX_CONF_CM_ID                   (2U)
#define J1939TP_TX_CONF_DT_ID                   (3U)

/* Rx PDU IDs */
#define J1939TP_RX_CM_PDU_ID                    (4U)
#define J1939TP_RX_DT_PDU_ID                    (5U)

/*==================================================================================================
*                                    RETRY CONFIGURATION
==================================================================================================*/
#define J1939TP_MAX_RETRIES                     (3U)       /* Maximum retry attempts */
#define J1939TP_RETRY_INTERVAL                  (100U)     /* Retry interval (ms) */

/*==================================================================================================
*                                    ADDRESS CONFIGURATION
==================================================================================================*/
/* Default source address */
#define J1939TP_DEFAULT_SA                      (0x80U)    /* Default source address */

/* Default destination address */
#define J1939TP_DEFAULT_DA                      (0x00U)    /* Default destination address */

/* Global/broadcast address */
#define J1939TP_GLOBAL_ADDRESS                  (0xFFU)    /* Global address for BAM */

/* NULL address */
#define J1939TP_NULL_ADDRESS                    (0xFEU)    /* NULL address */

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define J1939TP_MAIN_FUNCTION_PERIOD_MS         (10U)      /* Main function period in ms */

/*==================================================================================================
*                                    EXTERN CONFIGURATION
==================================================================================================*/
#define J1939TP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const J1939Tp_ConfigType J1939Tp_Config;

#define J1939TP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

#endif /* J1939TP_CFG_H */

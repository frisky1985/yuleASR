/**
 * @file J1939Tp_Cfg.h
 * @brief J1939Tp Pre-compile Configuration
 *
 * @copyright Copyright (c) 2026
 */

#ifndef J1939TP_CFG_H
#define J1939TP_CFG_H

/*==================================================================================================
 *                                 Pre-compile Switches
 *================================================================================================*/
#define J1939TP_VERSION_INFO_API        STD_ON
#define J1939TP_DEV_ERROR_DETECT        STD_ON
#define J1939TP_J1939TP_FPP_TX_ENABLED  STD_ON
#define J1939TP_J1939TP_FPP_RX_ENABLED  STD_ON

#define J1939TP_BAM_TX_ENABLED          STD_ON
#define J1939TP_BAM_RX_ENABLED          STD_ON
#define J1939TP_CMDT_TX_ENABLED         STD_ON
#define J1939TP_CMDT_RX_ENABLED         STD_ON

#define J1939TP_MAIN_FUNCTION_PERIOD    ((uint16)10U)   /* 10ms */
#define J1939TP_N_BROADCAST_TIME        ((uint8)50U)    /* 50ms between BAM frames */

/*==================================================================================================
 *                                   Maximum Values
 *================================================================================================*/
#define J1939TP_MAX_PG                  ((uint8)32U)
#define J1939TP_MAX_CONNECTIONS         ((uint8)8U)
#define J1939TP_MAX_TX_CHANNELS         ((uint8)4U)
#define J1939TP_MAX_RX_CHANNELS         ((uint8)4U)

/*==================================================================================================
 *                               Default Timeout Values (ms)
 *================================================================================================*/
#define J1939TP_T1_TIMEOUT_DEFAULT      ((uint16)750U)  /* T1 - Response timeout */
#define J1939TP_T2_TIMEOUT_DEFAULT      ((uint16)1250U) /* T2 - Send CTS timeout */
#define J1939TP_T3_TIMEOUT_DEFAULT      ((uint16)1250U) /* T3 - Send DT timeout */
#define J1939TP_T4_TIMEOUT_DEFAULT      ((uint16)1050U) /* T4 - CTS hold timeout */

#define J1939TP_N_BS_DEFAULT            ((uint16)500U)  /* T3 in ISO-TP terms */
#define J1939TP_N_CS_DEFAULT            ((uint16)500U)  /* Delay between DT frames */
#define J1939TP_N_BR_DEFAULT            ((uint16)0U)    /* Receiver ready delay */
#define J1939TP_N_AR_DEFAULT            ((uint16)500U)  /* Acknowledgment delay */

/*==================================================================================================
 *                              TP.CM (Connection Management) Control Bytes
 *================================================================================================*/
/* RTS Control Byte = 16 (0x10) */
#define J1939TP_CM_RTS                  ((uint8)0x10U)
/* CTS Control Byte = 17 (0x11) */
#define J1939TP_CM_CTS                  ((uint8)0x11U)
/* End of Message Acknowledgment = 19 (0x13) */
#define J1939TP_CM_ACK                  ((uint8)0x13U)
/* BAM Control Byte = 32 (0x20) */
#define J1939TP_CM_BAM                  ((uint8)0x20U)
/* Abort Control Byte = 255 (0xFF) */
#define J1939TP_CM_ABORT                ((uint8)0xFFU)

/*==================================================================================================
 *                                SAE J1939-21 Constants
 *================================================================================================*/
#define J1939TP_DT_PACKET_SIZE          ((uint8)7U)     /* DT data bytes per packet */
#define J1939TP_MAX_TP_PACKETS          ((uint8)255U)   /* Maximum packets in multi-packet */
#define J1939TP_MAX_TP_SIZE             ((uint16)1785U) /* 255 * 7 = max TP size */
#define J1939TP_CM_PACKET_SIZE          ((uint8)8U)     /* TP.CM packet size */
#define J1939TP_PGN_TP_CM               ((uint32)60416U)/* TP.CM PGN = 0xEC00 */
#define J1939TP_PGN_TP_DT               ((uint32)60160U)/* TP.DT PGN = 0xEB00 */

/*==================================================================================================
 *                             CAN ID Construction Macros
 *================================================================================================*/
/* Construct 29-bit CAN ID from Priority, PGN, and SA */
#define J1939TP_CAN_ID(prio, pgn, sa)   ((((uint32)(prio) & 0x07U) << 26) | \
                                         (((uint32)(pgn) & 0x03FFFFU) << 8) | \
                                         ((uint32)(sa) & 0xFFU))

/* Extract SA from CAN ID */
#define J1939TP_GET_SA(canid)           ((uint8)((canid) & 0xFFU))

/* Extract PGN from CAN ID */
#define J1939TP_GET_PGN(canid)          (((canid) >> 8) & 0x03FFFFU)

/* Extract Priority from CAN ID */
#define J1939TP_GET_PRIO(canid)         (((canid) >> 26) & 0x07U)

/*==================================================================================================
 *                                    PGN Ranges
 *================================================================================================*/
/* PDU1 format - Destination specific (PS = DA) */
#define J1939TP_IS_PDU1(pgn)            (((pgn) & 0x00F000U) < 0x00F000U)

/* PDU2 format - Broadcast (PS = Group Extension) */
#define J1939TP_IS_PDU2(pgn)            (((pgn) & 0x00F000U) >= 0x00F000U)

/*==================================================================================================
 *                             External Configuration References
 *================================================================================================*/
extern const J1939Tp_ConfigType J1939Tp_Config;

#endif /* J1939TP_CFG_H */

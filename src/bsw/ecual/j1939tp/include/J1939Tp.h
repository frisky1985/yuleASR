/**
 * @file J1939Tp.h
 * @brief J1939 Transport Protocol module for commercial vehicles
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * Standards: SAE J1939-21 (Data Link Layer) and J1939-22 (TP)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Purpose: Transport Protocol for heavy-duty commercial vehicles
 *
 * Supports:
 * - BAM (Broadcast Announce Message) - 1-to-N broadcast
 * - RTS/CTS (Request To Send / Clear To Send) - 1-to-1 peer-to-peer
 * - TP.CM (Transport Protocol Connection Management)
 * - TP.DT (Transport Protocol Data Transfer)
 */

#ifndef J1939TP_H
#define J1939TP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "J1939Tp_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define J1939TP_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define J1939TP_MODULE_ID                 (0x3EU) /* J1939TP Module ID */
#define J1939TP_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define J1939TP_AR_RELEASE_MINOR_VERSION  (0x04U)
#define J1939TP_AR_RELEASE_REVISION_VERSION (0x00U)
#define J1939TP_SW_MAJOR_VERSION          (0x01U)
#define J1939TP_SW_MINOR_VERSION          (0x00U)
#define J1939TP_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define J1939TP_SID_INIT                  (0x01U)
#define J1939TP_SID_SHUTDOWN              (0x02U)
#define J1939TP_SID_TRANSMIT              (0x03U)
#define J1939TP_SID_CANCELTRANSMIT        (0x04U)
#define J1939TP_SID_CANCELRECEIVE         (0x05U)
#define J1939TP_SID_GETVERSIONINFO        (0x08U)
#define J1939TP_SID_MAINFUNCTION          (0x09U)
#define J1939TP_SID_RXINDICATION          (0x42U)
#define J1939TP_SID_TXCONFIRMATION        (0x43U)
#define J1939TP_SID_CHANGEPARAMETER       (0x06U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define J1939TP_E_PARAM_CONFIG            (0x01U)
#define J1939TP_E_PARAM_ID                (0x02U)
#define J1939TP_E_PARAM_POINTER           (0x03U)
#define J1939TP_E_INIT_FAILED             (0x04U)
#define J1939TP_E_UNINIT                  (0x20U)
#define J1939TP_E_INVALID_TX_ID           (0x30U)
#define J1939TP_E_INVALID_RX_ID           (0x40U)
#define J1939TP_E_INVALID_TX_BUFFER       (0x50U)
#define J1939TP_E_INVALID_RX_BUFFER       (0x60U)
#define J1939TP_E_INVALID_TX_LENGTH       (0x70U)
#define J1939TP_E_INVALID_RX_LENGTH       (0x80U)
#define J1939TP_E_INVALID_TATYPE          (0x90U)
#define J1939TP_E_OPER_NOT_SUPPORTED      (0xA0U)
#define J1939TP_E_COM                     (0xB0U)
#define J1939TP_E_INVALID_PG              (0xC0U)

/*==================================================================================================
*                                    RUNTIME ERROR CODES
==================================================================================================*/
#define J1939TP_E_RX_COM                  (0x01U)
#define J1939TP_E_TX_COM                  (0x02U)
#define J1939TP_E_RX_TIMEOUT_T1           (0x03U)  /* Response timeout */
#define J1939TP_E_RX_TIMEOUT_T2           (0x04U)  /* Sender timeout */
#define J1939TP_E_RX_TIMEOUT_T3           (0x05U)  /* Receiver timeout */
#define J1939TP_E_RX_TIMEOUT_T4           (0x06U)  /* BAM inter-packet time */
#define J1939TP_E_RX_TIMEOUT_TH           (0x07U)  /* Hold timeout */
#define J1939TP_E_TX_TIMEOUT_T1           (0x08U)
#define J1939TP_E_TX_TIMEOUT_T2           (0x09U)
#define J1939TP_E_TX_TIMEOUT_T3           (0x0AU)
#define J1939TP_E_RX_INVALID_SN           (0x0BU)
#define J1939TP_E_RX_UNEXPECTED_DT        (0x0CU)
#define J1939TP_E_RX_UNEXPECTED_CM        (0x0DU)
#define J1939TP_E_RTS_MISMATCH            (0x0EU)
#define J1939TP_E_CTS_MISMATCH            (0x0FU)
#define J1939TP_E_BAM_INCOMPLETE          (0x10U)
#define J1939TP_E_CHANNEL_BUSY            (0x11U)
#define J1939TP_E_INVALID_SA_DA           (0x12U)

/*==================================================================================================
*                                    J1939 PDU FORMAT CONSTANTS
==================================================================================================*/
/* TP.CM - Connection Management (PDU1 format, PS = DA) */
#define J1939TP_CM_PGN                    (0x00EC00U)  /* 60416 decimal */

/* TP.DT - Data Transfer (PDU1 format, PS = DA) */
#define J1939TP_DT_PGN                    (0x00EB00U)  /* 60160 decimal */

/* BAM - Broadcast Announce Message (global DA = 0xFF) */
#define J1939TP_BAM_GLOBAL_DA             (0xFFU)

/*==================================================================================================
*                                    TP.CM CONTROL BYTE DEFINITIONS
==================================================================================================*/
#define J1939TP_CM_RTS                    (0x10U)  /* Request To Send */
#define J1939TP_CM_CTS                    (0x11U)  /* Clear To Send */
#define J1939TP_CM_EOMACK                 (0x13U)  /* End of Message Acknowledge */
#define J1939TP_CM_BAM                    (0x20U)  /* Broadcast Announce Message */
#define J1939TP_CM_ABORT                  (0xFFU)  /* Connection Abort */

/*==================================================================================================
*                                    TP.CM BYTE OFFSETS
==================================================================================================*/
/* RTS/CTS/BAM common offsets */
#define J1939TP_CM_BYTE_CONTROL           (0U)   /* Control byte */
#define J1939TP_CM_BYTE_TOTAL_SIZE_LO     (1U)   /* Total message size (LSB) */
#define J1939TP_CM_BYTE_TOTAL_SIZE_HI     (2U)   /* Total message size (MSB) */
#define J1939TP_CM_BYTE_NUM_PACKETS       (3U)   /* Total number of packets */
#define J1939TP_CM_BYTE_MAX_PACKETS       (4U)   /* Max packets per CTS (RTS only) */

/* CTS specific offsets */
#define J1939TP_CTS_BYTE_NEXT_SN          (5U)   /* Next expected packet sequence number */

/* Abort reason offset */
#define J1939TP_ABORT_BYTE_REASON         (5U)   /* Abort reason code */

/* PGN offsets (bytes 5-7, little-endian) */
#define J1939TP_CM_BYTE_PGN_LO            (5U)   /* PGN LSB */
#define J1939TP_CM_BYTE_PGN_MID           (6U)   /* PGN middle byte */
#define J1939TP_CM_BYTE_PGN_HI            (7U)   /* PGN MSB */

/*==================================================================================================
*                                    TP.DT BYTE OFFSETS
==================================================================================================*/
#define J1939TP_DT_BYTE_SN                (0U)   /* Sequence number */
#define J1939TP_DT_DATA_START             (1U)   /* Data starts here */
#define J1939TP_DT_MAX_DATA_LEN           (7U)   /* 7 bytes per DT frame */

/*==================================================================================================
*                                    J1939 TIMING CONSTANTS (ms)
==================================================================================================*/
/* T1 - Response time: receiver must send CTS within 200ms of RTS */
#define J1939TP_T1_TIMEOUT_DEFAULT        (200U)

/* T2 - Sender response time: sender must send data within 50ms of CTS */
#define J1939TP_T2_TIMEOUT_DEFAULT        (50U)

/* T3 - Receiver timeout: receiver must receive packet within 200ms of CTS or prev data */
#define J1939TP_T3_TIMEOUT_DEFAULT        (200U)

/* T4 - BAM inter-packet time: min 50ms between BAM packets */
#define J1939TP_T4_TIMEOUT_DEFAULT        (50U)

/* Th - Hold timeout: max 750ms between CTS messages when holding */
#define J1939TP_TH_TIMEOUT_DEFAULT        (750U)

/*==================================================================================================
*                                    J1939 PROTOCOL LIMITS
==================================================================================================*/
#define J1939TP_MAX_MESSAGE_LENGTH        (1785U)  /* 255 packets * 7 bytes */
#define J1939TP_MAX_DT_PACKETS            (255U)   /* Max sequence number */
#define J1939TP_MAX_CTS_PACKETS           (255U)   /* Max packets per CTS */
#define J1939TP_CAN_FRAME_SIZE            (8U)     /* Standard CAN frame size */

/* J1939 Name fields */
#define J1939TP_DEFAULT_SA                (0xFEU)  /* Cannot claim address */
#define J1939TP_GLOBAL_DA                 (0xFFU)  /* Global destination */

/*==================================================================================================
*                                    J1939 ADDRESSING TYPE
==================================================================================================*/
typedef uint8 J1939Tp_AddressType;

/*==================================================================================================
*                                    J1939 PGN TYPE
==================================================================================================*/
typedef uint32 J1939Tp_PgnType;

/*==================================================================================================
*                                    J1939 CHANNEL TYPE
==================================================================================================*/
typedef uint8 J1939Tp_ChannelType;

/*==================================================================================================
*                                    J1939 CONNECTION TYPE
==================================================================================================*/
typedef enum {
    J1939TP_CONN_IDLE = 0,
    J1939TP_CONN_BAM_TX,            /* BAM transmitting */
    J1939TP_CONN_BAM_RX,            /* BAM receiving */
    J1939TP_CONN_RTS_TX_WAIT_CTS,   /* RTS sent, waiting for CTS */
    J1939TP_CONN_CTS_RX_WAIT_DATA,  /* CTS sent, waiting for data */
    J1939TP_CONN_DT_TX,             /* Transmitting DT packets */
    J1939TP_CONN_DT_RX,             /* Receiving DT packets */
    J1939TP_CONN_WAIT_EOMACK,       /* Waiting for EOM ACK */
    J1939TP_CONN_COMPLETE
} J1939Tp_ConnectionStateType;

/*==================================================================================================
*                                    J1939 PROTOCOL TYPE
==================================================================================================*/
typedef enum {
    J1939TP_PROTOCOL_BAM = 0,       /* Broadcast Announce Message */
    J1939TP_PROTOCOL_RTS_CTS        /* RTS/CTS peer-to-peer */
} J1939Tp_ProtocolType;

/*==================================================================================================
*                                    J1939 COMMUNICATION TYPE
==================================================================================================*/
typedef enum {
    J1939TP_COMM_BROADCAST = 0,     /* Broadcast (BAM) */
    J1939TP_COMM_PEER_TO_PEER       /* Peer-to-peer (RTS/CTS) */
} J1939Tp_CommType;

/*==================================================================================================
*                                    J1939 CHANNEL MODE
==================================================================================================*/
typedef enum {
    J1939TP_MODE_FULL_DUPLEX = 0,
    J1939TP_MODE_HALF_DUPLEX
} J1939Tp_ChannelModeType;

/*==================================================================================================
*                                    J1939 NSDU TYPE
==================================================================================================*/
typedef uint8 J1939Tp_NsduType;

/*==================================================================================================
*                                    J1939 TX NSDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType J1939TpTxPduId;
    PduIdType J1939TpTxPduConfirmationId;
    J1939Tp_PgnType J1939TpTxPgn;
    J1939Tp_AddressType J1939TpTxSa;
    J1939Tp_AddressType J1939TpTxDa;
    uint16 J1939TpTxT1Timeout;      /* T1 timeout in ms */
    uint16 J1939TpTxT2Timeout;      /* T2 timeout in ms */
    uint16 J1939TpTxT3Timeout;      /* T3 timeout in ms */
    uint16 J1939TpTxT4Timeout;      /* T4 timeout in ms (BAM only) */
    uint16 J1939TpTxMaxMessageLength;
    uint8 J1939TpTxProtocolType;    /* BAM or RTS/CTS */
    uint8 J1939TpTxCommType;        /* Broadcast or Peer-to-peer */
    uint8 J1939TpTxPriority;
} J1939Tp_TxNsduConfigType;

/*==================================================================================================
*                                    J1939 RX NSDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType J1939TpRxPduId;
    PduIdType J1939TpRxPduConfirmationId;
    J1939Tp_PgnType J1939TpRxPgn;
    J1939Tp_AddressType J1939TpRxSa;
    J1939Tp_AddressType J1939TpRxDa;
    uint16 J1939TpRxT1Timeout;      /* T1 timeout in ms */
    uint16 J1939TpRxT2Timeout;      /* T2 timeout in ms */
    uint16 J1939TpRxT3Timeout;      /* T3 timeout in ms */
    uint16 J1939TpRxThTimeout;      /* Th hold timeout in ms */
    uint16 J1939TpRxMaxMessageLength;
    uint8 J1939TpRxMaxCtsPackets;   /* Max packets per CTS */
    uint8 J1939TpRxProtocolType;
    uint8 J1939TpRxCommType;
    uint8 J1939TpRxPriority;
} J1939Tp_RxNsduConfigType;

/*==================================================================================================
*                                    J1939 CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    J1939Tp_ChannelType ChannelId;
    J1939Tp_ChannelModeType ChannelMode;
    uint8 NumTxNsdu;
    uint8 NumRxNsdu;
    const J1939Tp_TxNsduConfigType* TxNsduConfigs;
    const J1939Tp_RxNsduConfigType* RxNsduConfigs;
} J1939Tp_ChannelConfigType;

/*==================================================================================================
*                                    J1939 GENERAL CONFIG TYPE
==================================================================================================*/
typedef struct {
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean J1939TpDynamicChannelAllocation;
    uint8 J1939TpMaxChannelCnt;
    uint16 J1939TpMainFunctionPeriod;
    boolean J1939TpChangeParameterApi;
} J1939Tp_GeneralConfigType;

/*==================================================================================================
*                                    J1939 CONFIG TYPE
==================================================================================================*/
typedef struct {
    const J1939Tp_GeneralConfigType* GeneralConfig;
    const J1939Tp_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
} J1939Tp_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define J1939TP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const J1939Tp_ConfigType J1939Tp_Config;

#define J1939TP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define J1939TP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the J1939 Transport Protocol module
 * @param CfgPtr Pointer to configuration structure
 */
void J1939Tp_Init(const J1939Tp_ConfigType* CfgPtr);

/**
 * @brief Shuts down the J1939 Transport Protocol module
 */
void J1939Tp_Shutdown(void);

/**
 * @brief Transmits data using J1939 transport protocol
 * @param TxSduId Tx SDU ID
 * @param TxInfoPtr Pointer to Tx info
 * @return Result of operation
 */
Std_ReturnType J1939Tp_Transmit(PduIdType TxSduId, const PduInfoType* TxInfoPtr);

/**
 * @brief Cancels an ongoing transmission
 * @param TxSduId Tx SDU ID to cancel
 * @return Result of operation
 */
Std_ReturnType J1939Tp_CancelTransmit(PduIdType TxSduId);

/**
 * @brief Cancels an ongoing reception
 * @param RxSduId Rx SDU ID to cancel
 * @return Result of operation
 */
Std_ReturnType J1939Tp_CancelReceive(PduIdType RxSduId);

/**
 * @brief Changes protocol parameters
 * @param id SDU ID
 * @param parameter Parameter to change
 * @param value New value
 * @return Result of operation
 */
Std_ReturnType J1939Tp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void J1939Tp_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Main function for periodic processing
 */
void J1939Tp_MainFunction(void);

/**
 * @brief Rx indication callback from CAN Interface
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void J1939Tp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Tx confirmation callback from CAN Interface
 * @param TxPduId Transmitted PDU ID
 */
void J1939Tp_TxConfirmation(PduIdType TxPduId);

#define J1939TP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* J1939TP_H */

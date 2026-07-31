/**
 * @file J1939Tp.h
 * @brief J1939 Transport Protocol module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-05-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: J1939 Transport Protocol (J1939Tp)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Purpose: SAE J1939-21 transport protocol for commercial vehicle communication
 *          Supports multi-frame transport, BAM, and RTS/CTS flow control
 */

#ifndef J1939TP_H
#define J1939TP_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "J1939Tp_Cfg.h"
#include "ComStack_Types.h"
#include "J1939.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define J1939TP_VENDOR_ID                      (0x01U) /* YuleTech Vendor ID */
#define J1939TP_MODULE_ID                      (0x44U) /* J1939TP Module ID */
#define J1939TP_AR_RELEASE_MAJOR_VERSION       (0x04U)
#define J1939TP_AR_RELEASE_MINOR_VERSION       (0x04U)
#define J1939TP_AR_RELEASE_REVISION_VERSION    (0x00U)
#define J1939TP_SW_MAJOR_VERSION               (0x01U)
#define J1939TP_SW_MINOR_VERSION               (0x00U)
#define J1939TP_SW_PATCH_VERSION               (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define J1939TP_SID_INIT                       (0x01U)
#define J1939TP_SID_SHUTDOWN                   (0x02U)
#define J1939TP_SID_TRANSMIT                   (0x03U)
#define J1939TP_SID_CANCELTRANSMIT             (0x04U)
#define J1939TP_SID_CANCELRECEIVE              (0x05U)
#define J1939TP_SID_CHANGEPARAMETER            (0x06U)
#define J1939TP_SID_GETVERSIONINFO             (0x07U)
#define J1939TP_SID_MAINFUNCTION               (0x08U)
#define J1939TP_SID_RXINDICATION               (0x42U)
#define J1939TP_SID_TXCONFIRMATION             (0x43U)
#define J1939TP_SID_TPSTARTRECEPTION           (0x44U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define J1939TP_E_PARAM_CONFIG                 (0x01U)
#define J1939TP_E_PARAM_ID                     (0x02U)
#define J1939TP_E_PARAM_POINTER                (0x03U)
#define J1939TP_E_PARAM_LENGTH                 (0x04U)
#define J1939TP_E_INIT_FAILED                  (0x05U)
#define J1939TP_E_UNINIT                       (0x20U)
#define J1939TP_E_INVALID_TX_ID                (0x30U)
#define J1939TP_E_INVALID_RX_ID                (0x40U)
#define J1939TP_E_INVALID_TX_BUFFER            (0x50U)
#define J1939TP_E_INVALID_RX_BUFFER            (0x60U)
#define J1939TP_E_INVALID_TX_LENGTH            (0x70U)
#define J1939TP_E_INVALID_RX_LENGTH            (0x80U)
#define J1939TP_E_INVALID_PGN                  (0x90U)
#define J1939TP_E_INVALID_DA                   (0xA0U)
#define J1939TP_E_INVALID_SA                   (0xB0U)
#define J1939TP_E_COM                          (0xC0U)
#define J1939TP_E_INVALID_STATE                (0xD0U)

/*==================================================================================================
*                                    RUNTIME ERROR CODES
==================================================================================================*/
#define J1939TP_E_RX_COM                       (0x01U)
#define J1939TP_E_TX_COM                       (0x02U)
#define J1939TP_E_RX_TIMEOUT                   (0x03U)
#define J1939TP_E_TX_TIMEOUT                   (0x04U)
#define J1939TP_E_RX_INVALID_SN                (0x05U)
#define J1939TP_E_RX_UNEXPECTED_CM             (0x06U)
#define J1939TP_E_RX_UNEXPECTED_DT             (0x07U)
#define J1939TP_E_RX_BAM_OVERLAP               (0x08U)
#define J1939TP_E_RX_BAM_TIMEOUT               (0x09U)
#define J1939TP_E_TX_ABORTED                   (0x0AU)
#define J1939TP_E_RX_ABORTED                   (0x0BU)
#define J1939TP_E_BUFFER_OVERFLOW              (0x0CU)
#define J1939TP_E_INVALID_SEQUENCE             (0x0DU)
#define J1939TP_E_NO_CONNECTION                (0x0EU)
#define J1939TP_E_CONNECTION_BUSY              (0x0FU)

/*==================================================================================================
*                                    J1939 TP CM MESSAGE TYPES (Control Byte)
*                                    SAE J1939-21 Section 5.10.3
==================================================================================================*/
typedef enum {
    J1939TP_CM_RTS = 16,           /* Request to Send - 0x10 */
    J1939TP_CM_CTS = 17,           /* Clear to Send - 0x11 */
    J1939TP_CM_EOM_ACK = 19,       /* End of Message Acknowledgment - 0x13 */
    J1939TP_CM_BAM = 32,           /* Broadcast Announce Message - 0x20 */
    J1939TP_CM_ABORT = 255         /* Connection Abort - 0xFF */
} J1939Tp_CmType;

/*==================================================================================================
*                                    J1939 TP CONNECTION STATE
==================================================================================================*/
typedef enum {
    J1939TP_CONN_IDLE = 0,         /* Connection idle */
    J1939TP_CONN_TX_RTS_SENT,      /* RTS sent, waiting for CTS */
    J1939TP_CONN_TX_DT_SENDING,    /* Sending DT frames */
    J1939TP_CONN_TX_WAIT_CTS,      /* Waiting for next CTS */
    J1939TP_CONN_TX_WAIT_EOM_ACK,  /* Waiting for EOM ACK */
    J1939TP_CONN_RX_WAIT_RTS,      /* Waiting for RTS */
    J1939TP_CONN_RX_SENDING_CTS,   /* Sending CTS */
    J1939TP_CONN_RX_DT_RECEIVING,  /* Receiving DT frames */
    J1939TP_CONN_RX_SENDING_EOM,   /* Sending EOM ACK */
    J1939TP_CONN_BAM_RECEIVING,    /* Receiving BAM broadcast */
    J1939TP_CONN_TX_BAM_SENDING,   /* Sending BAM broadcast */
    J1939TP_CONN_ABORTED           /* Connection aborted */
} J1939Tp_ConnectionStateType;

/*==================================================================================================
*                                    J1939 TP ABORT REASONS
==================================================================================================*/
typedef enum {
    J1939TP_ABORT_BUSY = 1,        /* Already in one or more connection managed sessions */
    J1939TP_ABORT_RESOURCES = 2,   /* System resources were needed for another task */
    J1939TP_ABORT_TIMEOUT = 3,     /* A timeout occurred and this is the connection abort */
    J1939TP_ABORT_NO_CTS = 4,      /* CTS messages received when data transfer is in progress */
    J1939TP_ABORT_MAX_RETRIES = 5, /* Maximum number of retransmissions reached */
    J1939TP_ABORT_UNEXPECTED_DT = 6, /* Unexpected data transfer packet */
    J1939TP_ABORT_BAD_SEQ = 7,     /* Bad sequence number */
    J1939TP_ABORT_DUP_SEQ = 8,     /* Duplicate sequence number */
    J1939TP_ABORT_MSG_TOO_LONG = 9,/* Message is too large */
    J1939TP_ABORT_UNKNOWN = 255    /* Abort reason not specified */
} J1939Tp_AbortReasonType;

/*==================================================================================================
*                                    J1939 TP CONNECTION TYPE
==================================================================================================*/
typedef enum {
    J1939TP_CONN_TYPE_UNICAST = 0, /* Point-to-point (RTS/CTS) */
    J1939TP_CONN_TYPE_BROADCAST    /* Broadcast (BAM) */
} J1939Tp_ConnTypeType;

/*==================================================================================================
*                                    J1939 TP NSDU CONFIG TYPE
*==================================================================================================*/
#define J1939TP_NUM_CONNECTIONS                 (2U)
#define J1939TP_NUM_NSDUS                      (1U)
#define J1939TP_PROTOCOL_BAM                   (0U)
#define J1939TP_PROTOCOL_RTSCTS                (1U)

typedef struct {
    PduIdType NSduId;
    uint8     ConnectionIdx;
    uint8     Protocol;
    PduIdType TxPduId;
    PduIdType RxPduId;
} J1939Tp_NSduConfigType;

extern const J1939Tp_NSduConfigType J1939Tp_NSduConfig[J1939TP_NUM_NSDUS];

/*==================================================================================================
*                                    J1939 TP CHANNEL TYPE
==================================================================================================*/
typedef uint8 J1939Tp_ChannelType;

/*==================================================================================================
*                                    J1939 TP PG TYPE (Parameter Group)
==================================================================================================*/
typedef uint32 J1939Tp_PgType;

/*==================================================================================================
*                                    J1939 TP ADDRESS TYPE
==================================================================================================*/
typedef uint8 J1939Tp_AddressType;

/*==================================================================================================
*                                    J1939 TP CM PDU STRUCTURE
*                                    SAE J1939-21 Section 5.10.3
==================================================================================================*/
typedef struct {
    uint8 ControlByte;             /* Control byte (RTS/CTS/BAM/EOM/Abort) */
    uint16 TotalMessageSize;       /* Total size of message (bytes) */
    uint8 NumPackets;              /* Total number of packets */
    uint8 MaxNumPackets;           /* Maximum number of packets (CTS only) */
    uint8 NextPacketNum;           /* Next packet number (CTS only) */
    J1939Tp_PgType Pgn;            /* Parameter Group Number */
    J1939Tp_AddressType SourceAddress;    /* Source address */
    J1939Tp_AddressType DestinationAddress; /* Destination address */
} J1939Tp_CmPduType;

/*==================================================================================================
*                                    J1939 TP DT PDU STRUCTURE
*                                    SAE J1939-21 Section 5.10.4
==================================================================================================*/
typedef struct {
    uint8 SequenceNumber;          /* Sequence number (1-255) */
    uint8 Data[7];                 /* Data bytes */
    uint8 DataLength;              /* Valid data length in this packet */
} J1939Tp_DtPduType;

/*==================================================================================================
*                                    J1939 TP TX SDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType J1939TpTxSduId;           /* Tx SDU ID */
    PduIdType J1939TpTxFcPduId;         /* Tx Flow Control PDU ID */
    PduIdType J1939TpTxDtPduId;         /* Tx Data Transfer PDU ID */
    PduIdType J1939TpTxConfirmationId;  /* Tx Confirmation ID */
    J1939Tp_PgType J1939TpTxPg;         /* Parameter Group Number */
    J1939Tp_AddressType J1939TpTxSa;    /* Source Address */
    J1939Tp_AddressType J1939TpTxDa;    /* Destination Address */
    uint16 J1939TpTxMaxMessageLength;   /* Maximum message length */
    uint16 J1939TpTxTimeout;            /* Transmission timeout (ms) */
    uint16 J1939TpTxRetryLimit;         /* Maximum retry count */
    uint8 J1939TpTxPriority;            /* CAN priority */
} J1939Tp_TxSduConfigType;

/*==================================================================================================
*                                    J1939 TP RX SDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType J1939TpRxSduId;           /* Rx SDU ID */
    PduIdType J1939TpRxFcPduId;         /* Rx Flow Control PDU ID */
    PduIdType J1939TpRxDtPduId;         /* Rx Data Transfer PDU ID */
    PduIdType J1939TpRxIndicationId;    /* Rx Indication ID */
    J1939Tp_PgType J1939TpRxPg;         /* Parameter Group Number */
    J1939Tp_AddressType J1939TpRxSa;    /* Source Address (expected) */
    J1939Tp_AddressType J1939TpRxDa;    /* Destination Address (expected) */
    uint16 J1939TpRxMaxMessageLength;   /* Maximum message length */
    uint16 J1939TpRxTimeout;            /* Reception timeout (ms) */
    uint16 J1939TpRxMaxNumPackets;      /* Maximum packets per CTS */
    uint8 J1939TpRxPriority;            /* CAN priority */
} J1939Tp_RxSduConfigType;

/*==================================================================================================
*                                    J1939 TP CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    J1939Tp_ChannelType ChannelId;      /* Channel ID */
    uint8 NumTxSdu;                     /* Number of Tx SDUs */
    uint8 NumRxSdu;                     /* Number of Rx SDUs */
    const J1939Tp_TxSduConfigType* TxSduConfigs;  /* Tx SDU configurations */
    const J1939Tp_RxSduConfigType* RxSduConfigs;  /* Rx SDU configurations */
} J1939Tp_ChannelConfigType;

/*==================================================================================================
*                                    J1939 TP GENERAL CONFIG TYPE
==================================================================================================*/
typedef struct {
    boolean DevErrorDetect;             /* Development error detection */
    boolean VersionInfoApi;             /* Version info API enable */
    boolean J1939TpDynamicChannelAllocation; /* Dynamic channel allocation */
    uint8 J1939TpMaxChannelCnt;         /* Maximum number of channels */
    boolean J1939TpBamSupport;          /* BAM support enabled */
    boolean J1939TpRtsCtsSupport;       /* RTS/CTS support enabled */
    uint16 J1939TpMainFunctionPeriod;   /* Main function period (ms) */
    uint16 J1939TpDefaultTimeout;       /* Default timeout (ms) */
    uint8 J1939TpMaxNumPackets;         /* Default max packets per CTS */
} J1939Tp_GeneralConfigType;

/*==================================================================================================
*                                    J1939 TP CONFIG TYPE
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
 * @param ConfigPtr Pointer to configuration structure
 */
void J1939Tp_Init(const J1939Tp_ConfigType* ConfigPtr);

/**
 * @brief Shuts down the J1939 Transport Protocol module
 */
void J1939Tp_Shutdown(void);

/**
 * @brief Transmits data using J1939 transport protocol
 * @param TxSduId Tx SDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType J1939Tp_Transmit(PduIdType TxSduId, const PduInfoType* PduInfoPtr);

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
void J1939Tp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Start reception of a TP message
 * @param RxSduId Rx SDU ID
 * @param PduInfoPtr Pointer to PDU info
 * @param TpSduLength Total TP SDU length
 * @return Result of operation
 */
Std_ReturnType J1939Tp_StartReception(PduIdType RxSduId, const PduInfoType* PduInfoPtr, PduLengthType TpSduLength);

#define J1939TP_STOP_SEC_CODE
#include "MemMap.h"

#endif /* J1939TP_H */

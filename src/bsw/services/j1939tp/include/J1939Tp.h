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
 * @file J1939Tp.h
 * @brief AUTOSAR J1939 Transport Protocol (J1939Tp) module header
 *
 * Implements SAE J1939-21 Transport Protocol for commercial vehicle
 * multi-frame message transmission over CAN.
 *
 * @copyright Copyright (c) 2026
 * @version 1.0.0
 */

#ifndef J1939TP_H
#define J1939TP_H

/*==================================================================================================
 *                                           Includes
 *================================================================================================*/
#include "Std_Types.h"
#include "J1939Tp_Cfg.h"

/*==================================================================================================
 *                                    Macro Definitions
 *================================================================================================*/
#define J1939TP_MODULE_ID               ((uint16)79U)
#define J1939TP_VENDOR_ID               ((uint16)1U)

#define J1939TP_SW_MAJOR_VERSION        ((uint8)1U)
#define J1939TP_SW_MINOR_VERSION        ((uint8)0U)
#define J1939TP_SW_PATCH_VERSION        ((uint8)0U)

/* Service IDs for DET */
#define J1939TP_SID_INIT                ((uint8)0x01U)
#define J1939TP_SID_DEINIT              ((uint8)0x02U)
#define J1939TP_SID_GET_VERSION_INFO    ((uint8)0x03U)
#define J1939TP_SID_MAIN_FUNCTION       ((uint8)0x04U)
#define J1939TP_SID_TRANSMIT            ((uint8)0x05U)
#define J1939TP_SID_CANCEL_TRANSMIT     ((uint8)0x06U)
#define J1939TP_SID_CANCEL_RECEIVE      ((uint8)0x07U)
#define J1939TP_SID_CHANGE_PARAMETER    ((uint8)0x08U)
#define J1939TP_SID_RX_INDICATION       ((uint8)0x42U)
#define J1939TP_SID_TX_CONFIRMATION     ((uint8)0x40U)

/* Error codes */
#define J1939TP_E_NO_ERROR              ((uint8)0x00U)
#define J1939TP_E_PARAM_POINTER         ((uint8)0x01U)
#define J1939TP_E_UNINIT                ((uint8)0x02U)
#define J1939TP_E_INVALID_PDU_SDU_ID    ((uint8)0x03U)
#define J1939TP_E_INVALID_TSA           ((uint8)0x04U)
#define J1939TP_E_INVALID_PARAMETER     ((uint8)0x05U)
#define J1939TP_E_CANCEL_NOT_SUPPORTED  ((uint8)0x06U)
#define J1939TP_E_INVALID_VALUE         ((uint8)0x07U)
#define J1939TP_E_INIT_FAILED           ((uint8)0x08U)

/*==================================================================================================
 *                                     Type Definitions
 *================================================================================================*/

typedef uint16 J1939Tp_PgType;
typedef uint16 J1939Tp_SduType;

/**
 * @brief J1939 TP state type
 */
typedef enum {
    J1939TP_STATE_UNINIT = 0,
    J1939TP_STATE_INIT,
    J1939TP_STATE_IDLE,
    J1939TP_STATE_BAM_TX,
    J1939TP_STATE_BAM_RX,
    J1939TP_STATE_RTS_TX,
    J1939TP_STATE_CTS_RX,
    J1939TP_STATE_DT_TX,
    J1939TP_STATE_DT_RX,
    J1939TP_STATE_EOM_ACK
} J1939Tp_StateType;

/**
 * @brief J1939 TP communication type
 */
typedef enum {
    J1939TP_BAM = 0,    /* Broadcast Announce Message */
    J1939TP_CTS,        /* RTS/CTS session */
    J1939TP_DIRECT      /* Direct transmission (single frame) */
} J1939Tp_CommunicationType;

/**
 * @brief Parameter type for TP layer
 */
typedef enum {
    J1939TP_PARAM_BROADCAST_TIME = 0,
    J1939TP_PARAM_BLOCK_SIZE,
    J1939TP_PARAM_T1,
    J1939TP_PARAM_T2,
    J1939TP_PARAM_T3,
    J1939TP_PARAM_T4,
    J1939TP_PARAM_N_Bs,
    J1939TP_PARAM_N_Cs,
    J1939TP_PARAM_N_Br,
    J1939TP_PARAM_N_Ar
} J1939Tp_ParameterType;

/**
 * @brief Parameter value type
 */
typedef struct {
    uint16 Value;
    boolean Valid;
} J1939Tp_ParameterValueType;

/**
 * @brief TP Connection configuration
 */
typedef struct {
    J1939Tp_SduType SduId;
    J1939Tp_CommunicationType ComType;
    uint8 BlockSize;
    uint16 T1Timeout;       /* T1 timeout in ms */
    uint16 T2Timeout;       /* T2 timeout in ms */
    uint16 T3Timeout;       /* T3 timeout in ms */
    uint16 T4Timeout;       /* T4 timeout in ms */
    PduIdType TxPduId;      /* TP.CM (Connection Management) PDU */
    PduIdType TxDtPduId;    /* TP.DT (Data Transfer) PDU */
    PduIdType RxPduId;      /* Reception PDU */
} J1939Tp_ConnectionConfigType;

/**
 * @brief PG (Parameter Group) configuration
 */
typedef struct {
    J1939Tp_PgType PgId;
    PduIdType PduId;
    boolean DirectNPdu;
    boolean PgIsVariable;
    uint16 PgLength;
    uint8 DirectSdu;
    uint8 MetaDataLength;
} J1939Tp_PgConfigType;

/**
 * @brief Tx channel runtime state
 */
typedef struct {
    J1939Tp_StateType State;
    uint8 SeqNumber;
    uint16 TotalBytes;
    uint16 SentBytes;
    uint16 PacketsToSend;
    uint16 PacketsSent;
    uint8 DestAddr;
    uint8 SrcAddr;
    uint32 Pgn;
    uint16 T1Timer;
    uint16 T3Timer;
    uint16 T4Timer;
    boolean BcTimerActive;
} J1939Tp_TxChannelType;

/**
 * @brief Rx channel runtime state
 */
typedef struct {
    J1939Tp_StateType State;
    uint8 SeqNumber;
    uint16 TotalBytes;
    uint16 ReceivedBytes;
    uint16 PacketsToReceive;
    uint16 PacketsReceived;
    uint8 SrcAddr;
    uint8 DestAddr;
    uint32 Pgn;
    uint16 T1Timer;
    uint16 T2Timer;
    uint16 NBrTimer;
} J1939Tp_RxChannelType;

/**
 * @brief J1939Tp configuration type
 */
typedef struct {
    uint8 ConnectionCount;
    const J1939Tp_ConnectionConfigType* Connections;
    uint8 PgCount;
    const J1939Tp_PgConfigType* PgConfigs;
} J1939Tp_ConfigType;

/*==================================================================================================
 *                                  Function Prototypes
 *================================================================================================*/

/**
 * @brief Initialize J1939Tp module
 * @param ConfigPtr Pointer to configuration structure
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType J1939Tp_Init(const J1939Tp_ConfigType* ConfigPtr);

/**
 * @brief Deinitialize J1939Tp module
 */
extern void J1939Tp_DeInit(void);

/**
 * @brief Get version information
 * @param VersionInfo Pointer to version info structure
 */
extern void J1939Tp_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/**
 * @brief Main function for periodic processing
 */
extern void J1939Tp_MainFunction(void);

/**
 * @brief Request transmission of a PDU
 * @param TxSduId SDU ID to transmit
 * @param TxInfoPtr PDU info structure
 * @return E_OK if accepted, E_NOT_OK otherwise
 */
extern Std_ReturnType J1939Tp_Transmit(
    PduIdType TxSduId,
    const PduInfoType* TxInfoPtr
);

/**
 * @brief Cancel an ongoing transmission
 * @param TxSduId SDU ID to cancel
 * @return E_OK if cancelled, E_NOT_OK otherwise
 */
extern Std_ReturnType J1939Tp_CancelTransmit(PduIdType TxSduId);

/**
 * @brief Cancel an ongoing reception
 * @param RxSduId SDU ID to cancel
 * @return E_OK if cancelled, E_NOT_OK otherwise
 */
extern Std_ReturnType J1939Tp_CancelReceive(PduIdType RxSduId);

/**
 * @brief Change a TP parameter
 * @param SduId SDU ID
 * @param Parameter Parameter to change
 * @param Value New value
 * @return E_OK if changed, E_NOT_OK otherwise
 */
extern Std_ReturnType J1939Tp_ChangeParameter(
    PduIdType SduId,
    J1939Tp_ParameterType Parameter,
    uint16 Value
);

/**
 * @brief PDU reception callback from CanIf
 */
extern void J1939Tp_RxIndication(
    PduIdType RxPduId,
    const PduInfoType* PduInfoPtr
);

/**
 * @brief Transmission confirmation callback from CanIf
 */
extern void J1939Tp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

#endif /* J1939TP_H */

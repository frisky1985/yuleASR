/*
 * LinTp.h
 *
 *  Created on: May 5, 2026
 *      Author: YuleTech
 *
 *  AUTOSAR LinTp (LIN Transport Protocol) API Header
 *  Based on ISO 17987-2 Transport Protocol
 *  Following AUTOSAR_SWS_LINTransportProtocol
 */

#ifndef LINTP_H
#define LINTP_H

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
#include "Std_Types.h"
#include "LinTp_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
 *                                         VERSION INFORMATION
 *================================================================================================*/
#define LINTP_VENDOR_ID                         0x0099U /* YuleTech Vendor ID */
#define LINTP_MODULE_ID                         0x0062U /* LinTp Module ID */

#define LINTP_SW_MAJOR_VERSION                  1U
#define LINTP_SW_MINOR_VERSION                  0U
#define LINTP_SW_PATCH_VERSION                  0U

#define LINTP_AR_RELEASE_MAJOR_VERSION          4U
#define LINTP_AR_RELEASE_MINOR_VERSION          4U
#define LINTP_AR_RELEASE_REVISION_VERSION       0U

/*==================================================================================================
 *                                         TYPE DEFINITIONS
 *================================================================================================*/

/* LinTp channel state type */
typedef enum
{
    LINTP_CH_IDLE = 0,              /* Channel is idle */
    LINTP_CH_BUSY,                  /* Channel is busy processing a message */
    LINTP_CH_TX_ACTIVE,             /* Channel is transmitting */
    LINTP_CH_RX_ACTIVE              /* Channel is receiving */
} LinTp_ChannelStateType;

/* LinTp internal state type */
typedef enum
{
    LINTP_STATE_UNINIT = 0,         /* Module not initialized */
    LINTP_STATE_INIT                /* Module initialized */
} LinTp_StateType;

/* LinTp transmission state machine */
typedef enum
{
    LINTP_TX_IDLE = 0,              /* No transmission active */
    LINTP_TX_SF,                    /* Transmitting Single Frame */
    LINTP_TX_FF,                    /* Transmitting First Frame */
    LINTP_TX_CF,                    /* Transmitting Consecutive Frame */
    LINTP_TX_WAIT_FC,               /* Waiting for Flow Control */
    LINTP_TX_WAIT_TX_CONFIRM,       /* Waiting for transmission confirmation */
    LINTP_TX_COMPLETED,             /* Transmission completed */
    LINTP_TX_ERROR                  /* Transmission error */
} LinTp_TxStateType;

/* LinTp reception state machine */
typedef enum
{
    LINTP_RX_IDLE = 0,              /* No reception active */
    LINTP_RX_WAIT_FF,               /* Waiting for First Frame */
    LINTP_RX_WAIT_CF,               /* Waiting for Consecutive Frame */
    LINTP_RX_SEND_FC,               /* Sending Flow Control */
    LINTP_RX_COMPLETED,             /* Reception completed */
    LINTP_RX_ERROR                  /* Reception error */
} LinTp_RxStateType;

/* PCI (Protocol Control Information) types */
typedef enum
{
    LINTP_PCI_SF = 0x00,            /* Single Frame */
    LINTP_PCI_FF = 0x10,            /* First Frame */
    LINTP_PCI_CF = 0x20,            /* Consecutive Frame */
    LINTP_PCI_FC = 0x30             /* Flow Control */
} LinTp_PciType;

/* Flow Control status */
typedef enum
{
    LINTP_FC_CTS = 0x00,            /* Continue To Send */
    LINTP_FC_WAIT = 0x01,           /* Wait */
    LINTP_FC_OVFLW = 0x02           /* Overflow/Abort */
} LinTp_FlowStatusType;

/* Parameter change type */
typedef enum
{
    LINTP_PARAMETER_BS = 0x01,      /* Block Size */
    LINTP_PARAMETER_STMIN = 0x02    /* Separation Time Minimum */
} LinTp_ParameterType;

/* Channel configuration type */
typedef struct
{
    uint16              ChannelId;              /* LinTp channel ID */
    uint16              LinIfChannelId;         /* Associated LinIf channel ID */
    uint32              N_As;                   /* Sender response timeout (ms) */
    uint32              N_Cs;                   /* Sender confirmation timeout (ms) */
    uint32              N_Cr;                   /* Receiver confirmation timeout (ms) */
    uint8               DefaultBs;              /* Default Block Size */
    uint8               DefaultStMin;           /* Default STmin (ms) */
    boolean             TransmitCancellation;   /* Transmit cancellation supported */
} LinTp_ChannelConfigType;

/* NSC (Network Service Connection) configuration */
typedef struct
{
    uint16              NsduId;                 /* N-SDU identifier */
    uint16              ChannelId;              /* Associated channel ID */
    uint16              PduRNSduId;             /* PduR N-SDU ID */
    uint8               Address;                /* Network address (NSA) */
    boolean             IsTx;                   /* TRUE = Tx connection, FALSE = Rx connection */
} LinTp_NsduConfigType;

/* Channel runtime data */
typedef struct
{
    LinTp_ChannelStateType  State;              /* Channel state */
    LinTp_TxStateType       TxState;            /* Transmission sub-state */
    LinTp_RxStateType       RxState;            /* Reception sub-state */
    uint16                  CurrentNsduId;      /* Current N-SDU being processed */
    PduLengthType           DataLength;         /* Total data length */
    PduLengthType           DataIndex;          /* Current data index */
    PduLengthType           BufferSize;         /* Available buffer size */
    uint8                   SequenceNumber;     /* Consecutive frame sequence number */
    uint8                   BlockSize;          /* Current block size */
    uint8                   BlockCount;         /* CF frames sent in current block */
    uint8                   StMin;              /* Separation time minimum */
    uint8                   WftCount;           /* Wait frame transmission count */
    uint32                  TimeoutCounter;     /* Timeout counter */
    boolean                 BufferProvided;     /* Buffer provided by PduR */
    uint8                   *TxBuffer;          /* Transmit buffer pointer */
    uint8                   *RxBuffer;          /* Receive buffer pointer */
} LinTp_ChannelRuntimeType;

/* LinTp configuration type */
typedef struct
{
    const LinTp_ChannelConfigType   *ChannelConfig;     /* Channel configurations */
    const LinTp_NsduConfigType      *NsduConfig;        /* N-SDU configurations */
    uint8                           ChannelCount;       /* Number of channels */
    uint8                           NsduCount;          /* Number of N-SDUs */
} LinTp_ConfigType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *================================================================================================*/

/* Initialization and shutdown */
void LinTp_Init(const LinTp_ConfigType *ConfigPtr);
void LinTp_DeInit(void);

/* Transmission functions */
Std_ReturnType LinTp_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);
#if (LINTP_CANCEL_TRANSMIT_API == STD_ON)
Std_ReturnType LinTp_CancelTransmit(PduIdType TxPduId);
#endif

/* Reception functions */
#if (LINTP_CANCEL_RECEIVE_API == STD_ON)
Std_ReturnType LinTp_CancelReceive(PduIdType RxPduId);
#endif

/* Parameter management */
#if (LINTP_CHANGE_PARAMETER_API == STD_ON)
Std_ReturnType LinTp_ChangeParameter(PduIdType PduId, LinTp_ParameterType Parameter, uint16 Value);
#endif

/* Version info */
#if (LINTP_VERSION_INFO_API == STD_ON)
void LinTp_GetVersionInfo(Std_VersionInfoType *VersionInfo);
#endif

/* Main function - cyclic processing */
void LinTp_MainFunction(void);

/* Callback functions from LinIf */
void LinTp_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void LinTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
Std_ReturnType LinTp_TriggerTransmit(PduIdType TxPduId, PduInfoType *PduInfoPtr);

/* Internal functions */
static void LinTp_ProcessTxStateMachine(uint8 ChannelIdx);
static void LinTp_ProcessRxStateMachine(uint8 ChannelIdx);
static void LinTp_SendFlowControl(uint8 ChannelIdx, LinTp_FlowStatusType FlowStatus);
static void LinTp_ProcessSingleFrame(uint8 ChannelIdx, const PduInfoType *PduInfoPtr);
static void LinTp_ProcessFirstFrame(uint8 ChannelIdx, const PduInfoType *PduInfoPtr);
static void LinTp_ProcessConsecutiveFrame(uint8 ChannelIdx, const PduInfoType *PduInfoPtr);
static void LinTp_ProcessFlowControl(uint8 ChannelIdx, const PduInfoType *PduInfoPtr);
static Std_ReturnType LinTp_CopyTxData(uint8 ChannelIdx);
static Std_ReturnType LinTp_ProvideRxBuffer(uint8 ChannelIdx);
static void LinTp_AbortTransmission(uint8 ChannelIdx);
static void LinTp_AbortReception(uint8 ChannelIdx);

/*==================================================================================================
 *                                         EXTERNAL DECLARATIONS
 *================================================================================================*/
#define LINTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const LinTp_ConfigType LinTp_Config;

#define LINTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

#endif /* LINTP_H */

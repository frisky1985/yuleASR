/******************************************************************************
 * @file    isotp_types.h
 * @brief   ISO 15765-2 IsoTp Transport Layer Type Definitions
 *
 * ISO 15765-2:2016 compliant
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ISOTP_TYPES_H
#define ISOTP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * IsoTp Module Version Information
 ******************************************************************************/
#define ISOTP_VENDOR_ID                 0x01U
#define ISOTP_MODULE_ID                 0x32U  /* IsoTp module ID per AUTOSAR */
#define ISOTP_SW_MAJOR_VERSION          1U
#define ISOTP_SW_MINOR_VERSION          0U
#define ISOTP_SW_PATCH_VERSION          0U

/******************************************************************************
 * IsoTp Protocol Constants (ISO 15765-2:2016)
 ******************************************************************************/

/* N_PCI Type Values */
#define ISOTP_N_PCI_SF                  0x00U  /* Single Frame */
#define ISOTP_N_PCI_FF                  0x10U  /* First Frame */
#define ISOTP_N_PCI_CF                  0x20U  /* Consecutive Frame */
#define ISOTP_N_PCI_FC                  0x30U  /* Flow Control */

/* N_PCI Mask */
#define ISOTP_N_PCI_MASK                0xF0U
#define ISOTP_N_PCI_TYPE_MASK           0xF0U
#define ISOTP_N_PCI_DL_MASK             0x0FU
#define ISOTP_N_PCI_SN_MASK             0x0FU
#define ISOTP_N_PCI_FS_MASK             0x0FU

/* Flow Status Values */
#define ISOTP_FS_CTS                    0x00U  /* Continue to Send */
#define ISOTP_FS_WT                     0x01U  /* Wait */
#define ISOTP_FS_OVFLW                  0x02U  /* Overflow */

/* Addressing Mode Types (ISO 15765-2:2016 Section 7) */
typedef enum {
    ISOTP_NORMAL_ADDRESSING = 0,         /* Standard addressing (CAN ID only) */
    ISOTP_EXTENDED_ADDRESSING,           /* Extended addressing (N_TA in first byte) */
    ISOTP_NORMAL_FIXED_ADDRESSING,       /* Normal fixed addressing (29-bit CAN ID) */
    ISOTP_MIXED_11BIT_ADDRESSING,        /* Mixed 11-bit addressing */
    ISOTP_MIXED_29BIT_ADDRESSING         /* Mixed 29-bit addressing */
} Isotp_AddressingModeType;

/* CAN Frame Types */
typedef enum {
    ISOTP_CAN_20 = 0,                    /* CAN 2.0 (8 bytes max) */
    ISOTP_CAN_FD                         /* CAN FD (64 bytes max) */
} Isotp_CanFrameType;

/******************************************************************************
 * Buffer and Timing Constants
 ******************************************************************************/

/* Default CAN Frame Sizes */
#define ISOTP_CAN20_FRAME_SIZE          8U
#define ISOTP_CANFD_MAX_FRAME_SIZE      64U
#define ISOTP_CANFD_DEFAULT_FRAME_SIZE  64U

/* Single Frame Data Length Limits */
#define ISOTP_SF_DL_CAN20_NORMAL        7U   /* Normal addressing: 7 bytes */
#define ISOTP_SF_DL_CAN20_EXTENDED      6U   /* Extended addressing: 6 bytes */
#define ISOTP_SF_DL_CANFD_NORMAL        62U  /* CAN FD normal: 62 bytes */
#define ISOTP_SF_DL_CANFD_EXTENDED      61U  /* CAN FD extended: 61 bytes */

/* First Frame Data Length */
#define ISOTP_FF_DL_CAN20_NORMAL        6U   /* First 6 bytes in FF */
#define ISOTP_FF_DL_CAN20_EXTENDED      5U   /* First 5 bytes in extended FF */
#define ISOTP_FF_DL_CANFD_NORMAL        62U  /* First 62 bytes in CAN FD FF */
#define ISOTP_FF_DL_CANFD_EXTENDED      61U  /* First 61 bytes in CAN FD extended FF */

/* Max Data Length per ISO 15765-2 */
#define ISOTP_MAX_MESSAGE_LENGTH        4095U

/* Default Timing Values (milliseconds) */
#define ISOTP_DEFAULT_N_BS              1000U  /* N_Bs timeout */
#define ISOTP_DEFAULT_N_BR              1000U  /* N_Br timeout */
#define ISOTP_DEFAULT_N_CS              1000U  /* N_Cs timeout */
#define ISOTP_DEFAULT_N_CR              1000U  /* N_Cr timeout */
#define ISOTP_DEFAULT_STMIN             20U    /* Default STmin (20ms) */
#define ISOTP_DEFAULT_BS                8U     /* Default Block Size */

/******************************************************************************
 * IsoTp Protocol Control Information (N_PCI)
 ******************************************************************************/

typedef struct {
    uint8_t pciType;                     /* N_PCI type (SF/FF/CF/FC) */
    uint8_t seqNum;                      /* Sequence Number (CF only) */
    uint8_t flowStatus;                  /* Flow Status (FC only) */
    uint8_t blockSize;                   /* Block Size (FC only) */
    uint8_t stMin;                       /* Separation Time Minimum (FC only) */
    uint16_t dataLength;                 /* Complete data length */
} Isotp_NpciType;

/******************************************************************************
 * IsoTp Return Types
 ******************************************************************************/

typedef enum {
    ISOTP_E_OK = 0x00U,                  /* Operation successful */
    ISOTP_E_NOT_OK = 0x01U,              /* General error */
    ISOTP_E_BUSY = 0x02U,                /* Transmission/reception in progress */
    ISOTP_E_TIMEOUT = 0x03U,              /* Timeout occurred */
    ISOTP_E_BUFFER_OVERFLOW = 0x04U,     /* Buffer overflow */
    ISOTP_E_INVALID_PARAMETER = 0x05U,   /* Invalid parameter */
    ISOTP_E_INVALID_FRAME = 0x06U,       /* Invalid frame received */
    ISOTP_E_SEQUENCE_ERROR = 0x07U,      /* Sequence number error */
    ISOTP_E_UNEXPECTED_FRAME = 0x08U,    /* Unexpected frame type */
    ISOTP_E_WFT_OVRN = 0x09U,            /* Wait frame overrun */
    ISOTP_E_NO_BUFFER = 0x0AU,           /* No buffer available */
    ISOTP_E_CANCELLED = 0x0BU            /* Operation cancelled */
} Isotp_ReturnType;

/******************************************************************************
 * IsoTp Connection States
 ******************************************************************************/

typedef enum {
    ISOTP_STATE_IDLE = 0x00U,            /* Idle state */
    ISOTP_STATE_TX_READY,                /* TX ready */
    ISOTP_STATE_TX_WAIT_FC,              /* TX waiting for Flow Control */
    ISOTP_STATE_TX_SEND_CF,              /* TX sending Consecutive Frames */
    ISOTP_STATE_TX_WAIT_STMIN,           /* TX waiting for STmin */
    ISOTP_STATE_TX_DONE,                 /* TX completed */
    ISOTP_STATE_TX_ERROR,                /* TX error */
    ISOTP_STATE_RX_WAIT_FF,              /* RX waiting for First Frame */
    ISOTP_STATE_RX_WAIT_FC_SENT,         /* RX sent FC, waiting for CF */
    ISOTP_STATE_RX_WAIT_CF,              /* RX waiting for Consecutive Frame */
    ISOTP_STATE_RX_DONE,                 /* RX completed */
    ISOTP_STATE_RX_ERROR                 /* RX error */
} Isotp_StateType;

/******************************************************************************
 * IsoTp Addressing Information (N_AI)
 ******************************************************************************/

typedef struct {
    uint16_t sourceAddress;              /* N_SA - Source Address */
    uint16_t targetAddress;              /* N_TA - Target Address */
    uint8_t targetAddressType;           /* N_TAtype (physical/functional) */
    uint8_t networkLayerAddress;         /* N_AE - Network Layer Address */
    Isotp_AddressingModeType addrMode;   /* Addressing mode */
    uint32_t txCanId;                    /* TX CAN ID */
    uint32_t rxCanId;                    /* RX CAN ID */
} Isotp_NaiType;

/******************************************************************************
 * IsoTp Timing Configuration
 ******************************************************************************/

typedef struct {
    uint16_t nAs;                        /* N_As - Sender address timeout */
    uint16_t nAr;                        /* N_Ar - Receiver address timeout */
    uint16_t nBs;                        /* N_Bs - Sender block timeout */
    uint16_t nBr;                        /* N_Br - Receiver block timeout */
    uint16_t nCs;                        /* N_Cs - Sender consecutive frame timeout */
    uint16_t nCr;                        /* N_Cr - Receiver consecutive frame timeout */
} Isotp_TimingConfigType;

/******************************************************************************
 * IsoTp Connection Configuration
 ******************************************************************************/

typedef struct {
    uint8_t channelId;                   /* Channel ID */
    Isotp_NaiType addressing;            /* Addressing information */
    Isotp_TimingConfigType timing;       /* Timing configuration */
    Isotp_CanFrameType canFrameType;     /* CAN 2.0 or CAN FD */
    uint16_t rxBufferSize;               /* RX buffer size */
    uint16_t txBufferSize;               /* TX buffer size */
    bool functionalAddressingEnabled;    /* Functional addressing enabled */
} Isotp_ChannelConfigType;

/******************************************************************************
 * IsoTp Buffer Types
 ******************************************************************************/

typedef struct {
    uint8_t *data;                       /* Buffer data pointer */
    uint16_t size;                       /* Buffer total size */
    uint16_t writePos;                   /* Current write position */
    uint16_t readPos;                    /* Current read position */
    uint16_t dataLength;                 /* Current data length */
    bool locked;                         /* Buffer locked flag */
} Isotp_BufferType;

/******************************************************************************
 * IsoTp Connection Runtime
 ******************************************************************************/

typedef struct {
    uint8_t channelId;                   /* Channel ID */
    Isotp_StateType state;               /* Current state */
    Isotp_StateType nextState;           /* Next state */
    
    /* TX context */
    uint8_t *txBuffer;                   /* TX data buffer */
    uint16_t txLength;                   /* Total TX data length */
    uint16_t txSentLength;               /* Bytes already sent */
    uint8_t txSeqNum;                    /* Current TX sequence number */
    uint8_t txBlockSize;                 /* Block size for CF */
    uint8_t txStMin;                     /* STmin value */
    uint16_t txBlockCount;               /* Current block count */
    uint32_t txStartTime;                /* TX start timestamp */
    uint32_t txWaitStartTime;            /* Wait start timestamp */
    
    /* RX context */
    uint8_t *rxBuffer;                   /* RX data buffer */
    uint16_t rxLength;                   /* Expected RX data length */
    uint16_t rxReceivedLength;           /* Bytes already received */
    uint8_t rxSeqNum;                    /* Expected RX sequence number */
    uint8_t rxBlockSize;                 /* Block size for FC */
    uint8_t rxStMin;                     /* STmin for FC response */
    uint16_t rxBlockCount;               /* Current block count */
    uint32_t rxStartTime;                /* RX start timestamp */
    uint32_t rxWaitStartTime;            /* Wait start timestamp */
    uint8_t rxWftCounter;                /* Wait frame transmission counter */
    
    /* Callback context */
    void *userData;                      /* User data pointer */
} Isotp_ChannelContextType;

/******************************************************************************
 * IsoTp Callback Function Types
 ******************************************************************************/

/* Transmit confirmation callback */
typedef void (*Isotp_TxConfirmationCallback)(
    uint8_t channelId,
    Isotp_ReturnType result,
    void *userData
);

/* Reception indication callback */
typedef void (*Isotp_RxIndicationCallback)(
    uint8_t channelId,
    uint8_t *data,
    uint16_t length,
    void *userData
);

/* Flow control request callback */
typedef Isotp_ReturnType (*Isotp_FlowControlCallback)(
    uint8_t channelId,
    uint16_t dataLength,
    uint8_t *blockSize,
    uint8_t *stMin,
    void *userData
);

/* Frame transmit callback (to CAN interface) */
typedef Isotp_ReturnType (*Isotp_CanTransmitCallback)(
    uint32_t canId,
    uint8_t *data,
    uint8_t length,
    bool isFd,
    void *userData
);

/******************************************************************************
 * IsoTp Module Context
 ******************************************************************************/

typedef struct {
    bool initialized;                    /* Initialization flag */
    
    /* Channel configurations */
    const Isotp_ChannelConfigType *channelConfigs;
    uint8_t numChannels;                 /* Number of channels */
    
    /* Channel runtime contexts */
    Isotp_ChannelContextType *channelContexts;
    
    /* Callbacks */
    Isotp_TxConfirmationCallback txConfirmationCallback;
    Isotp_RxIndicationCallback rxIndicationCallback;
    Isotp_FlowControlCallback flowControlCallback;
    Isotp_CanTransmitCallback canTransmitCallback;
    
} Isotp_ContextType;

/******************************************************************************
 * IsoTp Constants
 ******************************************************************************/

#define ISOTP_MIN_STMIN_MS              0x01U   /* Minimum STmin in ms */
#define ISOTP_MAX_STMIN_MS              0x7FU   /* Maximum STmin in ms */
#define ISOTP_MIN_STMIN_US              0xF1U   /* Minimum STmin in us (100us) */
#define ISOTP_MAX_STMIN_US              0xF9U   /* Maximum STmin in us (900us) */

#define ISOTP_MAX_BLOCK_SIZE            0xFFU   /* Maximum block size */
#define ISOTP_MAX_WFT                   0xFFU   /* Maximum wait frames */
#define ISOTP_DEFAULT_WFT_MAX           10U     /* Default max wait frames */

#ifdef __cplusplus
}
#endif

#endif /* ISOTP_TYPES_H */

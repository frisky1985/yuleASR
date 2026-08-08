/******************************************************************************
 * @file    docan_types.h
 * @brief   DoCAN (Diagnostic Communication over CAN) Types Definition
 *
 * ISO 15765-2:2016 compliant implementation
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DOCAN_TYPES_H
#define DOCAN_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define DOCAN_VENDOR_ID                 0x0001U
#define DOCAN_MODULE_ID                 0x56U
#define DOCAN_SW_MAJOR_VERSION          1U
#define DOCAN_SW_MINOR_VERSION          0U
#define DOCAN_SW_PATCH_VERSION          0U

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/
#define DOCAN_MAX_CONNECTIONS           4U      /**< Maximum parallel connections */
#define DOCAN_MAX_FRAME_LENGTH          64U     /**< CAN FD max data length */
#define DOCAN_MAX_STD_FRAME_LENGTH      8U      /**< Standard CAN max data length */
#define DOCAN_MAX_MESSAGE_LENGTH        4095U   /**< Max ISO-TP message length (12-bit) */
#define DOCAN_MAX_EXT_MESSAGE_LENGTH    0xFFFFFFFFU  /**< Extended addressing max length */

/* Protocol Control Information (PCI) sizes */
#define DOCAN_PCI_SF_STD_SIZE           1U      /**< Single Frame PCI size */
#define DOCAN_PCI_FF_STD_SIZE           2U      /**< First Frame PCI size */
#define DOCAN_PCI_CF_SIZE               1U      /**< Consecutive Frame PCI size */
#define DOCAN_PCI_FC_SIZE               3U      /**< Flow Control PCI size */

/******************************************************************************
 * Frame Types (ISO 15765-2:2016)
 ******************************************************************************/
typedef enum {
    DOCAN_FRAME_TYPE_SF = 0x00U,        /**< Single Frame */
    DOCAN_FRAME_TYPE_FF = 0x10U,        /**< First Frame */
    DOCAN_FRAME_TYPE_CF = 0x20U,        /**< Consecutive Frame */
    DOCAN_FRAME_TYPE_FC = 0x30U         /**< Flow Control Frame */
} DoCan_FrameTypeType;

/******************************************************************************
 * Flow Control Status
 ******************************************************************************/
typedef enum {
    DOCAN_FC_STATUS_CTS = 0x00U,        /**< Continue To Send */
    DOCAN_FC_STATUS_WT  = 0x01U,        /**< Wait */
    DOCAN_FC_STATUS_OVFLW = 0x02U       /**< Overflow (buffer overflow) */
} DoCan_FlowStatusType;

/******************************************************************************
 * Addressing Modes (ISO 15765-2:2016 Section 7.3)
 ******************************************************************************/
typedef enum {
    DOCAN_ADDRESSING_NORMAL = 0x00U,    /**< Normal addressing */
    DOCAN_ADDRESSING_EXTENDED,          /**< Extended addressing */
    DOCAN_ADDRESSING_MIXED,             /**< Mixed 11-bit addressing */
    DOCAN_ADDRESSING_MIXED_29BIT,       /**< Mixed 29-bit addressing */
    DOCAN_ADDRESSING_NORMAL_FIXED       /**< Normal fixed addressing */
} DoCan_AddressingModeType;

/******************************************************************************
 * CAN ID Types
 ******************************************************************************/
typedef enum {
    DOCAN_CAN_ID_TYPE_STANDARD = 0x00U, /**< 11-bit CAN ID */
    DOCAN_CAN_ID_TYPE_EXTENDED          /**< 29-bit CAN ID */
} DoCan_CanIdTypeType;

/******************************************************************************
 * CAN Frame Types (Classic vs FD)
 ******************************************************************************/
typedef enum {
    DOCAN_CAN_FRAME_CLASSIC = 0x00U,    /**< Classic CAN (max 8 bytes) */
    DOCAN_CAN_FRAME_FD                  /**< CAN FD (max 64 bytes) */
} DoCan_CanFrameType;

/******************************************************************************
 * Connection States
 ******************************************************************************/
typedef enum {
    DOCAN_CONN_STATE_IDLE = 0x00U,      /**< Idle, no active transfer */
    DOCAN_CONN_STATE_TX_WAIT_FC,        /**< TX: Waiting for Flow Control */
    DOCAN_CONN_STATE_TX_CF,             /**< TX: Sending Consecutive Frames */
    DOCAN_CONN_STATE_TX_WAIT_CONFIRM,   /**< TX: Waiting for transmission confirmation */
    DOCAN_CONN_STATE_RX_WAIT_FF,        /**< RX: Waiting for First Frame */
    DOCAN_CONN_STATE_RX_WAIT_CF,        /**< RX: Waiting for Consecutive Frames */
    DOCAN_CONN_STATE_RX_SEND_FC,        /**< RX: Sending Flow Control */
    DOCAN_CONN_STATE_ERROR              /**< Error state */
} DoCan_ConnectionStateType;

/******************************************************************************
 * Return Types
 ******************************************************************************/
typedef enum {
    DOCAN_OK = 0x00U,
    DOCAN_E_NOT_OK = 0x01U,
    DOCAN_E_PARAM_POINTER = 0x02U,
    DOCAN_E_PARAM_LENGTH = 0x03U,
    DOCAN_E_PARAM_CONFIG = 0x04U,
    DOCAN_E_CONN_BUSY = 0x05U,
    DOCAN_E_CONN_NOT_FOUND = 0x06U,
    DOCAN_E_TIMEOUT = 0x07U,
    DOCAN_E_BUFFER_OVERRUN = 0x08U,
    DOCAN_E_INVALID_FRAME = 0x09U,
    DOCAN_E_SEQUENCE_ERROR = 0x0AU,
    DOCAN_E_WFT_OVERRUN = 0x0BU,        /**< Wait frame count exceeded */
    DOCAN_E_NO_BUFFER = 0x0CU           /**< No buffer available */
} DoCan_ReturnType;

/******************************************************************************
 * Flow Control Parameters
 ******************************************************************************/
typedef struct {
    uint8_t BlockSize;                  /**< BS: Number of CFs before next FC (0=unlimited) */
    uint8_t STmin;                      /**< STmin: Minimum separation time in ms */
                                        /**< 0x00-0x7F: 0-127ms */
                                        /**< 0xF1-0xF9: 0.1-0.9ms (100us-900us) */
    uint8_t MaxWaitFrames;              /**< Maximum number of WAIT FC frames */
} DoCan_FlowControlParamsType;

/******************************************************************************
 * CAN Address Information
 ******************************************************************************/
typedef struct {
    uint32_t TxCanId;                   /**< CAN ID for transmission */
    uint32_t RxCanId;                   /**< CAN ID for reception */
    DoCan_CanIdTypeType CanIdType;      /**< Standard (11-bit) or Extended (29-bit) */
    DoCan_AddressingModeType AddressingMode;  /**< Addressing mode */
    uint8_t Ta;                         /**< Target Address (for extended/mixed) */
    uint8_t Sa;                         /**< Source Address (for extended/mixed) */
    uint8_t Ae;                         /**< Address Extension (for mixed) */
} DoCan_AddressInfoType;

/******************************************************************************
 * Timeout Configuration (ISO 15765-2:2016 Section 9.2)
 ******************************************************************************/
typedef struct {
    uint16_t As;                        /**< Sender timeout for transmission */
    uint16_t Ar;                        /**< Receiver timeout for transmission */
    uint16_t Bs;                        /**< Sender timeout waiting for FC */
    uint16_t Br;                        /**< Receiver timeout before sending FC */
    uint16_t Cs;                        /**< Sender timeout between CFs */
    uint16_t Cr;                        /**< Receiver timeout waiting for CF */
    uint16_t N_WFTmax;                  /**< Maximum number of WAIT frames */
} DoCan_TimeoutConfigType;

/******************************************************************************
 * CAN Frame Structure
 ******************************************************************************/
typedef struct {
    uint32_t CanId;                     /**< CAN ID */
    DoCan_CanIdTypeType CanIdType;      /**< Standard or Extended */
    DoCan_CanFrameType FrameType;       /**< Classic CAN or CAN FD */
    uint8_t DataLength;                 /**< DLC (actual data bytes) */
    uint8_t Data[DOCAN_MAX_FRAME_LENGTH];  /**< Frame data */
} DoCan_CanFrameInfoType;

/******************************************************************************
 * PDU (Protocol Data Unit) Information
 ******************************************************************************/
typedef struct {
    uint8_t *SduDataPtr;                /**< Pointer to data buffer */
    uint8_t *MetaDataPtr;               /**< Pointer to metadata (for CAN FD) */
    uint16_t SduLength;                 /**< SDU length */
} DoCan_PduInfoType;

/******************************************************************************
 * Connection Configuration
 ******************************************************************************/
typedef struct {
    uint8_t ConnectionId;               /**< Connection identifier */
    DoCan_AddressInfoType AddressInfo;  /**< Address information */
    DoCan_FlowControlParamsType DefaultFcParams;  /**< Default FC parameters */
    DoCan_TimeoutConfigType Timeouts;   /**< Timeout configuration */
    uint16_t BufferSize;                /**< Maximum buffer size */
    DoCan_CanFrameType CanFrameType;    /**< Classic CAN or CAN FD support */
} DoCan_ConnectionConfigType;

/******************************************************************************
 * Connection Runtime State
 ******************************************************************************/
typedef struct {
    uint8_t ConnectionId;               /**< Connection identifier */
    DoCan_ConnectionStateType State;    /**< Current state */
    
    /* Transfer information */
    uint32_t TransferTotalLength;       /**< Total message length */
    uint32_t TransferCurrentPos;        /**< Current position in transfer */
    uint16_t TransferNextSequenceNum;   /**< Next expected/used sequence number */
    
    /* Flow control state */
    uint8_t CurrentBlockSize;           /**< Remaining CFs before next FC needed */
    uint8_t CurrentSTmin;               /**< Current separation time */
    uint8_t WaitFrameCount;             /**< Number of WAIT frames received/sent */
    
    /* Timeout tracking */
    uint32_t TimeoutDeadline;           /**< Timeout deadline timestamp */
    
    /* Buffer management */
    uint8_t *BufferPtr;                 /**< Pointer to message buffer */
    uint16_t BufferSize;                /**< Buffer size */
    uint16_t BufferPos;                 /**< Current buffer position */
    
    /* Flags */
    boolean IsTx;                       /**< TRUE if transmitting, FALSE if receiving */
    boolean IsCanFd;                    /**< TRUE if using CAN FD */
} DoCan_ConnectionInfoType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

/** 
 * @brief Callback for sending CAN frame 
 * @param CanId CAN identifier
 * @param Frame Pointer to CAN frame data
 * @param Length Frame data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
typedef Std_ReturnType (*DoCan_CanTxCallbackType)(
    uint32_t CanId,
    const uint8_t *Frame,
    uint8_t Length,
    DoCan_CanFrameType FrameType
);

/**
 * @brief Callback for receiving indication
 * @param ConnectionId Connection identifier
 * @param DataPtr Pointer to received data
 * @param Length Data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
typedef Std_ReturnType (*DoCan_RxIndicationCallbackType)(
    uint8_t ConnectionId,
    const uint8_t *DataPtr,
    uint32_t Length
);

/**
 * @brief Callback for transmission confirmation
 * @param ConnectionId Connection identifier
 * @param Result Transmission result
 */
typedef void (*DoCan_TxConfirmationCallbackType)(
    uint8_t ConnectionId,
    Std_ReturnType Result
);

/**
 * @brief Callback for getting current time in milliseconds
 * @return Current time in milliseconds
 */
typedef uint32_t (*DoCan_GetTimeMsCallbackType)(void);

/**
 * @brief Callback for buffer request
 * @param ConnectionId Connection identifier
 * @param Length Required buffer length
 * @param BufferPtr Output: pointer to buffer
 * @return E_OK if buffer available, E_NOT_OK otherwise
 */
typedef Std_ReturnType (*DoCan_BufferRequestCallbackType)(
    uint8_t ConnectionId,
    uint32_t Length,
    uint8_t **BufferPtr
);

/******************************************************************************
 * Module Configuration
 ******************************************************************************/
typedef struct {
    uint8_t NumConnections;                                     /**< Number of connections */
    const DoCan_ConnectionConfigType *ConnectionConfigs;        /**< Connection configurations */
    DoCan_CanTxCallbackType CanTxCallback;                      /**< CAN TX callback */
    DoCan_RxIndicationCallbackType RxIndicationCallback;        /**< RX indication callback */
    DoCan_TxConfirmationCallbackType TxConfirmationCallback;    /**< TX confirmation callback */
    DoCan_GetTimeMsCallbackType GetTimeMsCallback;              /**< Get time callback */
    DoCan_BufferRequestCallbackType BufferRequestCallback;      /**< Buffer request callback */
} DoCan_ConfigType;

/******************************************************************************
 * Protocol Constants
 ******************************************************************************/

/* PCI Nibble masks */
#define DOCAN_PCI_TYPE_MASK             0xF0U
#define DOCAN_PCI_SF_DL_MASK            0x0FU
#define DOCAN_PCI_FF_DL_UPPER_MASK      0x0FU
#define DOCAN_PCI_CF_SN_MASK            0x0FU
#define DOCAN_PCI_FC_FS_MASK            0x0FU

/* Frame type shifts */
#define DOCAN_PCI_SF                    0x00U   /**< Single Frame */
#define DOCAN_PCI_FF                    0x10U   /**< First Frame */
#define DOCAN_PCI_CF                    0x20U   /**< Consecutive Frame */
#define DOCAN_PCI_FC                    0x30U   /**< Flow Control */

/* Special STmin values (ISO 15765-2:2016 Section 9.2) */
#define DOCAN_STMIN_US_100              0xF1U   /**< 100 microseconds */
#define DOCAN_STMIN_US_200              0xF2U   /**< 200 microseconds */
#define DOCAN_STMIN_US_300              0xF3U   /**< 300 microseconds */
#define DOCAN_STMIN_US_400              0xF4U   /**< 400 microseconds */
#define DOCAN_STMIN_US_500              0xF5U   /**< 500 microseconds */
#define DOCAN_STMIN_US_600              0xF6U   /**< 600 microseconds */
#define DOCAN_STMIN_US_700              0xF7U   /**< 700 microseconds */
#define DOCAN_STMIN_US_800              0xF8U   /**< 800 microseconds */
#define DOCAN_STMIN_US_900              0xF9U   /**< 900 microseconds */

/* Sequence number range */
#define DOCAN_SN_MAX                    0x0FU   /**< Maximum sequence number */
#define DOCAN_SN_MASK                   0x0FU   /**< Sequence number mask */

/* Default timeouts (in milliseconds) */
#define DOCAN_DEFAULT_TIMEOUT_AS        1000U   /**< Default As timeout */
#define DOCAN_DEFAULT_TIMEOUT_AR        1000U   /**< Default Ar timeout */
#define DOCAN_DEFAULT_TIMEOUT_BS        1000U   /**< Default Bs timeout */
#define DOCAN_DEFAULT_TIMEOUT_BR        1000U   /**< Default Br timeout */
#define DOCAN_DEFAULT_TIMEOUT_CS        1000U   /**< Default Cs timeout */
#define DOCAN_DEFAULT_TIMEOUT_CR        1000U   /**< Default Cr timeout */

/* Default FC parameters */
#define DOCAN_DEFAULT_BLOCK_SIZE        8U      /**< Default block size */
#define DOCAN_DEFAULT_STMIN             0U      /**< Default STmin (0ms) */
#define DOCAN_DEFAULT_MAX_WFT           10U     /**< Default max WAIT frames */

#ifdef __cplusplus
}
#endif

#endif /* DOCAN_TYPES_H */

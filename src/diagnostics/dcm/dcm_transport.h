/******************************************************************************
 * @file    dcm_transport.h
 * @brief   DCM Transport Layer Abstraction Module
 *
 * Unified transport layer abstraction supporting multiple diagnostic protocols:
 * - DoIP (ISO 13400-2) - Diagnostic over IP
 * - DoCAN (ISO 15765-2) - Diagnostic over CAN
 * - IsoTp (ISO 15765-2) - ISO Transport Protocol
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_TRANSPORT_H
#define DCM_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "dcm_types.h"

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define DCM_TRANSPORT_VENDOR_ID                 0x01U
#define DCM_TRANSPORT_MODULE_ID                 0x37U
#define DCM_TRANSPORT_SW_MAJOR_VERSION          1U
#define DCM_TRANSPORT_SW_MINOR_VERSION          0U
#define DCM_TRANSPORT_SW_PATCH_VERSION          0U

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/
#define DCM_TRANSPORT_MAX_CHANNELS              8U      /* Max transport channels */
#define DCM_TRANSPORT_MAX_PROTOCOLS             3U      /* DoIP, DoCAN, IsoTp */
#define DCM_TRANSPORT_MAX_PRIORITY_LEVELS       8U      /* Priority levels 0-7 */
#define DCM_TRANSPORT_DEFAULT_PRIORITY          4U      /* Default priority level */
#define DCM_TRANSPORT_INVALID_CHANNEL_ID        0xFFU
#define DCM_TRANSPORT_INVALID_PROTOCOL_ID       0xFFU

/******************************************************************************
 * Transport Protocol Types
 ******************************************************************************/
typedef enum {
    DCM_TRANSPORT_PROTOCOL_NONE     = 0x00U,
    DCM_TRANSPORT_PROTOCOL_DOIP     = 0x01U,    /* ISO 13400-2 DoIP */
    DCM_TRANSPORT_PROTOCOL_DOCAN    = 0x02U,    /* ISO 15765-2 DoCAN */
    DCM_TRANSPORT_PROTOCOL_ISOTP    = 0x03U     /* ISO 15765-2 IsoTp */
} Dcm_TransportProtocolType;

/******************************************************************************
 * Transport Channel States
 ******************************************************************************/
typedef enum {
    DCM_TRANSPORT_STATE_UNINIT      = 0x00U,    /* Uninitialized */
    DCM_TRANSPORT_STATE_IDLE        = 0x01U,    /* Idle, ready for use */
    DCM_TRANSPORT_STATE_CONNECTING  = 0x02U,    /* Connection in progress */
    DCM_TRANSPORT_STATE_CONNECTED   = 0x03U,    /* Connected and active */
    DCM_TRANSPORT_STATE_TX_ACTIVE   = 0x04U,    /* Transmission in progress */
    DCM_TRANSPORT_STATE_RX_ACTIVE   = 0x05U,    /* Reception in progress */
    DCM_TRANSPORT_STATE_DISCONNECTING = 0x06U,  /* Disconnecting */
    DCM_TRANSPORT_STATE_ERROR       = 0x07U,    /* Error state */
    DCM_TRANSPORT_STATE_SUSPENDED   = 0x08U    /* Suspended temporarily */
} Dcm_TransportStateType;

/******************************************************************************
 * Transport Return Types
 ******************************************************************************/
typedef enum {
    DCM_TRANSPORT_OK                = 0x00U,
    DCM_TRANSPORT_ERROR             = 0x01U,
    DCM_TRANSPORT_BUSY              = 0x02U,
    DCM_TRANSPORT_TIMEOUT           = 0x03U,
    DCM_TRANSPORT_INVALID_PARAMETER = 0x04U,
    DCM_TRANSPORT_NOT_INITIALIZED   = 0x05U,
    DCM_TRANSPORT_NO_CHANNEL        = 0x06U,
    DCM_TRANSPORT_NOT_CONNECTED     = 0x07U,
    DCM_TRANSPORT_BUFFER_OVERFLOW   = 0x08U,
    DCM_TRANSPORT_MESSAGE_TOO_LARGE = 0x09U,
    DCM_TRANSPORT_PROTOCOL_ERROR    = 0x0AU,
    DCM_TRANSPORT_CHANNEL_SUSPENDED = 0x0BU,
    DCM_TRANSPORT_PRIORITY_BLOCKED  = 0x0CU
} Dcm_TransportReturnType;

/******************************************************************************
 * Transport Event Types
 ******************************************************************************/
typedef enum {
    DCM_TRANSPORT_EVT_CONNECT       = 0x01U,    /* Connection established */
    DCM_TRANSPORT_EVT_DISCONNECT    = 0x02U,    /* Connection closed */
    DCM_TRANSPORT_EVT_RX_COMPLETE   = 0x03U,    /* Reception complete */
    DCM_TRANSPORT_EVT_TX_COMPLETE   = 0x04U,    /* Transmission complete */
    DCM_TRANSPORT_EVT_ERROR         = 0x05U,    /* Error occurred */
    DCM_TRANSPORT_EVT_TIMEOUT       = 0x06U,    /* Timeout occurred */
    DCM_TRANSPORT_EVT_PRIORITY_CHANGED = 0x07U  /* Priority changed */
} Dcm_TransportEventType;

/******************************************************************************
 * Transport Channel Information
 ******************************************************************************/
typedef struct {
    uint8_t channelId;                          /* Channel identifier */
    Dcm_TransportProtocolType protocol;         /* Transport protocol type */
    Dcm_TransportStateType state;               /* Current state */
    uint8_t priority;                           /* Priority level (0-7, 0=highest) */
    uint16_t sourceAddress;                     /* Logical source address */
    uint16_t targetAddress;                     /* Logical target address */
    uint32_t rxBufferSize;                      /* RX buffer size */
    uint32_t txBufferSize;                      /* TX buffer size */
    bool isActive;                              /* Channel active flag */
    bool isDefault;                             /* Default channel flag */
    uint32_t connectionTime;                    /* Connection timestamp */
    uint32_t lastActivityTime;                  /* Last activity timestamp */
} Dcm_TransportChannelInfoType;

/******************************************************************************
 * Transport Data Message
 ******************************************************************************/
typedef struct {
    uint8_t *data;                              /* Data buffer */
    uint32_t length;                            /* Data length */
    uint16_t sourceAddress;                     /* Source address */
    uint16_t targetAddress;                     /* Target address */
    uint8_t priority;                           /* Message priority */
    Dcm_AddressingMode addrMode;                /* Addressing mode */
    uint32_t timestamp;                         /* Timestamp */
} Dcm_TransportMessageType;

/******************************************************************************
 * Transport Configuration
 ******************************************************************************/
typedef struct {
    Dcm_TransportProtocolType protocol;         /* Protocol type */
    uint8_t priority;                           /* Priority level (0-7) */
    uint16_t sourceAddress;                     /* Source address */
    uint16_t targetAddress;                     /* Target address */
    uint32_t rxBufferSize;                      /* RX buffer size */
    uint32_t txBufferSize;                      /* TX buffer size */
    bool isDefault;                             /* Is default channel */
    uint32_t connectionTimeout;                 /* Connection timeout (ms) */
    uint32_t transmissionTimeout;               /* Transmission timeout (ms) */
    
    /* Protocol-specific configuration pointers */
    const void *protocolConfig;                 /* Protocol-specific config */
} Dcm_TransportChannelConfigType;

typedef struct {
    const Dcm_TransportChannelConfigType *channelConfigs;
    uint8_t numChannels;
    bool autoProtocolSelection;                 /* Automatic protocol selection */
    uint8_t defaultProtocolPriority;            /* Default protocol priority */
} Dcm_TransportConfigType;

/******************************************************************************
 * Transport Statistics
 ******************************************************************************/
typedef struct {
    uint32_t messagesTransmitted;
    uint32_t messagesReceived;
    uint32_t bytesTransmitted;
    uint32_t bytesReceived;
    uint32_t txErrors;
    uint32_t rxErrors;
    uint32_t timeouts;
    uint32_t connectionsEstablished;
    uint32_t connectionsClosed;
    uint32_t retransmissions;
    uint32_t priorityPreemptions;
} Dcm_TransportStatisticsType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

/**
 * @brief Transport message received callback
 * @param channelId Channel identifier
 * @param message Received message
 * @return DCM_TRANSPORT_OK on success
 */
typedef Dcm_TransportReturnType (*Dcm_TransportRxCallbackType)(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
);

/**
 * @brief Transport message transmission confirmation callback
 * @param channelId Channel identifier
 * @param result Transmission result
 */
typedef void (*Dcm_TransportTxConfirmationType)(
    uint8_t channelId,
    Dcm_TransportReturnType result
);

/**
 * @brief Transport event notification callback
 * @param channelId Channel identifier
 * @param event Event type
 * @param eventData Optional event data
 */
typedef void (*Dcm_TransportEventCallbackType)(
    uint8_t channelId,
    Dcm_TransportEventType event,
    const void *eventData
);

/**
 * @brief Protocol-specific send function type
 */
typedef Dcm_TransportReturnType (*Dcm_TransportSendFuncType)(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
);

/**
 * @brief Protocol-specific receive handler type
 */
typedef Dcm_TransportReturnType (*Dcm_TransportReceiveFuncType)(
    uint8_t channelId,
    Dcm_TransportMessageType *message
);

/**
 * @brief Protocol-specific status query function type
 */
typedef Dcm_TransportStateType (*Dcm_TransportStatusFuncType)(
    uint8_t channelId
);

/******************************************************************************
 * Protocol Interface Structure
 ******************************************************************************/
typedef struct {
    Dcm_TransportProtocolType protocol;         /* Protocol type */
    const char *protocolName;                   /* Protocol name string */
    
    /* Protocol operations */
    Dcm_TransportReturnType (*init)(const void *config);
    Dcm_TransportReturnType (*deinit)(void);
    Dcm_TransportReturnType (*connect)(uint8_t channelId);
    Dcm_TransportReturnType (*disconnect)(uint8_t channelId);
    Dcm_TransportSendFuncType send;
    Dcm_TransportReceiveFuncType receive;
    Dcm_TransportStatusFuncType getStatus;
    void (*mainFunction)(void);
    
    /* Capability flags */
    bool supportsPhysicalAddressing;
    bool supportsFunctionalAddressing;
    bool supportsMultiFrame;
    uint32_t maxMessageSize;
} Dcm_TransportProtocolInterfaceType;

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

/**
 * @brief Initialize transport layer module
 *
 * @param config Pointer to transport configuration
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_Init(const Dcm_TransportConfigType *config);

/**
 * @brief Deinitialize transport layer module
 *
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_DeInit(void);

/**
 * @brief Start transport layer operations
 *
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_Start(void);

/**
 * @brief Stop transport layer operations
 *
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_Stop(void);

/**
 * @brief Main function - process pending operations
 *
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 * @note Should be called periodically (e.g., every 10ms)
 */
void Dcm_Transport_MainFunction(uint32_t elapsedTimeMs);

/******************************************************************************
 * Channel Management
 ******************************************************************************/

/**
 * @brief Open a transport channel
 *
 * @param protocol Transport protocol to use
 * @param priority Channel priority (0-7, 0=highest)
 * @param channelId Output: assigned channel ID
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_OpenChannel(
    Dcm_TransportProtocolType protocol,
    uint8_t priority,
    uint8_t *channelId
);

/**
 * @brief Close a transport channel
 *
 * @param channelId Channel identifier
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_CloseChannel(uint8_t channelId);

/**
 * @brief Configure a transport channel
 *
 * @param channelId Channel identifier
 * @param config Channel configuration
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_ConfigureChannel(
    uint8_t channelId,
    const Dcm_TransportChannelConfigType *config
);

/**
 * @brief Connect transport channel
 *
 * @param channelId Channel identifier
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_Connect(uint8_t channelId);

/**
 * @brief Disconnect transport channel
 *
 * @param channelId Channel identifier
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_Disconnect(uint8_t channelId);

/**
 * @brief Suspend transport channel (temporarily disable)
 *
 * @param channelId Channel identifier
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_SuspendChannel(uint8_t channelId);

/**
 * @brief Resume suspended transport channel
 *
 * @param channelId Channel identifier
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_ResumeChannel(uint8_t channelId);

/******************************************************************************
 * Unified Send/Receive Interface
 ******************************************************************************/

/**
 * @brief Send message via transport layer (protocol-agnostic)
 *
 * This function sends a diagnostic message using the specified channel.
 * The actual transport protocol (DoIP, DoCAN, IsoTp) is transparent to the caller.
 *
 * @param channelId Channel identifier
 * @param message Message to send
 * @return DCM_TRANSPORT_OK if transmission started successfully
 */
Dcm_TransportReturnType Dcm_Transport_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
);

/**
 * @brief Send message with priority (protocol-agnostic)
 *
 * @param channelId Channel identifier
 * @param message Message to send
 * @param priority Priority level (0-7, 0=highest)
 * @return DCM_TRANSPORT_OK if transmission started successfully
 */
Dcm_TransportReturnType Dcm_Transport_SendWithPriority(
    uint8_t channelId,
    const Dcm_TransportMessageType *message,
    uint8_t priority
);

/**
 * @brief Receive message from transport layer (protocol-agnostic)
 *
 * @param channelId Channel identifier
 * @param message Output: received message
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
);

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

/**
 * @brief Register message received callback
 *
 * @param callback Callback function
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_RegisterRxCallback(
    Dcm_TransportRxCallbackType callback
);

/**
 * @brief Register transmission confirmation callback
 *
 * @param callback Callback function
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_RegisterTxCallback(
    Dcm_TransportTxConfirmationType callback
);

/**
 * @brief Register event notification callback
 *
 * @param channelId Channel identifier
 * @param callback Callback function
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_RegisterEventCallback(
    uint8_t channelId,
    Dcm_TransportEventCallbackType callback
);

/******************************************************************************
 * Status Query Functions
 ******************************************************************************/

/**
 * @brief Get transport layer module status
 *
 * @return true if initialized and running
 */
bool Dcm_Transport_IsInitialized(void);

/**
 * @brief Check if module is running
 *
 * @return true if running
 */
bool Dcm_Transport_IsRunning(void);

/**
 * @brief Get channel state
 *
 * @param channelId Channel identifier
 * @return Channel state
 */
Dcm_TransportStateType Dcm_Transport_GetChannelState(uint8_t channelId);

/**
 * @brief Get channel information
 *
 * @param channelId Channel identifier
 * @param info Output: channel information
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_GetChannelInfo(
    uint8_t channelId,
    Dcm_TransportChannelInfoType *info
);

/**
 * @brief Get channel statistics
 *
 * @param channelId Channel identifier
 * @param stats Output: statistics
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_GetStatistics(
    uint8_t channelId,
    Dcm_TransportStatisticsType *stats
);

/**
 * @brief Reset channel statistics
 *
 * @param channelId Channel identifier
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_ResetStatistics(uint8_t channelId);

/******************************************************************************
 * Protocol Selection and Priority
 ******************************************************************************/

/**
 * @brief Set channel priority
 *
 * @param channelId Channel identifier
 * @param priority Priority level (0-7, 0=highest)
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_SetPriority(
    uint8_t channelId,
    uint8_t priority
);

/**
 * @brief Get channel priority
 *
 * @param channelId Channel identifier
 * @return Priority level (0-7), 0xFF if invalid channel
 */
uint8_t Dcm_Transport_GetPriority(uint8_t channelId);

/**
 * @brief Select default transport protocol
 *
 * @param protocol Default protocol type
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_SelectDefaultProtocol(
    Dcm_TransportProtocolType protocol
);

/**
 * @brief Get default transport protocol
 *
 * @return Default protocol type
 */
Dcm_TransportProtocolType Dcm_Transport_GetDefaultProtocol(void);

/**
 * @brief Find channel by protocol type
 *
 * @param protocol Protocol type to find
 * @return Channel ID if found, DCM_TRANSPORT_INVALID_CHANNEL_ID otherwise
 */
uint8_t Dcm_Transport_FindChannelByProtocol(Dcm_TransportProtocolType protocol);

/**
 * @brief Find best available channel based on priority
 *
 * @return Channel ID of highest priority available channel
 */
uint8_t Dcm_Transport_FindBestAvailableChannel(void);

/******************************************************************************
 * Protocol-Specific Functions
 ******************************************************************************/

/**
 * @brief Register protocol interface
 *
 * @param protocolInterface Protocol interface structure
 * @return DCM_TRANSPORT_OK on success
 */
Dcm_TransportReturnType Dcm_Transport_RegisterProtocolInterface(
    const Dcm_TransportProtocolInterfaceType *protocolInterface
);

/**
 * @brief Get protocol interface by type
 *
 * @param protocol Protocol type
 * @return Pointer to protocol interface, NULL if not registered
 */
const Dcm_TransportProtocolInterfaceType* Dcm_Transport_GetProtocolInterface(
    Dcm_TransportProtocolType protocol
);

/**
 * @brief Get protocol name string
 *
 * @param protocol Protocol type
 * @return Protocol name string
 */
const char* Dcm_Transport_GetProtocolName(Dcm_TransportProtocolType protocol);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Check if protocol is supported
 *
 * @param protocol Protocol type
 * @return true if supported
 */
bool Dcm_Transport_IsProtocolSupported(Dcm_TransportProtocolType protocol);

/**
 * @brief Get number of active channels
 *
 * @return Number of active channels
 */
uint8_t Dcm_Transport_GetActiveChannelCount(void);

/**
 * @brief Get number of connected channels
 *
 * @return Number of connected channels
 */
uint8_t Dcm_Transport_GetConnectedChannelCount(void);

/**
 * @brief Convert transport return type to DCM return type
 *
 * @param transportRet Transport return type
 * @return DCM return type
 */
Dcm_ReturnType Dcm_Transport_ConvertReturnType(Dcm_TransportReturnType transportRet);

/**
 * @brief Validate transport message
 *
 * @param message Message to validate
 * @return true if valid
 */
bool Dcm_Transport_ValidateMessage(const Dcm_TransportMessageType *message);

/******************************************************************************
 * Internal Protocol Adaptors (called by transport layer)
 ******************************************************************************/

/* DoIP Protocol Adaptor */
Dcm_TransportReturnType Dcm_Transport_DoIp_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
);
Dcm_TransportReturnType Dcm_Transport_DoIp_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
);
Dcm_TransportStateType Dcm_Transport_DoIp_GetStatus(uint8_t channelId);
void Dcm_Transport_DoIp_MainFunction(void);

/* DoCAN Protocol Adaptor */
Dcm_TransportReturnType Dcm_Transport_DoCan_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
);
Dcm_TransportReturnType Dcm_Transport_DoCan_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
);
Dcm_TransportStateType Dcm_Transport_DoCan_GetStatus(uint8_t channelId);
void Dcm_Transport_DoCan_MainFunction(void);

/* IsoTp Protocol Adaptor */
Dcm_TransportReturnType Dcm_Transport_IsoTp_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
);
Dcm_TransportReturnType Dcm_Transport_IsoTp_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
);
Dcm_TransportStateType Dcm_Transport_IsoTp_GetStatus(uint8_t channelId);
void Dcm_Transport_IsoTp_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* DCM_TRANSPORT_H */

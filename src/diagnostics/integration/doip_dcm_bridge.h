/******************************************************************************
 * @file    doip_dcm_bridge.h
 * @brief   DoIP-DCM Bridge Layer - Integration between DoIP and DCM modules
 *
 * This module provides the bridge between ISO 13400-2 DoIP protocol and
 * AUTOSAR DCM diagnostic communication manager.
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ISO 13400-2:2019 DoIP Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DOIP_DCM_BRIDGE_H
#define DOIP_DCM_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../doip/doip_core.h"
#include "../dcm/dcm.h"

/******************************************************************************
 * Bridge Module Version Information
 ******************************************************************************/
#define DOIP_DCM_BRIDGE_VENDOR_ID               0x01U
#define DOIP_DCM_BRIDGE_MODULE_ID               0x36U
#define DOIP_DCM_BRIDGE_SW_MAJOR_VERSION        1U
#define DOIP_DCM_BRIDGE_SW_MINOR_VERSION        0U
#define DOIP_DCM_BRIDGE_SW_PATCH_VERSION        0U

/******************************************************************************
 * Bridge Configuration Constants
 ******************************************************************************/

/* Maximum number of concurrent diagnostic sessions */
#define DOIP_DCM_BRIDGE_MAX_SESSIONS            8U

/* Buffer sizes for large packet handling */
#define DOIP_DCM_BRIDGE_STD_BUFFER_SIZE         4096U
#define DOIP_DCM_BRIDGE_LARGE_BUFFER_SIZE       65535U  /* Max DoIP payload */
#define DOIP_DCM_BRIDGE_BUFFER_THRESHOLD        4096U

/* Message buffer pool configuration */
#define DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS       16U
#define DOIP_DCM_BRIDGE_MSG_TIMEOUT_MS          5000U

/* Session timeout values */
#define DOIP_DCM_BRIDGE_SESSION_TIMEOUT_MS      300000U  /* 5 minutes */
#define DOIP_DCM_BRIDGE_ALIVE_CHECK_INTERVAL_MS 30000U   /* 30 seconds */

/******************************************************************************
 * Bridge Return Types
 ******************************************************************************/

typedef enum {
    DOIP_DCM_BRIDGE_OK = 0x00U,
    DOIP_DCM_BRIDGE_ERROR = 0x01U,
    DOIP_DCM_BRIDGE_BUSY = 0x02U,
    DOIP_DCM_BRIDGE_TIMEOUT = 0x03U,
    DOIP_DCM_BRIDGE_INVALID_PARAMETER = 0x04U,
    DOIP_DCM_BRIDGE_NOT_INITIALIZED = 0x05U,
    DOIP_DCM_BRIDGE_NO_SESSION = 0x06U,
    DOIP_DCM_BRIDGE_NO_BUFFER = 0x07U,
    DOIP_DCM_BRIDGE_MESSAGE_TOO_LARGE = 0x08U,
    DOIP_DCM_BRIDGE_SESSION_LIMIT = 0x09U,
    DOIP_DCM_BRIDGE_TRANSFER_IN_PROGRESS = 0x0AU
} DoIp_Dcm_Bridge_ReturnType;

/******************************************************************************
 * Session States
 ******************************************************************************/

typedef enum {
    DOIP_DCM_SESSION_INACTIVE = 0x00U,
    DOIP_DCM_SESSION_CONNECTING = 0x01U,
    DOIP_DCM_SESSION_ACTIVE = 0x02U,
    DOIP_DCM_SESSION_PROCESSING = 0x03U,
    DOIP_DCM_SESSION_LARGE_TRANSFER = 0x04U,
    DOIP_DCM_SESSION_CLOSING = 0x05U,
    DOIP_DCM_SESSION_ERROR = 0xFFU
} DoIp_Dcm_SessionStateType;

/******************************************************************************
 * Large Transfer States
 ******************************************************************************/

typedef enum {
    DOIP_DCM_TRANSFER_IDLE = 0x00U,
    DOIP_DCM_TRANSFER_RX_IN_PROGRESS = 0x01U,
    DOIP_DCM_TRANSFER_TX_IN_PROGRESS = 0x02U,
    DOIP_DCM_TRANSFER_RX_COMPLETE = 0x03U,
    DOIP_DCM_TRANSFER_TX_COMPLETE = 0x04U,
    DOIP_DCM_TRANSFER_ERROR = 0xFFU
} DoIp_Dcm_TransferStateType;

/******************************************************************************
 * Message Buffer Types
 ******************************************************************************/

/* Buffered message entry */
typedef struct {
    uint8_t connectionId;
    uint16_t sourceAddress;
    uint16_t targetAddress;
    uint8_t *data;
    uint32_t dataLength;
    uint32_t timestamp;
    bool inUse;
    bool isLargeMessage;
} DoIp_Dcm_BufferedMessageType;

/* Message buffer pool */
typedef struct {
    DoIp_Dcm_BufferedMessageType entries[DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS];
    uint8_t *dataPool;
    uint32_t dataPoolSize;
    uint8_t usedCount;
} DoIp_Dcm_MessageBufferType;

/******************************************************************************
 * Large Transfer Context
 ******************************************************************************/

typedef struct {
    DoIp_Dcm_TransferStateType state;
    uint8_t *buffer;
    uint32_t bufferSize;
    uint32_t totalLength;
    uint32_t transferredLength;
    uint32_t segmentSize;
    uint16_t sequenceNumber;
    bool isReceive;  /* true = RX, false = TX */
} DoIp_Dcm_LargeTransferContextType;

/******************************************************************************
 * Diagnostic Session Context
 ******************************************************************************/

typedef struct {
    /* Session identification */
    uint8_t sessionId;
    uint8_t connectionId;
    uint16_t sourceAddress;
    uint16_t targetAddress;
    
    /* Session state */
    DoIp_Dcm_SessionStateType state;
    
    /* Timing */
    uint32_t sessionStartTime;
    uint32_t lastActivityTime;
    uint32_t timeoutValue;
    
    /* DCM protocol channel */
    uint8_t dcmChannelId;
    Dcm_ProtocolType protocolType;
    
    /* Buffers */
    uint8_t *rxBuffer;
    uint8_t *txBuffer;
    uint32_t rxBufferSize;
    uint32_t txBufferSize;
    
    /* Large transfer support */
    DoIp_Dcm_LargeTransferContextType largeTransfer;
    bool useLargeBuffers;
    
    /* Statistics */
    uint32_t requestsProcessed;
    uint32_t responsesSent;
    uint32_t errors;
    
    /* Session flags */
    bool isActive;
    bool routingActivated;
    bool authenticated;
} DoIp_Dcm_SessionContextType;

/******************************************************************************
 * Bridge Configuration
 ******************************************************************************/

typedef struct {
    /* Session configuration */
    uint8_t maxSessions;
    uint32_t sessionTimeoutMs;
    uint32_t aliveCheckIntervalMs;
    
    /* Buffer configuration */
    uint32_t standardBufferSize;
    uint32_t largeBufferSize;
    uint32_t bufferThreshold;
    
    /* Message buffering */
    uint8_t maxBufferedMessages;
    uint32_t messageTimeoutMs;
    
    /* DCM configuration */
    const Dcm_ConfigType *dcmConfig;
    
    /* Callbacks */
    void (*onSessionCreated)(uint8_t sessionId, uint16_t sourceAddr);
    void (*onSessionClosed)(uint8_t sessionId);
    void (*onLargeTransferStarted)(uint8_t sessionId, uint32_t totalLength);
    void (*onError)(uint8_t sessionId, uint8_t errorCode);
} DoIp_Dcm_BridgeConfigType;

/******************************************************************************
 * Bridge Statistics
 ******************************************************************************/

typedef struct {
    uint32_t totalSessions;
    uint32_t activeSessions;
    uint32_t totalRequests;
    uint32_t totalResponses;
    uint32_t largeTransfers;
    uint32_t bufferedMessages;
    uint32_t droppedMessages;
    uint32_t errors;
    uint32_t timeouts;
} DoIp_Dcm_BridgeStatisticsType;

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

/**
 * @brief Initialize DoIP-DCM Bridge module
 *
 * @param config Pointer to bridge configuration
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_Init(
    const DoIp_Dcm_BridgeConfigType *config
);

/**
 * @brief Deinitialize DoIP-DCM Bridge module
 *
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_DeInit(void);

/**
 * @brief Start bridge operations
 *
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_Start(void);

/**
 * @brief Stop bridge operations
 *
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_Stop(void);

/**
 * @brief Main function - process pending operations
 *
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 *
 * @note Should be called periodically (e.g., every 10ms)
 */
void DoIp_Dcm_Bridge_MainFunction(uint32_t elapsedTimeMs);

/******************************************************************************
 * Session Management
 ******************************************************************************/

/**
 * @brief Create new diagnostic session
 *
 * @param connectionId DoIP connection ID
 * @param sourceAddress Tester source address
 * @param targetAddress Target ECU address
 * @param sessionId Output session ID
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_CreateSession(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint16_t targetAddress,
    uint8_t *sessionId
);

/**
 * @brief Close diagnostic session
 *
 * @param sessionId Session ID
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_CloseSession(uint8_t sessionId);

/**
 * @brief Get session context
 *
 * @param sessionId Session ID
 * @return Pointer to session context or NULL if not found
 */
DoIp_Dcm_SessionContextType* DoIp_Dcm_Bridge_GetSession(uint8_t sessionId);

/**
 * @brief Find session by connection ID
 *
 * @param connectionId DoIP connection ID
 * @return Session ID or 0xFF if not found
 */
uint8_t DoIp_Dcm_Bridge_FindSessionByConnection(uint8_t connectionId);

/**
 * @brief Check if session is valid
 *
 * @param sessionId Session ID
 * @return true if valid and active
 */
bool DoIp_Dcm_Bridge_IsSessionValid(uint8_t sessionId);

/******************************************************************************
 * Message Processing
 ******************************************************************************/

/**
 * @brief Process incoming DoIP diagnostic message
 *
 * Converts DoIP diagnostic message to DCM request and processes it.
 * Supports both standard and large messages.
 *
 * @param connectionId DoIP connection ID
 * @param message DoIP diagnostic message
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_ProcessDiagnosticMessage(
    uint8_t connectionId,
    const DoIp_DiagnosticMessageType *message
);

/**
 * @brief Handle DCM response and send as DoIP message
 *
 * Converts DCM response to DoIP diagnostic message and sends it.
 *
 * @param sessionId Session ID
 * @param response DCM response
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_SendResponse(
    uint8_t sessionId,
    const Dcm_ResponseType *response
);

/**
 * @brief Process request-response cycle for a session
 *
 * @param sessionId Session ID
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_ProcessRequestResponse(
    uint8_t sessionId
);

/******************************************************************************
 * Large Packet Handling (>4096 bytes)
 ******************************************************************************/

/**
 * @brief Initialize large transfer context
 *
 * @param sessionId Session ID
 * @param totalLength Total transfer length
 * @param isReceive true for RX, false for TX
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_InitLargeTransfer(
    uint8_t sessionId,
    uint32_t totalLength,
    bool isReceive
);

/**
 * @brief Process large message segment
 *
 * @param sessionId Session ID
 * @param segmentData Segment data
 * @param segmentLength Segment length
 * @param sequenceNumber Sequence number
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_ProcessLargeSegment(
    uint8_t sessionId,
    const uint8_t *segmentData,
    uint32_t segmentLength,
    uint16_t sequenceNumber
);

/**
 * @brief Get next large response segment
 *
 * @param sessionId Session ID
 * @param segmentData Output buffer
 * @param maxLength Maximum buffer size
 * @param segmentLength Output segment length
 * @param sequenceNumber Output sequence number
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_GetNextResponseSegment(
    uint8_t sessionId,
    uint8_t *segmentData,
    uint32_t maxLength,
    uint32_t *segmentLength,
    uint16_t *sequenceNumber
);

/**
 * @brief Check if message requires large buffer
 *
 * @param messageLength Message length
 * @return true if large buffer needed
 */
bool DoIp_Dcm_Bridge_IsLargeMessage(uint32_t messageLength);

/**
 * @brief Complete large transfer
 *
 * @param sessionId Session ID
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_CompleteLargeTransfer(
    uint8_t sessionId
);

/******************************************************************************
 * Message Buffering
 ******************************************************************************/

/**
 * @brief Buffer diagnostic message for later processing
 *
 * @param connectionId DoIP connection ID
 * @param message DoIP diagnostic message
 * @return DOIP_DCM_BRIDGE_OK on success
 */
DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_BufferMessage(
    uint8_t connectionId,
    const DoIp_DiagnosticMessageType *message
);

/**
 * @brief Process buffered messages
 *
 * Called from MainFunction to process pending messages.
 *
 * @return Number of messages processed
 */
uint8_t DoIp_Dcm_Bridge_ProcessBufferedMessages(void);

/**
 * @brief Clear all buffered messages for a connection
 *
 * @param connectionId DoIP connection ID
 */
void DoIp_Dcm_Bridge_ClearBufferedMessages(uint8_t connectionId);

/**
 * @brief Get number of pending buffered messages
 *
 * @return Number of buffered messages
 */
uint8_t DoIp_Dcm_Bridge_GetBufferedMessageCount(void);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Get module status
 *
 * @return true if initialized
 */
bool DoIp_Dcm_Bridge_IsInitialized(void);

/**
 * @brief Get bridge statistics
 *
 * @param stats Output statistics structure
 */
void DoIp_Dcm_Bridge_GetStatistics(
    DoIp_Dcm_BridgeStatisticsType *stats
);

/**
 * @brief Reset bridge statistics
 */
void DoIp_Dcm_Bridge_ResetStatistics(void);

/**
 * @brief Check if session has pending operations
 *
 * @param sessionId Session ID
 * @return true if pending
 */
bool DoIp_Dcm_Bridge_HasPendingOperations(uint8_t sessionId);

/**
 * @brief Get version information
 *
 * @param major Major version
 * @param minor Minor version
 * @param patch Patch version
 */
void DoIp_Dcm_Bridge_GetVersionInfo(
    uint8_t *major,
    uint8_t *minor,
    uint8_t *patch
);

/******************************************************************************
 * DoIP Callback Registration
 ******************************************************************************/

/**
 * @brief Handle DoIP routing activation event
 *
 * Called by DoIP when routing activation is successful.
 *
 * @param connectionId DoIP connection ID
 * @param sourceAddress Tester source address
 * @param result Activation result code
 */
void DoIp_Dcm_Bridge_OnRoutingActivation(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint8_t result
);

/**
 * @brief Handle DoIP diagnostic message event
 *
 * Called by DoIP when diagnostic message is received.
 *
 * @param connectionId DoIP connection ID
 * @param sourceAddress Source address
 * @param targetAddress Target address
 * @param data Message data
 * @param length Message length
 */
void DoIp_Dcm_Bridge_OnDiagnosticMessage(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint16_t targetAddress,
    uint8_t *data,
    uint16_t length
);

/**
 * @brief Handle DoIP connection closed event
 *
 * Called by DoIP when connection is closed.
 *
 * @param connectionId DoIP connection ID
 */
void DoIp_Dcm_Bridge_OnConnectionClosed(uint8_t connectionId);

#ifdef __cplusplus
}
#endif

#endif /* DOIP_DCM_BRIDGE_H */

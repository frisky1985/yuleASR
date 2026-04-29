/******************************************************************************
 * @file    doip_dcm_bridge.c
 * @brief   DoIP-DCM Bridge Layer Implementation
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "doip_dcm_bridge.h"
#include <string.h>
#include <stddef.h>

/******************************************************************************
 * Module Configuration
 ******************************************************************************/

#define DOIP_DCM_BRIDGE_INVALID_SESSION     0xFFU
#define DOIP_DCM_BRIDGE_INVALID_CHANNEL     0xFFU

/******************************************************************************
 * Module Global Variables
 ******************************************************************************/

static bool g_bridgeInitialized = false;
static DoIp_Dcm_BridgeConfigType g_bridgeConfig;
static DoIp_Dcm_SessionContextType g_sessions[DOIP_DCM_BRIDGE_MAX_SESSIONS];
static DoIp_Dcm_MessageBufferType g_messageBuffer;
static DoIp_Dcm_BridgeStatisticsType g_statistics;

/* Standard buffers for sessions */
static uint8_t g_stdRxBuffers[DOIP_DCM_BRIDGE_MAX_SESSIONS][DOIP_DCM_BRIDGE_STD_BUFFER_SIZE];
static uint8_t g_stdTxBuffers[DOIP_DCM_BRIDGE_MAX_SESSIONS][DOIP_DCM_BRIDGE_STD_BUFFER_SIZE];

/* Large buffers (allocated on demand) */
static uint8_t g_largeRxBuffer[DOIP_DCM_BRIDGE_LARGE_BUFFER_SIZE];
static uint8_t g_largeTxBuffer[DOIP_DCM_BRIDGE_LARGE_BUFFER_SIZE];
static bool g_largeBufferInUse = false;

/* Message buffer pool */
static uint8_t g_msgBufferData[DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS][DOIP_DCM_BRIDGE_STD_BUFFER_SIZE];
static DoIp_Dcm_BufferedMessageType g_msgBufferEntries[DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS];

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get current timestamp in milliseconds
 */
static uint32_t DoIp_Dcm_Bridge_GetTimestamp(void)
{
    static uint32_t counter = 0U;
    return counter++;
}

/**
 * @brief Validate session ID
 */
static bool DoIp_Dcm_Bridge_IsValidSessionId(uint8_t sessionId)
{
    return (sessionId < DOIP_DCM_BRIDGE_MAX_SESSIONS);
}

/**
 * @brief Find free session slot
 */
static int8_t DoIp_Dcm_Bridge_FindFreeSession(void)
{
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_SESSIONS; i++) {
        if (!g_sessions[i].isActive) {
            return (int8_t)i;
        }
    }
    return -1;
}

/**
 * @brief Find session by connection ID
 */
static int8_t DoIp_Dcm_Bridge_FindSessionByConnId(uint8_t connectionId)
{
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_SESSIONS; i++) {
        if (g_sessions[i].isActive && g_sessions[i].connectionId == connectionId) {
            return (int8_t)i;
        }
    }
    return -1;
}

/**
 * @brief Initialize session context
 */
static void DoIp_Dcm_Bridge_InitSessionContext(
    DoIp_Dcm_SessionContextType *session,
    uint8_t sessionId
)
{
    memset(session, 0, sizeof(DoIp_Dcm_SessionContextType));
    session->connectionId = DOIP_DCM_BRIDGE_INVALID_SESSION;
    session->dcmChannelId = DOIP_DCM_BRIDGE_INVALID_CHANNEL;
    session->state = DOIP_DCM_SESSION_INACTIVE;
    session->rxBuffer = g_stdRxBuffers[sessionId];
    session->txBuffer = g_stdTxBuffers[sessionId];
    session->rxBufferSize = DOIP_DCM_BRIDGE_STD_BUFFER_SIZE;
    session->txBufferSize = DOIP_DCM_BRIDGE_STD_BUFFER_SIZE;
    session->largeTransfer.state = DOIP_DCM_TRANSFER_IDLE;
    session->largeTransfer.buffer = NULL;
    session->useLargeBuffers = false;
    session->isActive = false;
}

/**
 * @brief Allocate large buffer for session
 */
static DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_AllocateLargeBuffer(
    DoIp_Dcm_SessionContextType *session,
    bool forReceive
)
{
    if (g_largeBufferInUse) {
        return DOIP_DCM_BRIDGE_BUSY;
    }

    g_largeBufferInUse = true;
    
    if (forReceive) {
        session->rxBuffer = g_largeRxBuffer;
        session->rxBufferSize = DOIP_DCM_BRIDGE_LARGE_BUFFER_SIZE;
    } else {
        session->txBuffer = g_largeTxBuffer;
        session->txBufferSize = DOIP_DCM_BRIDGE_LARGE_BUFFER_SIZE;
    }
    
    session->useLargeBuffers = true;
    return DOIP_DCM_BRIDGE_OK;
}

/**
 * @brief Free large buffer
 */
static void DoIp_Dcm_Bridge_FreeLargeBuffer(DoIp_Dcm_SessionContextType *session)
{
    if (session->useLargeBuffers) {
        session->rxBuffer = g_stdRxBuffers[session->connectionId];
        session->txBuffer = g_stdTxBuffers[session->connectionId];
        session->rxBufferSize = DOIP_DCM_BRIDGE_STD_BUFFER_SIZE;
        session->txBufferSize = DOIP_DCM_BRIDGE_STD_BUFFER_SIZE;
        session->useLargeBuffers = false;
        g_largeBufferInUse = false;
    }
}

/**
 * @brief Convert DoIP message to DCM request
 */
static Dcm_ReturnType DoIp_Dcm_Bridge_ConvertToDcmRequest(
    const DoIp_DiagnosticMessageType *doipMsg,
    Dcm_RequestType *dcmRequest,
    DoIp_Dcm_SessionContextType *session
)
{
    if (doipMsg == NULL || dcmRequest == NULL || session == NULL) {
        return DCM_E_NOT_OK;
    }

    /* Check if data fits in buffer */
    if (doipMsg->userDataLength > session->rxBufferSize) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }

    /* Copy data to session RX buffer */
    memcpy(session->rxBuffer, doipMsg->userData, doipMsg->userDataLength);

    /* Setup DCM request */
    dcmRequest->data = session->rxBuffer;
    dcmRequest->length = doipMsg->userDataLength;
    dcmRequest->sourceAddress = doipMsg->sourceAddress;
    dcmRequest->addrMode = DCM_ADDR_PHYSICAL;
    dcmRequest->protocol = DCM_PROTOCOL_UDS_ON_IP;
    dcmRequest->timestamp = DoIp_Dcm_Bridge_GetTimestamp();

    return DCM_E_OK;
}

/**
 * @brief Convert DCM response to DoIP message
 */
static DoIp_ReturnType DoIp_Dcm_Bridge_ConvertToDoIpMessage(
    const Dcm_ResponseType *dcmResponse,
    DoIp_DiagnosticMessageType *doipMsg,
    DoIp_Dcm_SessionContextType *session
)
{
    if (dcmResponse == NULL || doipMsg == NULL || session == NULL) {
        return DOIP_ERROR;
    }

    /* Check if response fits in buffer */
    if (dcmResponse->length > session->txBufferSize) {
        return DOIP_ERROR;
    }

    /* Setup DoIP message */
    doipMsg->sourceAddress = session->targetAddress;
    doipMsg->targetAddress = session->sourceAddress;
    doipMsg->userData = session->txBuffer;
    doipMsg->userDataLength = (uint16_t)dcmResponse->length;

    /* Copy response data */
    memcpy(session->txBuffer, dcmResponse->data, dcmResponse->length);

    return DOIP_OK;
}

/**
 * @brief Process UDS service request
 */
static Dcm_ReturnType DoIp_Dcm_Bridge_ProcessUdsRequest(
    DoIp_Dcm_SessionContextType *session
)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    Dcm_ReturnType result;

    /* Setup request */
    request.data = session->rxBuffer;
    request.length = session->largeTransfer.transferredLength > 0U ? 
                     session->largeTransfer.transferredLength : 
                     session->rxBufferSize;  /* Will be set properly during conversion */
    request.sourceAddress = session->sourceAddress;
    request.addrMode = DCM_ADDR_PHYSICAL;
    request.protocol = DCM_PROTOCOL_UDS_ON_IP;
    request.timestamp = DoIp_Dcm_Bridge_GetTimestamp();

    /* Setup response buffer */
    response.data = session->txBuffer;
    response.maxLength = session->txBufferSize;
    response.length = 0U;
    response.isNegativeResponse = false;
    response.negativeResponseCode = 0U;
    response.suppressPositiveResponse = false;

    /* Process request through DCM */
    result = Dcm_ProcessRequest(&request, &response);

    if (result == DCM_E_OK) {
        /* Send response */
        DoIp_DiagnosticMessageType doipMsg;
        if (DoIp_Dcm_Bridge_ConvertToDoIpMessage(&response, &doipMsg, session) == DOIP_OK) {
            DoIp_SendDiagnosticMessage(session->connectionId, &doipMsg);
            session->responsesSent++;
            g_statistics.totalResponses++;
        }
    }

    return result;
}

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_Init(
    const DoIp_Dcm_BridgeConfigType *config
)
{
    if (config == NULL) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    if (g_bridgeInitialized) {
        (void)DoIp_Dcm_Bridge_DeInit();
    }

    /* Copy configuration */
    memcpy(&g_bridgeConfig, config, sizeof(DoIp_Dcm_BridgeConfigType));

    /* Initialize sessions */
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_SESSIONS; i++) {
        DoIp_Dcm_Bridge_InitSessionContext(&g_sessions[i], i);
        g_sessions[i].sessionId = i;  /* Add session ID to context */
    }

    /* Initialize message buffer pool */
    memset(&g_messageBuffer, 0, sizeof(g_messageBuffer));
    g_messageBuffer.dataPool = (uint8_t *)g_msgBufferData;
    g_messageBuffer.dataPoolSize = sizeof(g_msgBufferData);
    g_messageBuffer.usedCount = 0U;

    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS; i++) {
        g_messageBuffer.entries[i].data = g_msgBufferData[i];
        g_messageBuffer.entries[i].inUse = false;
    }

    /* Initialize statistics */
    memset(&g_statistics, 0, sizeof(g_statistics));

    /* Initialize DCM if not already done */
    if (!Dcm_IsInitialized()) {
        if (config->dcmConfig != NULL) {
            (void)Dcm_Init(config->dcmConfig);
        }
    }

    g_bridgeInitialized = true;

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_DeInit(void)
{
    if (!g_bridgeInitialized) {
        return DOIP_DCM_BRIDGE_NOT_INITIALIZED;
    }

    /* Close all active sessions */
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_SESSIONS; i++) {
        if (g_sessions[i].isActive) {
            (void)DoIp_Dcm_Bridge_CloseSession(i);
        }
    }

    /* Clear message buffer */
    memset(&g_messageBuffer, 0, sizeof(g_messageBuffer));

    /* Reset statistics */
    memset(&g_statistics, 0, sizeof(g_statistics));

    g_bridgeInitialized = false;

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_Start(void)
{
    if (!g_bridgeInitialized) {
        return DOIP_DCM_BRIDGE_NOT_INITIALIZED;
    }

    /* Register DoIP callbacks */
    DoIp_RegisterRoutingActivationCallback(
        (DoIp_RoutingActivationCallback)DoIp_Dcm_Bridge_OnRoutingActivation
    );
    DoIp_RegisterDiagnosticCallback(
        (DoIp_DiagnosticCallback)DoIp_Dcm_Bridge_OnDiagnosticMessage
    );
    DoIp_RegisterConnectionCallback(
        (DoIp_ConnectionCallback)DoIp_Dcm_Bridge_OnConnectionClosed
    );

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_Stop(void)
{
    if (!g_bridgeInitialized) {
        return DOIP_DCM_BRIDGE_NOT_INITIALIZED;
    }

    /* Close all sessions */
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_SESSIONS; i++) {
        if (g_sessions[i].isActive) {
            (void)DoIp_Dcm_Bridge_CloseSession(i);
        }
    }

    return DOIP_DCM_BRIDGE_OK;
}

void DoIp_Dcm_Bridge_MainFunction(uint32_t elapsedTimeMs)
{
    if (!g_bridgeInitialized) {
        return;
    }

    /* Update DCM */
    Dcm_MainFunction(elapsedTimeMs);

    /* Process buffered messages */
    (void)DoIp_Dcm_Bridge_ProcessBufferedMessages();

    /* Check session timeouts */
    uint32_t currentTime = DoIp_Dcm_Bridge_GetTimestamp();
    
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_SESSIONS; i++) {
        DoIp_Dcm_SessionContextType *session = &g_sessions[i];
        
        if (!session->isActive) {
            continue;
        }

        /* Check session timeout */
        if ((currentTime - session->lastActivityTime) > session->timeoutValue) {
            /* Session timeout - close it */
            (void)DoIp_Dcm_Bridge_CloseSession(i);
            g_statistics.timeouts++;
            continue;
        }

        /* Process active sessions */
        if (session->state == DOIP_DCM_SESSION_PROCESSING) {
            /* Continue processing pending request */
            (void)DoIp_Dcm_Bridge_ProcessRequestResponse(i);
        }

        /* Check large transfer timeout */
        if (session->state == DOIP_DCM_SESSION_LARGE_TRANSFER) {
            /* Monitor large transfer progress */
            if (session->largeTransfer.state == DOIP_DCM_TRANSFER_ERROR) {
                session->state = DOIP_DCM_SESSION_ERROR;
                g_statistics.errors++;
            }
        }
    }

    /* Clean up expired buffered messages */
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS; i++) {
        DoIp_Dcm_BufferedMessageType *entry = &g_messageBuffer.entries[i];
        
        if (entry->inUse) {
            if ((currentTime - entry->timestamp) > g_bridgeConfig.messageTimeoutMs) {
                /* Message expired - clear it */
                entry->inUse = false;
                g_messageBuffer.usedCount--;
                g_statistics.droppedMessages++;
            }
        }
    }
}

/******************************************************************************
 * Session Management Implementation
 ******************************************************************************/

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_CreateSession(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint16_t targetAddress,
    uint8_t *sessionId
)
{
    if (!g_bridgeInitialized) {
        return DOIP_DCM_BRIDGE_NOT_INITIALIZED;
    }

    if (sessionId == NULL) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    /* Find free session slot */
    int8_t freeSlot = DoIp_Dcm_Bridge_FindFreeSession();
    if (freeSlot < 0) {
        return DOIP_DCM_BRIDGE_SESSION_LIMIT;
    }

    uint8_t sid = (uint8_t)freeSlot;
    DoIp_Dcm_SessionContextType *session = &g_sessions[sid];

    /* Initialize session */
    DoIp_Dcm_Bridge_InitSessionContext(session, sid);
    
    session->connectionId = connectionId;
    session->sourceAddress = sourceAddress;
    session->targetAddress = targetAddress;
    session->sessionStartTime = DoIp_Dcm_Bridge_GetTimestamp();
    session->lastActivityTime = session->sessionStartTime;
    session->timeoutValue = g_bridgeConfig.sessionTimeoutMs;
    session->state = DOIP_DCM_SESSION_ACTIVE;
    session->isActive = true;
    session->routingActivated = true;

    *sessionId = sid;

    g_statistics.totalSessions++;
    g_statistics.activeSessions++;

    /* Call user callback */
    if (g_bridgeConfig.onSessionCreated != NULL) {
        g_bridgeConfig.onSessionCreated(sid, sourceAddress);
    }

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_CloseSession(uint8_t sessionId)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    /* Free large buffer if in use */
    if (session->useLargeBuffers) {
        DoIp_Dcm_Bridge_FreeLargeBuffer(session);
    }

    /* Clear any buffered messages for this session */
    DoIp_Dcm_Bridge_ClearBufferedMessages(session->connectionId);

    /* Call user callback */
    if (g_bridgeConfig.onSessionClosed != NULL) {
        g_bridgeConfig.onSessionClosed(sessionId);
    }

    /* Reset session */
    uint8_t connId = session->connectionId;
    DoIp_Dcm_Bridge_InitSessionContext(session, sessionId);
    session->connectionId = connId;  /* Preserve for callback reference */

    g_statistics.activeSessions--;

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_SessionContextType* DoIp_Dcm_Bridge_GetSession(uint8_t sessionId)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return NULL;
    }

    if (!g_sessions[sessionId].isActive) {
        return NULL;
    }

    return &g_sessions[sessionId];
}

uint8_t DoIp_Dcm_Bridge_FindSessionByConnection(uint8_t connectionId)
{
    int8_t sessionId = DoIp_Dcm_Bridge_FindSessionByConnId(connectionId);
    return (sessionId >= 0) ? (uint8_t)sessionId : DOIP_DCM_BRIDGE_INVALID_SESSION;
}

bool DoIp_Dcm_Bridge_IsSessionValid(uint8_t sessionId)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return false;
    }

    return g_sessions[sessionId].isActive;
}

/******************************************************************************
 * Message Processing Implementation
 ******************************************************************************/

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_ProcessDiagnosticMessage(
    uint8_t connectionId,
    const DoIp_DiagnosticMessageType *message
)
{
    if (!g_bridgeInitialized) {
        return DOIP_DCM_BRIDGE_NOT_INITIALIZED;
    }

    if (message == NULL) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    /* Find existing session or create new one */
    uint8_t sessionId = DoIp_Dcm_Bridge_FindSessionByConnection(connectionId);
    
    if (sessionId == DOIP_DCM_BRIDGE_INVALID_SESSION) {
        /* Create new session */
        DoIp_Dcm_Bridge_ReturnType result = DoIp_Dcm_Bridge_CreateSession(
            connectionId,
            message->sourceAddress,
            message->targetAddress,
            &sessionId
        );
        
        if (result != DOIP_DCM_BRIDGE_OK) {
            return result;
        }
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    /* Check for large message */
    if (DoIp_Dcm_Bridge_IsLargeMessage(message->userDataLength)) {
        /* Initialize large transfer */
        DoIp_Dcm_Bridge_ReturnType result = DoIp_Dcm_Bridge_InitLargeTransfer(
            sessionId,
            message->userDataLength,
            true
        );
        
        if (result != DOIP_DCM_BRIDGE_OK) {
            return result;
        }

        /* Copy first segment */
        result = DoIp_Dcm_Bridge_ProcessLargeSegment(
            sessionId,
            message->userData,
            message->userDataLength,
            0U
        );
        
        if (result != DOIP_DCM_BRIDGE_OK) {
            return result;
        }

        /* Process if complete (single segment large message) */
        if (session->largeTransfer.transferredLength >= session->largeTransfer.totalLength) {
            return DoIp_Dcm_Bridge_CompleteLargeTransfer(sessionId);
        }
        
        return DOIP_DCM_BRIDGE_OK;
    }

    /* Standard message processing */
    if (message->userDataLength > session->rxBufferSize) {
        return DOIP_DCM_BRIDGE_MESSAGE_TOO_LARGE;
    }

    /* Copy data to session buffer */
    memcpy(session->rxBuffer, message->userData, message->userDataLength);
    session->rxBufferSize = message->userDataLength;
    session->lastActivityTime = DoIp_Dcm_Bridge_GetTimestamp();
    session->state = DOIP_DCM_SESSION_PROCESSING;

    /* Process request-response */
    return DoIp_Dcm_Bridge_ProcessRequestResponse(sessionId);
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_SendResponse(
    uint8_t sessionId,
    const Dcm_ResponseType *response
)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    DoIp_DiagnosticMessageType doipMsg;
    DoIp_ReturnType result;

    /* Check if large response */
    if (DoIp_Dcm_Bridge_IsLargeMessage(response->length)) {
        /* Setup for large transfer */
        DoIp_Dcm_Bridge_ReturnType ret = DoIp_Dcm_Bridge_InitLargeTransfer(
            sessionId,
            response->length,
            false
        );
        
        if (ret != DOIP_DCM_BRIDGE_OK) {
            return ret;
        }

        /* Copy response to large buffer */
        memcpy(session->txBuffer, response->data, response->length);
        session->largeTransfer.transferredLength = response->length;

        /* Send as segments (simplified - single segment for now) */
        result = DoIp_Dcm_Bridge_ConvertToDoIpMessage(response, &doipMsg, session);
        
        if (result == DOIP_OK) {
            result = DoIp_SendDiagnosticMessage(session->connectionId, &doipMsg);
        }

        (void)DoIp_Dcm_Bridge_CompleteLargeTransfer(sessionId);
    } else {
        /* Standard response */
        result = DoIp_Dcm_Bridge_ConvertToDoIpMessage(response, &doipMsg, session);
        
        if (result == DOIP_OK) {
            result = DoIp_SendDiagnosticMessage(session->connectionId, &doipMsg);
        }
    }

    if (result == DOIP_OK) {
        session->responsesSent++;
        g_statistics.totalResponses++;
        session->state = DOIP_DCM_SESSION_ACTIVE;
    } else {
        g_statistics.errors++;
        session->errors++;
    }

    return (result == DOIP_OK) ? DOIP_DCM_BRIDGE_OK : DOIP_DCM_BRIDGE_ERROR;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_ProcessRequestResponse(
    uint8_t sessionId
)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    Dcm_RequestType request;
    Dcm_ResponseType response;
    Dcm_ReturnType dcmResult;

    /* Setup request */
    request.data = session->rxBuffer;
    request.length = session->rxBufferSize;
    request.sourceAddress = session->sourceAddress;
    request.addrMode = DCM_ADDR_PHYSICAL;
    request.protocol = DCM_PROTOCOL_UDS_ON_IP;
    request.timestamp = DoIp_Dcm_Bridge_GetTimestamp();

    /* Setup response */
    response.data = session->txBuffer;
    response.maxLength = session->txBufferSize;
    response.length = 0U;
    response.isNegativeResponse = false;
    response.negativeResponseCode = 0U;
    response.suppressPositiveResponse = false;

    /* Process through DCM */
    dcmResult = Dcm_ProcessRequest(&request, &response);

    session->requestsProcessed++;
    g_statistics.totalRequests++;

    if (dcmResult == DCM_E_OK) {
        /* Check if positive response should be suppressed */
        if (!response.suppressPositiveResponse) {
            /* Send response */
            DoIp_Dcm_Bridge_ReturnType bridgeResult = DoIp_Dcm_Bridge_SendResponse(
                sessionId,
                &response
            );
            
            if (bridgeResult != DOIP_DCM_BRIDGE_OK) {
                session->errors++;
                g_statistics.errors++;
            }
        }
        
        session->state = DOIP_DCM_SESSION_ACTIVE;
    } else if (dcmResult == DCM_E_PENDING) {
        /* Async operation - will continue in next MainFunction cycle */
        session->state = DOIP_DCM_SESSION_PROCESSING;
    } else {
        /* Error - send negative response */
        session->errors++;
        g_statistics.errors++;
        session->state = DOIP_DCM_SESSION_ACTIVE;
    }

    session->lastActivityTime = DoIp_Dcm_Bridge_GetTimestamp();

    return (dcmResult == DCM_E_OK || dcmResult == DCM_E_PENDING) ? 
           DOIP_DCM_BRIDGE_OK : DOIP_DCM_BRIDGE_ERROR;
}

/******************************************************************************
 * Large Packet Handling Implementation
 ******************************************************************************/

bool DoIp_Dcm_Bridge_IsLargeMessage(uint32_t messageLength)
{
    return (messageLength > DOIP_DCM_BRIDGE_BUFFER_THRESHOLD);
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_InitLargeTransfer(
    uint8_t sessionId,
    uint32_t totalLength,
    bool isReceive
)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    /* Check if already in transfer */
    if (session->largeTransfer.state != DOIP_DCM_TRANSFER_IDLE) {
        return DOIP_DCM_BRIDGE_TRANSFER_IN_PROGRESS;
    }

    /* Allocate large buffer */
    DoIp_Dcm_Bridge_ReturnType result = DoIp_Dcm_Bridge_AllocateLargeBuffer(
        session,
        isReceive
    );
    
    if (result != DOIP_DCM_BRIDGE_OK) {
        return result;
    }

    /* Initialize transfer context */
    session->largeTransfer.state = isReceive ? 
        DOIP_DCM_TRANSFER_RX_IN_PROGRESS : DOIP_DCM_TRANSFER_TX_IN_PROGRESS;
    session->largeTransfer.totalLength = totalLength;
    session->largeTransfer.transferredLength = 0U;
    session->largeTransfer.sequenceNumber = 0U;
    session->largeTransfer.isReceive = isReceive;
    
    if (isReceive) {
        session->largeTransfer.buffer = session->rxBuffer;
        session->largeTransfer.segmentSize = session->rxBufferSize;
    } else {
        session->largeTransfer.buffer = session->txBuffer;
        session->largeTransfer.segmentSize = session->txBufferSize;
    }

    session->state = DOIP_DCM_SESSION_LARGE_TRANSFER;
    g_statistics.largeTransfers++;

    /* Call user callback */
    if (g_bridgeConfig.onLargeTransferStarted != NULL) {
        g_bridgeConfig.onLargeTransferStarted(sessionId, totalLength);
    }

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_ProcessLargeSegment(
    uint8_t sessionId,
    const uint8_t *segmentData,
    uint32_t segmentLength,
    uint16_t sequenceNumber
)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    if (segmentData == NULL || segmentLength == 0U) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    if (session->largeTransfer.state != DOIP_DCM_TRANSFER_RX_IN_PROGRESS) {
        return DOIP_DCM_BRIDGE_ERROR;
    }

    /* Verify sequence number */
    if (sequenceNumber != session->largeTransfer.sequenceNumber) {
        session->largeTransfer.state = DOIP_DCM_TRANSFER_ERROR;
        return DOIP_DCM_BRIDGE_ERROR;
    }

    /* Check buffer space */
    uint32_t remainingSpace = session->largeTransfer.totalLength - 
                              session->largeTransfer.transferredLength;
    
    if (segmentLength > remainingSpace) {
        session->largeTransfer.state = DOIP_DCM_TRANSFER_ERROR;
        return DOIP_DCM_BRIDGE_ERROR;
    }

    /* Copy segment data */
    uint8_t *dest = &session->largeTransfer.buffer[session->largeTransfer.transferredLength];
    memcpy(dest, segmentData, segmentLength);

    session->largeTransfer.transferredLength += segmentLength;
    session->largeTransfer.sequenceNumber++;

    /* Check if transfer complete */
    if (session->largeTransfer.transferredLength >= session->largeTransfer.totalLength) {
        session->largeTransfer.state = DOIP_DCM_TRANSFER_RX_COMPLETE;
    }

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_GetNextResponseSegment(
    uint8_t sessionId,
    uint8_t *segmentData,
    uint32_t maxLength,
    uint32_t *segmentLength,
    uint16_t *sequenceNumber
)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    if (segmentData == NULL || segmentLength == NULL || sequenceNumber == NULL) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    if (session->largeTransfer.state != DOIP_DCM_TRANSFER_TX_IN_PROGRESS &&
        session->largeTransfer.state != DOIP_DCM_TRANSFER_TX_COMPLETE) {
        return DOIP_DCM_BRIDGE_ERROR;
    }

    /* Calculate segment to send */
    uint32_t remaining = session->largeTransfer.totalLength - 
                         session->largeTransfer.transferredLength;
    
    if (remaining == 0U) {
        *segmentLength = 0U;
        return DOIP_DCM_BRIDGE_OK;
    }

    uint32_t toSend = (remaining < maxLength) ? remaining : maxLength;

    /* Copy data to output buffer */
    uint8_t *src = &session->largeTransfer.buffer[session->largeTransfer.transferredLength];
    memcpy(segmentData, src, toSend);

    *segmentLength = toSend;
    *sequenceNumber = session->largeTransfer.sequenceNumber;

    session->largeTransfer.transferredLength += toSend;
    session->largeTransfer.sequenceNumber++;

    /* Check if complete */
    if (session->largeTransfer.transferredLength >= session->largeTransfer.totalLength) {
        session->largeTransfer.state = DOIP_DCM_TRANSFER_TX_COMPLETE;
    }

    return DOIP_DCM_BRIDGE_OK;
}

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_CompleteLargeTransfer(
    uint8_t sessionId
)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return DOIP_DCM_BRIDGE_NO_SESSION;
    }

    /* Check if transfer is complete */
    if (session->largeTransfer.state != DOIP_DCM_TRANSFER_RX_COMPLETE &&
        session->largeTransfer.state != DOIP_DCM_TRANSFER_TX_COMPLETE) {
        return DOIP_DCM_BRIDGE_ERROR;
    }

    /* If receive complete, process the message */
    if (session->largeTransfer.state == DOIP_DCM_TRANSFER_RX_COMPLETE) {
        session->rxBufferSize = session->largeTransfer.transferredLength;
        session->state = DOIP_DCM_SESSION_PROCESSING;
        
        /* Process the request */
        DoIp_Dcm_Bridge_ProcessRequestResponse(sessionId);
    }

    /* Free large buffer */
    DoIp_Dcm_Bridge_FreeLargeBuffer(session);

    /* Reset transfer context */
    session->largeTransfer.state = DOIP_DCM_TRANSFER_IDLE;
    session->largeTransfer.buffer = NULL;
    session->state = DOIP_DCM_SESSION_ACTIVE;

    return DOIP_DCM_BRIDGE_OK;
}

/******************************************************************************
 * Message Buffering Implementation
 ******************************************************************************/

DoIp_Dcm_Bridge_ReturnType DoIp_Dcm_Bridge_BufferMessage(
    uint8_t connectionId,
    const DoIp_DiagnosticMessageType *message
)
{
    if (!g_bridgeInitialized) {
        return DOIP_DCM_BRIDGE_NOT_INITIALIZED;
    }

    if (message == NULL) {
        return DOIP_DCM_BRIDGE_INVALID_PARAMETER;
    }

    /* Find free buffer entry */
    DoIp_Dcm_BufferedMessageType *entry = NULL;
    
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS; i++) {
        if (!g_messageBuffer.entries[i].inUse) {
            entry = &g_messageBuffer.entries[i];
            break;
        }
    }

    if (entry == NULL) {
        g_statistics.droppedMessages++;
        return DOIP_DCM_BRIDGE_NO_BUFFER;
    }

    /* Check data size */
    if (message->userDataLength > DOIP_DCM_BRIDGE_STD_BUFFER_SIZE) {
        return DOIP_DCM_BRIDGE_MESSAGE_TOO_LARGE;
    }

    /* Store message */
    entry->connectionId = connectionId;
    entry->sourceAddress = message->sourceAddress;
    entry->targetAddress = message->targetAddress;
    entry->dataLength = message->userDataLength;
    entry->timestamp = DoIp_Dcm_Bridge_GetTimestamp();
    entry->inUse = true;
    entry->isLargeMessage = DoIp_Dcm_Bridge_IsLargeMessage(message->userDataLength);

    /* Copy data */
    memcpy(entry->data, message->userData, message->userDataLength);

    g_messageBuffer.usedCount++;
    g_statistics.bufferedMessages++;

    return DOIP_DCM_BRIDGE_OK;
}

uint8_t DoIp_Dcm_Bridge_ProcessBufferedMessages(void)
{
    uint8_t processed = 0U;
    uint32_t currentTime = DoIp_Dcm_Bridge_GetTimestamp();

    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS; i++) {
        DoIp_Dcm_BufferedMessageType *entry = &g_messageBuffer.entries[i];
        
        if (!entry->inUse) {
            continue;
        }

        /* Check timeout */
        if ((currentTime - entry->timestamp) > g_bridgeConfig.messageTimeoutMs) {
            entry->inUse = false;
            g_messageBuffer.usedCount--;
            g_statistics.droppedMessages++;
            continue;
        }

        /* Check if we can process this message now */
        DoIp_Dcm_SessionContextType *session = NULL;
        int8_t sessionId = DoIp_Dcm_Bridge_FindSessionByConnId(entry->connectionId);
        
        if (sessionId >= 0) {
            session = &g_sessions[sessionId];
            
            /* Skip if session is busy */
            if (session->state == DOIP_DCM_SESSION_PROCESSING ||
                session->state == DOIP_DCM_SESSION_LARGE_TRANSFER) {
                continue;
            }
        }

        /* Process the message */
        DoIp_DiagnosticMessageType msg;
        msg.sourceAddress = entry->sourceAddress;
        msg.targetAddress = entry->targetAddress;
        msg.userData = entry->data;
        msg.userDataLength = (uint16_t)entry->dataLength;

        DoIp_Dcm_Bridge_ReturnType result = DoIp_Dcm_Bridge_ProcessDiagnosticMessage(
            entry->connectionId,
            &msg
        );

        /* Mark as processed (or expired) */
        entry->inUse = false;
        g_messageBuffer.usedCount--;

        if (result == DOIP_DCM_BRIDGE_OK) {
            processed++;
        }

        /* Limit processing per cycle */
        if (processed >= 4U) {
            break;
        }
    }

    return processed;
}

void DoIp_Dcm_Bridge_ClearBufferedMessages(uint8_t connectionId)
{
    for (uint8_t i = 0U; i < DOIP_DCM_BRIDGE_MAX_BUFFERED_MSGS; i++) {
        DoIp_Dcm_BufferedMessageType *entry = &g_messageBuffer.entries[i];
        
        if (entry->inUse && entry->connectionId == connectionId) {
            entry->inUse = false;
            g_messageBuffer.usedCount--;
        }
    }
}

uint8_t DoIp_Dcm_Bridge_GetBufferedMessageCount(void)
{
    return g_messageBuffer.usedCount;
}

/******************************************************************************
 * Utility Functions Implementation
 ******************************************************************************/

bool DoIp_Dcm_Bridge_IsInitialized(void)
{
    return g_bridgeInitialized;
}

void DoIp_Dcm_Bridge_GetStatistics(DoIp_Dcm_BridgeStatisticsType *stats)
{
    if (stats != NULL) {
        memcpy(stats, &g_statistics, sizeof(DoIp_Dcm_BridgeStatisticsType));
    }
}

void DoIp_Dcm_Bridge_ResetStatistics(void)
{
    memset(&g_statistics, 0, sizeof(g_statistics));
}

bool DoIp_Dcm_Bridge_HasPendingOperations(uint8_t sessionId)
{
    if (!DoIp_Dcm_Bridge_IsValidSessionId(sessionId)) {
        return false;
    }

    DoIp_Dcm_SessionContextType *session = &g_sessions[sessionId];
    
    if (!session->isActive) {
        return false;
    }

    return (session->state == DOIP_DCM_SESSION_PROCESSING ||
            session->state == DOIP_DCM_SESSION_LARGE_TRANSFER);
}

void DoIp_Dcm_Bridge_GetVersionInfo(
    uint8_t *major,
    uint8_t *minor,
    uint8_t *patch
)
{
    if (major != NULL) {
        *major = DOIP_DCM_BRIDGE_SW_MAJOR_VERSION;
    }
    if (minor != NULL) {
        *minor = DOIP_DCM_BRIDGE_SW_MINOR_VERSION;
    }
    if (patch != NULL) {
        *patch = DOIP_DCM_BRIDGE_SW_PATCH_VERSION;
    }
}

/******************************************************************************
 * DoIP Callback Implementation
 ******************************************************************************/

void DoIp_Dcm_Bridge_OnRoutingActivation(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint8_t result
)
{
    if (!g_bridgeInitialized) {
        return;
    }

    if (result == DOIP_RA_RES_SUCCESS) {
        /* Check if session already exists */
        int8_t existingSession = DoIp_Dcm_Bridge_FindSessionByConnId(connectionId);
        
        if (existingSession < 0) {
            /* Create new session */
            uint8_t sessionId;
            uint16_t targetAddress = 0x0000U;  /* Will be updated on first message */
            
            (void)DoIp_Dcm_Bridge_CreateSession(
                connectionId,
                sourceAddress,
                targetAddress,
                &sessionId
            );
        } else {
            /* Update existing session */
            DoIp_Dcm_SessionContextType *session = &g_sessions[existingSession];
            session->sourceAddress = sourceAddress;
            session->routingActivated = true;
        }
    }
}

void DoIp_Dcm_Bridge_OnDiagnosticMessage(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint16_t targetAddress,
    uint8_t *data,
    uint16_t length
)
{
    if (!g_bridgeInitialized) {
        return;
    }

    if (data == NULL || length == 0U) {
        return;
    }

    /* Build DoIP message structure */
    DoIp_DiagnosticMessageType message;
    message.sourceAddress = sourceAddress;
    message.targetAddress = targetAddress;
    message.userData = data;
    message.userDataLength = length;

    /* Try to process immediately */
    DoIp_Dcm_Bridge_ReturnType result = DoIp_Dcm_Bridge_ProcessDiagnosticMessage(
        connectionId,
        &message
    );

    /* If busy, buffer the message */
    if (result == DOIP_DCM_BRIDGE_BUSY || result == DOIP_DCM_BRIDGE_TRANSFER_IN_PROGRESS) {
        (void)DoIp_Dcm_Bridge_BufferMessage(connectionId, &message);
    }
}

void DoIp_Dcm_Bridge_OnConnectionClosed(uint8_t connectionId)
{
    if (!g_bridgeInitialized) {
        return;
    }

    /* Find and close associated session */
    int8_t sessionId = DoIp_Dcm_Bridge_FindSessionByConnId(connectionId);
    
    if (sessionId >= 0) {
        (void)DoIp_Dcm_Bridge_CloseSession((uint8_t)sessionId);
    }

    /* Clear any buffered messages */
    DoIp_Dcm_Bridge_ClearBufferedMessages(connectionId);
}

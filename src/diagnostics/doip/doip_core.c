/******************************************************************************
 * @file    doip_core.c
 * @brief   ISO 13400-2 DoIP Protocol Stack Core Implementation
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "doip_core.h"
#include <string.h>
#include <stddef.h>

/******************************************************************************
 * Module Configuration
 ******************************************************************************/

#define DOIP_MAX_CONNECTIONS            8U
#define DOIP_MAX_MESSAGE_SIZE           65535U
#define DOIP_HEADER_SIZE                8U
#define DOIP_RX_BUFFER_SIZE             4096U

/******************************************************************************
 * Module Global Variables
 ******************************************************************************/

static DoIp_ContextType g_doipContext;
static DoIp_ConnectionContextType g_connectionContexts[DOIP_MAX_CONNECTIONS];
static uint8_t g_rxBuffers[DOIP_MAX_CONNECTIONS][DOIP_RX_BUFFER_SIZE];

/* Callbacks */
static DoIp_RoutingActivationCallback g_routingActivationCallback = NULL;
static DoIp_DiagnosticCallback g_diagnosticCallback = NULL;
static DoIp_ConnectionCallback g_connectionCallback = NULL;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get current time in milliseconds
 */
static uint32_t DoIp_GetCurrentTime(void)
{
    static uint32_t counter = 0;
    return counter++;
}

/**
 * @brief Validate connection ID
 */
static bool DoIp_IsValidConnection(uint8_t connectionId)
{
    return (connectionId < g_doipContext.numConnections) &&
           (connectionId < DOIP_MAX_CONNECTIONS);
}

/**
 * @brief Find free connection slot
 */
static int8_t DoIp_FindFreeConnection(void)
{
    for (uint8_t i = 0U; i < DOIP_MAX_CONNECTIONS; i++) {
        if (g_connectionContexts[i].state == DOIP_STATE_INIT) {
            return (int8_t)i;
        }
    }
    return -1;
}

/**
 * @brief Count active routing activations
 */
static uint8_t DoIp_CountActiveRoutingActivations(void)
{
    uint8_t count = 0U;
    for (uint8_t i = 0U; i < DOIP_MAX_CONNECTIONS; i++) {
        if (g_connectionContexts[i].state == DOIP_STATE_REGISTERED ||
            g_connectionContexts[i].state == DOIP_STATE_DIAGNOSTIC_SESSION) {
            count++;
        }
    }
    return count;
}

/******************************************************************************
 * Byte Conversion Functions
 ******************************************************************************/

uint16_t DoIp_BytesToUint16(const uint8_t *bytes)
{
    return ((uint16_t)bytes[0] << 8) | bytes[1];
}

uint32_t DoIp_BytesToUint32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) |
           bytes[3];
}

void DoIp_Uint16ToBytes(uint16_t value, uint8_t *bytes)
{
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)(value & 0xFFU);
}

void DoIp_Uint32ToBytes(uint32_t value, uint8_t *bytes)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)(value & 0xFFU);
}

/******************************************************************************
 * Header Functions
 ******************************************************************************/

DoIp_ReturnType DoIp_BuildHeader(
    uint16_t payloadType,
    uint32_t payloadLength,
    uint8_t *buffer
)
{
    if (buffer == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    buffer[0] = DOIP_PROTOCOL_VERSION;
    buffer[1] = DOIP_PROTOCOL_VERSION_INV;
    buffer[2] = (uint8_t)(payloadType >> 8);
    buffer[3] = (uint8_t)(payloadType & 0xFFU);
    buffer[4] = (uint8_t)(payloadLength >> 24);
    buffer[5] = (uint8_t)(payloadLength >> 16);
    buffer[6] = (uint8_t)(payloadLength >> 8);
    buffer[7] = (uint8_t)(payloadLength & 0xFFU);
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_ParseHeader(
    const uint8_t *buffer,
    DoIp_HeaderType *header
)
{
    if (buffer == NULL || header == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    header->protocolVersion = buffer[0];
    header->protocolVersionInv = buffer[1];
    header->payloadType = DoIp_BytesToUint16(&buffer[2]);
    header->payloadLength = DoIp_BytesToUint32(&buffer[4]);
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_ValidateHeader(const DoIp_HeaderType *header)
{
    if (header == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    /* Check protocol version */
    if (header->protocolVersion != DOIP_PROTOCOL_VERSION) {
        return DOIP_ERROR;
    }
    
    /* Check inverse protocol version */
    if ((uint8_t)(header->protocolVersion + header->protocolVersionInv) != 0xFFU) {
        return DOIP_ERROR;
    }
    
    /* Check payload type */
    if (!DoIp_IsValidPayloadType(header->payloadType)) {
        return DOIP_ERROR;
    }
    
    /* Check payload length */
    if (header->payloadLength > DOIP_MAX_MESSAGE_SIZE) {
        return DOIP_ERROR;
    }
    
    return DOIP_OK;
}

bool DoIp_IsValidPayloadType(uint16_t payloadType)
{
    switch (payloadType) {
        case DOIP_PT_VID_REQUEST:
        case DOIP_PT_VID_REQUEST_EID:
        case DOIP_PT_VID_REQUEST_VIN:
        case DOIP_PT_VEHICLE_ANNOUNCEMENT:
        case DOIP_PT_ROUTING_ACTIVATION_REQ:
        case DOIP_PT_ROUTING_ACTIVATION_RES:
        case DOIP_PT_ALIVE_CHECK_REQ:
        case DOIP_PT_ALIVE_CHECK_RES:
        case DOIP_PT_ENTITY_STATUS_REQ:
        case DOIP_PT_ENTITY_STATUS_RES:
        case DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_REQ:
        case DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_RES:
        case DOIP_PT_DIAGNOSTIC_MSG:
        case DOIP_PT_DIAGNOSTIC_ACK:
        case DOIP_PT_DIAGNOSTIC_NACK:
            return true;
        default:
            return false;
    }
}

const char* DoIp_GetPayloadTypeName(uint16_t payloadType)
{
    switch (payloadType) {
        case DOIP_PT_VID_REQUEST: return "VID_REQUEST";
        case DOIP_PT_VID_REQUEST_EID: return "VID_REQUEST_EID";
        case DOIP_PT_VID_REQUEST_VIN: return "VID_REQUEST_VIN";
        case DOIP_PT_VEHICLE_ANNOUNCEMENT: return "VEHICLE_ANNOUNCEMENT";
        case DOIP_PT_ROUTING_ACTIVATION_REQ: return "ROUTING_ACTIVATION_REQ";
        case DOIP_PT_ROUTING_ACTIVATION_RES: return "ROUTING_ACTIVATION_RES";
        case DOIP_PT_ALIVE_CHECK_REQ: return "ALIVE_CHECK_REQ";
        case DOIP_PT_ALIVE_CHECK_RES: return "ALIVE_CHECK_RES";
        case DOIP_PT_ENTITY_STATUS_REQ: return "ENTITY_STATUS_REQ";
        case DOIP_PT_ENTITY_STATUS_RES: return "ENTITY_STATUS_RES";
        case DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_REQ: return "POWER_MODE_INFO_REQ";
        case DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_RES: return "POWER_MODE_INFO_RES";
        case DOIP_PT_DIAGNOSTIC_MSG: return "DIAGNOSTIC_MSG";
        case DOIP_PT_DIAGNOSTIC_ACK: return "DIAGNOSTIC_ACK";
        case DOIP_PT_DIAGNOSTIC_NACK: return "DIAGNOSTIC_NACK";
        default: return "UNKNOWN";
    }
}

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

DoIp_ReturnType DoIp_Init(
    const DoIp_EntityConfigType *entityConfig,
    const DoIp_ConnectionConfigType *connections,
    uint8_t numConnections
)
{
    if (entityConfig == NULL || connections == NULL || numConnections == 0U ||
        numConnections > DOIP_MAX_CONNECTIONS) {
        return DOIP_INVALID_PARAMETER;
    }
    
    memset(&g_doipContext, 0, sizeof(g_doipContext));
    memset(g_connectionContexts, 0, sizeof(g_connectionContexts));
    
    g_doipContext.entityConfig = entityConfig;
    g_doipContext.connections = connections;
    g_doipContext.numConnections = numConnections;
    g_doipContext.connectionContexts = g_connectionContexts;
    
    /* Initialize connection contexts */
    for (uint8_t i = 0U; i < DOIP_MAX_CONNECTIONS; i++) {
        g_connectionContexts[i].connectionId = i;
        g_connectionContexts[i].state = DOIP_STATE_INIT;
        g_connectionContexts[i].socketHandle = 0xFFFFFFFFU;
    }
    
    g_doipContext.initialized = true;
    
    return DOIP_OK;
}

void DoIp_DeInit(void)
{
    /* Close all connections */
    for (uint8_t i = 0U; i < DOIP_MAX_CONNECTIONS; i++) {
        if (g_connectionContexts[i].state != DOIP_STATE_INIT) {
            DoIp_CloseConnection(i);
        }
    }
    
    memset(&g_doipContext, 0, sizeof(g_doipContext));
    memset(g_connectionContexts, 0, sizeof(g_connectionContexts));
}

DoIp_ReturnType DoIp_Start(void)
{
    if (!g_doipContext.initialized) {
        return DOIP_NOT_INITIALIZED;
    }
    
    /* Create UDP socket for vehicle discovery */
    /* DoIp_CreateUdpSocket(DOIP_UDP_DISCOVERY_PORT, &g_doipContext.udpSocket); */
    
    /* Create TCP listen socket */
    /* DoIp_CreateTcpSocket(DOIP_TCP_DATA_PORT, &g_doipContext.tcpListenSocket); */
    
    g_doipContext.discoveryEnabled = true;
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_Stop(void)
{
    /* Close sockets */
    /* DoIp_CloseSocket(g_doipContext.udpSocket); */
    /* DoIp_CloseSocket(g_doipContext.tcpListenSocket); */
    
    g_doipContext.discoveryEnabled = false;
    
    return DOIP_OK;
}

/******************************************************************************
 * Vehicle Discovery Functions
 ******************************************************************************/

DoIp_ReturnType DoIp_BuildVehicleAnnouncement(
    const DoIp_VehicleAnnouncementType *announcement,
    uint8_t *buffer,
    uint16_t *length
)
{
    if (announcement == NULL || buffer == NULL || length == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    /* Build header */
    uint16_t payloadLength = 32U;  /* VIN(17) + LA(2) + EID(6) + GID(6) + FA(1) */
    if (g_doipContext.entityConfig->vinSyncRequired) {
        payloadLength++;  /* Add sync status byte */
    }
    
    DoIp_BuildHeader(DOIP_PT_VEHICLE_ANNOUNCEMENT, payloadLength, buffer);
    
    /* Build payload */
    uint16_t idx = DOIP_HEADER_SIZE;
    
    /* VIN */
    memcpy(&buffer[idx], announcement->vin, 17);
    idx += 17;
    
    /* Logical Address */
    buffer[idx++] = announcement->logicalAddress[0];
    buffer[idx++] = announcement->logicalAddress[1];
    
    /* EID */
    memcpy(&buffer[idx], announcement->eid, 6);
    idx += 6;
    
    /* GID */
    memcpy(&buffer[idx], announcement->gid, 6);
    idx += 6;
    
    /* Further Action */
    buffer[idx++] = announcement->furtherAction;
    
    /* Sync Status (if required) */
    if (g_doipContext.entityConfig->vinSyncRequired) {
        buffer[idx++] = announcement->syncStatus;
    }
    
    *length = idx;
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_SendVehicleAnnouncement(uint8_t connectionId)
{
    if (!DoIp_IsValidConnection(connectionId)) {
        return DOIP_INVALID_PARAMETER;
    }
    
    DoIp_VehicleAnnouncementType announcement;
    uint8_t buffer[DOIP_HEADER_SIZE + 33];
    uint16_t length;
    
    /* Fill announcement data */
    memcpy(announcement.vin, g_doipContext.entityConfig->vin, 17);
    DoIp_Uint16ToBytes(g_doipContext.entityConfig->logicalAddress, announcement.logicalAddress);
    memcpy(announcement.eid, g_doipContext.entityConfig->eid, 6);
    memcpy(announcement.gid, g_doipContext.entityConfig->gid, 6);
    announcement.furtherAction = g_doipContext.entityConfig->furtherAction;
    announcement.syncStatus = 0x00U;  /* VIN and GID synchronized */
    
    DoIp_BuildVehicleAnnouncement(&announcement, buffer, &length);
    
    /* Send via UDP multicast */
    DoIp_ReturnType result = DoIp_SocketSend(
        g_doipContext.udpSocket,
        buffer,
        length
    );
    
    if (result == DOIP_OK) {
        g_doipContext.txCount++;
    }
    
    return result;
}

DoIp_ReturnType DoIp_BroadcastVehicleAnnouncement(void)
{
    /* Send multiple vehicle announcements */
    for (uint8_t i = 0U; i < DOIP_DEFAULT_A_ANNOUNCE_NUM; i++) {
        DoIp_SendVehicleAnnouncement(0);
    }
    
    return DOIP_OK;
}

/******************************************************************************
 * Routing Activation Functions
 ******************************************************************************/

DoIp_ReturnType DoIp_BuildRoutingActivationResponse(
    const DoIp_RoutingActivationResponseType *response,
    uint8_t *buffer,
    uint16_t *length
)
{
    if (response == NULL || buffer == NULL || length == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    uint16_t payloadLength = 9U;  /* LA Tester(2) + LA DoIP(2) + Code(1) + Reserved(4) */
    
    DoIp_BuildHeader(DOIP_PT_ROUTING_ACTIVATION_RES, payloadLength, buffer);
    
    uint16_t idx = DOIP_HEADER_SIZE;
    
    /* Logical Address Tester */
    DoIp_Uint16ToBytes(response->logicalAddressTester, &buffer[idx]);
    idx += 2;
    
    /* Logical Address DoIP Entity */
    DoIp_Uint16ToBytes(response->logicalAddressDoip, &buffer[idx]);
    idx += 2;
    
    /* Response Code */
    buffer[idx++] = response->responseCode;
    
    /* Reserved (4 bytes) */
    buffer[idx++] = 0x00U;
    buffer[idx++] = 0x00U;
    buffer[idx++] = 0x00U;
    buffer[idx++] = 0x00U;
    
    *length = idx;
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_ProcessRoutingActivation(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint8_t activationType,
    uint8_t *responseCode
)
{
    if (!DoIp_IsValidConnection(connectionId) || responseCode == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    DoIp_ConnectionContextType *context = &g_connectionContexts[connectionId];
    const DoIp_ConnectionConfigType *config = &g_doipContext.connections[connectionId];
    
    /* Check if source address is valid */
    if (!DoIp_IsValidSourceAddress(sourceAddress)) {
        *responseCode = DOIP_RA_RES_UNSUPPORTED_SA;
        return DOIP_OK;
    }
    
    /* Check if already registered */
    if (context->state == DOIP_STATE_REGISTERED || 
        context->state == DOIP_STATE_DIAGNOSTIC_SESSION) {
        /* Allow re-registration with same address */
        if (context->registeredSourceAddress != sourceAddress) {
            *responseCode = DOIP_RA_RES_NO_RESOURCES;
            return DOIP_OK;
        }
    }
    
    /* Check max routing activations */
    if (DoIp_CountActiveRoutingActivations() >= config->maxRoutingActivations) {
        *responseCode = DOIP_RA_RES_NO_RESOURCES;
        return DOIP_OK;
    }
    
    /* Check activation type */
    if (activationType > DOIP_RA_VEHICLE_MANUFACTURER) {
        *responseCode = DOIP_RA_RES_RA_TYPE_UNSUPPORTED;
        return DOIP_OK;
    }
    
    /* Call user callback for additional validation */
    if (g_routingActivationCallback != NULL) {
        bool approved = g_routingActivationCallback(
            sourceAddress,
            activationType,
            NULL,  /* Authentication data */
            0,
            NULL,  /* Confirmation data */
            0
        );
        
        if (!approved) {
            *responseCode = DOIP_RA_RES_CONFIRMATION_NOT_RECEIVED;
            return DOIP_OK;
        }
    }
    
    /* Registration successful */
    context->registeredSourceAddress = sourceAddress;
    context->activationType = activationType;
    context->state = DOIP_STATE_REGISTERED;
    context->authenticated = true;
    *responseCode = DOIP_RA_RES_SUCCESS;
    
    g_doipContext.routingActivations++;
    g_doipContext.routingActive = true;
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_SendRoutingActivationResponse(
    uint8_t connectionId,
    const DoIp_RoutingActivationResponseType *response
)
{
    if (!DoIp_IsValidConnection(connectionId) || response == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    uint8_t buffer[DOIP_HEADER_SIZE + 9U + DOIP_MAX_OEM_DATA];
    uint16_t length;
    
    DoIp_BuildRoutingActivationResponse(response, buffer, &length);
    
    return DoIp_SocketSend(
        g_connectionContexts[connectionId].socketHandle,
        buffer,
        length
    );
}

/******************************************************************************
 * Diagnostic Message Functions
 ******************************************************************************/

DoIp_ReturnType DoIp_BuildDiagnosticMessage(
    const DoIp_DiagnosticMessageType *message,
    uint8_t *buffer,
    uint16_t *length
)
{
    if (message == NULL || buffer == NULL || length == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    uint32_t payloadLength = 4U + message->userDataLength;  /* SA + TA + data */
    
    DoIp_BuildHeader(DOIP_PT_DIAGNOSTIC_MSG, payloadLength, buffer);
    
    uint16_t idx = DOIP_HEADER_SIZE;
    
    /* Source Address */
    DoIp_Uint16ToBytes(message->sourceAddress, &buffer[idx]);
    idx += 2;
    
    /* Target Address */
    DoIp_Uint16ToBytes(message->targetAddress, &buffer[idx]);
    idx += 2;
    
    /* User Data */
    memcpy(&buffer[idx], message->userData, message->userDataLength);
    idx += message->userDataLength;
    
    *length = idx;
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_SendDiagnosticMessage(
    uint8_t connectionId,
    const DoIp_DiagnosticMessageType *diagnosticMessage
)
{
    if (!DoIp_IsValidConnection(connectionId) || diagnosticMessage == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    uint8_t buffer[DOIP_HEADER_SIZE + 4U + DOIP_RX_BUFFER_SIZE];
    uint16_t length;
    
    DoIp_BuildDiagnosticMessage(diagnosticMessage, buffer, &length);
    
    DoIp_ReturnType result = DoIp_SocketSend(
        g_connectionContexts[connectionId].socketHandle,
        buffer,
        length
    );
    
    if (result == DOIP_OK) {
        g_doipContext.diagnosticMsgs++;
        g_doipContext.txCount++;
    }
    
    return result;
}

DoIp_ReturnType DoIp_BuildDiagnosticAck(
    const DoIp_DiagnosticAckType *ack,
    bool isPositive,
    uint8_t *buffer,
    uint16_t *length
)
{
    if (ack == NULL || buffer == NULL || length == NULL) {
        return DOIP_INVALID_PARAMETER;
    }
    
    uint16_t payloadType = isPositive ? DOIP_PT_DIAGNOSTIC_ACK : DOIP_PT_DIAGNOSTIC_NACK;
    uint32_t payloadLength = 5U;  /* SA + TA + Code */
    
    if (isPositive && ack->previousData != NULL) {
        payloadLength += ack->previousDataLength;
    }
    
    DoIp_BuildHeader(payloadType, payloadLength, buffer);
    
    uint16_t idx = DOIP_HEADER_SIZE;
    
    /* Source Address */
    DoIp_Uint16ToBytes(ack->sourceAddress, &buffer[idx]);
    idx += 2;
    
    /* Target Address */
    DoIp_Uint16ToBytes(ack->targetAddress, &buffer[idx]);
    idx += 2;
    
    /* Ack Code */
    buffer[idx++] = ack->ackCode;
    
    /* Previous diagnostic message data (for ACK only) */
    if (isPositive && ack->previousData != NULL) {
        memcpy(&buffer[idx], ack->previousData, ack->previousDataLength);
        idx += ack->previousDataLength;
    }
    
    *length = idx;
    
    return DOIP_OK;
}

DoIp_ReturnType DoIp_SendDiagnosticAck(
    uint8_t connectionId,
    const DoIp_DiagnosticAckType *ack
)
{
    uint8_t buffer[DOIP_HEADER_SIZE + 5U + DOIP_RX_BUFFER_SIZE];
    uint16_t length;
    
    DoIp_BuildDiagnosticAck(ack, true, buffer, &length);
    
    return DoIp_SocketSend(
        g_connectionContexts[connectionId].socketHandle,
        buffer,
        length
    );
}

DoIp_ReturnType DoIp_SendDiagnosticNack(
    uint8_t connectionId,
    const DoIp_DiagnosticAckType *nack
)
{
    uint8_t buffer[DOIP_HEADER_SIZE + 5U];
    uint16_t length;
    
    DoIp_BuildDiagnosticAck(nack, false, buffer, &length);
    
    return DoIp_SocketSend(
        g_connectionContexts[connectionId].socketHandle,
        buffer,
        length
    );
}

/******************************************************************************
 * Connection Management
 ******************************************************************************/

void DoIp_CloseConnection(uint8_t connectionId)
{
    if (!DoIp_IsValidConnection(connectionId)) {
        return;
    }
    
    DoIp_ConnectionContextType *context = &g_connectionContexts[connectionId];
    
    /* Close socket */
    if (context->socketHandle != 0xFFFFFFFFU) {
        DoIp_CloseSocket(context->socketHandle);
    }
    
    /* Reset context */
    context->state = DOIP_STATE_INIT;
    context->socketHandle = 0xFFFFFFFFU;
    context->registeredSourceAddress = 0U;
    context->authenticated = false;
    
    g_doipContext.routingActive = (DoIp_CountActiveRoutingActivations() > 0);
    
    if (g_connectionCallback != NULL) {
        g_connectionCallback(connectionId, false);
    }
}

DoIp_StateType DoIp_GetConnectionState(uint8_t connectionId)
{
    if (!DoIp_IsValidConnection(connectionId)) {
        return DOIP_STATE_ERROR;
    }
    
    return g_connectionContexts[connectionId].state;
}

bool DoIp_IsConnectionRegistered(uint8_t connectionId)
{
    if (!DoIp_IsValidConnection(connectionId)) {
        return false;
    }
    
    DoIp_StateType state = g_connectionContexts[connectionId].state;
    return (state == DOIP_STATE_REGISTERED || state == DOIP_STATE_DIAGNOSTIC_SESSION);
}

DoIp_ConnectionContextType* DoIp_GetConnectionContext(uint8_t connectionId)
{
    if (!DoIp_IsValidConnection(connectionId)) {
        return NULL;
    }
    
    return &g_connectionContexts[connectionId];
}

/******************************************************************************
 * Socket Interface Stubs (Platform Specific)
 ******************************************************************************/

DoIp_ReturnType DoIp_CreateUdpSocket(uint16_t port, uint32_t *socketHandle)
{
    (void)port;
    (void)socketHandle;
    /* Platform-specific implementation */
    return DOIP_OK;
}

DoIp_ReturnType DoIp_CreateTcpSocket(uint16_t port, uint32_t *socketHandle)
{
    (void)port;
    (void)socketHandle;
    /* Platform-specific implementation */
    return DOIP_OK;
}

DoIp_ReturnType DoIp_AcceptTcpConnection(uint32_t listenSocket, uint32_t *clientSocket)
{
    (void)listenSocket;
    (void)clientSocket;
    /* Platform-specific implementation */
    return DOIP_OK;
}

DoIp_ReturnType DoIp_SocketSend(uint32_t socketHandle, const uint8_t *data, uint16_t length)
{
    (void)socketHandle;
    (void)data;
    (void)length;
    /* Platform-specific implementation */
    return DOIP_OK;
}

DoIp_ReturnType DoIp_SocketReceive(
    uint32_t socketHandle,
    uint8_t *buffer,
    uint16_t maxLength,
    uint16_t *receivedLength
)
{
    (void)socketHandle;
    (void)buffer;
    (void)maxLength;
    (void)receivedLength;
    /* Platform-specific implementation */
    return DOIP_OK;
}

void DoIp_CloseSocket(uint32_t socketHandle)
{
    (void)socketHandle;
    /* Platform-specific implementation */
}

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

void DoIp_RegisterRoutingActivationCallback(DoIp_RoutingActivationCallback callback)
{
    g_routingActivationCallback = callback;
}

void DoIp_RegisterDiagnosticCallback(DoIp_DiagnosticCallback callback)
{
    g_diagnosticCallback = callback;
}

void DoIp_RegisterConnectionCallback(DoIp_ConnectionCallback callback)
{
    g_connectionCallback = callback;
}

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

bool DoIp_IsValidSourceAddress(uint16_t sourceAddress)
{
    /* Valid tester source addresses: 0x0E00-0x0EFF */
    return (sourceAddress >= 0x0E00U && sourceAddress <= 0x0EFFU);
}

bool DoIp_IsValidTargetAddress(uint16_t targetAddress)
{
    /* Valid target addresses: 0x0001-0x0DFF and 0x1000-0x7FFF */
    return ((targetAddress >= 0x0001U && targetAddress <= 0x0DFFU) ||
            (targetAddress >= 0x1000U && targetAddress <= 0x7FFFU));
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

void DoIp_MainFunction(void)
{
    if (!g_doipContext.initialized) {
        return;
    }
    
    /* Check for new TCP connections */
    uint32_t newSocket;
    if (DoIp_AcceptTcpConnection(g_doipContext.tcpListenSocket, &newSocket) == DOIP_OK) {
        int8_t connId = DoIp_FindFreeConnection();
        if (connId >= 0) {
            g_connectionContexts[connId].socketHandle = newSocket;
            g_connectionContexts[connId].state = DOIP_STATE_VEHICLE_DISCOVERY;
            g_connectionContexts[connId].connectionStart = DoIp_GetCurrentTime();
            
            if (g_connectionCallback != NULL) {
                g_connectionCallback((uint8_t)connId, true);
            }
        }
    }
    
    /* Process received data on connections */
    for (uint8_t i = 0U; i < DOIP_MAX_CONNECTIONS; i++) {
        if (g_connectionContexts[i].state != DOIP_STATE_INIT) {
            uint16_t receivedLength;
            DoIp_SocketReceive(
                g_connectionContexts[i].socketHandle,
                g_rxBuffers[i],
                DOIP_RX_BUFFER_SIZE,
                &receivedLength
            );
            
            if (receivedLength > 0U) {
                g_doipContext.rxCount++;
                /* Process received message */
                /* Parse header and dispatch */
            }
        }
    }
    
    /* Perform alive checks */
    DoIp_PerformAliveChecks();
}

void DoIp_PerformAliveChecks(void)
{
    uint32_t currentTime = DoIp_GetCurrentTime();
    
    for (uint8_t i = 0U; i < DOIP_MAX_CONNECTIONS; i++) {
        DoIp_ConnectionContextType *context = &g_connectionContexts[i];
        
        if (context->state == DOIP_STATE_REGISTERED || 
            context->state == DOIP_STATE_DIAGNOSTIC_SESSION) {
            
            const DoIp_ConnectionConfigType *config = &g_doipContext.connections[i];
            
            /* Check inactivity timeout */
            if ((currentTime - context->lastActivity) > config->inactivityTimeout) {
                if (context->aliveCheckPending) {
                    /* Close connection */
                    DoIp_CloseConnection(i);
                } else {
                    /* Send alive check request */
                    DoIp_SendAliveCheckRequest(i);
                    context->aliveCheckPending = 1U;
                }
            }
        }
    }
}

DoIp_ReturnType DoIp_SendAliveCheckRequest(uint8_t connectionId)
{
    if (!DoIp_IsValidConnection(connectionId)) {
        return DOIP_INVALID_PARAMETER;
    }
    
    uint8_t buffer[DOIP_HEADER_SIZE];
    uint16_t length;
    
    DoIp_BuildHeader(DOIP_PT_ALIVE_CHECK_REQ, 0U, buffer);
    length = DOIP_HEADER_SIZE;
    
    return DoIp_SocketSend(
        g_connectionContexts[connectionId].socketHandle,
        buffer,
        length
    );
}

/******************************************************************************
 * Statistics Functions
 ******************************************************************************/

void DoIp_GetStats(
    uint32_t *rxCount,
    uint32_t *txCount,
    uint32_t *routingActivations,
    uint32_t *diagnosticMsgs
)
{
    if (rxCount != NULL) {
        *rxCount = g_doipContext.rxCount;
    }
    if (txCount != NULL) {
        *txCount = g_doipContext.txCount;
    }
    if (routingActivations != NULL) {
        *routingActivations = g_doipContext.routingActivations;
    }
    if (diagnosticMsgs != NULL) {
        *diagnosticMsgs = g_doipContext.diagnosticMsgs;
    }
}

void DoIp_ResetStats(void)
{
    g_doipContext.rxCount = 0U;
    g_doipContext.txCount = 0U;
    g_doipContext.routingActivations = 0U;
    g_doipContext.diagnosticMsgs = 0U;
}

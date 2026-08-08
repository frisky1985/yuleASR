/******************************************************************************
 * @file    doip_core.h
 * @brief   ISO 13400-2 DoIP Protocol Stack Core
 *
 * ISO 13400-2:2019 compliant
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DOIP_CORE_H
#define DOIP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "doip_types.h"

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

/**
 * @brief Initialize DoIP module
 * @param entityConfig Entity configuration
 * @param connections Connection configurations
 * @param numConnections Number of connections
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_Init(
    const DoIp_EntityConfigType *entityConfig,
    const DoIp_ConnectionConfigType *connections,
    uint8_t numConnections
);

/**
 * @brief Deinitialize DoIP module
 */
void DoIp_DeInit(void);

/**
 * @brief Start DoIP services (bind sockets, start listening)
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_Start(void);

/**
 * @brief Stop DoIP services
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_Stop(void);

/**
 * @brief Main processing function (to be called cyclically)
 */
void DoIp_MainFunction(void);

/******************************************************************************
 * Vehicle Discovery Functions
 ******************************************************************************/

/**
 * @brief Send vehicle announcement message
 * @param connectionId Connection ID
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendVehicleAnnouncement(uint8_t connectionId);

/**
 * @brief Broadcast vehicle announcement (UDP)
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_BroadcastVehicleAnnouncement(void);

/**
 * @brief Handle vehicle identification request
 * @param requestData Request data buffer
 * @param requestLength Request length
 * @param responseBuffer Response buffer
 * @param responseLength Output response length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_HandleVehicleIdRequest(
    const uint8_t *requestData,
    uint16_t requestLength,
    uint8_t *responseBuffer,
    uint16_t *responseLength
);

/**
 * @brief Build vehicle announcement message
 * @param announcement Announcement data
 * @param buffer Output buffer
 * @param length Output length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_BuildVehicleAnnouncement(
    const DoIp_VehicleAnnouncementType *announcement,
    uint8_t *buffer,
    uint16_t *length
);

/******************************************************************************
 * Routing Activation Functions
 ******************************************************************************/

/**
 * @brief Handle routing activation request
 * @param connectionId Connection ID
 * @param request Request data
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_HandleRoutingActivationRequest(
    uint8_t connectionId,
    const DoIp_RoutingActivationRequestType *request
);

/**
 * @brief Send routing activation response
 * @param connectionId Connection ID
 * @param response Response data
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendRoutingActivationResponse(
    uint8_t connectionId,
    const DoIp_RoutingActivationResponseType *response
);

/**
 * @brief Build routing activation response
 * @param response Response data
 * @param buffer Output buffer
 * @param length Output length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_BuildRoutingActivationResponse(
    const DoIp_RoutingActivationResponseType *response,
    uint8_t *buffer,
    uint16_t *length
);

/**
 * @brief Process routing activation request
 * @param connectionId Connection ID
 * @param sourceAddress Source address
 * @param activationType Activation type
 * @param responseCode Output response code
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_ProcessRoutingActivation(
    uint8_t connectionId,
    uint16_t sourceAddress,
    uint8_t activationType,
    uint8_t *responseCode
);

/******************************************************************************
 * Alive Check Functions
 ******************************************************************************/

/**
 * @brief Send alive check request
 * @param connectionId Connection ID
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendAliveCheckRequest(uint8_t connectionId);

/**
 * @brief Handle alive check response
 * @param connectionId Connection ID
 * @param responseData Response data
 * @param responseLength Response length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_HandleAliveCheckResponse(
    uint8_t connectionId,
    const uint8_t *responseData,
    uint16_t responseLength
);

/**
 * @brief Check for inactive connections and perform alive check
 */
void DoIp_PerformAliveChecks(void);

/******************************************************************************
 * Diagnostic Message Functions
 ******************************************************************************/

/**
 * @brief Send diagnostic message
 * @param connectionId Connection ID
 * @param diagnosticMessage Diagnostic message data
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendDiagnosticMessage(
    uint8_t connectionId,
    const DoIp_DiagnosticMessageType *diagnosticMessage
);

/**
 * @brief Handle received diagnostic message
 * @param connectionId Connection ID
 * @param messageData Message data buffer
 * @param messageLength Message length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_HandleDiagnosticMessage(
    uint8_t connectionId,
    const uint8_t *messageData,
    uint16_t messageLength
);

/**
 * @brief Send diagnostic message acknowledgment
 * @param connectionId Connection ID
 * @param ack Acknowledgment data
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendDiagnosticAck(
    uint8_t connectionId,
    const DoIp_DiagnosticAckType *ack
);

/**
 * @brief Send diagnostic message negative acknowledgment
 * @param connectionId Connection ID
 * @param nack Negative acknowledgment data
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendDiagnosticNack(
    uint8_t connectionId,
    const DoIp_DiagnosticAckType *nack
);

/******************************************************************************
 * Message Building Functions
 ******************************************************************************/

/**
 * @brief Build DoIP header
 * @param payloadType Payload type
 * @param payloadLength Payload length
 * @param buffer Output buffer
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_BuildHeader(
    uint16_t payloadType,
    uint32_t payloadLength,
    uint8_t *buffer
);

/**
 * @brief Parse DoIP header
 * @param buffer Input buffer
 * @param header Output header structure
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_ParseHeader(
    const uint8_t *buffer,
    DoIp_HeaderType *header
);

/**
 * @brief Build diagnostic message
 * @param message Diagnostic message data
 * @param buffer Output buffer
 * @param length Output length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_BuildDiagnosticMessage(
    const DoIp_DiagnosticMessageType *message,
    uint8_t *buffer,
    uint16_t *length
);

/**
 * @brief Build diagnostic acknowledgment
 * @param ack Acknowledgment data
 * @param isPositive True for ACK, false for NACK
 * @param buffer Output buffer
 * @param length Output length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_BuildDiagnosticAck(
    const DoIp_DiagnosticAckType *ack,
    bool isPositive,
    uint8_t *buffer,
    uint16_t *length
);

/******************************************************************************
 * Connection Management
 ******************************************************************************/

/**
 * @brief Accept new TCP connection
 * @param connectionId Output connection ID
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_AcceptConnection(uint8_t *connectionId);

/**
 * @brief Close TCP connection
 * @param connectionId Connection ID
 */
void DoIp_CloseConnection(uint8_t connectionId);

/**
 * @brief Get connection state
 * @param connectionId Connection ID
 * @return Connection state
 */
DoIp_StateType DoIp_GetConnectionState(uint8_t connectionId);

/**
 * @brief Check if connection is registered
 * @param connectionId Connection ID
 * @return true if registered
 */
bool DoIp_IsConnectionRegistered(uint8_t connectionId);

/**
 * @brief Get connection context
 * @param connectionId Connection ID
 * @return Connection context pointer or NULL
 */
DoIp_ConnectionContextType* DoIp_GetConnectionContext(uint8_t connectionId);

/******************************************************************************
 * Socket Interface (Platform Abstraction)
 ******************************************************************************/

/**
 * @brief Create UDP socket
 * @param port Port number
 * @param socketHandle Output socket handle
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_CreateUdpSocket(uint16_t port, uint32_t *socketHandle);

/**
 * @brief Create TCP listen socket
 * @param port Port number
 * @param socketHandle Output socket handle
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_CreateTcpSocket(uint16_t port, uint32_t *socketHandle);

/**
 * @brief Accept TCP connection
 * @param listenSocket Listen socket handle
 * @param clientSocket Output client socket handle
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_AcceptTcpConnection(
    uint32_t listenSocket,
    uint32_t *clientSocket
);

/**
 * @brief Send data on socket
 * @param socketHandle Socket handle
 * @param data Data buffer
 * @param length Data length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SocketSend(
    uint32_t socketHandle,
    const uint8_t *data,
    uint16_t length
);

/**
 * @brief Receive data from socket
 * @param socketHandle Socket handle
 * @param buffer Receive buffer
 * @param maxLength Maximum length
 * @param receivedLength Output received length
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SocketReceive(
    uint32_t socketHandle,
    uint8_t *buffer,
    uint16_t maxLength,
    uint16_t *receivedLength
);

/**
 * @brief Close socket
 * @param socketHandle Socket handle
 */
void DoIp_CloseSocket(uint32_t socketHandle);

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

/**
 * @brief Register routing activation callback
 * @param callback Callback function
 */
void DoIp_RegisterRoutingActivationCallback(DoIp_RoutingActivationCallback callback);

/**
 * @brief Register diagnostic message callback
 * @param callback Callback function
 */
void DoIp_RegisterDiagnosticCallback(DoIp_DiagnosticCallback callback);

/**
 * @brief Register connection callback
 * @param callback Callback function
 */
void DoIp_RegisterConnectionCallback(DoIp_ConnectionCallback callback);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Validate DoIP header
 * @param header Header to validate
 * @return DOIP_OK if valid
 */
DoIp_ReturnType DoIp_ValidateHeader(const DoIp_HeaderType *header);

/**
 * @brief Check if payload type is valid
 * @param payloadType Payload type
 * @return true if valid
 */
bool DoIp_IsValidPayloadType(uint16_t payloadType);

/**
 * @brief Get payload type name (for debugging)
 * @param payloadType Payload type
 * @return Name string
 */
const char* DoIp_GetPayloadTypeName(uint16_t payloadType);

/**
 * @brief Check if source address is valid
 * @param sourceAddress Source address
 * @return true if valid
 */
bool DoIp_IsValidSourceAddress(uint16_t sourceAddress);

/**
 * @brief Check if target address is valid
 * @param targetAddress Target address
 * @return true if valid
 */
bool DoIp_IsValidTargetAddress(uint16_t targetAddress);

/**
 * @brief Convert bytes to 16-bit value (big-endian)
 * @param bytes Byte array
 * @return 16-bit value
 */
uint16_t DoIp_BytesToUint16(const uint8_t *bytes);

/**
 * @brief Convert bytes to 32-bit value (big-endian)
 * @param bytes Byte array
 * @return 32-bit value
 */
uint32_t DoIp_BytesToUint32(const uint8_t *bytes);

/**
 * @brief Convert 16-bit value to bytes (big-endian)
 * @param value Value
 * @param bytes Output byte array
 */
void DoIp_Uint16ToBytes(uint16_t value, uint8_t *bytes);

/**
 * @brief Convert 32-bit value to bytes (big-endian)
 * @param value Value
 * @param bytes Output byte array
 */
void DoIp_Uint32ToBytes(uint32_t value, uint8_t *bytes);

/******************************************************************************
 * Entity Status Functions
 ******************************************************************************/

/**
 * @brief Handle entity status request
 * @param connectionId Connection ID
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_HandleEntityStatusRequest(uint8_t connectionId);

/**
 * @brief Send entity status response
 * @param connectionId Connection ID
 * @param status Entity status
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendEntityStatusResponse(
    uint8_t connectionId,
    const DoIp_EntityStatusType *status
);

/******************************************************************************
 * Power Mode Functions
 ******************************************************************************/

/**
 * @brief Handle diagnostic power mode request
 * @param connectionId Connection ID
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_HandlePowerModeRequest(uint8_t connectionId);

/**
 * @brief Send diagnostic power mode response
 * @param connectionId Connection ID
 * @param powerMode Power mode
 * @return DOIP_OK on success
 */
DoIp_ReturnType DoIp_SendPowerModeResponse(
    uint8_t connectionId,
    DoIp_PowerModeType powerMode
);

/******************************************************************************
 * Statistics Functions
 ******************************************************************************/

/**
 * @brief Get DoIP statistics
 * @param rxCount Output RX count
 * @param txCount Output TX count
 * @param routingActivations Output routing activation count
 * @param diagnosticMsgs Output diagnostic message count
 */
void DoIp_GetStats(
    uint32_t *rxCount,
    uint32_t *txCount,
    uint32_t *routingActivations,
    uint32_t *diagnosticMsgs
);

/**
 * @brief Reset DoIP statistics
 */
void DoIp_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* DOIP_CORE_H */

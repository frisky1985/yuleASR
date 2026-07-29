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

/******************************************************************************
 * @file    dcm_transport.c
 * @brief   DCM Transport Layer Abstraction Module Implementation
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

#include "dcm_transport.h"
#include "../doip/doip_core.h"
#include "../docan/docan_core.h"
#include "../isotp/isotp_pdur.h"
#include <string.h>

/******************************************************************************
 * Module Global Variables
 ******************************************************************************/

/* Transport layer module state */
typedef struct {
    bool initialized;
    bool running;
    Dcm_TransportConfigType config;
    Dcm_TransportProtocolType defaultProtocol;
} Dcm_TransportModuleStateType;

/* Transport channel context */
typedef struct {
    bool inUse;
    Dcm_TransportChannelConfigType config;
    Dcm_TransportStateType state;
    Dcm_TransportStatisticsType stats;
    Dcm_TransportEventCallbackType eventCallback;
    void *protocolContext;                      /* Protocol-specific context */
    uint32_t connectionStartTime;
    uint32_t lastActivityTime;
    bool txPending;
    bool rxPending;
} Dcm_TransportChannelContextType;

/* Protocol interface registration */
typedef struct {
    const Dcm_TransportProtocolInterfaceType *interfaces[DCM_TRANSPORT_MAX_PROTOCOLS];
    uint8_t numRegistered;
} Dcm_TransportProtocolRegistryType;

/* Module context */
static Dcm_TransportModuleStateType g_transportModuleState;
static Dcm_TransportChannelContextType g_channelContexts[DCM_TRANSPORT_MAX_CHANNELS];
static Dcm_TransportProtocolRegistryType g_protocolRegistry;

/* Global callbacks */
static Dcm_TransportRxCallbackType g_rxCallback = NULL;
static Dcm_TransportTxConfirmationType g_txCallback = NULL;

/* Timing */
static uint32_t g_currentTimeMs = 0U;

/******************************************************************************
 * Protocol Interface Definitions
 ******************************************************************************/

/* DoIP Protocol Interface */
static const Dcm_TransportProtocolInterfaceType g_DoIpInterface = {
    .protocol = DCM_TRANSPORT_PROTOCOL_DOIP,
    .protocolName = "DoIP",
    .init = NULL,
    .deinit = NULL,
    .connect = NULL,
    .disconnect = NULL,
    .send = Dcm_Transport_DoIp_Send,
    .receive = Dcm_Transport_DoIp_Receive,
    .getStatus = Dcm_Transport_DoIp_GetStatus,
    .mainFunction = Dcm_Transport_DoIp_MainFunction,
    .supportsPhysicalAddressing = true,
    .supportsFunctionalAddressing = false,
    .maxMessageSize = 0xFFFFFFFFU  /* No practical limit for DoIP */
};

/* DoCAN Protocol Interface */
static const Dcm_TransportProtocolInterfaceType g_DoCanInterface = {
    .protocol = DCM_TRANSPORT_PROTOCOL_DOCAN,
    .protocolName = "DoCAN",
    .init = NULL,
    .deinit = NULL,
    .connect = NULL,
    .disconnect = NULL,
    .send = Dcm_Transport_DoCan_Send,
    .receive = Dcm_Transport_DoCan_Receive,
    .getStatus = Dcm_Transport_DoCan_GetStatus,
    .mainFunction = Dcm_Transport_DoCan_MainFunction,
    .supportsPhysicalAddressing = true,
    .supportsFunctionalAddressing = true,
    .maxMessageSize = 4095U  /* ISO-TP max message size */
};

/* IsoTp Protocol Interface */
static const Dcm_TransportProtocolInterfaceType g_IsoTpInterface = {
    .protocol = DCM_TRANSPORT_PROTOCOL_ISOTP,
    .protocolName = "IsoTp",
    .init = NULL,
    .deinit = NULL,
    .connect = NULL,
    .disconnect = NULL,
    .send = Dcm_Transport_IsoTp_Send,
    .receive = Dcm_Transport_IsoTp_Receive,
    .getStatus = Dcm_Transport_IsoTp_GetStatus,
    .mainFunction = Dcm_Transport_IsoTp_MainFunction,
    .supportsPhysicalAddressing = true,
    .supportsFunctionalAddressing = true,
    .maxMessageSize = 4095U  /* ISO-TP max message size */
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get current time in milliseconds
 */
static uint32_t Dcm_Transport_GetCurrentTimeMs(void)
{
    return g_currentTimeMs;
}

/**
 * @brief Validate channel ID
 */
static bool Dcm_Transport_IsValidChannel(uint8_t channelId)
{
    return (channelId < DCM_TRANSPORT_MAX_CHANNELS) &&
           (g_channelContexts[channelId].inUse);
}

/**
 * @brief Check if channel is ready for transmission
 */
static bool Dcm_Transport_IsChannelReady(uint8_t channelId)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return false;
    }
    
    Dcm_TransportStateType state = g_channelContexts[channelId].state;
    return (state == DCM_TRANSPORT_STATE_IDLE) ||
           (state == DCM_TRANSPORT_STATE_CONNECTED);
}

/**
 * @brief Notify event to registered callback
 */
static void Dcm_Transport_NotifyEvent(
    uint8_t channelId,
    Dcm_TransportEventType event,
    const void *eventData
)
{
    if (Dcm_Transport_IsValidChannel(channelId)) {
        Dcm_TransportEventCallbackType callback = g_channelContexts[channelId].eventCallback;
        if (callback != NULL) {
            callback(channelId, event, eventData);
        }
    }
}

/**
 * @brief Find protocol interface
 */
static const Dcm_TransportProtocolInterfaceType* Dcm_Transport_FindProtocolInterface(
    Dcm_TransportProtocolType protocol
)
{
    for (uint8_t i = 0U; i < g_protocolRegistry.numRegistered; i++) {
        if (g_protocolRegistry.interfaces[i]->protocol == protocol) {
            return g_protocolRegistry.interfaces[i];
        }
    }
    return NULL;
}

/**
 * @brief Initialize default protocol interfaces
 */
static void Dcm_Transport_InitDefaultProtocols(void)
{
    g_protocolRegistry.numRegistered = 0U;
    
    /* Register built-in protocol interfaces */
    g_protocolRegistry.interfaces[0] = &g_DoIpInterface;
    g_protocolRegistry.interfaces[1] = &g_DoCanInterface;
    g_protocolRegistry.interfaces[2] = &g_IsoTpInterface;
    g_protocolRegistry.numRegistered = 3U;
}

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_Init(const Dcm_TransportConfigType *config)
{
    if (config == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (g_transportModuleState.initialized) {
        return DCM_TRANSPORT_OK;  /* Already initialized */
    }
    
    /* Clear module state */
    memset(&g_transportModuleState, 0U, sizeof(g_transportModuleState));
    memset(g_channelContexts, 0U, sizeof(g_channelContexts));
    memset(&g_protocolRegistry, 0U, sizeof(g_protocolRegistry));
    
    /* Store configuration */
    memcpy(&g_transportModuleState.config, config, sizeof(Dcm_TransportConfigType));
    
    /* Initialize default protocol */
    g_transportModuleState.defaultProtocol = DCM_TRANSPORT_PROTOCOL_ISOTP;
    
    /* Initialize protocol interfaces */
    Dcm_Transport_InitDefaultProtocols();
    
    /* Initialize channels from configuration */
    if (config->channelConfigs != NULL) {
        for (uint8_t i = 0U; i < config->numChannels; i++) {
            if (i < DCM_TRANSPORT_MAX_CHANNELS) {
                g_channelContexts[i].inUse = true;
                memcpy(&g_channelContexts[i].config, 
                       &config->channelConfigs[i], 
                       sizeof(Dcm_TransportChannelConfigType));
                g_channelContexts[i].state = DCM_TRANSPORT_STATE_IDLE;
                
                /* If marked as default, update default protocol */
                if (config->channelConfigs[i].isDefault) {
                    g_transportModuleState.defaultProtocol = config->channelConfigs[i].protocol;
                }
            }
        }
    }
    
    g_transportModuleState.initialized = true;
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_DeInit(void)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    /* Stop if running */
    if (g_transportModuleState.running) {
        (void)Dcm_Transport_Stop();
    }
    
    /* Close all channels */
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse) {
            (void)Dcm_Transport_CloseChannel(i);
        }
    }
    
    /* Clear module state */
    memset(&g_transportModuleState, 0U, sizeof(g_transportModuleState));
    memset(g_channelContexts, 0U, sizeof(g_channelContexts));
    memset(&g_protocolRegistry, 0U, sizeof(g_protocolRegistry));
    
    g_rxCallback = NULL;
    g_txCallback = NULL;
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_Start(void)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (g_transportModuleState.running) {
        return DCM_TRANSPORT_OK;  /* Already running */
    }
    
    /* Connect all configured channels */
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse && 
            g_channelContexts[i].state == DCM_TRANSPORT_STATE_IDLE) {
            (void)Dcm_Transport_Connect(i);
        }
    }
    
    g_transportModuleState.running = true;
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_Stop(void)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!g_transportModuleState.running) {
        return DCM_TRANSPORT_OK;  /* Already stopped */
    }
    
    /* Disconnect all channels */
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse) {
            (void)Dcm_Transport_Disconnect(i);
        }
    }
    
    g_transportModuleState.running = false;
    
    return DCM_TRANSPORT_OK;
}

void Dcm_Transport_MainFunction(uint32_t elapsedTimeMs)
{
    if (!g_transportModuleState.initialized || !g_transportModuleState.running) {
        return;
    }
    
    /* Update timing */
    g_currentTimeMs += elapsedTimeMs;
    
    /* Call protocol-specific main functions */
    Dcm_Transport_DoIp_MainFunction();
    Dcm_Transport_DoCan_MainFunction();
    Dcm_Transport_IsoTp_MainFunction();
    
    /* Process channel timeouts and state transitions */
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (!g_channelContexts[i].inUse) {
            continue;
        }
        
        Dcm_TransportChannelContextType *ctx = &g_channelContexts[i];
        
        /* Check connection timeout */
        if (ctx->state == DCM_TRANSPORT_STATE_CONNECTING) {
            uint32_t timeout = ctx->config.connectionTimeout;
            if ((g_currentTimeMs - ctx->connectionStartTime) > timeout) {
                ctx->state = DCM_TRANSPORT_STATE_ERROR;
                ctx->stats.timeouts++;
                Dcm_Transport_NotifyEvent(i, DCM_TRANSPORT_EVT_TIMEOUT, NULL);
            }
        }
        
        /* Check inactivity timeout for connected channels */
        if (ctx->state == DCM_TRANSPORT_STATE_CONNECTED) {
            uint32_t timeout = ctx->config.transmissionTimeout * 10U;  /* Scale for inactivity */
            if ((g_currentTimeMs - ctx->lastActivityTime) > timeout) {
                /* Optional: disconnect idle connections */
            }
        }
    }
}

/******************************************************************************
 * Channel Management
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_OpenChannel(
    Dcm_TransportProtocolType protocol,
    uint8_t priority,
    uint8_t *channelId
)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (channelId == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (priority >= DCM_TRANSPORT_MAX_PRIORITY_LEVELS) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (!Dcm_Transport_IsProtocolSupported(protocol)) {
        return DCM_TRANSPORT_PROTOCOL_ERROR;
    }
    
    /* Find available channel slot */
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (!g_channelContexts[i].inUse) {
            g_channelContexts[i].inUse = true;
            g_channelContexts[i].config.protocol = protocol;
            g_channelContexts[i].config.priority = priority;
            g_channelContexts[i].state = DCM_TRANSPORT_STATE_IDLE;
            memset(&g_channelContexts[i].stats, 0U, sizeof(Dcm_TransportStatisticsType));
            
            *channelId = i;
            return DCM_TRANSPORT_OK;
        }
    }
    
    return DCM_TRANSPORT_NO_CHANNEL;
}

Dcm_TransportReturnType Dcm_Transport_CloseChannel(uint8_t channelId)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    Dcm_TransportChannelContextType *ctx = &g_channelContexts[channelId];
    
    /* Disconnect if connected */
    if (ctx->state == DCM_TRANSPORT_STATE_CONNECTED) {
        (void)Dcm_Transport_Disconnect(channelId);
    }
    
    /* Clear channel context */
    memset(ctx, 0U, sizeof(Dcm_TransportChannelContextType));
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_ConfigureChannel(
    uint8_t channelId,
    const Dcm_TransportChannelConfigType *config
)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId) || config == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    /* Copy configuration */
    memcpy(&g_channelContexts[channelId].config, 
           config, 
           sizeof(Dcm_TransportChannelConfigType));
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_Connect(uint8_t channelId)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    Dcm_TransportChannelContextType *ctx = &g_channelContexts[channelId];
    
    if (ctx->state != DCM_TRANSPORT_STATE_IDLE) {
        return DCM_TRANSPORT_ERROR;
    }
    
    ctx->state = DCM_TRANSPORT_STATE_CONNECTING;
    ctx->connectionStartTime = Dcm_Transport_GetCurrentTimeMs();
    
    /* Protocol-specific connection will be handled by the adaptor */
    /* For now, mark as connected (actual connection state tracked by protocol) */
    ctx->state = DCM_TRANSPORT_STATE_CONNECTED;
    ctx->lastActivityTime = Dcm_Transport_GetCurrentTimeMs();
    ctx->stats.connectionsEstablished++;
    
    Dcm_Transport_NotifyEvent(channelId, DCM_TRANSPORT_EVT_CONNECT, NULL);
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_Disconnect(uint8_t channelId)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    Dcm_TransportChannelContextType *ctx = &g_channelContexts[channelId];
    
    if (ctx->state == DCM_TRANSPORT_STATE_IDLE) {
        return DCM_TRANSPORT_OK;  /* Already disconnected */
    }
    
    ctx->state = DCM_TRANSPORT_STATE_DISCONNECTING;
    
    /* Protocol-specific disconnection will be handled by the adaptor */
    
    ctx->state = DCM_TRANSPORT_STATE_IDLE;
    ctx->stats.connectionsClosed++;
    
    Dcm_Transport_NotifyEvent(channelId, DCM_TRANSPORT_EVT_DISCONNECT, NULL);
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_SuspendChannel(uint8_t channelId)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (g_channelContexts[channelId].state == DCM_TRANSPORT_STATE_SUSPENDED) {
        return DCM_TRANSPORT_OK;  /* Already suspended */
    }
    
    g_channelContexts[channelId].state = DCM_TRANSPORT_STATE_SUSPENDED;
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_ResumeChannel(uint8_t channelId)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (g_channelContexts[channelId].state != DCM_TRANSPORT_STATE_SUSPENDED) {
        return DCM_TRANSPORT_ERROR;
    }
    
    g_channelContexts[channelId].state = DCM_TRANSPORT_STATE_IDLE;
    
    return DCM_TRANSPORT_OK;
}

/******************************************************************************
 * Unified Send/Receive Interface
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
)
{
    return Dcm_Transport_SendWithPriority(channelId, message, 
                                          DCM_TRANSPORT_DEFAULT_PRIORITY);
}

Dcm_TransportReturnType Dcm_Transport_SendWithPriority(
    uint8_t channelId,
    const Dcm_TransportMessageType *message,
    uint8_t priority
)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId) || message == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (!Dcm_Transport_ValidateMessage(message)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    Dcm_TransportChannelContextType *ctx = &g_channelContexts[channelId];
    
    /* Check channel state */
    if (ctx->state == DCM_TRANSPORT_STATE_SUSPENDED) {
        return DCM_TRANSPORT_CHANNEL_SUSPENDED;
    }
    
    if (!Dcm_Transport_IsChannelReady(channelId)) {
        return DCM_TRANSPORT_NOT_CONNECTED;
    }
    
    /* Get protocol interface */
    const Dcm_TransportProtocolInterfaceType *interface = 
        Dcm_Transport_GetProtocolInterface(ctx->config.protocol);
    
    if (interface == NULL || interface->send == NULL) {
        return DCM_TRANSPORT_PROTOCOL_ERROR;
    }
    
    /* Check message size limit */
    if (message->length > interface->maxMessageSize) {
        return DCM_TRANSPORT_MESSAGE_TOO_LARGE;
    }
    
    /* Handle priority preemption */
    if (priority < ctx->config.priority && ctx->txPending) {
        ctx->stats.priorityPreemptions++;
    }
    
    /* Set TX active state */
    ctx->state = DCM_TRANSPORT_STATE_TX_ACTIVE;
    ctx->txPending = true;
    
    /* Call protocol-specific send function */
    Dcm_TransportReturnType result = interface->send(channelId, message);
    
    if (result == DCM_TRANSPORT_OK) {
        ctx->stats.messagesTransmitted++;
        ctx->stats.bytesTransmitted += message->length;
        ctx->lastActivityTime = Dcm_Transport_GetCurrentTimeMs();
        
        Dcm_Transport_NotifyEvent(channelId, DCM_TRANSPORT_EVT_TX_COMPLETE, NULL);
        
        /* Call TX confirmation callback */
        if (g_txCallback != NULL) {
            g_txCallback(channelId, DCM_TRANSPORT_OK);
        }
    } else {
        ctx->stats.txErrors++;
    }
    
    /* Restore state */
    ctx->state = DCM_TRANSPORT_STATE_CONNECTED;
    ctx->txPending = false;
    
    return result;
}

Dcm_TransportReturnType Dcm_Transport_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId) || message == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    Dcm_TransportChannelContextType *ctx = &g_channelContexts[channelId];
    
    /* Check channel state */
    if (ctx->state == DCM_TRANSPORT_STATE_SUSPENDED) {
        return DCM_TRANSPORT_CHANNEL_SUSPENDED;
    }
    
    if (!Dcm_Transport_IsChannelReady(channelId)) {
        return DCM_TRANSPORT_NOT_CONNECTED;
    }
    
    /* Get protocol interface */
    const Dcm_TransportProtocolInterfaceType *interface = 
        Dcm_Transport_GetProtocolInterface(ctx->config.protocol);
    
    if (interface == NULL || interface->receive == NULL) {
        return DCM_TRANSPORT_PROTOCOL_ERROR;
    }
    
    /* Set RX active state */
    ctx->state = DCM_TRANSPORT_STATE_RX_ACTIVE;
    ctx->rxPending = true;
    
    /* Call protocol-specific receive function */
    Dcm_TransportReturnType result = interface->receive(channelId, message);
    
    if (result == DCM_TRANSPORT_OK) {
        ctx->stats.messagesReceived++;
        ctx->stats.bytesReceived += message->length;
        ctx->lastActivityTime = Dcm_Transport_GetCurrentTimeMs();
        
        Dcm_Transport_NotifyEvent(channelId, DCM_TRANSPORT_EVT_RX_COMPLETE, NULL);
        
        /* Call RX callback */
        if (g_rxCallback != NULL) {
            (void)g_rxCallback(channelId, message);
        }
    }
    
    /* Restore state */
    ctx->state = DCM_TRANSPORT_STATE_CONNECTED;
    ctx->rxPending = false;
    
    return result;
}

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_RegisterRxCallback(
    Dcm_TransportRxCallbackType callback
)
{
    if (callback == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    g_rxCallback = callback;
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_RegisterTxCallback(
    Dcm_TransportTxConfirmationType callback
)
{
    if (callback == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    g_txCallback = callback;
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_RegisterEventCallback(
    uint8_t channelId,
    Dcm_TransportEventCallbackType callback
)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    g_channelContexts[channelId].eventCallback = callback;
    return DCM_TRANSPORT_OK;
}

/******************************************************************************
 * Status Query Functions
 ******************************************************************************/

bool Dcm_Transport_IsInitialized(void)
{
    return g_transportModuleState.initialized;
}

bool Dcm_Transport_IsRunning(void)
{
    return g_transportModuleState.running;
}

Dcm_TransportStateType Dcm_Transport_GetChannelState(uint8_t channelId)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_STATE_UNINIT;
    }
    
    return g_channelContexts[channelId].state;
}

Dcm_TransportReturnType Dcm_Transport_GetChannelInfo(
    uint8_t channelId,
    Dcm_TransportChannelInfoType *info
)
{
    if (info == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    Dcm_TransportChannelContextType *ctx = &g_channelContexts[channelId];
    
    info->channelId = channelId;
    info->protocol = ctx->config.protocol;
    info->state = ctx->state;
    info->priority = ctx->config.priority;
    info->sourceAddress = ctx->config.sourceAddress;
    info->targetAddress = ctx->config.targetAddress;
    info->rxBufferSize = ctx->config.rxBufferSize;
    info->txBufferSize = ctx->config.txBufferSize;
    info->isActive = (ctx->state == DCM_TRANSPORT_STATE_CONNECTED);
    info->isDefault = ctx->config.isDefault;
    info->connectionTime = ctx->connectionStartTime;
    info->lastActivityTime = ctx->lastActivityTime;
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_GetStatistics(
    uint8_t channelId,
    Dcm_TransportStatisticsType *stats
)
{
    if (stats == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    memcpy(stats, &g_channelContexts[channelId].stats, sizeof(Dcm_TransportStatisticsType));
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportReturnType Dcm_Transport_ResetStatistics(uint8_t channelId)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    memset(&g_channelContexts[channelId].stats, 0U, sizeof(Dcm_TransportStatisticsType));
    
    return DCM_TRANSPORT_OK;
}

/******************************************************************************
 * Protocol Selection and Priority
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_SetPriority(
    uint8_t channelId,
    uint8_t priority
)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (priority >= DCM_TRANSPORT_MAX_PRIORITY_LEVELS) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    g_channelContexts[channelId].config.priority = priority;
    
    Dcm_Transport_NotifyEvent(channelId, DCM_TRANSPORT_EVT_PRIORITY_CHANGED, NULL);
    
    return DCM_TRANSPORT_OK;
}

uint8_t Dcm_Transport_GetPriority(uint8_t channelId)
{
    if (!Dcm_Transport_IsValidChannel(channelId)) {
        return 0xFFU;
    }
    
    return g_channelContexts[channelId].config.priority;
}

Dcm_TransportReturnType Dcm_Transport_SelectDefaultProtocol(
    Dcm_TransportProtocolType protocol
)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_NOT_INITIALIZED;
    }
    
    if (!Dcm_Transport_IsProtocolSupported(protocol)) {
        return DCM_TRANSPORT_PROTOCOL_ERROR;
    }
    
    g_transportModuleState.defaultProtocol = protocol;
    
    return DCM_TRANSPORT_OK;
}

Dcm_TransportProtocolType Dcm_Transport_GetDefaultProtocol(void)
{
    if (!g_transportModuleState.initialized) {
        return DCM_TRANSPORT_PROTOCOL_NONE;
    }
    
    return g_transportModuleState.defaultProtocol;
}

uint8_t Dcm_Transport_FindChannelByProtocol(Dcm_TransportProtocolType protocol)
{
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse && 
            g_channelContexts[i].config.protocol == protocol) {
            return i;
        }
    }
    
    return DCM_TRANSPORT_INVALID_CHANNEL_ID;
}

uint8_t Dcm_Transport_FindBestAvailableChannel(void)
{
    uint8_t bestChannel = DCM_TRANSPORT_INVALID_CHANNEL_ID;
    uint8_t bestPriority = DCM_TRANSPORT_MAX_PRIORITY_LEVELS;
    
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse && 
            Dcm_Transport_IsChannelReady(i)) {
            if (g_channelContexts[i].config.priority < bestPriority) {
                bestPriority = g_channelContexts[i].config.priority;
                bestChannel = i;
            }
        }
    }
    
    return bestChannel;
}

/******************************************************************************
 * Protocol-Specific Functions
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_RegisterProtocolInterface(
    const Dcm_TransportProtocolInterfaceType *protocolInterface
)
{
    if (protocolInterface == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    if (g_protocolRegistry.numRegistered >= DCM_TRANSPORT_MAX_PROTOCOLS) {
        return DCM_TRANSPORT_ERROR;
    }
    
    g_protocolRegistry.interfaces[g_protocolRegistry.numRegistered] = protocolInterface;
    g_protocolRegistry.numRegistered++;
    
    return DCM_TRANSPORT_OK;
}

const Dcm_TransportProtocolInterfaceType* Dcm_Transport_GetProtocolInterface(
    Dcm_TransportProtocolType protocol
)
{
    return Dcm_Transport_FindProtocolInterface(protocol);
}

const char* Dcm_Transport_GetProtocolName(Dcm_TransportProtocolType protocol)
{
    const Dcm_TransportProtocolInterfaceType *interface = 
        Dcm_Transport_FindProtocolInterface(protocol);
    
    if (interface != NULL) {
        return interface->protocolName;
    }
    
    return "Unknown";
}

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

bool Dcm_Transport_IsProtocolSupported(Dcm_TransportProtocolType protocol)
{
    return (Dcm_Transport_FindProtocolInterface(protocol) != NULL);
}

uint8_t Dcm_Transport_GetActiveChannelCount(void)
{
    uint8_t count = 0U;
    
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse) {
            count++;
        }
    }
    
    return count;
}

uint8_t Dcm_Transport_GetConnectedChannelCount(void)
{
    uint8_t count = 0U;
    
    for (uint8_t i = 0U; i < DCM_TRANSPORT_MAX_CHANNELS; i++) {
        if (g_channelContexts[i].inUse && 
            g_channelContexts[i].state == DCM_TRANSPORT_STATE_CONNECTED) {
            count++;
        }
    }
    
    return count;
}

Dcm_ReturnType Dcm_Transport_ConvertReturnType(Dcm_TransportReturnType transportRet)
{
    switch (transportRet) {
        case DCM_TRANSPORT_OK:
            return DCM_E_OK;
        case DCM_TRANSPORT_BUSY:
            return DCM_E_BUSY_REPEAT_REQUEST;
        case DCM_TRANSPORT_TIMEOUT:
            return DCM_E_BUSY_REPEAT_REQUEST;
        case DCM_TRANSPORT_INVALID_PARAMETER:
            return DCM_E_REQUEST_OUT_OF_RANGE;
        case DCM_TRANSPORT_NOT_INITIALIZED:
            return DCM_E_NOT_OK;
        case DCM_TRANSPORT_NO_CHANNEL:
            return DCM_E_NOT_OK;
        case DCM_TRANSPORT_NOT_CONNECTED:
            return DCM_E_NOT_OK;
        case DCM_TRANSPORT_BUFFER_OVERFLOW:
            return DCM_E_RESPONSE_BUFFER_TOO_SMALL;
        case DCM_TRANSPORT_MESSAGE_TOO_LARGE:
            return DCM_E_REQUEST_OUT_OF_RANGE;
        default:
            return DCM_E_NOT_OK;
    }
}

bool Dcm_Transport_ValidateMessage(const Dcm_TransportMessageType *message)
{
    if (message == NULL) {
        return false;
    }
    
    if (message->data == NULL && message->length > 0U) {
        return false;
    }
    
    if (message->length == 0U) {
        return false;
    }
    
    /* Max UDS message size check (typically 4095 bytes for ISO-TP) */
    if (message->length > 4095U) {
        return false;
    }
    
    return true;
}

/******************************************************************************
 * DoIP Protocol Adaptor Implementation
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_DoIp_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
)
{
    (void)channelId;
    
    if (message == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    /* Convert to DoIP diagnostic message format */
    DoIp_DiagnosticMessageType doipMsg;
    doipMsg.sourceAddress = message->sourceAddress;
    doipMsg.targetAddress = message->targetAddress;
    doipMsg.userData = message->data;
    doipMsg.userDataLength = (uint16_t)message->length;
    
    /* Send via DoIP layer */
    /* Use connection ID 0 as default for now - could be mapped from channelId */
    DoIp_ReturnType result = DoIp_SendDiagnosticMessage(0U, &doipMsg);
    
    if (result == DOIP_OK) {
        return DCM_TRANSPORT_OK;
    } else if (result == DOIP_BUSY) {
        return DCM_TRANSPORT_BUSY;
    } else {
        return DCM_TRANSPORT_ERROR;
    }
}

Dcm_TransportReturnType Dcm_Transport_DoIp_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
)
{
    (void)channelId;
    (void)message;
    
    /* DoIP reception is typically asynchronous via callback */
    /* This function would check for buffered messages */
    return DCM_TRANSPORT_OK;
}

Dcm_TransportStateType Dcm_Transport_DoIp_GetStatus(uint8_t channelId)
{
    (void)channelId;
    
    DoIp_StateType doipState = DoIp_GetConnectionState(0U);
    
    switch (doipState) {
        case DOIP_STATE_INIT:
            return DCM_TRANSPORT_STATE_IDLE;
        case DOIP_STATE_REGISTERED:
        case DOIP_STATE_DIAGNOSTIC_SESSION:
            return DCM_TRANSPORT_STATE_CONNECTED;
        case DOIP_STATE_ERROR:
            return DCM_TRANSPORT_STATE_ERROR;
        default:
            return DCM_TRANSPORT_STATE_CONNECTING;
    }
}

void Dcm_Transport_DoIp_MainFunction(void)
{
    /* Call DoIP main function if available */
    DoIp_MainFunction();
}

/******************************************************************************
 * DoCAN Protocol Adaptor Implementation
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_DoCan_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
)
{
    (void)channelId;
    
    if (message == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    /* Send via DoCAN layer */
    /* Map channelId to DoCAN connection ID */
    DoCan_ReturnType result = DoCan_Transmit(
        channelId,  /* Connection ID */
        message->data,
        message->length
    );
    
    if (result == DOCAN_OK) {
        return DCM_TRANSPORT_OK;
    } else if (result == DOCAN_E_CONN_BUSY) {
        return DCM_TRANSPORT_BUSY;
    } else {
        return DCM_TRANSPORT_ERROR;
    }
}

Dcm_TransportReturnType Dcm_Transport_DoCan_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
)
{
    (void)channelId;
    (void)message;
    
    /* DoCAN reception is typically asynchronous via RxIndication */
    return DCM_TRANSPORT_OK;
}

Dcm_TransportStateType Dcm_Transport_DoCan_GetStatus(uint8_t channelId)
{
    DoCan_ConnectionStateType state;
    DoCan_ReturnType result = DoCan_GetConnectionState(channelId, &state);
    
    if (result != DOCAN_OK) {
        return DCM_TRANSPORT_STATE_UNINIT;
    }
    
    switch (state) {
        case DOCAN_CONN_STATE_IDLE:
            return DCM_TRANSPORT_STATE_IDLE;
        case DOCAN_CONN_STATE_TX_CF:
            return DCM_TRANSPORT_STATE_TX_ACTIVE;
        case DOCAN_CONN_STATE_RX_WAIT_CF:
        case DOCAN_CONN_STATE_RX_SEND_FC:
            return DCM_TRANSPORT_STATE_RX_ACTIVE;
        case DOCAN_CONN_STATE_ERROR:
            return DCM_TRANSPORT_STATE_ERROR;
        default:
            return DCM_TRANSPORT_STATE_CONNECTED;
    }
}

void Dcm_Transport_DoCan_MainFunction(void)
{
    /* Call DoCAN main function */
    DoCan_MainFunction();
}

/******************************************************************************
 * IsoTp Protocol Adaptor Implementation
 ******************************************************************************/

Dcm_TransportReturnType Dcm_Transport_IsoTp_Send(
    uint8_t channelId,
    const Dcm_TransportMessageType *message
)
{
    if (message == NULL) {
        return DCM_TRANSPORT_INVALID_PARAMETER;
    }
    
    /* Send via IsoTp-PduR layer */
    Isotp_ReturnType result = Isotp_PduR_Transmit(
        channelId,
        message->data,
        (uint16_t)message->length
    );
    
    if (result == ISOTP_E_OK) {
        return DCM_TRANSPORT_OK;
    } else if (result == ISOTP_E_BUSY) {
        return DCM_TRANSPORT_BUSY;
    } else {
        return DCM_TRANSPORT_ERROR;
    }
}

Dcm_TransportReturnType Dcm_Transport_IsoTp_Receive(
    uint8_t channelId,
    Dcm_TransportMessageType *message
)
{
    (void)channelId;
    (void)message;
    
    /* IsoTp reception is asynchronous via callback */
    return DCM_TRANSPORT_OK;
}

Dcm_TransportStateType Dcm_Transport_IsoTp_GetStatus(uint8_t channelId)
{
    /* Get IsoTp channel status */
    /* This would typically call Isotp_GetChannelState if available */
    (void)channelId;
    
    /* For now, assume connected if initialized */
    if (g_transportModuleState.initialized) {
        return DCM_TRANSPORT_STATE_CONNECTED;
    }
    return DCM_TRANSPORT_STATE_UNINIT;
}

void Dcm_Transport_IsoTp_MainFunction(void)
{
    /* Call IsoTp-PduR main function */
    Isotp_PduR_MainFunction();
}

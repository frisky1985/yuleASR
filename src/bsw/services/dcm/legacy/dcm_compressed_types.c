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
 * @file    dcm_compressed_types.c
 * @brief   DCM Compressed Types Conversion Implementation
 *
 * Conversion functions between original and compressed (bit-field) types.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_compressed_types.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Protocol Config Conversions
 ******************************************************************************/

void Dcm_CompPrintSavingsReport(void);
Dcm_ReturnType Dcm_CompExpandContextArray(const Dcm_CompContextType *srcArray,                                           Dcm_ContextType *dstArray,                                           uint8_t count);
Dcm_ReturnType Dcm_CompConvertContextArray(const Dcm_ContextType *srcArray,                                            Dcm_CompContextType *dstArray,                                            uint8_t count);
void Dcm_CompConvertProtocolConfig(const Dcm_ProtocolConfigType *src,
                                    Dcm_CompProtocolConfigType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_CompProtocolConfigType));
    
    dst->rxBufferSize = (src->rxBufferSize > 4095U) ? 4095U : src->rxBufferSize;
    dst->txBufferSize = (src->txBufferSize > 4095U) ? 4095U : src->txBufferSize;
    dst->protocolType = (uint32_t)src->protocolType & 0x0FU;
    dst->responsePendingTime = src->responsePendingTime;
    dst->responseMaxTime = src->responseMaxTime;
    dst->sourceAddress = src->sourceAddress & 0x07FFU;
    dst->functionalAddr = src->functionalAddressingEnabled ? 1U : 0U;
    dst->suppressPosResp = src->suppressPosResponseAllowed ? 1U : 0U;
}

void Dcm_CompExpandProtocolConfig(const Dcm_CompProtocolConfigType *src,
                                   Dcm_ProtocolConfigType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_ProtocolConfigType));
    
    dst->rxBufferSize = (uint16_t)src->rxBufferSize;
    dst->txBufferSize = (uint16_t)src->txBufferSize;
    dst->protocolType = (Dcm_ProtocolType)src->protocolType;
    dst->responsePendingTime = src->responsePendingTime;
    dst->responseMaxTime = src->responseMaxTime;
    dst->sourceAddress = src->sourceAddress;
    dst->functionalAddressingEnabled = (src->functionalAddr != 0U);
    dst->suppressPosResponseAllowed = (src->suppressPosResp != 0U);
}

/******************************************************************************
 * Session Config Conversions
 ******************************************************************************/

void Dcm_CompConvertSessionConfig(const Dcm_SessionConfigType *src,
                                   Dcm_CompSessionConfigType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_CompSessionConfigType));
    
    dst->sessionType = (uint32_t)src->sessionType & 0x0FU;
    dst->isDefaultSession = src->isDefaultSession ? 1U : 0U;
    dst->suppressPosResp = src->suppressPosResponseAllowed ? 1U : 0U;
    dst->p2ServerMax = src->timing.p2ServerMax;
    dst->p2StarServerMax = src->timing.p2StarServerMax;
    dst->s3Server = src->timing.s3Server;
    dst->sessionTimeoutMs = src->sessionTimeoutMs;
    dst->supportedSecurityLevels = src->supportedSecurityLevels;
}

void Dcm_CompExpandSessionConfig(const Dcm_CompSessionConfigType *src,
                                  Dcm_SessionConfigType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_SessionConfigType));
    
    dst->sessionType = (Dcm_SessionType)src->sessionType;
    dst->isDefaultSession = (src->isDefaultSession != 0U);
    dst->suppressPosResponseAllowed = (src->suppressPosResp != 0U);
    dst->timing.p2ServerMax = src->p2ServerMax;
    dst->timing.p2StarServerMax = src->p2StarServerMax;
    dst->timing.s3Server = src->s3Server;
    dst->sessionTimeoutMs = src->sessionTimeoutMs;
    dst->supportedSecurityLevels = src->supportedSecurityLevels;
}

/******************************************************************************
 * Channel Conversions
 ******************************************************************************/

void Dcm_CompConvertChannel(const Dcm_ChannelType *src,
                             Dcm_CompChannelType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_CompChannelType));
    
    dst->rxBuffer = src->rxBuffer;
    dst->txBuffer = src->txBuffer;
    dst->rxBufferSize = (src->rxBufferSize > 4095U) ? 4095U : (uint32_t)src->rxBufferSize;
    dst->txBufferSize = (src->txBufferSize > 4095U) ? 4095U : (uint32_t)src->txBufferSize;
    dst->rxDataLength = (src->rxDataLength > 4095U) ? 4095U : (uint32_t)src->rxDataLength;
    dst->txDataLength = (src->txDataLength > 4095U) ? 4095U : (uint32_t)src->txDataLength;
    dst->channelId = src->channelId;
    dst->protocol = (uint32_t)src->protocol & 0x0FU;
    dst->state = (uint32_t)src->state & 0x0FU;
    dst->txPending = src->txPending ? 1U : 0U;
}

void Dcm_CompExpandChannel(const Dcm_CompChannelType *src,
                            Dcm_ChannelType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_ChannelType));
    
    dst->rxBuffer = src->rxBuffer;
    dst->txBuffer = src->txBuffer;
    dst->rxBufferSize = (uint32_t)src->rxBufferSize;
    dst->txBufferSize = (uint32_t)src->txBufferSize;
    dst->rxDataLength = (uint32_t)src->rxDataLength;
    dst->txDataLength = (uint32_t)src->txDataLength;
    dst->channelId = src->channelId;
    dst->protocol = (Dcm_ProtocolType)src->protocol;
    dst->state = (Dcm_StateType)src->state;
    dst->txPending = (src->txPending != 0U);
}

/******************************************************************************
 * Context Conversions
 ******************************************************************************/

void Dcm_CompConvertContext(const Dcm_ContextType *src,
                             Dcm_CompContextType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_CompContextType));
    
    dst->initialized = src->initialized ? 1U : 0U;
    dst->state = (uint32_t)src->state & 0x0FU;
    dst->currentSession = (uint32_t)src->currentSession & 0x0FU;
    dst->previousSession = (uint32_t)src->previousSession & 0x0FU;
    dst->currentSecLevel = src->currentSecurityLevel & 0x0FU;
    dst->responsePending = src->responsePending ? 1U : 0U;
    dst->suppressPosResp = src->suppressPositiveResponse ? 1U : 0U;
    dst->testerSourceAddr = src->testerSourceAddress;
    dst->sessionTimer = src->sessionTimer;
    dst->s3Timer = src->s3Timer;
    /* Note: pointers are copied as-is, caller must manage memory */
    dst->channels = (Dcm_CompChannelType *)src->channels;
    dst->numChannels = src->numChannels;
    dst->sessionConfigs = (const Dcm_CompSessionConfigType *)src->sessionConfigs;
    dst->numSessions = src->numSessions;
    dst->serviceTable = src->serviceTable;
    dst->numServices = src->numServices;
}

void Dcm_CompExpandContext(const Dcm_CompContextType *src,
                            Dcm_ContextType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_ContextType));
    
    dst->initialized = (src->initialized != 0U);
    dst->state = (Dcm_StateType)src->state;
    dst->currentSession = (Dcm_SessionType)src->currentSession;
    dst->previousSession = (Dcm_SessionType)src->previousSession;
    dst->currentSecurityLevel = (uint8_t)src->currentSecLevel;
    dst->responsePending = (src->responsePending != 0U);
    dst->suppressPositiveResponse = (src->suppressPosResp != 0U);
    dst->testerSourceAddress = src->testerSourceAddr;
    dst->sessionTimer = src->sessionTimer;
    dst->s3Timer = src->s3Timer;
    /* Note: pointers are copied as-is */
    dst->channels = (Dcm_ChannelType *)src->channels;
    dst->numChannels = src->numChannels;
    dst->sessionConfigs = (const Dcm_SessionConfigType *)src->sessionConfigs;
    dst->numSessions = src->numSessions;
    dst->serviceTable = src->serviceTable;
    dst->numServices = src->numServices;
}

/******************************************************************************
 * Priority Queue Entry Conversions
 ******************************************************************************/

void Dcm_CompConvertPqEntry(const Dcm_PqEntry *src,
                             Dcm_CompPqEntryType *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_CompPqEntryType));
    
    dst->entryType = (uint32_t)src->entryType & 0x0FU;
    dst->priority = (uint32_t)src->priority & 0x0FU;
    dst->sequence = src->sequence & 0x0FFFU;
    dst->timestamp = src->timestamp;
    
    switch (src->entryType) {
        case DCM_PQ_ENTRY_SERVICE_REQUEST:
            dst->data.request.serviceId = src->data.request.serviceId;
            dst->data.request.subfunction = src->data.request.subfunction;
            dst->data.request.sourceAddr = src->data.request.sourceAddr & 0x07FFU;
            dst->data.request.protocol = (uint32_t)src->data.request.protocol & 0x0FU;
            dst->data.request.data = src->data.request.data;
            dst->data.request.length = src->data.request.length;
            break;
            
        case DCM_PQ_ENTRY_ASYNC_RESPONSE:
            dst->data.async.handle = src->data.async.handle;
            dst->data.async.result = (uint16_t)src->data.async.result & 0x00FFU;
            dst->data.async.responseData = src->data.async.responseData;
            dst->data.async.responseLength = (src->data.async.responseLength > 65535U) 
                                              ? 65535U 
                                              : (uint32_t)src->data.async.responseLength;
            break;
            
        case DCM_PQ_ENTRY_TIMER_CALLBACK:
            dst->data.timer.timerId = src->data.timer.timerId;
            dst->data.timer.callback = src->data.timer.callback;
            dst->data.timer.userData = src->data.timer.userData;
            break;
            
        case DCM_PQ_ENTRY_PROTOCOL_EVENT:
            dst->data.event.eventType = (uint32_t)src->data.event.eventType & 0x00FFU;
            dst->data.event.channelId = src->data.event.channelId & 0x00FFU;
            dst->data.event.eventData = src->data.event.eventData;
            break;
            
        default:
            break;
    }
}

void Dcm_CompExpandPqEntry(const Dcm_CompPqEntryType *src,
                            Dcm_PqEntry *dst)
{
    if ((src == NULL) || (dst == NULL)) {
        return;
    }
    
    (void)memset(dst, 0, sizeof(Dcm_PqEntry));
    
    dst->entryType = (Dcm_PqEntryType)src->entryType;
    dst->priority = (Dcm_PriorityLevel)src->priority;
    dst->sequence = src->sequence;
    dst->timestamp = src->timestamp;
    
    switch (src->entryType) {
        case DCM_PQ_ENTRY_SERVICE_REQUEST:
            dst->data.request.serviceId = src->data.request.serviceId;
            dst->data.request.subfunction = src->data.request.subfunction;
            dst->data.request.sourceAddr = src->data.request.sourceAddr;
            dst->data.request.protocol = (Dcm_ProtocolType)src->data.request.protocol;
            dst->data.request.data = src->data.request.data;
            dst->data.request.length = src->data.request.length;
            break;
            
        case DCM_PQ_ENTRY_ASYNC_RESPONSE:
            dst->data.async.handle = src->data.async.handle;
            dst->data.async.result = (Dcm_ReturnType)src->data.async.result;
            dst->data.async.responseData = src->data.async.responseData;
            dst->data.async.responseLength = (uint32_t)src->data.async.responseLength;
            break;
            
        case DCM_PQ_ENTRY_TIMER_CALLBACK:
            dst->data.timer.timerId = src->data.timer.timerId;
            dst->data.timer.callback = src->data.timer.callback;
            dst->data.timer.userData = src->data.timer.userData;
            break;
            
        case DCM_PQ_ENTRY_PROTOCOL_EVENT:
            dst->data.event.eventType = (uint8_t)src->data.event.eventType;
            dst->data.event.channelId = src->data.event.channelId;
            dst->data.event.eventData = src->data.event.eventData;
            break;
            
        default:
            break;
    }
}

/******************************************************************************
 * Batch Conversion Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_CompConvertContextArray(const Dcm_ContextType *srcArray,
                                            Dcm_CompContextType *dstArray,
                                            uint8_t count)
{
    if ((srcArray == NULL) || (dstArray == NULL) || (count == 0U)) {
        return DCM_E_NOT_OK;
    }
    
    for (uint8_t i = 0U; i < count; i++) {
        Dcm_CompConvertContext(&srcArray[i], &dstArray[i]);
    }
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_CompExpandContextArray(const Dcm_CompContextType *srcArray,
                                           Dcm_ContextType *dstArray,
                                           uint8_t count)
{
    if ((srcArray == NULL) || (dstArray == NULL) || (count == 0U)) {
        return DCM_E_NOT_OK;
    }
    
    for (uint8_t i = 0U; i < count; i++) {
        Dcm_CompExpandContext(&srcArray[i], &dstArray[i]);
    }
    
    return DCM_E_OK;
}

/******************************************************************************
 * Memory Savings Report
 ******************************************************************************/

void Dcm_CompPrintSavingsReport(void)
{
    (void)printf("\n========== DCM Compressed Types Memory Savings ==========\n");
    (void)printf("Dcm_ContextType:      %zu -> %zu bytes (saved %zu bytes, %lu%%)\n",
                 sizeof(Dcm_ContextType), sizeof(Dcm_CompContextType),
                 DCM_COMP_SAVINGS_CONTEXT,
                 (unsigned long)(DCM_COMP_SAVINGS_CONTEXT * 100 / sizeof(Dcm_ContextType)));
    (void)printf("Dcm_ChannelType:      %zu -> %zu bytes (saved %zu bytes, %lu%%)\n",
                 sizeof(Dcm_ChannelType), sizeof(Dcm_CompChannelType),
                 DCM_COMP_SAVINGS_CHANNEL,
                 (unsigned long)(DCM_COMP_SAVINGS_CHANNEL * 100 / sizeof(Dcm_ChannelType)));
    (void)printf("Dcm_PqEntry:          %zu -> %zu bytes (saved %zu bytes, %lu%%)\n",
                 sizeof(Dcm_PqEntry), sizeof(Dcm_CompPqEntryType),
                 DCM_COMP_SAVINGS_PQENTRY,
                 (unsigned long)(DCM_COMP_SAVINGS_PQENTRY * 100 / sizeof(Dcm_PqEntry)));
    (void)printf("Dcm_SessionConfigType:%zu -> %zu bytes (saved %zu bytes, %lu%%)\n",
                 sizeof(Dcm_SessionConfigType), sizeof(Dcm_CompSessionConfigType),
                 DCM_COMP_SAVINGS_SESSION,
                 (unsigned long)(DCM_COMP_SAVINGS_SESSION * 100 / sizeof(Dcm_SessionConfigType)));
    (void)printf("=========================================================\n\n");
}

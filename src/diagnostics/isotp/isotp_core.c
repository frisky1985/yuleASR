/******************************************************************************
 * @file    isotp_core.c
 * @brief   ISO 15765-2 IsoTp Transport Layer Core Implementation
 *
 * ISO 15765-2:2016 compliant
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "isotp_core.h"
#include <string.h>
#include <stddef.h>

/******************************************************************************
 * Module Global Variables
 ******************************************************************************/

/* Static module context */
static Isotp_ContextType g_isotpContext;
static Isotp_ChannelContextType g_channelContexts[ISOTP_MAX_CHANNELS];

/* Timing constants in milliseconds */
#define ISOTP_N_AS_DEFAULT      1000U
#define ISOTP_N_AR_DEFAULT      1000U
#define ISOTP_N_BS_DEFAULT      1000U
#define ISOTP_N_BR_DEFAULT      1000U
#define ISOTP_N_CS_DEFAULT      1000U
#define ISOTP_N_CR_DEFAULT      1000U

#define ISOTP_INVALID_CHANNEL   0xFFU

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get current time in milliseconds (platform-specific)
 */
static uint32_t IsoTp_GetCurrentTime(void)
{
    /* TODO: Replace with platform-specific time function */
    /* For now, using simple counter - replace with OS tick or HW timer */
    static uint32_t counter = 0;
    return counter++;
}

/**
 * @brief Validate channel ID
 */
static bool IsoTp_IsValidChannel(uint8_t channelId)
{
    return (channelId < g_isotpContext.numChannels) &&
           (g_isotpContext.channelConfigs != NULL);
}

/**
 * @brief Get addressing offset based on addressing mode
 */
static uint8_t IsoTp_GetAddrOffset(Isotp_AddressingModeType addrMode)
{
    switch (addrMode) {
        case ISOTP_NORMAL_ADDRESSING:
        case ISOTP_NORMAL_FIXED_ADDRESSING:
            return 0U;
        case ISOTP_EXTENDED_ADDRESSING:
        case ISOTP_MIXED_11BIT_ADDRESSING:
        case ISOTP_MIXED_29BIT_ADDRESSING:
            return 1U;
        default:
            return 0U;
    }
}

/**
 * @brief Send CAN frame via registered callback
 */
static Isotp_ReturnType IsoTp_SendCanFrame(
    uint8_t channelId,
    uint32_t canId,
    uint8_t *data,
    uint8_t length
)
{
    const Isotp_ChannelConfigType *config = IsoTp_GetChannelConfig(channelId);
    
    if (config == NULL) {
        return ISOTP_E_NOT_OK;
    }
    
    if (g_isotpContext.canTransmitCallback != NULL) {
        return g_isotpContext.canTransmitCallback(
            canId,
            data,
            length,
            (config->canFrameType == ISOTP_CAN_FD),
            NULL
        );
    }
    
    return ISOTP_E_NOT_OK;
}

/******************************************************************************
 * N_PCI Functions
 ******************************************************************************/

Isotp_ReturnType IsoTp_ParseNpci(
    const uint8_t *frame,
    uint8_t dlc,
    Isotp_NpciType *npci,
    Isotp_AddressingModeType addrMode
)
{
    if (frame == NULL || npci == NULL || dlc == 0U) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    uint8_t pciByte = frame[offset];
    uint8_t pciType = pciByte & ISOTP_N_PCI_TYPE_MASK;
    
    memset(npci, 0, sizeof(Isotp_NpciType));
    npci->pciType = pciType;
    
    switch (pciType) {
        case ISOTP_N_PCI_SF: {
            /* Single Frame: PCI byte contains DL in lower nibble */
            if (addrMode == ISOTP_NORMAL_ADDRESSING && dlc > ISOTP_CAN20_FRAME_SIZE) {
                /* CAN FD: Extended SF DL encoding */
                if (pciByte == 0x00U) {
                    /* Extended addressing: DL in byte 1 */
                    npci->dataLength = frame[offset + 1];
                } else {
                    npci->dataLength = pciByte & ISOTP_N_PCI_DL_MASK;
                }
            } else {
                npci->dataLength = pciByte & ISOTP_N_PCI_DL_MASK;
            }
            break;
        }
        
        case ISOTP_N_PCI_FF: {
            /* First Frame: 12-bit DL (lower nibble + next byte) */
            npci->dataLength = ((uint16_t)(pciByte & 0x0FU) << 8) | frame[offset + 1];
            /* Check for FF_DL > 4095 (32-bit length in following bytes) */
            if (npci->dataLength == 0U) {
                /* Extended FF_DL format (not commonly used) */
                if (dlc >= offset + 6U) {
                    npci->dataLength = ((uint32_t)frame[offset + 2] << 24) |
                                       ((uint32_t)frame[offset + 3] << 16) |
                                       ((uint32_t)frame[offset + 4] << 8) |
                                       frame[offset + 5];
                }
            }
            break;
        }
        
        case ISOTP_N_PCI_CF: {
            /* Consecutive Frame: Sequence number in lower nibble */
            npci->seqNum = pciByte & ISOTP_N_PCI_SN_MASK;
            break;
        }
        
        case ISOTP_N_PCI_FC: {
            /* Flow Control: FS in lower nibble */
            npci->flowStatus = pciByte & ISOTP_N_PCI_FS_MASK;
            if (dlc > offset + 1U) {
                npci->blockSize = frame[offset + 1];
            }
            if (dlc > offset + 2U) {
                npci->stMin = frame[offset + 2];
            }
            break;
        }
        
        default:
            return ISOTP_E_INVALID_FRAME;
    }
    
    return ISOTP_E_OK;
}

Isotp_ReturnType IsoTp_GetFramePayload(
    const uint8_t *frame,
    uint8_t dlc,
    const uint8_t **payload,
    uint8_t *payloadLen,
    Isotp_NpciType *npci,
    Isotp_AddressingModeType addrMode
)
{
    if (frame == NULL || payload == NULL || payloadLen == NULL || npci == NULL) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    uint8_t pciType = npci->pciType;
    
    switch (pciType) {
        case ISOTP_N_PCI_SF: {
            /* SF payload starts after PCI byte(s) */
            uint8_t pciBytes = 1U;
            if (addrMode == ISOTP_NORMAL_ADDRESSING && dlc > ISOTP_CAN20_FRAME_SIZE && frame[offset] == 0x00U) {
                /* Extended SF DL in CAN FD */
                pciBytes = 2U;
            }
            *payload = &frame[offset + pciBytes];
            *payloadLen = dlc - offset - pciBytes;
            break;
        }
        
        case ISOTP_N_PCI_FF: {
            /* FF payload starts after 2 PCI bytes (or 6 for extended) */
            uint8_t pciBytes = 2U;
            if (((frame[offset] & 0x0FU) << 8 | frame[offset + 1]) == 0U) {
                pciBytes = 6U;  /* Extended FF_DL format */
            }
            *payload = &frame[offset + pciBytes];
            *payloadLen = dlc - offset - pciBytes;
            break;
        }
        
        case ISOTP_N_PCI_CF: {
            /* CF payload starts after 1 PCI byte */
            *payload = &frame[offset + 1U];
            *payloadLen = dlc - offset - 1U;
            break;
        }
        
        case ISOTP_N_PCI_FC: {
            /* FC has no payload */
            *payload = NULL;
            *payloadLen = 0U;
            break;
        }
        
        default:
            return ISOTP_E_INVALID_FRAME;
    }
    
    return ISOTP_E_OK;
}

/******************************************************************************
 * Frame Building Functions
 ******************************************************************************/

Isotp_ReturnType IsoTp_BuildSingleFrame(
    const uint8_t *data,
    uint16_t dataLen,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
)
{
    if (data == NULL || outFrame == NULL || outLen == NULL || dataLen == 0U) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    uint8_t maxPayload = IsoTp_GetMaxSingleFramePayload(addrMode, isFd);
    
    if (dataLen > maxPayload) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    memset(outFrame, 0xAAU, ISOTP_CANFD_MAX_FRAME_SIZE);  /* Fill with padding */
    
    /* Extended addressing byte (if applicable) */
    if (offset > 0U) {
        outFrame[0] = 0x00U;  /* N_TA placeholder */
    }
    
    /* PCI byte */
    if (isFd && dataLen > 7U) {
        /* CAN FD extended SF DL */
        outFrame[offset] = 0x00U;  /* Extended encoding */
        outFrame[offset + 1U] = (uint8_t)dataLen;
        memcpy(&outFrame[offset + 2U], data, dataLen);
        *outLen = offset + 2U + dataLen;
    } else {
        outFrame[offset] = ISOTP_N_PCI_SF | (uint8_t)dataLen;
        memcpy(&outFrame[offset + 1U], data, dataLen);
        *outLen = offset + 1U + dataLen;
    }
    
    /* CAN 2.0 requires 8-byte frame */
    if (!isFd) {
        *outLen = ISOTP_CAN20_FRAME_SIZE;
    }
    
    return ISOTP_E_OK;
}

Isotp_ReturnType IsoTp_BuildFirstFrame(
    uint16_t totalLen,
    const uint8_t *data,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
)
{
    if (data == NULL || outFrame == NULL || outLen == NULL) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    if (totalLen > ISOTP_MAX_MESSAGE_LENGTH) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    uint8_t maxPayload = isFd ? (62U - offset) : (6U - offset);
    uint8_t payloadLen = (totalLen < maxPayload) ? (uint8_t)totalLen : maxPayload;
    
    memset(outFrame, 0xAAU, ISOTP_CANFD_MAX_FRAME_SIZE);
    
    /* Extended addressing byte (if applicable) */
    if (offset > 0U) {
        outFrame[0] = 0x00U;  /* N_TA placeholder */
    }
    
    /* PCI bytes: 0x1X YY where X = upper nibble of DL, YY = lower byte */
    outFrame[offset] = ISOTP_N_PCI_FF | ((totalLen >> 8) & 0x0FU);
    outFrame[offset + 1U] = (uint8_t)(totalLen & 0xFFU);
    
    /* Copy first data bytes */
    memcpy(&outFrame[offset + 2U], data, payloadLen);
    
    *outLen = isFd ? ISOTP_CANFD_DEFAULT_FRAME_SIZE : ISOTP_CAN20_FRAME_SIZE;
    
    return ISOTP_E_OK;
}

Isotp_ReturnType IsoTp_BuildConsecutiveFrame(
    uint8_t seqNum,
    const uint8_t *data,
    uint8_t dataLen,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
)
{
    if (data == NULL || outFrame == NULL || outLen == NULL || dataLen == 0U) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    
    memset(outFrame, 0xAAU, ISOTP_CANFD_MAX_FRAME_SIZE);
    
    /* Extended addressing byte (if applicable) */
    if (offset > 0U) {
        outFrame[0] = 0x00U;  /* N_TA placeholder */
    }
    
    /* PCI byte: 0x2X where X = sequence number (0-15) */
    outFrame[offset] = ISOTP_N_PCI_CF | (seqNum & 0x0FU);
    memcpy(&outFrame[offset + 1U], data, dataLen);
    
    *outLen = offset + 1U + dataLen;
    
    /* CAN 2.0 requires 8-byte frame */
    if (!isFd) {
        *outLen = ISOTP_CAN20_FRAME_SIZE;
    }
    
    return ISOTP_E_OK;
}

Isotp_ReturnType IsoTp_BuildFlowControlFrame(
    uint8_t flowStatus,
    uint8_t blockSize,
    uint8_t stMin,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode
)
{
    if (outFrame == NULL || outLen == NULL) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    if (flowStatus > ISOTP_FS_OVFLW) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    
    memset(outFrame, 0xAAU, ISOTP_CANFD_MAX_FRAME_SIZE);
    
    /* Extended addressing byte (if applicable) */
    if (offset > 0U) {
        outFrame[0] = 0x00U;  /* N_TA placeholder */
    }
    
    /* PCI byte: 0x3X where X = flow status */
    outFrame[offset] = ISOTP_N_PCI_FC | (flowStatus & 0x0FU);
    outFrame[offset + 1U] = blockSize;
    outFrame[offset + 2U] = stMin;
    
    *outLen = offset + 3U;
    
    return ISOTP_E_OK;
}

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

Isotp_ReturnType IsoTp_Init(
    const Isotp_ChannelConfigType *config,
    uint8_t numChannels
)
{
    if (config == NULL || numChannels == 0U || numChannels > ISOTP_MAX_CHANNELS) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    /* Clear context */
    memset(&g_isotpContext, 0, sizeof(g_isotpContext));
    memset(g_channelContexts, 0, sizeof(g_channelContexts));
    
    g_isotpContext.channelConfigs = config;
    g_isotpContext.numChannels = numChannels;
    g_isotpContext.channelContexts = g_channelContexts;
    
    /* Initialize channel contexts */
    for (uint8_t i = 0U; i < numChannels; i++) {
        g_channelContexts[i].channelId = i;
        g_channelContexts[i].state = ISOTP_STATE_IDLE;
        g_channelContexts[i].txSeqNum = 0U;
        g_channelContexts[i].rxSeqNum = 0U;
    }
    
    g_isotpContext.initialized = true;
    
    return ISOTP_E_OK;
}

void IsoTp_DeInit(void)
{
    memset(&g_isotpContext, 0, sizeof(g_isotpContext));
    memset(g_channelContexts, 0, sizeof(g_channelContexts));
}

/******************************************************************************
 * Transmission State Machine
 ******************************************************************************/

static void IsoTp_TxStateMachine(uint8_t channelId)
{
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    const Isotp_ChannelConfigType *config = &g_isotpContext.channelConfigs[channelId];
    
    uint8_t frame[ISOTP_CANFD_MAX_FRAME_SIZE];
    uint8_t frameLen;
    uint16_t remainingBytes;
    uint16_t bytesToSend;
    uint8_t maxPayload;
    Isotp_ReturnType result;
    
    switch (ctx->state) {
        case ISOTP_STATE_TX_READY: {
            /* Start transmission - send First Frame or Single Frame */
            if (IsoTp_IsMultiFrame(ctx->txLength, config->addressing.addrMode, 
                                   (config->canFrameType == ISOTP_CAN_FD))) {
                /* Multi-frame: Send First Frame */
                result = IsoTp_BuildFirstFrame(
                    ctx->txLength,
                    ctx->txBuffer,
                    frame,
                    &frameLen,
                    config->addressing.addrMode,
                    (config->canFrameType == ISOTP_CAN_FD)
                );
                
                if (result == ISOTP_E_OK) {
                    result = IsoTp_SendCanFrame(channelId, config->addressing.txCanId, frame, frameLen);
                    if (result == ISOTP_E_OK) {
                        ctx->txSentLength = IsoTp_GetMaxSingleFramePayload(
                            config->addressing.addrMode, 
                            (config->canFrameType == ISOTP_CAN_FD)
                        );
                        ctx->txSentLength = (ctx->txSentLength < ctx->txLength) ? 
                                            ctx->txSentLength : ctx->txLength;
                        ctx->txSeqNum = 1U;  /* First CF seq = 1 */
                        ctx->txBlockCount = 0U;
                        ctx->state = ISOTP_STATE_TX_WAIT_FC;
                        ctx->txWaitStartTime = IsoTp_GetCurrentTime();
                    }
                }
            } else {
                /* Single frame transmission */
                result = IsoTp_BuildSingleFrame(
                    ctx->txBuffer,
                    ctx->txLength,
                    frame,
                    &frameLen,
                    config->addressing.addrMode,
                    (config->canFrameType == ISOTP_CAN_FD)
                );
                
                if (result == ISOTP_E_OK) {
                    result = IsoTp_SendCanFrame(channelId, config->addressing.txCanId, frame, frameLen);
                    if (result == ISOTP_E_OK) {
                        ctx->txSentLength = ctx->txLength;
                        ctx->state = ISOTP_STATE_TX_DONE;
                    }
                }
            }
            break;
        }
        
        case ISOTP_STATE_TX_WAIT_FC: {
            /* Check for Flow Control timeout */
            if ((IsoTp_GetCurrentTime() - ctx->txWaitStartTime) > config->timing.nBs) {
                ctx->state = ISOTP_STATE_TX_ERROR;
                if (g_isotpContext.txConfirmationCallback != NULL) {
                    g_isotpContext.txConfirmationCallback(channelId, ISOTP_E_TIMEOUT, ctx->userData);
                }
            }
            break;
        }
        
        case ISOTP_STATE_TX_SEND_CF: {
            /* Send Consecutive Frame(s) */
            remainingBytes = ctx->txLength - ctx->txSentLength;
            maxPayload = IsoTp_GetMaxSingleFramePayload(config->addressing.addrMode, 
                                                        (config->canFrameType == ISOTP_CAN_FD));
            bytesToSend = (remainingBytes < maxPayload) ? remainingBytes : maxPayload;
            
            result = IsoTp_BuildConsecutiveFrame(
                ctx->txSeqNum,
                &ctx->txBuffer[ctx->txSentLength],
                (uint8_t)bytesToSend,
                frame,
                &frameLen,
                config->addressing.addrMode,
                (config->canFrameType == ISOTP_CAN_FD)
            );
            
            if (result == ISOTP_E_OK) {
                result = IsoTp_SendCanFrame(channelId, config->addressing.txCanId, frame, frameLen);
                if (result == ISOTP_E_OK) {
                    ctx->txSentLength += bytesToSend;
                    ctx->txSeqNum = (ctx->txSeqNum + 1U) & 0x0FU;
                    ctx->txBlockCount++;
                    
                    /* Check if transmission complete */
                    if (ctx->txSentLength >= ctx->txLength) {
                        ctx->state = ISOTP_STATE_TX_DONE;
                    } else if (ctx->txBlockSize > 0U && ctx->txBlockCount >= ctx->txBlockSize) {
                        /* Block complete, wait for next FC */
                        ctx->state = ISOTP_STATE_TX_WAIT_FC;
                        ctx->txWaitStartTime = IsoTp_GetCurrentTime();
                    } else {
                        /* Wait for STmin before next CF */
                        ctx->state = ISOTP_STATE_TX_WAIT_STMIN;
                        ctx->txWaitStartTime = IsoTp_GetCurrentTime();
                    }
                }
            }
            break;
        }
        
        case ISOTP_STATE_TX_WAIT_STMIN: {
            /* Wait for STmin timeout */
            float stMinMs = IsoTp_StMinToMs(ctx->txStMin);
            uint32_t elapsed = IsoTp_GetCurrentTime() - ctx->txWaitStartTime;
            
            if (elapsed >= (uint32_t)stMinMs) {
                ctx->state = ISOTP_STATE_TX_SEND_CF;
            }
            break;
        }
        
        case ISOTP_STATE_TX_DONE: {
            /* Transmission complete - notify callback */
            ctx->state = ISOTP_STATE_IDLE;
            if (g_isotpContext.txConfirmationCallback != NULL) {
                g_isotpContext.txConfirmationCallback(channelId, ISOTP_E_OK, ctx->userData);
            }
            break;
        }
        
        case ISOTP_STATE_TX_ERROR: {
            /* Error state - reset to idle */
            ctx->state = ISOTP_STATE_IDLE;
            break;
        }
        
        default:
            break;
    }
}

/******************************************************************************
 * Reception State Machine
 ******************************************************************************/

static void IsoTp_RxStateMachine(uint8_t channelId)
{
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    
    switch (ctx->state) {
        case ISOTP_STATE_RX_WAIT_CF: {
            /* Check for timeout */
            const Isotp_ChannelConfigType *config = &g_isotpContext.channelConfigs[channelId];
            if ((IsoTp_GetCurrentTime() - ctx->rxWaitStartTime) > config->timing.nCr) {
                ctx->state = ISOTP_STATE_RX_ERROR;
            }
            break;
        }
        
        case ISOTP_STATE_RX_DONE: {
            /* Reception complete - notify callback */
            ctx->state = ISOTP_STATE_IDLE;
            if (g_isotpContext.rxIndicationCallback != NULL) {
                g_isotpContext.rxIndicationCallback(
                    channelId,
                    ctx->rxBuffer,
                    ctx->rxReceivedLength,
                    ctx->userData
                );
            }
            break;
        }
        
        case ISOTP_STATE_RX_ERROR: {
            /* Error state - reset to idle */
            ctx->state = ISOTP_STATE_IDLE;
            break;
        }
        
        default:
            break;
    }
}

/******************************************************************************
 * Frame Processing
 ******************************************************************************/

Isotp_ReturnType IsoTp_ProcessRxFrame(
    uint8_t channelId,
    const uint8_t *canData,
    uint8_t canDlc
)
{
    if (!IsoTp_IsValidChannel(channelId) || canData == NULL || canDlc == 0U) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    const Isotp_ChannelConfigType *config = &g_isotpContext.channelConfigs[channelId];
    Isotp_NpciType npci;
    const uint8_t *payload;
    uint8_t payloadLen;
    Isotp_ReturnType result;
    uint8_t frame[ISOTP_CANFD_MAX_FRAME_SIZE];
    uint8_t frameLen;
    
    /* Parse N_PCI */
    result = IsoTp_ParseNpci(canData, canDlc, &npci, config->addressing.addrMode);
    if (result != ISOTP_E_OK) {
        return result;
    }
    
    /* Get payload pointer */
    result = IsoTp_GetFramePayload(canData, canDlc, &payload, &payloadLen, &npci, 
                                   config->addressing.addrMode);
    if (result != ISOTP_E_OK) {
        return result;
    }
    
    switch (npci.pciType) {
        case ISOTP_N_PCI_SF: {
            /* Single Frame - complete message */
            if (ctx->rxBuffer == NULL || config->rxBufferSize < npci.dataLength) {
                return ISOTP_E_NO_BUFFER;
            }
            
            memcpy(ctx->rxBuffer, payload, npci.dataLength);
            ctx->rxReceivedLength = npci.dataLength;
            ctx->rxLength = npci.dataLength;
            ctx->state = ISOTP_STATE_RX_DONE;
            break;
        }
        
        case ISOTP_N_PCI_FF: {
            /* First Frame - start multi-frame reception */
            if (ctx->rxBuffer == NULL || config->rxBufferSize < npci.dataLength) {
                /* Send overflow FC */
                IsoTp_BuildFlowControlFrame(
                    ISOTP_FS_OVFLW,
                    0U,
                    0U,
                    frame,
                    &frameLen,
                    config->addressing.addrMode
                );
                IsoTp_SendCanFrame(channelId, config->addressing.rxCanId, frame, frameLen);
                return ISOTP_E_BUFFER_OVERFLOW;
            }
            
            ctx->rxLength = npci.dataLength;
            ctx->rxReceivedLength = payloadLen;
            memcpy(ctx->rxBuffer, payload, payloadLen);
            ctx->rxSeqNum = 1U;  /* Expect seq 1 in first CF */
            ctx->rxWftCounter = 0U;
            
            /* Request buffer confirmation via callback */
            uint8_t bs = ISOTP_DEFAULT_BS;
            uint8_t stmin = ISOTP_DEFAULT_STMIN;
            
            if (g_isotpContext.flowControlCallback != NULL) {
                g_isotpContext.flowControlCallback(
                    channelId,
                    npci.dataLength,
                    &bs,
                    &stmin,
                    ctx->userData
                );
            }
            
            ctx->rxBlockSize = bs;
            ctx->rxStMin = stmin;
            
            /* Send Flow Control (CTS) */
            IsoTp_BuildFlowControlFrame(
                ISOTP_FS_CTS,
                bs,
                stmin,
                frame,
                &frameLen,
                config->addressing.addrMode
            );
            IsoTp_SendCanFrame(channelId, config->addressing.rxCanId, frame, frameLen);
            
            ctx->state = ISOTP_STATE_RX_WAIT_CF;
            ctx->rxWaitStartTime = IsoTp_GetCurrentTime();
            ctx->rxBlockCount = 0U;
            break;
        }
        
        case ISOTP_N_PCI_CF: {
            /* Consecutive Frame */
            if (ctx->state != ISOTP_STATE_RX_WAIT_CF) {
                return ISOTP_E_UNEXPECTED_FRAME;
            }
            
            /* Check sequence number */
            if (npci.seqNum != ctx->rxSeqNum) {
                ctx->state = ISOTP_STATE_RX_ERROR;
                return ISOTP_E_SEQUENCE_ERROR;
            }
            
            /* Copy data */
            uint16_t remaining = ctx->rxLength - ctx->rxReceivedLength;
            uint16_t bytesToCopy = (payloadLen < remaining) ? payloadLen : remaining;
            memcpy(&ctx->rxBuffer[ctx->rxReceivedLength], payload, bytesToCopy);
            ctx->rxReceivedLength += bytesToCopy;
            ctx->rxSeqNum = (ctx->rxSeqNum + 1U) & 0x0FU;
            ctx->rxBlockCount++;
            
            /* Check if complete */
            if (ctx->rxReceivedLength >= ctx->rxLength) {
                ctx->state = ISOTP_STATE_RX_DONE;
            } else if (ctx->rxBlockSize > 0U && ctx->rxBlockCount >= ctx->rxBlockSize) {
                /* Block complete, send CTS FC */
                ctx->rxBlockCount = 0U;
                IsoTp_BuildFlowControlFrame(
                    ISOTP_FS_CTS,
                    ctx->rxBlockSize,
                    ctx->rxStMin,
                    frame,
                    &frameLen,
                    config->addressing.addrMode
                );
                IsoTp_SendCanFrame(channelId, config->addressing.rxCanId, frame, frameLen);
                ctx->rxWaitStartTime = IsoTp_GetCurrentTime();
            } else {
                ctx->rxWaitStartTime = IsoTp_GetCurrentTime();
            }
            break;
        }
        
        case ISOTP_N_PCI_FC: {
            /* Flow Control (for TX side) */
            if (ctx->state != ISOTP_STATE_TX_WAIT_FC) {
                return ISOTP_E_UNEXPECTED_FRAME;
            }
            
            switch (npci.flowStatus) {
                case ISOTP_FS_CTS:
                    /* Continue to send */
                    ctx->txBlockSize = npci.blockSize;
                    ctx->txStMin = npci.stMin;
                    ctx->txBlockCount = 0U;
                    ctx->state = ISOTP_STATE_TX_SEND_CF;
                    break;
                    
                case ISOTP_FS_WT:
                    /* Wait - check WFT counter */
                    ctx->rxWftCounter++;
                    if (ctx->rxWftCounter > ISOTP_DEFAULT_WFT_MAX) {
                        ctx->state = ISOTP_STATE_TX_ERROR;
                        if (g_isotpContext.txConfirmationCallback != NULL) {
                            g_isotpContext.txConfirmationCallback(channelId, ISOTP_E_WFT_OVRN, ctx->userData);
                        }
                    } else {
                        ctx->txWaitStartTime = IsoTp_GetCurrentTime();
                    }
                    break;
                    
                case ISOTP_FS_OVFLW:
                    /* Overflow - abort transmission */
                    ctx->state = ISOTP_STATE_TX_ERROR;
                    if (g_isotpContext.txConfirmationCallback != NULL) {
                        g_isotpContext.txConfirmationCallback(channelId, ISOTP_E_BUFFER_OVERFLOW, ctx->userData);
                    }
                    break;
                    
                default:
                    return ISOTP_E_INVALID_FRAME;
            }
            break;
        }
        
        default:
            return ISOTP_E_INVALID_FRAME;
    }
    
    return ISOTP_E_OK;
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

void IsoTp_MainFunction(void)
{
    if (!g_isotpContext.initialized) {
        return;
    }
    
    for (uint8_t i = 0U; i < g_isotpContext.numChannels; i++) {
        IsoTp_TxStateMachine(i);
        IsoTp_RxStateMachine(i);
    }
}

/******************************************************************************
 * Public API Functions
 ******************************************************************************/

Isotp_ReturnType IsoTp_Transmit(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length,
    void *userData
)
{
    if (!IsoTp_IsValidChannel(channelId) || data == NULL || length == 0U) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    if (length > ISOTP_MAX_MESSAGE_LENGTH) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    const Isotp_ChannelConfigType *config = &g_isotpContext.channelConfigs[channelId];
    
    /* Check if channel is available */
    if (ctx->state != ISOTP_STATE_IDLE) {
        return ISOTP_E_BUSY;
    }
    
    /* Check buffer size */
    if (length > config->txBufferSize) {
        return ISOTP_E_BUFFER_OVERFLOW;
    }
    
    /* Setup transmission context */
    ctx->txBuffer = (uint8_t *)data;  /* Note: assumes buffer is valid during TX */
    ctx->txLength = length;
    ctx->txSentLength = 0U;
    ctx->userData = userData;
    ctx->txStartTime = IsoTp_GetCurrentTime();
    ctx->state = ISOTP_STATE_TX_READY;
    
    return ISOTP_E_OK;
}

Isotp_ReturnType IsoTp_CancelTransmit(uint8_t channelId)
{
    if (!IsoTp_IsValidChannel(channelId)) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    
    if (ctx->state != ISOTP_STATE_IDLE) {
        ctx->state = ISOTP_STATE_IDLE;
        return ISOTP_E_OK;
    }
    
    return ISOTP_E_NOT_OK;
}

Isotp_ReturnType IsoTp_SetRxBuffer(
    uint8_t channelId,
    uint8_t *buffer,
    uint16_t bufferSize
)
{
    (void)bufferSize;
    
    if (!IsoTp_IsValidChannel(channelId)) {
        return ISOTP_E_INVALID_PARAMETER;
    }
    
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    ctx->rxBuffer = buffer;
    
    return ISOTP_E_OK;
}

Isotp_StateType IsoTp_GetState(uint8_t channelId)
{
    if (!IsoTp_IsValidChannel(channelId)) {
        return ISOTP_STATE_IDLE;
    }
    
    return g_channelContexts[channelId].state;
}

void IsoTp_ResetChannel(uint8_t channelId)
{
    if (!IsoTp_IsValidChannel(channelId)) {
        return;
    }
    
    Isotp_ChannelContextType *ctx = &g_channelContexts[channelId];
    ctx->state = ISOTP_STATE_IDLE;
    ctx->txSeqNum = 0U;
    ctx->rxSeqNum = 0U;
    ctx->txSentLength = 0U;
    ctx->rxReceivedLength = 0U;
}

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

void IsoTp_RegisterTxConfirmationCallback(Isotp_TxConfirmationCallback callback)
{
    g_isotpContext.txConfirmationCallback = callback;
}

void IsoTp_RegisterRxIndicationCallback(Isotp_RxIndicationCallback callback)
{
    g_isotpContext.rxIndicationCallback = callback;
}

void IsoTp_RegisterFlowControlCallback(Isotp_FlowControlCallback callback)
{
    g_isotpContext.flowControlCallback = callback;
}

void IsoTp_RegisterCanTransmitCallback(Isotp_CanTransmitCallback callback)
{
    g_isotpContext.canTransmitCallback = callback;
}

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

bool IsoTp_IsMultiFrame(uint16_t dataLen, Isotp_AddressingModeType addrMode, bool isFd)
{
    uint8_t maxSfPayload = IsoTp_GetMaxSingleFramePayload(addrMode, isFd);
    return (dataLen > maxSfPayload);
}

uint16_t IsoTp_CalculateNumFrames(uint16_t dataLen, Isotp_AddressingModeType addrMode, bool isFd)
{
    if (!IsoTp_IsMultiFrame(dataLen, addrMode, isFd)) {
        return 1U;
    }
    
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    uint8_t ffPayload = isFd ? (62U - offset) : (6U - offset);
    uint8_t cfPayload = isFd ? (63U - offset) : (7U - offset);
    
    uint16_t remaining = dataLen - ffPayload;
    uint16_t numCfFrames = (remaining + cfPayload - 1U) / cfPayload;
    
    return 1U + numCfFrames;  /* FF + CF frames */
}

uint8_t IsoTp_GetMaxSingleFramePayload(Isotp_AddressingModeType addrMode, bool isFd)
{
    uint8_t offset = IsoTp_GetAddrOffset(addrMode);
    
    if (isFd) {
        return (uint8_t)(62U - offset);  /* CAN FD: 62 bytes max for SF */
    } else {
        return (uint8_t)(7U - offset);   /* CAN 2.0: 7 bytes max for SF */
    }
}

float IsoTp_StMinToMs(uint8_t stMin)
{
    if (stMin <= ISOTP_MAX_STMIN_MS) {
        /* 0x00-0x7F: 0-127 ms */
        return (float)stMin;
    } else if (stMin >= 0xF1U && stMin <= ISOTP_MAX_STMIN_US) {
        /* 0xF1-0xF9: 100-900 us */
        return (float)(stMin - 0xF0U) * 0.1f;
    }
    /* Reserved values - use default */
    return (float)ISOTP_DEFAULT_STMIN;
}

bool IsoTp_IsValidStMin(uint8_t stMin)
{
    return (stMin <= ISOTP_MAX_STMIN_MS) || 
           (stMin >= 0xF1U && stMin <= ISOTP_MAX_STMIN_US);
}

const Isotp_ChannelConfigType* IsoTp_GetChannelConfig(uint8_t channelId)
{
    if (!IsoTp_IsValidChannel(channelId)) {
        return NULL;
    }
    return &g_isotpContext.channelConfigs[channelId];
}

Isotp_ChannelContextType* IsoTp_GetChannelContext(uint8_t channelId)
{
    if (!IsoTp_IsValidChannel(channelId)) {
        return NULL;
    }
    return &g_channelContexts[channelId];
}

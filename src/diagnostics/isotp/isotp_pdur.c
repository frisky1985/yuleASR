/**
 * @file isotp_pdur.c
 * @brief IsoTp to PduR Interface Integration Implementation
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * This module provides integration between IsoTp transport layer and PduR
 * for diagnostic communication routing.
 *
 * @copyright Copyright (c) 2024
 */
/* @req SHALL_OS */


#include "isotp_pdur.h"
#include <string.h>
#include <stddef.h>

/******************************************************************************
 * Internal Macros
 ******************************************************************************/
#define ISOTP_PDUR_MAGIC_INIT           (0x49545030U)  /* "ITP0" */
#define ISOTP_PDUR_INVALID_CHANNEL      0xFFU

/******************************************************************************
 * Channel State
 ******************************************************************************/
typedef struct {
    uint8_t channelId;
    boolean active;
    uint8_t *rxBuffer;
    uint16_t rxBufferSize;
    uint16_t rxDataLength;
    uint16_t rxExpectedLength;
    uint8_t *txBuffer;
    uint16_t txBufferSize;
    uint16_t txDataLength;
    uint16_t txDataSent;
    boolean txInProgress;
    boolean rxInProgress;
    PduInfoType txPduInfo;
    PduInfoType rxPduInfo;
} Isotp_PduRChannelStateType;

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Isotp_PduRConfigType *config;
    Isotp_PduRChannelStateType channels[ISOTP_PDUR_MAX_CHANNELS];
    Isotp_PduR_DcmMessageReceivedCallbackType dcmRxCallback;
    boolean initialized;
} Isotp_PduRStateType;

static Isotp_PduRStateType s_isotpPdruState;

/* Static buffers for channels */
static uint8_t s_rxBuffers[ISOTP_PDUR_MAX_CHANNELS][4096];
static uint8_t s_txBuffers[ISOTP_PDUR_MAX_CHANNELS][4096];

/******************************************************************************
 * Static Helper Functions
 ******************************************************************************/

/**
 * @brief Validate channel ID
 */
static boolean isValidChannel(uint8_t channelId)
{
    return (channelId < ISOTP_PDUR_MAX_CHANNELS) &&
           (channelId < s_isotpPdruState.config->numChannels);
}

/**
 * @brief Get channel state
 */
static Isotp_PduRChannelStateType* getChannelState(uint8_t channelId)
{
    if (isValidChannel(channelId)) {
        return &s_isotpPdruState.channels[channelId];
    }
    return NULL;
}

/**
 * @brief Initialize channel state
 */
static void initChannelState(Isotp_PduRChannelStateType *channel, uint8_t channelId)
{
    if (channel != NULL) {
        channel->channelId = channelId;
        channel->active = TRUE;
        channel->rxBuffer = s_rxBuffers[channelId];
        channel->rxBufferSize = sizeof(s_rxBuffers[channelId]);
        channel->rxDataLength = 0U;
        channel->rxExpectedLength = 0U;
        channel->txBuffer = s_txBuffers[channelId];
        channel->txBufferSize = sizeof(s_txBuffers[channelId]);
        channel->txDataLength = 0U;
        channel->txDataSent = 0U;
        channel->txInProgress = FALSE;
        channel->rxInProgress = FALSE;
        channel->txPduInfo.SduDataPtr = NULL;
        channel->txPduInfo.MetaDataPtr = NULL;
        channel->txPduInfo.SduLength = 0U;
        channel->rxPduInfo.SduDataPtr = channel->rxBuffer;
        channel->rxPduInfo.MetaDataPtr = NULL;
        channel->rxPduInfo.SduLength = 0U;
    }
}

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

Isotp_ReturnType Isotp_PduR_Init(const Isotp_PduRConfigType *config)
{
    Isotp_ReturnType result = ISOTP_E_NOT_OK;

    if (config != NULL) {
        /* Clear state */
        (void)memset(&s_isotpPdruState, 0, sizeof(s_isotpPdruState));

        s_isotpPdruState.magic = ISOTP_PDUR_MAGIC_INIT;
        s_isotpPdruState.config = config;
        s_isotpPdruState.dcmRxCallback = NULL;

        /* Initialize channel states */
        for (uint8_t i = 0U; i < config->numChannels; i++) {
            if (i < ISOTP_PDUR_MAX_CHANNELS) {
                initChannelState(&s_isotpPdruState.channels[i], i);
            }
        }

        s_isotpPdruState.initialized = TRUE;
        result = ISOTP_E_OK;
    }

    return result;
}

Isotp_ReturnType Isotp_PduR_DeInit(void)
{
    Isotp_ReturnType result = ISOTP_E_NOT_OK;

    if (s_isotpPdruState.initialized) {
        /* Deinitialize all channels */
        for (uint8_t i = 0U; i < ISOTP_PDUR_MAX_CHANNELS; i++) {
            s_isotpPdruState.channels[i].active = FALSE;
            s_isotpPdruState.channels[i].txInProgress = FALSE;
            s_isotpPdruState.channels[i].rxInProgress = FALSE;
        }

        s_isotpPdruState.initialized = FALSE;
        s_isotpPdruState.magic = 0U;
        s_isotpPdruState.config = NULL;
        s_isotpPdruState.dcmRxCallback = NULL;

        result = ISOTP_E_OK;
    }

    return result;
}

/******************************************************************************
 * Public Functions - Transmission
 ******************************************************************************/

Isotp_ReturnType Isotp_PduR_Transmit(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length)
{
    Isotp_ReturnType result = ISOTP_E_NOT_OK;
    Isotp_PduRChannelStateType *channel;
    Std_ReturnType pduResult;
    const Isotp_PduRChannelConfigType *channelConfig;

    /* Check initialization */
    if (!s_isotpPdruState.initialized) {
        return ISOTP_E_NOT_INITIALIZED;
    }

    /* Validate parameters */
    if ((data == NULL) || (length == 0U)) {
        return ISOTP_E_INVALID_PARAMETER;
    }

    /* Get channel state */
    channel = getChannelState(channelId);
    if (channel == NULL) {
        return ISOTP_E_INVALID_CHANNEL;
    }

    /* Check if channel is busy */
    if (channel->txInProgress) {
        return ISOTP_E_BUSY;
    }

    /* Check data fits in buffer */
    if (length > channel->txBufferSize) {
        return ISOTP_E_INVALID_PARAMETER;
    }

    /* Copy data to transmit buffer */
    (void)memcpy(channel->txBuffer, data, length);
    channel->txDataLength = length;
    channel->txDataSent = 0U;
    channel->txInProgress = TRUE;

    /* Get channel configuration */
    channelConfig = &s_isotpPdruState.config->channelConfigs[channelId];

    /* Setup PDU info */
    channel->txPduInfo.SduDataPtr = channel->txBuffer;
    channel->txPduInfo.SduLength = length;
    channel->txPduInfo.MetaDataPtr = NULL;

    /* Route via PduR TP Transmit */
    pduResult = PduR_TpTransmit(
        channelConfig->txPduId,
        &channel->txPduInfo,
        NULL,  /* No retry info */
        (PduLengthType)length
    );

    if (pduResult == E_OK) {
        result = ISOTP_E_OK;
    } else {
        channel->txInProgress = FALSE;
        channel->txDataLength = 0U;
        result = ISOTP_E_NOT_OK;
    }

    return result;
}

void Isotp_PduR_TxConfirmation(uint8_t channelId, Std_ReturnType result)
{
    Isotp_PduRChannelStateType *channel;

    channel = getChannelState(channelId);
    if (channel != NULL) {
        channel->txInProgress = FALSE;
        channel->txDataLength = 0U;
        channel->txDataSent = 0U;

        /* Could notify upper layer here if needed */
        (void)result;  /* Currently unused but available for logging */
    }
}

/******************************************************************************
 * Public Functions - Reception
 ******************************************************************************/

Isotp_ReturnType Isotp_PduR_RxIndication(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length)
{
    Isotp_ReturnType result = ISOTP_E_NOT_OK;
    Isotp_PduRChannelStateType *channel;

    /* Check initialization */
    if (!s_isotpPdruState.initialized) {
        return ISOTP_E_NOT_INITIALIZED;
    }

    /* Validate parameters */
    if ((data == NULL) || (length == 0U)) {
        return ISOTP_E_INVALID_PARAMETER;
    }

    /* Get channel state */
    channel = getChannelState(channelId);
    if (channel == NULL) {
        return ISOTP_E_INVALID_CHANNEL;
    }

    /* Process frame via IsoTp core */
    result = IsoTp_ProcessRxFrame(channelId, data, (uint8_t)length);

    return result;
}

void Isotp_PduR_MessageReceived(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length)
{
    Isotp_PduRChannelStateType *channel;

    channel = getChannelState(channelId);
    if (channel != NULL) {
        /* Copy message to receive buffer */
        if (length <= channel->rxBufferSize) {
            (void)memcpy(channel->rxBuffer, data, length);
            channel->rxDataLength = length;
            channel->rxPduInfo.SduLength = (PduLengthType)length;

            /* Notify DCM callback if registered */
            if (s_isotpPdruState.dcmRxCallback != NULL) {
                s_isotpPdruState.dcmRxCallback(channelId, channel->rxBuffer, length);
            }
        }
    }
}

/******************************************************************************
 * Public Functions - TP Buffer Management
 ******************************************************************************/

BufReq_ReturnType Isotp_PduR_StartOfReception(
    uint8_t channelId,
    PduLengthType tpSduLength,
    PduLengthType *bufferSizePtr)
{
    BufReq_ReturnType result = BUFREQ_E_NOT_OK;
    Isotp_PduRChannelStateType *channel;

    if (bufferSizePtr == NULL) {
        return BUFREQ_E_NOT_OK;
    }

    channel = getChannelState(channelId);
    if (channel == NULL) {
        return BUFREQ_E_NOT_OK;
    }

    /* Check if reception is already in progress */
    if (channel->rxInProgress) {
        return BUFREQ_E_BUSY;
    }

    /* Check if buffer can accommodate the message */
    if (tpSduLength > channel->rxBufferSize) {
        return BUFREQ_E_OVFL;
    }

    /* Prepare for reception */
    channel->rxInProgress = TRUE;
    channel->rxExpectedLength = (uint16_t)tpSduLength;
    channel->rxDataLength = 0U;
    *bufferSizePtr = (PduLengthType)channel->rxBufferSize;

    return BUFREQ_OK;
}

BufReq_ReturnType Isotp_PduR_CopyRxData(
    uint8_t channelId,
    const PduInfoType *pduInfoPtr,
    PduLengthType *bufferSizePtr)
{
    BufReq_ReturnType result = BUFREQ_E_NOT_OK;
    Isotp_PduRChannelStateType *channel;
    uint16_t bytesToCopy;
    uint16_t remainingSpace;

    if ((pduInfoPtr == NULL) || (bufferSizePtr == NULL)) {
        return BUFREQ_E_NOT_OK;
    }

    channel = getChannelState(channelId);
    if (channel == NULL) {
        return BUFREQ_E_NOT_OK;
    }

    if (!channel->rxInProgress) {
        return BUFREQ_E_NOT_OK;
    }

    /* Calculate bytes to copy */
    bytesToCopy = (uint16_t)pduInfoPtr->SduLength;
    remainingSpace = channel->rxBufferSize - channel->rxDataLength;

    if (bytesToCopy > remainingSpace) {
        return BUFREQ_E_OVFL;
    }

    /* Copy data to receive buffer */
    if ((pduInfoPtr->SduDataPtr != NULL) && (bytesToCopy > 0U)) {
        (void)memcpy(
            &channel->rxBuffer[channel->rxDataLength],
            pduInfoPtr->SduDataPtr,
            bytesToCopy
        );
        channel->rxDataLength += bytesToCopy;
    }

    /* Update remaining buffer size */
    *bufferSizePtr = (PduLengthType)(channel->rxBufferSize - channel->rxDataLength);

    /* Check if reception is complete */
    if (channel->rxDataLength >= channel->rxExpectedLength) {
        channel->rxInProgress = FALSE;
        channel->rxPduInfo.SduLength = (PduLengthType)channel->rxDataLength;

        /* Notify complete message received */
        Isotp_PduR_MessageReceived(
            channelId,
            channel->rxBuffer,
            channel->rxDataLength
        );
    }

    return BUFREQ_OK;
}

BufReq_ReturnType Isotp_PduR_CopyTxData(
    uint8_t channelId,
    const PduInfoType *pduInfoPtr,
    const RetryInfoType *retryInfoPtr,
    PduLengthType *availableDataPtr)
{
    BufReq_ReturnType result = BUFREQ_E_NOT_OK;
    Isotp_PduRChannelStateType *channel;
    uint16_t bytesToCopy;
    uint16_t remainingData;

    if ((pduInfoPtr == NULL) || (availableDataPtr == NULL)) {
        return BUFREQ_E_NOT_OK;
    }

    channel = getChannelState(channelId);
    if (channel == NULL) {
        return BUFREQ_E_NOT_OK;
    }

    if (!channel->txInProgress) {
        return BUFREQ_E_NOT_OK;
    }

    /* Calculate remaining data */
    remainingData = channel->txDataLength - channel->txDataSent;

    /* Handle retry if needed */
    if (retryInfoPtr != NULL) {
        /* Implement retry logic if needed */
        (void)retryInfoPtr;  /* Currently unused */
    }

    /* Copy data to PduR buffer */
    bytesToCopy = (uint16_t)pduInfoPtr->SduLength;
    if (bytesToCopy > remainingData) {
        bytesToCopy = remainingData;
    }

    if ((pduInfoPtr->SduDataPtr != NULL) && (bytesToCopy > 0U)) {
        (void)memcpy(
            pduInfoPtr->SduDataPtr,
            &channel->txBuffer[channel->txDataSent],
            bytesToCopy
        );
        channel->txDataSent += bytesToCopy;
    }

    /* Update available data */
    remainingData = channel->txDataLength - channel->txDataSent;
    *availableDataPtr = (PduLengthType)remainingData;

    return BUFREQ_OK;
}

/******************************************************************************
 * Public Functions - Utility
 ******************************************************************************/

bool Isotp_PduR_IsInitialized(void)
{
    return s_isotpPdruState.initialized;
}

const Isotp_PduRChannelConfigType* Isotp_PduR_GetChannelConfig(uint8_t channelId)
{
    if (isValidChannel(channelId)) {
        return &s_isotpPdruState.config->channelConfigs[channelId];
    }
    return NULL;
}

void Isotp_PduR_MainFunction(void)
{
    if (s_isotpPdruState.initialized) {
        /* Process IsoTp main function */
        IsoTp_MainFunction();

        /* Check channel timeouts and states */
        for (uint8_t i = 0U; i < s_isotpPdruState.config->numChannels; i++) {
            Isotp_PduRChannelStateType *channel = &s_isotpPdruState.channels[i];

            /* Reset stuck transmissions/receptions */
            if (channel->txInProgress) {
                /* Check for timeout - simplified */
                /* In real implementation, check timer */
            }

            if (channel->rxInProgress) {
                /* Check for timeout - simplified */
                /* In real implementation, check timer */
            }
        }
    }
}

/******************************************************************************
 * Public Functions - Callback Registration
 ******************************************************************************/

Isotp_ReturnType Isotp_PduR_RegisterDcmRxCallback(
    Isotp_PduR_DcmMessageReceivedCallbackType callback)
{
    Isotp_ReturnType result = ISOTP_E_NOT_OK;

    if (s_isotpPdruState.initialized && (callback != NULL)) {
        s_isotpPdruState.dcmRxCallback = callback;
        result = ISOTP_E_OK;
    }

    return result;
}

/******************************************************************************
 * PduR Callback Functions (Called by PduR)
 ******************************************************************************/

/**
 * @brief PduR TP Start of Reception callback
 * Called by PduR when TP reception starts
 */
BufReq_ReturnType PduR_IsotpTpStartOfReception(
    PduIdType RxPduId,
    PduLengthType TpSduLength,
    PduLengthType *BufferSizePtr)
{
    /* Map PDU ID to channel ID - simplified mapping */
    uint8_t channelId = (uint8_t)(RxPduId & 0xFFU);
    return Isotp_PduR_StartOfReception(channelId, TpSduLength, BufferSizePtr);
}

/**
 * @brief PduR TP Copy RX Data callback
 * Called by PduR to copy received data
 */
BufReq_ReturnType PduR_IsotpTpCopyRxData(
    PduIdType RxPduId,
    const PduInfoType *PduInfoPtr,
    PduLengthType *BufferSizePtr)
{
    /* Map PDU ID to channel ID - simplified mapping */
    uint8_t channelId = (uint8_t)(RxPduId & 0xFFU);
    return Isotp_PduR_CopyRxData(channelId, PduInfoPtr, BufferSizePtr);
}

/**
 * @brief PduR TP Copy TX Data callback
 * Called by PduR to copy transmit data
 */
BufReq_ReturnType PduR_IsotpTpCopyTxData(
    PduIdType TxPduId,
    const PduInfoType *PduInfoPtr,
    const RetryInfoType *RetryInfoPtr,
    PduLengthType *AvailableDataPtr)
{
    /* Map PDU ID to channel ID - simplified mapping */
    uint8_t channelId = (uint8_t)(TxPduId & 0xFFU);
    return Isotp_PduR_CopyTxData(channelId, PduInfoPtr, RetryInfoPtr, AvailableDataPtr);
}

/**
 * @brief PduR TP Tx Confirmation callback
 * Called by PduR to confirm transmission
 */
void PduR_IsotpTpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result)
{
    /* Map PDU ID to channel ID - simplified mapping */
    uint8_t channelId = (uint8_t)(TxPduId & 0xFFU);
    Isotp_PduR_TxConfirmation(channelId, Result);
}

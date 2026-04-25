/******************************************************************************
 * @file    isotp_core.h
 * @brief   ISO 15765-2 IsoTp Transport Layer Core Module
 *
 * ISO 15765-2:2016 compliant
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ISOTP_CORE_H
#define ISOTP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "isotp_types.h"

/******************************************************************************
 * Initialization and Lifecycle
 ******************************************************************************/

/**
 * @brief Initialize IsoTp module
 * @param config Channel configurations array
 * @param numChannels Number of channels
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_Init(
    const Isotp_ChannelConfigType *config,
    uint8_t numChannels
);

/**
 * @brief Deinitialize IsoTp module
 */
void IsoTp_DeInit(void);

/**
 * @brief Main processing function (to be called cyclically)
 */
void IsoTp_MainFunction(void);

/******************************************************************************
 * Transmission Functions
 ******************************************************************************/

/**
 * @brief Transmit data via IsoTp
 * @param channelId Channel ID
 * @param data Data buffer
 * @param length Data length
 * @param userData User data pointer for callback
 * @return ISOTP_E_OK on success, ISOTP_E_BUSY if channel busy
 */
Isotp_ReturnType IsoTp_Transmit(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length,
    void *userData
);

/**
 * @brief Cancel ongoing transmission
 * @param channelId Channel ID
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_CancelTransmit(uint8_t channelId);

/******************************************************************************
 * Reception Functions
 ******************************************************************************/

/**
 * @brief Process received CAN frame
 * @param channelId Channel ID
 * @param canData CAN frame data
 * @param canDlc Data length code
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_ProcessRxFrame(
    uint8_t channelId,
    const uint8_t *canData,
    uint8_t canDlc
);

/**
 * @brief Provide RX buffer for reception
 * @param channelId Channel ID
 * @param buffer Buffer pointer
 * @param bufferSize Buffer size
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_SetRxBuffer(
    uint8_t channelId,
    uint8_t *buffer,
    uint16_t bufferSize
);

/******************************************************************************
 * Frame Building Functions
 ******************************************************************************/

/**
 * @brief Build Single Frame
 * @param data Input data
 * @param dataLen Data length
 * @param outFrame Output frame buffer
 * @param outLen Output frame length
 * @param addrMode Addressing mode
 * @param isFd CAN FD flag
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_BuildSingleFrame(
    const uint8_t *data,
    uint16_t dataLen,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
);

/**
 * @brief Build First Frame
 * @param totalLen Total message length
 * @param data First data bytes
 * @param outFrame Output frame buffer
 * @param outLen Output frame length
 * @param addrMode Addressing mode
 * @param isFd CAN FD flag
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_BuildFirstFrame(
    uint16_t totalLen,
    const uint8_t *data,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
);

/**
 * @brief Build Consecutive Frame
 * @param seqNum Sequence number
 * @param data Data bytes
 * @param dataLen Data length
 * @param outFrame Output frame buffer
 * @param outLen Output frame length
 * @param addrMode Addressing mode
 * @param isFd CAN FD flag
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_BuildConsecutiveFrame(
    uint8_t seqNum,
    const uint8_t *data,
    uint8_t dataLen,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
);

/**
 * @brief Build Flow Control Frame
 * @param flowStatus Flow status (CTS, WT, OVFLW)
 * @param blockSize Block size
 * @param stMin Separation time minimum
 * @param outFrame Output frame buffer
 * @param outLen Output frame length
 * @param addrMode Addressing mode
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_BuildFlowControlFrame(
    uint8_t flowStatus,
    uint8_t blockSize,
    uint8_t stMin,
    uint8_t *outFrame,
    uint8_t *outLen,
    Isotp_AddressingModeType addrMode
);

/******************************************************************************
 * Frame Parsing Functions
 ******************************************************************************/

/**
 * @brief Parse N_PCI from CAN frame
 * @param frame CAN frame data
 * @param dlc Frame DLC
 * @param npci Parsed N_PCI structure
 * @param addrMode Addressing mode
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_ParseNpci(
    const uint8_t *frame,
    uint8_t dlc,
    Isotp_NpciType *npci,
    Isotp_AddressingModeType addrMode
);

/**
 * @brief Get frame data payload
 * @param frame CAN frame data
 * @param dlc Frame DLC
 * @param payload Output payload pointer (offset into frame)
 * @param payloadLen Output payload length
 * @param addrMode Addressing mode
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType IsoTp_GetFramePayload(
    const uint8_t *frame,
    uint8_t dlc,
    const uint8_t **payload,
    uint8_t *payloadLen,
    Isotp_NpciType *npci,
    Isotp_AddressingModeType addrMode
);

/******************************************************************************
 * State Machine Functions
 ******************************************************************************/

/**
 * @brief Get channel state
 * @param channelId Channel ID
 * @return Current state
 */
Isotp_StateType IsoTp_GetState(uint8_t channelId);

/**
 * @brief Reset channel to idle state
 * @param channelId Channel ID
 */
void IsoTp_ResetChannel(uint8_t channelId);

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

/**
 * @brief Register transmit confirmation callback
 * @param callback Callback function
 */
void IsoTp_RegisterTxConfirmationCallback(Isotp_TxConfirmationCallback callback);

/**
 * @brief Register reception indication callback
 * @param callback Callback function
 */
void IsoTp_RegisterRxIndicationCallback(Isotp_RxIndicationCallback callback);

/**
 * @brief Register flow control callback
 * @param callback Callback function
 */
void IsoTp_RegisterFlowControlCallback(Isotp_FlowControlCallback callback);

/**
 * @brief Register CAN transmit callback
 * @param callback Callback function
 */
void IsoTp_RegisterCanTransmitCallback(Isotp_CanTransmitCallback callback);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Check if message needs multi-frame
 * @param dataLen Data length
 * @param addrMode Addressing mode
 * @param isFd CAN FD flag
 * @return true if multi-frame needed
 */
bool IsoTp_IsMultiFrame(
    uint16_t dataLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
);

/**
 * @brief Calculate number of CAN frames needed
 * @param dataLen Data length
 * @param addrMode Addressing mode
 * @param isFd CAN FD flag
 * @return Number of frames
 */
uint16_t IsoTp_CalculateNumFrames(
    uint16_t dataLen,
    Isotp_AddressingModeType addrMode,
    bool isFd
);

/**
 * @brief Get maximum payload for Single Frame
 * @param addrMode Addressing mode
 * @param isFd CAN FD flag
 * @return Maximum payload size
 */
uint8_t IsoTp_GetMaxSingleFramePayload(
    Isotp_AddressingModeType addrMode,
    bool isFd
);

/**
 * @brief Convert STmin value to milliseconds
 * @param stMin STmin value (0-0x7F = ms, 0xF1-0xF9 = 100-900us)
 * @return STmin in milliseconds (fractional values rounded up)
 */
float32_t IsoTp_StMinToMs(uint8_t stMin);

/**
 * @brief Validate STmin value
 * @param stMin STmin value to validate
 * @return true if valid
 */
bool IsoTp_IsValidStMin(uint8_t stMin);

/**
 * @brief Get channel configuration
 * @param channelId Channel ID
 * @return Configuration pointer or NULL
 */
const Isotp_ChannelConfigType* IsoTp_GetChannelConfig(uint8_t channelId);

/**
 * @brief Get channel context
 * @param channelId Channel ID
 * @return Context pointer or NULL
 */
Isotp_ChannelContextType* IsoTp_GetChannelContext(uint8_t channelId);

#ifdef __cplusplus
}
#endif

#endif /* ISOTP_CORE_H */

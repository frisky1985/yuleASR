/**
 * @file isotp_pdur.h
 * @brief IsoTp to PduR Interface Integration
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * This module provides integration between IsoTp transport layer and PduR
 * for diagnostic communication routing.
 *
 * @copyright Copyright (c) 2024
 */

#ifndef ISOTP_PDUR_H
#define ISOTP_PDUR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "isotp_core.h"
#include "../../bsw/services/pdur/include/PduR.h"

/* PduR_ModuleType fallback: master PduR.h 未定义该枚举, 归档版有 (PDUR_MODULE_*) */
#ifndef ISOTP_PDUR_MODULE_TYPE_FALLBACK
#define ISOTP_PDUR_MODULE_TYPE_FALLBACK
#ifndef PDUR_MODULE_TYPE_DEFINED
typedef uint8_t PduR_ModuleType;
#endif
#endif

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define ISOTP_PDUR_MAJOR_VERSION        1U
#define ISOTP_PDUR_MINOR_VERSION        0U
#define ISOTP_PDUR_PATCH_VERSION        0U

/******************************************************************************
 * PDU IDs for Diagnostic Communication
 ******************************************************************************/
/* DCM to IsoTp Tx PDU ID */
#define ISOTP_PDUR_DCM_TX_PDU_ID        0x0100U
/* IsoTp to DCM Rx PDU ID */
#define ISOTP_PDUR_DCM_RX_PDU_ID        0x0101U
/* Maximum number of diagnostic channels */
#define ISOTP_PDUR_MAX_CHANNELS         4U

/******************************************************************************
 * Configuration Types
 ******************************************************************************/
/**
 * @brief IsoTp-PduR Channel Configuration
 */
typedef struct {
    uint8_t channelId;                      /* IsoTp channel ID */
    PduIdType txPduId;                      /* Transmit PDU ID for PduR */
    PduIdType rxPduId;                      /* Receive PDU ID for PduR */
    uint16_t bufferSize;                    /* Buffer size for this channel */
    PduR_ModuleType upperLayerModule;       /* Upper layer module (DCM) */
} Isotp_PduRChannelConfigType;

/**
 * @brief IsoTp-PduR Integration Configuration
 */
typedef struct {
    const Isotp_PduRChannelConfigType *channelConfigs;
    uint8_t numChannels;
} Isotp_PduRConfigType;

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize IsoTp-PduR integration
 *
 * @param config Pointer to integration configuration
 * @return Isotp_ReturnType Initialization result
 */
Isotp_ReturnType Isotp_PduR_Init(const Isotp_PduRConfigType *config);

/**
 * @brief Deinitialize IsoTp-PduR integration
 *
 * @return Isotp_ReturnType Result of operation
 */
Isotp_ReturnType Isotp_PduR_DeInit(void);

/******************************************************************************
 * Transmission Functions (Upper Layer -> IsoTp -> PduR)
 ******************************************************************************/

/**
 * @brief Transmit diagnostic message via IsoTp-PduR
 *
 * @param channelId Channel ID
 * @param data Data buffer
 * @param length Data length
 * @return Isotp_ReturnType Transmission result
 */
Isotp_ReturnType Isotp_PduR_Transmit(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length
);

/**
 * @brief Transmit confirmation callback from PduR
 *
 * @param channelId Channel ID
 * @param result Transmission result
 */
void Isotp_PduR_TxConfirmation(uint8_t channelId, Std_ReturnType result);

/******************************************************************************
 * Reception Functions (PduR -> IsoTp -> Upper Layer)
 ******************************************************************************/

/**
 * @brief Process received frame from PduR
 *
 * @param channelId Channel ID
 * @param data Frame data
 * @param length Frame length
 * @return Isotp_ReturnType Processing result
 */
Isotp_ReturnType Isotp_PduR_RxIndication(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length
);

/**
 * @brief Receive indication callback for complete message
 *
 * @param channelId Channel ID
 * @param data Message data
 * @param length Message length
 */
void Isotp_PduR_MessageReceived(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length
);

/******************************************************************************
 * TP Buffer Management (PduR TP API)
 ******************************************************************************/

/**
 * @brief Start of reception notification from PduR
 *
 * @param channelId Channel ID
 * @param tpSduLength Total TP SDU length
 * @param bufferSizePtr Available buffer size
 * @return BufReq_ReturnType Buffer request result
 */
BufReq_ReturnType Isotp_PduR_StartOfReception(
    uint8_t channelId,
    PduLengthType tpSduLength,
    PduLengthType *bufferSizePtr
);

/**
 * @brief Copy received data from PduR
 *
 * @param channelId Channel ID
 * @param pduInfoPtr Pointer to PDU info with data
 * @param bufferSizePtr Remaining buffer size
 * @return BufReq_ReturnType Buffer request result
 */
BufReq_ReturnType Isotp_PduR_CopyRxData(
    uint8_t channelId,
    const PduInfoType *pduInfoPtr,
    PduLengthType *bufferSizePtr
);

/**
 * @brief Copy transmit data for PduR
 *
 * @param channelId Channel ID
 * @param pduInfoPtr Pointer to PDU info for data
 * @param retryInfoPtr Retry information
 * @param availableDataPtr Available data for transmission
 * @return BufReq_ReturnType Buffer request result
 */
BufReq_ReturnType Isotp_PduR_CopyTxData(
    uint8_t channelId,
    const PduInfoType *pduInfoPtr,
    const RetryInfoType *retryInfoPtr,
    PduLengthType *availableDataPtr
);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Check if integration is initialized
 *
 * @return bool True if initialized
 */
bool Isotp_PduR_IsInitialized(void);

/**
 * @brief Get channel configuration
 *
 * @param channelId Channel ID
 * @return const Isotp_PduRChannelConfigType* Configuration pointer
 */
const Isotp_PduRChannelConfigType* Isotp_PduR_GetChannelConfig(uint8_t channelId);

/**
 * @brief Main function - process pending operations
 */
void Isotp_PduR_MainFunction(void);

/******************************************************************************
 * Callback Registration (for DCM integration)
 ******************************************************************************/

/**
 * @brief DCM Diagnostic Message Received Callback
 */
typedef void (*Isotp_PduR_DcmMessageReceivedCallbackType)(
    uint8_t channelId,
    const uint8_t *data,
    uint16_t length
);

/**
 * @brief Register DCM message received callback
 *
 * @param callback Callback function
 * @return Isotp_ReturnType Registration result
 */
Isotp_ReturnType Isotp_PduR_RegisterDcmRxCallback(
    Isotp_PduR_DcmMessageReceivedCallbackType callback
);

#ifdef __cplusplus
}
#endif

#endif /* ISOTP_PDUR_H */

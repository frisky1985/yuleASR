/******************************************************************************
 * @file    pdur_interface.h
 * @brief   PDU Router Interface for IsoTp
 *
 * Provides routing between different transport layers (IsoTp, DoIP, etc.)
 * and the DCM (Diagnostic Communication Manager)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef PDUR_INTERFACE_H
#define PDUR_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * PduR Module Version
 ******************************************************************************/
#define PDUR_VENDOR_ID                  0x01U
#define PDUR_MODULE_ID                  0x31U
#define PDUR_SW_MAJOR_VERSION           1U
#define PDUR_SW_MINOR_VERSION           0U
#define PDUR_SW_PATCH_VERSION           0U

/******************************************************************************
 * PDU Types and Constants
 ******************************************************************************/

#define PDUR_MAX_PDUS                   32U
#define PDUR_MAX_ROUTING_PATHS          16U
#define PDUR_MAX_BUFFER_SIZE            4095U

typedef uint16_t PduR_PduIdType;

/* Protocol types */
typedef enum {
    PDUR_PROT_NONE = 0x00U,
    PDUR_PROT_ISOTP = 0x01U,             /* ISO 15765-2 */
    PDUR_PROT_DOIP = 0x02U,              /* ISO 13400-2 */
    PDUR_PROT_J1939TP = 0x03U,           /* J1939 Transport */
    PDUR_PROT_LIN = 0x04U,               /* LIN TP */
    PDUR_PROT_FR = 0x05U                 /* FlexRay TP */
} PduR_ProtocolType;

/* PDU info structure */
typedef struct {
    uint8_t *sduDataPtr;                 /* PDU data pointer */
    uint8_t *metaDataPtr;                /* Meta data pointer */
    uint16_t sduLength;                  /* PDU length */
} PduInfoType;

/******************************************************************************
 * Routing Direction
 ******************************************************************************/

typedef enum {
    PDUR_DIRECTION_TX = 0x00U,           /* Transmit direction */
    PDUR_DIRECTION_RX = 0x01U            /* Receive direction */
} PduR_DirectionType;

/******************************************************************************
 * PduR Return Types
 ******************************************************************************/

typedef enum {
    PDUR_OK = 0x00U,
    PDUR_ERROR = 0x01U,
    PDUR_BUSY = 0x02U,
    PDUR_NOT_OK = 0x03U,
    PDUR_INVALID_PARAMETER = 0x04U,
    PDUR_NO_BUFFER = 0x05U,
    PDUR_ROUTE_NOT_FOUND = 0x06U,
    PDUR_QUEUE_FULL = 0x07U
} PduR_ReturnType;

/******************************************************************************
 * Routing Path Configuration
 ******************************************************************************/

typedef struct {
    PduR_PduIdType srcPduId;             /* Source PDU ID */
    PduR_PduIdType destPduId;            /* Destination PDU ID */
    PduR_ProtocolType srcProtocol;       /* Source protocol */
    PduR_ProtocolType destProtocol;      /* Destination protocol */
    PduR_DirectionType direction;        /* Direction */
    uint16_t bufferSize;                 /* Buffer size for this route */
    bool enabled;                        /* Route enabled */
} PduR_RoutingPathType;

/******************************************************************************
 * PDU Configuration
 ******************************************************************************/

typedef struct {
    PduR_PduIdType pduId;                /* PDU identifier */
    PduR_ProtocolType protocol;          /* Associated protocol */
    uint16_t bufferSize;                 /* Buffer size */
    bool isTp;                           /* Transport protocol PDU */
} PduR_PduConfigType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

/* Transmit confirmation callback */
typedef void (*PduR_TxConfirmationCallback)(
    PduR_PduIdType pduId,
    PduR_ReturnType result
);

/* Reception indication callback */
typedef void (*PduR_RxIndicationCallback)(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo
);

/* Copy data callback for TP */
typedef PduR_ReturnType (*PduR_CopyTxDataCallback)(
    PduR_PduIdType pduId,
    PduInfoType *pduInfo,
    uint16_t *availableDataPtr
);

/* Copy RX data callback */
typedef PduR_ReturnType (*PduR_CopyRxDataCallback)(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo,
    uint16_t *bufferSizePtr
);

/* Start of reception callback */
typedef PduR_ReturnType (*PduR_StartOfReceptionCallback)(
    PduR_PduIdType pduId,
    const PduInfoType *info,
    uint16_t tpSduLength,
    uint16_t *bufferSizePtr
);

/******************************************************************************
 * Upper Layer Interface (DCM)
 ******************************************************************************/

/**
 * @brief Initialize PduR module
 * @param pduConfigs PDU configurations array
 * @param numPdus Number of PDUs
 * @param routingPaths Routing paths array
 * @param numPaths Number of routing paths
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_Init(
    const PduR_PduConfigType *pduConfigs,
    uint8_t numPdus,
    const PduR_RoutingPathType *routingPaths,
    uint8_t numPaths
);

/**
 * @brief Deinitialize PduR module
 */
void PduR_DeInit(void);

/**
 * @brief Transmit PDU
 * @param pduId PDU ID to transmit
 * @param pduInfo PDU information
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_Transmit(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo
);

/**
 * @brief Cancel transmit operation
 * @param pduId PDU ID
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_CancelTransmit(PduR_PduIdType pduId);

/******************************************************************************
 * Lower Layer Interface (IsoTp, DoIP)
 ******************************************************************************/

/**
 * @brief Lower layer transmit confirmation
 * @param pduId PDU ID
 * @param result Transmit result
 */
void PduR_LoTpTxConfirmation(PduR_PduIdType pduId, PduR_ReturnType result);

/**
 * @brief Lower layer RX indication
 * @param pduId PDU ID
 * @param pduInfo PDU information
 * @param result Receive result
 */
void PduR_LoTpRxIndication(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo,
    PduR_ReturnType result
);

/**
 * @brief Start of reception indication
 * @param pduId PDU ID
 * @param info PDU info
 * @param tpSduLength Total SDU length
 * @param bufferSizePtr Output buffer size
 * @return PDUR_OK if buffer available
 */
PduR_ReturnType PduR_LoTpStartOfReception(
    PduR_PduIdType pduId,
    const PduInfoType *info,
    uint16_t tpSduLength,
    uint16_t *bufferSizePtr
);

/**
 * @brief Copy received data
 * @param pduId PDU ID
 * @param pduInfo PDU info with buffer
 * @param bufferSizePtr Remaining buffer size
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_LoTpCopyRxData(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo,
    uint16_t *bufferSizePtr
);

/**
 * @brief Copy transmit data
 * @param pduId PDU ID
 * @param pduInfo PDU info
 * @param availableDataPtr Available data count
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_LoTpCopyTxData(
    PduR_PduIdType pduId,
    PduInfoType *pduInfo,
    uint16_t *availableDataPtr
);

/******************************************************************************
 * Upper Layer Interface (DCM)
 ******************************************************************************/

/**
 * @brief Upper layer transmit confirmation
 * @param pduId PDU ID
 * @param result Transmit result
 */
void PduR_UpTxConfirmation(PduR_PduIdType pduId, PduR_ReturnType result);

/**
 * @brief Upper layer RX indication
 * @param pduId PDU ID
 * @param pduInfo PDU information
 */
void PduR_UpRxIndication(PduR_PduIdType pduId, const PduInfoType *pduInfo);

/******************************************************************************
 * Routing Functions
 ******************************************************************************/

/**
 * @brief Route PDU from source to destination
 * @param srcPduId Source PDU ID
 * @param pduInfo PDU information
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_RoutePdu(
    PduR_PduIdType srcPduId,
    const PduInfoType *pduInfo
);

/**
 * @brief Get routing path for PDU
 * @param pduId PDU ID
 * @param direction Direction
 * @return Routing path pointer or NULL
 */
const PduR_RoutingPathType* PduR_GetRoutingPath(
    PduR_PduIdType pduId,
    PduR_DirectionType direction
);

/**
 * @brief Enable routing path
 * @param srcPduId Source PDU ID
 * @param direction Direction
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_EnableRouting(
    PduR_PduIdType srcPduId,
    PduR_DirectionType direction
);

/**
 * @brief Disable routing path
 * @param srcPduId Source PDU ID
 * @param direction Direction
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_DisableRouting(
    PduR_PduIdType srcPduId,
    PduR_DirectionType direction
);

/******************************************************************************
 * IsoTp Specific Interface
 ******************************************************************************/

/**
 * @brief IsoTp transmit wrapper
 * @param isoTpPduId IsoTp PDU ID
 * @param data Data buffer
 * @param length Data length
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_IsotpTransmit(
    PduR_PduIdType isoTpPduId,
    const uint8_t *data,
    uint16_t length
);

/**
 * @brief IsoTp reception indication wrapper
 * @param isoTpPduId IsoTp PDU ID
 * @param data Data buffer
 * @param length Data length
 */
void PduR_IsotpRxIndication(
    PduR_PduIdType isoTpPduId,
    const uint8_t *data,
    uint16_t length
);

/******************************************************************************
 * DoIP Specific Interface
 ******************************************************************************/

/**
 * @brief DoIP transmit wrapper
 * @param doIpPduId DoIP PDU ID
 * @param data Data buffer
 * @param length Data length
 * @return PDUR_OK on success
 */
PduR_ReturnType PduR_DoIpTransmit(
    PduR_PduIdType doIpPduId,
    const uint8_t *data,
    uint16_t length
);

/**
 * @brief DoIP reception indication wrapper
 * @param doIpPduId DoIP PDU ID
 * @param data Data buffer
 * @param length Data length
 */
void PduR_DoIpRxIndication(
    PduR_PduIdType doIpPduId,
    const uint8_t *data,
    uint16_t length
);

/******************************************************************************
 * Buffer Management
 ******************************************************************************/

/**
 * @brief Allocate buffer for PDU
 * @param pduId PDU ID
 * @param size Required size
 * @return Buffer pointer or NULL
 */
uint8_t* PduR_AllocateBuffer(PduR_PduIdType pduId, uint16_t size);

/**
 * @brief Free PDU buffer
 * @param pduId PDU ID
 */
void PduR_FreeBuffer(PduR_PduIdType pduId);

/**
 * @brief Get buffer for PDU
 * @param pduId PDU ID
 * @return Buffer pointer or NULL
 */
uint8_t* PduR_GetBuffer(PduR_PduIdType pduId);

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main processing function
 */
void PduR_MainFunction(void);

/******************************************************************************
 * Diagnostic Functions
 ******************************************************************************/

/**
 * @brief Get routing statistics
 * @param routedCount Output routed PDU count
 * @param droppedCount Output dropped PDU count
 */
void PduR_GetStats(uint32_t *routedCount, uint32_t *droppedCount);

/**
 * @brief Reset routing statistics
 */
void PduR_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* PDUR_INTERFACE_H */

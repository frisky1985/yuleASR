/******************************************************************************
 * @file    pdur_interface.c
 * @brief   PDU Router Interface Implementation
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "pdur_interface.h"
#include "isotp_core.h"
#include <string.h>
#include <stddef.h>

/******************************************************************************
 * Module Configuration
 ******************************************************************************/

#define PDUR_MAX_BUFFERS                PDUR_MAX_PDUS

/******************************************************************************
 * Module Types
 ******************************************************************************/

typedef struct {
    uint8_t *data;
    uint16_t size;
    uint16_t used;
    bool allocated;
    PduR_PduIdType pduId;
} PduR_BufferType;

typedef struct {
    bool initialized;
    const PduR_PduConfigType *pduConfigs;
    uint8_t numPdus;
    const PduR_RoutingPathType *routingPaths;
    uint8_t numPaths;
    
    /* Buffers */
    PduR_BufferType buffers[PDUR_MAX_BUFFERS];
    uint8_t bufferMemory[PDUR_MAX_PDUS][PDUR_MAX_BUFFER_SIZE];
    
    /* Statistics */
    uint32_t routedCount;
    uint32_t droppedCount;
    
    /* Callbacks */
    PduR_TxConfirmationCallback txConfirmationCallback;
    PduR_RxIndicationCallback rxIndicationCallback;
} PduR_ContextType;

/******************************************************************************
 * Module Global Variables
 ******************************************************************************/

static PduR_ContextType g_pdurContext;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Validate PDU ID
 */
static bool PduR_IsValidPduId(PduR_PduIdType pduId)
{
    return (pduId < g_pdurContext.numPdus) && g_pdurContext.initialized;
}

/**
 * @brief Find PDU configuration
 */
static const PduR_PduConfigType* PduR_FindPduConfig(PduR_PduIdType pduId)
{
    if (!PduR_IsValidPduId(pduId)) {
        return NULL;
    }
    return &g_pdurContext.pduConfigs[pduId];
}

/**
 * @brief Find routing path
 */
static const PduR_RoutingPathType* PduR_FindRoutingPath(
    PduR_PduIdType srcPduId,
    PduR_DirectionType direction
)
{
    for (uint8_t i = 0U; i < g_pdurContext.numPaths; i++) {
        const PduR_RoutingPathType *path = &g_pdurContext.routingPaths[i];
        if (path->enabled && path->direction == direction) {
            if ((direction == PDUR_DIRECTION_TX && path->srcPduId == srcPduId) ||
                (direction == PDUR_DIRECTION_RX && path->destPduId == srcPduId)) {
                return path;
            }
        }
    }
    return NULL;
}

/**
 * @brief Find buffer by PDU ID
 */
static PduR_BufferType* PduR_FindBuffer(PduR_PduIdType pduId)
{
    for (uint8_t i = 0U; i < PDUR_MAX_BUFFERS; i++) {
        if (g_pdurContext.buffers[i].allocated && 
            g_pdurContext.buffers[i].pduId == pduId) {
            return &g_pdurContext.buffers[i];
        }
    }
    return NULL;
}

/**
 * @brief Find free buffer
 */
static PduR_BufferType* PduR_FindFreeBuffer(void)
{
    for (uint8_t i = 0U; i < PDUR_MAX_BUFFERS; i++) {
        if (!g_pdurContext.buffers[i].allocated) {
            return &g_pdurContext.buffers[i];
        }
    }
    return NULL;
}

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

PduR_ReturnType PduR_Init(
    const PduR_PduConfigType *pduConfigs,
    uint8_t numPdus,
    const PduR_RoutingPathType *routingPaths,
    uint8_t numPaths
)
{
    if (pduConfigs == NULL || numPdus == 0U || numPdus > PDUR_MAX_PDUS ||
        (routingPaths == NULL && numPaths > 0U) || numPaths > PDUR_MAX_ROUTING_PATHS) {
        return PDUR_INVALID_PARAMETER;
    }
    
    memset(&g_pdurContext, 0, sizeof(g_pdurContext));
    
    g_pdurContext.pduConfigs = pduConfigs;
    g_pdurContext.numPdus = numPdus;
    g_pdurContext.routingPaths = routingPaths;
    g_pdurContext.numPaths = numPaths;
    
    /* Initialize buffers */
    for (uint8_t i = 0U; i < PDUR_MAX_BUFFERS; i++) {
        g_pdurContext.buffers[i].data = g_pdurContext.bufferMemory[i];
        g_pdurContext.buffers[i].size = PDUR_MAX_BUFFER_SIZE;
        g_pdurContext.buffers[i].used = 0U;
        g_pdurContext.buffers[i].allocated = false;
        g_pdurContext.buffers[i].pduId = 0xFFFFU;
    }
    
    g_pdurContext.initialized = true;
    
    return PDUR_OK;
}

void PduR_DeInit(void)
{
    memset(&g_pdurContext, 0, sizeof(g_pdurContext));
}

/******************************************************************************
 * Transmission Functions
 ******************************************************************************/

PduR_ReturnType PduR_Transmit(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo
)
{
    if (!PduR_IsValidPduId(pduId) || pduInfo == NULL) {
        return PDUR_INVALID_PARAMETER;
    }
    
    const PduR_RoutingPathType *path = PduR_FindRoutingPath(pduId, PDUR_DIRECTION_TX);
    if (path == NULL) {
        g_pdurContext.droppedCount++;
        return PDUR_ROUTE_NOT_FOUND;
    }
    
    /* Route to destination */
    PduR_ReturnType result = PDUR_OK;
    
    switch (path->destProtocol) {
        case PDUR_PROT_ISOTP: {
            /* Forward to IsoTp */
            extern PduR_ReturnType PduR_IsotpTransmit(PduR_PduIdType, const uint8_t*, uint16_t);
            result = PduR_IsotpTransmit(path->destPduId, pduInfo->sduDataPtr, pduInfo->sduLength);
            break;
        }
        
        case PDUR_PROT_DOIP: {
            /* Forward to DoIP */
            extern PduR_ReturnType PduR_DoIpTransmit(PduR_PduIdType, const uint8_t*, uint16_t);
            result = PduR_DoIpTransmit(path->destPduId, pduInfo->sduDataPtr, pduInfo->sduLength);
            break;
        }
        
        default:
            result = PDUR_ERROR;
            break;
    }
    
    if (result == PDUR_OK) {
        g_pdurContext.routedCount++;
    } else {
        g_pdurContext.droppedCount++;
    }
    
    return result;
}

PduR_ReturnType PduR_CancelTransmit(PduR_PduIdType pduId)
{
    if (!PduR_IsValidPduId(pduId)) {
        return PDUR_INVALID_PARAMETER;
    }
    
    /* Cancel operation depends on underlying protocol */
    /* For now, just return OK */
    (void)pduId;
    
    return PDUR_OK;
}

/******************************************************************************
 * Lower Layer Interface
 ******************************************************************************/

void PduR_LoTpTxConfirmation(PduR_PduIdType pduId, PduR_ReturnType result)
{
    /* Forward to upper layer */
    PduR_UpTxConfirmation(pduId, result);
}

void PduR_LoTpRxIndication(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo,
    PduR_ReturnType result
)
{
    if (result != PDUR_OK) {
        g_pdurContext.droppedCount++;
        return;
    }
    
    /* Find routing path */
    const PduR_RoutingPathType *path = PduR_FindRoutingPath(pduId, PDUR_DIRECTION_RX);
    if (path == NULL) {
        g_pdurContext.droppedCount++;
        return;
    }
    
    /* Forward to upper layer */
    PduR_UpRxIndication(path->destPduId, pduInfo);
    g_pdurContext.routedCount++;
}

PduR_ReturnType PduR_LoTpStartOfReception(
    PduR_PduIdType pduId,
    const PduInfoType *info,
    uint16_t tpSduLength,
    uint16_t *bufferSizePtr
)
{
    (void)info;
    
    if (bufferSizePtr == NULL) {
        return PDUR_INVALID_PARAMETER;
    }
    
    const PduR_PduConfigType *config = PduR_FindPduConfig(pduId);
    if (config == NULL) {
        return PDUR_INVALID_PARAMETER;
    }
    
    if (tpSduLength > config->bufferSize) {
        *bufferSizePtr = 0U;
        return PDUR_NO_BUFFER;
    }
    
    /* Allocate buffer */
    uint8_t *buffer = PduR_AllocateBuffer(pduId, tpSduLength);
    if (buffer == NULL) {
        *bufferSizePtr = 0U;
        return PDUR_NO_BUFFER;
    }
    
    *bufferSizePtr = config->bufferSize;
    return PDUR_OK;
}

PduR_ReturnType PduR_LoTpCopyRxData(
    PduR_PduIdType pduId,
    const PduInfoType *pduInfo,
    uint16_t *bufferSizePtr
)
{
    if (pduInfo == NULL || bufferSizePtr == NULL) {
        return PDUR_INVALID_PARAMETER;
    }
    
    PduR_BufferType *buffer = PduR_FindBuffer(pduId);
    if (buffer == NULL) {
        return PDUR_NO_BUFFER;
    }
    
    uint16_t remaining = buffer->size - buffer->used;
    uint16_t toCopy = (pduInfo->sduLength < remaining) ? pduInfo->sduLength : remaining;
    
    memcpy(&buffer->data[buffer->used], pduInfo->sduDataPtr, toCopy);
    buffer->used += toCopy;
    
    *bufferSizePtr = buffer->size - buffer->used;
    
    return PDUR_OK;
}

PduR_ReturnType PduR_LoTpCopyTxData(
    PduR_PduIdType pduId,
    PduInfoType *pduInfo,
    uint16_t *availableDataPtr
)
{
    if (pduInfo == NULL) {
        return PDUR_INVALID_PARAMETER;
    }
    
    PduR_BufferType *buffer = PduR_FindBuffer(pduId);
    if (buffer == NULL) {
        return PDUR_NO_BUFFER;
    }
    
    uint16_t remaining = buffer->used - buffer->used;  /* Simplified */
    uint16_t toCopy = (pduInfo->sduLength < remaining) ? pduInfo->sduLength : remaining;
    
    memcpy(pduInfo->sduDataPtr, buffer->data, toCopy);
    pduInfo->sduLength = toCopy;
    
    if (availableDataPtr != NULL) {
        *availableDataPtr = remaining - toCopy;
    }
    
    return PDUR_OK;
}

/******************************************************************************
 * Upper Layer Interface
 ******************************************************************************/

void PduR_UpTxConfirmation(PduR_PduIdType pduId, PduR_ReturnType result)
{
    /* Forward to registered callback or DCM */
    if (g_pdurContext.txConfirmationCallback != NULL) {
        g_pdurContext.txConfirmationCallback(pduId, result);
    }
    
    /* Also forward to DCM if configured */
    /* DCM_TxConfirmation(pduId, result); */
    (void)pduId;
    (void)result;
}

void PduR_UpRxIndication(PduR_PduIdType pduId, const PduInfoType *pduInfo)
{
    /* Forward to registered callback or DCM */
    if (g_pdurContext.rxIndicationCallback != NULL) {
        g_pdurContext.rxIndicationCallback(pduId, pduInfo);
    }
    
    /* Also forward to DCM if configured */
    /* DCM_RxIndication(pduId, pduInfo); */
    (void)pduId;
    (void)pduInfo;
}

/******************************************************************************
 * Routing Functions
 ******************************************************************************/

PduR_ReturnType PduR_RoutePdu(
    PduR_PduIdType srcPduId,
    const PduInfoType *pduInfo
)
{
    if (!PduR_IsValidPduId(srcPduId) || pduInfo == NULL) {
        return PDUR_INVALID_PARAMETER;
    }
    
    const PduR_RoutingPathType *path = PduR_FindRoutingPath(srcPduId, PDUR_DIRECTION_TX);
    if (path == NULL) {
        return PDUR_ROUTE_NOT_FOUND;
    }
    
    /* Route based on destination protocol */
    return PduR_Transmit(srcPduId, pduInfo);
}

const PduR_RoutingPathType* PduR_GetRoutingPath(
    PduR_PduIdType pduId,
    PduR_DirectionType direction
)
{
    return PduR_FindRoutingPath(pduId, direction);
}

PduR_ReturnType PduR_EnableRouting(
    PduR_PduIdType srcPduId,
    PduR_DirectionType direction
)
{
    for (uint8_t i = 0U; i < g_pdurContext.numPaths; i++) {
        PduR_RoutingPathType *path = (PduR_RoutingPathType *)&g_pdurContext.routingPaths[i];
        if (path->srcPduId == srcPduId && path->direction == direction) {
            path->enabled = true;
            return PDUR_OK;
        }
    }
    return PDUR_ROUTE_NOT_FOUND;
}

PduR_ReturnType PduR_DisableRouting(
    PduR_PduIdType srcPduId,
    PduR_DirectionType direction
)
{
    for (uint8_t i = 0U; i < g_pdurContext.numPaths; i++) {
        PduR_RoutingPathType *path = (PduR_RoutingPathType *)&g_pdurContext.routingPaths[i];
        if (path->srcPduId == srcPduId && path->direction == direction) {
            path->enabled = false;
            return PDUR_OK;
        }
    }
    return PDUR_ROUTE_NOT_FOUND;
}

/******************************************************************************
 * IsoTp Specific Interface
 ******************************************************************************/

PduR_ReturnType PduR_IsotpTransmit(
    PduR_PduIdType isoTpPduId,
    const uint8_t *data,
    uint16_t length
)
{
    /* Call IsoTp transmit */
    extern Isotp_ReturnType IsoTp_Transmit(uint8_t, const uint8_t*, uint16_t, void*);
    Isotp_ReturnType result = IsoTp_Transmit((uint8_t)isoTpPduId, data, length, NULL);
    
    return (result == 0U) ? PDUR_OK : PDUR_ERROR;
}

void PduR_IsotpRxIndication(
    PduR_PduIdType isoTpPduId,
    const uint8_t *data,
    uint16_t length
)
{
    PduInfoType pduInfo;
    pduInfo.sduDataPtr = (uint8_t *)data;
    pduInfo.sduLength = length;
    pduInfo.metaDataPtr = NULL;
    
    PduR_LoTpRxIndication(isoTpPduId, &pduInfo, PDUR_OK);
}

/******************************************************************************
 * DoIP Specific Interface
 ******************************************************************************/

PduR_ReturnType PduR_DoIpTransmit(
    PduR_PduIdType doIpPduId,
    const uint8_t *data,
    uint16_t length
)
{
    /* Call DoIP transmit */
    /* extern DoIp_ReturnType DoIp_Transmit(uint16_t, const uint8_t*, uint16_t); */
    /* return DoIp_Transmit(doIpPduId, data, length); */
    
    (void)doIpPduId;
    (void)data;
    (void)length;
    
    return PDUR_OK;
}

void PduR_DoIpRxIndication(
    PduR_PduIdType doIpPduId,
    const uint8_t *data,
    uint16_t length
)
{
    PduInfoType pduInfo;
    pduInfo.sduDataPtr = (uint8_t *)data;
    pduInfo.sduLength = length;
    pduInfo.metaDataPtr = NULL;
    
    PduR_LoTpRxIndication(doIpPduId, &pduInfo, PDUR_OK);
}

/******************************************************************************
 * Buffer Management
 ******************************************************************************/

uint8_t* PduR_AllocateBuffer(PduR_PduIdType pduId, uint16_t size)
{
    if (size > PDUR_MAX_BUFFER_SIZE) {
        return NULL;
    }
    
    PduR_BufferType *buffer = PduR_FindBuffer(pduId);
    if (buffer != NULL) {
        /* Buffer already allocated for this PDU */
        if (size <= buffer->size) {
            buffer->used = 0U;
            return buffer->data;
        }
        return NULL;
    }
    
    buffer = PduR_FindFreeBuffer();
    if (buffer == NULL) {
        return NULL;
    }
    
    buffer->allocated = true;
    buffer->pduId = pduId;
    buffer->used = 0U;
    buffer->size = PDUR_MAX_BUFFER_SIZE;
    
    return buffer->data;
}

void PduR_FreeBuffer(PduR_PduIdType pduId)
{
    PduR_BufferType *buffer = PduR_FindBuffer(pduId);
    if (buffer != NULL) {
        buffer->allocated = false;
        buffer->pduId = 0xFFFFU;
        buffer->used = 0U;
    }
}

uint8_t* PduR_GetBuffer(PduR_PduIdType pduId)
{
    PduR_BufferType *buffer = PduR_FindBuffer(pduId);
    if (buffer != NULL) {
        return buffer->data;
    }
    return NULL;
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

void PduR_MainFunction(void)
{
    if (!g_pdurContext.initialized) {
        return;
    }
    
    /* Process any pending routing operations */
}

/******************************************************************************
 * Diagnostic Functions
 ******************************************************************************/

void PduR_GetStats(uint32_t *routedCount, uint32_t *droppedCount)
{
    if (routedCount != NULL) {
        *routedCount = g_pdurContext.routedCount;
    }
    if (droppedCount != NULL) {
        *droppedCount = g_pdurContext.droppedCount;
    }
}

void PduR_ResetStats(void)
{
    g_pdurContext.routedCount = 0U;
    g_pdurContext.droppedCount = 0U;
}

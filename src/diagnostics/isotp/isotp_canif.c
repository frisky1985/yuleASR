/******************************************************************************
 * @file    isotp_canif.c
 * @brief   IsoTp CAN Interface Adapter Implementation
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "isotp_canif.h"
#include <string.h>
#include <stddef.h>

/******************************************************************************
 * Module Configuration
 ******************************************************************************/

#define CANIF_MAX_CONTROLLERS           4U
#define CANIF_MAX_FILTERS               16U
#define CANIF_MAX_CALLBACKS             32U
#define CANIF_MAX_ISOTP_CHANNELS        16U
#define CANIF_TX_QUEUE_SIZE             16U

/******************************************************************************
 * Module Global Variables
 ******************************************************************************/

/* Module context */
typedef struct {
    bool initialized;
    const CanIf_ControllerConfigType *controllers;
    uint8_t numControllers;
    const CanIf_HwDriverType *hwDriver;
    
    /* Callbacks */
    CanIf_TxConfirmationCallback txConfirmationCallback;
    CanIf_ControllerModeCallback controllerModeCallback;
    CanIf_ErrorCallback errorCallback;
    
    /* Controller states */
    uint8_t controllerModes[CANIF_MAX_CONTROLLERS];
} CanIf_ContextType;

static CanIf_ContextType g_canifContext;

/* IsoTp channel mapping */
typedef struct {
    uint8_t channelId;
    uint8_t controllerId;
    uint32_t txCanId;
    uint32_t rxCanId;
    bool isFd;
    bool mapped;
} CanIf_IsotpChannelMapType;

static CanIf_IsotpChannelMapType g_isotpChannelMap[CANIF_MAX_ISOTP_CHANNELS];

/* Rx callbacks registry */
typedef struct {
    uint32_t canId;
    CanIf_RxCallback callback;
    void *userData;
    bool active;
} CanIf_RxCallbackEntryType;

static CanIf_RxCallbackEntryType g_rxCallbacks[CANIF_MAX_CALLBACKS];

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Validate controller ID
 */
static bool CanIf_IsValidController(uint8_t controllerId)
{
    return (controllerId < g_canifContext.numControllers) &&
           (g_canifContext.initialized);
}

/**
 * @brief Find callback entry by CAN ID
 */
static CanIf_RxCallbackEntryType* CanIf_FindRxCallback(uint32_t canId)
{
    for (uint8_t i = 0U; i < CANIF_MAX_CALLBACKS; i++) {
        if (g_rxCallbacks[i].active && g_rxCallbacks[i].canId == canId) {
            return &g_rxCallbacks[i];
        }
    }
    return NULL;
}

/**
 * @brief Find free callback entry
 */
static CanIf_RxCallbackEntryType* CanIf_FindFreeRxCallback(void)
{
    for (uint8_t i = 0U; i < CANIF_MAX_CALLBACKS; i++) {
        if (!g_rxCallbacks[i].active) {
            return &g_rxCallbacks[i];
        }
    }
    return NULL;
}

/**
 * @brief Find IsoTp channel mapping
 */
static CanIf_IsotpChannelMapType* CanIf_FindIsotpChannelMap(uint8_t channelId)
{
    if (channelId >= CANIF_MAX_ISOTP_CHANNELS) {
        return NULL;
    }
    return &g_isotpChannelMap[channelId];
}

/**
 * @brief Find IsoTp channel map by CAN ID
 */
static CanIf_IsotpChannelMapType* CanIf_FindIsotpChannelByCanId(
    uint32_t canId,
    bool isRx
)
{
    for (uint8_t i = 0U; i < CANIF_MAX_ISOTP_CHANNELS; i++) {
        if (g_isotpChannelMap[i].mapped) {
            uint32_t targetId = isRx ? g_isotpChannelMap[i].rxCanId : 
                                        g_isotpChannelMap[i].txCanId;
            if (targetId == canId) {
                return &g_isotpChannelMap[i];
            }
        }
    }
    return NULL;
}

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

CanIf_ReturnType CanIf_Init(
    const CanIf_ControllerConfigType *controllers,
    uint8_t numControllers,
    const CanIf_HwDriverType *hwDriver
)
{
    if (controllers == NULL || numControllers == 0U || 
        numControllers > CANIF_MAX_CONTROLLERS || hwDriver == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    /* Clear context */
    memset(&g_canifContext, 0, sizeof(g_canifContext));
    memset(g_isotpChannelMap, 0, sizeof(g_isotpChannelMap));
    memset(g_rxCallbacks, 0, sizeof(g_rxCallbacks));
    
    g_canifContext.controllers = controllers;
    g_canifContext.numControllers = numControllers;
    g_canifContext.hwDriver = hwDriver;
    
    /* Initialize controllers */
    for (uint8_t i = 0U; i < numControllers; i++) {
        g_canifContext.controllerModes[i] = 0U;  /* Stopped */
        
        if (hwDriver->init != NULL) {
            hwDriver->init(i, &controllers[i]);
        }
    }
    
    g_canifContext.initialized = true;
    
    return CANIF_OK;
}

void CanIf_DeInit(void)
{
    if (!g_canifContext.initialized) {
        return;
    }
    
    /* Deinitialize controllers */
    for (uint8_t i = 0U; i < g_canifContext.numControllers; i++) {
        if (g_canifContext.hwDriver->deinit != NULL) {
            g_canifContext.hwDriver->deinit(i);
        }
    }
    
    memset(&g_canifContext, 0, sizeof(g_canifContext));
    memset(g_isotpChannelMap, 0, sizeof(g_isotpChannelMap));
    memset(g_rxCallbacks, 0, sizeof(g_rxCallbacks));
}

/******************************************************************************
 * Controller Management
 ******************************************************************************/

CanIf_ReturnType CanIf_SetControllerMode(uint8_t controllerId, uint8_t mode)
{
    if (!CanIf_IsValidController(controllerId)) {
        return CANIF_INVALID_PARAMETER;
    }
    
    CanIf_ReturnType result = CANIF_OK;
    
    switch (mode) {
        case 0U:  /* Stop */
            if (g_canifContext.hwDriver->stop != NULL) {
                result = g_canifContext.hwDriver->stop(controllerId);
            }
            break;
            
        case 1U:  /* Start */
            if (g_canifContext.hwDriver->start != NULL) {
                result = g_canifContext.hwDriver->start(controllerId);
            }
            break;
            
        default:
            return CANIF_INVALID_PARAMETER;
    }
    
    if (result == CANIF_OK) {
        g_canifContext.controllerModes[controllerId] = mode;
        
        if (g_canifContext.controllerModeCallback != NULL) {
            g_canifContext.controllerModeCallback(controllerId, mode, NULL);
        }
    }
    
    return result;
}

CanIf_ReturnType CanIf_GetControllerMode(uint8_t controllerId, uint8_t *mode)
{
    if (!CanIf_IsValidController(controllerId) || mode == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    *mode = g_canifContext.controllerModes[controllerId];
    return CANIF_OK;
}

/******************************************************************************
 * Transmission Functions
 ******************************************************************************/

CanIf_ReturnType CanIf_Transmit(
    uint8_t controllerId,
    const CanIf_FrameType *frame
)
{
    if (!CanIf_IsValidController(controllerId) || frame == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    /* Validate CAN ID */
    CanIf_IdType idType = (frame->canId > 0x7FFU) ? CAN_ID_TYPE_EXTENDED : CAN_ID_TYPE_STANDARD;
    if (!CanIf_IsValidCanId(frame->canId, idType)) {
        return CANIF_INVALID_PARAMETER;
    }
    
    /* Check controller is started */
    if (g_canifContext.controllerModes[controllerId] != 1U) {
        return CANIF_ERROR;
    }
    
    /* Check FD compatibility */
    const CanIf_ControllerConfigType *config = &g_canifContext.controllers[controllerId];
    if (frame->isFd && !config->fdEnabled) {
        return CANIF_ERROR;
    }
    
    /* Validate DLC */
    uint8_t maxDlc = frame->isFd ? 15U : 8U;
    if (frame->dlc > maxDlc) {
        return CANIF_INVALID_PARAMETER;
    }
    
    if (g_canifContext.hwDriver->transmit != NULL) {
        return g_canifContext.hwDriver->transmit(controllerId, frame);
    }
    
    return CANIF_ERROR;
}

CanIf_ReturnType CanIf_TransmitBlocking(
    uint8_t controllerId,
    const CanIf_FrameType *frame,
    uint32_t timeoutMs
)
{
    /* For blocking transmit, we would typically use a semaphore/flag */
    /* This is a simplified implementation */
    
    CanIf_ReturnType result = CanIf_Transmit(controllerId, frame);
    if (result != CANIF_OK) {
        return result;
    }
    
    /* In a real implementation, wait for TX confirmation */
    /* For now, assume success if TX was accepted */
    
    (void)timeoutMs;  /* Unused in this simplified implementation */
    
    return CANIF_OK;
}

/******************************************************************************
 * Reception Functions
 ******************************************************************************/

CanIf_ReturnType CanIf_ProcessRxFrame(
    uint8_t controllerId,
    const CanIf_FrameType *frame
)
{
    if (!CanIf_IsValidController(controllerId) || frame == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    /* Try IsoTp channel mapping first */
    CanIf_IsotpChannelMapType *isotpMap = CanIf_FindIsotpChannelByCanId(frame->canId, true);
    if (isotpMap != NULL && isotpMap->mapped) {
        /* Forward to IsoTp for processing */
        extern Isotp_ReturnType IsoTp_ProcessRxFrame(uint8_t, const uint8_t*, uint8_t);
        IsoTp_ProcessRxFrame(isotpMap->channelId, frame->data, frame->dlc);
        return CANIF_OK;
    }
    
    /* Try registered callbacks */
    CanIf_RxCallbackEntryType *callback = CanIf_FindRxCallback(frame->canId);
    if (callback != NULL && callback->callback != NULL) {
        callback->callback(controllerId, frame, callback->userData);
        return CANIF_OK;
    }
    
    /* No handler found - frame ignored */
    return CANIF_OK;
}

CanIf_ReturnType CanIf_RegisterRxCallback(
    uint32_t canId,
    CanIf_RxCallback callback,
    void *userData
)
{
    if (callback == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    CanIf_RxCallbackEntryType *entry = CanIf_FindRxCallback(canId);
    if (entry != NULL) {
        /* Update existing entry */
        entry->callback = callback;
        entry->userData = userData;
        return CANIF_OK;
    }
    
    /* Find free entry */
    entry = CanIf_FindFreeRxCallback();
    if (entry == NULL) {
        return CANIF_NO_BUFFER;
    }
    
    entry->canId = canId;
    entry->callback = callback;
    entry->userData = userData;
    entry->active = true;
    
    return CANIF_OK;
}

CanIf_ReturnType CanIf_UnregisterRxCallback(uint32_t canId)
{
    CanIf_RxCallbackEntryType *entry = CanIf_FindRxCallback(canId);
    if (entry == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    entry->active = false;
    entry->callback = NULL;
    entry->userData = NULL;
    
    return CANIF_OK;
}

/******************************************************************************
 * Filter Functions
 ******************************************************************************/

CanIf_ReturnType CanIf_AddFilter(
    uint8_t controllerId,
    const CanIf_FilterType *filter,
    uint8_t *filterIndex
)
{
    if (!CanIf_IsValidController(controllerId) || filter == NULL) {
        return CANIF_INVALID_PARAMETER;
    }
    
    if (g_canifContext.hwDriver->setFilter != NULL) {
        return g_canifContext.hwDriver->setFilter(controllerId, filter);
    }
    
    return CANIF_ERROR;
}

CanIf_ReturnType CanIf_RemoveFilter(uint8_t controllerId, uint8_t filterIndex)
{
    if (!CanIf_IsValidController(controllerId)) {
        return CANIF_INVALID_PARAMETER;
    }
    
    if (g_canifContext.hwDriver->clearFilter != NULL) {
        return g_canifContext.hwDriver->clearFilter(controllerId, filterIndex);
    }
    
    return CANIF_ERROR;
}

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

void CanIf_RegisterTxConfirmationCallback(CanIf_TxConfirmationCallback callback)
{
    g_canifContext.txConfirmationCallback = callback;
}

void CanIf_RegisterControllerModeCallback(CanIf_ControllerModeCallback callback)
{
    g_canifContext.controllerModeCallback = callback;
}

void CanIf_RegisterErrorCallback(CanIf_ErrorCallback callback)
{
    g_canifContext.errorCallback = callback;
}

/******************************************************************************
 * IsoTp Integration Functions
 ******************************************************************************/

Isotp_ReturnType CanIf_IsotpTransmitWrapper(
    uint32_t canId,
    uint8_t *data,
    uint8_t length,
    bool isFd,
    void *userData
)
{
    (void)userData;  /* Unused */
    
    /* Find IsoTp channel mapping to get controller ID */
    CanIf_IsotpChannelMapType *map = CanIf_FindIsotpChannelByCanId(canId, false);
    if (map == NULL || !map->mapped) {
        return ISOTP_E_NOT_OK;
    }
    
    /* Build CAN frame */
    CanIf_FrameType frame;
    memset(&frame, 0, sizeof(frame));
    
    frame.canId = canId;
    frame.idType = (canId > 0x7FFU) ? CAN_ID_TYPE_EXTENDED : CAN_ID_TYPE_STANDARD;
    frame.dlc = CanIf_LengthToDlc(length);
    frame.isFd = isFd;
    frame.isBrs = isFd;  /* Enable BRS for FD frames */
    
    memcpy(frame.data, data, length);
    
    /* Pad remaining bytes */
    uint8_t actualLength = CanIf_DlcToLength(frame.dlc);
    for (uint8_t i = length; i < actualLength; i++) {
        frame.data[i] = 0xAAU;  /* Padding pattern */
    }
    
    CanIf_ReturnType result = CanIf_Transmit(map->controllerId, &frame);
    
    return (result == CANIF_OK) ? ISOTP_E_OK : ISOTP_E_NOT_OK;
}

CanIf_ReturnType CanIf_MapIsotpChannel(
    uint8_t channelId,
    uint8_t controllerId,
    uint32_t txCanId,
    uint32_t rxCanId
)
{
    if (channelId >= CANIF_MAX_ISOTP_CHANNELS || !CanIf_IsValidController(controllerId)) {
        return CANIF_INVALID_PARAMETER;
    }
    
    CanIf_IsotpChannelMapType *map = &g_isotpChannelMap[channelId];
    map->channelId = channelId;
    map->controllerId = controllerId;
    map->txCanId = txCanId;
    map->rxCanId = rxCanId;
    map->mapped = true;
    
    return CANIF_OK;
}

CanIf_ReturnType CanIf_InitIsotpChannel(uint8_t channelId, bool isFd)
{
    if (channelId >= CANIF_MAX_ISOTP_CHANNELS) {
        return CANIF_INVALID_PARAMETER;
    }
    
    CanIf_IsotpChannelMapType *map = &g_isotpChannelMap[channelId];
    if (!map->mapped) {
        return CANIF_ERROR;
    }
    
    map->isFd = isFd;
    
    /* Add hardware filter for RX CAN ID */
    CanIf_FilterType filter;
    filter.canId = map->rxCanId;
    filter.mask = (map->rxCanId > 0x7FFU) ? 0x1FFFFFFFU : 0x7FFU;
    filter.idType = (map->rxCanId > 0x7FFU) ? CAN_ID_TYPE_EXTENDED : CAN_ID_TYPE_STANDARD;
    filter.controllerId = map->controllerId;
    
    uint8_t filterIndex;
    return CanIf_AddFilter(map->controllerId, &filter, &filterIndex);
}

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

uint8_t CanIf_DlcToLength(uint8_t dlc)
{
    /* CAN FD DLC to length mapping */
    static const uint8_t dlcTable[] = {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U,  /* 0-8: same */
        12U, 16U, 20U, 24U, 32U, 48U, 64U     /* 9-15: mapped */
    };
    
    if (dlc > 15U) {
        return 0U;
    }
    
    return dlcTable[dlc];
}

uint8_t CanIf_LengthToDlc(uint8_t length)
{
    /* Length to CAN FD DLC mapping */
    if (length <= 8U) {
        return length;
    } else if (length <= 12U) {
        return 9U;
    } else if (length <= 16U) {
        return 10U;
    } else if (length <= 20U) {
        return 11U;
    } else if (length <= 24U) {
        return 12U;
    } else if (length <= 32U) {
        return 13U;
    } else if (length <= 48U) {
        return 14U;
    } else if (length <= 64U) {
        return 15U;
    }
    
    return 15U;  /* Max DLC */
}

bool CanIf_IsValidCanId(uint32_t canId, CanIf_IdType idType)
{
    if (idType == CAN_ID_TYPE_STANDARD) {
        return (canId <= 0x7FFU);
    } else {
        return (canId <= 0x1FFFFFFFU);
    }
}

const CanIf_ControllerConfigType* CanIf_GetControllerConfig(uint8_t controllerId)
{
    if (!CanIf_IsValidController(controllerId)) {
        return NULL;
    }
    return &g_canifContext.controllers[controllerId];
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

void CanIf_MainFunction(void)
{
    if (!g_canifContext.initialized) {
        return;
    }
    
    /* Process any pending operations */
    /* In a full implementation, this would handle:
     * - TX queue management
     * - Timeout handling
     * - Error recovery
     */
}

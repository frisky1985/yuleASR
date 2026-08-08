/******************************************************************************
 * @file    isotp_canif.h
 * @brief   IsoTp CAN Interface Adapter (CanIf)
 *
 * Provides CAN interface adaptation layer for IsoTp protocol stack
 * Abstracts underlying CAN hardware for portability
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ISOTP_CANIF_H
#define ISOTP_CANIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "isotp_types.h"

/******************************************************************************
 * CanIf Module Version
 ******************************************************************************/
#define CANIF_VENDOR_ID                 0x01U
#define CANIF_MODULE_ID                 0x30U
#define CANIF_SW_MAJOR_VERSION          1U
#define CANIF_SW_MINOR_VERSION          0U
#define CANIF_SW_PATCH_VERSION          0U

/******************************************************************************
 * CanIf Return Types
 ******************************************************************************/

typedef enum {
    CANIF_OK = 0x00U,
    CANIF_ERROR = 0x01U,
    CANIF_BUSY = 0x02U,
    CANIF_TIMEOUT = 0x03U,
    CANIF_INVALID_PARAMETER = 0x04U,
    CANIF_NOT_INITIALIZED = 0x05U,
    CANIF_NO_BUFFER = 0x06U,
    CANIF_TX_QUEUE_FULL = 0x07U,
    CANIF_HW_ERROR = 0x08U
} CanIf_ReturnType;

/******************************************************************************
 * CAN Hardware Types
 ******************************************************************************/

typedef enum {
    CAN_HW_STANDARD = 0x00U,             /* Standard CAN 2.0 */
    CAN_HW_FD,                           /* CAN FD capable */
    CAN_HW_FD_BRS                        /* CAN FD with Bit Rate Switching */
} CanIf_HwType;

typedef enum {
    CAN_ID_TYPE_STANDARD = 0x00U,        /* 11-bit CAN ID */
    CAN_ID_TYPE_EXTENDED                 /* 29-bit CAN ID */
} CanIf_IdType;

/******************************************************************************
 * CAN Frame Structure
 ******************************************************************************/

typedef struct {
    uint32_t canId;                      /* CAN identifier */
    CanIf_IdType idType;                 /* Standard or Extended ID */
    uint8_t dlc;                         /* Data length code */
    uint8_t data[64];                    /* Frame data (up to 64 bytes for CAN FD) */
    bool isFd;                           /* CAN FD frame */
    bool isBrs;                          /* Bit rate switching enabled */
    uint64_t timestamp;                  /* Reception timestamp */
} CanIf_FrameType;

/******************************************************************************
 * CAN Controller Configuration
 ******************************************************************************/

typedef struct {
    uint8_t controllerId;                /* Controller ID */
    CanIf_HwType hwType;                 /* Hardware type */
    uint32_t baudrate;                   /* Nominal baudrate (bps) */
    uint32_t fdBaudrate;                 /* FD data baudrate (bps) */
    bool fdEnabled;                      /* FD mode enabled */
    uint8_t txQueueSize;                 /* TX queue size */
    uint8_t rxQueueSize;                 /* RX queue size */
} CanIf_ControllerConfigType;

/******************************************************************************
 * Hardware Filter Configuration
 ******************************************************************************/

typedef struct {
    uint32_t canId;                      /* CAN ID to filter */
    uint32_t mask;                       /* Filter mask */
    CanIf_IdType idType;                 /* ID type */
    uint8_t controllerId;                /* Associated controller */
} CanIf_FilterType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

/* Frame reception callback */
typedef void (*CanIf_RxCallback)(
    uint8_t controllerId,
    const CanIf_FrameType *frame,
    void *userData
);

/* Transmit confirmation callback */
typedef void (*CanIf_TxConfirmationCallback)(
    uint8_t controllerId,
    uint32_t canId,
    CanIf_ReturnType result,
    void *userData
);

/* Controller mode change callback */
typedef void (*CanIf_ControllerModeCallback)(
    uint8_t controllerId,
    uint8_t mode,
    void *userData
);

/* Bus error callback */
typedef void (*CanIf_ErrorCallback)(
    uint8_t controllerId,
    uint32_t errorCode,
    void *userData
);

/******************************************************************************
 * Hardware Driver Interface
 ******************************************************************************/

/* Hardware driver function pointers (to be implemented by platform) */
typedef struct {
    /* Initialize hardware */
    CanIf_ReturnType (*init)(uint8_t controllerId, const CanIf_ControllerConfigType *config);
    
    /* Deinitialize hardware */
    CanIf_ReturnType (*deinit)(uint8_t controllerId);
    
    /* Start controller */
    CanIf_ReturnType (*start)(uint8_t controllerId);
    
    /* Stop controller */
    CanIf_ReturnType (*stop)(uint8_t controllerId);
    
    /* Transmit frame */
    CanIf_ReturnType (*transmit)(uint8_t controllerId, const CanIf_FrameType *frame);
    
    /* Set filter */
    CanIf_ReturnType (*setFilter)(uint8_t controllerId, const CanIf_FilterType *filter);
    
    /* Clear filter */
    CanIf_ReturnType (*clearFilter)(uint8_t controllerId, uint8_t filterIndex);
    
    /* Get controller status */
    CanIf_ReturnType (*getStatus)(uint8_t controllerId, uint32_t *status);
    
} CanIf_HwDriverType;

/******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * @brief Initialize CanIf module
 * @param controllers Array of controller configurations
 * @param numControllers Number of controllers
 * @param hwDriver Hardware driver interface
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_Init(
    const CanIf_ControllerConfigType *controllers,
    uint8_t numControllers,
    const CanIf_HwDriverType *hwDriver
);

/**
 * @brief Deinitialize CanIf module
 */
void CanIf_DeInit(void);

/******************************************************************************
 * Controller Management
 ******************************************************************************/

/**
 * @brief Set controller mode
 * @param controllerId Controller ID
 * @param mode Target mode (0=stopped, 1=started, 2=sleep)
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_SetControllerMode(uint8_t controllerId, uint8_t mode);

/**
 * @brief Get controller mode
 * @param controllerId Controller ID
 * @param mode Output mode
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_GetControllerMode(uint8_t controllerId, uint8_t *mode);

/******************************************************************************
 * Transmission Functions
 ******************************************************************************/

/**
 * @brief Transmit CAN frame
 * @param controllerId Controller ID
 * @param frame Frame to transmit
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_Transmit(
    uint8_t controllerId,
    const CanIf_FrameType *frame
);

/**
 * @brief Transmit frame with wait for confirmation
 * @param controllerId Controller ID
 * @param frame Frame to transmit
 * @param timeoutMs Timeout in milliseconds
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_TransmitBlocking(
    uint8_t controllerId,
    const CanIf_FrameType *frame,
    uint32_t timeoutMs
);

/******************************************************************************
 * Reception Functions
 ******************************************************************************/

/**
 * @brief Process received frame (called by hardware ISR)
 * @param controllerId Controller ID
 * @param frame Received frame
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_ProcessRxFrame(
    uint8_t controllerId,
    const CanIf_FrameType *frame
);

/**
 * @brief Register receive callback
 * @param canId CAN ID to receive
 * @param callback Callback function
 * @param userData User data pointer
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_RegisterRxCallback(
    uint32_t canId,
    CanIf_RxCallback callback,
    void *userData
);

/**
 * @brief Unregister receive callback
 * @param canId CAN ID
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_UnregisterRxCallback(uint32_t canId);

/******************************************************************************
 * Filter Functions
 ******************************************************************************/

/**
 * @brief Add hardware filter
 * @param controllerId Controller ID
 * @param filter Filter configuration
 * @param filterIndex Output filter index
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_AddFilter(
    uint8_t controllerId,
    const CanIf_FilterType *filter,
    uint8_t *filterIndex
);

/**
 * @brief Remove hardware filter
 * @param controllerId Controller ID
 * @param filterIndex Filter index
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_RemoveFilter(uint8_t controllerId, uint8_t filterIndex);

/******************************************************************************
 * Callback Registration
 ******************************************************************************/

/**
 * @brief Register transmit confirmation callback
 * @param callback Callback function
 */
void CanIf_RegisterTxConfirmationCallback(CanIf_TxConfirmationCallback callback);

/**
 * @brief Register controller mode callback
 * @param callback Callback function
 */
void CanIf_RegisterControllerModeCallback(CanIf_ControllerModeCallback callback);

/**
 * @brief Register error callback
 * @param callback Callback function
 */
void CanIf_RegisterErrorCallback(CanIf_ErrorCallback callback);

/******************************************************************************
 * IsoTp Integration Functions
 ******************************************************************************/

/**
 * @brief IsoTp CAN transmit callback wrapper
 * @param canId CAN ID
 * @param data Frame data
 * @param length Frame length
 * @param isFd CAN FD flag
 * @param userData User data pointer
 * @return ISOTP_E_OK on success
 */
Isotp_ReturnType CanIf_IsotpTransmitWrapper(
    uint32_t canId,
    uint8_t *data,
    uint8_t length,
    bool isFd,
    void *userData
);

/**
 * @brief Map IsoTp channel to CAN controller
 * @param channelId IsoTp channel ID
 * @param controllerId CAN controller ID
 * @param txCanId Transmit CAN ID
 * @param rxCanId Receive CAN ID
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_MapIsotpChannel(
    uint8_t channelId,
    uint8_t controllerId,
    uint32_t txCanId,
    uint32_t rxCanId
);

/**
 * @brief Initialize IsoTp channel mapping
 * @param channelId IsoTp channel ID
 * @param isFd CAN FD flag
 * @return CANIF_OK on success
 */
CanIf_ReturnType CanIf_InitIsotpChannel(uint8_t channelId, bool isFd);

/******************************************************************************
 * Utility Functions
 ******************************************************************************/

/**
 * @brief Convert DLC to actual data length
 * @param dlc Data length code
 * @return Actual data length in bytes
 */
uint8_t CanIf_DlcToLength(uint8_t dlc);

/**
 * @brief Convert data length to DLC
 * @param length Data length in bytes
 * @return Data length code
 */
uint8_t CanIf_LengthToDlc(uint8_t length);

/**
 * @brief Check if CAN ID is valid
 * @param canId CAN ID
 * @param idType ID type (standard/extended)
 * @return true if valid
 */
bool CanIf_IsValidCanId(uint32_t canId, CanIf_IdType idType);

/**
 * @brief Get controller configuration
 * @param controllerId Controller ID
 * @return Configuration pointer or NULL
 */
const CanIf_ControllerConfigType* CanIf_GetControllerConfig(uint8_t controllerId);

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main processing function (called cyclically)
 */
void CanIf_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* ISOTP_CANIF_H */

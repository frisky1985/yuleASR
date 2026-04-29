/******************************************************************************
 * @file    docan_core.h
 * @brief   DoCAN (Diagnostic Communication over CAN) Core Module
 *
 * ISO 15765-2:2016 compliant implementation
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DOCAN_CORE_H
#define DOCAN_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "docan_types.h"

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize the DoCAN module
 * @param ConfigPtr Pointer to configuration structure
 * @return DOCAN_OK if successful, error code otherwise
 */
DoCan_ReturnType DoCan_Init(const DoCan_ConfigType *ConfigPtr);

/**
 * @brief Deinitialize the DoCAN module
 * @return DOCAN_OK if successful, error code otherwise
 */
DoCan_ReturnType DoCan_DeInit(void);

/**
 * @brief Get version information
 * @param VersionInfo Pointer to version info structure
 */
void DoCan_GetVersionInfo(Std_VersionInfoType *VersionInfo);

/******************************************************************************
 * Function Prototypes - Transmission
 ******************************************************************************/

/**
 * @brief Transmit diagnostic message
 * @param ConnectionId Connection identifier
 * @param DataPtr Pointer to data to transmit
 * @param Length Data length (1-4095 bytes)
 * @return DOCAN_OK if transmission started, error code otherwise
 */
DoCan_ReturnType DoCan_Transmit(
    uint8_t ConnectionId,
    const uint8_t *DataPtr,
    uint32_t Length
);

/**
 * @brief Cancel ongoing transmission
 * @param ConnectionId Connection identifier
 * @return DOCAN_OK if cancelled, error code otherwise
 */
DoCan_ReturnType DoCan_CancelTransmit(uint8_t ConnectionId);

/******************************************************************************
 * Function Prototypes - Reception
 ******************************************************************************/

/**
 * @brief Process received CAN frame
 * @param RxPduId PDU identifier (maps to connection)
 * @param CanId CAN identifier of received frame
 * @param DataPtr Pointer to frame data
 * @param Length Frame data length
 * @return DOCAN_OK if processed, error code otherwise
 */
DoCan_ReturnType DoCan_RxIndication(
    uint8_t RxPduId,
    uint32_t CanId,
    const uint8_t *DataPtr,
    uint8_t Length
);

/**
 * @brief Set buffer for reception
 * @param ConnectionId Connection identifier
 * @param BufferPtr Pointer to reception buffer
 * @param BufferSize Buffer size
 * @return DOCAN_OK if successful, error code otherwise
 */
DoCan_ReturnType DoCan_SetRxBuffer(
    uint8_t ConnectionId,
    uint8_t *BufferPtr,
    uint16_t BufferSize
);

/******************************************************************************
 * Function Prototypes - Main Function
 ******************************************************************************/

/**
 * @brief Main processing function (must be called periodically)
 * Handles timeouts, state transitions, and frame transmission timing
 */
void DoCan_MainFunction(void);

/******************************************************************************
 * Function Prototypes - Connection Management
 ******************************************************************************/

/**
 * @brief Get connection state
 * @param ConnectionId Connection identifier
 * @param StatePtr Output: pointer to state variable
 * @return DOCAN_OK if successful, error code otherwise
 */
DoCan_ReturnType DoCan_GetConnectionState(
    uint8_t ConnectionId,
    DoCan_ConnectionStateType *StatePtr
);

/**
 * @brief Reset connection to idle state
 * @param ConnectionId Connection identifier
 * @return DOCAN_OK if successful, error code otherwise
 */
DoCan_ReturnType DoCan_ResetConnection(uint8_t ConnectionId);

/**
 * @brief Set flow control parameters for a connection
 * @param ConnectionId Connection identifier
 * @param FcParams Pointer to FC parameters
 * @return DOCAN_OK if successful, error code otherwise
 */
DoCan_ReturnType DoCan_SetFlowControlParams(
    uint8_t ConnectionId,
    const DoCan_FlowControlParamsType *FcParams
);

/******************************************************************************
 * Function Prototypes - Address Management
 ******************************************************************************/

/**
 * @brief Find connection by CAN ID
 * @param CanId CAN identifier
 * @param CanIdType Standard or Extended CAN ID
 * @param IsRx TRUE if for reception, FALSE if for transmission
 * @return Connection ID if found, 0xFF if not found
 */
uint8_t DoCan_FindConnectionByCanId(
    uint32_t CanId,
    DoCan_CanIdTypeType CanIdType,
    boolean IsRx
);

/**
 * @brief Get CAN ID for connection
 * @param ConnectionId Connection identifier
 * @param IsTx TRUE for TX CAN ID, FALSE for RX CAN ID
 * @return CAN ID, 0 if invalid
 */
uint32_t DoCan_GetCanId(uint8_t ConnectionId, boolean IsTx);

/******************************************************************************
 * Function Prototypes - Utility
 ******************************************************************************/

/**
 * @brief Convert STmin value to microseconds
 * @param STmin STmin value from FC frame
 * @return STmin in microseconds
 */
uint32_t DoCan_STminToMicroseconds(uint8_t STmin);

/**
 * @brief Calculate maximum payload for frame type
 * @param IsCanFd TRUE for CAN FD, FALSE for Classic CAN
 * @param AddressingMode Addressing mode
 * @return Maximum payload bytes
 */
uint8_t DoCan_GetMaxPayloadLength(
    boolean IsCanFd,
    DoCan_AddressingModeType AddressingMode
);

/**
 * @brief Check if message fits in single frame
 * @param Length Message length
 * @param IsCanFd TRUE for CAN FD, FALSE for Classic CAN
 * @param AddressingMode Addressing mode
 * @return TRUE if fits in SF, FALSE otherwise
 */
boolean DoCan_CanUseSingleFrame(
    uint32_t Length,
    boolean IsCanFd,
    DoCan_AddressingModeType AddressingMode
);

/******************************************************************************
 * Function Prototypes - Internal State Access (for debugging/testing)
 ******************************************************************************/

#ifdef DOCAN_DEBUG_ACCESS
/**
 * @brief Get pointer to connection runtime state
 * @param ConnectionId Connection identifier
 * @return Pointer to state structure, NULL if invalid
 */
const DoCan_ConnectionStateType* DoCan_GetConnectionStatePtr(uint8_t ConnectionId);

/**
 * @brief Get number of active connections
 * @return Number of active connections
 */
uint8_t DoCan_GetActiveConnectionCount(void);
#endif /* DOCAN_DEBUG_ACCESS */

#ifdef __cplusplus
}
#endif

#endif /* DOCAN_CORE_H */

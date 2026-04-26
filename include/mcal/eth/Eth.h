/**
 * @file Eth.h
 * @brief Eth (Ethernet Driver) API Header
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Eth Module - Ethernet Hardware Driver
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x11
 *
 * Features:
 * - Multi-controller support (up to ETH_MAX_CONTROLLERS)
 * - DMA-based TX/RX with descriptor rings
 * - PHY management (MII/RMII/RGMII interface)
 * - Interrupt-driven and polled modes
 * - Link state management
 * - MAC address filtering
 * - Flow control support
 * - MISRA C:2012 compliant
 */

#ifndef ETH_H
#define ETH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Eth_Types.h"

/*============================================================================*
 * External Configuration
 *============================================================================*/
extern const Eth_ConfigType Eth_Config;

/*============================================================================*
 * Initialization API
 *============================================================================*/

/**
 * @brief Initialize the Eth module
 *
 * Initializes the Ethernet driver with the provided configuration.
 * Must be called before any other Eth API.
 *
 * @param config Pointer to configuration structure (NULL for default)
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_Init(const Eth_ConfigType* config);

/**
 * @brief Deinitialize the Eth module
 *
 * Deinitializes the Ethernet driver and releases resources.
 *
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_Deinit(void);

/**
 * @brief Initialize a specific controller
 *
 * @param ctrlIdx Controller index (0 to controllerCount-1)
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_ControllerInit(uint8_t ctrlIdx);

/**
 * @brief Deinitialize a specific controller
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_ControllerDeinit(uint8_t ctrlIdx);

/**
 * @brief Set controller mode
 *
 * @param ctrlIdx Controller index
 * @param mode New mode (ETH_MODE_DOWN, ETH_MODE_ACTIVE, etc.)
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_SetControllerMode(uint8_t ctrlIdx, Eth_ModeType mode);

/**
 * @brief Get controller mode
 *
 * @param ctrlIdx Controller index
 * @return Current controller mode
 */
Eth_ModeType Eth_GetControllerMode(uint8_t ctrlIdx);

/**
 * @brief Check if module is initialized
 *
 * @return true if initialized, false otherwise
 */
bool Eth_IsInitialized(void);

/**
 * @brief Check if controller is initialized
 *
 * @param ctrlIdx Controller index
 * @return true if initialized, false otherwise
 */
bool Eth_IsControllerInitialized(uint8_t ctrlIdx);

/*============================================================================*
 * MAC Address API
 *============================================================================*/

/**
 * @brief Set MAC address for controller
 *
 * @param ctrlIdx Controller index
 * @param macAddr Pointer to MAC address
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_SetMacAddr(
    uint8_t ctrlIdx,
    const Eth_MacAddrType* macAddr
);

/**
 * @brief Get MAC address from controller
 *
 * @param ctrlIdx Controller index
 * @param macAddr Pointer to store MAC address
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_GetMacAddr(
    uint8_t ctrlIdx,
    Eth_MacAddrType* macAddr
);

/**
 * @brief Add MAC address to filter
 *
 * @param ctrlIdx Controller index
 * @param macAddr Pointer to MAC address
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_AddMacFilter(
    uint8_t ctrlIdx,
    const Eth_MacAddrType* macAddr
);

/**
 * @brief Remove MAC address from filter
 *
 * @param ctrlIdx Controller index
 * @param macAddr Pointer to MAC address
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_RemoveMacFilter(
    uint8_t ctrlIdx,
    const Eth_MacAddrType* macAddr
);

/**
 * @brief Clear all MAC address filters
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_ClearMacFilters(uint8_t ctrlIdx);

/*============================================================================*
 * Transmission API
 *============================================================================*/

/**
 * @brief Transmit Ethernet frame
 *
 * Synchronous transmission of an Ethernet frame.
 *
 * @param ctrlIdx Controller index
 * @param data Pointer to frame data (including header)
 * @param len Frame length (including header, excluding FCS)
 * @param bufIdx Pointer to store buffer index (can be NULL)
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_Transmit(
    uint8_t ctrlIdx,
    const uint8_t* data,
    uint16_t len,
    uint8_t* bufIdx
);

/**
 * @brief Get TX buffer for transmission
 *
 * Acquires a TX buffer that can be filled and then transmitted.
 *
 * @param ctrlIdx Controller index
 * @param len Required buffer length
 * @param buf Pointer to store buffer pointer
 * @param bufIdx Pointer to store buffer index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_GetTxBuffer(
    uint8_t ctrlIdx,
    uint16_t len,
    uint8_t** buf,
    uint8_t* bufIdx
);

/**
 * @brief Transmit a previously acquired TX buffer
 *
 * @param ctrlIdx Controller index
 * @param bufIdx Buffer index from Eth_GetTxBuffer
 * @param len Actual frame length
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_TransmitTxBuffer(
    uint8_t ctrlIdx,
    uint8_t bufIdx,
    uint16_t len
);

/**
 * @brief Check if TX is ready (not busy)
 *
 * @param ctrlIdx Controller index
 * @return true if TX is ready, false otherwise
 */
bool Eth_IsTxReady(uint8_t ctrlIdx);

/**
 * @brief Wait for TX completion
 *
 * @param ctrlIdx Controller index
 * @param timeout Timeout in milliseconds (0 for non-blocking)
 * @return ETH_OK on success, ETH_E_TIMEOUT on timeout
 */
Eth_ErrorCode_t Eth_WaitTxComplete(uint8_t ctrlIdx, uint32_t timeout);

/*============================================================================*
 * Reception API
 *============================================================================*/

/**
 * @brief Receive Ethernet frame
 *
 * Synchronous reception of an Ethernet frame.
 *
 * @param ctrlIdx Controller index
 * @param data Pointer to buffer for frame data
 * @param len Pointer to store received length
 * @param frameInfo Pointer to store frame info (can be NULL)
 * @return ETH_OK on success, ETH_E_NO_CTRL if no frame available
 */
Eth_ErrorCode_t Eth_Receive(
    uint8_t ctrlIdx,
    uint8_t* data,
    uint16_t* len,
    Eth_FrameInfoType* frameInfo
);

/**
 * @brief Get pending RX frame count
 *
 * @param ctrlIdx Controller index
 * @return Number of pending RX frames
 */
uint16_t Eth_GetRxPendingCount(uint8_t ctrlIdx);

/**
 * @brief Check if RX frame is available
 *
 * @param ctrlIdx Controller index
 * @return true if RX frame is available, false otherwise
 */
bool Eth_IsRxAvailable(uint8_t ctrlIdx);

/**
 * @brief Flush all pending RX buffers
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_FlushRxBuffers(uint8_t ctrlIdx);

/*============================================================================*
 * PHY Management API
 *============================================================================*/

/**
 * @brief Read PHY register via MDIO
 *
 * @param ctrlIdx Controller index
 * @param regAddr PHY register address (0-31)
 * @param regVal Pointer to store register value
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_ReadPhy(
    uint8_t ctrlIdx,
    uint8_t regAddr,
    uint16_t* regVal
);

/**
 * @brief Write PHY register via MDIO
 *
 * @param ctrlIdx Controller index
 * @param regAddr PHY register address (0-31)
 * @param regVal Register value to write
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_WritePhy(
    uint8_t ctrlIdx,
    uint8_t regAddr,
    uint16_t regVal
);

/**
 * @brief Get PHY ID
 *
 * @param ctrlIdx Controller index
 * @param phyId1 Pointer to store PHY ID1
 * @param phyId2 Pointer to store PHY ID2
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_GetPhyId(
    uint8_t ctrlIdx,
    uint16_t* phyId1,
    uint16_t* phyId2
);

/**
 * @brief Check PHY link state
 *
 * @param ctrlIdx Controller index
 * @return true if link is up, false otherwise
 */
bool Eth_GetLinkState(uint8_t ctrlIdx);

/**
 * @brief Wait for link up
 *
 * @param ctrlIdx Controller index
 * @param timeout Timeout in milliseconds
 * @return ETH_OK on success, ETH_E_TIMEOUT on timeout
 */
Eth_ErrorCode_t Eth_WaitLinkUp(uint8_t ctrlIdx, uint32_t timeout);

/**
 * @brief Get current baud rate
 *
 * @param ctrlIdx Controller index
 * @return Current baud rate (Eth_SpeedType)
 */
Eth_SpeedType Eth_GetBaudRate(uint8_t ctrlIdx);

/**
 * @brief Get current duplex mode
 *
 * @param ctrlIdx Controller index
 * @return Current duplex mode
 */
Eth_DuplexModeType Eth_GetDuplexMode(uint8_t ctrlIdx);

/**
 * @brief Update PHY configuration (after link change)
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_UpdatePhyConfig(uint8_t ctrlIdx);

/*============================================================================*
 * Flow Control API
 *============================================================================*/

/**
 * @brief Enable flow control (IEEE 802.3x)
 *
 * @param ctrlIdx Controller index
 * @param pauseTime Pause time in quanta (0 to disable)
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_EnableFlowControl(uint8_t ctrlIdx, uint16_t pauseTime);

/**
 * @brief Disable flow control
 *
 * @param ctrlIdx Controller index
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_DisableFlowControl(uint8_t ctrlIdx);

/**
 * @brief Send pause frame
 *
 * @param ctrlIdx Controller index
 * @param pauseTime Pause time in quanta
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_SendPauseFrame(uint8_t ctrlIdx, uint16_t pauseTime);

/*============================================================================*
 * Interrupt API
 *============================================================================*/

/**
 * @brief Enable interrupts for controller
 *
 * @param ctrlIdx Controller index
 * @param irqMask Interrupt mask (Eth_IrqEventType combination)
 */
void Eth_EnableIrq(uint8_t ctrlIdx, uint32_t irqMask);

/**
 * @brief Disable interrupts for controller
 *
 * @param ctrlIdx Controller index
 * @param irqMask Interrupt mask (Eth_IrqEventType combination)
 */
void Eth_DisableIrq(uint8_t ctrlIdx, uint32_t irqMask);

/**
 * @brief Get pending interrupts
 *
 * @param ctrlIdx Controller index
 * @return Pending interrupt flags
 */
uint32_t Eth_GetPendingIrqs(uint8_t ctrlIdx);

/**
 * @brief Clear interrupt flag
 *
 * @param ctrlIdx Controller index
 * @param event Interrupt event to clear
 */
void Eth_ClearIrq(uint8_t ctrlIdx, Eth_IrqEventType event);

/**
 * @brief Process interrupts for controller (polled mode)
 *
 * @param ctrlIdx Controller index
 * @param event Pointer to store event type
 * @return true if event processed, false otherwise
 */
bool Eth_ProcessIrq(uint8_t ctrlIdx, Eth_IrqEventType* event);

/*============================================================================*
 * Main Function (Cyclic Processing)
 *============================================================================*/

/**
 * @brief Main function for cyclic processing
 *
 * Must be called periodically to process:
 * - Link state changes
 * - PHY status updates
 * - Deferred interrupt handling
 *
 * @param ctrlIdx Controller index (0xFF for all controllers)
 */
void Eth_MainFunction(uint8_t ctrlIdx);

/*============================================================================*
 * Statistics API
 *============================================================================*/

/**
 * @brief Get TX frame count
 *
 * @param ctrlIdx Controller index
 * @return Number of transmitted frames
 */
uint32_t Eth_GetTxFrameCount(uint8_t ctrlIdx);

/**
 * @brief Get RX frame count
 *
 * @param ctrlIdx Controller index
 * @return Number of received frames
 */
uint32_t Eth_GetRxFrameCount(uint8_t ctrlIdx);

/**
 * @brief Get error counts
 *
 * @param ctrlIdx Controller index
 * @param txErrors Pointer to store TX errors
 * @param rxErrors Pointer to store RX errors
 * @param crcErrors Pointer to store CRC errors
 */
void Eth_GetErrorCounts(
    uint8_t ctrlIdx,
    uint32_t* txErrors,
    uint32_t* rxErrors,
    uint32_t* crcErrors
);

/**
 * @brief Reset statistics counters
 *
 * @param ctrlIdx Controller index
 */
void Eth_ResetStatistics(uint8_t ctrlIdx);

/*============================================================================*
 * Callback Registration
 *============================================================================*/

/**
 * @brief Set TX confirmation callback
 *
 * @param callback Callback function pointer
 */
void Eth_SetTxConfirmationCallback(Eth_TxConfirmationCallback_t callback);

/**
 * @brief Set RX indication callback
 *
 * @param callback Callback function pointer
 */
void Eth_SetRxIndicationCallback(Eth_RxIndicationCallback_t callback);

/**
 * @brief Set link state change callback
 *
 * @param callback Callback function pointer
 */
void Eth_SetLinkStateChangeCallback(Eth_LinkStateChangeCallback_t callback);

/**
 * @brief Set error callback
 *
 * @param callback Callback function pointer
 */
void Eth_SetErrorCallback(Eth_ErrorCallback_t callback);

/*============================================================================*
 * Version Info API
 *============================================================================*/

/**
 * @brief Get Eth module version
 *
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_GetVersionInfo(
    uint8_t* major,
    uint8_t* minor,
    uint8_t* patch
);

/*============================================================================*
 * Hardware Interface Registration
 *============================================================================*/

/**
 * @brief Register hardware interface
 *
 * @param hwInterface Hardware interface implementation
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_RegisterHwInterface(
    const Eth_HwInterfaceType* hwInterface
);

/**
 * @brief Get registered hardware interface
 *
 * @return Pointer to hardware interface, NULL if not registered
 */
const Eth_HwInterfaceType* Eth_GetHwInterface(void);

/*============================================================================*
 * Utility Functions
 *============================================================================*/

/**
 * @brief Convert speed enum to Mbps value
 *
 * @param speed Speed enum
 * @return Speed in Mbps (10, 100, 1000, etc.)
 */
uint16_t Eth_SpeedToMbps(Eth_SpeedType speed);

/**
 * @brief Convert Mbps value to speed enum
 *
 * @param mbps Speed in Mbps
 * @return Speed enum
 */
Eth_SpeedType Eth_MbpsToSpeed(uint16_t mbps);

/**
 * @brief Compare two MAC addresses
 *
 * @param addr1 First MAC address
 * @param addr2 Second MAC address
 * @return true if equal, false otherwise
 */
bool Eth_MacAddrEqual(
    const Eth_MacAddrType* addr1,
    const Eth_MacAddrType* addr2
);

/**
 * @brief Copy MAC address
 *
 * @param dest Destination MAC address
 * @param src Source MAC address
 */
void Eth_MacAddrCopy(Eth_MacAddrType* dest, const Eth_MacAddrType* src);

/**
 * @brief Check if MAC address is multicast
 *
 * @param addr MAC address
 * @return true if multicast, false otherwise
 */
bool Eth_IsMulticastAddr(const Eth_MacAddrType* addr);

/**
 * @brief Check if MAC address is broadcast
 *
 * @param addr MAC address
 * @return true if broadcast, false otherwise
 */
bool Eth_IsBroadcastAddr(const Eth_MacAddrType* addr);

/**
 * @brief Convert MAC address to string
 *
 * @param addr MAC address
 * @param str Buffer for string (at least 18 bytes: "xx:xx:xx:xx:xx:xx\0")
 * @return ETH_OK on success, error code otherwise
 */
Eth_ErrorCode_t Eth_MacAddrToString(
    const Eth_MacAddrType* addr,
    char* str
);

#ifdef __cplusplus
}
#endif

#endif /* ETH_H */

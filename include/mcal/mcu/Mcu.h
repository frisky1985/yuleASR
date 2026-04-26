/**
 * @file Mcu.h
 * @brief Mcu (Microcontroller Driver) API Header
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Mcu Module - Microcontroller Driver
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x12
 *
 * Features:
 * - Multi-platform clock initialization (OSC/PLL/System/Bus)
 * - Reset reason detection and handling
 * - RAM initialization with ECC support
 * - Low power mode management
 * - Peripheral clock gating
 * - MISRA C:2012 compliant
 */

#ifndef MCU_H
#define MCU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mcu_Types.h"

/*============================================================================*
 * External Configuration
 *============================================================================*/
extern const Mcu_ConfigType Mcu_Config;

/*============================================================================*
 * Initialization API
 *============================================================================*/

/**
 * @brief Initialize the Mcu module
 *
 * Initializes the microcontroller driver with the provided configuration.
 * Must be called before any other Mcu API.
 *
 * @param config Pointer to configuration structure (NULL for default)
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_Init(const Mcu_ConfigType* config);

/**
 * @brief Deinitialize the Mcu module
 *
 * Deinitializes the microcontroller driver and releases resources.
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_Deinit(void);

/**
 * @brief Check if module is initialized
 *
 * @return true if initialized, false otherwise
 */
bool Mcu_IsInitialized(void);

/*============================================================================*
 * Clock Initialization API
 *============================================================================*/

/**
 * @brief Initialize clock system
 *
 * Configures the clock system according to the specified clock setting.
 * This includes OSC, PLL, and clock dividers.
 *
 * @param clockSetting Pointer to clock setting configuration
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitClock(const Mcu_ClockSettingConfigType* clockSetting);

/**
 * @brief Initialize clock by setting ID
 *
 * @param clockSettingId Clock setting ID (index in configuration)
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitClockById(uint8_t clockSettingId);

/**
 * @brief Distribute PLL clock to system
 *
 * Must be called after PLL is locked to switch system clock to PLL output.
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_DistributePllClock(void);

/**
 * @brief Check if PLL is locked
 *
 * @return true if PLL is locked, false otherwise
 */
bool Mcu_GetPllStatus(void);

/**
 * @brief Wait for PLL lock
 *
 * @param timeout Timeout in microseconds (0 for non-blocking)
 * @return MCU_OK on success, MCU_E_TIMEOUT on timeout
 */
Mcu_ErrorCode_t Mcu_WaitPllLock(uint32_t timeout);

/**
 * @brief Get current clock setting ID
 *
 * @return Current clock setting ID, 0xFF if not configured
 */
uint8_t Mcu_GetClockSetting(void);

/*============================================================================*
 * Clock Frequency API
 *============================================================================*/

/**
 * @brief Get system clock frequency
 *
 * @return System clock frequency in Hz
 */
uint32_t Mcu_GetSysClockFreq(void);

/**
 * @brief Get CPU clock frequency
 *
 * @return CPU clock frequency in Hz
 */
uint32_t Mcu_GetCpuClockFreq(void);

/**
 * @brief Get bus clock frequency
 *
 * @return Bus clock frequency in Hz
 */
uint32_t Mcu_GetBusClockFreq(void);

/**
 * @brief Get peripheral clock frequency
 *
 * @param periphId Peripheral ID
 * @return Peripheral clock frequency in Hz, 0 if disabled
 */
uint32_t Mcu_GetPeriphClockFreq(uint8_t periphId);

/**
 * @brief Get specific clock frequency
 *
 * @param clockType Clock type (SYS, CPU, BUS, etc.)
 * @return Clock frequency in Hz
 */
uint32_t Mcu_GetClockFrequency(Mcu_ClockType clockType);

/*============================================================================*
 * Peripheral Clock API
 *============================================================================*/

/**
 * @brief Enable peripheral clock
 *
 * @param periphId Peripheral ID
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_EnablePeriphClock(uint8_t periphId);

/**
 * @brief Disable peripheral clock
 *
 * @param periphId Peripheral ID
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_DisablePeriphClock(uint8_t periphId);

/**
 * @brief Check if peripheral clock is enabled
 *
 * @param periphId Peripheral ID
 * @return true if enabled, false otherwise
 */
bool Mcu_IsPeriphClockEnabled(uint8_t periphId);

/**
 * @brief Enable multiple peripheral clocks
 *
 * @param periphMask Bitmap of peripheral IDs to enable
 */
void Mcu_EnablePeriphClocks(uint64_t periphMask);

/**
 * @brief Disable multiple peripheral clocks
 *
 * @param periphMask Bitmap of peripheral IDs to disable
 */
void Mcu_DisablePeriphClocks(uint64_t periphMask);

/*============================================================================*
 * RAM Initialization API
 *============================================================================*/

/**
 * @brief Initialize all RAM sections
 *
 * Initializes all configured RAM sections according to their configuration.
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitRam(void);

/**
 * @brief Initialize specific RAM section
 *
 * @param sectionId RAM section ID
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitRamSection(uint8_t sectionId);

/**
 * @brief Initialize RAM section by address range
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @param initValue Initialization value
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitRamRange(
    uint32_t startAddr,
    uint32_t size,
    uint8_t initValue
);

/**
 * @brief Clear RAM section (set to zero)
 *
 * @param sectionId RAM section ID
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_ClearRamSection(uint8_t sectionId);

/**
 * @brief Get RAM section info
 *
 * @param sectionId RAM section ID
 * @param startAddr Pointer to store start address
 * @param size Pointer to store size
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_GetRamSectionInfo(
    uint8_t sectionId,
    uint32_t* startAddr,
    uint32_t* size
);

/*============================================================================*
 * Reset API
 *============================================================================*/

/**
 * @brief Perform microcontroller reset
 *
 * Initiates a system reset.
 *
 * @return This function does not return on success
 */
Mcu_ErrorCode_t Mcu_PerformReset(void);

/**
 * @brief Get reset reason
 *
 * @return Reset reason (Mcu_ResetType)
 */
Mcu_ResetType Mcu_GetResetReason(void);

/**
 * @brief Get raw reset register value
 *
 * @return Raw reset register value (platform-specific)
 */
uint32_t Mcu_GetResetRawValue(void);

/**
 * @brief Clear reset reason
 *
 * Clears the reset reason register.
 */
void Mcu_ClearResetReason(void);

/**
 * @brief Check if specific reset occurred
 *
 * @param resetType Reset type to check
 * @return true if this reset occurred, false otherwise
 */
bool Mcu_WasResetBy(Mcu_ResetType resetType);

/*============================================================================*
 * Mode Management API
 *============================================================================*/

/**
 * @brief Set microcontroller mode
 *
 * Sets the power mode (normal, sleep, deep sleep, etc.).
 *
 * @param mode Power mode to set
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_SetMode(Mcu_ModeType mode);

/**
 * @brief Get current microcontroller mode
 *
 * @return Current power mode
 */
Mcu_ModeType Mcu_GetMode(void);

/**
 * @brief Check if mode is available
 *
 * @param mode Power mode to check
 * @return true if mode is available, false otherwise
 */
bool Mcu_IsModeAvailable(Mcu_ModeType mode);

/**
 * @brief Enter sleep mode
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_EnterSleepMode(void);

/**
 * @brief Enter deep sleep mode
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_EnterDeepSleepMode(void);

/**
 * @brief Enter standby mode
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_EnterStandbyMode(void);

/**
 * @brief Wake up from low power mode
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_WakeUp(void);

/*============================================================================*
 * Watchdog API
 *============================================================================*/

/**
 * @brief Enable watchdog
 *
 * @param timeoutMs Watchdog timeout in milliseconds
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_EnableWatchdog(uint32_t timeoutMs);

/**
 * @brief Disable watchdog
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_DisableWatchdog(void);

/**
 * @brief Service watchdog (kick/refresh)
 *
 * Must be called periodically to prevent watchdog reset.
 */
void Mcu_ServiceWatchdog(void);

/**
 * @brief Check if watchdog caused last reset
 *
 * @return true if watchdog reset occurred, false otherwise
 */
bool Mcu_WasWatchdogReset(void);

/*============================================================================*
 * Main Function (Cyclic Processing)
 *============================================================================*/

/**
 * @brief Main function for cyclic processing
 *
 * Must be called periodically to process:
 * - Clock monitoring
 * - Watchdog servicing (if window watchdog)
 * - Mode transitions
 */
void Mcu_MainFunction(void);

/*============================================================================*
 * Version Info API
 *============================================================================*/

/**
 * @brief Get Mcu module version
 *
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_GetVersionInfo(
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
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_RegisterHwInterface(
    const Mcu_HwInterfaceType* hwInterface
);

/**
 * @brief Get registered hardware interface
 *
 * @return Pointer to hardware interface, NULL if not registered
 */
const Mcu_HwInterfaceType* Mcu_GetHwInterface(void);

/*============================================================================*
 * Utility Functions
 *============================================================================*/

/**
 * @brief Delay in microseconds (busy wait)
 *
 * @param us Microseconds to delay
 */
void Mcu_DelayUs(uint32_t us);

/**
 * @brief Delay in milliseconds (busy wait)
 *
 * @param ms Milliseconds to delay
 */
void Mcu_DelayMs(uint32_t ms);

/**
 * @brief Get hardware type
 *
 * @return Hardware type (Aurix, S32G, etc.)
 */
Mcu_HardwareType Mcu_GetHardwareType(void);

/**
 * @brief Get unique device ID
 *
 * @param id Buffer to store device ID (platform-specific size)
 * @param size Size of buffer
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_GetDeviceId(uint8_t* id, uint8_t* size);

#ifdef __cplusplus
}
#endif

#endif /* MCU_H */

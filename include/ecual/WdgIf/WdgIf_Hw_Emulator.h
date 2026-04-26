/******************************************************************************
 * @file    WdgIf_Hw_Emulator.h
 * @brief   Watchdog Interface Hardware Emulation
 *
 * Software emulation of watchdog hardware for testing and development.
 * Simulates watchdog behavior without actual hardware.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_HW_EMULATOR_H
#define WDGIF_HW_EMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf_Internal.h"

/******************************************************************************
 * Emulator Constants
 ******************************************************************************/

#define WDGIF_EMU_MAX_INSTANCES         3U
#define WDGIF_EMU_DEFAULT_TIMEOUT       100U
#define WDGIF_EMU_MIN_TIMEOUT           1U
#define WDGIF_EMU_MAX_TIMEOUT           60000U  /* 60 seconds */

/* Emulator flags */
#define WDGIF_EMU_FLAG_ENABLED          0x01U
#define WDGIF_EMU_FLAG_WINDOW_MODE      0x02U
#define WDGIF_EMU_FLAG_INTERRUPT_MODE   0x04U
#define WDGIF_EMU_FLAG_TRIGGERED        0x08U
#define WDGIF_EMU_FLAG_TIMEOUT_OCCURRED 0x10U
#define WDGIF_EMU_FLAG_RESET_OCCURRED   0x20U

/******************************************************************************
 * Emulator Types
 ******************************************************************************/

/* Emulated hardware state */
typedef struct {
    uint32 flags;
    uint32 timeoutValue;
    uint32 windowStart;
    uint32 windowEnd;
    uint32 currentCount;
    uint32 lastTriggerTime;
    uint32 triggerCount;
    uint32 missedTriggerCount;
    WdgIf_ModeType currentMode;
    boolean isRunning;
    uint32 tickFrequency;       /* Simulated tick frequency in Hz */
    uint32 elapsedTimeMs;
} WdgIf_EmulatorStateType;

/* Configuration for emulator */
typedef struct {
    uint32 simulatedClockFreq;
    boolean simulateWindowViolation;
    boolean simulateTimeout;
    uint32 triggerFailureRate;  /* 0-1000 (0.0% - 100.0%) */
} WdgIf_EmulatorConfigType;

/******************************************************************************
 * Emulator Interface Functions
 ******************************************************************************/

/**
 * @brief Initialize emulated watchdog
 * @param config Device configuration
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_Init(const WdgIf_DeviceConfigType *config);

/**
 * @brief Deinitialize emulated watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_DeInit(WdgIf_DeviceType device);

/**
 * @brief Set emulated watchdog mode
 * @param device Device index
 * @param mode Watchdog mode
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode);

/**
 * @brief Trigger emulated watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_Trigger(WdgIf_DeviceType device);

/**
 * @brief Get emulator version
 * @param version Pointer to store version
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_GetVersion(uint16 *version);

/**
 * @brief Force reset via emulated watchdog
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_Reset(WdgIf_DeviceType device);

/**
 * @brief Get emulated watchdog status
 * @param device Device index
 * @return Driver status
 */
WdgIf_DriverStatusType WdgIf_Emulator_GetStatus(WdgIf_DeviceType device);

/******************************************************************************
 * Emulator Control Functions
 ******************************************************************************/

/**
 * @brief Advance emulator timer (for testing)
 * @param device Device index
 * @param timeMs Time to advance in milliseconds
 */
void WdgIf_Emulator_AdvanceTime(WdgIf_DeviceType device, uint32 timeMs);

/**
 * @brief Check if emulator timeout occurred
 * @param device Device index
 * @return TRUE if timeout occurred
 */
boolean WdgIf_Emulator_IsTimeoutOccurred(WdgIf_DeviceType device);

/**
 * @brief Check if emulator reset occurred
 * @param device Device index
 * @return TRUE if reset occurred
 */
boolean WdgIf_Emulator_IsResetOccurred(WdgIf_DeviceType device);

/**
 * @brief Get emulator state
 * @param device Device index
 * @return Pointer to emulator state
 */
const WdgIf_EmulatorStateType* WdgIf_Emulator_GetState(WdgIf_DeviceType device);

/**
 * @brief Reset emulator state
 * @param device Device index
 */
void WdgIf_Emulator_ResetState(WdgIf_DeviceType device);

/**
 * @brief Configure emulator behavior
 * @param device Device index
 * @param emuConfig Emulator configuration
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Emulator_Configure(
    WdgIf_DeviceType device,
    const WdgIf_EmulatorConfigType *emuConfig
);

/**
 * @brief Simulate timeout condition (for testing)
 * @param device Device index
 */
void WdgIf_Emulator_SimulateTimeout(WdgIf_DeviceType device);

/**
 * @brief Simulate window violation (for testing)
 * @param device Device index
 */
void WdgIf_Emulator_SimulateWindowViolation(WdgIf_DeviceType device);

/******************************************************************************
 * Hardware Interface Structure
 ******************************************************************************/
extern const WdgIf_HwInterfaceType WdgIf_EmulatorHwInterface;

/******************************************************************************
 * External State Array (for testing access)
 ******************************************************************************/
extern WdgIf_EmulatorStateType WdgIf_EmulatorState[WDGIF_EMU_MAX_INSTANCES];

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_HW_EMULATOR_H */

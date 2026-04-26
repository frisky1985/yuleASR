/******************************************************************************
 * @file    WdgIf_Hw_Emulator.c
 * @brief   Watchdog Interface Hardware Emulation Implementation
 *
 * Software emulation of watchdog hardware for testing and development.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf_Hw_Emulator.h"
#include <string.h>

/******************************************************************************
 * Local Variables
 ******************************************************************************/
/* Emulator state for each device */
WdgIf_EmulatorStateType WdgIf_EmulatorState[WDGIF_EMU_MAX_INSTANCES];

/* Emulator configuration */
static WdgIf_EmulatorConfigType WdgIf_EmulatorConfigs[WDGIF_EMU_MAX_INSTANCES];

/* Module initialized flag */
static boolean WdgIf_EmulatorInitialized = FALSE;

/* Default emulator configuration */
static const WdgIf_EmulatorConfigType WdgIf_EmulatorDefaultConfig = {
    .simulatedClockFreq = 1000000U,  /* 1 MHz */
    .simulateWindowViolation = FALSE,
    .simulateTimeout = FALSE,
    .triggerFailureRate = 0U  /* 0% failure rate */
};

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static void WdgIf_Emulator_ResetStateInternal(uint8 instance);
static boolean WdgIf_Emulator_ShouldFail(uint8 instance);

/******************************************************************************
 * Hardware Interface Structure
 ******************************************************************************/
const WdgIf_HwInterfaceType WdgIf_EmulatorHwInterface = {
    .init = WdgIf_Emulator_Init,
    .deinit = WdgIf_Emulator_DeInit,
    .setMode = WdgIf_Emulator_SetMode,
    .trigger = WdgIf_Emulator_Trigger,
    .getVersion = WdgIf_Emulator_GetVersion,
    .reset = WdgIf_Emulator_Reset,
    .getStatus = WdgIf_Emulator_GetStatus
};

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initialize emulated watchdog
 */
Std_ReturnType WdgIf_Emulator_Init(const WdgIf_DeviceConfigType *config)
{
    uint8 instance;

    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    instance = (uint8)config->deviceId;
    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return E_NOT_OK;
    }

    /* Initialize emulator state */
    WdgIf_Emulator_ResetStateInternal(instance);

    /* Setup emulator configuration */
    WdgIf_EmulatorConfigs[instance] = WdgIf_EmulatorDefaultConfig;

    /* Set default timeout */
    if ((NULL_PTR != config->timeoutConfig) &&
        (config->timeoutConfig->slowModeTimeout > 0U)) {
        WdgIf_EmulatorState[instance].timeoutValue =
            config->timeoutConfig->slowModeTimeout;
    } else {
        WdgIf_EmulatorState[instance].timeoutValue = WDGIF_EMU_DEFAULT_TIMEOUT;
    }

    /* Mark as enabled and running */
    WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_ENABLED;
    WdgIf_EmulatorState[instance].isRunning = TRUE;
    WdgIf_EmulatorState[instance].currentMode = WDGIF_SLOW_MODE;

    WdgIf_EmulatorInitialized = TRUE;

    return E_OK;
}

/**
 * @brief Deinitialize emulated watchdog
 */
Std_ReturnType WdgIf_Emulator_DeInit(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return E_NOT_OK;
    }

    /* Reset state */
    WdgIf_Emulator_ResetStateInternal(instance);

    /* Check if any instances still active */
    if ((WdgIf_EmulatorState[0].flags == 0U) &&
        (WdgIf_EmulatorState[1].flags == 0U) &&
        (WdgIf_EmulatorState[2].flags == 0U)) {
        WdgIf_EmulatorInitialized = FALSE;
    }

    return E_OK;
}

/**
 * @brief Set emulated watchdog mode
 */
Std_ReturnType WdgIf_Emulator_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode)
{
    uint8 instance = (uint8)device;
    uint32 timeoutValue;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return E_NOT_OK;
    }

    switch (mode) {
        case WDGIF_OFF_MODE:
            WdgIf_EmulatorState[instance].flags &= ~WDGIF_EMU_FLAG_ENABLED;
            WdgIf_EmulatorState[instance].isRunning = FALSE;
            WdgIf_EmulatorState[instance].currentMode = WDGIF_OFF_MODE;
            break;

        case WDGIF_SLOW_MODE:
            WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_ENABLED;
            WdgIf_EmulatorState[instance].isRunning = TRUE;
            WdgIf_EmulatorState[instance].currentMode = WDGIF_SLOW_MODE;
            timeoutValue = 500U;  /* 500ms for slow mode */
            WdgIf_EmulatorState[instance].timeoutValue = timeoutValue;
            WdgIf_EmulatorState[instance].windowEnd = timeoutValue;
            WdgIf_EmulatorState[instance].currentCount = 0U;
            break;

        case WDGIF_FAST_MODE:
            WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_ENABLED;
            WdgIf_EmulatorState[instance].isRunning = TRUE;
            WdgIf_EmulatorState[instance].currentMode = WDGIF_FAST_MODE;
            timeoutValue = 50U;   /* 50ms for fast mode */
            WdgIf_EmulatorState[instance].timeoutValue = timeoutValue;
            WdgIf_EmulatorState[instance].windowEnd = timeoutValue;
            WdgIf_EmulatorState[instance].currentCount = 0U;
            break;

        default:
            return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Trigger emulated watchdog
 */
Std_ReturnType WdgIf_Emulator_Trigger(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;
    WdgIf_EmulatorStateType *state;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return E_NOT_OK;
    }

    state = &WdgIf_EmulatorState[instance];

    /* Check if enabled */
    if ((state->flags & WDGIF_EMU_FLAG_ENABLED) == 0U) {
        return E_OK;  /* Watchdog disabled, nothing to do */
    }

    /* Check for simulated failures */
    if (WdgIf_Emulator_ShouldFail(instance)) {
        state->missedTriggerCount++;
        return E_NOT_OK;
    }

    /* Check for timeout */
    if (state->currentCount >= state->timeoutValue) {
        state->flags |= WDGIF_EMU_FLAG_TIMEOUT_OCCURRED;
        state->missedTriggerCount++;
        return E_NOT_OK;
    }

    /* Check window mode violation */
    if ((state->flags & WDGIF_EMU_FLAG_WINDOW_MODE) != 0U) {
        if ((state->currentCount < state->windowStart) ||
            (state->currentCount > state->windowEnd)) {
            /* Window violation */
            return E_NOT_OK;
        }
    }

    /* Successful trigger - reset counter */
    state->currentCount = 0U;
    state->triggerCount++;
    state->flags |= WDGIF_EMU_FLAG_TRIGGERED;
    state->lastTriggerTime = state->elapsedTimeMs;

    return E_OK;
}

/**
 * @brief Get emulator version
 */
Std_ReturnType WdgIf_Emulator_GetVersion(uint16 *version)
{
    if (NULL_PTR == version) {
        return E_NOT_OK;
    }

    /* Version 1.0 for emulator */
    *version = 0x0100U;

    return E_OK;
}

/**
 * @brief Force reset via emulated watchdog
 */
Std_ReturnType WdgIf_Emulator_Reset(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return E_NOT_OK;
    }

    /* Set timeout flag and reset flag */
    WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_TIMEOUT_OCCURRED;
    WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_RESET_OCCURRED;

    /* In emulator, just mark as reset - don't actually reset */
    /* In real hardware, this would cause a system reset */

    return E_OK;
}

/**
 * @brief Get emulated watchdog status
 */
WdgIf_DriverStatusType WdgIf_Emulator_GetStatus(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;
    WdgIf_EmulatorStateType *state;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return WDGIF_STATUS_ERROR;
    }

    if (!WdgIf_EmulatorInitialized) {
        return WDGIF_STATUS_UNINIT;
    }

    state = &WdgIf_EmulatorState[instance];

    /* Check for timeout or reset */
    if ((state->flags & WDGIF_EMU_FLAG_TIMEOUT_OCCURRED) != 0U) {
        return WDGIF_STATUS_ERROR;
    }

    if ((state->flags & WDGIF_EMU_FLAG_ENABLED) == 0U) {
        return WDGIF_STATUS_IDLE;
    }

    if (state->isRunning) {
        return WDGIF_STATUS_BUSY;
    }

    return WDGIF_STATUS_IDLE;
}

/******************************************************************************
 * Emulator Control Functions
 ******************************************************************************/

/**
 * @brief Advance emulator timer
 */
void WdgIf_Emulator_AdvanceTime(WdgIf_DeviceType device, uint32 timeMs)
{
    uint8 instance = (uint8)device;
    WdgIf_EmulatorStateType *state;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return;
    }

    state = &WdgIf_EmulatorState[instance];

    if (!state->isRunning) {
        return;
    }

    /* Advance elapsed time */
    state->elapsedTimeMs += timeMs;

    /* Advance watchdog counter */
    state->currentCount += timeMs;

    /* Check for timeout */
    if (state->currentCount >= state->timeoutValue) {
        state->flags |= WDGIF_EMU_FLAG_TIMEOUT_OCCURRED;
    }
}

/**
 * @brief Check if emulator timeout occurred
 */
boolean WdgIf_Emulator_IsTimeoutOccurred(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return FALSE;
    }

    return ((WdgIf_EmulatorState[instance].flags &
             WDGIF_EMU_FLAG_TIMEOUT_OCCURRED) != 0U);
}

/**
 * @brief Check if emulator reset occurred
 */
boolean WdgIf_Emulator_IsResetOccurred(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return FALSE;
    }

    return ((WdgIf_EmulatorState[instance].flags &
             WDGIF_EMU_FLAG_RESET_OCCURRED) != 0U);
}

/**
 * @brief Get emulator state
 */
const WdgIf_EmulatorStateType* WdgIf_Emulator_GetState(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return NULL_PTR;
    }

    return &WdgIf_EmulatorState[instance];
}

/**
 * @brief Reset emulator state
 */
void WdgIf_Emulator_ResetState(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance < WDGIF_EMU_MAX_INSTANCES) {
        WdgIf_Emulator_ResetStateInternal(instance);
    }
}

/**
 * @brief Configure emulator behavior
 */
Std_ReturnType WdgIf_Emulator_Configure(
    WdgIf_DeviceType device,
    const WdgIf_EmulatorConfigType *emuConfig)
{
    uint8 instance = (uint8)device;

    if ((instance >= WDGIF_EMU_MAX_INSTANCES) || (NULL_PTR == emuConfig)) {
        return E_NOT_OK;
    }

    WdgIf_EmulatorConfigs[instance] = *emuConfig;

    return E_OK;
}

/**
 * @brief Simulate timeout condition
 */
void WdgIf_Emulator_SimulateTimeout(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance < WDGIF_EMU_MAX_INSTANCES) {
        WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_TIMEOUT_OCCURRED;
        WdgIf_EmulatorState[instance].currentCount =
            WdgIf_EmulatorState[instance].timeoutValue + 1U;
    }
}

/**
 * @brief Simulate window violation
 */
void WdgIf_Emulator_SimulateWindowViolation(WdgIf_DeviceType device)
{
    uint8 instance = (uint8)device;

    if (instance < WDGIF_EMU_MAX_INSTANCES) {
        WdgIf_EmulatorState[instance].flags |= WDGIF_EMU_FLAG_WINDOW_MODE;
        /* Set counter outside window */
        WdgIf_EmulatorState[instance].currentCount =
            WdgIf_EmulatorState[instance].windowEnd + 1U;
    }
}

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**
 * @brief Reset emulator state for instance
 */
static void WdgIf_Emulator_ResetStateInternal(uint8 instance)
{
    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return;
    }

    (void)memset(&WdgIf_EmulatorState[instance], 0,
                 sizeof(WdgIf_EmulatorStateType));
    (void)memset(&WdgIf_EmulatorConfigs[instance], 0,
                 sizeof(WdgIf_EmulatorConfigType));

    /* Reset to default configuration */
    WdgIf_EmulatorConfigs[instance] = WdgIf_EmulatorDefaultConfig;
}

/**
 * @brief Determine if trigger should fail (for testing)
 */
static boolean WdgIf_Emulator_ShouldFail(uint8 instance)
{
    static uint32 callCount = 0U;
    uint32 failureRate;

    if (instance >= WDGIF_EMU_MAX_INSTANCES) {
        return FALSE;
    }

    failureRate = WdgIf_EmulatorConfigs[instance].triggerFailureRate;

    if (failureRate == 0U) {
        return FALSE;
    }

    callCount++;

    /* Simple pseudo-random failure simulation */
    if ((callCount % (1000U / failureRate)) == 0U) {
        return TRUE;
    }

    /* Check for forced timeout simulation */
    if (WdgIf_EmulatorConfigs[instance].simulateTimeout) {
        return TRUE;
    }

    return FALSE;
}

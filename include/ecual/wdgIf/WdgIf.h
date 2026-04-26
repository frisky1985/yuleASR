/******************************************************************************
 * @file    WdgIf.h
 * @brief   Watchdog Interface (WdgIf) - AUTOSAR R22-11
 *
 * This module provides an abstraction layer for accessing the watchdog driver.
 * It supports multiple watchdog instances (Internal, External1, External2)
 * and provides a uniform interface to the upper layers (WdgM).
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_H
#define WDGIF_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define WDGIF_VENDOR_ID                 0x01U
#define WDGIF_MODULE_ID                 0x43U  /* WdgIf module ID per AUTOSAR */
#define WDGIF_SW_MAJOR_VERSION          1U
#define WDGIF_SW_MINOR_VERSION          0U
#define WDGIF_SW_PATCH_VERSION          0U

/******************************************************************************
 * API Service IDs
 ******************************************************************************/
#define WDGIF_SID_SETMODE               0x01U
#define WDGIF_SID_SETTRIGGERCONDITION   0x02U
#define WDGIF_SID_GETVERSIONINFO        0x03U
#define WDGIF_SID_INIT                  0x04U
#define WDGIF_SID_DEINIT                0x05U
#define WDGIF_SID_GETSTATUS             0x06U
#define WDGIF_SID_RESETPROCESSOR        0x07U

/******************************************************************************
 * Error Codes
 ******************************************************************************/
#define WDGIF_E_NOT_INITIALIZED         0x01U
#define WDGIF_E_INVALID_DEVICE          0x02U
#define WDGIF_E_INVALID_MODE            0x03U
#define WDGIF_E_NULL_POINTER            0x04U
#define WDGIF_E_TIMEOUT                 0x05U
#define WDGIF_E_TRIGGER_FAILED          0x06U
#define WDGIF_E_MODE_SWITCH_FAILED      0x07U
#define WDGIF_E_SAFETY_VIOLATION        0x08U

/******************************************************************************
 * Watchdog Device Types
 ******************************************************************************/
typedef uint8 WdgIf_DeviceType;

#define WDGIF_INTERNAL_WDG              0x00U  /* Internal MCU watchdog */
#define WDGIF_EXTERNAL_WDG1             0x01U  /* External watchdog 1 */
#define WDGIF_EXTERNAL_WDG2             0x02U  /* External watchdog 2 */
#define WDGIF_MAX_DEVICES               0x03U

#define WDGIF_DEVICE_IS_VALID(device)   ((device) < WDGIF_MAX_DEVICES)

/******************************************************************************
 * Watchdog Modes
 ******************************************************************************/
typedef uint8 WdgIf_ModeType;

#define WDGIF_OFF_MODE                  0x00U  /* Watchdog disabled */
#define WDGIF_SLOW_MODE                 0x01U  /* Slow mode (long timeout) */
#define WDGIF_FAST_MODE                 0x02U  /* Fast mode (short timeout) */

#define WDGIF_MODE_IS_VALID(mode)       ((mode) <= WDGIF_FAST_MODE)

/******************************************************************************
 * Watchdog Driver Status
 ******************************************************************************/
typedef enum {
    WDGIF_STATUS_UNINIT = 0,
    WDGIF_STATUS_IDLE,
    WDGIF_STATUS_BUSY,
    WDGIF_STATUS_ERROR
} WdgIf_DriverStatusType;

/******************************************************************************
 * Watchdog Device Status
 ******************************************************************************/
typedef struct {
    boolean initialized;
    WdgIf_ModeType currentMode;
    WdgIf_DriverStatusType driverStatus;
    uint32 triggerCount;
    uint32 errorCount;
    uint32 timeoutValue;        /* Current timeout in ms */
    uint32 lastTriggerTime;     /* Timestamp of last trigger */
    boolean triggerEnabled;
} WdgIf_DeviceStatusType;

/******************************************************************************
 * Hardware Platform Types
 ******************************************************************************/
typedef enum {
    WDGIF_HW_UNKNOWN = 0,
    WDGIF_HW_AURIX_TC3xx,
    WDGIF_HW_AURIX_TC4xx,
    WDGIF_HW_S32K3xx,
    WDGIF_HW_S32G3,
    WDGIF_HW_EMULATOR
} WdgIf_HardwareType;

/******************************************************************************
 * Timeout Configuration
 ******************************************************************************/
typedef struct {
    uint32 slowModeTimeout;     /* Timeout in ms for slow mode */
    uint32 fastModeTimeout;     /* Timeout in ms for fast mode */
    uint32 windowStartPercent;  /* Window start as percentage (0-100) */
    uint32 windowEndPercent;    /* Window end as percentage (0-100) */
} WdgIf_TimeoutConfigType;

/******************************************************************************
 * Safety Configuration
 ******************************************************************************/
typedef struct {
    boolean enableSafetyChecks;
    uint8 maxConsecutiveErrors;
    uint32 maxTriggerIntervalMs;
    boolean enableTimeWindowCheck;
    boolean enableResetOnFailure;
} WdgIf_SafetyConfigType;

/******************************************************************************
 * Dem Event Configuration
 ******************************************************************************/
typedef struct {
    boolean reportToDem;
    uint16 timeoutEventId;
    uint16 triggerFailEventId;
    uint16 modeFailEventId;
} WdgIf_DemConfigType;

/******************************************************************************
 * Device Configuration
 ******************************************************************************/
typedef struct {
    WdgIf_DeviceType deviceId;
    boolean enabled;
    WdgIf_HardwareType hardwareType;
    const WdgIf_TimeoutConfigType *timeoutConfig;
    const WdgIf_SafetyConfigType *safetyConfig;
    const WdgIf_DemConfigType *demConfig;
    void *hardwareConfig;       /* Platform-specific configuration */
} WdgIf_DeviceConfigType;

/******************************************************************************
 * Module Configuration
 ******************************************************************************/
typedef struct {
    uint8 numDevices;
    const WdgIf_DeviceConfigType *deviceConfigs[WDGIF_MAX_DEVICES];
    boolean enableVersionCheck;
    boolean enableMulticoreSupport;
} WdgIf_ConfigType;

/******************************************************************************
 * External Variables
 ******************************************************************************/
extern const WdgIf_ConfigType *WdgIf_ConfigPtr;
extern WdgIf_DeviceStatusType WdgIf_DeviceStatus[WDGIF_MAX_DEVICES];

/******************************************************************************
 * Core API Functions
 ******************************************************************************/

/**
 * @brief Initialize WdgIf module
 * @param config Pointer to module configuration
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_Init(const WdgIf_ConfigType *config);

/**
 * @brief Deinitialize WdgIf module
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_DeInit(void);

/**
 * @brief Set watchdog mode for specific device
 * @param device Watchdog device identifier
 * @param mode New watchdog mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_SetMode(
    WdgIf_DeviceType device,
    WdgIf_ModeType mode
);

/**
 * @brief Trigger watchdog for specific device
 * @param device Watchdog device identifier
 * @param timeout Timeout value in ms (0 = use default)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_SetTriggerCondition(
    WdgIf_DeviceType device,
    uint16 timeout
);

/**
 * @brief Get version information
 * @param versionInfo Pointer to store version info
 */
void WdgIf_GetVersionInfo(Std_VersionInfoType *versionInfo);

/******************************************************************************
 * Status and Control Functions
 ******************************************************************************/

/**
 * @brief Get device status
 * @param device Watchdog device identifier
 * @return Device status
 */
WdgIf_DriverStatusType WdgIf_GetDriverStatus(WdgIf_DeviceType device);

/**
 * @brief Check if device is initialized
 * @param device Watchdog device identifier
 * @return TRUE if initialized, FALSE otherwise
 */
boolean WdgIf_IsInitialized(WdgIf_DeviceType device);

/**
 * @brief Get current watchdog mode
 * @param device Watchdog device identifier
 * @return Current mode
 */
WdgIf_ModeType WdgIf_GetCurrentMode(WdgIf_DeviceType device);

/**
 * @brief Force processor reset via watchdog
 * @param device Watchdog device identifier
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_ResetProcessor(WdgIf_DeviceType device);

/******************************************************************************
 * Safety Functions
 ******************************************************************************/

/**
 * @brief Increment error counter for device
 * @param device Watchdog device identifier
 * @param errorType Type of error occurred
 */
void WdgIf_ReportError(WdgIf_DeviceType device, uint8 errorType);

/**
 * @brief Get error counter value
 * @param device Watchdog device identifier
 * @return Error counter value
 */
uint32 WdgIf_GetErrorCount(WdgIf_DeviceType device);

/**
 * @brief Clear error counter for device
 * @param device Watchdog device identifier
 */
void WdgIf_ClearErrorCount(WdgIf_DeviceType device);

/**
 * @brief Check if safety threshold exceeded
 * @param device Watchdog device identifier
 * @return TRUE if safety threshold exceeded
 */
boolean WdgIf_IsSafetyThresholdExceeded(WdgIf_DeviceType device);

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main function - called cyclically
 * Handles periodic checks and maintenance tasks
 */
void WdgIf_MainFunction(void);

/******************************************************************************
 * Callback Functions (to be implemented by application or WdgM)
 ******************************************************************************/

/**
 * @brief Notification callback for mode change
 * @param device Watchdog device identifier
 * @param newMode New watchdog mode
 */
extern void WdgIf_ModeChangeNotification(
    WdgIf_DeviceType device,
    WdgIf_ModeType newMode
);

/**
 * @brief Notification callback for trigger failure
 * @param device Watchdog device identifier
 * @param errorCode Error code
 */
extern void WdgIf_TriggerFailureNotification(
    WdgIf_DeviceType device,
    uint8 errorCode
);

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_H */

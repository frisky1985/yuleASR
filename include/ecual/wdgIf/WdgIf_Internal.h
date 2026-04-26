/******************************************************************************
 * @file    WdgIf_Internal.h
 * @brief   Watchdog Interface Internal Definitions
 *
 * Internal types and functions for WdgIf module implementation.
 * Not to be included by other modules.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_INTERNAL_H
#define WDGIF_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf.h"

/* Define NULL_PTR if not defined */
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

/******************************************************************************
 * Internal Constants
 ******************************************************************************/

/* Magic numbers for safety checks */
#define WDGIF_MAGIC_INITIALIZED         0x57444749U  /* "WDGI" */
#define WDGIF_MAGIC_UNINITIALIZED       0x00000000U

/* Safety thresholds */
#define WDGIF_DEFAULT_MAX_ERRORS        3U
#define WDGIF_MAX_TRIGGER_INTERVAL_MS   100U

/* Time window defaults (percentage of timeout) */
#define WDGIF_WINDOW_START_DEFAULT      25U
#define WDGIF_WINDOW_END_DEFAULT        75U

/* Error types for reporting */
#define WDGIF_ERROR_NONE                0x00U
#define WDGIF_ERROR_TIMEOUT             0x01U
#define WDGIF_ERROR_TRIGGER_FAIL        0x02U
#define WDGIF_ERROR_MODE_SWITCH_FAIL    0x03U
#define WDGIF_ERROR_INVALID_PARAMETER   0x04U
#define WDGIF_ERROR_HARDWARE_FAIL       0x05U

/******************************************************************************
 * Internal Types
 ******************************************************************************/

/* Internal module state */
typedef struct {
    uint32 magicNumber;
    boolean initialized;
    uint32 activeDevices;
    uint32 totalTriggerCount;
    uint32 totalErrorCount;
} WdgIf_InternalStateType;

/* Hardware interface operations */
typedef struct {
    Std_ReturnType (*init)(const WdgIf_DeviceConfigType *config);
    Std_ReturnType (*deinit)(WdgIf_DeviceType device);
    Std_ReturnType (*setMode)(WdgIf_DeviceType device, WdgIf_ModeType mode);
    Std_ReturnType (*trigger)(WdgIf_DeviceType device);
    Std_ReturnType (*getVersion)(uint16 *version);
    Std_ReturnType (*reset)(WdgIf_DeviceType device);
    WdgIf_DriverStatusType (*getStatus)(WdgIf_DeviceType device);
} WdgIf_HwInterfaceType;

/* Platform-specific context */
typedef struct {
    void *hwContext;
    uint32 lastTriggerTimestamp;
    uint32 triggerWindowStart;
    uint32 triggerWindowEnd;
    boolean inTriggerWindow;
} WdgIf_DeviceContextType;

/******************************************************************************
 * Internal Variables
 ******************************************************************************/
extern WdgIf_InternalStateType WdgIf_InternalState;
extern WdgIf_DeviceContextType WdgIf_DeviceContext[WDGIF_MAX_DEVICES];
extern const WdgIf_HwInterfaceType *WdgIf_HwInterfaces[WDGIF_MAX_DEVICES];

/******************************************************************************
 * Internal Functions
 ******************************************************************************/

/**
 * @brief Validate device index
 * @param device Device index to validate
 * @return TRUE if valid, FALSE otherwise
 */
static inline boolean WdgIf_IsDeviceValid(WdgIf_DeviceType device)
{
    return ((device) < WDGIF_MAX_DEVICES);
}

/**
 * @brief Check if module is initialized
 * @return TRUE if initialized, FALSE otherwise
 */
static inline boolean WdgIf_IsModuleInitialized(void)
{
    return (WdgIf_InternalState.magicNumber == WDGIF_MAGIC_INITIALIZED);
}

/**
 * @brief Get timeout value for mode
 * @param device Device index
 * @param mode Watchdog mode
 * @return Timeout value in ms
 */
uint32 WdgIf_GetTimeoutForMode(WdgIf_DeviceType device, WdgIf_ModeType mode);

/**
 * @brief Validate timeout value
 * @param device Device index
 * @param timeout Timeout value to validate
 * @return TRUE if valid, FALSE otherwise
 */
boolean WdgIf_ValidateTimeout(WdgIf_DeviceType device, uint32 timeout);

/**
 * @brief Check if trigger is within valid time window
 * @param device Device index
 * @return TRUE if within window, FALSE otherwise
 */
boolean WdgIf_IsInTriggerWindow(WdgIf_DeviceType device);

/**
 * @brief Update trigger timestamp
 * @param device Device index
 */
void WdgIf_UpdateTriggerTimestamp(WdgIf_DeviceType device);

/**
 * @brief Report error to Dem
 * @param device Device index
 * @param errorType Error type
 */
void WdgIf_ReportDemEvent(WdgIf_DeviceType device, uint8 errorType);

/**
 * @brief Get hardware interface for device
 * @param device Device index
 * @return Pointer to hardware interface
 */
const WdgIf_HwInterfaceType* WdgIf_GetHwInterface(WdgIf_DeviceType device);

/**
 * @brief Initialize hardware interface
 * @param device Device index
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_InitHwInterface(WdgIf_DeviceType device);

/**
 * @brief Perform safety checks
 * @param device Device index
 * @return E_OK if checks passed
 */
Std_ReturnType WdgIf_PerformSafetyChecks(WdgIf_DeviceType device);

/**
 * @brief Handle safety violation
 * @param device Device index
 * @param violationType Type of violation
 */
void WdgIf_HandleSafetyViolation(WdgIf_DeviceType device, uint8 violationType);

/******************************************************************************
 * Safety Macros
 ******************************************************************************/

/* Development error check */
#if defined(WDGIF_DEV_ERROR_DETECT) && (WDGIF_DEV_ERROR_DETECT == STD_ON)
    #define WDGIF_DET_REPORT_ERROR(apiId, errorId) \
        WdgIf_ReportDetError((apiId), (errorId))
#else
    #define WDGIF_DET_REPORT_ERROR(apiId, errorId) ((void)0)
#endif

/* Safety check macro */
#define WDGIF_SAFETY_CHECK(condition, apiId, errorId) \
    do { \
        if (!(condition)) { \
            WDGIF_DET_REPORT_ERROR((apiId), (errorId)); \
            return E_NOT_OK; \
        } \
    } while (0)

/* Null pointer check */
#define WDGIF_CHECK_NULL_POINTER(ptr, apiId) \
    WDGIF_SAFETY_CHECK(((ptr) != NULL_PTR), (apiId), WDGIF_E_NULL_POINTER)

/* Device validity check */
#define WDGIF_CHECK_DEVICE(device, apiId) \
    WDGIF_SAFETY_CHECK(WdgIf_IsDeviceValid(device), (apiId), WDGIF_E_INVALID_DEVICE)

/* Module initialization check */
#define WDGIF_CHECK_INITIALIZED(apiId) \
    WDGIF_SAFETY_CHECK(WdgIf_IsModuleInitialized(), (apiId), WDGIF_E_NOT_INITIALIZED)

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_INTERNAL_H */

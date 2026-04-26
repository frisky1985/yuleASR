/******************************************************************************
 * @file    WdgIf.c
 * @brief   Watchdog Interface (WdgIf) - Core Implementation
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
#include "ecual/wdgIf/WdgIf_Internal.h"
#include "ecual/wdgIf/WdgIf_Hw_Aurix.h"
#include "ecual/wdgIf/WdgIf_Hw_S32K3.h"
#include "ecual/wdgIf/WdgIf_Hw_Emulator.h"
#include <string.h>

/* Define NULL_PTR if not defined */
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
/* Version information now comes from WdgIf_Cfg.h */
#include "ecual/WdgIf/WdgIf_Cfg.h"
#define WDGIF_SW_VERSION_MAJOR          WDGIF_CFG_SW_MAJOR_VERSION
#define WDGIF_SW_VERSION_MINOR          WDGIF_CFG_SW_MINOR_VERSION
#define WDGIF_SW_VERSION_PATCH          WDGIF_CFG_SW_PATCH_VERSION
#define WDGIF_VENDOR_ID_VALUE           WDGIF_CFG_VENDOR_ID
#define WDGIF_MODULE_ID_VALUE           WDGIF_CFG_MODULE_ID

/******************************************************************************
 * Internal Variables
 ******************************************************************************/
/* Module configuration pointer */
const WdgIf_ConfigType *WdgIf_ConfigPtr = NULL_PTR;

/* Device status array */
WdgIf_DeviceStatusType WdgIf_DeviceStatus[WDGIF_MAX_DEVICES];

/* Internal state */
WdgIf_InternalStateType WdgIf_InternalState;

/* Device context array */
WdgIf_DeviceContextType WdgIf_DeviceContext[WDGIF_MAX_DEVICES];

/* Hardware interface pointers */
const WdgIf_HwInterfaceType *WdgIf_HwInterfaces[WDGIF_MAX_DEVICES];

/* Default timeout configuration */
static const WdgIf_TimeoutConfigType WdgIf_DefaultTimeoutConfig = {
    .slowModeTimeout = 500U,      /* 500ms */
    .fastModeTimeout = 50U,       /* 50ms */
    .windowStartPercent = WDGIF_WINDOW_START_DEFAULT,
    .windowEndPercent = WDGIF_WINDOW_END_DEFAULT
};

/* Default safety configuration */
__attribute__((unused))
static const WdgIf_SafetyConfigType WdgIf_DefaultSafetyConfig = {
    .enableSafetyChecks = TRUE,
    .maxConsecutiveErrors = WDGIF_DEFAULT_MAX_ERRORS,
    .maxTriggerIntervalMs = WDGIF_MAX_TRIGGER_INTERVAL_MS,
    .enableTimeWindowCheck = FALSE,
    .enableResetOnFailure = FALSE
};

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static Std_ReturnType WdgIf_InitDevice(WdgIf_DeviceType device);
static void WdgIf_DeInitDevice(WdgIf_DeviceType device);
static const WdgIf_HwInterfaceType* WdgIf_GetHardwareInterface(
    WdgIf_HardwareType hwType
);
static void WdgIf_ReportDemEventInternal(WdgIf_DeviceType device, uint8 errorType);

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initialize WdgIf module
 */
Std_ReturnType WdgIf_Init(const WdgIf_ConfigType *config)
{
    uint8 i;
    Std_ReturnType result = E_OK;

    /* Check for NULL pointer */
    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Check if already initialized */
    if (WdgIf_IsModuleInitialized()) {
        WDGIF_DET_REPORT_ERROR(WDGIF_SID_INIT, WDGIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    /* Initialize internal state */
    (void)memset(&WdgIf_InternalState, 0, sizeof(WdgIf_InternalState));
    (void)memset(WdgIf_DeviceStatus, 0, sizeof(WdgIf_DeviceStatus));
    (void)memset(WdgIf_DeviceContext, 0, sizeof(WdgIf_DeviceContext));
    (void)memset(WdgIf_HwInterfaces, 0, sizeof(WdgIf_HwInterfaces));

    /* Store configuration pointer */
    WdgIf_ConfigPtr = config;

    /* Initialize each enabled device */
    for (i = 0U; i < WDGIF_MAX_DEVICES; i++) {
        if ((i < config->numDevices) && 
            (NULL_PTR != config->deviceConfigs[i]) &&
            (config->deviceConfigs[i]->enabled)) {
            
            result = WdgIf_InitDevice((WdgIf_DeviceType)i);
            
            if (E_OK == result) {
                WdgIf_InternalState.activeDevices |= (1U << i);
            }
        }
    }

    /* Mark module as initialized */
    if (WdgIf_InternalState.activeDevices > 0U) {
        WdgIf_InternalState.magicNumber = WDGIF_MAGIC_INITIALIZED;
        WdgIf_InternalState.initialized = TRUE;
    } else {
        result = E_NOT_OK;
    }

    return result;
}

/**
 * @brief Deinitialize WdgIf module
 */
Std_ReturnType WdgIf_DeInit(void)
{
    uint8 i;

    /* Check if initialized */
    if (!WdgIf_IsModuleInitialized()) {
        WDGIF_DET_REPORT_ERROR(WDGIF_SID_DEINIT, WDGIF_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    /* Deinitialize all active devices */
    for (i = 0U; i < WDGIF_MAX_DEVICES; i++) {
        if ((WdgIf_InternalState.activeDevices & (1U << i)) != 0U) {
            WdgIf_DeInitDevice((WdgIf_DeviceType)i);
        }
    }

    /* Clear internal state */
    (void)memset(&WdgIf_InternalState, 0, sizeof(WdgIf_InternalState));
    (void)memset(WdgIf_DeviceStatus, 0, sizeof(WdgIf_DeviceStatus));
    (void)memset(WdgIf_DeviceContext, 0, sizeof(WdgIf_DeviceContext));
    
    WdgIf_ConfigPtr = NULL_PTR;

    return E_OK;
}

/**
 * @brief Set watchdog mode for specific device
 */
Std_ReturnType WdgIf_SetMode(WdgIf_DeviceType device, WdgIf_ModeType mode)
{
    Std_ReturnType result = E_NOT_OK;
    const WdgIf_DeviceConfigType *deviceConfig;

    /* Parameter checks */
    WDGIF_CHECK_INITIALIZED(WDGIF_SID_SETMODE);
    WDGIF_CHECK_DEVICE(device, WDGIF_SID_SETMODE);

    if (!WDGIF_MODE_IS_VALID(mode)) {
        WDGIF_DET_REPORT_ERROR(WDGIF_SID_SETMODE, WDGIF_E_INVALID_MODE);
        WdgIf_ReportDemEventInternal(device, WDGIF_ERROR_INVALID_PARAMETER);
        return E_NOT_OK;
    }

    /* Check if device is active */
    if ((WdgIf_InternalState.activeDevices & (1U << device)) == 0U) {
        WDGIF_DET_REPORT_ERROR(WDGIF_SID_SETMODE, WDGIF_E_INVALID_DEVICE);
        return E_NOT_OK;
    }

    deviceConfig = WdgIf_ConfigPtr->deviceConfigs[device];

    /* Safety check: prevent disabling watchdog in safety-critical modes */
    if ((WDGIF_OFF_MODE == mode) && 
        (NULL_PTR != deviceConfig->safetyConfig) &&
        (deviceConfig->safetyConfig->enableSafetyChecks)) {
        /* Log warning but allow if explicitly configured */
        WdgIf_ReportDemEventInternal(device, WDGIF_ERROR_MODE_SWITCH_FAIL);
    }

    /* Call hardware-specific mode change */
    if ((NULL_PTR != WdgIf_HwInterfaces[device]) &&
        (NULL_PTR != WdgIf_HwInterfaces[device]->setMode)) {
        result = WdgIf_HwInterfaces[device]->setMode(device, mode);
    }

    if (E_OK == result) {
        /* Update device status */
        WdgIf_DeviceStatus[device].currentMode = mode;
        WdgIf_DeviceStatus[device].timeoutValue = 
            WdgIf_GetTimeoutForMode(device, mode);
        
        /* Notify application */
        WdgIf_ModeChangeNotification(device, mode);
    } else {
        /* Report error */
        WdgIf_ReportError(device, WDGIF_ERROR_MODE_SWITCH_FAIL);
        WdgIf_ReportDemEventInternal(device, WDGIF_ERROR_MODE_SWITCH_FAIL);
    }

    return result;
}

/**
 * @brief Trigger watchdog for specific device
 */
Std_ReturnType WdgIf_SetTriggerCondition(WdgIf_DeviceType device, uint16 timeout)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 timeoutMs;

    /* Parameter checks */
    WDGIF_CHECK_INITIALIZED(WDGIF_SID_SETTRIGGERCONDITION);
    WDGIF_CHECK_DEVICE(device, WDGIF_SID_SETTRIGGERCONDITION);

    /* Check if device is active */
    if ((WdgIf_InternalState.activeDevices & (1U << device)) == 0U) {
        WDGIF_DET_REPORT_ERROR(WDGIF_SID_SETTRIGGERCONDITION, 
                               WDGIF_E_INVALID_DEVICE);
        return E_NOT_OK;
    }

    /* Check if watchdog is enabled */
    if (WDGIF_OFF_MODE == WdgIf_DeviceStatus[device].currentMode) {
        /* Watchdog disabled, nothing to do */
        return E_OK;
    }

    /* Determine timeout value */
    if (0U == timeout) {
        /* Use current configured timeout */
        timeoutMs = WdgIf_DeviceStatus[device].timeoutValue;
    } else {
        timeoutMs = (uint32)timeout;
    }

    /* Validate timeout */
    if (!WdgIf_ValidateTimeout(device, timeoutMs)) {
        WDGIF_DET_REPORT_ERROR(WDGIF_SID_SETTRIGGERCONDITION, 
                               WDGIF_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }

    /* Check trigger window if enabled */
    if ((NULL_PTR != WdgIf_ConfigPtr->deviceConfigs[device]) &&
        (NULL_PTR != WdgIf_ConfigPtr->deviceConfigs[device]->safetyConfig) &&
        (WdgIf_ConfigPtr->deviceConfigs[device]->safetyConfig->enableTimeWindowCheck)) {
        if (!WdgIf_IsInTriggerWindow(device)) {
            WdgIf_ReportError(device, WDGIF_ERROR_TRIGGER_FAIL);
            WdgIf_ReportDemEventInternal(device, WDGIF_ERROR_TRIGGER_FAIL);
            return E_NOT_OK;
        }
    }

    /* Call hardware-specific trigger */
    if ((NULL_PTR != WdgIf_HwInterfaces[device]) &&
        (NULL_PTR != WdgIf_HwInterfaces[device]->trigger)) {
        result = WdgIf_HwInterfaces[device]->trigger(device);
    }

    if (E_OK == result) {
        /* Update status */
        WdgIf_DeviceStatus[device].triggerCount++;
        WdgIf_InternalState.totalTriggerCount++;
        WdgIf_UpdateTriggerTimestamp(device);
        
        /* Clear error count on successful trigger */
        WdgIf_DeviceStatus[device].errorCount = 0U;
    } else {
        /* Report error */
        WdgIf_ReportError(device, WDGIF_ERROR_TRIGGER_FAIL);
        WdgIf_ReportDemEventInternal(device, WDGIF_ERROR_TRIGGER_FAIL);
    }

    return result;
}

/**
 * @brief Get version information
 */
void WdgIf_GetVersionInfo(Std_VersionInfoType *versionInfo)
{
    if (NULL_PTR != versionInfo) {
        versionInfo->vendorID = WDGIF_VENDOR_ID_VALUE;
        versionInfo->moduleID = WDGIF_MODULE_ID_VALUE;
        versionInfo->sw_major_version = WDGIF_SW_VERSION_MAJOR;
        versionInfo->sw_minor_version = WDGIF_SW_VERSION_MINOR;
        versionInfo->sw_patch_version = WDGIF_SW_VERSION_PATCH;
    }
}

/******************************************************************************
 * Status and Control Functions
 ******************************************************************************/

/**
 * @brief Get device status
 */
WdgIf_DriverStatusType WdgIf_GetDriverStatus(WdgIf_DeviceType device)
{
    if (!WdgIf_IsDeviceValid(device)) {
        return WDGIF_STATUS_ERROR;
    }

    if (!WdgIf_IsModuleInitialized()) {
        return WDGIF_STATUS_UNINIT;
    }

    return WdgIf_DeviceStatus[device].driverStatus;
}

/**
 * @brief Check if device is initialized
 */
boolean WdgIf_IsInitialized(WdgIf_DeviceType device)
{
    if (!WdgIf_IsDeviceValid(device)) {
        return FALSE;
    }

    return WdgIf_DeviceStatus[device].initialized;
}

/**
 * @brief Get current watchdog mode
 */
WdgIf_ModeType WdgIf_GetCurrentMode(WdgIf_DeviceType device)
{
    if ((!WdgIf_IsDeviceValid(device)) || (!WdgIf_IsModuleInitialized())) {
        return WDGIF_OFF_MODE;
    }

    return WdgIf_DeviceStatus[device].currentMode;
}

/**
 * @brief Force processor reset via watchdog
 */
Std_ReturnType WdgIf_ResetProcessor(WdgIf_DeviceType device)
{
    WDGIF_CHECK_INITIALIZED(WDGIF_SID_RESETPROCESSOR);
    WDGIF_CHECK_DEVICE(device, WDGIF_SID_RESETPROCESSOR);

    if ((NULL_PTR != WdgIf_HwInterfaces[device]) &&
        (NULL_PTR != WdgIf_HwInterfaces[device]->reset)) {
        return WdgIf_HwInterfaces[device]->reset(device);
    }

    return E_NOT_OK;
}

/******************************************************************************
 * Safety Functions
 ******************************************************************************/

/**
 * @brief Increment error counter for device
 */
void WdgIf_ReportError(WdgIf_DeviceType device, uint8 errorType)
{
    if (!WdgIf_IsDeviceValid(device)) {
        return;
    }

    WdgIf_DeviceStatus[device].errorCount++;
    WdgIf_InternalState.totalErrorCount++;

    /* Check if safety threshold exceeded */
    if (WdgIf_IsSafetyThresholdExceeded(device)) {
        WdgIf_HandleSafetyViolation(device, errorType);
    }
}

/**
 * @brief Get error counter value
 */
uint32 WdgIf_GetErrorCount(WdgIf_DeviceType device)
{
    if (!WdgIf_IsDeviceValid(device)) {
        return 0U;
    }

    return WdgIf_DeviceStatus[device].errorCount;
}

/**
 * @brief Clear error counter for device
 */
void WdgIf_ClearErrorCount(WdgIf_DeviceType device)
{
    if (WdgIf_IsDeviceValid(device)) {
        WdgIf_DeviceStatus[device].errorCount = 0U;
    }
}

/**
 * @brief Check if safety threshold exceeded
 */
boolean WdgIf_IsSafetyThresholdExceeded(WdgIf_DeviceType device)
{
    const WdgIf_SafetyConfigType *safetyConfig;
    uint8 maxErrors;

    if (!WdgIf_IsDeviceValid(device)) {
        return FALSE;
    }

    /* Get max errors from config or use default */
    if ((NULL_PTR != WdgIf_ConfigPtr) &&
        (NULL_PTR != WdgIf_ConfigPtr->deviceConfigs[device]) &&
        (NULL_PTR != WdgIf_ConfigPtr->deviceConfigs[device]->safetyConfig)) {
        safetyConfig = WdgIf_ConfigPtr->deviceConfigs[device]->safetyConfig;
        maxErrors = safetyConfig->maxConsecutiveErrors;
    } else {
        maxErrors = WDGIF_DEFAULT_MAX_ERRORS;
    }

    return (WdgIf_DeviceStatus[device].errorCount >= maxErrors);
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

/**
 * @brief Main function - called cyclically
 */
void WdgIf_MainFunction(void)
{
    uint8 i;

    if (!WdgIf_IsModuleInitialized()) {
        return;
    }

    for (i = 0U; i < WDGIF_MAX_DEVICES; i++) {
        if ((WdgIf_InternalState.activeDevices & (1U << i)) != 0U) {
            /* Perform safety checks */
            (void)WdgIf_PerformSafetyChecks((WdgIf_DeviceType)i);
        }
    }
}

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**
 * @brief Initialize a specific device
 */
static Std_ReturnType WdgIf_InitDevice(WdgIf_DeviceType device)
{
    Std_ReturnType result = E_NOT_OK;
    const WdgIf_DeviceConfigType *config;
    const WdgIf_HwInterfaceType *hwInterface;

    if (!WdgIf_IsDeviceValid(device)) {
        return E_NOT_OK;
    }

    if (NULL_PTR == WdgIf_ConfigPtr) {
        return E_NOT_OK;
    }

    config = WdgIf_ConfigPtr->deviceConfigs[device];
    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Get hardware interface based on hardware type */
    hwInterface = WdgIf_GetHardwareInterface(config->hardwareType);
    if (NULL_PTR == hwInterface) {
        return E_NOT_OK;
    }

    WdgIf_HwInterfaces[device] = hwInterface;

    /* Initialize hardware */
    if (NULL_PTR != hwInterface->init) {
        result = hwInterface->init(config);
    }

    if (E_OK == result) {
        /* Initialize device status */
        WdgIf_DeviceStatus[device].initialized = TRUE;
        WdgIf_DeviceStatus[device].currentMode = WDGIF_OFF_MODE;
        WdgIf_DeviceStatus[device].driverStatus = WDGIF_STATUS_IDLE;
        WdgIf_DeviceStatus[device].triggerCount = 0U;
        WdgIf_DeviceStatus[device].errorCount = 0U;

        /* Set default timeout from configuration */
        if (NULL_PTR != config->timeoutConfig) {
            WdgIf_DeviceStatus[device].timeoutValue = 
                config->timeoutConfig->slowModeTimeout;
        } else {
            WdgIf_DeviceStatus[device].timeoutValue = 
                WdgIf_DefaultTimeoutConfig.slowModeTimeout;
        }

        WdgIf_DeviceStatus[device].triggerEnabled = TRUE;
    }

    return result;
}

/**
 * @brief Deinitialize a specific device
 */
static void WdgIf_DeInitDevice(WdgIf_DeviceType device)
{
    if (!WdgIf_IsDeviceValid(device)) {
        return;
    }

    if ((NULL_PTR != WdgIf_HwInterfaces[device]) &&
        (NULL_PTR != WdgIf_HwInterfaces[device]->deinit)) {
        (void)WdgIf_HwInterfaces[device]->deinit(device);
    }

    /* Clear device status */
    (void)memset(&WdgIf_DeviceStatus[device], 0, 
                 sizeof(WdgIf_DeviceStatusType));
    WdgIf_HwInterfaces[device] = NULL_PTR;
}

/**
 * @brief Get hardware interface based on hardware type
 */
static const WdgIf_HwInterfaceType* WdgIf_GetHardwareInterface(
    WdgIf_HardwareType hwType)
{
    const WdgIf_HwInterfaceType *interface = NULL_PTR;

    switch (hwType) {
        case WDGIF_HW_AURIX_TC3xx:
        case WDGIF_HW_AURIX_TC4xx:
            interface = &WdgIf_AurixHwInterface;
            break;

        case WDGIF_HW_S32K3xx:
        case WDGIF_HW_S32G3:
            interface = &WdgIf_S32K3HwInterface;
            break;

        case WDGIF_HW_EMULATOR:
            interface = &WdgIf_EmulatorHwInterface;
            break;

        case WDGIF_HW_UNKNOWN:
        default:
            interface = NULL_PTR;
            break;
    }

    return interface;
}

/**
 * @brief Report DEM event (stub - integrate with actual Dem)
 */
static void WdgIf_ReportDemEventInternal(WdgIf_DeviceType device, 
                                          uint8 errorType)
{
    const WdgIf_DemConfigType *demConfig;

    /* Suppress unused parameter warnings */
    (void)device;
    (void)errorType;

    if (NULL_PTR == WdgIf_ConfigPtr) {
        return;
    }

    if (device < WdgIf_ConfigPtr->numDevices) {
        demConfig = WdgIf_ConfigPtr->deviceConfigs[device]->demConfig;
        
        if ((NULL_PTR != demConfig) && (demConfig->reportToDem)) {
            /* TODO: Integrate with actual Dem_ReportErrorStatus() */
            /* Dem_ReportErrorStatus(eventId, DEM_EVENT_STATUS_FAILED); */
        }
    }
}

/******************************************************************************
 * Weak Default Callbacks (can be overridden by application)
 ******************************************************************************/

/**
 * @brief Default mode change notification (weak)
 */
__attribute__((weak)) void WdgIf_ModeChangeNotification(
    WdgIf_DeviceType device,
    WdgIf_ModeType newMode)
{
    /* Default: do nothing */
    (void)device;
    (void)newMode;
}

/**
 * @brief Default trigger failure notification (weak)
 */
__attribute__((weak)) void WdgIf_TriggerFailureNotification(
    WdgIf_DeviceType device,
    uint8 errorCode)
{
    /* Default: do nothing */
    (void)device;
    (void)errorCode;
}

/******************************************************************************
 * Internal Functions Implementation
 ******************************************************************************/

/**
 * @brief Get timeout value for mode
 */
uint32 WdgIf_GetTimeoutForMode(WdgIf_DeviceType device, WdgIf_ModeType mode)
{
    const WdgIf_TimeoutConfigType *timeoutConfig;
    uint32 timeout = 100U;  /* Default 100ms */

    if ((!WdgIf_IsDeviceValid(device)) || (NULL_PTR == WdgIf_ConfigPtr)) {
        return timeout;
    }

    if (device < WdgIf_ConfigPtr->numDevices) {
        if (NULL_PTR != WdgIf_ConfigPtr->deviceConfigs[device]->timeoutConfig) {
            timeoutConfig = WdgIf_ConfigPtr->deviceConfigs[device]->timeoutConfig;
        } else {
            timeoutConfig = &WdgIf_DefaultTimeoutConfig;
        }

        switch (mode) {
            case WDGIF_SLOW_MODE:
                timeout = timeoutConfig->slowModeTimeout;
                break;

            case WDGIF_FAST_MODE:
                timeout = timeoutConfig->fastModeTimeout;
                break;

            case WDGIF_OFF_MODE:
            default:
                timeout = 0U;
                break;
        }
    }

    return timeout;
}

/**
 * @brief Validate timeout value
 */
boolean WdgIf_ValidateTimeout(WdgIf_DeviceType device, uint32 timeout)
{
    /* Suppress unused parameter */
    (void)device;

    /* Check reasonable bounds */
    if ((timeout < WDGIF_EMU_MIN_TIMEOUT) || 
        (timeout > WDGIF_EMU_MAX_TIMEOUT)) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Check if trigger is within valid time window
 */
boolean WdgIf_IsInTriggerWindow(WdgIf_DeviceType device)
{
    /* TODO: Implement actual time window check */
    /* For now, always return TRUE */
    (void)device;
    return TRUE;
}

/**
 * @brief Update trigger timestamp
 */
void WdgIf_UpdateTriggerTimestamp(WdgIf_DeviceType device)
{
    if (WdgIf_IsDeviceValid(device)) {
        /* TODO: Use actual OS timestamp */
        WdgIf_DeviceStatus[device].lastTriggerTime++;
    }
}

/**
 * @brief Handle safety violation
 */
void WdgIf_HandleSafetyViolation(WdgIf_DeviceType device, uint8 violationType)
{
    const WdgIf_SafetyConfigType *safetyConfig;

    WdgIf_TriggerFailureNotification(device, violationType);

    if ((NULL_PTR != WdgIf_ConfigPtr) &&
        (device < WdgIf_ConfigPtr->numDevices)) {
        
        safetyConfig = WdgIf_ConfigPtr->deviceConfigs[device]->safetyConfig;
        
        if ((NULL_PTR != safetyConfig) && (safetyConfig->enableResetOnFailure)) {
            /* Force reset via watchdog */
            (void)WdgIf_ResetProcessor(device);
        }
    }
}

/**
 * @brief Perform safety checks
 */
Std_ReturnType WdgIf_PerformSafetyChecks(WdgIf_DeviceType device)
{
    /* TODO: Implement comprehensive safety checks */
    (void)device;
    return E_OK;
}

#if defined(WDGIF_DEV_ERROR_DETECT) && (WDGIF_DEV_ERROR_DETECT == STD_ON)
/**
 * @brief Report DET error to Development Error Tracer
 */
void WdgIf_ReportDetError(uint8 apiId, uint8 errorId)
{
    (void)Det_ReportError(
        WDGIF_MODULE_ID,    /* Module ID */
        0U,                 /* Instance ID */
        apiId,              /* API ID */
        errorId             /* Error ID */
    );
}
#endif

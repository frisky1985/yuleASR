/** @file WdgIf.c
 * @brief Watchdog Interface implementation
 * 
 * AUTOSAR R22-11 compliant WdgIf module
 * ECUAL layer - Watchdog Driver Interface
 */

/*============================================================================
 *  INCLUDES
 *===========================================================================*/
#include "WdgIf.h"
#include "WdgIf_Cfg.h"

#if (WDGIF_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*============================================================================
 *  VERSION CHECK
 *===========================================================================*/
#define WDGIF_SW_MAJOR_VERSION_CHECK        1
#define WDGIF_SW_MINOR_VERSION_CHECK        0
#define WDGIF_SW_PATCH_VERSION_CHECK        0

#if (WDGIF_SW_MAJOR_VERSION != WDGIF_SW_MAJOR_VERSION_CHECK)
    #error "WdgIf: Software major version mismatch"
#endif

#if (WDGIF_SW_MINOR_VERSION != WDGIF_SW_MINOR_VERSION_CHECK)
    #error "WdgIf: Software minor version mismatch"
#endif

/*============================================================================
 *  INTERNAL STATE
 *===========================================================================*/

/** @brief Module initialization state */
static WdgIf_StatusType WdgIf_Status = WDGIF_UNINIT;

/** @brief Pointer to configuration structure */
static const WdgIf_ConfigType* WdgIf_ConfigPtr = NULL;

/** @brief Current mode for each device */
static WdgIf_ModeType WdgIf_CurrentMode[WDGIF_NUMBER_OF_DEVICES];

/** @brief Device initialized flags */
static boolean WdgIf_DeviceInitialized[WDGIF_NUMBER_OF_DEVICES];

/*============================================================================
 *  INTERNAL FUNCTIONS
 *===========================================================================*/

/**
 * @brief Validate device index
 */
static inline boolean WdgIf_IsValidDevice(WdgIf_DeviceType Device)
{
    return (Device < WDGIF_NUMBER_OF_DEVICES);
}

/**
 * @brief Validate mode
 */
static inline boolean WdgIf_IsValidMode(WdgIf_ModeType Mode)
{
    return (Mode <= WDGIF_FAST_MODE);
}

/*============================================================================
 *  API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize WdgIf module
 * SWS_WdgIf_00001
 */
void WdgIf_Init(const WdgIf_ConfigType* ConfigPtr)
{
    uint8 i;
    
    #if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, 0, WDGIF_SID_INIT, WDGIF_E_INV_POINTER);
        return;
    }
    
    if (ConfigPtr->DeviceConfig == NULL)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, 0, WDGIF_SID_INIT, WDGIF_E_INV_POINTER);
        return;
    }
    #endif
    
    /* Store configuration */
    WdgIf_ConfigPtr = ConfigPtr;
    
    /* Initialize device states */
    for (i = 0; i < WDGIF_NUMBER_OF_DEVICES; i++)
    {
        WdgIf_CurrentMode[i] = WDGIF_OFF_MODE;
        WdgIf_DeviceInitialized[i] = FALSE;
    }
    
    /* Mark module as initialized */
    WdgIf_Status = WDGIF_IDLE;
}

/**
 * @brief Deinitialize WdgIf module
 * SWS_WdgIf_00002
 */
void WdgIf_DeInit(void)
{
    uint8 i;
    
    #if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_Status == WDGIF_UNINIT)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, 0, WDGIF_SID_DEINIT, WDGIF_E_DRIVER_UNINIT);
        return;
    }
    #endif
    
    /* Reset all device states */
    for (i = 0; i < WDGIF_NUMBER_OF_DEVICES; i++)
    {
        WdgIf_CurrentMode[i] = WDGIF_OFF_MODE;
        WdgIf_DeviceInitialized[i] = FALSE;
    }
    
    /* Clear configuration */
    WdgIf_ConfigPtr = NULL;
    WdgIf_Status = WDGIF_UNINIT;
}

/**
 * @brief Set watchdog mode for specified device
 * SWS_WdgIf_00003
 */
Std_ReturnType WdgIf_SetMode(WdgIf_DeviceType Device, WdgIf_ModeType WdgMode)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_Status == WDGIF_UNINIT)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_SETMODE, WDGIF_E_DRIVER_UNINIT);
        return E_NOT_OK;
    }
    
    if (!WdgIf_IsValidDevice(Device))
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_SETMODE, WDGIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    
    if (!WdgIf_IsValidMode(WdgMode))
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_SETMODE, WDGIF_E_PARAM_MODE);
        return E_NOT_OK;
    }
    #endif
    
    /* Set the mode (simulated - would call underlying Wdg driver) */
    WdgIf_CurrentMode[Device] = WdgMode;
    WdgIf_DeviceInitialized[Device] = TRUE;
    result = E_OK;
    
    #if (WDGIF_MODE_CHANGE_CB_ENABLED == STD_ON)
    /* Call mode change callback if enabled */
    if (WdgIf_ConfigPtr->DeviceConfig[Device].ModeChangeCallback != NULL)
    {
        WdgIf_ConfigPtr->DeviceConfig[Device].ModeChangeCallback(WdgMode);
    }
    #endif
    
    return result;
}

/**
 * @brief Trigger (kick) the watchdog for specified device
 * SWS_WdgIf_00004
 */
Std_ReturnType WdgIf_Trigger(WdgIf_DeviceType Device)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_Status == WDGIF_UNINIT)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_TRIGGER, WDGIF_E_DRIVER_UNINIT);
        return E_NOT_OK;
    }
    
    if (!WdgIf_IsValidDevice(Device))
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_TRIGGER, WDGIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    #endif
    
    /* Check if device is initialized and not in OFF mode */
    if ((WdgIf_DeviceInitialized[Device] == TRUE) && 
        (WdgIf_CurrentMode[Device] != WDGIF_OFF_MODE))
    {
        /* Trigger watchdog (simulated - would call underlying Wdg driver) */
        result = E_OK;
        
        #if (WDGIF_TRIGGER_CB_ENABLED == STD_ON)
        /* Call trigger callback if enabled */
        if (WdgIf_ConfigPtr->DeviceConfig[Device].TriggerCallback != NULL)
        {
            WdgIf_ConfigPtr->DeviceConfig[Device].TriggerCallback();
        }
        #endif
    }
    
    return result;
}

/**
 * @brief Set trigger condition (timeout) for watchdog
 * SWS_WdgIf_00006
 */
Std_ReturnType WdgIf_SetTriggerCondition(WdgIf_DeviceType Device, 
                                          WdgIf_TimeoutType Timeout)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_Status == WDGIF_UNINIT)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_SETTRIGGERCONDITION, 
                              WDGIF_E_DRIVER_UNINIT);
        return E_NOT_OK;
    }
    
    if (!WdgIf_IsValidDevice(Device))
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_SETTRIGGERCONDITION, 
                              WDGIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    
    if (Timeout == 0)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, Device, WDGIF_SID_SETTRIGGERCONDITION, 
                              WDGIF_E_INV_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    /* Set trigger condition (simulated) */
    result = E_OK;
    
    return result;
}

/**
 * @brief Get version information
 * SWS_WdgIf_00005
 */
#if (WDGIF_VERSION_INFO_API == STD_ON)
void WdgIf_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    #if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL)
    {
        (void)Det_ReportError(WDGIF_MODULE_ID, 0, WDGIF_SID_GETVERSIONINFO, 
                              WDGIF_E_INV_POINTER);
        return;
    }
    #endif
    
    VersionInfo->vendorID = WDGIF_VENDOR_ID;
    VersionInfo->moduleID = WDGIF_MODULE_ID;
    VersionInfo->sw_major_version = WDGIF_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = WDGIF_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = WDGIF_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Get current mode for a device (internal use)
 */
WdgIf_ModeType WdgIf_GetCurrentMode(WdgIf_DeviceType Device)
{
    if (WdgIf_IsValidDevice(Device))
    {
        return WdgIf_CurrentMode[Device];
    }
    return WDGIF_OFF_MODE;
}

/**
 * @brief Check if device is initialized (internal use)
 */
boolean WdgIf_IsDeviceInitialized(WdgIf_DeviceType Device)
{
    if (WdgIf_IsValidDevice(Device))
    {
        return WdgIf_DeviceInitialized[Device];
    }
    return FALSE;
}

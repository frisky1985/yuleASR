/**
 * @file Wdg.h
 * @brief WDG (Watchdog) Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: WDG Driver (WDG)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef WDG_H
#define WDG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Wdg_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define WDG_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define WDG_MODULE_ID                   (0x10U) /* WDG Driver Module ID */
#define WDG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define WDG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define WDG_AR_RELEASE_REVISION_VERSION (0x00U)
#define WDG_SW_MAJOR_VERSION            (0x01U)
#define WDG_SW_MINOR_VERSION            (0x00U)
#define WDG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define WDG_VARIANT_PRE_COMPILE         (0x01U)
#define WDG_VARIANT_LINK_TIME           (0x02U)
#define WDG_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define WDG_SID_INIT                    (0x00U)
#define WDG_SID_SETMODE                 (0x01U)
#define WDG_SID_TRIGGER                 (0x02U)
#define WDG_SID_GETVERSIONINFO          (0x03U)
#define WDG_SID_SETTRIGGERCONDITION     (0x04U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define WDG_E_DRIVER_STATE              (0x10U)
#define WDG_E_PARAM_MODE                (0x11U)
#define WDG_E_PARAM_POINTER             (0x12U)
#define WDG_E_PARAM_CONFIG              (0x13U)
#define WDG_E_PARAM_TIMEOUT             (0x14U)
#define WDG_E_DISABLE_NOT_ALLOWED       (0x15U)
#define WDG_E_FORBIDDEN_INVOCATION      (0x16U)
#define WDG_E_ALREADY_INITIALIZED       (0x17U)
#define WDG_E_UNINIT                    (0x18U)

/*==================================================================================================
*                                    WDG MODE TYPE
==================================================================================================*/
typedef enum {
    WDGIF_OFF_MODE = 0,
    WDGIF_SLOW_MODE,
    WDGIF_FAST_MODE
} WdgIf_ModeType;

/*==================================================================================================
*                                    WDG TIMEOUT TYPE
==================================================================================================*/
typedef uint16 Wdg_TimeoutType;

/*==================================================================================================
*                                    WDG CLOCK PRESCALER TYPE
==================================================================================================*/
typedef enum {
    WDG_PRESCALER_1 = 0,
    WDG_PRESCALER_2,
    WDG_PRESCALER_4,
    WDG_PRESCALER_8,
    WDG_PRESCALER_16,
    WDG_PRESCALER_32,
    WDG_PRESCALER_64,
    WDG_PRESCALER_128
} Wdg_ClockPrescalerType;

/*==================================================================================================
*                                    WDG MODE SETTINGS TYPE
==================================================================================================*/
typedef struct {
    Wdg_TimeoutType TimeoutPeriod;
    Wdg_ClockPrescalerType ClockPrescaler;
    boolean WindowModeEnabled;
    Wdg_TimeoutType WindowStart;
    Wdg_TimeoutType WindowEnd;
    boolean InterruptMode;
} Wdg_ModeSettingsType;

/*==================================================================================================
*                                    WDG CONFIG TYPE
==================================================================================================*/
typedef struct {
    uint32 BaseAddress;
    Wdg_ModeSettingsType FastModeSettings;
    Wdg_ModeSettingsType SlowModeSettings;
    WdgIf_ModeType InitialMode;
    Wdg_TimeoutType DefaultTimeout;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean DisableAllowed;
} Wdg_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define WDG_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Wdg_ConfigType Wdg_Config;

#define WDG_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define WDG_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the WDG driver
 * @param ConfigPtr Pointer to configuration structure
 */
void Wdg_Init(const Wdg_ConfigType* ConfigPtr);

/**
 * @brief Sets the watchdog mode
 * @param Mode Mode to set (OFF, SLOW, FAST)
 * @return Result of operation
 */
Std_ReturnType Wdg_SetMode(WdgIf_ModeType Mode);

/**
 * @brief Triggers the watchdog (kicks the dog)
 */
void Wdg_Trigger(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Wdg_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets the trigger condition timeout
 * @param timeout Timeout value in ms
 * @return Result of operation
 */
Std_ReturnType Wdg_SetTriggerCondition(uint16 timeout);

#define WDG_STOP_SEC_CODE
#include "MemMap.h"

#endif /* WDG_H */

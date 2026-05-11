/*==================================================================================================
 *                                      WATCHDOG HARDWARE ABSTRACTION
 *==================================================================================================
 * FILENAME: Wdg_Hw.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_WatchdogDriver.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Hardware abstraction layer for Watchdog Driver
 *              Supports Independent Watchdog (IWDG) and Window Watchdog (WWDG)
 *              Platform: ARM Cortex-M (STM32, NXP i.MX, NXP S32K)
 *==================================================================================================
 */

#ifndef WDG_HW_H
#define WDG_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Wdg.h"
#include "Wdg_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define WDG_HW_VENDOR_ID                    (0x01U)
#define WDG_HW_MODULE_ID                    (0x10U)
#define WDG_HW_INSTANCE_ID                  (0U)

#define WDG_HW_AR_RELEASE_MAJOR_VERSION     (4U)
#define WDG_HW_AR_RELEASE_MINOR_VERSION     (7U)
#define WDG_HW_AR_RELEASE_REVISION_VERSION  (0U)

#define WDG_HW_SW_MAJOR_VERSION             (1U)
#define WDG_HW_SW_MINOR_VERSION             (0U)
#define WDG_HW_SW_PATCH_VERSION             (0U)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define WDG_HW_SID_INIT                     (0x10U)
#define WDG_HW_SID_DEINIT                   (0x11U)
#define WDG_HW_SID_SETTRIGGERCONDITION      (0x12U)
#define WDG_HW_SID_TRIGGER                  (0x13U)
#define WDG_HW_SID_DISABLE                  (0x14U)
#define WDG_HW_SID_GETSTATUS                (0x15U)
#define WDG_HW_SID_SETWINDOW                (0x16U)
#define WDG_HW_SID_IRQHANDLER               (0x17U)
#define WDG_HW_SID_GETRESETREASON           (0x18U)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
#define WDG_HW_E_UNINIT                     (0x01U)
#define WDG_HW_E_PARAM_POINTER              (0x02U)
#define WDG_HW_E_PARAM_TIMEOUT              (0x03U)
#define WDG_HW_E_PARAM_MODE                 (0x04U)
#define WDG_HW_E_DISABLE_NOT_ALLOWED        (0x05U)
#define WDG_HW_E_ALREADY_INITIALIZED        (0x06U)
#define WDG_HW_E_HW_FAILURE                 (0x07U)

/*==================================================================================================
 *                                    WATCHDOG TYPES
 *==================================================================================================*/

typedef enum {
    WDG_HW_TYPE_NONE = 0,
    WDG_HW_TYPE_IWDG,           /* Independent Watchdog */
    WDG_HW_TYPE_WWDG,           /* Window Watchdog */
    WDG_HW_TYPE_EXTERNAL        /* External watchdog */
} Wdg_Hw_TypeType;

typedef enum {
    WDG_HW_STATUS_UNINIT = 0,
    WDG_HW_STATUS_IDLE,
    WDG_HW_STATUS_RUNNING,
    WDG_HW_STATUS_STOPPED,
    WDG_HW_STATUS_ERROR
} Wdg_Hw_StatusType;

typedef enum {
    WDG_HW_RESET_NONE = 0,
    WDG_HW_RESET_IWDG,
    WDG_HW_RESET_WWDG,
    WDG_HW_RESET_SOFTWARE,
    WDG_HW_RESET_EXTERNAL,
    WDG_HW_RESET_POWER_ON
} Wdg_Hw_ResetReasonType;

/*==================================================================================================
 *                                    CONFIGURATION TYPES
 *==================================================================================================*/
typedef struct {
    uint32 baseAddress;
    uint32 clockFreqHz;
    boolean useInterrupt;
    boolean windowModeEnabled;
    uint32 windowStart;         /* Minimum timeout (window start) */
    uint32 windowEnd;           /* Maximum timeout (window end) */
    uint8 prescaler;
} Wdg_Hw_IwdgConfigType;

typedef struct {
    uint32 baseAddress;
    uint32 clockFreqHz;
    boolean useInterrupt;
    boolean windowModeEnabled;
    uint32 windowValue;         /* Window value */
    uint8 prescaler;
} Wdg_Hw_WwdgConfigType;

typedef struct {
    Wdg_Hw_TypeType wdgType;
    union {
        Wdg_Hw_IwdgConfigType iwdg;
        Wdg_Hw_WwdgConfigType wwdg;
    } config;
    boolean disableAllowed;
} Wdg_Hw_ConfigType;

/*==================================================================================================
 *                                    PLATFORM SELECTION
 *==================================================================================================*/
/* Platform selection via compiler flags:
 * Define one of the following:
 * - STM32 (for STM32F4/F7 series)
 * - STM32H7 (for STM32H7 specific)
 * - NXP_IMXRT (for i.MX RT series)
 * - NXP_S32K (for S32K series)
 * - GENERIC (generic implementation with mock registers for testing)
 */

#if !defined(STM32) && !defined(STM32H7) && !defined(NXP_IMXRT) && !defined(NXP_S32K) && !defined(GENERIC)
    #define GENERIC     /* Default to generic implementation */
#endif

/*==================================================================================================
 *                                    API DECLARATIONS
 *==================================================================================================*/
#define WDG_HW_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initialize watchdog hardware
 * @param ConfigPtr Pointer to hardware configuration
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Wdg_Hw_Init(const Wdg_Hw_ConfigType* ConfigPtr);

/**
 * @brief Deinitialize watchdog hardware
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Wdg_Hw_DeInit(void);

/**
 * @brief Set trigger condition (timeout value)
 * @param Timeout Timeout value in milliseconds
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Wdg_Hw_SetTriggerCondition(uint16 Timeout);

/**
 * @brief Trigger (refresh) the watchdog
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Wdg_Hw_Trigger(void);

/**
 * @brief Disable the watchdog
 * @return E_OK: Success, E_NOT_OK: Failed (disable not allowed)
 */
extern Std_ReturnType Wdg_Hw_Disable(void);

/**
 * @brief Get current watchdog status
 * @return Wdg_Hw_StatusType: Current status
 */
extern Wdg_Hw_StatusType Wdg_Hw_GetStatus(void);

/**
 * @brief Set window mode parameters
 * @param StartValue Window start value (minimum)
 * @param EndValue Window end value (maximum)
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Wdg_Hw_SetWindow(uint32 StartValue, uint32 EndValue);

/**
 * @brief Watchdog interrupt handler
 * @return None
 */
extern void Wdg_Hw_IRQHandler(void);

/**
 * @brief Get reset reason
 * @return Wdg_Hw_ResetReasonType: Reason for last reset
 */
extern Wdg_Hw_ResetReasonType Wdg_Hw_GetResetReason(void);

/**
 * @brief Check if watchdog is enabled
 * @return TRUE: Enabled, FALSE: Disabled
 */
extern boolean Wdg_Hw_IsEnabled(void);

/**
 * @brief Get current counter value
 * @return Current counter value
 */
extern uint32 Wdg_Hw_GetCounter(void);

/**
 * @brief Configure early warning interrupt
 * @param Threshold Counter value threshold for interrupt
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Wdg_Hw_SetEarlyWarningInterrupt(uint32 Threshold);

/**
 * @brief Clear interrupt flag
 * @return None
 */
extern void Wdg_Hw_ClearInterruptFlag(void);

#define WDG_HW_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* WDG_HW_H */

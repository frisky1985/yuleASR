/**
 * @file Pwm.h
 * @brief PWM Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: PWM Driver (PWM)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef PWM_H
#define PWM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Pwm_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define PWM_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define PWM_MODULE_ID                   (0x11U) /* PWM Driver Module ID */
#define PWM_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define PWM_AR_RELEASE_MINOR_VERSION    (0x04U)
#define PWM_AR_RELEASE_REVISION_VERSION (0x00U)
#define PWM_SW_MAJOR_VERSION            (0x01U)
#define PWM_SW_MINOR_VERSION            (0x00U)
#define PWM_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define PWM_VARIANT_PRE_COMPILE         (0x01U)
#define PWM_VARIANT_LINK_TIME           (0x02U)
#define PWM_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define PWM_SID_INIT                    (0x00U)
#define PWM_SID_DEINIT                  (0x01U)
#define PWM_SID_SETDUTYCYCLE            (0x02U)
#define PWM_SID_SETPERIODANDDUTY        (0x03U)
#define PWM_SID_SETOUTPUTTOIDLE         (0x04U)
#define PWM_SID_GETOUTPUTSTATE          (0x05U)
#define PWM_SID_DISABLENOTIFICATION     (0x06U)
#define PWM_SID_ENABLENOTIFICATION      (0x07U)
#define PWM_SID_GETVERSIONINFO          (0x08U)
#define PWM_SID_SETPOWERSTATE           (0x09U)
#define PWM_SID_GETTARGETPOWERSTATE     (0x0AU)
#define PWM_SID_GETCURRENTPOWERSTATE    (0x0BU)
#define PWM_SID_PREPAREPOWERSTATE       (0x0CU)
#define PWM_SID_SETCLOCKDIV             (0x0DU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define PWM_E_PARAM_CONFIG              (0x0AU)
#define PWM_E_UNINIT                    (0x0BU)
#define PWM_E_PARAM_CHANNEL             (0x0CU)
#define PWM_E_PERIOD_UNCHANGEABLE       (0x0DU)
#define PWM_E_ALREADY_INITIALIZED       (0x0EU)
#define PWM_E_PARAM_POINTER             (0x0FU)
#define PWM_E_POWER_STATE_NOT_SUPPORTED (0x10U)
#define PWM_E_TRANSITION_NOT_POSSIBLE   (0x11U)
#define PWM_E_PERIPHERAL_NOT_PREPARED   (0x12U)

/*==================================================================================================
*                                    PWM CHANNEL TYPE
==================================================================================================*/
typedef uint8 Pwm_ChannelType;

/*==================================================================================================
*                                    PWM PERIOD TYPE
==================================================================================================*/
typedef uint32 Pwm_PeriodType;

/*==================================================================================================
*                                    PWM DUTY CYCLE TYPE
==================================================================================================*/
typedef uint16 Pwm_DutyCycleType;

/*==================================================================================================
*                                    PWM OUTPUT STATE TYPE
==================================================================================================*/
typedef enum {
    PWM_LOW = 0,
    PWM_HIGH
} Pwm_OutputStateType;

/*==================================================================================================
*                                    PWM EDGE NOTIFICATION TYPE
==================================================================================================*/
typedef enum {
    PWM_RISING_EDGE = 0,
    PWM_FALLING_EDGE,
    PWM_BOTH_EDGES
} Pwm_EdgeNotificationType;

/*==================================================================================================
*                                    PWM POWER STATE TYPE
==================================================================================================*/
typedef enum {
    PWM_FULL_POWER = 0,
    PWM_LOW_POWER
} Pwm_PowerStateRequestResultType;

/*==================================================================================================
*                                    PWM POWER STATE REQUEST RESULT TYPE
==================================================================================================*/
typedef enum {
    PWM_SERVICE_ACCEPTED = 0,
    PWM_NOT_INIT,
    PWM_SEQUENCE_ERROR,
    PWM_HW_FAILURE,
    PWM_POWER_STATE_NOT_SUPP,
    PWM_TRANS_NOT_POSSIBLE
} Pwm_PowerStateType;

/*==================================================================================================
*                                    PWM CHANNEL CLASS TYPE
==================================================================================================*/
typedef enum {
    PWM_VARIABLE_PERIOD = 0,
    PWM_FIXED_PERIOD,
    PWM_FIXED_PERIOD_SHIFTED
} Pwm_ChannelClassType;

/*==================================================================================================
*                                    PWM IDLE STATE TYPE
==================================================================================================*/
typedef enum {
    PWM_IDLE_LOW = 0,
    PWM_IDLE_HIGH
} Pwm_IdleStateType;

/*==================================================================================================
*                                    PWM POLARITY TYPE
==================================================================================================*/
typedef enum {
    PWM_POLARITY_LOW = 0,
    PWM_POLARITY_HIGH
} Pwm_PolarityType;

/*==================================================================================================
*                                    PWM CLOCK SOURCE TYPE
==================================================================================================*/
typedef enum {
    PWM_CLOCK_SYSTEM = 0,
    PWM_CLOCK_BUS,
    PWM_CLOCK_EXTERNAL
} Pwm_ClockSourceType;

/*==================================================================================================
*                                    PWM CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    Pwm_ChannelType ChannelId;
    uint32 BaseAddress;
    Pwm_ChannelClassType ChannelClass;
    Pwm_PeriodType DefaultPeriod;
    Pwm_DutyCycleType DefaultDutyCycle;
    Pwm_IdleStateType IdleState;
    Pwm_PolarityType Polarity;
    Pwm_ClockSourceType ClockSource;
    uint32 ClockPrescaler;
    boolean NotificationSupported;
    void (*NotificationFn)(void);
} Pwm_ChannelConfigType;

/*==================================================================================================
*                                    PWM CONFIG TYPE
==================================================================================================*/
typedef struct {
    const Pwm_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean DeInitApi;
    boolean SetDutyCycleApi;
    boolean SetPeriodAndDutyApi;
    boolean SetOutputToIdleApi;
    boolean GetOutputStateApi;
    boolean NotificationSupported;
    boolean PowerStateSupported;
} Pwm_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define PWM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Pwm_ConfigType Pwm_Config;

#define PWM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define PWM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the PWM driver
 * @param ConfigPtr Pointer to configuration structure
 */
void Pwm_Init(const Pwm_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the PWM driver
 */
void Pwm_DeInit(void);

/**
 * @brief Sets the duty cycle for a channel
 * @param Channel Channel to configure
 * @param DutyCycle Duty cycle value (0-0x8000)
 */
void Pwm_SetDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle);

/**
 * @brief Sets period and duty cycle for a channel
 * @param Channel Channel to configure
 * @param Period Period in ticks
 * @param DutyCycle Duty cycle value
 */
void Pwm_SetPeriodAndDuty(Pwm_ChannelType Channel, Pwm_PeriodType Period, uint16 DutyCycle);

/**
 * @brief Sets output to idle state
 * @param Channel Channel to set
 */
void Pwm_SetOutputToIdle(Pwm_ChannelType Channel);

/**
 * @brief Gets output state of a channel
 * @param Channel Channel to check
 * @return Current output state
 */
Pwm_OutputStateType Pwm_GetOutputState(Pwm_ChannelType Channel);

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable
 */
void Pwm_DisableNotification(Pwm_ChannelType Channel);

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable
 * @param Notification Notification edge type
 */
void Pwm_EnableNotification(Pwm_ChannelType Channel, Pwm_EdgeNotificationType Notification);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Pwm_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets power state
 * @param PowerState Power state to set
 * @param Result Result of operation
 */
void Pwm_SetPowerState(Pwm_PowerStateType PowerState, Pwm_PowerStateRequestResultType* Result);

/**
 * @brief Gets target power state
 * @param TargetPowerState Pointer to store target power state
 * @param Result Result of operation
 */
void Pwm_GetTargetPowerState(Pwm_PowerStateType* TargetPowerState, Pwm_PowerStateRequestResultType* Result);

/**
 * @brief Gets current power state
 * @param CurrentPowerState Pointer to store current power state
 * @param Result Result of operation
 */
void Pwm_GetCurrentPowerState(Pwm_PowerStateType* CurrentPowerState, Pwm_PowerStateRequestResultType* Result);

/**
 * @brief Prepares power state transition
 * @param PowerState Target power state
 * @param Result Result of operation
 */
void Pwm_PreparePowerState(Pwm_PowerStateType PowerState, Pwm_PowerStateRequestResultType* Result);

#define PWM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* PWM_H */

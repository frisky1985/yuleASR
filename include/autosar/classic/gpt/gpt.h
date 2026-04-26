/******************************************************************************
 * @file    gpt.h
 * @brief   GPT (General Purpose Timer) Driver interface - AUTOSAR Classic Platform
 *
 * GPT Driver provides timer services for AUTOSAR basic software.
 * Module ID: 0x10 (16 in decimal)
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024-2026
 ******************************************************************************/
#ifndef GPT_H
#define GPT_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "Std_Types.h"
#include "gpt_Cfg.h"

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define GPT_VENDOR_ID                           (0x01U)
#define GPT_MODULE_ID                           (0x10U)  /* GPT = 16 */

#define GPT_AR_RELEASE_MAJOR_VERSION            (0x04U)
#define GPT_AR_RELEASE_MINOR_VERSION            (0x04U)
#define GPT_AR_RELEASE_REVISION_VERSION         (0x00U)

#define GPT_SW_MAJOR_VERSION                    (0x01U)
#define GPT_SW_MINOR_VERSION                    (0x00U)
#define GPT_SW_PATCH_VERSION                    (0x00U)

/******************************************************************************
 * Service IDs
 ******************************************************************************/
#define GPT_SID_INIT                            (0x00U)
#define GPT_SID_DEINIT                          (0x01U)
#define GPT_SID_GETTIMEELAPSED                  (0x02U)
#define GPT_SID_GETTIMEREMAINING                (0x03U)
#define GPT_SID_STARTTIMER                      (0x04U)
#define GPT_SID_STOPTIMER                       (0x05U)
#define GPT_SID_ENABLEINTERRUPT                 (0x06U)
#define GPT_SID_DISABLEINTERRUPT                (0x07U)
#define GPT_SID_GETVERSIONINFO                  (0x08U)
#define GPT_SID_SETMODE                         (0x09U)
#define GPT_SID_DISABLEWAKEUP                   (0x0AU)
#define GPT_SID_ENABLEWAKEUP                    (0x0BU)
#define GPT_SID_CHECKWAKEUP                     (0x0CU)
#define GPT_SID_GETPREDEFTIMERVALUE             (0x0DU)

/******************************************************************************
 * Development Error Codes
 ******************************************************************************/
#define GPT_E_PARAM_CHANNEL                     (0x0AU)
#define GPT_E_PARAM_VALUE                       (0x0BU)
#define GPT_E_PARAM_POINTER                     (0x0CU)
#define GPT_E_PARAM_MODE                        (0x0DU)
#define GPT_E_PARAM_PREDEF_TIMER                (0x0EU)
#define GPT_E_ALREADY_INITIALIZED               (0x0FU)
#define GPT_E_CHANNEL_BUSY                      (0x10U)
#define GPT_E_UNINIT                            (0x11U)
#define GPT_E_INIT_FAILED                       (0x12U)
#define GPT_E_PARAM_CONFIG                      (0x13U)

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* GPT Channel Type - 0-31 channels supported */
typedef uint8 Gpt_ChannelType;

/* GPT Value Type - up to 32-bit timer values */
typedef uint32 Gpt_ValueType;

/* GPT Predef Timer Type */
typedef enum {
    GPT_PREDEF_TIMER_1US_16BIT = 0x01U,
    GPT_PREDEF_TIMER_1US_24BIT = 0x02U,
    GPT_PREDEF_TIMER_1US_32BIT = 0x04U,
    GPT_PREDEF_TIMER_100US_32BIT = 0x08U
} Gpt_PredefTimerType;

/* GPT Mode Type - Normal / Sleep */
typedef enum {
    GPT_MODE_NORMAL = 0,
    GPT_MODE_SLEEP
} Gpt_ModeType;

/* GPT Channel Mode - Continuous / One-shot */
typedef enum {
    GPT_CH_MODE_CONTINUOUS = 0,
    GPT_CH_MODE_ONESHOT
} Gpt_ChannelModeType;

/* GPT Clock Prescaler Type */
typedef enum {
    GPT_CLOCK_PRESCALER_1 = 0,
    GPT_CLOCK_PRESCALER_2,
    GPT_CLOCK_PRESCALER_4,
    GPT_CLOCK_PRESCALER_8,
    GPT_CLOCK_PRESCALER_16,
    GPT_CLOCK_PRESCALER_32,
    GPT_CLOCK_PRESCALER_64,
    GPT_CLOCK_PRESCALER_128
} Gpt_ClockPrescalerType;

/* GPT Clock Source Type */
typedef enum {
    GPT_CLOCK_SOURCE_SYSTEM = 0,
    GPT_CLOCK_SOURCE_PERIPHERAL,
    GPT_CLOCK_SOURCE_EXTERNAL,
    GPT_CLOCK_SOURCE_LOW_FREQ
} Gpt_ClockSourceType;

/* GPT Channel Status */
typedef enum {
    GPT_CH_STATUS_UNINIT = 0,
    GPT_CH_STATUS_READY,
    GPT_CH_STATUS_RUNNING,
    GPT_CH_STATUS_STOPPED,
    GPT_CH_STATUS_EXPIRED
} Gpt_ChannelStatusType;

/* GPT Sync/Async Control Type */
typedef enum {
    GPT_CONTROL_SYNC = 0,
    GPT_CONTROL_ASYNC
} Gpt_ControlType;

/* GPT Capture Mode Type */
typedef enum {
    GPT_CAPTURE_DISABLE = 0,
    GPT_CAPTURE_RISING_EDGE,
    GPT_CAPTURE_FALLING_EDGE,
    GPT_CAPTURE_BOTH_EDGES
} Gpt_CaptureModeType;

/* GPT PWM Mode Type */
typedef enum {
    GPT_PWM_DISABLE = 0,
    GPT_PWM_MODE_VARIABLE,
    GPT_PWM_MODE_FIXED
} Gpt_PwmModeType;

/* Forward declaration */
struct Gpt_ChannelConfigType;

/* GPT Notification Function Pointer */
typedef void (*Gpt_NotificationCallbackType)(void);

/* GPT Channel Configuration Type */
typedef struct {
    Gpt_ChannelType ChannelId;
    Gpt_ChannelModeType ChannelMode;
    Gpt_ClockPrescalerType ClockPrescaler;
    Gpt_ClockSourceType ClockSource;
    Gpt_ValueType MaxTickValue;
    uint32 ClockFrequency;
    boolean WakeupSupport;
    boolean NotificationEnabled;
    Gpt_NotificationCallbackType NotificationFn;
    /* Extended features */
    Gpt_CaptureModeType CaptureMode;
    Gpt_PwmModeType PwmMode;
    Gpt_ValueType PwmPeriod;
    Gpt_ValueType PwmDutyCycle;
} Gpt_ChannelConfigType;

/* GPT Configuration Type */
typedef struct {
    const Gpt_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean WakeupFunctionalityApi;
    boolean DeInitApi;
    boolean TimeElapsedApi;
    boolean TimeRemainingApi;
    boolean EnableDisableNotificationApi;
    boolean NotificationSupported;
    Gpt_ModeType DefaultMode;
    boolean PredefTimer1us16bitEnable;
    boolean PredefTimer1us24bitEnable;
    boolean PredefTimer1us32bitEnable;
    boolean PredefTimer100us32bitEnable;
    uint32 SystemClockFrequency;
    Gpt_ControlType ControlMode;
} Gpt_ConfigType;

/******************************************************************************
 * External Configuration Pointer
 ******************************************************************************/
#define GPT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Gpt_ConfigType Gpt_Config;

#define GPT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/******************************************************************************
 * API Functions
 ******************************************************************************/
#define GPT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the GPT driver
 * @param ConfigPtr Pointer to configuration structure
 */
extern void Gpt_Init(const Gpt_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the GPT driver
 */
#if (GPT_DEINIT_API == STD_ON)
extern void Gpt_DeInit(void);
#endif

/**
 * @brief Gets time elapsed for a channel
 * @param Channel Channel to check
 * @return Time elapsed in ticks
 */
#if (GPT_TIME_ELAPSED_API == STD_ON)
extern Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel);
#endif

/**
 * @brief Gets time remaining for a channel
 * @param Channel Channel to check
 * @return Time remaining in ticks
 */
#if (GPT_TIME_REMAINING_API == STD_ON)
extern Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel);
#endif

/**
 * @brief Starts a timer channel
 * @param Channel Channel to start
 * @param Value Timeout value in ticks
 */
extern void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value);

/**
 * @brief Stops a timer channel
 * @param Channel Channel to stop
 */
extern void Gpt_StopTimer(Gpt_ChannelType Channel);

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable
 */
#if (GPT_ENABLE_DISABLE_NOTIFICATION_API == STD_ON)
extern void Gpt_EnableNotification(Gpt_ChannelType Channel);
#endif

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable
 */
#if (GPT_ENABLE_DISABLE_NOTIFICATION_API == STD_ON)
extern void Gpt_DisableNotification(Gpt_ChannelType Channel);
#endif

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
extern void Gpt_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set (Normal or Sleep)
 */
extern void Gpt_SetMode(Gpt_ModeType Mode);

/**
 * @brief Disables wakeup for a channel
 * @param Channel Channel to disable
 */
#if (GPT_WAKEUP_FUNCTIONALITY_API == STD_ON)
extern void Gpt_DisableWakeup(Gpt_ChannelType Channel);
#endif

/**
 * @brief Enables wakeup for a channel
 * @param Channel Channel to enable
 */
#if (GPT_WAKEUP_FUNCTIONALITY_API == STD_ON)
extern void Gpt_EnableWakeup(Gpt_ChannelType Channel);
#endif

/**
 * @brief Checks for wakeup events
 * @param Channel Channel to check
 * @return Wakeup detected flag
 */
#if (GPT_WAKEUP_FUNCTIONALITY_API == STD_ON)
extern Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel);
#endif

/**
 * @brief Gets predefined timer value
 * @param PredefTimer Predefined timer to read
 * @param TimeValuePtr Pointer to store time value
 * @return Result of operation
 */
extern Std_ReturnType Gpt_GetPredefTimerValue(Gpt_PredefTimerType PredefTimer, 
                                               uint32* TimeValuePtr);

/******************************************************************************
 * Extended API Functions
 ******************************************************************************/

/**
 * @brief Gets channel status
 * @param Channel Channel to check
 * @return Channel status
 */
extern Gpt_ChannelStatusType Gpt_GetChannelStatus(Gpt_ChannelType Channel);

/**
 * @brief Sets channel control mode (sync/async)
 * @param Channel Channel to configure
 * @param ControlMode Control mode to set
 * @return Result of operation
 */
extern Std_ReturnType Gpt_SetControlMode(Gpt_ChannelType Channel, 
                                          Gpt_ControlType ControlMode);

/**
 * @brief Enables capture mode for a channel
 * @param Channel Channel to configure
 * @param CaptureMode Capture mode (edge detection)
 * @return Result of operation
 */
extern Std_ReturnType Gpt_EnableCapture(Gpt_ChannelType Channel,
                                         Gpt_CaptureModeType CaptureMode);

/**
 * @brief Disables capture mode for a channel
 * @param Channel Channel to configure
 */
extern void Gpt_DisableCapture(Gpt_ChannelType Channel);

/**
 * @brief Gets captured value for a channel
 * @param Channel Channel to read
 * @return Captured timer value
 */
extern Gpt_ValueType Gpt_GetCaptureValue(Gpt_ChannelType Channel);

/**
 * @brief Enables PWM mode for a channel
 * @param Channel Channel to configure
 * @param Period PWM period in ticks
 * @param DutyCycle PWM duty cycle (0-10000 = 0.00% - 100.00%)
 * @return Result of operation
 */
extern Std_ReturnType Gpt_EnablePwm(Gpt_ChannelType Channel,
                                     Gpt_ValueType Period,
                                     uint16 DutyCycle);

/**
 * @brief Disables PWM mode for a channel
 * @param Channel Channel to configure
 */
extern void Gpt_DisablePwm(Gpt_ChannelType Channel);

/**
 * @brief Sets PWM duty cycle for a channel
 * @param Channel Channel to configure
 * @param DutyCycle PWM duty cycle (0-10000 = 0.00% - 100.00%)
 * @return Result of operation
 */
extern Std_ReturnType Gpt_SetPwmDutyCycle(Gpt_ChannelType Channel, uint16 DutyCycle);

/**
 * @brief Synchronizes all running channels
 * @return Result of operation
 */
extern Std_ReturnType Gpt_Synchronize(void);

/**
 * @brief Converts milliseconds to timer ticks
 * @param Channel Channel reference
 * @param Milliseconds Time in milliseconds
 * @return Timer ticks
 */
extern Gpt_ValueType Gpt_MsToTicks(Gpt_ChannelType Channel, uint32 Milliseconds);

/**
 * @brief Converts timer ticks to milliseconds
 * @param Channel Channel reference
 * @param Ticks Timer ticks
 * @return Time in milliseconds
 */
extern uint32 Gpt_TicksToMs(Gpt_ChannelType Channel, Gpt_ValueType Ticks);

/******************************************************************************
 * Internal Functions (for interrupt handler)
 ******************************************************************************/

/**
 * @brief Process timer expiration for a channel (called from ISR)
 * @param Channel Channel that expired
 */
extern void Gpt_ProcessExpiration(Gpt_ChannelType Channel);

/**
 * @brief Process capture event for a channel (called from ISR)
 * @param Channel Channel that captured
 */
extern void Gpt_ProcessCapture(Gpt_ChannelType Channel);

#define GPT_STOP_SEC_CODE
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* GPT_H */

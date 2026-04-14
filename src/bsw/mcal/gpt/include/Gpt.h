/**
 * @file Gpt.h
 * @brief GPT (General Purpose Timer) Driver interface following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: GPT Driver (GPT)
 * Layer: MCAL (Microcontroller Driver Layer)
 */

#ifndef GPT_H
#define GPT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Gpt_Cfg.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define GPT_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define GPT_MODULE_ID                   (0x0EU) /* GPT Driver Module ID */
#define GPT_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define GPT_AR_RELEASE_MINOR_VERSION    (0x04U)
#define GPT_AR_RELEASE_REVISION_VERSION (0x00U)
#define GPT_SW_MAJOR_VERSION            (0x01U)
#define GPT_SW_MINOR_VERSION            (0x00U)
#define GPT_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                         CONFIGURATION VARIANTS
==================================================================================================*/
#define GPT_VARIANT_PRE_COMPILE         (0x01U)
#define GPT_VARIANT_LINK_TIME           (0x02U)
#define GPT_VARIANT_POST_BUILD          (0x03U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define GPT_SID_INIT                    (0x00U)
#define GPT_SID_DEINIT                  (0x01U)
#define GPT_SID_GETTIMEELAPSED          (0x02U)
#define GPT_SID_GETTIMEREMAINING        (0x03U)
#define GPT_SID_STARTTIMER              (0x04U)
#define GPT_SID_STOPTIMER               (0x05U)
#define GPT_SID_ENABLEINTERRUPT         (0x06U)
#define GPT_SID_DISABLEINTERRUPT        (0x07U)
#define GPT_SID_GETVERSIONINFO          (0x08U)
#define GPT_SID_SETMODE                 (0x09U)
#define GPT_SID_DISABLEWAKEUP           (0x0AU)
#define GPT_SID_ENABLEWAKEUP            (0x0BU)
#define GPT_SID_CHECKWAKEUP             (0x0CU)
#define GPT_SID_GETPREDEFTIMERVALUE     (0x0DU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define GPT_E_PARAM_CHANNEL             (0x0AU)
#define GPT_E_PARAM_VALUE               (0x0BU)
#define GPT_E_PARAM_POINTER             (0x0CU)
#define GPT_E_PARAM_MODE                (0x0DU)
#define GPT_E_PARAM_PREDEF_TIMER        (0x0EU)
#define GPT_E_ALREADY_INITIALIZED       (0x0FU)
#define GPT_E_CHANNEL_BUSY              (0x10U)
#define GPT_E_UNINIT                    (0x11U)
#define GPT_E_INIT_FAILED               (0x12U)
#define GPT_E_PARAM_CONFIG              (0x13U)

/*==================================================================================================
*                                    GPT CHANNEL TYPE
==================================================================================================*/
typedef uint8 Gpt_ChannelType;

/*==================================================================================================
*                                    GPT VALUE TYPE
==================================================================================================*/
typedef uint32 Gpt_ValueType;

/*==================================================================================================
*                                    GPT MODE TYPE
==================================================================================================*/
typedef enum {
    GPT_MODE_NORMAL = 0,
    GPT_MODE_SLEEP
} Gpt_ModeType;

/*==================================================================================================
*                                    GPT PREDEF TIMER TYPE
==================================================================================================*/
typedef enum {
    GPT_PREDEF_TIMER_1US_16BIT = 0x01U,
    GPT_PREDEF_TIMER_1US_24BIT = 0x02U,
    GPT_PREDEF_TIMER_1US_32BIT = 0x04U,
    GPT_PREDEF_TIMER_100US_32BIT = 0x08U
} Gpt_PredefTimerType;

/*==================================================================================================
*                                    GPT CHANNEL MODE TYPE
==================================================================================================*/
typedef enum {
    GPT_CH_MODE_CONTINUOUS = 0,
    GPT_CH_MODE_ONESHOT
} Gpt_ChannelModeType;

/*==================================================================================================
*                                    GPT CLOCK PRESCALER TYPE
==================================================================================================*/
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

/*==================================================================================================
*                                    GPT CHANNEL CONFIG TYPE
==================================================================================================*/
typedef struct {
    Gpt_ChannelType ChannelId;
    uint32 BaseAddress;
    Gpt_ChannelModeType ChannelMode;
    Gpt_ClockPrescalerType ClockPrescaler;
    Gpt_ValueType MaxTickValue;
    uint32 ClockFrequency;
    boolean WakeupSupport;
    boolean NotificationEnabled;
    void (*NotificationFn)(void);
} Gpt_ChannelConfigType;

/*==================================================================================================
*                                    GPT CONFIG TYPE
==================================================================================================*/
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
    boolean PredefTimer1usEnablingGrade;
    boolean PredefTimer100us32bitEnable;
} Gpt_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define GPT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Gpt_ConfigType Gpt_Config;

#define GPT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define GPT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the GPT driver
 * @param ConfigPtr Pointer to configuration structure
 */
void Gpt_Init(const Gpt_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the GPT driver
 */
void Gpt_DeInit(void);

/**
 * @brief Gets time elapsed for a channel
 * @param Channel Channel to check
 * @return Time elapsed in ticks
 */
Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel);

/**
 * @brief Gets time remaining for a channel
 * @param Channel Channel to check
 * @return Time remaining in ticks
 */
Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel);

/**
 * @brief Starts a timer channel
 * @param Channel Channel to start
 * @param Value Timeout value in ticks
 */
void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value);

/**
 * @brief Stops a timer channel
 * @param Channel Channel to stop
 */
void Gpt_StopTimer(Gpt_ChannelType Channel);

/**
 * @brief Enables notification for a channel
 * @param Channel Channel to enable
 */
void Gpt_EnableNotification(Gpt_ChannelType Channel);

/**
 * @brief Disables notification for a channel
 * @param Channel Channel to disable
 */
void Gpt_DisableNotification(Gpt_ChannelType Channel);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Gpt_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets the operation mode
 * @param Mode Mode to set
 */
void Gpt_SetMode(Gpt_ModeType Mode);

/**
 * @brief Disables wakeup for a channel
 * @param Channel Channel to disable
 */
void Gpt_DisableWakeup(Gpt_ChannelType Channel);

/**
 * @brief Enables wakeup for a channel
 * @param Channel Channel to enable
 */
void Gpt_EnableWakeup(Gpt_ChannelType Channel);

/**
 * @brief Checks for wakeup events
 * @param Channel Channel to check
 * @return Wakeup detected flag
 */
Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel);

/**
 * @brief Gets predefined timer value
 * @param PredefTimer Predefined timer to read
 * @param TimeValuePtr Pointer to store time value
 * @return Result of operation
 */
Std_ReturnType Gpt_GetPredefTimerValue(Gpt_PredefTimerType PredefTimer, uint32* TimeValuePtr);

#define GPT_STOP_SEC_CODE
#include "MemMap.h"

#endif /* GPT_H */

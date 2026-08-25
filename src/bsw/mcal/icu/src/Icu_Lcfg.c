/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/* @req SWS_Icu_00001 @req SWS_Icu_00002 @req SWS_Icu_00003 */


/**
 * @file Icu_Lcfg.c
 * @brief ICU Driver link-time configuration source file
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Icu_Lcfg.h"
#include "Icu_Cfg.h"

/*==================================================================================================
*                                    NOTIFICATION CALLBACKS
==================================================================================================*/

/* Example notification callbacks - to be implemented by application */
void Icu_Channel0_Notification(void);
void Icu_Channel1_Notification(void);
void Icu_Channel2_Notification(void);
void Icu_Channel3_Notification(void);
void Icu_Channel4_Notification(void);
void Icu_Channel5_Notification(void);
void Icu_Channel6_Notification(void);
void Icu_Channel7_Notification(void);

/* Default empty notification callbacks */
__attribute__((weak)) void Icu_Channel0_Notification(void) { }
__attribute__((weak)) void Icu_Channel1_Notification(void) { }
__attribute__((weak)) void Icu_Channel2_Notification(void) { }
__attribute__((weak)) void Icu_Channel3_Notification(void) { }
__attribute__((weak)) void Icu_Channel4_Notification(void) { }
__attribute__((weak)) void Icu_Channel5_Notification(void) { }
__attribute__((weak)) void Icu_Channel6_Notification(void) { }
__attribute__((weak)) void Icu_Channel7_Notification(void) { }

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/

#define ICU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

const Icu_ChannelConfigType Icu_ChannelConfig[ICU_NUM_CHANNELS] = {
    /* Channel 0 - Edge Detection Mode */
    {
        .ChannelId = ICU_CHANNEL_0,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT,
        .DefaultActivation = ICU_RISING_EDGE,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel0_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 0U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    },

    /* Channel 1 - Signal Measurement Mode */
    {
        .ChannelId = ICU_CHANNEL_1,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT,
        .DefaultActivation = ICU_BOTH_EDGES,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel1_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 0U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    },

    /* Channel 2 - Timestamp Mode */
    {
        .ChannelId = ICU_CHANNEL_2,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_TIMESTAMP,
        .DefaultActivation = ICU_RISING_EDGE,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel2_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 8U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    },

    /* Channel 3 - Edge Counter Mode */
    {
        .ChannelId = ICU_CHANNEL_3,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_EDGE_COUNTER,
        .DefaultActivation = ICU_BOTH_EDGES,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel3_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 0U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    },

    /* Channel 4 - Edge Detection Mode with Wakeup */
    {
        .ChannelId = ICU_CHANNEL_4,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT,
        .DefaultActivation = ICU_RISING_EDGE,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel4_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 0U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = TRUE,
        .ClockPrescaler = 1U
    },

    /* Channel 5 - Signal Measurement Mode */
    {
        .ChannelId = ICU_CHANNEL_5,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT,
        .DefaultActivation = ICU_BOTH_EDGES,
        .SignalMeasurementProperty = ICU_DUTY_CYCLE,
        .NotificationFn = Icu_Channel5_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 0U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    },

    /* Channel 6 - Timestamp Mode */
    {
        .ChannelId = ICU_CHANNEL_6,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_TIMESTAMP,
        .DefaultActivation = ICU_RISING_EDGE,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel6_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 8U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    },

    /* Channel 7 - Edge Counter Mode */
    {
        .ChannelId = ICU_CHANNEL_7,
        .BaseAddress = 0U,
        .MeasurementMode = ICU_MODE_EDGE_COUNTER,
        .DefaultActivation = ICU_BOTH_EDGES,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .NotificationFn = Icu_Channel7_Notification,
        .NotificationEnabled = TRUE,
        .BufferSize = 0U,
        .BufferPtr = NULL_PTR,
        .WakeupSupport = FALSE,
        .ClockPrescaler = 1U
    }
};

const Icu_ConfigType Icu_Config = {
    .Channels = Icu_ChannelConfig,
    .NumChannels = ICU_NUM_CHANNELS,
    .DevErrorDetect = ICU_DEV_ERROR_DETECT,
    .VersionInfoApi = ICU_VERSION_INFO_API,
    .DeInitApi = ICU_DE_INIT_API,
    .SetModeApi = ICU_SET_MODE_API,
    .WakeupFunctionalityApi = ICU_WAKEUP_FUNCTIONALITY_API,
    .DisableWakeupApi = ICU_DISABLE_WAKEUP_API,
    .TimestampApi = ICU_TIMESTAMP_API,
    .EdgeCountApi = ICU_EDGE_COUNT_API,
    .SignalMeasurementApi = ICU_SIGNAL_MEASUREMENT_API,
    .DefaultMode = ICU_DEFAULT_MODE
};

#define ICU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

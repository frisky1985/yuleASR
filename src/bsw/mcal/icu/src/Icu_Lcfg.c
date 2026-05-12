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
        .Channel = ICU_CHANNEL_0,
        .BaseAddress = ICU_CH0_BASE_ADDR,
        .Mode = ICU_CH0_MODE,
        .Edge = ICU_CH0_EDGE,
        .Property = ICU_CH0_PROPERTY,
        .Notification = Icu_Channel0_Notification,
        .TimestampEnabled = ICU_CH0_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH0_BUFFER_SIZE,
        .WakeupSupport = ICU_CH0_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH0_PRESCALER
    },
    
    /* Channel 1 - Signal Measurement Mode */
    {
        .Channel = ICU_CHANNEL_1,
        .BaseAddress = ICU_CH1_BASE_ADDR,
        .Mode = ICU_CH1_MODE,
        .Edge = ICU_CH1_EDGE,
        .Property = ICU_CH1_PROPERTY,
        .Notification = Icu_Channel1_Notification,
        .TimestampEnabled = ICU_CH1_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH1_BUFFER_SIZE,
        .WakeupSupport = ICU_CH1_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH1_PRESCALER
    },
    
    /* Channel 2 - Timestamp Mode */
    {
        .Channel = ICU_CHANNEL_2,
        .BaseAddress = ICU_CH2_BASE_ADDR,
        .Mode = ICU_CH2_MODE,
        .Edge = ICU_CH2_EDGE,
        .Property = ICU_CH2_PROPERTY,
        .Notification = Icu_Channel2_Notification,
        .TimestampEnabled = ICU_CH2_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH2_BUFFER_SIZE,
        .WakeupSupport = ICU_CH2_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH2_PRESCALER
    },
    
    /* Channel 3 - Edge Counter Mode */
    {
        .Channel = ICU_CHANNEL_3,
        .BaseAddress = ICU_CH3_BASE_ADDR,
        .Mode = ICU_CH3_MODE,
        .Edge = ICU_CH3_EDGE,
        .Property = ICU_CH3_PROPERTY,
        .Notification = Icu_Channel3_Notification,
        .TimestampEnabled = ICU_CH3_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH3_BUFFER_SIZE,
        .WakeupSupport = ICU_CH3_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH3_PRESCALER
    },
    
    /* Channel 4 - Edge Detection Mode with Wakeup */
    {
        .Channel = ICU_CHANNEL_4,
        .BaseAddress = ICU_CH4_BASE_ADDR,
        .Mode = ICU_CH4_MODE,
        .Edge = ICU_CH4_EDGE,
        .Property = ICU_CH4_PROPERTY,
        .Notification = Icu_Channel4_Notification,
        .TimestampEnabled = ICU_CH4_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH4_BUFFER_SIZE,
        .WakeupSupport = ICU_CH4_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH4_PRESCALER
    },
    
    /* Channel 5 - Signal Measurement Mode */
    {
        .Channel = ICU_CHANNEL_5,
        .BaseAddress = ICU_CH5_BASE_ADDR,
        .Mode = ICU_CH5_MODE,
        .Edge = ICU_CH5_EDGE,
        .Property = ICU_CH5_PROPERTY,
        .Notification = Icu_Channel5_Notification,
        .TimestampEnabled = ICU_CH5_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH5_BUFFER_SIZE,
        .WakeupSupport = ICU_CH5_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH5_PRESCALER
    },
    
    /* Channel 6 - Timestamp Mode */
    {
        .Channel = ICU_CHANNEL_6,
        .BaseAddress = ICU_CH6_BASE_ADDR,
        .Mode = ICU_CH6_MODE,
        .Edge = ICU_CH6_EDGE,
        .Property = ICU_CH6_PROPERTY,
        .Notification = Icu_Channel6_Notification,
        .TimestampEnabled = ICU_CH6_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH6_BUFFER_SIZE,
        .WakeupSupport = ICU_CH6_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH6_PRESCALER
    },
    
    /* Channel 7 - Edge Counter Mode */
    {
        .Channel = ICU_CHANNEL_7,
        .BaseAddress = ICU_CH7_BASE_ADDR,
        .Mode = ICU_CH7_MODE,
        .Edge = ICU_CH7_EDGE,
        .Property = ICU_CH7_PROPERTY,
        .Notification = Icu_Channel7_Notification,
        .TimestampEnabled = ICU_CH7_TIMESTAMP_ENABLED,
        .TimestampBufferSize = ICU_CH7_BUFFER_SIZE,
        .WakeupSupport = ICU_CH7_WAKEUP_SUPPORT,
        .ClockPrescaler = ICU_CH7_PRESCALER
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

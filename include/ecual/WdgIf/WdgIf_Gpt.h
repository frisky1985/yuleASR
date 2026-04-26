/******************************************************************************
 * @file    WdgIf_Gpt.h
 * @brief   Watchdog Interface GPT Timer Integration
 *
 * Integration with GPT (General Purpose Timer) for periodic watchdog
 * triggering. This module allows automatic watchdog service via
 * hardware timers.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_GPT_H
#define WDGIF_GPT_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "ecual/wdgIf/WdgIf.h"

/******************************************************************************
 * GPT Integration Constants
 ******************************************************************************/

#define WDGIF_GPT_MAX_CHANNELS          3U
#define WDGIF_GPT_MIN_PERIOD_MS         1U
#define WDGIF_GPT_MAX_PERIOD_MS         10000U

/* GPT channel states */
typedef enum {
    WDGIF_GPT_CHANNEL_UNINIT = 0,
    WDGIF_GPT_CHANNEL_READY,
    WDGIF_GPT_CHANNEL_RUNNING,
    WDGIF_GPT_CHANNEL_STOPPED,
    WDGIF_GPT_CHANNEL_ERROR
} WdgIf_GptChannelStateType;

/******************************************************************************
 * GPT Channel Configuration
 ******************************************************************************/
typedef struct {
    uint8 channelId;
    WdgIf_DeviceType wdgDevice;
    uint32 periodMs;
    uint32 tickFrequency;
    boolean enableNotification;
    boolean oneShotMode;
    uint8 priority;             /* Interrupt priority */
} WdgIf_GptChannelConfigType;

/******************************************************************************
 * GPT Integration State
 ******************************************************************************/
typedef struct {
    boolean initialized;
    uint8 numChannels;
    const WdgIf_GptChannelConfigType *channelConfigs[WDGIF_GPT_MAX_CHANNELS];
} WdgIf_GptConfigType;

typedef struct {
    WdgIf_GptChannelStateType state;
    uint32 currentTicks;
    uint32 targetTicks;
    uint32 triggerCount;
    uint32 missedTriggers;
} WdgIf_GptChannelState;

/******************************************************************************
 * GPT API Functions
 ******************************************************************************/

/**
 * @brief Initialize GPT integration
 * @param config GPT configuration
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Gpt_Init(const WdgIf_GptConfigType *config);

/**
 * @brief Deinitialize GPT integration
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Gpt_DeInit(void);

/**
 * @brief Start GPT channel for watchdog triggering
 * @param channel GPT channel ID
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Gpt_StartChannel(uint8 channel);

/**
 * @brief Stop GPT channel
 * @param channel GPT channel ID
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Gpt_StopChannel(uint8 channel);

/**
 * @brief Set GPT channel period
 * @param channel GPT channel ID
 * @param periodMs Period in milliseconds
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Gpt_SetPeriod(uint8 channel, uint32 periodMs);

/**
 * @brief Get GPT channel state
 * @param channel GPT channel ID
 * @return Channel state
 */
WdgIf_GptChannelStateType WdgIf_Gpt_GetChannelState(uint8 channel);

/**
 * @brief GPT notification callback (called from ISR)
 * @param channel GPT channel ID
 */
void WdgIf_Gpt_Notification(uint8 channel);

/**
 * @brief Main function for GPT integration
 * Called cyclically to handle non-ISR tasks
 */
void WdgIf_Gpt_MainFunction(void);

/******************************************************************************
 * Callback Functions (to be called by GPT driver)
 ******************************************************************************/

/**
 * @brief GPT timer callback for watchdog trigger
 * This function is called from the GPT interrupt handler
 * @param channel GPT channel that triggered
 */
void WdgIf_Gpt_TimerCallback(uint8 channel);

/******************************************************************************
 * External Variables
 ******************************************************************************/
extern const WdgIf_GptConfigType *WdgIf_GptConfigPtr;
extern WdgIf_GptChannelState WdgIf_GptChannelStates[WDGIF_GPT_MAX_CHANNELS];

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_GPT_H */

/******************************************************************************
 * @file    WdgIf_Gpt.c
 * @brief   Watchdog Interface GPT Timer Integration Implementation
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
#include "ecual/wdgIf/WdgIf_Gpt.h"
#include <string.h>

/* Define NULL_PTR if not defined */
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

/******************************************************************************
 * Local Variables
 ******************************************************************************/
/* GPT configuration pointer */
const WdgIf_GptConfigType *WdgIf_GptConfigPtr = NULL_PTR;

/* Channel state array */
WdgIf_GptChannelState WdgIf_GptChannelStates[WDGIF_GPT_MAX_CHANNELS];

/* Module initialized flag */
static boolean WdgIf_GptInitialized = FALSE;

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static boolean WdgIf_Gpt_IsChannelValid(uint8 channel);
static void WdgIf_Gpt_ResetChannelState(uint8 channel);
static uint32 WdgIf_Gpt_MsToTicks(uint32 ms, uint32 tickFreq);

/******************************************************************************
 * API Implementation
 ******************************************************************************/

/**
 * @brief Initialize GPT integration
 */
Std_ReturnType WdgIf_Gpt_Init(const WdgIf_GptConfigType *config)
{
    uint8 i;

    if (NULL_PTR == config) {
        return E_NOT_OK;
    }

    /* Check if already initialized */
    if (WdgIf_GptInitialized) {
        return E_NOT_OK;
    }

    /* Validate number of channels */
    if (config->numChannels > WDGIF_GPT_MAX_CHANNELS) {
        return E_NOT_OK;
    }

    /* Store configuration */
    WdgIf_GptConfigPtr = config;

    /* Initialize channel states */
    for (i = 0U; i < WDGIF_GPT_MAX_CHANNELS; i++) {
        WdgIf_Gpt_ResetChannelState(i);
    }

    /* Validate configurations */
    for (i = 0U; i < config->numChannels; i++) {
        if (NULL_PTR != config->channelConfigs[i]) {
            /* Validate period */
            if ((config->channelConfigs[i]->periodMs < WDGIF_GPT_MIN_PERIOD_MS) ||
                (config->channelConfigs[i]->periodMs > WDGIF_GPT_MAX_PERIOD_MS)) {
                return E_NOT_OK;
            }

            /* Set channel to ready state */
            WdgIf_GptChannelStates[i].state = WDGIF_GPT_CHANNEL_READY;

            /* Calculate tick values */
            WdgIf_GptChannelStates[i].targetTicks = WdgIf_Gpt_MsToTicks(
                config->channelConfigs[i]->periodMs,
                config->channelConfigs[i]->tickFrequency
            );
        }
    }

    WdgIf_GptInitialized = TRUE;

    return E_OK;
}

/**
 * @brief Deinitialize GPT integration
 */
Std_ReturnType WdgIf_Gpt_DeInit(void)
{
    uint8 i;

    if (!WdgIf_GptInitialized) {
        return E_NOT_OK;
    }

    /* Stop all running channels */
    for (i = 0U; i < WDGIF_GPT_MAX_CHANNELS; i++) {
        if (WdgIf_GptChannelStates[i].state == WDGIF_GPT_CHANNEL_RUNNING) {
            (void)WdgIf_Gpt_StopChannel(i);
        }
    }

    /* Clear all states */
    for (i = 0U; i < WDGIF_GPT_MAX_CHANNELS; i++) {
        WdgIf_Gpt_ResetChannelState(i);
    }

    WdgIf_GptConfigPtr = NULL_PTR;
    WdgIf_GptInitialized = FALSE;

    return E_OK;
}

/**
 * @brief Start GPT channel for watchdog triggering
 */
Std_ReturnType WdgIf_Gpt_StartChannel(uint8 channel)
{
    if (!WdgIf_GptInitialized) {
        return E_NOT_OK;
    }

    if (!WdgIf_Gpt_IsChannelValid(channel)) {
        return E_NOT_OK;
    }

    /* Check if channel is configured */
    if (NULL_PTR == WdgIf_GptConfigPtr->channelConfigs[channel]) {
        return E_NOT_OK;
    }

    /* Check if already running */
    if (WdgIf_GptChannelStates[channel].state == WDGIF_GPT_CHANNEL_RUNNING) {
        return E_OK;  /* Already running */
    }

    /* Check if in ready or stopped state */
    if ((WdgIf_GptChannelStates[channel].state != WDGIF_GPT_CHANNEL_READY) &&
        (WdgIf_GptChannelStates[channel].state != WDGIF_GPT_CHANNEL_STOPPED)) {
        return E_NOT_OK;
    }

    /* Reset tick counter */
    WdgIf_GptChannelStates[channel].currentTicks = 0U;

    /* Set state to running */
    WdgIf_GptChannelStates[channel].state = WDGIF_GPT_CHANNEL_RUNNING;

    /* TODO: Configure and start actual GPT hardware timer */
    /* This would call the actual GPT driver APIs */

    return E_OK;
}

/**
 * @brief Stop GPT channel
 */
Std_ReturnType WdgIf_Gpt_StopChannel(uint8 channel)
{
    if (!WdgIf_GptInitialized) {
        return E_NOT_OK;
    }

    if (!WdgIf_Gpt_IsChannelValid(channel)) {
        return E_NOT_OK;
    }

    /* Check if running */
    if (WdgIf_GptChannelStates[channel].state != WDGIF_GPT_CHANNEL_RUNNING) {
        return E_OK;  /* Not running */
    }

    /* TODO: Stop actual GPT hardware timer */

    /* Update state */
    WdgIf_GptChannelStates[channel].state = WDGIF_GPT_CHANNEL_STOPPED;

    return E_OK;
}

/**
 * @brief Set GPT channel period
 */
Std_ReturnType WdgIf_Gpt_SetPeriod(uint8 channel, uint32 periodMs)
{
    uint32 newTargetTicks;

    if (!WdgIf_GptInitialized) {
        return E_NOT_OK;
    }

    if (!WdgIf_Gpt_IsChannelValid(channel)) {
        return E_NOT_OK;
    }

    /* Validate period */
    if ((periodMs < WDGIF_GPT_MIN_PERIOD_MS) ||
        (periodMs > WDGIF_GPT_MAX_PERIOD_MS)) {
        return E_NOT_OK;
    }

    /* Check if channel is configured */
    if ((NULL_PTR == WdgIf_GptConfigPtr) ||
        (NULL_PTR == WdgIf_GptConfigPtr->channelConfigs[channel])) {
        return E_NOT_OK;
    }

    /* Calculate new tick value */
    newTargetTicks = WdgIf_Gpt_MsToTicks(
        periodMs,
        WdgIf_GptConfigPtr->channelConfigs[channel]->tickFrequency
    );

    /* Update target ticks */
    WdgIf_GptChannelStates[channel].targetTicks = newTargetTicks;

    /* Reset current counter */
    WdgIf_GptChannelStates[channel].currentTicks = 0U;

    return E_OK;
}

/**
 * @brief Get GPT channel state
 */
WdgIf_GptChannelStateType WdgIf_Gpt_GetChannelState(uint8 channel)
{
    if (!WdgIf_GptInitialized) {
        return WDGIF_GPT_CHANNEL_UNINIT;
    }

    if (!WdgIf_Gpt_IsChannelValid(channel)) {
        return WDGIF_GPT_CHANNEL_ERROR;
    }

    return WdgIf_GptChannelStates[channel].state;
}

/**
 * @brief GPT notification callback (called from ISR)
 */
void WdgIf_Gpt_Notification(uint8 channel)
{
    if (!WdgIf_GptInitialized) {
        return;
    }

    if (!WdgIf_Gpt_IsChannelValid(channel)) {
        return;
    }

    /* Check if channel is running */
    if (WdgIf_GptChannelStates[channel].state != WDGIF_GPT_CHANNEL_RUNNING) {
        return;
    }

    /* Increment tick counter */
    WdgIf_GptChannelStates[channel].currentTicks++;

    /* Check if period expired */
    if (WdgIf_GptChannelStates[channel].currentTicks >=
        WdgIf_GptChannelStates[channel].targetTicks) {

        /* Reset counter */
        WdgIf_GptChannelStates[channel].currentTicks = 0U;

        /* Trigger watchdog */
        WdgIf_Gpt_TimerCallback(channel);

        /* Check if one-shot mode */
        if ((NULL_PTR != WdgIf_GptConfigPtr) &&
            (NULL_PTR != WdgIf_GptConfigPtr->channelConfigs[channel]) &&
            (WdgIf_GptConfigPtr->channelConfigs[channel]->oneShotMode)) {
            /* Stop the channel */
            WdgIf_GptChannelStates[channel].state = WDGIF_GPT_CHANNEL_STOPPED;
        }
    }
}

/**
 * @brief Main function for GPT integration
 */
void WdgIf_Gpt_MainFunction(void)
{
    uint8 i;

    if (!WdgIf_GptInitialized) {
        return;
    }

    /* Process each channel */
    for (i = 0U; i < WDGIF_GPT_MAX_CHANNELS; i++) {
        if (WdgIf_GptChannelStates[i].state == WDGIF_GPT_CHANNEL_RUNNING) {
            /* Check for missed triggers */
            /* This is handled in the notification callback */
        }
    }
}

/**
 * @brief GPT timer callback - triggers the watchdog
 */
void WdgIf_Gpt_TimerCallback(uint8 channel)
{
    Std_ReturnType result;
    WdgIf_DeviceType wdgDevice;

    if (!WdgIf_GptInitialized) {
        return;
    }

    if (channel >= WDGIF_GPT_MAX_CHANNELS) {
        return;
    }

    /* Get watchdog device for this channel */
    if ((NULL_PTR == WdgIf_GptConfigPtr) ||
        (NULL_PTR == WdgIf_GptConfigPtr->channelConfigs[channel])) {
        return;
    }

    wdgDevice = WdgIf_GptConfigPtr->channelConfigs[channel]->wdgDevice;

    /* Trigger the watchdog */
    result = WdgIf_SetTriggerCondition(wdgDevice, 0U);

    if (E_OK == result) {
        WdgIf_GptChannelStates[channel].triggerCount++;
    } else {
        WdgIf_GptChannelStates[channel].missedTriggers++;
        /* TODO: Report error to Dem */
    }
}

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**
 * @brief Check if channel index is valid
 */
static boolean WdgIf_Gpt_IsChannelValid(uint8 channel)
{
    return (channel < WDGIF_GPT_MAX_CHANNELS);
}

/**
 * @brief Reset channel state
 */
static void WdgIf_Gpt_ResetChannelState(uint8 channel)
{
    if (channel < WDGIF_GPT_MAX_CHANNELS) {
        (void)memset(&WdgIf_GptChannelStates[channel], 0,
                     sizeof(WdgIf_GptChannelState));
        WdgIf_GptChannelStates[channel].state = WDGIF_GPT_CHANNEL_UNINIT;
    }
}

/**
 * @brief Convert milliseconds to ticks
 */
static uint32 WdgIf_Gpt_MsToTicks(uint32 ms, uint32 tickFreq)
{
    uint32 ticks;

    if (tickFreq == 0U) {
        return 0U;
    }

    /* ticks = ms * tickFreq / 1000 */
    ticks = (ms * tickFreq) / 1000U;

    /* Ensure at least 1 tick */
    if (ticks == 0U) {
        ticks = 1U;
    }

    return ticks;
}

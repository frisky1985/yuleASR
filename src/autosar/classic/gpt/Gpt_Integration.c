/******************************************************************************
 * @file    Gpt_Integration.c
 * @brief   GPT (General Purpose Timer) Driver Integration Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024-2026
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "autosar/classic/gpt/Gpt_Integration.h"
#include "autosar/classic/gpt/Gpt_Cfg.h"

#if (GPT_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/******************************************************************************
 * Module IDs for Integration
 ******************************************************************************/
#define WDGM_MODULE_ID                          (0x0DU)
#define ECUM_MODULE_ID                          (0x0FU)
#define BSWM_MODULE_ID                          (0x21U)

/******************************************************************************
 * Local Variables
 ******************************************************************************/
#define GPT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* WdgM integration state */
static boolean Gpt_WdgM_TimerActive = FALSE;
static uint32 Gpt_WdgM_PeriodMs = 0U;

/* EcuM integration state */
static boolean Gpt_EcuM_TimerActive = FALSE;
static uint32 Gpt_EcuM_TimestampMs = 0U;
static uint64 Gpt_EcuM_TimestampMs64 = 0U;

/* BswM integration state */
static boolean Gpt_BswM_TimerActive = FALSE;
static uint32 Gpt_BswM_PeriodMs = 0U;

/* DDS integration state */
static boolean Gpt_Dds_TimerActive = FALSE;
static uint32 Gpt_Dds_PeriodUs = 0U;
static uint64 Gpt_Dds_TimestampStart = 0U;

#define GPT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/******************************************************************************
 * External Function Declarations
 ******************************************************************************/
extern void WdgM_CheckpointReached(void);
extern void EcuM_CheckWakeup(void);
extern void BswM_MainFunction(void);

/******************************************************************************
 * WdgM Integration Implementation
 ******************************************************************************/
#define GPT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief WdgM supervision timer callback
 */
void Gpt_WdgM_SupervisionCallback(void)
{
    /* Call WdgM checkpoint */
    WdgM_CheckpointReached();
    
    /* Restart timer if continuous mode */
    if (Gpt_WdgM_TimerActive) {
        /* Timer will be restarted automatically in continuous mode */
    }
}

/**
 * @brief Initialize WdgM supervision timer
 */
Std_ReturnType Gpt_WdgM_InitTimer(uint32 periodMs)
{
    if (periodMs == 0U) {
        return E_NOT_OK;
    }
    
    Gpt_WdgM_PeriodMs = periodMs;
    Gpt_WdgM_TimerActive = FALSE;
    
    return E_OK;
}

/**
 * @brief Start WdgM supervision timer
 */
Std_ReturnType Gpt_WdgM_StartTimer(void)
{
    Gpt_ValueType ticks;
    
    if (Gpt_WdgM_PeriodMs == 0U) {
        return E_NOT_OK;
    }
    
    /* Convert ms to ticks */
    ticks = Gpt_MsToTicks(GPT_WDGM_CHANNEL, Gpt_WdgM_PeriodMs);
    
    /* Start the timer */
    Gpt_StartTimer(GPT_WDGM_CHANNEL, ticks);
    Gpt_WdgM_TimerActive = TRUE;
    
    return E_OK;
}

/**
 * @brief Stop WdgM supervision timer
 */
Std_ReturnType Gpt_WdgM_StopTimer(void)
{
    Gpt_StopTimer(GPT_WDGM_CHANNEL);
    Gpt_WdgM_TimerActive = FALSE;
    
    return E_OK;
}

/**
 * @brief Get WdgM elapsed supervision time
 */
uint32 Gpt_WdgM_GetElapsedTime(void)
{
    Gpt_ValueType elapsed;
    
    elapsed = Gpt_GetTimeElapsed(GPT_WDGM_CHANNEL);
    return Gpt_TicksToMs(GPT_WDGM_CHANNEL, elapsed);
}

/******************************************************************************
 * EcuM Integration Implementation
 ******************************************************************************/

/**
 * @brief EcuM wakeup timer callback
 */
void Gpt_EcuM_WakeupCallback(void)
{
    /* Notify EcuM of wakeup event */
    EcuM_CheckWakeup();
}

/**
 * @brief Initialize EcuM timestamp timer
 */
Std_ReturnType Gpt_EcuM_InitTimestamp(void)
{
    Gpt_ValueType ticks;
    
    /* Configure for 1ms continuous operation */
    ticks = Gpt_MsToTicks(GPT_ECUM_CHANNEL, 1U);
    
    Gpt_EcuM_TimestampMs = 0U;
    Gpt_EcuM_TimestampMs64 = 0U;
    
    return E_OK;
}

/**
 * @brief Get current timestamp for EcuM
 */
uint32 Gpt_EcuM_GetTimestamp(void)
{
    Gpt_ValueType elapsed;
    uint32 timestamp;
    
    if (Gpt_ChannelStatus(GPT_ECUM_CHANNEL) == GPT_CH_STATUS_RUNNING) {
        elapsed = Gpt_GetTimeElapsed(GPT_ECUM_CHANNEL);
        timestamp = Gpt_EcuM_TimestampMs + Gpt_TicksToMs(GPT_ECUM_CHANNEL, elapsed);
    } else {
        timestamp = Gpt_EcuM_TimestampMs;
    }
    
    return timestamp;
}

/**
 * @brief Get extended timestamp for EcuM
 */
uint64 Gpt_EcuM_GetTimestamp64(void)
{
    Gpt_ValueType elapsed;
    uint64 timestamp;
    
    if (Gpt_ChannelStatus(GPT_ECUM_CHANNEL) == GPT_CH_STATUS_RUNNING) {
        elapsed = Gpt_GetTimeElapsed(GPT_ECUM_CHANNEL);
        timestamp = Gpt_EcuM_TimestampMs64 + Gpt_TicksToMs(GPT_ECUM_CHANNEL, elapsed);
    } else {
        timestamp = Gpt_EcuM_TimestampMs64;
    }
    
    return timestamp;
}

/**
 * @brief Set EcuM wakeup alarm
 */
Std_ReturnType Gpt_EcuM_SetWakeupAlarm(uint32 timeoutMs)
{
    Gpt_ValueType ticks;
    
    if (timeoutMs == 0U) {
        return E_NOT_OK;
    }
    
    ticks = Gpt_MsToTicks(GPT_ECUM_CHANNEL, timeoutMs);
    Gpt_StartTimer(GPT_ECUM_CHANNEL, ticks);
    
    return E_OK;
}

/**
 * @brief Cancel EcuM wakeup alarm
 */
Std_ReturnType Gpt_EcuM_CancelWakeupAlarm(void)
{
    Gpt_StopTimer(GPT_ECUM_CHANNEL);
    return E_OK;
}

/******************************************************************************
 * BswM Integration Implementation
 ******************************************************************************/

/**
 * @brief BswM timer callback
 */
void Gpt_BswM_TimerCallback(void)
{
    /* Call BswM main function */
    BswM_MainFunction();
}

/**
 * @brief Initialize BswM polling timer
 */
Std_ReturnType Gpt_BswM_InitTimer(uint32 periodMs)
{
    if (periodMs == 0U) {
        return E_NOT_OK;
    }
    
    Gpt_BswM_PeriodMs = periodMs;
    Gpt_BswM_TimerActive = FALSE;
    
    return E_OK;
}

/**
 * @brief Start BswM polling timer
 */
Std_ReturnType Gpt_BswM_StartTimer(void)
{
    Gpt_ValueType ticks;
    
    if (Gpt_BswM_PeriodMs == 0U) {
        return E_NOT_OK;
    }
    
    ticks = Gpt_MsToTicks(GPT_BSWM_CHANNEL, Gpt_BswM_PeriodMs);
    Gpt_StartTimer(GPT_BSWM_CHANNEL, ticks);
    Gpt_BswM_TimerActive = TRUE;
    
    return E_OK;
}

/**
 * @brief Stop BswM polling timer
 */
Std_ReturnType Gpt_BswM_StopTimer(void)
{
    Gpt_StopTimer(GPT_BSWM_CHANNEL);
    Gpt_BswM_TimerActive = FALSE;
    
    return E_OK;
}

/******************************************************************************
 * DDS Integration Implementation
 ******************************************************************************/

/**
 * @brief DDS timer callback
 */
void Gpt_Dds_TimerCallback(void)
{
    /* Update DDS timestamp base */
    Gpt_Dds_TimestampStart += Gpt_Dds_PeriodUs;
    
    /* Call DDS deadline monitoring */
    /* Dds_TimerCallback(); */
}

/**
 * @brief Initialize DDS high-resolution timer
 */
Std_ReturnType Gpt_Dds_InitTimer(uint32 periodUs)
{
    if (periodUs == 0U) {
        return E_NOT_OK;
    }
    
    Gpt_Dds_PeriodUs = periodUs;
    Gpt_Dds_TimerActive = FALSE;
    Gpt_Dds_TimestampStart = 0U;
    
    return E_OK;
}

/**
 * @brief Start DDS timer
 */
Std_ReturnType Gpt_Dds_StartTimer(void)
{
    Gpt_ValueType ticks;
    uint32 periodMs;
    uint32 remainderUs;
    
    if (Gpt_Dds_PeriodUs == 0U) {
        return E_NOT_OK;
    }
    
    /* Convert microseconds to ticks */
    periodMs = Gpt_Dds_PeriodUs / 1000U;
    remainderUs = Gpt_Dds_PeriodUs % 1000U;
    
    if (periodMs > 0U) {
        ticks = Gpt_MsToTicks(GPT_DDS_CHANNEL, periodMs);
    } else {
        /* Use minimum 1ms for software timers */
        ticks = Gpt_MsToTicks(GPT_DDS_CHANNEL, 1U);
    }
    
    Gpt_StartTimer(GPT_DDS_CHANNEL, ticks);
    Gpt_Dds_TimerActive = TRUE;
    
    return E_OK;
}

/**
 * @brief Stop DDS timer
 */
Std_ReturnType Gpt_Dds_StopTimer(void)
{
    Gpt_StopTimer(GPT_DDS_CHANNEL);
    Gpt_Dds_TimerActive = FALSE;
    
    return E_OK;
}

/**
 * @brief Get DDS high-resolution timestamp
 */
uint64 Gpt_Dds_GetTimestampUs(void)
{
    Gpt_ValueType elapsed;
    uint64 timestamp;
    
    if (Gpt_ChannelStatus(GPT_DDS_CHANNEL) == GPT_CH_STATUS_RUNNING) {
        elapsed = Gpt_GetTimeElapsed(GPT_DDS_CHANNEL);
        timestamp = Gpt_Dds_TimestampStart + Gpt_TicksToMs(GPT_DDS_CHANNEL, elapsed);
    } else {
        timestamp = Gpt_Dds_TimestampStart;
    }
    
    return timestamp;
}

/**
 * @brief Get DDS monotonic timestamp for RTPS
 */
uint64 Gpt_Dds_GetMonotonicTimestampNs(void)
{
    uint64 us;
    uint64 ns;
    
    us = Gpt_Dds_GetTimestampUs();
    ns = us * 1000U;  /* Convert us to ns */
    
    return ns;
}

/******************************************************************************
 * SchM Integration Implementation
 ******************************************************************************/

/**
 * @brief SchM timer callback
 */
void Gpt_SchM_ScheduleCallback(void)
{
    /* Schedule table processing would be called here */
    /* SchM_TimerCallback(); */
}

/**
 * @brief Initialize SchM schedule table timer
 */
Std_ReturnType Gpt_SchM_InitTimer(uint32 periodMs)
{
    (void)periodMs;
    return E_OK;
}

/**
 * @brief Start SchM schedule timer
 */
Std_ReturnType Gpt_SchM_StartTimer(void)
{
    return E_OK;
}

/**
 * @brief Stop SchM schedule timer
 */
Std_ReturnType Gpt_SchM_StopTimer(void)
{
    return E_OK;
}

/******************************************************************************
 * Det Integration Implementation
 ******************************************************************************/

/**
 * @brief GPT error hook for Det
 */
void Gpt_Det_ErrorHook(uint16 moduleId, uint8 instanceId, uint8 apiId, uint8 errorId)
{
    #if (GPT_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(moduleId, instanceId, apiId, errorId);
    #else
    (void)moduleId;
    (void)instanceId;
    (void)apiId;
    (void)errorId;
    #endif
}

/******************************************************************************
 * Dem Integration Implementation
 ******************************************************************************/

/**
 * @brief GPT error event for Dem
 */
void Gpt_Dem_ReportError(uint8 eventId)
{
    /* Report to Dem - to be implemented based on Dem interface */
    (void)eventId;
}

/******************************************************************************
 * MCU Integration Implementation
 ******************************************************************************/

/**
 * @brief MCU clock change notification
 */
void Gpt_Mcu_ClockChangeNotification(const void* clockSettings)
{
    /* Recalculate prescalers based on new clock settings */
    /* This would require re-initializing the GPT channels */
    (void)clockSettings;
}

/******************************************************************************
 * OS Integration Implementation
 ******************************************************************************/

/**
 * @brief Get GPT tick count for OS timing
 */
Gpt_ValueType Gpt_Os_GetTickCount(void)
{
    return Gpt_GetTimeElapsed(GPT_CHANNEL_0);
}

/**
 * @brief Convert OS ticks to GPT ticks
 */
Gpt_ValueType Gpt_Os_ConvertTicks(uint32 osTicks)
{
    Gpt_ValueType gptTicks;
    
    /* Conversion based on configured tick frequencies */
    /* Assuming OS tick = 1ms and GPT uses configured prescaler */
    gptTicks = Gpt_MsToTicks(GPT_CHANNEL_0, osTicks);
    
    return gptTicks;
}

#define GPT_STOP_SEC_CODE
#include "MemMap.h"

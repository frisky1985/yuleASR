/******************************************************************************
 * @file    Gpt_Integration.h
 * @brief   GPT (General Purpose Timer) Driver Integration Interface
 *
 * Integration interfaces for:
 * - WdgM (Watchdog Manager) - Supervision time base
 * - EcuM (ECU State Manager) - Timestamp functionality
 * - BswM (Basic Software Mode Manager) - Polling timer
 * - DDS (Data Distribution Service) - High-resolution timers
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024-2026
 ******************************************************************************/
#ifndef GPT_INTEGRATION_H
#define GPT_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "autosar/classic/gpt/gpt.h"
#include "Std_Types.h"

/******************************************************************************
 * WdgM Integration
 ******************************************************************************/

/**
 * @brief WdgM supervision timer callback
 * Called by GPT when WdgM supervision period expires
 */
void Gpt_WdgM_SupervisionCallback(void);

/**
 * @brief Initialize WdgM supervision timer
 * @param periodMs Supervision period in milliseconds
 * @return E_OK if successful
 */
Std_ReturnType Gpt_WdgM_InitTimer(uint32 periodMs);

/**
 * @brief Start WdgM supervision timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_WdgM_StartTimer(void);

/**
 * @brief Stop WdgM supervision timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_WdgM_StopTimer(void);

/**
 * @brief Get WdgM elapsed supervision time
 * @return Elapsed time in milliseconds
 */
uint32 Gpt_WdgM_GetElapsedTime(void);

/******************************************************************************
 * EcuM Integration
 ******************************************************************************/

/**
 * @brief EcuM wakeup timer callback
 * Called by GPT when wakeup timer expires
 */
void Gpt_EcuM_WakeupCallback(void);

/**
 * @brief Initialize EcuM timestamp timer
 * Provides continuous timestamp for EcuM
 * @return E_OK if successful
 */
Std_ReturnType Gpt_EcuM_InitTimestamp(void);

/**
 * @brief Get current timestamp for EcuM
 * @return Timestamp in milliseconds (32-bit wrapping)
 */
uint32 Gpt_EcuM_GetTimestamp(void);

/**
 * @brief Get extended timestamp for EcuM
 * @return Timestamp in milliseconds (64-bit non-wrapping)
 */
uint64 Gpt_EcuM_GetTimestamp64(void);

/**
 * @brief Set EcuM wakeup alarm
 * @param timeoutMs Wakeup timeout in milliseconds
 * @return E_OK if successful
 */
Std_ReturnType Gpt_EcuM_SetWakeupAlarm(uint32 timeoutMs);

/**
 * @brief Cancel EcuM wakeup alarm
 * @return E_OK if successful
 */
Std_ReturnType Gpt_EcuM_CancelWakeupAlarm(void);

/******************************************************************************
 * BswM Integration
 ******************************************************************************/

/**
 * @brief BswM timer callback
 * Called by GPT for BswM cyclic processing
 */
void Gpt_BswM_TimerCallback(void);

/**
 * @brief Initialize BswM polling timer
 * @param periodMs Polling period in milliseconds
 * @return E_OK if successful
 */
Std_ReturnType Gpt_BswM_InitTimer(uint32 periodMs);

/**
 * @brief Start BswM polling timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_BswM_StartTimer(void);

/**
 * @brief Stop BswM polling timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_BswM_StopTimer(void);

/******************************************************************************
 * DDS Timer Integration
 ******************************************************************************/

/**
 * @brief DDS timer callback
 * Called by GPT for DDS deadline monitoring
 */
void Gpt_Dds_TimerCallback(void);

/**
 * @brief Initialize DDS high-resolution timer
 * @param periodUs Timer period in microseconds
 * @return E_OK if successful
 */
Std_ReturnType Gpt_Dds_InitTimer(uint32 periodUs);

/**
 * @brief Start DDS timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_Dds_StartTimer(void);

/**
 * @brief Stop DDS timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_Dds_StopTimer(void);

/**
 * @brief Get DDS high-resolution timestamp
 * @return Timestamp in microseconds
 */
uint64 Gpt_Dds_GetTimestampUs(void);

/**
 * @brief Get DDS monotonic timestamp for RTPS
 * Used for DDS-Time_t in RTPS protocol
 * @return Monotonic timestamp in nanoseconds
 */
uint64 Gpt_Dds_GetMonotonicTimestampNs(void);

/******************************************************************************
 * SchM (Schedule Manager) Integration
 ******************************************************************************/

/**
 * @brief SchM timer callback for schedule table processing
 */
void Gpt_SchM_ScheduleCallback(void);

/**
 * @brief Initialize SchM schedule table timer
 * @param periodMs Schedule period in milliseconds
 * @return E_OK if successful
 */
Std_ReturnType Gpt_SchM_InitTimer(uint32 periodMs);

/**
 * @brief Start SchM schedule timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_SchM_StartTimer(void);

/**
 * @brief Stop SchM schedule timer
 * @return E_OK if successful
 */
Std_ReturnType Gpt_SchM_StopTimer(void);

/******************************************************************************
 * Det (Development Error Tracer) Integration
 ******************************************************************************/

/**
 * @brief GPT error hook for Det
 * Called when a development error is detected
 * @param moduleId Module ID where error occurred
 * @param instanceId Instance ID
 * @param apiId API ID where error occurred
 * @param errorId Error code
 */
void Gpt_Det_ErrorHook(uint16 moduleId, uint8 instanceId, uint8 apiId, uint8 errorId);

/******************************************************************************
 * Dem (Diagnostic Event Manager) Integration
 ******************************************************************************/

/**
 * @brief GPT error event for Dem
 * Called when a runtime error is detected
 * @param eventId Diagnostic event ID
 */
void Gpt_Dem_ReportError(uint8 eventId);

/******************************************************************************
 * MCU Integration
 ******************************************************************************/

/**
 * @brief MCU clock change notification
 * Called when MCU clock settings change
 * GPT must recalculate prescalers when clock changes
 * @param clockSettings Clock settings pointer
 */
void Gpt_Mcu_ClockChangeNotification(const void* clockSettings);

/******************************************************************************
 * OS Integration
 ******************************************************************************/

/**
 * @brief Get GPT tick count for OS timing
 * Used by OS for task scheduling
 * @return Current GPT tick count
 */
Gpt_ValueType Gpt_Os_GetTickCount(void);

/**
 * @brief Convert OS ticks to GPT ticks
 * @param osTicks OS tick count
 * @return GPT tick count
 */
Gpt_ValueType Gpt_Os_ConvertTicks(uint32 osTicks);

#ifdef __cplusplus
}
#endif

#endif /* GPT_INTEGRATION_H */

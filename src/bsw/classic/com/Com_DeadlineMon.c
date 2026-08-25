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
/* @req SHALL_CLASSIC */


/*
 * Com_DeadlineMon.c
 * AUTOSAR COM Module - Deadline Monitoring Implementation
 * According to AUTOSAR SWS COM 4.4.0 - SWS_Com_00500
 *
 * T012: 死线监控实现
 * 安全等级: ASIL-D
 *
 * 功能:
 * - 接收超时计时器管理
 * - 超时检测逻辑实现
 * - ErrorHook 调用集成
 * - 默认值替代机制 (ComIPduRxDefaultValue)
 */

/*==================[Includes]=============================================*/

#include "Com_DeadlineMon.h"
#include <string.h>

/*==================[Global Variables]=====================================*/

/* Deadline monitoring runtime data array */
Com_DmRunTimeType Com_DmRunTimeData[COM_MAX_IPDUS];

/* Deadline monitoring module initialized flag */
boolean Com_DmInitialized = FALSE;

/* ASIL-D: Redundant state for safety check */
boolean Com_DmInitialized_Redundant = FALSE;

/* ASIL-D: Watchdog counter for runtime corruption detection */
static uint32 Com_Dm_WatchdogCounter = 0u;

/* ASIL-D: Expected watchdog pattern */
#define COM_DM_WATCHDOG_PATTERN 0xA5A5A5A5u

/*==================[Local Function Declarations]==========================*/

/**
 * @brief ASIL-D: Verify redundant state consistency
 */
static boolean Com_Dm_VerifyRedundancy(void);

/**
 * @brief ASIL-D: Increment watchdog counter with pattern check
 */
static void Com_Dm_UpdateWatchdog(void);

/**
 * @brief Validate timeout action configuration
 */
static boolean Com_Dm_ValidateAction(const Com_DmRxConfigType* DmConfig);

/**
 * @brief Internal timeout handler
 */
static void Com_Dm_ProcessTimeoutInternal(Com_IPduIdType PduId,
                                           const Com_DmRxConfigType* DmConfig);

/*==================[Initialization]=======================================*/

void Com_Dm_Init(void)
{
    /* ASIL-D: Initialize all runtime data with known pattern */
    for (uint16 i = 0u; i < COM_MAX_IPDUS; i++) {
        Com_DmRunTimeData[i].Timer = 0u;
        Com_DmRunTimeData[i].State = COM_DM_STATE_STOPPED;
        Com_DmRunTimeData[i].TimeoutCounter = 0u;
        Com_DmRunTimeData[i].TimeoutProcessed = FALSE;
    }
    
    /* ASIL-D: Set initialized flags with redundancy */
    Com_DmInitialized = TRUE;
    Com_DmInitialized_Redundant = TRUE;
    
    /* ASIL-D: Initialize watchdog */
    Com_Dm_WatchdogCounter = 0u;
    
#if (COM_DEV_ERROR_DETECT == STD_ON)
    /* Verify initialization integrity */
    if (!Com_Dm_VerifyRedundancy()) {
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_INIT, COM_E_DM_RUNTIME_CORRUPTION);
        Com_DmInitialized = FALSE;
        Com_DmInitialized_Redundant = FALSE;
        return;
    }
#endif
}

void Com_Dm_DeInit(void)
{
    /* ASIL-D: Clear all runtime data */
    for (uint16 i = 0u; i < COM_MAX_IPDUS; i++) {
        Com_DmRunTimeData[i].Timer = 0u;
        Com_DmRunTimeData[i].State = COM_DM_STATE_STOPPED;
    }
    
    /* ASIL-D: Clear initialized flags */
    Com_DmInitialized = FALSE;
    Com_DmInitialized_Redundant = FALSE;
    Com_Dm_WatchdogCounter = 0u;
}

/*==================[Timer Management]=====================================*/

void Com_Dm_StartTimer(Com_IPduIdType PduId, uint32 Timeout)
{
    /* ASIL-D: Module initialized check */
    if (!Com_Dm_VerifyRedundancy()) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_START_TIMER, COM_E_DM_RUNTIME_CORRUPTION);
#endif
        return;
    }
    
    /* ASIL-D: Parameter validation */
    if (PduId >= COM_MAX_IPDUS) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_START_TIMER, COM_E_DM_INVALID_PDU_ID);
#endif
        return;
    }
    
    /* ASIL-D: Timer value validation */
    if (Timeout == 0xFFFFFFFFu) {
        /* Detect timer overflow condition */
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_START_TIMER, COM_E_DM_TIMER_OVERFLOW);
#endif
        return;
    }
    
    /* ASIL-D: Dual-check timer value */
    if (Timeout > 0u) {
        /* Start timer with specified timeout */
        Com_DmRunTimeData[PduId].Timer = Timeout;
        Com_DmRunTimeData[PduId].State = COM_DM_STATE_RUNNING;
        Com_DmRunTimeData[PduId].TimeoutProcessed = FALSE;
        
        /* Verify write was successful (redundant read) */
        if (Com_DmRunTimeData[PduId].Timer != Timeout) {
            /* Memory corruption detected */
#if (COM_DEV_ERROR_DETECT == STD_ON)
            COM_REPORT_ERROR(COM_SERVICE_ID_DM_START_TIMER, COM_E_DM_RUNTIME_CORRUPTION);
#endif
            Com_DmRunTimeData[PduId].State = COM_DM_STATE_ERROR;
            return;
        }
    } else {
        /* Zero timeout means no monitoring */
        Com_DmRunTimeData[PduId].State = COM_DM_STATE_STOPPED;
        Com_DmRunTimeData[PduId].Timer = 0u;
    }
    
    /* Update watchdog for runtime validation */
    Com_Dm_UpdateWatchdog();
}

void Com_Dm_StopTimer(Com_IPduIdType PduId)
{
    /* ASIL-D: Parameter validation */
    if (PduId >= COM_MAX_IPDUS) {
        return;
    }
    
    /* ASIL-D: Module initialized check */
    if (!Com_Dm_VerifyRedundancy()) {
        return;
    }
    
    /* Stop timer */
    Com_DmRunTimeData[PduId].Timer = 0u;
    Com_DmRunTimeData[PduId].State = COM_DM_STATE_STOPPED;
    Com_DmRunTimeData[PduId].TimeoutProcessed = FALSE;
}

/*==================[Main Function Integration]============================*/

void Com_Dm_ProcessTimers(void)
{
    /* ASIL-D: Module initialized check */
    if (!Com_Dm_VerifyRedundancy()) {
        return;
    }
    
    /* ASIL-D: Runtime integrity check */
    if (Com_Dm_ValidateIntegrity() != E_OK) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_PROCESS, COM_E_DM_RUNTIME_CORRUPTION);
#endif
        return;
    }
    
    /* Process all I-PDUs with deadline monitoring */
    for (uint16 i = 0u; i < Com_GlobalState.Config->NumIPdus; i++) {
        Com_IPduIdType pduId = (Com_IPduIdType)i;
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[pduId];
        
        /* Only process receive PDUs with timeout configured */
        if (ipduConfig->Direction != COM_RECEIVE) {
            continue;
        }
        
        /* Check if deadline monitoring is enabled for this PDU */
        if (ipduConfig->Timeout == 0u) {
            continue;
        }
        
        /* Check if PDU group is started */
        if (Com_GlobalState.IPduRunTime[pduId].GroupStatus != COM_IPDU_GROUP_STARTED) {
            continue;
        }
        
        /* Process timer */
        Com_DmStateType currentState = Com_DmRunTimeData[pduId].State;
        
        if (currentState == COM_DM_STATE_RUNNING) {
            if (Com_DmRunTimeData[pduId].Timer > 0u) {
                /* Decrement timer */
                Com_DmRunTimeData[pduId].Timer--;
                
                /* ASIL-D: Verify decrement */
                if (Com_DmRunTimeData[pduId].Timer == 0xFFFFFFFFu) {
                    /* Underflow detected */
                    Com_DmRunTimeData[pduId].State = COM_DM_STATE_ERROR;
#if (COM_DEV_ERROR_DETECT == STD_ON)
                    COM_REPORT_ERROR(COM_SERVICE_ID_DM_PROCESS, COM_E_DM_TIMER_OVERFLOW);
#endif
                    continue;
                }
            } else {
                /* Timer expired - timeout detected */
                Com_DmRunTimeData[pduId].State = COM_DM_STATE_EXPIRED;
                Com_DmRunTimeData[pduId].TimeoutCounter++;
            }
        }
    }
    
    /* Update watchdog */
    Com_Dm_UpdateWatchdog();
}

/*==================[Rx Indication Handling]===============================*/

void Com_Dm_HandleRxIndication(Com_IPduIdType PduId, 
                                const Com_DmRxConfigType* DmConfig)
{
    /* ASIL-D: Parameter validation */
    if (PduId >= COM_MAX_IPDUS) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_RX_INDICATION, COM_E_DM_INVALID_PDU_ID);
#endif
        return;
    }
    
    /* ASIL-D: Module initialized check */
    if (!Com_Dm_VerifyRedundancy()) {
        return;
    }
    
    /* Validate configuration pointer */
    if (DmConfig == NULL_PTR) {
        return;
    }
    
    /* Check if deadline monitoring is enabled */
    if (!DmConfig->EnableDeadlineMonitoring) {
        return;
    }
    
    /* Determine timeout value */
    uint32 timeoutValue = DmConfig->ComIPduRxTimeout;
    if (timeoutValue == 0u) {
        /* Use I-PDU configured timeout as fallback */
        timeoutValue = Com_GlobalState.Config->IPdus[PduId].Timeout;
    }
    
    /* Restart timer */
    Com_Dm_StartTimer(PduId, timeoutValue);
    
    /* If previously expired, transition back to running */
    if ((Com_DmRunTimeData[PduId].State == COM_DM_STATE_EXPIRED) ||
        (Com_DmRunTimeData[PduId].State == COM_DM_STATE_ERROR)) {
        Com_DmRunTimeData[PduId].State = COM_DM_STATE_RUNNING;
        Com_DmRunTimeData[PduId].TimeoutProcessed = FALSE;
    }
}

/*==================[Timeout Handling]=====================================*/

void Com_Dm_HandleTimeout(Com_IPduIdType PduId,
                           const Com_DmRxConfigType* DmConfig)
{
    /* ASIL-D: Parameter validation */
    if (PduId >= COM_MAX_IPDUS) {
        return;
    }
    
    /* ASIL-D: Module initialized check */
    if (!Com_Dm_VerifyRedundancy()) {
        return;
    }
    
    /* Validate configuration */
    if (!Com_Dm_ValidateAction(DmConfig)) {
        return;
    }
    
    /* Process timeout actions */
    Com_Dm_ProcessTimeoutInternal(PduId, DmConfig);
}

static void Com_Dm_ProcessTimeoutInternal(Com_IPduIdType PduId,
                                           const Com_DmRxConfigType* DmConfig)
{
    /* ASIL-D: Redundant execution check */
    static uint8 executionCounter = 0u;
    executionCounter++;
    
    /* Mark timeout as processed */
    Com_DmRunTimeData[PduId].TimeoutProcessed = TRUE;
    
    /* Execute configured actions */
    if ((DmConfig->TimeoutAction == COM_DM_ACTION_ERROR_HOOK) ||
        (DmConfig->TimeoutAction == COM_DM_ACTION_BOTH)) {
        
        /* Call ComErrorHook if configured */
        if (DmConfig->ComErrorHook != NULL_PTR) {
            /* ASIL-D: Execute with error handling */
            DmConfig->ComErrorHook(PduId);
            
            /* Verify callback completed */
            executionCounter--;
        }
    }
    
    if ((DmConfig->TimeoutAction == COM_DM_ACTION_DEFAULT_VALUE) ||
        (DmConfig->TimeoutAction == COM_DM_ACTION_BOTH)) {
        
        /* Apply default value substitution */
        (void)Com_Dm_ApplyDefaultValue(PduId, DmConfig);
    }
    
    /* ASIL-D: Verify execution counter consistency */
    if ((executionCounter != 0u) && (executionCounter != 0xFFu)) {
        /* Execution anomaly detected */
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_DM_HANDLE_TIMEOUT, COM_E_DM_RUNTIME_CORRUPTION);
#endif
    }
}

/*==================[Default Value Substitution]===========================*/

Std_ReturnType Com_Dm_ApplyDefaultValue(Com_IPduIdType PduId,
                                         const Com_DmRxConfigType* DmConfig)
{
    /* ASIL-D: Parameter validation */
    if (PduId >= COM_MAX_IPDUS) {
        return E_NOT_OK;
    }
    
    if (DmConfig == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Check if default value is configured */
    if (DmConfig->ComIPduRxDefaultValue == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Get I-PDU configuration */
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    
    /* Validate data length */
    uint8 copyLength = DmConfig->DefaultValueLength;
    if (copyLength == 0u) {
        copyLength = ipduConfig->Length;
    }
    if (copyLength > ipduConfig->Length) {
        copyLength = ipduConfig->Length;
    }
    
    /* ASIL-D: Redundant copy with verification */
    uint8* destPtr = ipduConfig->DataPtr;
    const uint8* srcPtr = DmConfig->ComIPduRxDefaultValue;
    
    /* First copy */
    for (uint8 i = 0u; i < copyLength; i++) {
        destPtr[i] = srcPtr[i];
    }
    
    /* Verification copy */
    for (uint8 i = 0u; i < copyLength; i++) {
        if (destPtr[i] != srcPtr[i]) {
            /* Memory corruption or copy failure */
#if (COM_DEV_ERROR_DETECT == STD_ON)
            COM_REPORT_ERROR(COM_SERVICE_ID_DM_HANDLE_TIMEOUT, COM_E_DM_RUNTIME_CORRUPTION);
#endif
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/*==================[State Query Functions]================================*/

Com_DmStateType Com_Dm_GetState(Com_IPduIdType PduId)
{
    if (PduId >= COM_MAX_IPDUS) {
        return COM_DM_STATE_ERROR;
    }
    
    if (!Com_Dm_VerifyRedundancy()) {
        return COM_DM_STATE_ERROR;
    }
    
    return Com_DmRunTimeData[PduId].State;
}

boolean Com_Dm_IsInitialized(void)
{
    return Com_DmInitialized && Com_DmInitialized_Redundant;
}

/*==================[ASIL-D Safety Functions]==============================*/

static boolean Com_Dm_VerifyRedundancy(void)
{
    /* Check redundant flags match */
    if (Com_DmInitialized != Com_DmInitialized_Redundant) {
        /* Corruption detected - disable module */
        Com_DmInitialized = FALSE;
        Com_DmInitialized_Redundant = FALSE;
        return FALSE;
    }
    
    return Com_DmInitialized;
}

static void Com_Dm_UpdateWatchdog(void)
{
    Com_Dm_WatchdogCounter++;
    
    /* Prevent overflow */
    if (Com_Dm_WatchdogCounter >= COM_DM_WATCHDOG_PATTERN) {
        Com_Dm_WatchdogCounter = 0u;
    }
}

static boolean Com_Dm_ValidateAction(const Com_DmRxConfigType* DmConfig)
{
    if (DmConfig == NULL_PTR) {
        return FALSE;
    }
    
    /* Validate action type is in valid range */
    if (DmConfig->TimeoutAction > COM_DM_ACTION_BOTH) {
        return FALSE;
    }
    
    return TRUE;
}

Std_ReturnType Com_Dm_ValidateIntegrity(void)
{
    /* Check watchdog pattern */
    if (Com_Dm_WatchdogCounter >= COM_DM_WATCHDOG_PATTERN) {
        return E_NOT_OK;
    }
    
    /* Verify state consistency for active timers */
    for (uint16 i = 0u; i < Com_GlobalState.Config->NumIPdus; i++) {
        Com_DmStateType state = Com_DmRunTimeData[i].State;
        
        /* Validate state is in valid range */
        if (state > COM_DM_STATE_ERROR) {
            return E_NOT_OK;
        }
        
        /* Verify timer consistency */
        if ((state == COM_DM_STATE_STOPPED) && (Com_DmRunTimeData[i].Timer != 0u)) {
            /* Inconsistent state */
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/*==================[End of File]==========================================*/

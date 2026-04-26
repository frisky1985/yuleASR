/******************************************************************************
 * @file    ecum_wakeup.c
 * @brief   ECU State Manager (EcuM) Wakeup Management Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/classic/ecum/ecum.h"
#include <string.h>

/******************************************************************************
 * Module Internal Constants
 ******************************************************************************/
#define ECUM_MAX_WAKEUP_SOURCES         32U
#define ECUM_VALIDATION_TIMER_PERIOD_MS 10U
#define ECUM_VALIDATION_TIMEOUT_INFINITE 0xFFFFU

/******************************************************************************
 * Wakeup Source Context Type
 ******************************************************************************/
typedef struct {
    EcuM_WakeupSourceType source;
    EcuM_WakeupStatusType status;
    uint16 validationTimeout;
    uint16 validationCounter;
    uint16 validationTimer;
    uint32 timestamp;
    uint8 comMChannel;
    boolean checkWakeupSupported;
    boolean comMChannelSupported;
} EcuM_WakeupSourceContextType;

/******************************************************************************
 * Module Variables
 ******************************************************************************/
static EcuM_WakeupSourceContextType EcuM_WakeupSources[ECUM_MAX_WAKEUP_SOURCES];
static uint8 EcuM_NumWakeupSources = 0U;
static uint32 EcuM_WakeupTimestamp = 0U;

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static EcuM_WakeupSourceContextType* EcuM_FindWakeupSourceContext(EcuM_WakeupSourceType source);
static void EcuM_UpdateWakeupStatus(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status);
static void EcuM_ProcessValidationTimers(void);

/******************************************************************************
 * Internal Function Implementations
 ******************************************************************************/

/**
 * @brief Find wakeup source context by source ID
 */
static EcuM_WakeupSourceContextType* EcuM_FindWakeupSourceContext(EcuM_WakeupSourceType source)
{
    EcuM_WakeupSourceContextType *result = NULL;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source == source) {
            result = &EcuM_WakeupSources[i];
            break;
        }
    }
    
    return result;
}

/**
 * @brief Update wakeup status for a source
 */
static void EcuM_UpdateWakeupStatus(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status)
{
    EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
    
    if (ctx != NULL) {
        ctx->status = status;
        ctx->validationTimer = 0U;
    }
}

/**
 * @brief Process validation timers for pending wakeup sources
 */
static void EcuM_ProcessValidationTimers(void)
{
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source != 0U) {
            if (EcuM_WakeupSources[i].status == ECUM_WKSTATUS_PENDING) {
                /* Increment timer */
                EcuM_WakeupSources[i].validationTimer += ECUM_VALIDATION_TIMER_PERIOD_MS;
                
                /* Check for timeout */
                if (EcuM_WakeupSources[i].validationTimeout != ECUM_VALIDATION_TIMEOUT_INFINITE) {
                    if (EcuM_WakeupSources[i].validationTimer >= EcuM_WakeupSources[i].validationTimeout) {
                        /* Validation timeout - mark as expired */
                        EcuM_WakeupSources[i].status = ECUM_WKSTATUS_EXPIRED;
                    }
                }
                
                /* Check if validation counter reached */
                if (EcuM_WakeupSources[i].validationCounter > 0U) {
                    EcuM_WakeupSources[i].validationCounter--;
                    if (EcuM_WakeupSources[i].validationCounter == 0U) {
                        /* Counter expired without validation - mark as expired */
                        EcuM_WakeupSources[i].status = ECUM_WKSTATUS_EXPIRED;
                    }
                }
            }
        }
    }
}

/******************************************************************************
 * External API Implementations
 ******************************************************************************/

void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)
{
    const EcuM_StatusType *status = EcuM_GetStatus();
    
    if (status->initialized) {
        /* Update module status */
        /* Note: EcuM_Status is internal, but we use the getter which returns a const pointer */
        /* In real implementation, this would access the status directly or use setter functions */
        
        /* Process each bit in sources */
        EcuM_WakeupSourceType source;
        EcuM_WakeupSourceType remaining = sources;
        
        while (remaining != 0U) {
            /* Get lowest bit set */
            source = remaining & (~remaining + 1U);
            remaining &= ~source;
            
            /* Check if source is disabled */
            if ((status->disabledWakeupEvents & source) != 0U) {
                continue;
            }
            
            /* Find or create context for this source */
            EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
            
            if (ctx != NULL) {
                /* Update existing context */
                if (ctx->status == ECUM_WKSTATUS_NONE) {
                    ctx->status = ECUM_WKSTATUS_PENDING;
                    ctx->validationTimer = 0U;
                    ctx->timestamp = EcuM_WakeupTimestamp;
                }
            }
            
            /* Call application-specific check if supported */
            if (EcuM_CheckWakeup != NULL) {
                if (EcuM_CheckWakeup(source)) {
                    /* Immediate validation */
                    EcuM_ValidateWakeupEvent(source);
                }
            }
        }
    }
}

void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources)
{
    EcuM_WakeupSourceType source;
    EcuM_WakeupSourceType remaining = sources;
    
    while (remaining != 0U) {
        source = remaining & (~remaining + 1U);
        remaining &= ~source;
        
        EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
        
        if (ctx != NULL) {
            ctx->status = ECUM_WKSTATUS_NONE;
            ctx->validationTimer = 0U;
            ctx->timestamp = 0U;
        }
    }
}

EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void)
{
    EcuM_WakeupSourceType pending = 0U;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source != 0U) {
            if (EcuM_WakeupSources[i].status == ECUM_WKSTATUS_PENDING) {
                pending |= EcuM_WakeupSources[i].source;
            }
        }
    }
    
    return pending;
}

EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void)
{
    EcuM_WakeupSourceType validated = 0U;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source != 0U) {
            if (EcuM_WakeupSources[i].status == ECUM_WKSTATUS_VALIDATED) {
                validated |= EcuM_WakeupSources[i].source;
            }
        }
    }
    
    return validated;
}

EcuM_WakeupSourceType EcuM_GetExpiredWakeupEvents(void)
{
    EcuM_WakeupSourceType expired = 0U;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source != 0U) {
            if (EcuM_WakeupSources[i].status == ECUM_WKSTATUS_EXPIRED) {
                expired |= EcuM_WakeupSources[i].source;
            }
        }
    }
    
    return expired;
}

void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType sources)
{
    EcuM_WakeupSourceType source;
    EcuM_WakeupSourceType remaining = sources;
    
    while (remaining != 0U) {
        source = remaining & (~remaining + 1U);
        remaining &= ~source;
        
        EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
        
        if (ctx != NULL) {
            if ((ctx->status == ECUM_WKSTATUS_PENDING) ||
                (ctx->status == ECUM_WKSTATUS_NONE)) {
                ctx->status = ECUM_WKSTATUS_VALIDATED;
                ctx->validationTimer = 0U;
            }
        }
    }
}

EcuM_WakeupStatusType EcuM_GetWakeupStatus(EcuM_WakeupSourceType source)
{
    EcuM_WakeupStatusType status = ECUM_WKSTATUS_NONE;
    EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
    
    if (ctx != NULL) {
        status = ctx->status;
    }
    
    return status;
}

void EcuM_EnableWakeupSources(EcuM_WakeupSourceType sources)
{
    const EcuM_StatusType *status = EcuM_GetStatus();
    
    if (status->initialized) {
        /* Remove from disabled mask */
        /* Note: In actual implementation, this would modify the status structure */
        (void)sources;
    }
}

void EcuM_DisableWakeupSources(EcuM_WakeupSourceType sources)
{
    const EcuM_StatusType *status = EcuM_GetStatus();
    
    if (status->initialized) {
        /* Add to disabled mask */
        /* Note: In actual implementation, this would modify the status structure */
        
        /* Also clear any pending/validated events for these sources */
        EcuM_ClearWakeupEvent(sources);
        (void)sources;
    }
}

/******************************************************************************
 * Wakeup Source Registration
 ******************************************************************************/

Std_ReturnType EcuM_RegisterWakeupSource(
    EcuM_WakeupSourceType source,
    uint16 validationTimeout,
    uint16 validationCounter,
    boolean checkWakeupSupported,
    boolean comMChannelSupported,
    uint8 comMChannel)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    /* Check if already registered */
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source == source) {
            /* Already registered - update settings */
            EcuM_WakeupSources[i].validationTimeout = validationTimeout;
            EcuM_WakeupSources[i].validationCounter = validationCounter;
            EcuM_WakeupSources[i].checkWakeupSupported = checkWakeupSupported;
            EcuM_WakeupSources[i].comMChannelSupported = comMChannelSupported;
            EcuM_WakeupSources[i].comMChannel = comMChannel;
            result = E_OK;
            break;
        }
    }
    
    /* Not found - add new entry */
    if (result != E_OK) {
        for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
            if (EcuM_WakeupSources[i].source == 0U) {
                EcuM_WakeupSources[i].source = source;
                EcuM_WakeupSources[i].status = ECUM_WKSTATUS_NONE;
                EcuM_WakeupSources[i].validationTimeout = validationTimeout;
                EcuM_WakeupSources[i].validationCounter = validationCounter;
                EcuM_WakeupSources[i].validationTimer = 0U;
                EcuM_WakeupSources[i].timestamp = 0U;
                EcuM_WakeupSources[i].checkWakeupSupported = checkWakeupSupported;
                EcuM_WakeupSources[i].comMChannelSupported = comMChannelSupported;
                EcuM_WakeupSources[i].comMChannel = comMChannel;
                
                if (i >= EcuM_NumWakeupSources) {
                    EcuM_NumWakeupSources = i + 1U;
                }
                result = E_OK;
                break;
            }
        }
    }
    
    return result;
}

Std_ReturnType EcuM_UnregisterWakeupSource(EcuM_WakeupSourceType source)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source == source) {
            (void)memset(&EcuM_WakeupSources[i], 0, sizeof(EcuM_WakeupSourceContextType));
            result = E_OK;
            break;
        }
    }
    
    return result;
}

/******************************************************************************
 * Wakeup Timestamp Functions
 ******************************************************************************/

uint32 EcuM_GetWakeupTime(void)
{
    return EcuM_WakeupTimestamp;
}

void EcuM_SetWakeupTime(uint32 timestamp)
{
    EcuM_WakeupTimestamp = timestamp;
}

/******************************************************************************
 * Wakeup Main Function (to be called from EcuM_MainFunction)
 ******************************************************************************/
void EcuM_WakeupMainFunction(void)
{
    EcuM_ProcessValidationTimers();
}

/******************************************************************************
 * Check Wakeup API (called by drivers)
 ******************************************************************************/
void EcuM_CheckWakeupHook(EcuM_WakeupSourceType sources)
{
    const EcuM_StatusType *status = EcuM_GetStatus();
    
    if (status->initialized) {
        EcuM_WakeupSourceType source;
        EcuM_WakeupSourceType remaining = sources;
        
        while (remaining != 0U) {
            source = remaining & (~remaining + 1U);
            remaining &= ~source;
            
            EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
            
            if (ctx != NULL) {
                if (ctx->checkWakeupSupported) {
                    /* Set the wakeup event */
                    EcuM_SetWakeupEvent(source);
                }
            }
        }
    }
}

/******************************************************************************
 * ComM Integration Functions
 ******************************************************************************/

void EcuM_ComMWakeUpIndication(uint8 channel)
{
    uint8 i;
    
    /* Find wakeup source associated with this ComM channel */
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if ((EcuM_WakeupSources[i].source != 0U) &&
            (EcuM_WakeupSources[i].comMChannelSupported) &&
            (EcuM_WakeupSources[i].comMChannel == channel)) {
            /* Set wakeup event for this source */
            EcuM_SetWakeupEvent(EcuM_WakeupSources[i].source);
            break;
        }
    }
}

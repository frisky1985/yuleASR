/*******************************************************************************
 * EcuM 运行/睡眠/唤醒实现
 * 自动拆分自 EcuM.c
 ******************************************************************************/
#define ECUM_START_SEC_CODE
#include "BswM.h"
#include "SchM.h"
#include "MemMap.h"

void EcuM_MainFunction(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_MAINFUNCTION_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    EcuM_MainFunctionCounter++;
    
    /* Process based on current state */
    switch (EcuM_CurrentState)
    {
        case ECUM_STATE_RUN:
            EcuM_ProcessRun();
            break;
            
        case ECUM_STATE_POST_RUN:
            EcuM_ProcessPostRun();
            break;
            
        case ECUM_STATE_SLEEP:
            EcuM_ProcessSleep();
            break;
            
        case ECUM_STATE_SHUTDOWN:
            /* Shutdown is handled sequentially, not cyclically */
            break;
            
        default:
            /* Invalid state - should not happen */
            break;
    }
    
    /* Update wakeup validation timers */
    EcuM_ValidateWakeupSources();
    EcuM_ExpireWakeupSources();
}

/**
 * @brief Process RUN state
 * @details Monitor run requests, handle normal operation
 */
void EcuM_ProcessRun(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_RUN);
    
    /* Check if all RUN requests are released */
    EcuM_CheckRunRequests();
    
    /* Set wakeup events based on pending interrupts */
    /* This would be checked via hardware registers or interrupt flags */
}

/**
 * @brief Process POST_RUN state
 * @details Handle transition from RUN to SLEEP or SHUTDOWN
 */
void EcuM_ProcessPostRun(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_POST_RUN);
    
    /* Wait for all PostRun activities to complete */
    /* Then transition based on shutdown target */
    
    switch (EcuM_ShutdownTarget)
    {
        case ECUM_SHUTDOWN_TARGET_SLEEP:
            EcuM_CurrentState = ECUM_STATE_SLEEP;
            EcuM_UpdateSubState(ECUM_SUBSTATE_GO_SLEEP);
            EcuM_GoSleep();
            break;
            
        case ECUM_SHUTDOWN_TARGET_OFF:
        case ECUM_SHUTDOWN_TARGET_RESET:
            EcuM_CurrentState = ECUM_STATE_SHUTDOWN;
            EcuM_UpdateSubState(ECUM_SUBSTATE_GO_OFF_ONE);
            EcuM_Shutdown();
            break;
            
        default:
            /* Invalid target */
            break;
    }
}

/*******************************************************************************
 *                              Sleep Management                               *
 ******************************************************************************/

/**
 * @brief Go to Sleep mode
 * @details Prepare and enter sleep mode
 */
void EcuM_GoSleep(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SLEEP_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
    
    if (EcuM_CurrentState != ECUM_STATE_SLEEP)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SLEEP_SID, ECUM_E_STATE_CHANGE_FAILED);
        return;
    }
#endif
    
    EcuM_ProcessGoSleep();
}

/**
 * @brief Go to Halt mode
 * @details Enter halt mode (CPU clock stopped)
 */
void EcuM_GoHalt(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_HALT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
    
    if (EcuM_CurrentState != ECUM_STATE_SLEEP)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_HALT_SID, ECUM_E_STATE_CHANGE_FAILED);
        return;
    }
#endif
    
#if (ECUM_HALT_MODE_SUPPORTED == STD_ON)
    EcuM_ProcessHalt();
#endif
}

/**
 * @brief Go to Poll mode
 * @details Enter poll mode (active wait)
 */
void EcuM_GoPoll(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_POLL_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
    
    if (EcuM_CurrentState != ECUM_STATE_SLEEP)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_POLL_SID, ECUM_E_STATE_CHANGE_FAILED);
        return;
    }
#endif
    
#if (ECUM_POLL_MODE_SUPPORTED == STD_ON)
    EcuM_ProcessPoll();
#endif
}

/**
 * @brief Process GoSleep state
 * @details Prepare BSW modules for sleep entry
 */
void EcuM_ProcessGoSleep(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_GO_SLEEP);
    
    /* Notify BswM about preparing for sleep */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_EcuM_CurrentState(ECUM_STATE_SLEEP);
#endif
    
    /* De-initialize RTE */
#if (ECUM_RTE_ENABLED == STD_ON)
    /* Rte_Stop(); */
#endif
    
    /* Release ComM channels */
#if (ECUM_COMM_ENABLED == STD_ON)
    /* Release all ComM channels */
#endif
    
    /* Wait for NvM to complete any pending operations */
#if (ECUM_NVM_ENABLED == STD_ON)
    /* NvM_CancelWriteAll(); */
#endif
    
    /* Disable watchdog */
#if (ECUM_WDGM_ENABLED == STD_ON)
    /* WdgM_DeInit(); */
#endif
    
    /* Enter sleep mode */
    EcuM_PerformSleep();
}

/**
 * @brief Process Sleep state
 * @details Handle sleep mode operation
 */
void EcuM_ProcessSleep(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_SLEEP);
    
    /* Sleep mode is entered - waiting for wakeup event */
    /* This function may not be called cyclically during actual sleep */
    
    /* Check for pending wakeup events */
    if (EcuM_PendingWakeupEvents != ECUM_WKSOURCE_NONE)
    {
        /* Wakeup detected - restart sequence */
        EcuM_WakeupRestart();
    }
}

/**
 * @brief Process Halt state
 * @details CPU is halted, waiting for interrupt
 */
void EcuM_ProcessHalt(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_HALT);
    
    /* Enable wakeup sources before halting */
    EcuM_EnableWakeupSources(EcuM_EnabledWakeupSources);
    
    /* Execute HALT instruction - CPU stops here */
    /* This is typically implemented in Mcu module */
    /* Mcu_PerformReset(); */
    
    /* After wakeup, execution continues here */
    EcuM_WakeupRestart();
}

/**
 * @brief Process Poll state
 * @details Active wait for wakeup
 */
void EcuM_ProcessPoll(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_POLL);
    
    /* In poll mode, we continuously check for wakeup */
    /* This consumes more power than halt mode */
    
    /* Poll wakeup sources */
    EcuM_CheckWakeup(EcuM_EnabledWakeupSources);
    
    /* If wakeup detected, restart */
    if (EcuM_PendingWakeupEvents != ECUM_WKSOURCE_NONE)
    {
        EcuM_WakeupRestart();
    }
}

/**
 * @brief Wakeup Restart sequence
 * @details Handle wakeup from sleep
 */
void EcuM_WakeupRestart(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_WAKEUPRESTART_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Transition to wakeup state */
    EcuM_CurrentState = ECUM_STATE_WAKE_SLEEP;
    EcuM_UpdateSubState(ECUM_SUBSTATE_WAKEUP_ONE);
    
    /* Process Wakeup One */
    EcuM_ProcessWakeupOne();
}

/**
 * @brief Process Wakeup One state
 * @details Initialize drivers after wakeup
 */
void EcuM_ProcessWakeupOne(void)
{
    /* Re-initialize MCU and essential drivers */
    EcuM_DriverRestart(EcuM_ConfigPtr);
    
    /* Validate wakeup sources */
    EcuM_AL_WakeupValidation();
    
    /* Transition to Wakeup Two */
    EcuM_UpdateSubState(ECUM_SUBSTATE_WAKEUP_TWO);
    EcuM_ProcessWakeupTwo();
}

/**
 * @brief Process Wakeup Two state
 * @details Re-initialize OS and BSW after wakeup
 */
void EcuM_ProcessWakeupTwo(void)
{
    /* Re-initialize OS if needed */
    
    /* Re-initialize SchM */
#if (ECUM_SCHM_ENABLED == STD_ON)
    SchM_Init();
#endif
    
    /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    BswM_EcuM_CurrentWakeup(EcuM_ValidatedWakeupEvents, ECUM_WKSTATUS_VALIDATED);
#endif
    
    /* Re-initialize communication */
#if (ECUM_COMM_ENABLED == STD_ON)
    ComM_Init();
#endif
    
    /* Re-initialize RTE */
#if (ECUM_RTE_ENABLED == STD_ON)
    /* Rte_Start(); */
#endif
    
    /* Transition back to RUN */
    EcuM_CurrentState = ECUM_STATE_RUN;
    EcuM_UpdateSubState(ECUM_SUBSTATE_RUN);
    
    /* Notify application about wakeup */
    EcuM_AL_WakeupReaction();
}

/*******************************************************************************
 *                              Shutdown Management                            *
 ******************************************************************************/

/**
 * @brief Shutdown sequence
 * @details Initiate shutdown sequence
 */
Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_REQUESTRUN_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (user >= ECUM_MAX_USERS)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_REQUESTRUN_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    /* Set the bit for this user */
    EcuM_RunRequests |= (1u << user);
    
    /* Check if transitioning from POST_RUN back to RUN */
    if (EcuM_CurrentState == ECUM_STATE_POST_RUN)
    {
        EcuM_CurrentState = ECUM_STATE_RUN;
        EcuM_UpdateSubState(ECUM_SUBSTATE_RUN);
    }
    
    return E_OK;
}

/**
 * @brief Release RUN mode request
 * @param user User releasing RUN mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_RELEASERUN_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (user >= ECUM_MAX_USERS)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_RELEASERUN_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    /* Clear the bit for this user */
    EcuM_RunRequests &= ~(1u << user);
    
    /* Check run requests */
    EcuM_CheckRunRequests();
    
    return E_OK;
}

/**
 * @brief Kill all RUN requests
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_KillAllRUNRequests(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_KILLALLRUNREQUESTS_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* Save killed requests for potential recovery */
    EcuM_KilledRunRequests = EcuM_RunRequests;
    
    /* Clear all requests */
    EcuM_RunRequests = 0u;
    
    /* Check if transition needed */
    EcuM_CheckRunRequests();
    
    return E_OK;
}

/**
 * @brief Check RUN requests and transition state if needed
 */
void EcuM_CheckRunRequests(void)
{
    if (EcuM_RunRequests == 0u)
    {
        /* No more RUN requests - transition to POST_RUN */
        if (EcuM_CurrentState == ECUM_STATE_RUN)
        {
            EcuM_CurrentState = ECUM_STATE_POST_RUN;
            EcuM_UpdateSubState(ECUM_SUBSTATE_POST_RUN);
            
#if (ECUM_BSWM_ENABLED == STD_ON)
            BswM_EcuM_CurrentState(ECUM_STATE_POST_RUN);
#endif
        }
    }
}

/*******************************************************************************
 *                              State Queries                                  *
 ******************************************************************************/

/**
 * @brief Get current ECU state
 * @param state Pointer to store state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SETWAKEUPEVENT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Only process enabled wakeup sources */
    sources &= EcuM_EnabledWakeupSources;
    
    if (sources != ECUM_WKSOURCE_NONE)
    {
        /* Add to pending events */
        EcuM_PendingWakeupEvents |= sources;
        
        /* Set status to pending */
        uint8 i;
        for (i = 0u; i < ECUM_MAX_WAKEUP_SOURCES; i++)
        {
            if (sources & (1u << i))
            {
                if (EcuM_WakeupStatus[i] == ECUM_WKSTATUS_NONE)
                {
                    EcuM_WakeupStatus[i] = ECUM_WKSTATUS_PENDING;
                    EcuM_WakeupValidationTimer[i] = ECUM_WAKEUP_VALIDATION_TIMEOUT / ECUM_MAIN_FUNCTION_PERIOD;
                }
            }
        }
        
        /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
        BswM_EcuM_CurrentWakeup(sources, ECUM_WKSTATUS_PENDING);
#endif
    }
}

/**
 * @brief Clear wakeup event
 * @param sources Wakeup source bitmask to clear
 */
void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_CLEARWAKEUPEVENT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Clear from pending and validated events */
    EcuM_PendingWakeupEvents &= ~sources;
    EcuM_ValidatedWakeupEvents &= ~sources;
    EcuM_ExpiredWakeupEvents &= ~sources;
    
    /* Clear status */
    uint8 i;
    for (i = 0u; i < ECUM_MAX_WAKEUP_SOURCES; i++)
    {
        if (sources & (1u << i))
        {
            EcuM_WakeupStatus[i] = ECUM_WKSTATUS_NONE;
            EcuM_WakeupValidationTimer[i] = 0u;
        }
    }
}

/**
 * @brief Check wakeup sources
 * @param sources Wakeup sources to check
 */
void EcuM_CheckWakeup(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_CHECKWAKEUP_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Check each wakeup source */
    /* This would typically call driver-specific CheckWakeup functions */
    
#if (ECUM_CHECK_WAKEUP_ENABLED == STD_ON)
    /* EcuM_AL_CheckWakeup would be called here */
    (void)sources;
#endif
}

/**
 * @brief Enable wakeup sources
 * @param sources Wakeup source bitmask
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_EnableWakeupSources(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_ENABLEWAKEUPSOURCES_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* Add to enabled sources */
    EcuM_EnabledWakeupSources |= sources;
    
    /* Enable in hardware - call integrator callout */
    /* EcuM_AL_EnableWakeupSources(sources); */
    
    return E_OK;
}

/**
 * @brief Disable wakeup sources
 * @param sources Wakeup source bitmask
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_DisableWakeupSources(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_DISABLEWAKEUPSOURCES_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* Remove from enabled sources */
    EcuM_EnabledWakeupSources &= ~sources;
    
    /* Disable in hardware - call integrator callout */
    /* EcuM_AL_DisableWakeupSources(sources); */
    
    return E_OK;
}

/**
 * @brief Get status of wakeup source
 * @param sources Wakeup source (single bit)
 * @return Wakeup status
 */
EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATUSOFWAKEUPSOURCE_SID, ECUM_E_NOT_INITIALIZED);
        return ECUM_WKSTATUS_NONE;
    }
    
    if ((sources == ECUM_WKSOURCE_NONE) || ((sources & (sources - 1u)) != 0u))
    {
        /* Multiple or no sources specified */
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATUSOFWAKEUPSOURCE_SID, ECUM_E_INVALID_PAR);
        return ECUM_WKSTATUS_NONE;
    }
#endif
    
    uint8 index = EcuM_GetWakeupSourceIndex(sources);
    
    if (index < ECUM_MAX_WAKEUP_SOURCES)
    {
        return EcuM_WakeupStatus[index];
    }
    
    return ECUM_WKSTATUS_NONE;
}

/**
 * @brief Get all pending/validated wakeup sources
 * @param sources Pointer to store sources
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetWakeupSources(EcuM_WakeupSourceType* sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETWAKEUPSOURCES_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (sources == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETWAKEUPSOURCES_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *sources = EcuM_PendingWakeupEvents | EcuM_ValidatedWakeupEvents;
    return E_OK;
}

/**
 * @brief Check validation of wakeup source
 * @param source Single wakeup source
 * @return E_OK if validated, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_CheckValidation(EcuM_WakeupSourceType source)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_VALIDATEMCUWAKEUPEVENT_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    uint8 index = EcuM_GetWakeupSourceIndex(source);
    
    if ((index < ECUM_MAX_WAKEUP_SOURCES) && 
        (EcuM_WakeupStatus[index] == ECUM_WKSTATUS_VALIDATED))
    {
        return E_OK;
    }
    
    return E_NOT_OK;
}

/*******************************************************************************
 *                          Wakeup Helper Functions                            *
 ******************************************************************************/

/**
 * @brief Validate pending wakeup sources
 * @details Called periodically to validate pending wakeups
 */
void EcuM_ValidateWakeupSources(void)
{
    uint8 i;
    EcuM_WakeupSourceType validatedSources = 0u;
    
    for (i = 0u; i < ECUM_MAX_WAKEUP_SOURCES; i++)
    {
        if (EcuM_WakeupStatus[i] == ECUM_WKSTATUS_PENDING)
        {
            if (EcuM_WakeupValidationTimer[i] > 0u)
            {
                EcuM_WakeupValidationTimer[i]--;
                
                /* Check if source is valid */
                /* In real implementation, this would check hardware state */
                if (EcuM_WakeupValidationTimer[i] == 0u)
                {
                    /* Validation successful - move to validated */
                    EcuM_WakeupStatus[i] = ECUM_WKSTATUS_VALIDATED;
                    validatedSources |= (1u << i);
                }
            }
        }
    }
    
    if (validatedSources != 0u)
    {
        EcuM_PendingWakeupEvents &= ~validatedSources;
        EcuM_ValidatedWakeupEvents |= validatedSources;
        
        /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
        BswM_EcuM_CurrentWakeup(validatedSources, ECUM_WKSTATUS_VALIDATED);
#endif
    }
}

/**
 * @brief Expire wakeup sources that failed validation
 */
void EcuM_ExpireWakeupSources(void)
{
    uint8 i;
    EcuM_WakeupSourceType expiredSources = 0u;
    
    for (i = 0u; i < ECUM_MAX_WAKEUP_SOURCES; i++)
    {
        if (EcuM_WakeupStatus[i] == ECUM_WKSTATUS_PENDING)
        {
            /* If validation timer expired without validation, mark as expired */
            if (EcuM_WakeupValidationTimer[i] == 0u)
            {
                EcuM_WakeupStatus[i] = ECUM_WKSTATUS_EXPIRED;
                expiredSources |= (1u << i);
            }
        }
    }
    
    if (expiredSources != 0u)
    {
        EcuM_PendingWakeupEvents &= ~expiredSources;
        EcuM_ExpiredWakeupEvents |= expiredSources;
        
        /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
        BswM_EcuM_CurrentWakeup(expiredSources, ECUM_WKSTATUS_EXPIRED);
#endif
    }
}

/**
 * @brief Get index of wakeup source
 * @param source Wakeup source bitmask (single bit)
 * @return Index of source, or 0xFF if invalid
 */
uint8 EcuM_GetWakeupSourceIndex(EcuM_WakeupSourceType source)
{
    uint8 index = 0u;
    
    while ((source > 1u) && (index < ECUM_MAX_WAKEUP_SOURCES))
    {
        source >>= 1u;
        index++;
    }
    
    return index;
}

/**
 * @brief Check if wakeup source is valid
 * @param source Wakeup source to check
 * @return TRUE if valid, FALSE otherwise
 */
boolean EcuM_IsValidWakeupSource(EcuM_WakeupSourceType source)
{
    /* Check if source is configured */
    return ((source & ECUM_CONFIGURED_WAKEUP_SOURCES) != 0u) ? TRUE : FALSE;
}

/*******************************************************************************
 *                          Boot Target Management                             *
 ******************************************************************************/

/**
 * @brief Select boot target
 * @param target Boot target
 * @return E_OK if successful, E_NOT_OK otherwise
 */
#define ECUM_STOP_SEC_CODE
#include "MemMap.h"

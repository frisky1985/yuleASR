/*==================================================================================================
 * ECU State Manager — 运行请求/唤醒管理实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

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
static void EcuM_CheckRunRequests(void)
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
Std_ReturnType EcuM_GetState(EcuM_StateType* state)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (state == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATE_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *state = EcuM_CurrentState;
    return E_OK;
}

/**
 * @brief Get current sub-state
 * @param subState Pointer to store sub-state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetSubState(EcuM_SubStateType* subState)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (subState == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATE_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *subState = EcuM_CurrentSubState;
    return E_OK;
}

/*******************************************************************************
 *                          Shutdown Target Management                         *
 ******************************************************************************/

/**
 * @brief Select shutdown target
 * @param target Shutdown target (OFF, RESET, SLEEP)
 * @param mode Mode specific to target
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTSHUTDOWNTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if ((target != ECUM_SHUTDOWN_TARGET_OFF) && 
        (target != ECUM_SHUTDOWN_TARGET_RESET) && 
        (target != ECUM_SHUTDOWN_TARGET_SLEEP))
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTSHUTDOWNTARGET_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    EcuM_ShutdownTarget = target;
    
    if (target == ECUM_SHUTDOWN_TARGET_SLEEP)
    {
        EcuM_SleepMode = mode;
    }
    else
    {
        EcuM_ShutdownMode = mode;
    }
    
    return E_OK;
}

/**
 * @brief Get current shutdown target
 * @param target Pointer to store target
 * @param mode Pointer to store mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetShutdownTarget(EcuM_ShutdownTargetType* target, uint8* mode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSHUTDOWNTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if ((target == NULL_PTR) || (mode == NULL_PTR))
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSHUTDOWNTARGET_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *target = EcuM_ShutdownTarget;
    
    if (EcuM_ShutdownTarget == ECUM_SHUTDOWN_TARGET_SLEEP)
    {
        *mode = EcuM_SleepMode;
    }
    else
    {
        *mode = EcuM_ShutdownMode;
    }
    
    return E_OK;
}

/**
 * @brief Get last shutdown target
 * @param target Pointer to store target
 * @param mode Pointer to store mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetLastShutdownTarget(EcuM_ShutdownTargetType* target, uint8* mode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETLASTSHUTDOWNTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if ((target == NULL_PTR) || (mode == NULL_PTR))
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETLASTSHUTDOWNTARGET_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    /* In a full implementation, this would be read from NV memory */
    *target = EcuM_ShutdownTarget;
    *mode = 0u;
    
    return E_OK;
}

/**
 * @brief Select shutdown cause
 * @param cause Shutdown cause
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_SelectShutdownCause(EcuM_ShutdownCauseType cause)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTSHUTDOWNCAUSE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    EcuM_ShutdownCause = cause;
    return E_OK;
}

/**
 * @brief Get shutdown cause
 * @param cause Pointer to store cause
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetShutdownCause(EcuM_ShutdownCauseType* cause)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSHUTDOWNCAUSE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (cause == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSHUTDOWNCAUSE_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *cause = EcuM_ShutdownCause;
    return E_OK;
}

/*******************************************************************************
 *                          Wakeup Source Management                           *
 ******************************************************************************/

/**
 * @brief Set wakeup event
 * @param sources Wakeup source bitmask
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
static void EcuM_ValidateWakeupSources(void)
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
static void EcuM_ExpireWakeupSources(void)
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
static uint8 EcuM_GetWakeupSourceIndex(EcuM_WakeupSourceType source)
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
static boolean EcuM_IsValidWakeupSource(EcuM_WakeupSourceType source)
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


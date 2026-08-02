/*==================================================================================================
 * ECU State Manager — 运行/睡眠阶段实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

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
static void EcuM_ProcessRun(void)
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
static void EcuM_ProcessPostRun(void)
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
static void EcuM_ProcessGoSleep(void)
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
static void EcuM_ProcessSleep(void)
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
static void EcuM_ProcessHalt(void)
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
static void EcuM_ProcessPoll(void)
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
static void EcuM_ProcessWakeupOne(void)
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
static void EcuM_ProcessWakeupTwo(void)
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
    Rte_Start();
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
void EcuM_Shutdown(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SHUTDOWN_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Start shutdown sequence */
    EcuM_CurrentState = ECUM_STATE_SHUTDOWN;
    EcuM_UpdateSubState(ECUM_SUBSTATE_GO_OFF_ONE);
    
    /* Process GoOffOne */
    EcuM_ProcessGoOffOne();
}

/**
 * @brief Process GoOffOne state
 * @details Write NV data, deinit BSW modules
 */


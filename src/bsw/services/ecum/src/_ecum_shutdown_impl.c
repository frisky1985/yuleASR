/*==================================================================================================
 * ECU State Manager — 关闭阶段实现
 * 自动拆分自 EcuM.c
 *================================================================================================*/

static void EcuM_ProcessGoOffOne(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_GO_OFF_ONE);
    
    /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_EcuM_CurrentState(ECUM_STATE_SHUTDOWN);
#endif
    
    /* De-initialize RTE */
#if (ECUM_RTE_ENABLED == STD_ON)
    /* Rte_Stop(); */
#endif
    
    /* De-initialize COM stack */
#if (ECUM_COMM_ENABLED == STD_ON)
    ComM_DeInit();
#endif
    
    /* Write all NV data */
#if (ECUM_NVM_ENABLED == STD_ON)
    NvM_WriteAll();
    /* Wait for completion or timeout */
#endif
    
    /* De-initialize diagnostic services */
    /* Dcm_DeInit(); */
    /* Dem_DeInit(); */
    
    /* De-initialize COM module */
    /* Com_DeInit(); */
    
    /* De-initialize PDU Router */
    /* PduR_DeInit(); */
    
    /* Transition to GoOffTwo */
    EcuM_UpdateSubState(ECUM_SUBSTATE_GO_OFF_TWO);
    EcuM_ProcessGoOffTwo();
}

/**
 * @brief Process GoOffTwo state
 * @details Shutdown OS and perform final actions
 */
static void EcuM_ProcessGoOffTwo(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_GO_OFF_TWO);
    
    /* Disable all interrupts */
    EcuM_DisableInterrupts();
    
    /* De-initialize SchM */
#if (ECUM_SCHM_ENABLED == STD_ON)
    SchM_Deinit();
#endif
    
    /* Perform shutdown based on target */
    switch (EcuM_ShutdownTarget)
    {
        case ECUM_SHUTDOWN_TARGET_OFF:
            EcuM_PerformShutdown();
            break;
            
        case ECUM_SHUTDOWN_TARGET_RESET:
            EcuM_PerformReset();
            break;
            
        case ECUM_SHUTDOWN_TARGET_SLEEP:
            /* Should not reach here - sleep is handled separately */
            break;
            
        default:
            break;
    }
}

/**
 * @brief Perform Shutdown (Power Off)
 */
static void EcuM_PerformShutdown(void)
{
    /* Call shutdown hook */
    EcuM_AL_SwitchOff();
    
    /* Set state to OFF */
    EcuM_CurrentState = ECUM_STATE_OFF;
    EcuM_IsInitialized = FALSE;
    
    /* Hardware shutdown - typically cuts power or enters standby */
    /* This function should not return */
    while (1)
    {
        /* Infinite loop - power will be cut externally */
    }
}

/**
 * @brief Perform Reset
 */
static void EcuM_PerformReset(void)
{
    /* Reset callout */
    EcuM_AL_Reset(ECUM_DEFAULT_RESET_TYPE);
    
    /* Hardware reset */
    /* Mcu_PerformReset(); */
    
    /* Should not return from reset */
    while (1)
    {
        /* Infinite loop - should reset before reaching here */
    }
}

/**
 * @brief Perform Sleep Entry
 */
static void EcuM_PerformSleep(void)
{
    /* Disable interrupts temporarily */
    EcuM_DisableInterrupts();
    
    /* Enable configured wakeup sources */
    EcuM_EnableWakeupSources(EcuM_EnabledWakeupSources);
    
    /* Enter sleep mode via callout */
    EcuM_AL_EnterSleep();
    
    /* After this point, CPU should be in sleep mode */
    /* Execution resumes here after wakeup */
    
    /* Disable wakeup sources */
    EcuM_DisableWakeupSources(ECUM_CONFIGURED_WAKEUP_SOURCES);
    
    /* Enable interrupts */
    EcuM_EnableInterrupts();
}

/*******************************************************************************
 *                              RUN Request Management                         *
 ******************************************************************************/

/**
 * @brief Request RUN mode
 * @param user User requesting RUN mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */


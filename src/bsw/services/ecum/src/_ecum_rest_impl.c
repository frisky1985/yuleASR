/*==================================================================================================
 * ECU State Manager — Boot/Mode/辅助函数
 * 自动拆分自 EcuM.c
 *================================================================================================*/

Std_ReturnType EcuM_SelectBootTarget(EcuM_BootTargetType target)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTBOOTTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if ((target != ECUM_BOOT_TARGET_OEM_BOOTLOADER) && 
        (target != ECUM_BOOT_TARGET_SYS_BOOTLOADER) && 
        (target != ECUM_BOOT_TARGET_APPLICATION))
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTBOOTTARGET_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    EcuM_BootTarget = target;
    return E_OK;
}

/**
 * @brief Get boot target
 * @param target Pointer to store target
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetBootTarget(EcuM_BootTargetType* target)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETBOOTTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (target == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETBOOTTARGET_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *target = EcuM_BootTarget;
    return E_OK;
}

/*******************************************************************************
 *                          Application Mode                                   *
 ******************************************************************************/

/**
 * @brief Select application mode
 * @param appMode Application mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_SelectApplicationMode(EcuM_AppModeType appMode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized)
    {
        /* Can only change app mode before initialization */
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTAPPMODE_SID, ECUM_E_STATE_CHANGE_FAILED);
        return E_NOT_OK;
    }
#endif
    
    EcuM_ApplicationMode = appMode;
    return E_OK;
}

/**
 * @brief Get application mode
 * @param appMode Pointer to store mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_GetApplicationMode(EcuM_AppModeType* appMode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETAPPMODE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (appMode == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETAPPMODE_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *appMode = EcuM_ApplicationMode;
    return E_OK;
}

/*******************************************************************************
 *                          Communication Mode                                 *
 ******************************************************************************/

/**
 * @brief Request communication mode
 * @param channel Communication channel
 * @param mode Requested mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_ComM_RequestComMode(uint8 channel, EcuM_ModeType mode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_COMMODEREQUEST_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* Forward to ComM */
#if (ECUM_COMM_ENABLED == STD_ON)
    /* ComM_EcuM_RequestComMode(channel, mode); */
    (void)channel;
    (void)mode;
#endif
    
    return E_OK;
}

/**
 * @brief Release communication mode
 * @param channel Communication channel
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType EcuM_ComM_ReleaseComMode(uint8 channel)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_COMMODERERELEASE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    (void)channel;
    return E_OK;
}

/*******************************************************************************
 *                          BSW Mode Management                                *
 ******************************************************************************/

/**
 * @brief Start BSW mode
 * @param mode BSW mode to start
 */
void EcuM_StartBswMode(EcuM_BswModeType mode)
{
    (void)mode;
    /* Mode handling implementation */
}

/**
 * @brief Stop BSW mode
 * @param mode BSW mode to stop
 */
void EcuM_StopBswMode(EcuM_BswModeType mode)
{
    (void)mode;
    /* Mode handling implementation */
}

/*******************************************************************************
 *                          Version Info                                       *
 ******************************************************************************/

/**
 * @brief Get version information
 * @param versionInfo Pointer to version info structure
 */
void EcuM_GetVersionInfo(Std_VersionInfoType* versionInfo)
{
#if (ECUM_VERSION_INFO_API == STD_ON)
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (versionInfo == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, 0x00u, ECUM_E_NULL_POINTER);
        return;
    }
#endif
    
    versionInfo->vendorID = 0x00u;
    versionInfo->moduleID = ECUM_MODULE_ID;
    versionInfo->sw_major_version = ECUM_SW_MAJOR_VERSION;
    versionInfo->sw_minor_version = ECUM_SW_MINOR_VERSION;
    versionInfo->sw_patch_version = ECUM_SW_PATCH_VERSION;
#endif
}

/*******************************************************************************
 *                          Helper Functions                                   *
 ******************************************************************************/

/**
 * @brief Update sub-state and notify BswM
 * @param newSubState New sub-state
 */
static void EcuM_UpdateSubState(EcuM_SubStateType newSubState)
{
    EcuM_SubStateType oldSubState = EcuM_CurrentSubState;
    EcuM_CurrentSubState = newSubState;
    
    /* Notify BswM of sub-state change if needed */
#if (ECUM_BSWM_ENABLED == STD_ON)
    /* BswM_EcuM_CurrentSubState(newSubState); */
#endif
    
    /* Reset state timer on state change */
    if (oldSubState != newSubState)
    {
        EcuM_StateTimer = 0u;
    }
}

/**
 * @brief Disable interrupts
 */
static void EcuM_DisableInterrupts(void)
{
    /* Disable global interrupts */
    /* This would call OS or Mcu function */
}

/**
 * @brief Enable interrupts
 */
static void EcuM_EnableInterrupts(void)
{
    /* Enable global interrupts */
    /* This would call OS or Mcu function */
}

/*******************************************************************************
 *                          Default Callout Implementations                    *
 ******************************************************************************/

/**
 * @brief Default Driver Init One - Pre-OS initialization
 */
__attribute__((weak)) void EcuM_DriverInitOne(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Driver Init Two - Post-OS initialization
 */
__attribute__((weak)) void EcuM_DriverInitTwo(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Driver Init Three - SW-C initialization
 */
__attribute__((weak)) void EcuM_DriverInitThree(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Driver Restart - Wakeup restart initialization
 */
__attribute__((weak)) void EcuM_DriverRestart(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Abstraction Layer Driver Init One
 */
__attribute__((weak)) void EcuM_AL_DriverInitOne(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Abstraction Layer Driver Init Two
 */
__attribute__((weak)) void EcuM_AL_DriverInitTwo(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Abstraction Layer Driver Init Three
 */
__attribute__((weak)) void EcuM_AL_DriverInitThree(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Abstraction Layer Driver Restart
 */
__attribute__((weak)) void EcuM_AL_DriverRestart(const EcuM_ConfigType* config)
{
    (void)config;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Switch Off - Power down
 */
__attribute__((weak)) void EcuM_AL_SwitchOff(void)
{
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Reset
 */
__attribute__((weak)) void EcuM_AL_Reset(EcuM_ResetType resetType)
{
    (void)resetType;
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Enter Sleep
 */
__attribute__((weak)) void EcuM_AL_EnterSleep(void)
{
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Wakeup Check
 */
__attribute__((weak)) void EcuM_AL_WakeupCheck(void)
{
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Wakeup Validation
 */
__attribute__((weak)) void EcuM_AL_WakeupValidation(void)
{
    /* Default implementation - integrator should override */
}

/**
 * @brief Default Wakeup Reaction
 */
__attribute__((weak)) void EcuM_AL_WakeupReaction(void)
{
    /* Default implementation - integrator should override */
}


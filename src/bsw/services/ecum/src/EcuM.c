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

/**
 * @file EcuM.c
 * @brief ECU State Manager - Multi-Phase Startup/Shutdown/Sleep Implementation
 * @version 2.0.0
 * @implements AUTOSAR Classic Platform EcuM SWS R4.0.3
 * 
 * This implementation provides:
 * - Multi-phase startup (StartupOne, StartupTwo, StartupThree)
 * - Complete shutdown sequence (GoOffOne, GoOffTwo)
 * - Sleep management (GoSleep, GoHalt, GoPoll)
 * - Wakeup source management and validation
 * - Full state machine with sub-state tracking
 */

#include "EcuM.h"
#include "EcuM_Cfg.h"
#include "Det.h"
#include "BswM.h"
#include "SchM.h"

#if (ECUM_NVM_ENABLED == STD_ON)
#include "NvM.h"
#endif

#if (ECUM_WDGM_ENABLED == STD_ON)
#include "WdgM.h"
#endif

#if (ECUM_COMM_ENABLED == STD_ON)
#include "ComM.h"
#endif

/*******************************************************************************
 *                             Module Identifiers                              *
 ******************************************************************************/

#define ECUM_MODULE_ID                      0x0Au
#define ECUM_INSTANCE_ID                    0x00u

/*******************************************************************************
 *                          Internal State Variables                           *
 ******************************************************************************/

/* Main state machine */
static EcuM_StateType EcuM_CurrentState = ECUM_STATE_OFF;
static EcuM_SubStateType EcuM_CurrentSubState = ECUM_SUBSTATE_STARTUP_ONE;
static EcuM_ShutdownTargetType EcuM_ShutdownTarget = ECUM_SHUTDOWN_TARGET_OFF;
static EcuM_ShutdownCauseType EcuM_ShutdownCause = ECUM_CAUSE_UNDEFINED;
static EcuM_BootTargetType EcuM_BootTarget = ECUM_BOOT_TARGET_APPLICATION;
static EcuM_AppModeType EcuM_ApplicationMode = ECUM_APPMODE_DEFAULT;

/* Initialization state */
static boolean EcuM_IsInitialized = FALSE;
static boolean EcuM_IsPreOsInitialized = FALSE;
static boolean EcuM_IsOsInitialized = FALSE;
static boolean EcuM_IsPostOsInitialized = FALSE;

/* RUN request management */
static uint32 EcuM_RunRequests = 0u;
static uint32 EcuM_KilledRunRequests = 0u;

/* Wakeup source management */
static EcuM_WakeupSourceType EcuM_PendingWakeupEvents = 0u;
static EcuM_WakeupSourceType EcuM_ValidatedWakeupEvents = 0u;
static EcuM_WakeupSourceType EcuM_ExpiredWakeupEvents = 0u;
static EcuM_WakeupSourceType EcuM_DisabledWakeupSources = 0u;
static EcuM_WakeupSourceType EcuM_EnabledWakeupSources = ECUM_CONFIGURED_WAKEUP_SOURCES;
static EcuM_WakeupStatusType EcuM_WakeupStatus[ECUM_MAX_WAKEUP_SOURCES];

/* Sleep/Shutdown mode */
static uint8 EcuM_SleepMode = ECUM_DEFAULT_SLEEP_MODE;
static uint8 EcuM_ShutdownMode = 0u;

/* Timing */
static uint32 EcuM_MainFunctionCounter = 0u;
static uint32 EcuM_StateTimer = 0u;
static uint32 EcuM_WakeupValidationTimer[ECUM_MAX_WAKEUP_SOURCES];

/* Configuration pointer */
static const EcuM_ConfigType* EcuM_ConfigPtr = NULL_PTR;

/*******************************************************************************
 *                          Internal Function Prototypes                       *
 ******************************************************************************/

/* State Machine Functions */
static void EcuM_ProcessStartupOne(void);
static void EcuM_ProcessStartupTwo(void);
static void EcuM_ProcessRun(void);
static void EcuM_ProcessPostRun(void);
static void EcuM_ProcessGoSleep(void);
static void EcuM_ProcessSleep(void);
static void EcuM_ProcessWakeupOne(void);
static void EcuM_ProcessWakeupTwo(void);
static void EcuM_ProcessGoOffOne(void);
static void EcuM_ProcessGoOffTwo(void);
static void EcuM_ProcessHalt(void);
static void EcuM_ProcessPoll(void);

/* Wakeup Management */
static void EcuM_ValidateWakeupSources(void);
static void EcuM_ExpireWakeupSources(void);
static uint8 EcuM_GetWakeupSourceIndex(EcuM_WakeupSourceType source);
static boolean EcuM_IsValidWakeupSource(EcuM_WakeupSourceType source);

/* Shutdown/Sleep Helpers */
static void EcuM_PerformShutdown(void);
static void EcuM_PerformReset(void);
static void EcuM_PerformSleep(void);
static void EcuM_CheckSleepTransition(void);

/* Helper Functions */
static void EcuM_UpdateSubState(EcuM_SubStateType newSubState);
static void EcuM_CheckRunRequests(void);
static void EcuM_DisableInterrupts(void);
static void EcuM_EnableInterrupts(void);

/*******************************************************************************
 *                              Initialization                                 *
 ******************************************************************************/

/**
 * @brief Initialize EcuM module
 * @details First entry point - initializes minimal hardware and starts OS
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

/*==================================================================================================
 *  子文件包含 (批量拆分)
 *================================================================================================*/
#include "ecum_startup.c"
#include "ecum_run_sleep.c"
#include "ecum_shutdown.c"
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

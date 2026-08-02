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
#include "Rte.h"

/*
 * Forward declarations for BswM callbacks invoked by EcuM.
 * BswM_EcuM_CurrentState and BswM_EcuM_CurrentWakeup are BswM callbacks
 * that notify the BSW Mode Manager of ECU state/wakeup changes.
 * These should be provided by the BswM implementation per AUTOSAR_SWS_BSWModeManager.
 * TODO: Replace stubs with actual BswM implementation once available.
 */
extern FUNC(void, BSWM_CODE) BswM_EcuM_CurrentState(VAR(EcuM_StateType, AUTOMATIC) CurrentState);
extern FUNC(void, BSWM_CODE) BswM_EcuM_CurrentWakeup(
    VAR(EcuM_WakeupSourceType, AUTOMATIC) WakeupSource,
    VAR(EcuM_WakeupStatusType, AUTOMATIC) WakeupStatus);

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
void EcuM_Init(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_INIT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Initialize state variables */
    EcuM_CurrentState = ECUM_STATE_STARTUP;
    EcuM_CurrentSubState = ECUM_SUBSTATE_STARTUP_ONE;
    EcuM_RunRequests = 0u;
    EcuM_KilledRunRequests = 0u;
    EcuM_PendingWakeupEvents = 0u;
    EcuM_ValidatedWakeupEvents = 0u;
    EcuM_ExpiredWakeupEvents = 0u;
    EcuM_StateTimer = 0u;
    EcuM_MainFunctionCounter = 0u;
    
    /* Clear wakeup status array */
    uint8 i;
    for (i = 0u; i < ECUM_MAX_WAKEUP_SOURCES; i++)
    {
        EcuM_WakeupStatus[i] = ECUM_WKSTATUS_NONE;
        EcuM_WakeupValidationTimer[i] = 0u;
    }
    
    EcuM_IsInitialized = TRUE;
    EcuM_IsPreOsInitialized = FALSE;
    EcuM_IsOsInitialized = FALSE;
    EcuM_IsPostOsInitialized = FALSE;
    
    /* Notify BswM about startup */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_EcuM_CurrentState(ECUM_STATE_STARTUP);
#endif
    
    /* Start the multi-phase startup sequence */
    EcuM_StartupOne();
}

/**
 * @brief Startup Phase One - Pre-OS Initialization
 * @details Initialize drivers that don't require OS, start OS
 */
void EcuM_StartupOne(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_STARTUPONE_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
    
    if (EcuM_CurrentSubState != ECUM_SUBSTATE_STARTUP_ONE)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_STARTUPONE_SID, ECUM_E_WRONG_API_ORDER);
        return;
    }
#endif
    
    EcuM_ProcessStartupOne();
}

/**
 * @brief Startup Phase Two - Post-OS Initialization
 * @details Initialize BSW modules after OS is running
 */
void EcuM_StartupTwo(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized == 0U)     {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_STARTUPTWO_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
    
    if (EcuM_CurrentSubState != ECUM_SUBSTATE_STARTUP_TWO)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_STARTUPTWO_SID, ECUM_E_WRONG_API_ORDER);
        return;
    }
#endif
    
    EcuM_ProcessStartupTwo();
}

/*******************************************************************************
 *                              State Processing                               *
 ******************************************************************************/

/**
 * @brief Process Startup One state
 * @details Initialize MCU, Port, Dio, Gpt, and other pre-OS drivers
 */
static void EcuM_ProcessStartupOne(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_STARTUP_ONE);
    
    /* Initialize Pre-OS drivers via callout */
    EcuM_DriverInitOne(EcuM_ConfigPtr);
    
    /* Initialize Mcu module */
    /* Mcu_Init(&Mcu_Config); */
    
    /* Initialize Port module */
    /* Port_Init(&Port_Config); */
    
    /* Initialize Dio module */
    /* Dio_Init(&Dio_Config); */
    
    /* Initialize Gpt module */
    /* Gpt_Init(&Gpt_Config); */
    
    /* Initialize Watchdog */
#if (ECUM_WDGM_ENABLED == STD_ON)
    /* WdgM_Init(NULL_PTR); */
#endif
    
    /* Set default shutdown target */
    EcuM_ShutdownTarget = ECUM_SHUTDOWN_TARGET_OFF;
    EcuM_ShutdownMode = 0u;
    
    /* Enable wakeup sources configured for startup */
    EcuM_EnableWakeupSources(ECUM_CONFIGURED_WAKEUP_SOURCES);
    
    EcuM_IsPreOsInitialized = TRUE;
    
    /* Check for wakeup events */
    EcuM_AL_WakeupCheck();
    
    /* Start OS - this is the point where OS takes over */
    /* StartOs(EcuM_ApplicationMode); */
    
    EcuM_IsOsInitialized = TRUE;
    
    /* Transition to Startup Two */
    EcuM_UpdateSubState(ECUM_SUBSTATE_STARTUP_TWO);
    
    /* Automatically continue to StartupTwo */
    EcuM_StartupTwo();
}

/**
 * @brief Process Startup Two state
 * @details Initialize Com, NvM, ComM, BswM and other post-OS modules
 */
static void EcuM_ProcessStartupTwo(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_STARTUP_TWO);
    
    /* Initialize SchM */
#if (ECUM_SCHM_ENABLED == STD_ON)
    SchM_Init(NULL_PTR);
#endif
    
    /* Initialize BswM first */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_Init(NULL_PTR);
    BswM_EcuM_CurrentState(ECUM_STATE_STARTUP);
#endif
    
    /* Initialize communication stack */
#if (ECUM_COMM_ENABLED == STD_ON)
    ComM_Init(NULL_PTR);
#endif
    
    /* Initialize NV memory */
#if (ECUM_NVM_ENABLED == STD_ON)
    NvM_Init(NULL_PTR);
    NvM_ReadAll();
#endif
    
    /* Initialize COM module */
    /* Com_Init(&Com_Config); */
    
    /* Initialize PDU Router */
    /* PduR_Init(&PduR_Config); */
    
    /* Initialize CAN Interface */
    /* CanIf_Init(&CanIf_Config); */
    
    /* Initialize CAN Driver */
    /* Can_Init(&Can_Config); */
    
    /* Call post-OS driver init callout */
    EcuM_DriverInitTwo(EcuM_ConfigPtr);
    
    /* Initialize diagnostic services */
    /* Dcm_Init(); */
    /* Dem_Init(); */
    
    /* Start RTE */
#if (ECUM_RTE_ENABLED == STD_ON)
    Rte_Start();
#endif
    
    /* Call third phase init (SW-C initialization) */
    EcuM_DriverInitThree(EcuM_ConfigPtr);
    
    EcuM_IsPostOsInitialized = TRUE;
    
    /* Transition to RUN state */
    EcuM_CurrentState = ECUM_STATE_RUN;
    EcuM_UpdateSubState(ECUM_SUBSTATE_RUN);
    
    /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
#endif
}

/*==================================================================================================
*                              Main Function                                  *
*==================================================================================================*/

/**
 * @brief EcuM Main Function - Cyclic processing
 * @details Called periodically to process state machine
 */
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
    SchM_Init(NULL_PTR);
#endif
    
    /* Notify BswM */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_EcuM_CurrentState(ECUM_STATE_RUN);
    BswM_EcuM_CurrentWakeup(EcuM_ValidatedWakeupEvents, ECUM_WKSTATUS_VALIDATED);
#endif
    
    /* Re-initialize communication */
#if (ECUM_COMM_ENABLED == STD_ON)
    ComM_Init(NULL_PTR);
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
    SchM_DeInit();
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

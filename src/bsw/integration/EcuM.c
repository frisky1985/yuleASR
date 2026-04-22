/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Integration Layer)
* Dependencies         : BswM, Mcu, Com, PduR, CanIf, NvM, Dcm, Dem
*
* SW Version           : 1.0.0
* Build Version        : YULETECH_ECUM_1.0.0
* Build Date           : 2026-04-22
* Author               : AI Agent (EcuM Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "EcuM.h"
#include "Mcu.h"
#include "BswM.h"
#include "Det.h"
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define ECUM_INSTANCE_ID                (0x00U)

/* Module state */
#define ECUM_STATE_UNINIT               (0x00U)
#define ECUM_STATE_INIT                 (0x01U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    #define ECUM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define ECUM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* ECU state machine states */
typedef enum
{
    ECUM_SUBSTATE_UNINIT = 0,
    ECUM_SUBSTATE_STARTUP_ONE,
    ECUM_SUBSTATE_STARTUP_TWO,
    ECUM_SUBSTATE_APP_RUN,
    ECUM_SUBSTATE_APP_POST_RUN,
    ECUM_SUBSTATE_SHUTDOWN,
    ECUM_SUBSTATE_SLEEP,
    ECUM_SUBSTATE_WAKE_SLEEP
} EcuM_SubStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const EcuM_ConfigType* ConfigPtr;
    EcuM_StateType CurrentState;
    EcuM_SubStateType SubState;
    EcuM_WakeupSourceType PendingWakeupEvents;
    EcuM_WakeupSourceType ValidatedWakeupEvents;
    EcuM_WakeupSourceType ExpiredWakeupEvents;
    boolean ShutdownRequested;
    boolean GoSleepRequested;
    uint8 ShutdownTarget;
    uint8 SleepMode;
    uint32 EcuMCounter;
} EcuM_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define ECUM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC EcuM_InternalStateType EcuM_InternalState;

#define ECUM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void EcuM_StartupOne(void);
STATIC void EcuM_StartupTwo(void);
STATIC void EcuM_OnGoOffOne(void);
STATIC void EcuM_OnGoOffTwo(void);
STATIC void EcuM_ProcessWakeupEvents(void);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define ECUM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Startup phase one - minimal initialization
 */
STATIC void EcuM_StartupOne(void)
{
    /* Initialize MCU */
    Mcu_Init(EcuM_InternalState.ConfigPtr->McuConfigPtr);

    /* Select default clock settings */
    Mcu_InitClock(MCU_CLOCK_SETTING_DEFAULT);

    /* Distribute PLL if needed */
    /* Mcu_DistributePllClock(); */

    /* Initialize port pins */
    /* Port_Init(EcuM_InternalState.ConfigPtr->PortConfigPtr); */

    EcuM_InternalState.SubState = ECUM_SUBSTATE_STARTUP_ONE;
}

/**
 * @brief   Startup phase two - full BSW initialization
 */
STATIC void EcuM_StartupTwo(void)
{
    /* Initialize DET (Development Error Tracer) */
    /* Det_Init(); */

    /* Initialize BswM */
    BswM_Init(EcuM_InternalState.ConfigPtr->BswMConfigPtr);

    /* Initialize OS */
    /* StartOS(OSDEFAULTAPPMODE); */

    EcuM_InternalState.SubState = ECUM_SUBSTATE_STARTUP_TWO;
    EcuM_InternalState.CurrentState = ECUM_STATE_APP_RUN;
}

/**
 * @brief   First phase of shutdown
 */
STATIC void EcuM_OnGoOffOne(void)
{
    /* Deinitialize communication stack */
    /* Com_DeInit(); */
    /* PduR_DeInit(); */
    /* CanIf_DeInit(); */

    EcuM_InternalState.CurrentState = ECUM_STATE_SHUTDOWN;
    EcuM_InternalState.SubState = ECUM_SUBSTATE_SHUTDOWN;
}

/**
 * @brief   Second phase of shutdown
 */
STATIC void EcuM_OnGoOffTwo(void)
{
    /* Deinitialize NvM */
    /* NvM_WriteAll(); */

    /* Deinitialize MCU */
    /* Mcu_DeInit(); */

    /* Infinite loop - system halted */
    for (;;)
    {
        /* System is shut down */
    }
}

/**
 * @brief   Process wakeup events
 */
STATIC void EcuM_ProcessWakeupEvents(void)
{
    if (EcuM_InternalState.PendingWakeupEvents != ECUM_WKSOURCE_NONE)
    {
        /* Validate pending wakeup events */
        /* In a full implementation, check with drivers */
        EcuM_InternalState.ValidatedWakeupEvents |= EcuM_InternalState.PendingWakeupEvents;
        EcuM_InternalState.PendingWakeupEvents = ECUM_WKSOURCE_NONE;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the EcuM module
 */
void EcuM_Init(void)
{
    /* Initialize internal state */
    EcuM_InternalState.State = ECUM_STATE_INIT;
    EcuM_InternalState.CurrentState = ECUM_STATE_STARTUP;
    EcuM_InternalState.SubState = ECUM_SUBSTATE_UNINIT;
    EcuM_InternalState.PendingWakeupEvents = ECUM_WKSOURCE_NONE;
    EcuM_InternalState.ValidatedWakeupEvents = ECUM_WKSOURCE_NONE;
    EcuM_InternalState.ExpiredWakeupEvents = ECUM_WKSOURCE_NONE;
    EcuM_InternalState.ShutdownRequested = FALSE;
    EcuM_InternalState.GoSleepRequested = FALSE;
    EcuM_InternalState.ShutdownTarget = ECUM_STATE_SLEEP;
    EcuM_InternalState.SleepMode = 0U;
    EcuM_InternalState.EcuMCounter = 0U;
}

/**
 * @brief   Startup the ECU (called from startup code)
 */
void EcuM_Startup(void)
{
    if (EcuM_InternalState.State == ECUM_STATE_INIT)
    {
        /* Startup phase one */
        EcuM_StartupOne();

        /* Startup phase two */
        EcuM_StartupTwo();
    }
}

/**
 * @brief   Shutdown the ECU
 */
void EcuM_Shutdown(void)
{
    if (EcuM_InternalState.State == ECUM_STATE_INIT)
    {
        EcuM_OnGoOffOne();
        EcuM_OnGoOffTwo();
    }
}

/**
 * @brief   Request ECU shutdown
 */
void EcuM_RequestShutdown(void)
{
    EcuM_InternalState.ShutdownRequested = TRUE;
}

/**
 * @brief   Request ECU sleep
 */
void EcuM_RequestSleep(uint8 sleepMode)
{
    EcuM_InternalState.GoSleepRequested = TRUE;
    EcuM_InternalState.SleepMode = sleepMode;
}

/**
 * @brief   Set wakeup event
 */
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType wakeupSource)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_InternalState.State != ECUM_STATE_INIT)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_SETWAKEUPEVENT, ECUM_E_UNINIT);
        return;
    }
#endif

    EcuM_InternalState.PendingWakeupEvents |= wakeupSource;
}

/**
 * @brief   Validate wakeup event
 */
void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType wakeupSource)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_InternalState.State != ECUM_STATE_INIT)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_VALIDATEWAKEUPEVENT, ECUM_E_UNINIT);
        return;
    }
#endif

    if ((EcuM_InternalState.PendingWakeupEvents & wakeupSource) != 0U)
    {
        EcuM_InternalState.PendingWakeupEvents &= ~wakeupSource;
        EcuM_InternalState.ValidatedWakeupEvents |= wakeupSource;
    }
}

/**
 * @brief   Get the current ECU state
 */
EcuM_StateType EcuM_GetState(void)
{
    return EcuM_InternalState.CurrentState;
}

/**
 * @brief   Get pending wakeup events
 */
EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void)
{
    return EcuM_InternalState.PendingWakeupEvents;
}

/**
 * @brief   Get validated wakeup events
 */
EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void)
{
    return EcuM_InternalState.ValidatedWakeupEvents;
}

/**
 * @brief   Clear validated wakeup events
 */
void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType wakeupSource)
{
    EcuM_InternalState.ValidatedWakeupEvents &= ~wakeupSource;
    EcuM_InternalState.ExpiredWakeupEvents &= ~wakeupSource;
}

/**
 * @brief   Main function for EcuM processing
 */
void EcuM_MainFunction(void)
{
    if (EcuM_InternalState.State == ECUM_STATE_INIT)
    {
        EcuM_InternalState.EcuMCounter++;

        /* Process wakeup events */
        EcuM_ProcessWakeupEvents();

        /* Handle shutdown request */
        if (EcuM_InternalState.ShutdownRequested)
        {
            EcuM_Shutdown();
            EcuM_InternalState.ShutdownRequested = FALSE;
        }

        /* Handle sleep request */
        if (EcuM_InternalState.GoSleepRequested)
        {
            EcuM_InternalState.CurrentState = ECUM_STATE_SLEEP;
            EcuM_InternalState.SubState = ECUM_SUBSTATE_SLEEP;
            EcuM_InternalState.GoSleepRequested = FALSE;
        }
    }
}

/**
 * @brief   Get version information
 */
#if (ECUM_VERSION_INFO_API == STD_ON)
void EcuM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_GETVERSIONINFO, ECUM_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = ECUM_VENDOR_ID;
        versioninfo->moduleID = ECUM_MODULE_ID;
        versioninfo->sw_major_version = ECUM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = ECUM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = ECUM_SW_PATCH_VERSION;
    }
}
#endif

#define ECUM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/

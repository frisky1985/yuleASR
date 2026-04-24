/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Integration Layer)
* Dependencies         : BswM, Mcu, Com, PduR, CanIf, NvM, Dcm, Dem
*
* SW Version           : 1.0.0
* Build Version        : YULETECH_ECUM_1.0.0
* Build Date           : 2026-04-24
* Author               : AI Agent (EcuM Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "EcuM.h"
#include "Os.h"
#include "Det.h"
#include "MemMap.h"

/* MCAL Layer */
#include "Mcu.h"
#include "Port.h"
#include "Dio.h"
#include "Gpt.h"
#include "Can.h"
#include "Spi.h"
#include "Adc.h"
#include "Pwm.h"
#include "Wdg.h"

/* ECUAL Layer */
#include "CanIf.h"
#include "CanTp.h"
#include "MemIf.h"
#include "Fee.h"
#include "Ea.h"
#include "EthIf.h"
#include "LinIf.h"
#include "IoHwAb.h"

/* Service Layer */
#include "PduR.h"
#include "Com.h"
#include "NvM.h"
#include "Dcm.h"
#include "Dem.h"
#include "BswM.h"

/* External configuration for modules without global config pointer */
extern const Com_ConfigType Com_Config;

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
    ECUM_SUBSTATE_STARTUP_THREE,
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
STATIC void EcuM_OnGoOffOne(void);
STATIC void EcuM_OnGoOffTwo(void);
STATIC void EcuM_ProcessWakeupEvents(void);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define ECUM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Startup phase one - MCAL initialization
 *
 * Initializes all MCAL drivers in the correct order:
 * Mcu -> Port -> Dio -> Gpt -> Can -> Spi -> Adc -> Pwm -> Wdg
 */
STATIC void EcuM_StartupOne(void)
{
    /* Initialize MCU (must be first) */
    (void)Mcu_Init(&Mcu_Config);

    /* Initialize clock */
    (void)Mcu_InitClock(MCU_CLOCK_SETTING_DEFAULT);

    /* Distribute PLL clock */
    (void)Mcu_DistributePllClock();

    /* Initialize port pins */
    Port_Init(&Port_Config);

    /* Initialize general purpose timers */
    Gpt_Init(&Gpt_Config);

    /* Initialize CAN controller */
    Can_Init(&Can_Config);

    /* Initialize SPI */
    Spi_Init(&Spi_Config);

    /* Initialize ADC */
    Adc_Init(&Adc_Config);

    /* Initialize PWM */
    Pwm_Init(&Pwm_Config);

    /* Initialize Watchdog */
    Wdg_Init(&Wdg_Config);

    EcuM_InternalState.SubState = ECUM_SUBSTATE_STARTUP_ONE;
}

/**
 * @brief   First phase of shutdown - Service layer deinitialization
 */
STATIC void EcuM_OnGoOffOne(void)
{
    /* Deinitialize Service layer (reverse order of init) */
    /* Dem_DeInit not implemented */
    Dcm_DeInit();
    /* NvM_DeInit not implemented */
    Com_DeInit();
    PduR_DeInit();

    /* Deinitialize BswM */
    BswM_Deinit();

    EcuM_InternalState.CurrentState = ECUM_STATE_SHUTDOWN;
    EcuM_InternalState.SubState = ECUM_SUBSTATE_SHUTDOWN;
}

/**
 * @brief   Second phase of shutdown - ECUAL and MCAL deinitialization
 */
STATIC void EcuM_OnGoOffTwo(void)
{
    /* Deinitialize ECUAL layer */
    IoHwAb_DeInit();
    /* LinIf_DeInit not implemented */
    /* EthIf_DeInit not implemented */
    /* Ea_DeInit not implemented */
    /* Fee_DeInit not implemented */
    /* MemIf_DeInit not implemented */
    /* CanTp_DeInit not implemented */
    CanIf_DeInit();

    /* Deinitialize MCAL layer */
    /* Wdg_DeInit not implemented */
    Pwm_DeInit();
    Adc_DeInit();
    /* Spi_DeInit not implemented */
    /* Can_DeInit not implemented */
    Gpt_DeInit();
    /* Dio has no DeInit */
    /* Port has no DeInit in this implementation */
    /* Mcu_DeInit not implemented */

    /* Shutdown OS */
    ShutdownOS(E_OS_OK);

    /* Infinite loop - system halted if OS shutdown returns */
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
        EcuM_InternalState.ValidatedWakeupEvents |= EcuM_InternalState.PendingWakeupEvents;
        EcuM_InternalState.PendingWakeupEvents = ECUM_WKSOURCE_NONE;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the EcuM module and performs Phase I startup (MCAL init + OS start)
 *
 * This function never returns because it starts the OS scheduler.
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

    /* Phase I: MCAL initialization */
    EcuM_StartupOne();

    /* Start OS - this never returns */
    StartOS(OSDEFAULTAPPMODE);
}

/**
 * @brief   Startup phase two - ECUAL initialization
 *
 * Called after OS has started (from startup hook or init task).
 */
void EcuM_StartupTwo(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_InternalState.State != ECUM_STATE_INIT)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_STARTUP_TWO, ECUM_E_UNINIT);
        return;
    }
    if (EcuM_InternalState.SubState != ECUM_SUBSTATE_STARTUP_ONE)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_STARTUP_TWO, ECUM_E_STATE_TRANSITION);
        return;
    }
#endif

    /* Initialize ECUAL layer */
    CanIf_Init(&CanIf_Config);
    CanTp_Init(&CanTp_Config);
    MemIf_Init(&MemIf_Config);
    Fee_Init(&Fee_Config);
    Ea_Init(&Ea_Config);
    EthIf_Init(&EthIf_Config);
    LinIf_Init(&LinIf_Config);
    IoHwAb_Init(&IoHwAb_Config);

    EcuM_InternalState.SubState = ECUM_SUBSTATE_STARTUP_TWO;
}

/**
 * @brief   Startup phase three - Service layer initialization
 *
 * Called after ECUAL initialization is complete.
 */
void EcuM_StartupThree(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_InternalState.State != ECUM_STATE_INIT)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_STARTUP_THREE, ECUM_E_UNINIT);
        return;
    }
    if (EcuM_InternalState.SubState != ECUM_SUBSTATE_STARTUP_TWO)
    {
        ECUM_DET_REPORT_ERROR(ECUM_SERVICE_ID_STARTUP_THREE, ECUM_E_STATE_TRANSITION);
        return;
    }
#endif

    /* Initialize Service layer */
    PduR_Init(&PduR_Config);
    Com_Init(&Com_Config);
    NvM_Init(&NvM_Config);
    Dcm_Init(&Dcm_Config);
    Dem_Init(&Dem_Config);

    /* Initialize BswM last among services */
    BswM_Init(&BswM_Config);

    EcuM_InternalState.SubState = ECUM_SUBSTATE_STARTUP_THREE;
    EcuM_InternalState.CurrentState = ECUM_STATE_RUN;

    /* Notify BswM that startup is complete, request RUN mode */
    BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN);
}

/**
 * @brief   Startup the ECU (legacy combined startup)
 *
 * Note: In the phased startup model, this is equivalent to EcuM_Init
 * which starts the OS. Phase II and III are executed separately.
 */
void EcuM_Startup(void)
{
    if (EcuM_InternalState.State == ECUM_STATE_INIT)
    {
        /* Phase I only - OS start never returns */
        EcuM_StartupOne();
        StartOS(OSDEFAULTAPPMODE);
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
 * @brief   Clear wakeup events
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

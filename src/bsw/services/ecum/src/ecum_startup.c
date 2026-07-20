/*******************************************************************************
 * EcuM 启动阶段实现
 * 自动拆分自 EcuM.c
 ******************************************************************************/
#define ECUM_START_SEC_CODE
#include "BswM.h"
#include "SchM.h"
#include "MemMap.h"

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
void EcuM_ProcessStartupOne(void)
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
void EcuM_ProcessStartupTwo(void)
{
    EcuM_UpdateSubState(ECUM_SUBSTATE_STARTUP_TWO);
    
    /* Initialize SchM */
#if (ECUM_SCHM_ENABLED == STD_ON)
    SchM_Init();
#endif
    
    /* Initialize BswM first */
#if (ECUM_BSWM_ENABLED == STD_ON)
    BswM_Init(NULL_PTR);
    BswM_EcuM_CurrentState(ECUM_STATE_STARTUP);
#endif
    
    /* Initialize communication stack */
#if (ECUM_COMM_ENABLED == STD_ON)
    ComM_Init();
#endif
    
    /* Initialize NV memory */
#if (ECUM_NVM_ENABLED == STD_ON)
    NvM_Init();
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
    /* Rte_Start(); */
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

/*******************************************************************************
 *                              Main Function                                  *
 ******************************************************************************/

/**
 * @brief EcuM Main Function - Cyclic processing
 * @details Called periodically to process state machine
 */
#define ECUM_STOP_SEC_CODE
#include "MemMap.h"

/**
 * @file EcuM.c
 * @brief ECU State Manager Implementation
 */

#include "EcuM.h"
#include "EcuM_Cfg.h"
#include "Det.h"

/* Internal State */
static EcuM_StateType EcuM_CurrentState = ECUM_STATE_OFF;
static EcuM_ShutdownTargetType EcuM_ShutdownTarget = ECUM_STATE_OFF;
static boolean EcuM_IsInitialized = FALSE;
static uint32 EcuM_RunRequests = 0;

#define ECUM_MODULE_ID                      0x0A
#define ECUM_INSTANCE_ID                    0x00

void EcuM_Init(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_INIT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Initialize state */
    EcuM_CurrentState = ECUM_STATE_STARTUP;
    EcuM_RunRequests = 0;
    
    /* TODO: Initialize BSW modules */
    /* TODO: Initialize Scheduler */
    /* TODO: Initialize OS */
    
    EcuM_IsInitialized = TRUE;
    
    /* Transition to RUN state */
    EcuM_CurrentState = ECUM_STATE_RUN;
}

void EcuM_StartupTwo(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_INIT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* TODO: Initialize SWCs */
    /* TODO: Start RTE */
}

Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_REQUESTRUN_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (user >= ECUM_MAX_USERS)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_REQUESTRUN_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    EcuM_RunRequests |= (1u << user);
    return E_OK;
}

Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_RELEASERUN_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (user >= ECUM_MAX_USERS)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_RELEASERUN_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    EcuM_RunRequests &= ~(1u << user);
    
    /* Check if all RUN requests released */
    if (EcuM_RunRequests == 0)
    {
        EcuM_CurrentState = ECUM_STATE_POST_RUN;
    }
    
    return E_OK;
}

Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTSHUTDOWNTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    (void)mode; /* Unused in this implementation */
    EcuM_ShutdownTarget = target;
    return E_OK;
}

Std_ReturnType EcuM_GetState(EcuM_StateType* state)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
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

void EcuM_Shutdown(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SHUTDOWN_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    EcuM_CurrentState = ECUM_STATE_SHUTDOWN;
    
    /* TODO: De-initialize modules */
    /* TODO: Shutdown OS */
}

void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SETWAKEUPEVENT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    (void)sources; /* TODO: Implement wakeup handling */
}

EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(EcuM_WakeupSourceType sources)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_VALIDATEMCUWAKEUPEVENT_SID, ECUM_E_NOT_INITIALIZED);
        return ECUM_WKSTATUS_NONE;
    }
#endif
    
    (void)sources; /* TODO: Implement wakeup status */
    return ECUM_WKSTATUS_NONE;
}

/**
 * @file SchM.c
 * @brief Scheduler Manager Implementation
 */

#include "SchM.h"
#include "SchM_Cfg.h"
#include "Det.h"

#define SCHM_MODULE_ID                      0x0C
#define SCHM_INSTANCE_ID                    0x00

static boolean SchM_IsInitialized = FALSE;
static boolean SchM_IsRunning = FALSE;

void SchM_Init(const void* ConfigPtr)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (SchM_IsInitialized)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_INIT_SID, SCHM_E_ALREADY_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_INIT_SID, 0x03); /* NULL_POINTER */
        return;
    }
#endif
    
    (void)ConfigPtr;
    
    /* TODO: Initialize scheduler structures */
    /* TODO: Configure schedule tables */
    
    SchM_IsInitialized = TRUE;
}

void SchM_Deinit(void)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (!SchM_IsInitialized)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_DEINIT_SID, SCHM_E_UNINIT);
        return;
    }
#endif
    
    if (SchM_IsRunning)
    {
        SchM_Stop();
    }
    
    SchM_IsInitialized = FALSE;
}

void SchM_Start(void)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (!SchM_IsInitialized)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_START_SID, SCHM_E_UNINIT);
        return;
    }
#endif
    
    SchM_IsRunning = TRUE;
    
    /* TODO: Start scheduler */
    /* TODO: Activate schedule tables */
}

void SchM_Stop(void)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (!SchM_IsInitialized)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_STOP_SID, SCHM_E_UNINIT);
        return;
    }
#endif
    
    SchM_IsRunning = FALSE;
    
    /* TODO: Stop scheduler */
    /* TODO: Deactivate schedule tables */
}

void SchM_SwitchPoint(SchM_PointType point)
{
#if (SCHM_DEV_ERROR_DETECT == STD_ON)
    if (!SchM_IsInitialized)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_SWITCHPOINT_SID, SCHM_E_UNINIT);
        return;
    }
    if (point >= SCHM_POINT_MAX)
    {
        Det_ReportError(SCHM_MODULE_ID, SCHM_INSTANCE_ID, SCHM_SWITCHPOINT_SID, 0x04); /* INVALID_PAR */
        return;
    }
#endif
    
    /* TODO: Switch schedule table point */
    (void)point;
}

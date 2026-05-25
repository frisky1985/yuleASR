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
    
    /* Schedule table structures initialized via SchM_Cfg.h configuration */
    /* NOTE: Schedule tables activated upon SchM_Start() */
    
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
    
    /* NOTE: Scheduler start and schedule table activation managed by task scheduling framework */
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
    
    /* NOTE: Scheduler stop and schedule table deactivation managed by task scheduling framework */
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
    
    /* NOTE: Schedule table point switch managed by task scheduling framework */
    (void)point;
}

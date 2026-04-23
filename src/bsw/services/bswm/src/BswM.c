/**
 * @file BswM.c
 * @brief BSW Mode Manager Implementation
 */

#include "BswM.h"
#include "BswM_Cfg.h"
#include "Det.h"

#define BSWM_MODULE_ID                      0x0B
#define BSWM_INSTANCE_ID                    0x00

static boolean BswM_IsInitialized = FALSE;
static uint16 BswM_CurrentMode[BSWM_MAX_ACTION_LISTS];

void BswM_Init(const void* ConfigPtr)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_IsInitialized)
    {
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_INIT_SID, BSWM_E_NOT_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_INIT_SID, BSWM_E_NULL_POINTER);
        return;
    }
#endif
    
    (void)ConfigPtr;
    
    /* Initialize all modes to default */
    for (uint16 i = 0; i < BSWM_MAX_ACTION_LISTS; i++)
    {
        BswM_CurrentMode[i] = 0;
    }
    
    BswM_IsInitialized = TRUE;
}

void BswM_Deinit(void)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (!BswM_IsInitialized)
    {
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_DEINIT_SID, BSWM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    BswM_IsInitialized = FALSE;
}

void BswM_RequestMode(BswM_UserType requesting_user, uint16 requested_mode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (!BswM_IsInitialized)
    {
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_REQUESTMODE_SID, BSWM_E_NOT_INITIALIZED);
        return;
    }
    if (requesting_user >= BSWM_MAX_ACTION_LISTS)
    {
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_REQUESTMODE_SID, BSWM_E_INVALID_PAR);
        return;
    }
#endif
    
    /* TODO: Validate mode request */
    /* TODO: Evaluate rules */
    /* TODO: Execute action lists */
    
    BswM_CurrentMode[requesting_user] = requested_mode;
}

void BswM_MainFunction(void)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (!BswM_IsInitialized)
    {
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_MAINFUNCTION_SID, BSWM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* TODO: Evaluate deferred rules */
    /* TODO: Process mode requests */
}

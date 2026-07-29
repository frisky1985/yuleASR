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

/** @file Wdgm.c
 * @brief Watchdog Manager implementation
 *
 * AUTOSAR R22-11 compliant Wdgm module
 * Service layer - Watchdog Management and Supervision
 */

/*============================================================================
 *  INCLUDES
 *===========================================================================*/
#include "Wdgm.h"
#include "Wdgm_Cfg.h"

#if (WDGM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*============================================================================
 *  VERSION CHECK
 *===========================================================================*/
#define WDGM_SW_MAJOR_VERSION_CHECK         1
#define WDGM_SW_MINOR_VERSION_CHECK         0
#define WDGM_SW_PATCH_VERSION_CHECK         0

#if (WDGM_SW_MAJOR_VERSION != WDGM_SW_MAJOR_VERSION_CHECK)
    #error "Wdgm: Software major version mismatch"
#endif

#if (WDGM_SW_MINOR_VERSION != WDGM_SW_MINOR_VERSION_CHECK)
    #error "Wdgm: Software minor version mismatch"
#endif

/*============================================================================
 *  INTERNAL STATE
 *===========================================================================*/

/** @brief Module initialization state */
static boolean Wdgm_Initialized = FALSE;

/** @brief Current global status */
static Wdgm_GlobalStatusType Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_DEACTIVATED;

/** @brief Current mode */
static WdgIf_ModeType Wdgm_CurrentMode = WDGIF_OFF_MODE;

/** @brief Pointer to configuration */
static const Wdgm_ConfigType* Wdgm_ConfigPtr = NULL;

/** @brief Supervision entities runtime data */
static Wdgm_SupervisedEntityType Wdgm_Entities[WDGM_MAX_SUPERVISED_ENTITIES];

/** @brief Checkpoint counters for alive supervision */
static Wdgm_AliveCounterType Wdgm_AliveCounters[WDGM_MAX_SUPERVISED_ENTITIES];

/** @brief Expected alive counters */
static Wdgm_AliveCounterType Wdgm_ExpectedAliveCounters[WDGM_MAX_SUPERVISED_ENTITIES];

/** @brief Consecutive failures counter */
static uint8 Wdgm_FailureCounters[WDGM_MAX_SUPERVISED_ENTITIES];

/** @brief First expired SEID */
static Wdgm_SupervisedEntityIdType Wdgm_FirstExpiredSEID = 0xFFFF;

/** @brief Supervision cycle counter */
static uint32 Wdgm_CycleCounter = 0;

/*============================================================================
 *  INTERNAL FUNCTIONS
 *===========================================================================*/

/**
 * @brief Validate SEID
 */
static inline boolean Wdgm_IsValidSEID(Wdgm_SupervisedEntityIdType SEID)
{
    return (SEID < WDGM_MAX_SUPERVISED_ENTITIES);
}

/**
 * @brief Update global status based on local statuses
 */
static void Wdgm_UpdateGlobalStatus(void)
{
    uint8 i;
    uint8 expiredCount = 0;
    uint8 failedCount = 0;
    uint8 okCount = 0;

    for (i = 0; i < WDGM_MAX_SUPERVISED_ENTITIES; i++)
    {
        if (Wdgm_Entities[i].IsInitialized)
        {
            switch (Wdgm_Entities[i].LocalStatus)
            {
                case WDGM_LOCAL_STATUS_EXPIRED:
                    expiredCount++;
                    break;
                case WDGM_LOCAL_STATUS_FAILED:
                    failedCount++;
                    break;
                case WDGM_LOCAL_STATUS_OK:
                    okCount++;
                    break;
                default:
                    break;
            }
        }
    }

    /* Determine global status */
    if (expiredCount > 0)
    {
        Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_EXPIRED;
    }
    else if (failedCount > 0)
    {
        Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_FAILED;
    }
    else if (okCount > 0)
    {
        Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_OK;
    }
    else
    {
        Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_DEACTIVATED;
    }
}

/**
 * @brief Check alive supervision for an entity
 */
static void Wdgm_CheckAliveSupervision(Wdgm_SupervisedEntityIdType SEID)
{
    if (Wdgm_Entities[SEID].IsInitialized &&
        (Wdgm_Entities[SEID].LocalStatus != WDGM_LOCAL_STATUS_DEACTIVATED))
    {
        /* Check if alive counter is within expected range */
        if (Wdgm_AliveCounters[SEID] < WDGM_ALIVE_THRESHOLD)
        {
            Wdgm_FailureCounters[SEID]++;

            if (Wdgm_FailureCounters[SEID] >= WDGM_EXPIRATION_TOLERANCE)
            {
                /* Too many consecutive failures */
                Wdgm_Entities[SEID].LocalStatus = WDGM_LOCAL_STATUS_EXPIRED;

                /* Record first expired SEID */
                if (Wdgm_FirstExpiredSEID == 0xFFFF)
                {
                    Wdgm_FirstExpiredSEID = SEID;
                }
            }
            else
            {
                Wdgm_Entities[SEID].LocalStatus = WDGM_LOCAL_STATUS_FAILED;
            }
        }
        else
        {
            /* Reset failure counter on success */
            Wdgm_FailureCounters[SEID] = 0;
            Wdgm_Entities[SEID].LocalStatus = WDGM_LOCAL_STATUS_OK;
        }

        /* Reset alive counter for next supervision cycle */
        Wdgm_AliveCounters[SEID] = 0;
    }
}

/**
 * @brief Trigger watchdog if global status is OK
 */
static void Wdgm_TriggerWatchdog(void)
{
    if ((Wdgm_GlobalStatus == WDGM_GLOBAL_STATUS_OK) ||
        (Wdgm_GlobalStatus == WDGM_GLOBAL_STATUS_DEACTIVATED))
    {
        /* Trigger watchdog via WdgIf */
        (void)WdgIf_Trigger(0);
    }
    /* If FAILED or EXPIRED, don't trigger - let watchdog timeout occur */
}

/*============================================================================
 *  API IMPLEMENTATION
 *===========================================================================*/

/**
 * @brief Initialize Watchdog Manager
 * SWS_Wdgm_00001
 */
void Wdgm_Init(const Wdgm_ConfigType* ConfigPtr)
{
    uint8 i;

    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_INIT, WDGM_E_PARAM_CONFIG);
        return;
    }
    #endif

    /* Store configuration */
    Wdgm_ConfigPtr = ConfigPtr;

    /* Initialize supervision entities */
    for (i = 0; i < WDGM_MAX_SUPERVISED_ENTITIES; i++)
    {
        Wdgm_Entities[i].SEId = i;
        Wdgm_Entities[i].LocalStatus = WDGM_LOCAL_STATUS_DEACTIVATED;
        Wdgm_Entities[i].AliveCounter = 0;
        Wdgm_Entities[i].IsInitialized = TRUE;

        Wdgm_AliveCounters[i] = 0;
        Wdgm_ExpectedAliveCounters[i] = WDGM_ALIVE_THRESHOLD;
        Wdgm_FailureCounters[i] = 0;
    }

    Wdgm_FirstExpiredSEID = 0xFFFF;
    Wdgm_CycleCounter = 0;

    /* Set initial mode */
    if (ConfigPtr->InitialMode != NULL)
    {
        Wdgm_CurrentMode = *ConfigPtr->InitialMode;
        (void)WdgIf_SetMode(0, Wdgm_CurrentMode);
    }

    Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_OK;
    Wdgm_Initialized = TRUE;
}

/**
 * @brief Deinitialize Watchdog Manager
 * SWS_Wdgm_00002
 */
void Wdgm_DeInit(void)
{
    uint8 i;

    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_DEINIT, WDGM_E_NOT_INITIALIZED);
        return;
    }

    #if (WDGM_DEINIT_API == STD_OFF)
    (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_DEINIT, WDGM_E_NO_DEINIT);
    return;
    #endif
    #endif

    /* Reset all entities */
    for (i = 0; i < WDGM_MAX_SUPERVISED_ENTITIES; i++)
    {
        Wdgm_Entities[i].IsInitialized = FALSE;
        Wdgm_Entities[i].LocalStatus = WDGM_LOCAL_STATUS_DEACTIVATED;
    }

    Wdgm_Initialized = FALSE;
    Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_DEACTIVATED;
    Wdgm_ConfigPtr = NULL;
}

/**
 * @brief Get version information
 * SWS_Wdgm_00003
 */
#if (WDGM_VERSION_INFO_API == STD_ON)
void Wdgm_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_GETVERSIONINFO, WDGM_E_PARAM_POINTER);
        return;
    }
    #endif

    VersionInfo->vendorID = WDGM_VENDOR_ID;
    VersionInfo->moduleID = WDGM_MODULE_ID;
    VersionInfo->sw_major_version = WDGM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = WDGM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = WDGM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Set Watchdog Manager mode
 * SWS_Wdgm_00004
 */
Std_ReturnType Wdgm_SetMode(WdgIf_ModeType Mode)
{
    Std_ReturnType result = E_NOT_OK;

    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_SETMODE, WDGM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    #if (WDGM_OFF_MODE_ENABLED == STD_OFF)
    if (Mode == WDGIF_OFF_MODE)
    {
        return E_NOT_OK;
    }
    #endif
    #endif

    result = WdgIf_SetMode(0, Mode);

    if (result == E_OK)
    {
        Wdgm_CurrentMode = Mode;
    }
    else
    {
        #if (WDGM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportRuntimeError(WDGM_MODULE_ID, 0, WDGM_SID_SETMODE, WDGM_E_SET_MODE);
        #endif
    }

    return result;
}

/**
 * @brief Get current Watchdog Manager mode
 * SWS_Wdgm_00020
 */
WdgIf_ModeType Wdgm_GetMode(void)
{
    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_GETMODE, WDGM_E_NOT_INITIALIZED);
        return WDGIF_OFF_MODE;
    }
    #endif

    return Wdgm_CurrentMode;
}

/**
 * @brief Report checkpoint reached
 * SWS_Wdgm_00010
 */
Std_ReturnType Wdgm_CheckpointReached(Wdgm_SupervisedEntityIdType SEID,
                                       Wdgm_CheckpointIdType CheckpointID)
{
    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, SEID, WDGM_SID_CHECKPOINTREACHED, WDGM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (!Wdgm_IsValidSEID(SEID))
    {
        (void)Det_ReportError(WDGM_MODULE_ID, SEID, WDGM_SID_CHECKPOINTREACHED, WDGM_E_PARAM_SEID);
        return E_NOT_OK;
    }
    #endif

    if (Wdgm_Entities[SEID].IsInitialized)
    {
        #if (WDGM_ALIVE_MONITORING == STD_ON)
        /* Increment alive counter */
        Wdgm_AliveCounters[SEID]++;
        #endif

        /* If entity was deactivated, activate it */
        if (Wdgm_Entities[SEID].LocalStatus == WDGM_LOCAL_STATUS_DEACTIVATED)
        {
            Wdgm_Entities[SEID].LocalStatus = WDGM_LOCAL_STATUS_OK;
        }

        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief Get local supervision status
 * SWS_Wdgm_00013
 */
Wdgm_LocalStatusType Wdgm_GetLocalStatus(Wdgm_SupervisedEntityIdType SEID)
{
    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, SEID, WDGM_SID_GETLOCALSTATUS, WDGM_E_NOT_INITIALIZED);
        return WDGM_LOCAL_STATUS_DEACTIVATED;
    }

    if (!Wdgm_IsValidSEID(SEID))
    {
        (void)Det_ReportError(WDGM_MODULE_ID, SEID, WDGM_SID_GETLOCALSTATUS, WDGM_E_PARAM_SEID);
        return WDGM_LOCAL_STATUS_DEACTIVATED;
    }
    #endif

    return Wdgm_Entities[SEID].LocalStatus;
}

/**
 * @brief Get global supervision status
 * SWS_Wdgm_00014
 */
Wdgm_GlobalStatusType Wdgm_GetGlobalStatus(void)
{
    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_GETGLOBALSTATUS, WDGM_E_NOT_INITIALIZED);
        return WDGM_GLOBAL_STATUS_DEACTIVATED;
    }
    #endif

    return Wdgm_GlobalStatus;
}

/**
 * @brief Perform reset
 * SWS_Wdgm_00015
 */
void Wdgm_PerformReset(void)
{
    /* Trigger watchdog timeout to reset system */
    Wdgm_GlobalStatus = WDGM_GLOBAL_STATUS_EXPIRED;

    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    /* Report runtime error */
    (void)Det_ReportRuntimeError(WDGM_MODULE_ID, 0, WDGM_SID_PERFORMRESET, WDGM_E_DATA_CORRUPT);
    #endif

    /* Stop triggering watchdog - reset will occur on next timeout */
}

/**
 * @brief Get first expired SEID
 * SWS_Wdgm_00016
 */
Wdgm_SupervisedEntityIdType Wdgm_GetFirstExpiredSEID(void)
{
    #if (WDGM_DEV_ERROR_DETECT == STD_ON)
    if (!Wdgm_Initialized)
    {
        (void)Det_ReportError(WDGM_MODULE_ID, 0, WDGM_SID_GETFIRSTEXPIREDSEID, WDGM_E_NOT_INITIALIZED);
        return 0xFFFF;
    }
    #endif

    return Wdgm_FirstExpiredSEID;
}

/**
 * @brief Main function - cyclic supervision
 * SWS_Wdgm_00017
 */
void Wdgm_MainFunction(void)
{
    uint8 i;

    if (!Wdgm_Initialized)
    {
        return;
    }

    Wdgm_CycleCounter++;

    /* Perform alive supervision for all entities */
    #if (WDGM_ALIVE_MONITORING == STD_ON)
    for (i = 0; i < WDGM_MAX_SUPERVISED_ENTITIES; i++)
    {
        Wdgm_CheckAliveSupervision(i);
    }
    #endif

    /* Update global status */
    Wdgm_UpdateGlobalStatus();

    /* Trigger watchdog based on global status */
    Wdgm_TriggerWatchdog();
}

/**
 * @file StbM.c
 * @brief Synchronized Time Base Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : Ethernet
* Dependencies         : Eth, Det
*
* SW Version           : 4.7.0
* Build Version        : YULETECH_AUTOSAR_4.7.0
* Build Date           : 2026-04-29
* Author               : AI Agent (StbM Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "StbM.h"
#include "StbM_Cfg.h"
#include "Eth.h"
#include "Det.h"
#include "MemMap.h"
#include <string.h>

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define STBM_STATE_UNINIT                       (0x00U)
#define STBM_STATE_INIT                         (0x01U)

/* Time conversion constants */
#define STBM_NS_PER_SECOND                      (1000000000ULL)
#define STBM_US_PER_SECOND                      (1000000ULL)
#define STBM_MS_PER_SECOND                      (1000ULL)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (STBM_DEV_ERROR_DETECT == STD_ON)
    #define STBM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(STBM_MODULE_ID, STBM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define STBM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

#define STBM_IS_VALID_TIMEBASE_ID(Id) \
    (((Id) < STBM_NUMBER_OF_TIMEBASES) ? TRUE : FALSE)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct {
    StbM_TimeStampType globalTime;
    StbM_VirtualLocalTimeType localTime;
    StbM_VirtualLocalTimeType lastSyncLocalTime;
    StbM_UserDataType userData;
    uint8 syncStatus;
    uint8 timeBaseStatus;
    uint32 updateCounter;
    StbM_RateDeviationType rateDeviation;
    uint32 timeoutCounter;
    boolean isMaster;
    boolean timeValid;
} StbM_TimeBaseType;

typedef struct {
    uint8 State;
    const StbM_ConfigType* ConfigPtr;
    StbM_TimeBaseType TimeBases[STBM_NUMBER_OF_TIMEBASES];
} StbM_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define STBM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC StbM_InternalStateType StbM_InternalState;

#define STBM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType StbM_FindTimeBaseConfig(uint8 timeBaseId, const StbM_TimeBaseConfigType** configPtr);
STATIC void StbM_UpdateTimeBases(void);
STATIC void StbM_UpdateTimeouts(void);
STATIC StbM_VirtualLocalTimeType StbM_GetVirtualLocalTime(uint8 timeBaseId);
STATIC void StbM_UpdateGlobalTime(uint8 timeBaseId, StbM_VirtualLocalTimeType currentLocalTime);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define STBM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find time base configuration
 */
STATIC Std_ReturnType StbM_FindTimeBaseConfig(uint8 timeBaseId, const StbM_TimeBaseConfigType** configPtr)
{
    Std_ReturnType result = E_NOT_OK;
    const StbM_ConfigType* cfgPtr = StbM_InternalState.ConfigPtr;
    uint8 i;

    if (cfgPtr != NULL_PTR)
    {
        for (i = 0U; i < cfgPtr->numTimeBases; i++)
        {
            if (cfgPtr->timeBaseConfigs[i].timeBaseId == timeBaseId)
            {
                *configPtr = &cfgPtr->timeBaseConfigs[i];
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Get virtual local time from hardware
 */
STATIC StbM_VirtualLocalTimeType StbM_GetVirtualLocalTime(uint8 timeBaseId)
{
    StbM_VirtualLocalTimeType localTime = 0ULL;
    const StbM_TimeBaseConfigType* configPtr;
    Eth_TimeStampType ethTimeStamp;
    Eth_RxStatusType ethStatus;

    (void)timeBaseId;

    /* Get time from Ethernet hardware timestamp */
    if (StbM_FindTimeBaseConfig(timeBaseId, &configPtr) == E_OK)
    {
        if (Eth_GetCurrentTime(configPtr->ethControllerId, &ethTimeStamp, &ethStatus) == E_OK)
        {
            /* Convert Eth_TimeStampType to VirtualLocalTimeType */
            localTime = ((StbM_VirtualLocalTimeType)ethTimeStamp.seconds * STBM_NS_PER_SECOND) +
                        (StbM_VirtualLocalTimeType)ethTimeStamp.nanoseconds;
        }
    }

    return localTime;
}

/**
 * @brief   Update global time based on local time and rate deviation
 */
STATIC void StbM_UpdateGlobalTime(uint8 timeBaseId, StbM_VirtualLocalTimeType currentLocalTime)
{
    StbM_TimeBaseType* tbPtr = &StbM_InternalState.TimeBases[timeBaseId];
    sint64 timeDiff;
    sint64 timeIncrement;

    if ((tbPtr->timeValid) != 0U)
    {
        /* Calculate elapsed time in local ticks */
        timeDiff = (sint64)(currentLocalTime - tbPtr->localTime);

        /* Apply rate correction */
        timeIncrement = (timeDiff * (1000000LL + tbPtr->rateDeviation)) / 1000000LL;

        /* Update global time */
        tbPtr->globalTime.nanoseconds += (uint32)((unsigned int)(timeIncrement) % STBM_NS_PER_SECOND);
        tbPtr->globalTime.seconds += (uint32)((unsigned int)(timeIncrement) / STBM_NS_PER_SECOND);

        /* Handle nanoseconds overflow */
        if (tbPtr->globalTime.nanoseconds >= STBM_NS_PER_SECOND)
        {
            tbPtr->globalTime.nanoseconds -= (uint32)STBM_NS_PER_SECOND;
            tbPtr->globalTime.seconds++;
        }
    }

    /* Update local time */
    tbPtr->localTime = currentLocalTime;
}

/**
 * @brief   Update all time bases
 */
STATIC void StbM_UpdateTimeBases(void)
{
    uint8 i;
    StbM_VirtualLocalTimeType currentTime;

    for (i = 0U; i < STBM_NUMBER_OF_TIMEBASES; i++)
    {
        currentTime = StbM_GetVirtualLocalTime(i);
        StbM_UpdateGlobalTime(i, currentTime);
    }
}

/**
 * @brief   Update timeouts for all time bases
 */
STATIC void StbM_UpdateTimeouts(void)
{
    uint8 i;
    StbM_TimeBaseType* tbPtr;
    const StbM_TimeBaseConfigType* configPtr;

    for (i = 0U; i < STBM_NUMBER_OF_TIMEBASES; i++)
    {
        tbPtr = &StbM_InternalState.TimeBases[i];

        if (tbPtr->syncStatus != STBM_SYNC_STATUS_UNKNOWN)
        {
            if (StbM_FindTimeBaseConfig(i, &configPtr) == E_OK)
            {
                tbPtr->timeoutCounter += STBM_MAIN_FUNCTION_PERIOD_MS;

                if (tbPtr->timeoutCounter >= configPtr->syncTimeout)
                {
                    tbPtr->syncStatus = STBM_SYNC_STATUS_SYNC_LOST;
                    tbPtr->timeBaseStatus = STBM_TIMEBASE_STATUS_TIMEOUT;
                    tbPtr->timeValid = FALSE;
                }
            }
        }
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the StbM module
 */
void StbM_Init(const StbM_ConfigType* ConfigPtr)
{
    uint8 i;

#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (StbM_InternalState.State == STBM_STATE_INIT)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_INIT, STBM_E_ALREADY_INITIALIZED);
        return;
    }

    if (ConfigPtr == NULL_PTR)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_INIT, STBM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    StbM_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize time bases */
    for (i = 0U; i < STBM_NUMBER_OF_TIMEBASES; i++)
    {
        StbM_InternalState.TimeBases[i].globalTime.seconds = 0U;
        StbM_InternalState.TimeBases[i].globalTime.nanoseconds = 0U;
        StbM_InternalState.TimeBases[i].globalTime.secondsHi = 0U;
        StbM_InternalState.TimeBases[i].localTime = 0ULL;
        StbM_InternalState.TimeBases[i].lastSyncLocalTime = 0ULL;
        StbM_InternalState.TimeBases[i].syncStatus = STBM_SYNC_STATUS_UNKNOWN;
        StbM_InternalState.TimeBases[i].timeBaseStatus = STBM_TIMEBASE_STATUS_PENDING;
        StbM_InternalState.TimeBases[i].updateCounter = 0U;
        StbM_InternalState.TimeBases[i].rateDeviation = 0;
        StbM_InternalState.TimeBases[i].timeoutCounter = 0U;
        StbM_InternalState.TimeBases[i].timeValid = FALSE;
        StbM_InternalState.TimeBases[i].userData.userByte0 = 0U;
        StbM_InternalState.TimeBases[i].userData.userByte1 = 0U;
        StbM_InternalState.TimeBases[i].userData.userByte2 = 0U;

        /* Determine master/slave from configuration */
        if (i < ConfigPtr->numTimeBases)
        {
            StbM_InternalState.TimeBases[i].isMaster = 
                (ConfigPtr->timeBaseConfigs[i].masterConfig == STBM_MASTER_CONFIG_MASTER) ?
                TRUE : FALSE;
        }
    }

    /* Set module state to initialized */
    StbM_InternalState.State = STBM_STATE_INIT;
}

/**
 * @brief   Deinitializes the StbM module
 */
void StbM_DeInit(void)
{
#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (StbM_InternalState.State != STBM_STATE_INIT)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_DEINIT, STBM_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    StbM_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    StbM_InternalState.State = STBM_STATE_UNINIT;
}

/**
 * @brief   Gets version information
 */
#if (STBM_VERSION_INFO_API == STD_ON)
void StbM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETVERSIONINFO, STBM_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = STBM_VENDOR_ID;
    versioninfo->moduleID = STBM_MODULE_ID;
    versioninfo->sw_major_version = STBM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = STBM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = STBM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief   Gets current synchronized time
 */
Std_ReturnType StbM_GetCurrentTime(uint8 timeBaseId, 
                                    StbM_TimeStampType* timeStampPtr,
                                    StbM_UserDataType* userDataPtr)
{
    Std_ReturnType result = E_NOT_OK;
    StbM_TimeBaseType* tbPtr;

#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (StbM_InternalState.State != STBM_STATE_INIT)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTTIME, STBM_E_UNINIT);
        return E_NOT_OK;
    }

    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTTIME, STBM_E_INVALID_TIMEBASE_ID);
        return E_NOT_OK;
    }

    if (timeStampPtr == NULL_PTR)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTTIME, STBM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    tbPtr = &StbM_InternalState.TimeBases[timeBaseId];

    if ((tbPtr->timeValid) != 0U)
    {
        /* Update time before returning */
        StbM_VirtualLocalTimeType currentTime = StbM_GetVirtualLocalTime(timeBaseId);
        StbM_UpdateGlobalTime(timeBaseId, currentTime);

        /* Copy current time */
        timeStampPtr->seconds = tbPtr->globalTime.seconds;
        timeStampPtr->nanoseconds = tbPtr->globalTime.nanoseconds;
        timeStampPtr->secondsHi = tbPtr->globalTime.secondsHi;

        /* Copy user data if requested */
        if (userDataPtr != NULL_PTR)
        {
            userDataPtr->userByte0 = tbPtr->userData.userByte0;
            userDataPtr->userByte1 = tbPtr->userData.userByte1;
            userDataPtr->userByte2 = tbPtr->userData.userByte2;
        }

        result = E_OK;
    }

    return result;
}

/**
 * @brief   Gets current virtual local time
 */
Std_ReturnType StbM_GetCurrentVirtualTime(uint8 timeBaseId,
                                           StbM_VirtualLocalTimeType* virtualLocalTimePtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (StbM_InternalState.State != STBM_STATE_INIT)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTVIRTUALTIME, STBM_E_UNINIT);
        return E_NOT_OK;
    }

    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTVIRTUALTIME, STBM_E_INVALID_TIMEBASE_ID);
        return E_NOT_OK;
    }

    if (virtualLocalTimePtr == NULL_PTR)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_GETCURRENTVIRTUALTIME, STBM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *virtualLocalTimePtr = StbM_GetVirtualLocalTime(timeBaseId);
    result = E_OK;

    return result;
}

/**
 * @brief   Sets global time
 */
Std_ReturnType StbM_SetGlobalTime(uint8 timeBaseId,
                                   const StbM_TimeStampType* timeStampPtr,
                                   const StbM_UserDataType* userDataPtr)
{
    Std_ReturnType result = E_NOT_OK;
    StbM_TimeBaseType* tbPtr;

#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (StbM_InternalState.State != STBM_STATE_INIT)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_SETGLOBALTIME, STBM_E_UNINIT);
        return E_NOT_OK;
    }

    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
    {
        STBM_DET_REPORT_ERROR(STBM_SID_SETGLOBALTIME, STBM_E_INVALID_TIMEBASE_ID);
        return E_NOT_OK;
    }

    if (timeStampPtr == NULL_PTR)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_SETGLOBALTIME, STBM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    tbPtr = &StbM_InternalState.TimeBases[timeBaseId];

    /* Only master can set global time directly */
    if ((tbPtr->isMaster) != 0U)
    {
        tbPtr->globalTime.seconds = timeStampPtr->seconds;
        tbPtr->globalTime.nanoseconds = timeStampPtr->nanoseconds;
        tbPtr->globalTime.secondsHi = timeStampPtr->secondsHi;
        tbPtr->localTime = StbM_GetVirtualLocalTime(timeBaseId);
        tbPtr->timeValid = TRUE;
        tbPtr->updateCounter++;

        if (userDataPtr != NULL_PTR)
        {
            tbPtr->userData.userByte0 = userDataPtr->userByte0;
            tbPtr->userData.userByte1 = userDataPtr->userByte1;
            tbPtr->userData.userByte2 = userDataPtr->userByte2;
        }

        result = E_OK;
    }

    return result;
}

/**
 * @brief   Sets global time from bus (time sync protocol)
 */
Std_ReturnType StbM_BusSetGlobalTime(uint8 timeBaseId,
                                      const StbM_TimeStampType* timeStampPtr,
                                      const StbM_VirtualLocalTimeType* virtualLocalTimePtr,
                                      const StbM_UserDataType* userDataPtr)
{
    Std_ReturnType result = E_NOT_OK;
    StbM_TimeBaseType* tbPtr;
    StbM_VirtualLocalTimeType rxLocalTime;

#if (STBM_DEV_ERROR_DETECT == STD_ON)
    if (StbM_InternalState.State != STBM_STATE_INIT)
    {
        STBM_DET_REPORT_ERROR(STBM_SID_BUSSETGLOBALTIME, STBM_E_UNINIT);
        return E_NOT_OK;
    }

    if (!STBM_IS_VALID_TIMEBASE_ID(timeBaseId))
    {
        STBM_DET_REPORT_ERROR(STBM_SID_BUSSETGLOBALTIME, STBM_E_INVALID_TIMEBASE_ID);
        return E_NOT_OK;
    }

    if ((timeStampPtr == NULL_PTR) || (virtualLocalTimePtr == NULL_PTR))
    {
        STBM_DET_REPORT_ERROR(STBM_SID_BUSSETGLOBALTIME, STBM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    tbPtr = &StbM_InternalState.TimeBases[timeBaseId];

    /* Store the received global time and the corresponding local time */
    tbPtr->globalTime = *timeStampPtr;
    tbPtr->localTime = *virtualLocalTimePtr;
    tbPtr->updateCounter++;
    tbPtr->timeValid = TRUE;
    tbPtr->syncStatus = STBM_SYNC_STATUS_SYNC;

    result = E_OK;
    return result;
}

#define STBM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/

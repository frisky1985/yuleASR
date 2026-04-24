/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Integration Layer)
* Dependencies         : EcuM, Com, PduR, CanIf, NvM, Dcm, Dem
*
* SW Version           : 1.0.0
* Build Version        : YULETECH_BSWM_1.0.0
* Build Date           : 2026-04-24
* Author               : AI Agent (BswM Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "BswM.h"
#include "Det.h"
#include "MemMap.h"

/* Service Layer headers for mode actions */
#include "Com.h"
#include "PduR.h"
#include "NvM.h"
#include "Dcm.h"
#include "Dem.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define BSWM_INSTANCE_ID                (0x00U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    #define BSWM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define BSWM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Mode request queue entry */
typedef struct
{
    BswM_UserType requestingUser;
    uint8 requestedMode;
    boolean IsValid;
} BswM_ModeRequestEntryType;

/* Module internal state */
typedef struct
{
    uint8 State;
    BswM_StateType ModuleState;
    const BswM_ConfigType* ConfigPtr;
    BswM_ModeType CurrentMode[BSWM_NUM_REQUESTING_USERS];
    BswM_ModeRequestEntryType ModeRequestQueue[BSWM_MODE_REQUEST_QUEUE_SIZE];
    uint8 QueueHead;
    uint8 QueueTail;
    uint8 QueueCount;
    boolean ModeRequestPending;
    BswM_ModeType ArbitrationResult;
    boolean ArbitrationChanged;
} BswM_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define BSWM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC BswM_InternalStateType BswM_InternalState;

#define BSWM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void BswM_ProcessModeRequest(BswM_UserType requestingUser, BswM_ModeType requestedMode);
STATIC void BswM_ExecuteModeActions(BswM_ModeType mode);
STATIC void BswM_ExecuteRules(void);
STATIC BswM_ModeType BswM_ArbitrateMode(void);
STATIC void BswM_EnterModeRun(void);
STATIC void BswM_EnterModeShutdown(void);
STATIC void BswM_EnterModeSleep(void);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define BSWM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Enter RUN mode actions
 */
STATIC void BswM_EnterModeRun(void)
{
    /* Start COM I-PDU groups */
    Com_IpduGroupControl(NULL_PTR, TRUE);

    /* Enable PDU routing */
    PduR_EnableRouting(0U);

    /* Switch NvM to normal write mode */
    /* NvM_SetBlockProtection(...) would be called here */
}

/**
 * @brief   Enter SHUTDOWN mode actions
 */
STATIC void BswM_EnterModeShutdown(void)
{
    /* Stop COM I-PDU groups */
    Com_IpduGroupControl(NULL_PTR, FALSE);

    /* Disable PDU routing */
    PduR_DisableRouting(0U);

    /* Write all pending NvM blocks */
    /* NvM_WriteAll() would be called here */
}

/**
 * @brief   Enter SLEEP mode actions
 */
STATIC void BswM_EnterModeSleep(void)
{
    /* Prepare communication for sleep */
    Com_IpduGroupControl(NULL_PTR, FALSE);

    /* Disable PDU routing */
    PduR_DisableRouting(0U);
}

/**
 * @brief   Execute mode-specific actions
 */
STATIC void BswM_ExecuteModeActions(BswM_ModeType mode)
{
    switch (mode)
    {
        case BSWM_MODE_STARTUP:
            /* Communication stack not yet initialized */
            break;

        case BSWM_MODE_RUN:
            BswM_EnterModeRun();
            break;

        case BSWM_MODE_POST_RUN:
            /* Prepare for shutdown - keep minimal communication */
            break;

        case BSWM_MODE_SHUTDOWN:
            BswM_EnterModeShutdown();
            break;

        case BSWM_MODE_SLEEP:
            BswM_EnterModeSleep();
            break;

        default:
            break;
    }
}

/**
 * @brief   Mode arbitration logic
 *
 * Simple state machine: STARTUP -> RUN -> SHUTDOWN/SLEEP
 * The mode with the highest priority request wins.
 */
STATIC BswM_ModeType BswM_ArbitrateMode(void)
{
    BswM_ModeType arbitrateMode = BSWM_MODE_STARTUP;
    uint8 i;

    for (i = 0U; i < BSWM_NUM_REQUESTING_USERS; i++)
    {
        BswM_ModeType userMode = BswM_InternalState.CurrentMode[i];

        /* Higher numeric value = higher priority for RUN/SHUTDOWN */
        if (userMode > arbitrateMode)
        {
            arbitrateMode = userMode;
        }
    }

    return arbitrateMode;
}

/**
 * @brief   Process a mode request
 */
STATIC void BswM_ProcessModeRequest(BswM_UserType requestingUser, BswM_ModeType requestedMode)
{
    if (requestingUser < BSWM_NUM_REQUESTING_USERS)
    {
        /* Store the requested mode */
        BswM_InternalState.CurrentMode[requestingUser] = requestedMode;

        /* Run arbitration */
        BswM_InternalState.ArbitrationResult = BswM_ArbitrateMode();
        BswM_InternalState.ArbitrationChanged = TRUE;
    }
}

/**
 * @brief   Execute BswM rules
 */
STATIC void BswM_ExecuteRules(void)
{
    uint8 i;

    if (BswM_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < BswM_InternalState.ConfigPtr->NumRules; i++)
        {
            const BswM_RuleConfigType* rulePtr = &BswM_InternalState.ConfigPtr->Rules[i];

            if (rulePtr->ConditionFnc != NULL_PTR)
            {
                if (rulePtr->ConditionFnc() == TRUE)
                {
                    /* Execute true action list */
                    if (rulePtr->TrueActionList != NULL_PTR)
                    {
                        rulePtr->TrueActionList();
                    }
                }
                else
                {
                    /* Execute false action list */
                    if (rulePtr->FalseActionList != NULL_PTR)
                    {
                        rulePtr->FalseActionList();
                    }
                }
            }
        }
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the BswM module
 */
void BswM_Init(const BswM_ConfigType* ConfigPtr)
{
    uint8 i;

#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_INIT, BSWM_E_PARAM_CONFIG);
        return;
    }
#endif

    /* Store configuration pointer */
    BswM_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize mode request queue */
    BswM_InternalState.QueueHead = 0U;
    BswM_InternalState.QueueTail = 0U;
    BswM_InternalState.QueueCount = 0U;
    BswM_InternalState.ModeRequestPending = FALSE;
    BswM_InternalState.ArbitrationResult = BSWM_MODE_STARTUP;
    BswM_InternalState.ArbitrationChanged = FALSE;

    /* Initialize current modes to default */
    for (i = 0U; i < BSWM_NUM_REQUESTING_USERS; i++)
    {
        BswM_InternalState.CurrentMode[i] = BSWM_MODE_STARTUP;
    }

    /* Clear mode request queue */
    for (i = 0U; i < BSWM_MODE_REQUEST_QUEUE_SIZE; i++)
    {
        BswM_InternalState.ModeRequestQueue[i].IsValid = FALSE;
    }

    /* Set module state to initialized */
    BswM_InternalState.State = BSWM_STATE_INIT;
    BswM_InternalState.ModuleState = BSWM_STATE_INIT;
}

/**
 * @brief   Deinitializes the BswM module
 */
void BswM_Deinit(void)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_InternalState.State != BSWM_STATE_INIT)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_DEINIT, BSWM_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    BswM_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    BswM_InternalState.State = BSWM_STATE_UNINIT;
    BswM_InternalState.ModuleState = BSWM_STATE_UNINIT;
}

/**
 * @brief   Main function for BswM processing
 *
 * Performs mode arbitration and executes mode switch actions.
 */
void BswM_MainFunction(void)
{
    uint8 i;

    if (BswM_InternalState.State == BSWM_STATE_INIT)
    {
        BswM_InternalState.ModuleState = BSWM_STATE_BUSY;

        /* Process pending mode requests */
        if (BswM_InternalState.ModeRequestPending)
        {
            for (i = 0U; i < BSWM_MODE_REQUEST_QUEUE_SIZE; i++)
            {
                if (BswM_InternalState.ModeRequestQueue[i].IsValid)
                {
                    BswM_ProcessModeRequest(
                        BswM_InternalState.ModeRequestQueue[i].requestingUser,
                        BswM_InternalState.ModeRequestQueue[i].requestedMode);

                    BswM_InternalState.ModeRequestQueue[i].IsValid = FALSE;
                }
            }

            BswM_InternalState.ModeRequestPending = FALSE;
        }

        /* Execute mode actions if arbitration result changed */
        if (BswM_InternalState.ArbitrationChanged)
        {
            BswM_ExecuteModeActions(BswM_InternalState.ArbitrationResult);
            BswM_InternalState.ArbitrationChanged = FALSE;
        }

        /* Execute rules periodically */
        BswM_ExecuteRules();

        BswM_InternalState.ModuleState = BSWM_STATE_INIT;
    }
}

/**
 * @brief   Request a mode change
 */
void BswM_RequestMode(BswM_UserType requestingUser, BswM_ModeType requestedMode)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_InternalState.State != BSWM_STATE_INIT)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_REQUESTMODE, BSWM_E_UNINIT);
        return;
    }

    if (requestingUser >= BSWM_NUM_REQUESTING_USERS)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_REQUESTMODE, BSWM_E_REQ_USER_OUT_OF_RANGE);
        return;
    }

    if (requestedMode >= BSWM_MODE_MAX)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_REQUESTMODE, BSWM_E_REQ_MODE_OUT_OF_RANGE);
        return;
    }
#endif

    if ((requestingUser < BSWM_NUM_REQUESTING_USERS) && (requestedMode < BSWM_MODE_MAX))
    {
        /* Add to mode request queue */
        if (BswM_InternalState.QueueCount < BSWM_MODE_REQUEST_QUEUE_SIZE)
        {
            BswM_InternalState.ModeRequestQueue[BswM_InternalState.QueueTail].requestingUser = requestingUser;
            BswM_InternalState.ModeRequestQueue[BswM_InternalState.QueueTail].requestedMode = requestedMode;
            BswM_InternalState.ModeRequestQueue[BswM_InternalState.QueueTail].IsValid = TRUE;

            BswM_InternalState.QueueTail = (BswM_InternalState.QueueTail + 1U) % BSWM_MODE_REQUEST_QUEUE_SIZE;
            BswM_InternalState.QueueCount++;
            BswM_InternalState.ModeRequestPending = TRUE;
        }
    }
}

/**
 * @brief   Get the current mode for a requesting user
 */
BswM_ModeType BswM_GetCurrentMode(BswM_UserType requestingUser)
{
    BswM_ModeType result = BSWM_MODE_STARTUP;

#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BswM_InternalState.State != BSWM_STATE_INIT)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_GETMODE, BSWM_E_UNINIT);
        return BSWM_MODE_STARTUP;
    }

    if (requestingUser >= BSWM_NUM_REQUESTING_USERS)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_GETMODE, BSWM_E_REQ_USER_OUT_OF_RANGE);
        return BSWM_MODE_STARTUP;
    }
#endif

    if (requestingUser < BSWM_NUM_REQUESTING_USERS)
    {
        result = BswM_InternalState.CurrentMode[requestingUser];
    }

    return result;
}

/**
 * @brief   Get the current BswM module state
 */
BswM_StateType BswM_GetState(void)
{
    return BswM_InternalState.ModuleState;
}

/**
 * @brief   Get version information
 */
#if (BSWM_VERSION_INFO_API == STD_ON)
void BswM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        BSWM_DET_REPORT_ERROR(BSWM_SERVICE_ID_GETVERSIONINFO, BSWM_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = BSWM_VENDOR_ID;
        versioninfo->moduleID = BSWM_MODULE_ID;
        versioninfo->sw_major_version = BSWM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = BSWM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = BSWM_SW_PATCH_VERSION;
    }
}
#endif

#define BSWM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/

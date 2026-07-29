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

/*==================================================================================================
 *                                      FEE-FLS INTEGRATION LAYER
 *==================================================================================================
 * FILENAME: Fee_Fls_Integration.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Fee-Fls integration layer
 *              Provides standardized interface between Fee and Fls modules
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Fee_Fls_Integration.h"
#include "Det.h"

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define FEE_FLS_INT_INSTANCE_ID         (0u)

/* Timeout definitions (in milliseconds) */
#define FEE_FLS_INT_DEFAULT_READ_TIMEOUT    (1000u)
#define FEE_FLS_INT_DEFAULT_WRITE_TIMEOUT   (5000u)
#define FEE_FLS_INT_DEFAULT_ERASE_TIMEOUT   (10000u)

#define FEE_FLS_INT_MAX_RETRIES             (3u)

/*==================================================================================================
 *                                    LOCAL TYPEDEFS
 *==================================================================================================*/
typedef enum {
    FEE_FLS_INT_OP_NONE = 0,
    FEE_FLS_INT_OP_READ,
    FEE_FLS_INT_OP_WRITE,
    FEE_FLS_INT_OP_ERASE,
    FEE_FLS_INT_OP_COMPARE
} Fee_Fls_Int_OperationType;

typedef struct {
    Fee_Fls_Int_OperationType CurrentOp;
    uint32 StartTime;
    uint32 Timeout;
    uint8 RetryCount;
    boolean JobPending;
    boolean JobCompleted;
    boolean JobFailed;
} Fee_Fls_Int_JobControlType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define FEE_FLS_INT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Fee_Fls_Int_Initialized = FALSE;
static Fee_Fls_Int_StateType Fee_Fls_Int_State = FEE_FLS_INT_STATE_UNINIT;
static Fee_Fls_Int_ConfigType Fee_Fls_Int_Config;
static Fee_Fls_Int_StatsType Fee_Fls_Int_Stats;
static Fee_Fls_Int_JobControlType Fee_Fls_Int_JobControl;

#define FEE_FLS_INT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
#define FEE_FLS_INT_START_SEC_CODE
#include "MemMap.h"

static void Fee_Fls_Int_ResetJobControl(void);
static Fee_Fls_Int_StatusType Fee_Fls_Int_WaitForJobCompletion(uint32 Timeout);
static void Fee_Fls_Int_UpdateStats(Fee_Fls_Int_OperationType Op, uint32 Duration, boolean Success);
static void Fee_Fls_Int_HandleJobSuccess(void);
static void Fee_Fls_Int_HandleJobError(void);

/*==================================================================================================
 *                                    API IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Fee-Fls integration layer
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_Init(const Fee_Fls_Int_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR)
    {
        /* Use default configuration */
        Fee_Fls_Int_Config.MaxReadTimeout = FEE_FLS_INT_DEFAULT_READ_TIMEOUT;
        Fee_Fls_Int_Config.MaxWriteTimeout = FEE_FLS_INT_DEFAULT_WRITE_TIMEOUT;
        Fee_Fls_Int_Config.MaxEraseTimeout = FEE_FLS_INT_DEFAULT_ERASE_TIMEOUT;
        Fee_Fls_Int_Config.EnableIntegrityCheck = TRUE;
        Fee_Fls_Int_Config.EnableStatistics = TRUE;
        Fee_Fls_Int_Config.MaxRetries = FEE_FLS_INT_MAX_RETRIES;
    }
    else
    {
        Fee_Fls_Int_Config = *ConfigPtr;
    }

    /* Reset statistics */
    Fee_Fls_Int_Stats.TotalReadOperations = 0u;
    Fee_Fls_Int_Stats.TotalWriteOperations = 0u;
    Fee_Fls_Int_Stats.TotalEraseOperations = 0u;
    Fee_Fls_Int_Stats.FailedOperations = 0u;
    Fee_Fls_Int_Stats.TimeoutCount = 0u;
    Fee_Fls_Int_Stats.IntegrityErrors = 0u;
    Fee_Fls_Int_Stats.AverageReadTime = 0u;
    Fee_Fls_Int_Stats.AverageWriteTime = 0u;
    Fee_Fls_Int_Stats.AverageEraseTime = 0u;

    /* Reset job control */
    Fee_Fls_Int_ResetJobControl();

    Fee_Fls_Int_Initialized = TRUE;
    Fee_Fls_Int_State = FEE_FLS_INT_STATE_IDLE;

    return FEE_FLS_INT_E_OK;
}

/**
 * @brief De-initializes the integration layer
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_DeInit(void)
{
    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    /* Cancel any pending operation */
    if (Fee_Fls_Int_JobControl.JobPending)
    {
        Fls_Cancel();
    }

    Fee_Fls_Int_Initialized = FALSE;
    Fee_Fls_Int_State = FEE_FLS_INT_STATE_UNINIT;

    return FEE_FLS_INT_E_OK;
}

/**
 * @brief Reads data from Flash via Fls
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_Read(uint32 Address, uint8* DataPtr, uint32 Length)
{
    Fee_Fls_Int_StatusType result = FEE_FLS_INT_E_OK;
    uint32 startTime;
    uint32 duration;

    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    if (DataPtr == NULL_PTR)
    {
        return FEE_FLS_INT_E_PARAM_POINTER;
    }

    if (Length == 0u)
    {
        return FEE_FLS_INT_E_PARAM_LENGTH;
    }

    /* Check if another operation is pending */
    if (Fee_Fls_Int_JobControl.JobPending)
    {
        return FEE_FLS_INT_E_BUSY;
    }

    Fee_Fls_Int_State = FEE_FLS_INT_STATE_BUSY;
    startTime = 0u;  /* In real implementation, get from GPT or OS */

    /* Setup job control */
    Fee_Fls_Int_JobControl.CurrentOp = FEE_FLS_INT_OP_READ;
    Fee_Fls_Int_JobControl.StartTime = startTime;
    Fee_Fls_Int_JobControl.Timeout = Fee_Fls_Int_Config.MaxReadTimeout;
    Fee_Fls_Int_JobControl.RetryCount = 0u;
    Fee_Fls_Int_JobControl.JobPending = TRUE;
    Fee_Fls_Int_JobControl.JobCompleted = FALSE;
    Fee_Fls_Int_JobControl.JobFailed = FALSE;

    /* Start FLS read operation */
    Fls_Read((Fls_AddressType)Address, DataPtr, (Fls_LengthType)Length);

    /* Wait for completion (blocking mode) or return pending (non-blocking) */
#if (FEE_POLL_MODE == STD_ON)
    result = Fee_Fls_Int_WaitForJobCompletion(Fee_Fls_Int_Config.MaxReadTimeout);
#endif

    /* Update statistics */
    duration = 0u;  /* Calculate duration */
    Fee_Fls_Int_UpdateStats(FEE_FLS_INT_OP_READ, duration,
                            (result == FEE_FLS_INT_E_OK));

    if (result != FEE_FLS_INT_E_OK)
    {
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_ERROR;
    }
    else
    {
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_IDLE;
    }

    return result;
}

/**
 * @brief Writes data to Flash via Fls
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_Write(uint32 Address, const uint8* DataPtr, uint32 Length)
{
    Fee_Fls_Int_StatusType result = FEE_FLS_INT_E_OK;
    Std_ReturnType flsResult;
    uint32 startTime;
    uint32 duration;

    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    if (DataPtr == NULL_PTR)
    {
        return FEE_FLS_INT_E_PARAM_POINTER;
    }

    if (Length == 0u)
    {
        return FEE_FLS_INT_E_PARAM_LENGTH;
    }

    /* Check if another operation is pending */
    if (Fee_Fls_Int_JobControl.JobPending)
    {
        return FEE_FLS_INT_E_BUSY;
    }

    Fee_Fls_Int_State = FEE_FLS_INT_STATE_BUSY;
    startTime = 0u;

    /* Setup job control */
    Fee_Fls_Int_JobControl.CurrentOp = FEE_FLS_INT_OP_WRITE;
    Fee_Fls_Int_JobControl.StartTime = startTime;
    Fee_Fls_Int_JobControl.Timeout = Fee_Fls_Int_Config.MaxWriteTimeout;
    Fee_Fls_Int_JobControl.RetryCount = 0u;
    Fee_Fls_Int_JobControl.JobPending = TRUE;
    Fee_Fls_Int_JobControl.JobCompleted = FALSE;
    Fee_Fls_Int_JobControl.JobFailed = FALSE;

    /* Start FLS write operation */
    flsResult = Fls_Write((Fls_AddressType)Address, DataPtr, (Fls_LengthType)Length);

    if (flsResult == E_OK)
    {
#if (FEE_POLL_MODE == STD_ON)
        result = Fee_Fls_Int_WaitForJobCompletion(Fee_Fls_Int_Config.MaxWriteTimeout);
#endif
    }
    else
    {
        result = FEE_FLS_INT_E_NOT_OK;
    }

    /* Update statistics */
    duration = 0u;
    Fee_Fls_Int_UpdateStats(FEE_FLS_INT_OP_WRITE, duration,
                            (result == FEE_FLS_INT_E_OK));

    if (result != FEE_FLS_INT_E_OK)
    {
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_ERROR;
    }
    else
    {
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_IDLE;
    }

    return result;
}

/**
 * @brief Erases Flash sector(s) via Fls
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_Erase(uint32 Address, uint32 Length)
{
    Fee_Fls_Int_StatusType result = FEE_FLS_INT_E_OK;
    Std_ReturnType flsResult;
    uint32 startTime;
    uint32 duration;

    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    if (Length == 0u)
    {
        return FEE_FLS_INT_E_PARAM_LENGTH;
    }

    /* Check if another operation is pending */
    if (Fee_Fls_Int_JobControl.JobPending)
    {
        return FEE_FLS_INT_E_BUSY;
    }

    Fee_Fls_Int_State = FEE_FLS_INT_STATE_BUSY;
    startTime = 0u;

    /* Setup job control */
    Fee_Fls_Int_JobControl.CurrentOp = FEE_FLS_INT_OP_ERASE;
    Fee_Fls_Int_JobControl.StartTime = startTime;
    Fee_Fls_Int_JobControl.Timeout = Fee_Fls_Int_Config.MaxEraseTimeout;
    Fee_Fls_Int_JobControl.RetryCount = 0u;
    Fee_Fls_Int_JobControl.JobPending = TRUE;
    Fee_Fls_Int_JobControl.JobCompleted = FALSE;
    Fee_Fls_Int_JobControl.JobFailed = FALSE;

    /* Start FLS erase operation */
    flsResult = Fls_Erase((Fls_AddressType)Address, (Fls_LengthType)Length);

    if (flsResult == E_OK)
    {
#if (FEE_POLL_MODE == STD_ON)
        result = Fee_Fls_Int_WaitForJobCompletion(Fee_Fls_Int_Config.MaxEraseTimeout);
#endif
    }
    else
    {
        result = FEE_FLS_INT_E_NOT_OK;
    }

    /* Update statistics */
    duration = 0u;
    Fee_Fls_Int_UpdateStats(FEE_FLS_INT_OP_ERASE, duration,
                            (result == FEE_FLS_INT_E_OK));

    if (result != FEE_FLS_INT_E_OK)
    {
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_ERROR;
    }
    else
    {
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_IDLE;
    }

    return result;
}

/**
 * @brief Compares Flash data with buffer via Fls
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_Compare(uint32 Address, const uint8* DataPtr, uint32 Length)
{
    Fee_Fls_Int_StatusType result = FEE_FLS_INT_E_OK;
    uint32 startTime;
    uint32 duration;

    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    if (DataPtr == NULL_PTR)
    {
        return FEE_FLS_INT_E_PARAM_POINTER;
    }

    if (Length == 0u)
    {
        return FEE_FLS_INT_E_PARAM_LENGTH;
    }

    /* Check if another operation is pending */
    if (Fee_Fls_Int_JobControl.JobPending)
    {
        return FEE_FLS_INT_E_BUSY;
    }

    Fee_Fls_Int_State = FEE_FLS_INT_STATE_BUSY;
    startTime = 0u;

    /* Setup job control */
    Fee_Fls_Int_JobControl.CurrentOp = FEE_FLS_INT_OP_COMPARE;
    Fee_Fls_Int_JobControl.StartTime = startTime;
    Fee_Fls_Int_JobControl.Timeout = Fee_Fls_Int_Config.MaxReadTimeout;
    Fee_Fls_Int_JobControl.RetryCount = 0u;
    Fee_Fls_Int_JobControl.JobPending = TRUE;
    Fee_Fls_Int_JobControl.JobCompleted = FALSE;
    Fee_Fls_Int_JobControl.JobFailed = FALSE;

    /* Start FLS compare operation */
    Fls_Compare((Fls_AddressType)Address, DataPtr, (Fls_LengthType)Length);

#if (FEE_POLL_MODE == STD_ON)
    result = Fee_Fls_Int_WaitForJobCompletion(Fee_Fls_Int_Config.MaxReadTimeout);
#endif

    /* Update statistics */
    duration = 0u;
    Fee_Fls_Int_UpdateStats(FEE_FLS_INT_OP_READ, duration,
                            (result == FEE_FLS_INT_E_OK));

    Fee_Fls_Int_State = FEE_FLS_INT_STATE_IDLE;

    return result;
}

/**
 * @brief Gets current integration state
 */
Fee_Fls_Int_StateType Fee_Fls_Int_GetState(void)
{
    return Fee_Fls_Int_State;
}

/**
 * @brief Gets operation statistics
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_GetStatistics(Fee_Fls_Int_StatsType* StatsPtr)
{
    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    if (StatsPtr == NULL_PTR)
    {
        return FEE_FLS_INT_E_PARAM_POINTER;
    }

    if (Fee_Fls_Int_Config.EnableStatistics == TRUE)
    {
        *StatsPtr = Fee_Fls_Int_Stats;
        return FEE_FLS_INT_E_OK;
    }

    return FEE_FLS_INT_E_NOT_OK;
}

/**
 * @brief Clears operation statistics
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_ClearStatistics(void)
{
    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    Fee_Fls_Int_Stats.TotalReadOperations = 0u;
    Fee_Fls_Int_Stats.TotalWriteOperations = 0u;
    Fee_Fls_Int_Stats.TotalEraseOperations = 0u;
    Fee_Fls_Int_Stats.FailedOperations = 0u;
    Fee_Fls_Int_Stats.TimeoutCount = 0u;
    Fee_Fls_Int_Stats.IntegrityErrors = 0u;
    Fee_Fls_Int_Stats.AverageReadTime = 0u;
    Fee_Fls_Int_Stats.AverageWriteTime = 0u;
    Fee_Fls_Int_Stats.AverageEraseTime = 0u;

    return FEE_FLS_INT_E_OK;
}

/**
 * @brief Main function for periodic processing
 */
Fee_Fls_Int_StatusType Fee_Fls_Int_MainFunction(void)
{
    MemIf_JobResultType flsResult;

    if (Fee_Fls_Int_Initialized == FALSE)
    {
        return FEE_FLS_INT_E_NOT_OK;
    }

    /* Call Fls MainFunction to process jobs */
    Fls_MainFunction();

    /* Check Fls job status if we have a pending job */
    if (Fee_Fls_Int_JobControl.JobPending)
    {
        flsResult = Fls_GetJobResult();

        if (flsResult == MEMIF_JOB_OK)
        {
            Fee_Fls_Int_HandleJobSuccess();
        }
        else if (flsResult == MEMIF_JOB_FAILED)
        {
            Fee_Fls_Int_HandleJobError();
        }
        /* else: still pending */
    }

    return FEE_FLS_INT_E_OK;
}

/**
 * @brief Handles Fls job end notification
 */
void Fee_Fls_Int_JobEndNotification(void)
{
    Fee_Fls_Int_HandleJobSuccess();
}

/**
 * @brief Handles Fls job error notification
 */
void Fee_Fls_Int_JobErrorNotification(void)
{
    Fee_Fls_Int_HandleJobError();
}

/*==================================================================================================
 *                                    LOCAL FUNCTION IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Resets job control structure
 */
static void Fee_Fls_Int_ResetJobControl(void)
{
    Fee_Fls_Int_JobControl.CurrentOp = FEE_FLS_INT_OP_NONE;
    Fee_Fls_Int_JobControl.StartTime = 0u;
    Fee_Fls_Int_JobControl.Timeout = 0u;
    Fee_Fls_Int_JobControl.RetryCount = 0u;
    Fee_Fls_Int_JobControl.JobPending = FALSE;
    Fee_Fls_Int_JobControl.JobCompleted = FALSE;
    Fee_Fls_Int_JobControl.JobFailed = FALSE;
}

/**
 * @brief Waits for FLS job completion with timeout
 */
static Fee_Fls_Int_StatusType Fee_Fls_Int_WaitForJobCompletion(uint32 Timeout)
{
    MemIf_JobResultType flsResult;
    uint32 elapsedTime = 0u;

    while (elapsedTime < Timeout)
    {
        /* Call Fls MainFunction to process the job */
        Fls_MainFunction();

        /* Check job result */
        flsResult = Fls_GetJobResult();

        if (flsResult == MEMIF_JOB_OK)
        {
            return FEE_FLS_INT_E_OK;
        }
        else if (flsResult == MEMIF_JOB_FAILED)
        {
            return FEE_FLS_INT_E_FLASH_ERROR;
        }

        /* Increment elapsed time (in real implementation, use proper timing) */
        elapsedTime++;
    }

    /* Timeout occurred */
    Fee_Fls_Int_Stats.TimeoutCount++;
    return FEE_FLS_INT_E_TIMEOUT;
}

/**
 * @brief Updates operation statistics
 */
static void Fee_Fls_Int_UpdateStats(Fee_Fls_Int_OperationType Op, uint32 Duration, boolean Success)
{
    if (Fee_Fls_Int_Config.EnableStatistics == FALSE)
    {
        return;
    }

    switch (Op)
    {
        case FEE_FLS_INT_OP_READ:
            Fee_Fls_Int_Stats.TotalReadOperations++;
            if (Fee_Fls_Int_Stats.TotalReadOperations > 1)
            {
                /* Calculate running average */
                Fee_Fls_Int_Stats.AverageReadTime =
                    (Fee_Fls_Int_Stats.AverageReadTime * (Fee_Fls_Int_Stats.TotalReadOperations - 1) + Duration) /
                    Fee_Fls_Int_Stats.TotalReadOperations;
            }
            else
            {
                Fee_Fls_Int_Stats.AverageReadTime = Duration;
            }
            break;

        case FEE_FLS_INT_OP_WRITE:
            Fee_Fls_Int_Stats.TotalWriteOperations++;
            if (Fee_Fls_Int_Stats.TotalWriteOperations > 1)
            {
                Fee_Fls_Int_Stats.AverageWriteTime =
                    (Fee_Fls_Int_Stats.AverageWriteTime * (Fee_Fls_Int_Stats.TotalWriteOperations - 1) + Duration) /
                    Fee_Fls_Int_Stats.TotalWriteOperations;
            }
            else
            {
                Fee_Fls_Int_Stats.AverageWriteTime = Duration;
            }
            break;

        case FEE_FLS_INT_OP_ERASE:
            Fee_Fls_Int_Stats.TotalEraseOperations++;
            if (Fee_Fls_Int_Stats.TotalEraseOperations > 1)
            {
                Fee_Fls_Int_Stats.AverageEraseTime =
                    (Fee_Fls_Int_Stats.AverageEraseTime * (Fee_Fls_Int_Stats.TotalEraseOperations - 1) + Duration) /
                    Fee_Fls_Int_Stats.TotalEraseOperations;
            }
            else
            {
                Fee_Fls_Int_Stats.AverageEraseTime = Duration;
            }
            break;

        default:
            break;
    }

    if (Success == FALSE)
    {
        Fee_Fls_Int_Stats.FailedOperations++;
    }
}

/**
 * @brief Handles successful job completion
 */
static void Fee_Fls_Int_HandleJobSuccess(void)
{
    Fee_Fls_Int_JobControl.JobCompleted = TRUE;
    Fee_Fls_Int_JobControl.JobPending = FALSE;
    Fee_Fls_Int_State = FEE_FLS_INT_STATE_IDLE;

    /* Notify Fee module */
    Fee_FlsJobEndNotification();
}

/**
 * @brief Handles job error
 */
static void Fee_Fls_Int_HandleJobError(void)
{
    /* Check if retry is needed */
    if (Fee_Fls_Int_JobControl.RetryCount < Fee_Fls_Int_Config.MaxRetries)
    {
        Fee_Fls_Int_JobControl.RetryCount++;
        /* Retry logic would be implemented here */
    }
    else
    {
        Fee_Fls_Int_JobControl.JobFailed = TRUE;
        Fee_Fls_Int_JobControl.JobPending = FALSE;
        Fee_Fls_Int_State = FEE_FLS_INT_STATE_ERROR;

        /* Notify Fee module */
        Fee_FlsJobErrorNotification();
    }
}

#define FEE_FLS_INT_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file RamTst.c
 * @brief RAM Test Implementation (Service Layer)
 * @req SHALL_RAMTST_MARCHC — March C / March C- algorithm
 *
 * March C:  (w0) ↑(r0,w1) ↑(r1,w0) ↓(r0,w1) ↓(r1,w0) ↑(r0)
 * March C-: (w0) ↑(r0,w1) ↑(r1,w0) ↓(r0,w1) ↓(r1,w0)
 *
 * Each MainFunction call processes RAMTST_WORDS_PER_CYCLE 32-bit words
 * to bound execution time.
 */

#include "RamTst.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define RAMTST_STATE_UNINIT                     (0x00U)
#define RAMTST_STATE_INIT                       (0x01U)
#define RAMTST_STATE_BUSY                       (0x02U)

/* March C step indices */
#define RAMTST_MARCH_STEP_W0                    (0U)    /* Write 0 (background)   */
#define RAMTST_MARCH_STEP_R0_W1                 (1U)    /* Read 0, Write 1        */
#define RAMTST_MARCH_STEP_R1_W0                 (2U)    /* Read 1, Write 0        */
#define RAMTST_MARCH_STEP_R0                    (3U)    /* Read 0 (final verify)  */

/* For March C- we skip the final R0 */
#define RAMTST_MARCH_STEP_COUNT_C               (4U)
#define RAMTST_MARCH_STEP_COUNT_C_MINUS         (3U)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    #define RAMTST_DET_REPORT_ERROR(api, err) \
        Det_ReportError(RAMTST_MODULE_ID, RAMTST_INSTANCE_ID, (api), (err))
#else
    #define RAMTST_DET_REPORT_ERROR(api, err)
#endif

#define RAMTST_IS_INIT() \
    ((RamTst_InternalState.State == RAMTST_STATE_INIT) || \
     (RamTst_InternalState.State == RAMTST_STATE_BUSY))

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/** Running state for a deferred March test */
typedef struct {
    boolean               Active;
    uint16                RegionId;
    const RamTst_RegionType* Region;
    RamTst_PhaseType      Phase;
    RamTst_AlgorithmType  Algorithm;
    uint32                CurrentOffset;      /* Word offset within region */
    uint8                 MarchStep;          /* Current March C step     */
    uint32                BackgroundPattern;  /* 32-bit background value  */
    boolean               MarchDirectionUp;
    RamTst_ResultType     Result;
    uint32                FailedAddress;
    uint32                ExpectedValue;
    uint32                ActualValue;
} RamTst_TestRunType;

/** Internal state */
typedef struct {
    uint8                State;
    const RamTst_ConfigType* ConfigPtr;
    RamTst_TestRunType   CurrentTest;
    RamTst_ResultType    LastResult;
} RamTst_InternalStateType;

/*==================================================================================================
 *                                    LOCAL DATA
 *==================================================================================================*/
static RamTst_InternalStateType RamTst_InternalState;

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static const RamTst_RegionType* RamTst_LocalFindRegion(uint16 RegionId);
static void RamTst_LocalRunMarchCStep(RamTst_TestRunType* test);
static uint8 RamTst_StepCount(void);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

static const RamTst_RegionType* RamTst_LocalFindRegion(uint16 RegionId)
{
    uint8 i;
    if (RamTst_InternalState.ConfigPtr == NULL_PTR)
    {
        return NULL_PTR;
    }
    for (i = 0U; i < RamTst_InternalState.ConfigPtr->NumRegions; i++)
    {
        if (RamTst_InternalState.ConfigPtr->Regions[i].RegionId == RegionId)
        {
            return &RamTst_InternalState.ConfigPtr->Regions[i];
        }
    }
    return NULL_PTR;
}

static uint8 RamTst_StepCount(void)
{
    if (RamTst_InternalState.CurrentTest.Algorithm == RAMTST_ALGO_MARCH_C_MINUS)
    {
        return RAMTST_MARCH_STEP_COUNT_C_MINUS;
    }
    return RAMTST_MARCH_STEP_COUNT_C;
}

/**
 * @brief Execute one step of March C/March C-.
 * Processes RAMTST_WORDS_PER_CYCLE 32-bit words per call.
 */
static void RamTst_LocalRunMarchCStep(RamTst_TestRunType* test)
{
    uint32 baseAddr;
    uint32 endAddr;
    uint32 processed;
    uint32 addr;
    uint32 expected;
    volatile uint32* ramPtr;
    uint32 wordsPerStep;

    if ((test->Region == NULL_PTR) || (!test->Active))
    {
        return;
    }

    baseAddr = test->Region->StartAddr + (test->CurrentOffset * 4U);
    endAddr  = test->Region->StartAddr + test->Region->Size;
    processed = 0U;
    wordsPerStep = RAMTST_WORDS_PER_CYCLE;

    while ((baseAddr + processed * 4U < endAddr) && (processed < wordsPerStep))
    {
        addr   = baseAddr + processed * 4U;
        ramPtr = (volatile uint32*)(uintptr)addr;

        switch (test->MarchStep)
        {
            case RAMTST_MARCH_STEP_W0:
                *ramPtr = test->BackgroundPattern;
                break;

            case RAMTST_MARCH_STEP_R0_W1:
                expected = test->BackgroundPattern;
                if (*ramPtr != expected)
                {
                    test->Result        = RAMTST_RESULT_FAILED;
                    test->FailedAddress = addr;
                    test->ExpectedValue = expected;
                    test->ActualValue   = *ramPtr;
                    test->Active        = FALSE;
                    return;
                }
                *ramPtr = (uint32)(~test->BackgroundPattern);
                break;

            case RAMTST_MARCH_STEP_R1_W0:
                expected = (uint32)(~test->BackgroundPattern);
                if (*ramPtr != expected)
                {
                    test->Result        = RAMTST_RESULT_FAILED;
                    test->FailedAddress = addr;
                    test->ExpectedValue = expected;
                    test->ActualValue   = *ramPtr;
                    test->Active        = FALSE;
                    return;
                }
                *ramPtr = test->BackgroundPattern;
                break;

            case RAMTST_MARCH_STEP_R0:
                expected = test->BackgroundPattern;
                if (*ramPtr != expected)
                {
                    test->Result        = RAMTST_RESULT_FAILED;
                    test->FailedAddress = addr;
                    test->ExpectedValue = expected;
                    test->ActualValue   = *ramPtr;
                    test->Active        = FALSE;
                    return;
                }
                break;

            default:
                break;
        }
        processed++;
    }

    test->CurrentOffset += processed;

    /* Check if current step is complete */
    {
        uint32 totalWords = test->Region->Size / 4U;
        if (test->CurrentOffset >= totalWords)
        {
            test->CurrentOffset = 0U;
            test->MarchStep++;

            /* Descending steps start from the end */
            if ((test->MarchStep == RAMTST_MARCH_STEP_R1_W0) && (!test->MarchDirectionUp))
            {
                test->CurrentOffset = totalWords - 1U;
            }
            if ((test->MarchStep == RAMTST_MARCH_STEP_R0) && (test->MarchDirectionUp))
            {
                test->CurrentOffset = totalWords - 1U;
            }

            /* All steps complete? */
            if (test->MarchStep >= RamTst_StepCount())
            {
                test->Phase   = RAMTST_PHASE_COMPLETE;
                test->Active  = FALSE;
                test->Result  = RAMTST_RESULT_PASSED;
            }
        }
    }

    if (test->Active)
    {
        test->Phase = (test->MarchDirectionUp) ? RAMTST_PHASE_MARCH_C_UP : RAMTST_PHASE_MARCH_C_DOWN;
    }
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

void RamTst_Init(const RamTst_ConfigType* ConfigPtr)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (RamTst_InternalState.State == RAMTST_STATE_INIT)
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_INIT, RAMTST_E_ALREADY_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_INIT, RAMTST_E_PARAM_POINTER);
        return;
    }
#endif

    RamTst_InternalState.ConfigPtr = ConfigPtr;
    RamTst_InternalState.State     = RAMTST_STATE_INIT;
    RamTst_InternalState.LastResult = RAMTST_RESULT_NOT_RUN;

    (void)memset(&RamTst_InternalState.CurrentTest, 0, sizeof(RamTst_TestRunType));

#if (RAMTST_RUN_ON_STARTUP == STD_ON)
    if (ConfigPtr->RunOnStartup)
    {
        if (ConfigPtr->NumRegions > 0U)
        {
            (void)RamTst_RunTest(ConfigPtr->Regions[0U].RegionId);
        }
    }
#endif
}

void RamTst_DeInit(void)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (!RAMTST_IS_INIT())
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_DEINIT, RAMTST_E_UNINIT);
        return;
    }
#endif

    if (RamTst_InternalState.CurrentTest.Active)
    {
        RamTst_InternalState.CurrentTest.Active = FALSE;
        RamTst_InternalState.CurrentTest.Result  = RAMTST_RESULT_ABORTED;
    }

    RamTst_InternalState.State     = RAMTST_STATE_UNINIT;
    RamTst_InternalState.ConfigPtr = NULL_PTR;
}

#if (RAMTST_VERSION_INFO_API == STD_ON)
void RamTst_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_GETVERSIONINFO, RAMTST_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID         = RAMTST_VENDOR_ID;
    versioninfo->moduleID         = RAMTST_MODULE_ID;
    versioninfo->sw_major_version = RAMTST_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = RAMTST_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = RAMTST_SW_PATCH_VERSION;
}
#endif

Std_ReturnType RamTst_RunTest(uint16 RegionId)
{
    const RamTst_RegionType* region;

#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (!RAMTST_IS_INIT())
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_RUNTEST, RAMTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (RamTst_InternalState.CurrentTest.Active)
    {
        return E_NOT_OK;  /* Busy */
    }

    region = RamTst_LocalFindRegion(RegionId);
    if (region == NULL_PTR)
    {
        return E_NOT_OK;
    }

    if (region->Size < 4U)
    {
        return E_NOT_OK;  /* Region too small for 32-bit test */
    }

    RamTst_InternalState.CurrentTest.Active          = TRUE;
    RamTst_InternalState.CurrentTest.RegionId        = RegionId;
    RamTst_InternalState.CurrentTest.Region          = region;
    RamTst_InternalState.CurrentTest.Phase           = RAMTST_PHASE_MARCH_C_UP;
    RamTst_InternalState.CurrentTest.Algorithm       = (RamTst_InternalState.ConfigPtr != NULL_PTR)
                                                         ? RamTst_InternalState.ConfigPtr->Algorithm
                                                         : RAMTST_ALGO_MARCH_C;
    RamTst_InternalState.CurrentTest.CurrentOffset   = 0U;
    RamTst_InternalState.CurrentTest.MarchStep       = RAMTST_MARCH_STEP_W0;
    RamTst_InternalState.CurrentTest.BackgroundPattern = RAMTST_MARCH_BACKGROUND;
    RamTst_InternalState.CurrentTest.MarchDirectionUp = TRUE;
    RamTst_InternalState.CurrentTest.Result          = RAMTST_RESULT_NOT_RUN;

    RamTst_InternalState.State = RAMTST_STATE_BUSY;

    return E_OK;
}

Std_ReturnType RamTst_GetResult(RamTst_ResultType* Result)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (Result == NULL_PTR)
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_GETRESULT, RAMTST_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Result = RamTst_InternalState.LastResult;
    return E_OK;
}

Std_ReturnType RamTst_Abort(void)
{
#if (RAMTST_DEV_ERROR_DETECT == STD_ON)
    if (!RAMTST_IS_INIT())
    {
        RAMTST_DET_REPORT_ERROR(RAMTST_SID_ABORT, RAMTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (!RamTst_InternalState.CurrentTest.Active)
    {
        return E_OK;
    }

    RamTst_InternalState.CurrentTest.Active = FALSE;
    RamTst_InternalState.CurrentTest.Result  = RAMTST_RESULT_ABORTED;
    RamTst_InternalState.CurrentTest.Phase  = RAMTST_PHASE_ABORTED;
    RamTst_InternalState.LastResult          = RAMTST_RESULT_ABORTED;
    RamTst_InternalState.State               = RAMTST_STATE_INIT;

    /* Call completion callback if registered */
    if ((RamTst_InternalState.ConfigPtr != NULL_PTR) &&
        (RamTst_InternalState.ConfigPtr->CompletionCb != NULL_PTR))
    {
        RamTst_InternalState.ConfigPtr->CompletionCb(RAMTST_RESULT_ABORTED);
    }

    return E_OK;
}

void RamTst_MainFunction(void)
{
    if (RamTst_InternalState.State != RAMTST_STATE_BUSY)
    {
        return;
    }

    if (!RamTst_InternalState.CurrentTest.Active)
    {
        RamTst_InternalState.State = RAMTST_STATE_INIT;
        RamTst_InternalState.LastResult = RamTst_InternalState.CurrentTest.Result;

        /* Call completion callback */
        if ((RamTst_InternalState.ConfigPtr != NULL_PTR) &&
            (RamTst_InternalState.ConfigPtr->CompletionCb != NULL_PTR))
        {
            RamTst_InternalState.ConfigPtr->CompletionCb(RamTst_InternalState.CurrentTest.Result);
        }
        return;
    }

    /* Execute one step */
    RamTst_LocalRunMarchCStep(&RamTst_InternalState.CurrentTest);

    /* Check for completion */
    if (!RamTst_InternalState.CurrentTest.Active)
    {
        RamTst_InternalState.State = RAMTST_STATE_INIT;
        RamTst_InternalState.LastResult = RamTst_InternalState.CurrentTest.Result;

        if ((RamTst_InternalState.ConfigPtr != NULL_PTR) &&
            (RamTst_InternalState.ConfigPtr->CompletionCb != NULL_PTR))
        {
            RamTst_InternalState.ConfigPtr->CompletionCb(RamTst_InternalState.CurrentTest.Result);
        }
    }
}

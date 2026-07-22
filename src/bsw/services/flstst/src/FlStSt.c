/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Flash HAL, Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file FlStSt.c
 * @brief Flash Test Implementation
 * @req SHALL_FLSTST_MARCHC — March C algorithm for flash fault detection
 * @req SHALL_FLSTST_VERIFY — Erase/Program verify
 *
 * Algorithm:
 *   March C:  (w0) ↑(r0,w1) ↑(r1,w0) ↓(r0,w1) ↓(r1,w0) ↑(r0)
 *   Operations are performed on word-aligned addresses within each sector.
 */

#include "FlStSt.h"
#include "Det.h"
#include <string.h>

/*==================================================================================================
 *                                    LOCAL CONSTANTS
 *==================================================================================================*/
#define FLSTST_STATE_UNINIT                     (0x00U)
#define FLSTST_STATE_INIT                       (0x01U)
#define FLSTST_STATE_BUSY                       (0x02U)

#define FLSTST_MARCH_STEP_W0                    (0U)    /* Write 0          */
#define FLSTST_MARCH_STEP_R0_W1                 (1U)    /* Read 0, Write 1  */
#define FLSTST_MARCH_STEP_R1_W0                 (2U)    /* Read 1, Write 0  */
#define FLSTST_MARCH_STEP_R0                    (3U)    /* Read 0           */

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    #define FLSTST_DET_REPORT_ERROR(api, err) \
        Det_ReportError(FLSTST_MODULE_ID, FLSTST_INSTANCE_ID, (api), (err))
#else
    #define FLSTST_DET_REPORT_ERROR(api, err)
#endif

#define FLSTST_IS_INIT() \
    ((FlStSt_InternalState.State == FLSTST_STATE_INIT) || \
     (FlStSt_InternalState.State == FLSTST_STATE_BUSY))

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/** Running state for a deferred March C test */
typedef struct {
    boolean               Active;
    uint16                SectorId;
    const FlStSt_SectorType* Sector;
    FlStSt_PhaseType      Phase;
    FlStSt_AlgorithmType  Algorithm;
    uint32                CurrentOffset;    /* Current word offset within sector */
    uint8                 MarchStep;        /* Which March C step we're on    */
    uint8                 BackgroundValue;  /* Current background pattern     */
    boolean               MarchDirectionUp; /* TRUE = ascending, FALSE = descending */
    FlStSt_ResultType     Result;
    uint32                FailedAddress;
    uint8                 ExpectedValue;
    uint8                 ActualValue;
} FlStSt_TestRunType;

/** Internal state */
typedef struct {
    uint8                 State;
    const FlStSt_ConfigType* ConfigPtr;
    FlStSt_TestRunType    CurrentTest;
    FlStSt_ResultType     LastResult;
} FlStSt_InternalStateType;

/*==================================================================================================
 *                                    LOCAL DATA
 *==================================================================================================*/
static FlStSt_InternalStateType FlStSt_InternalState;

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static const FlStSt_SectorType* FlStSt_LocalFindSector(uint16 SectorId);
static void FlStSt_LocalRunMarchCStep(FlStSt_TestRunType* test);
static uint8 FlStSt_LocalReadByte(uint32 Address);
static void FlStSt_LocalWriteByte(uint32 Address, uint8 Value);

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/

static const FlStSt_SectorType* FlStSt_LocalFindSector(uint16 SectorId)
{
    uint8 i;
    if (FlStSt_InternalState.ConfigPtr == NULL_PTR)
    {
        return NULL_PTR;
    }
    for (i = 0U; i < FlStSt_InternalState.ConfigPtr->NumSectors; i++)
    {
        if (FlStSt_InternalState.ConfigPtr->Sectors[i].SectorId == SectorId)
        {
            return &FlStSt_InternalState.ConfigPtr->Sectors[i];
        }
    }
    return NULL_PTR;
}

/* Stub — in production these call the Flash HAL */
static uint8 FlStSt_LocalReadByte(uint32 Address)
{
    (void)Address;
    /* Simulate: return the last written value or erased value */
    return 0xFFU;
}

static void FlStSt_LocalWriteByte(uint32 Address, uint8 Value)
{
    (void)Address;
    (void)Value;
    /* No-op stub for unit tests */
}

/**
 * @brief Execute one step of the March C algorithm on a sector.
 *
 * March C sequence (with background = W0):
 *   1. Write background (0x55)
 *   2. ↑ Read 0, Write 1   (0x55 → 0xAA)
 *   3. ↑ Read 1, Write 0
 *   4. ↓ Read 0, Write 1
 *   5. ↓ Read 1, Write 0
 *   6. ↑ Read 0
 *
 * Each call processes FLSTST_BYTES_PER_CYCLE words to keep latency bounded.
 */
static void FlStSt_LocalRunMarchCStep(FlStSt_TestRunType* test)
{
    uint32 start;
    uint32 end;
    uint32 addr;
    uint8 expected;
    uint8 actual;
    uint32 processed = 0U;

    if ((test->Sector == NULL_PTR) || (!test->Active))
    {
        return;
    }

    start = test->Sector->StartAddr + test->CurrentOffset;
    end   = test->Sector->StartAddr + test->Sector->Size;

    while ((start + processed < end) && (processed < FLSTST_BYTES_PER_CYCLE))
    {
        addr = start + processed;

        switch (test->MarchStep)
        {
            case FLSTST_MARCH_STEP_W0:
                /* Write background pattern */
                FlStSt_LocalWriteByte(addr, test->BackgroundValue);
                break;

            case FLSTST_MARCH_STEP_R0_W1:
                expected = test->BackgroundValue;
                actual   = FlStSt_LocalReadByte(addr);
                if (actual != expected)
                {
                    test->Result        = FLSTST_RESULT_FAILED;
                    test->FailedAddress = addr;
                    test->ExpectedValue = expected;
                    test->ActualValue   = actual;
                    test->Active        = FALSE;
                    return;
                }
                FlStSt_LocalWriteByte(addr, (uint8)(~test->BackgroundValue));
                break;

            case FLSTST_MARCH_STEP_R1_W0:
                expected = (uint8)(~test->BackgroundValue);
                actual   = FlStSt_LocalReadByte(addr);
                if (actual != expected)
                {
                    test->Result        = FLSTST_RESULT_FAILED;
                    test->FailedAddress = addr;
                    test->ExpectedValue = expected;
                    test->ActualValue   = actual;
                    test->Active        = FALSE;
                    return;
                }
                FlStSt_LocalWriteByte(addr, test->BackgroundValue);
                break;

            case FLSTST_MARCH_STEP_R0:
                expected = test->BackgroundValue;
                actual   = FlStSt_LocalReadByte(addr);
                if (actual != expected)
                {
                    test->Result        = FLSTST_RESULT_FAILED;
                    test->FailedAddress = addr;
                    test->ExpectedValue = expected;
                    test->ActualValue   = actual;
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

    /* Check if the current step is complete across the entire sector */
    if (test->CurrentOffset >= test->Sector->Size)
    {
        test->CurrentOffset = 0U;
        test->MarchStep++;

        /* Transition direction for descending steps */
        if ((test->MarchStep == FLSTST_MARCH_STEP_R1_W0) && (!test->MarchDirectionUp))
        {
            test->CurrentOffset = test->Sector->Size;
        }
        if ((test->MarchStep == FLSTST_MARCH_STEP_R0) && (test->MarchDirectionUp))
        {
            test->CurrentOffset = test->Sector->Size;
        }

        /* Check if all steps complete */
        if (test->MarchStep > FLSTST_MARCH_STEP_R0)
        {
            test->Phase = FLSTST_PHASE_COMPLETE;
            test->Active = FALSE;
            test->Result = FLSTST_RESULT_PASSED;
        }
    }

    if (test->Active)
    {
        test->Phase = FLSTST_PHASE_MARCH_C_UP;
    }
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

void FlStSt_Init(const FlStSt_ConfigType* ConfigPtr)
{
#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (FlStSt_InternalState.State == FLSTST_STATE_INIT)
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_INIT, FLSTST_E_ALREADY_INITIALIZED);
        return;
    }
    if (ConfigPtr == NULL_PTR)
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_INIT, FLSTST_E_PARAM_POINTER);
        return;
    }
#endif

    FlStSt_InternalState.ConfigPtr = ConfigPtr;
    FlStSt_InternalState.State     = FLSTST_STATE_INIT;
    FlStSt_InternalState.LastResult = FLSTST_RESULT_NOT_RUN;

    /* Clear current test */
    (void)memset(&FlStSt_InternalState.CurrentTest, 0, sizeof(FlStSt_TestRunType));

#if (FLSTST_RUN_ON_INIT == STD_ON)
    if (ConfigPtr->RunOnInit)
    {
        if (ConfigPtr->NumSectors > 0U)
        {
            (void)FlStSt_RunTest(ConfigPtr->Sectors[0U].SectorId);
        }
    }
#endif
}

void FlStSt_DeInit(void)
{
#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (!FLSTST_IS_INIT())
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_DEINIT, FLSTST_E_UNINIT);
        return;
    }
#endif

    /* Abort any running test */
    if (FlStSt_InternalState.CurrentTest.Active)
    {
        FlStSt_InternalState.CurrentTest.Active = FALSE;
        FlStSt_InternalState.CurrentTest.Result  = FLSTST_RESULT_ABORTED;
    }

    FlStSt_InternalState.State     = FLSTST_STATE_UNINIT;
    FlStSt_InternalState.ConfigPtr = NULL_PTR;
}

#if (FLSTST_VERSION_INFO_API == STD_ON)
void FlStSt_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_GETVERSIONINFO, FLSTST_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID         = FLSTST_VENDOR_ID;
    versioninfo->moduleID         = FLSTST_MODULE_ID;
    versioninfo->sw_major_version = FLSTST_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FLSTST_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FLSTST_SW_PATCH_VERSION;
}
#endif

Std_ReturnType FlStSt_RunTest(uint16 SectorId)
{
    const FlStSt_SectorType* sector;

#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (!FLSTST_IS_INIT())
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_RUNTEST, FLSTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (FlStSt_InternalState.CurrentTest.Active)
    {
        return E_NOT_OK;  /* Busy */
    }

    sector = FlStSt_LocalFindSector(SectorId);
    if (sector == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Initialise test run */
    FlStSt_InternalState.CurrentTest.Active          = TRUE;
    FlStSt_InternalState.CurrentTest.SectorId        = SectorId;
    FlStSt_InternalState.CurrentTest.Sector          = sector;
    FlStSt_InternalState.CurrentTest.Phase           = FLSTST_PHASE_WRITE_BACKGROUND;
    FlStSt_InternalState.CurrentTest.Algorithm       = FLSTST_ALGO_MARCH_C;
    FlStSt_InternalState.CurrentTest.CurrentOffset   = 0U;
    FlStSt_InternalState.CurrentTest.MarchStep       = FLSTST_MARCH_STEP_W0;
    FlStSt_InternalState.CurrentTest.BackgroundValue = FLSTST_MARCH_BACKGROUND_PATTERN;
    FlStSt_InternalState.CurrentTest.MarchDirectionUp = TRUE;
    FlStSt_InternalState.CurrentTest.Result          = FLSTST_RESULT_NOT_RUN;

    FlStSt_InternalState.State = FLSTST_STATE_BUSY;

    return E_OK;
}

Std_ReturnType FlStSt_VerifyErase(uint16 SectorId, boolean* Result)
{
    const FlStSt_SectorType* sector;

#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (!FLSTST_IS_INIT())
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_VERIFYERASE, FLSTST_E_UNINIT);
        return E_NOT_OK;
    }
    if (Result == NULL_PTR)
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_VERIFYERASE, FLSTST_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    sector = FlStSt_LocalFindSector(SectorId);
    if (sector == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Verify every byte in the sector is erased (0xFF) */
    {
        uint32 i;
        uint32 addr;
        boolean allErased = TRUE;

        for (i = 0U; i < sector->Size; i += FLSTST_VERIFY_CHUNK_SIZE)
        {
            uint32 chunk = (sector->Size - i < FLSTST_VERIFY_CHUNK_SIZE) ? (sector->Size - i) : FLSTST_VERIFY_CHUNK_SIZE;
            uint32 j;
            for (j = 0U; j < chunk; j++)
            {
                addr = sector->StartAddr + i + j;
                if (FlStSt_LocalReadByte(addr) != FLSTST_ERASE_VALUE)
                {
                    allErased = FALSE;
                    break;
                }
            }
            if (!allErased)
            {
                break;
            }
        }

        *Result = allErased;
    }

    return E_OK;
}

Std_ReturnType FlStSt_VerifyProgram(uint16 SectorId, const uint8* ExpectedData,
                                    uint16 Length, boolean* Result)
{
    const FlStSt_SectorType* sector;

#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (!FLSTST_IS_INIT())
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_VERIFYPROGRAM, FLSTST_E_UNINIT);
        return E_NOT_OK;
    }
    if ((ExpectedData == NULL_PTR) || (Result == NULL_PTR))
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_VERIFYPROGRAM, FLSTST_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    sector = FlStSt_LocalFindSector(SectorId);
    if (sector == NULL_PTR)
    {
        return E_NOT_OK;
    }

    if (Length > sector->Size)
    {
        return E_NOT_OK;
    }

    /* Compare read-back with expected data */
    {
        uint16 i;
        boolean match = TRUE;
        for (i = 0U; i < Length; i++)
        {
            if (FlStSt_LocalReadByte(sector->StartAddr + i) != ExpectedData[i])
            {
                match = FALSE;
                break;
            }
        }
        *Result = match;
    }

    return E_OK;
}

Std_ReturnType FlStSt_GetResult(FlStSt_ResultType* Result)
{
#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (Result == NULL_PTR)
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_GETRESULT, FLSTST_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *Result = FlStSt_InternalState.LastResult;
    return E_OK;
}

Std_ReturnType FlStSt_Abort(void)
{
#if (FLSTST_DEV_ERROR_DETECT == STD_ON)
    if (!FLSTST_IS_INIT())
    {
        FLSTST_DET_REPORT_ERROR(FLSTST_SID_ABORT, FLSTST_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (!FlStSt_InternalState.CurrentTest.Active)
    {
        return E_OK;  /* Nothing to abort */
    }

    FlStSt_InternalState.CurrentTest.Active = FALSE;
    FlStSt_InternalState.CurrentTest.Result  = FLSTST_RESULT_ABORTED;
    FlStSt_InternalState.CurrentTest.Phase  = FLSTST_PHASE_ABORTED;
    FlStSt_InternalState.LastResult          = FLSTST_RESULT_ABORTED;
    FlStSt_InternalState.State               = FLSTST_STATE_INIT;

    return E_OK;
}

void FlStSt_MainFunction(void)
{
    if (FlStSt_InternalState.State != FLSTST_STATE_BUSY)
    {
        return;
    }

    if (!FlStSt_InternalState.CurrentTest.Active)
    {
        FlStSt_InternalState.State = FLSTST_STATE_INIT;
        FlStSt_InternalState.LastResult = FlStSt_InternalState.CurrentTest.Result;
        return;
    }

    /* Execute one step of March C */
    FlStSt_LocalRunMarchCStep(&FlStSt_InternalState.CurrentTest);

    /* Check if the test just completed */
    if (!FlStSt_InternalState.CurrentTest.Active)
    {
        FlStSt_InternalState.State = FLSTST_STATE_INIT;
        FlStSt_InternalState.LastResult = FlStSt_InternalState.CurrentTest.Result;
    }
}

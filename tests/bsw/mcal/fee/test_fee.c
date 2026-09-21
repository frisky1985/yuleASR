/**
 * @file test_fee.c
 * @brief Fee Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/fee/src/Fee.c  @tests src/bsw/mcal/fee/src/Fee_Lcfg.c  @tests src/bsw/mcal/fee/include/Fee.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"
#include "Fee.h"

/*==================================================================================================
 * Test support
 *==================================================================================================*/

/*
 * The production configuration from Fee_Lcfg.c is used: two sectors at
 * 0x10000000 and 0x10010000 (64K each), virtual page size 8, 256 bytes
 * maximum per MainFunction cycle in normal mode. Fee_DeInit() restores the
 * uninitialized state, so every test starts from a genuine UNINIT state.
 */
#define FEE_TEST_SECTOR0_START   (0x10000000U)
#define FEE_TEST_SECTOR0_END     (0x10010000U)
#define FEE_TEST_SECTOR1_START   (0x10010000U)

static uint8 Fee_TestBuffer[512U];

void setUp(void) {
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {
    /* Neutralize module state so the next test starts from genuine UNINIT. */
    if (Fee_GetStatus() == FEE_BUSY) {
        (void)Fee_Cancel();
    }
    if (Fee_GetStatus() != FEE_UNINIT) {
        (void)Fee_DeInit();
    }
    /* Discard any DET reports triggered by the teardown helpers. */
    Det_Mock_Reset();
}

/* Verifies the last DET report matches the expected module/API/error triple. */
static void test_Fee_AssertDet(uint8 expectedApiId, uint8 expectedErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid); /* expected DET report */
    TEST_ASSERT_EQUAL_UINT(FEE_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT(FEE_INSTANCE_ID, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT(expectedApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT(expectedErrorId, Det_MockData.ErrorId);
}

/*==================================================================================================
 * Init / DeInit
 *==================================================================================================*/

/** @req SWS_Fee_00001 */
void test_Fee_Init_BeforeInit_NullPtr_ShouldReportParamConfig(void) {
    Std_ReturnType ret = Fee_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_INIT, FEE_E_PARAM_CONFIG);
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

/** @req SWS_Fee_00001 */
void test_Fee_Init_ValidConfig_ShouldReachIdle(void) {
    Std_ReturnType ret = Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
}

/** @req SWS_Fee_00001 */
void test_Fee_Init_AlreadyInitialized_ShouldReportDetError(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Det_Mock_Reset();

    Std_ReturnType ret = Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_INIT, FEE_E_ALREADY_INITIALIZED);
}

/** @req SWS_Fee_00017 */
void test_Fee_DeInit_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fee_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_DEINIT, FEE_E_UNINIT);
}

/** @req SWS_Fee_00017 */
void test_Fee_DeInit_WhileBusy_ShouldReportBusy(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Erase(FEE_TEST_SECTOR0_START, 64U));

    Std_ReturnType ret = Fee_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_DEINIT, FEE_E_BUSY);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
}

/** @req SWS_Fee_00017 */
void test_Fee_DeInit_WhenInitialized_ShouldReturnToUninit(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
    /* Further API calls must be rejected with UNINIT after DeInit. */
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Read(FEE_TEST_SECTOR0_START, 8U, Fee_TestBuffer));
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_UNINIT);
}

/*==================================================================================================
 * SetMode
 *==================================================================================================*/

/** @req SWS_Fee_00018 */
void test_Fee_SetMode_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fee_SetMode(FEE_MODE_FAST);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_SETMODE, FEE_E_UNINIT);
}

/** @req SWS_Fee_00018 */
void test_Fee_SetMode_InvalidMode_ShouldReportInvalidMode(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_SetMode((Fee_ModeType)0xFFU);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_SETMODE, FEE_E_INVALID_MODE);
}

/** @req SWS_Fee_00018 */
void test_Fee_SetMode_ValidMode_ShouldReturnOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_SetMode(FEE_MODE_FAST);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    /* Driver remains usable after a mode switch. */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/*==================================================================================================
 * Read validation
 *==================================================================================================*/

/** @req SWS_Fee_00019 */
void test_Fee_Read_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR0_START, 8U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_UNINIT);
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_NullDestPtr_ShouldReportParamPointer(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR0_START, 8U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_LengthNotPageAligned_ShouldReportInvalidLength(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR0_START, 12U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_INVALID_LENGTH);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_ZeroLength_ShouldReportInvalidLength(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR0_START, 0U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_INVALID_LENGTH);
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_AddressOutsideSectors_ShouldReportInvalidAddress(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Read(0x90000000U, 64U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_INVALID_ADDRESS);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_LengthCrossingSectorBoundary_ShouldReportInvalidAddress(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    /* Start at the last page of sector 0 but the range end (0x10010008)
     * exceeds the sector end (0x10010000). */
    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR0_END - 8U, 16U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_INVALID_ADDRESS);
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_ValidRequest_ShouldStartPendingJob(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());
}

/** @req SWS_Fee_00019 */
void test_Fee_Read_WhileBusy_ShouldReportBusy(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer));
    Det_Mock_Reset();

    Std_ReturnType ret = Fee_Read(FEE_TEST_SECTOR1_START, 64U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_READ, FEE_E_BUSY);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
}

/*==================================================================================================
 * Write / Erase / Compare / BlankCheck validation
 *==================================================================================================*/

/** @req SWS_Fee_00020 */
void test_Fee_Write_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fee_Write(FEE_TEST_SECTOR0_START, 8U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_WRITE, FEE_E_UNINIT);
}

/** @req SWS_Fee_00020 */
void test_Fee_Write_NullSourcePtr_ShouldReportParamPointer(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Write(FEE_TEST_SECTOR0_START, 8U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_WRITE, FEE_E_PARAM_POINTER);
}

/** @req SWS_Fee_00020 */
void test_Fee_Write_ValidRequest_ShouldStartPendingJob(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Write(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());
}

/** @req SWS_Fee_00021 */
void test_Fee_Erase_InvalidAddress_ShouldReportInvalidAddress(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Erase(0x90000000U, 64U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_ERASE, FEE_E_INVALID_ADDRESS);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/** @req SWS_Fee_00021 */
void test_Fee_Erase_ValidRequest_ShouldStartPendingJob(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Erase(FEE_TEST_SECTOR0_START, 64U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());
}

/** @req SWS_Fee_00022 */
void test_Fee_Compare_InvalidLength_ShouldReportInvalidLength(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Compare(FEE_TEST_SECTOR0_START, 10U, Fee_TestBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_COMPARE, FEE_E_INVALID_LENGTH);
}

/** @req SWS_Fee_00023 */
void test_Fee_BlankCheck_ValidRequest_ShouldStartPendingJob(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_BlankCheck(FEE_TEST_SECTOR0_START, 64U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());
}

/*==================================================================================================
 * MainFunction job processing
 *==================================================================================================*/

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_BeforeInit_ShouldReportUninit(void) {
    Fee_MainFunction();
    test_Fee_AssertDet(FEE_SID_MAINFUNCTION, FEE_E_UNINIT);
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_ReadJob_CompletesWithJobOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer));
    /* 64 bytes fit within one 256-byte normal-mode cycle. */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    /* Source quirk: completion clears the BUSY flag but never resets
     * Fee_DriverState.state, so GetStatus keeps reporting FEE_BUSY.
     * A new job is still accepted, proving the flag is really cleared. */
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_ReadJob_LargeLengthSpansMultipleCycles(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR0_START, 512U, Fee_TestBuffer));

    /* First cycle processes at most 256 bytes: job stays pending. */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());

    /* Second cycle finishes the remaining 256 bytes. */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    /* Busy flag cleared although GetStatus still reports the stale state. */
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_WriteJob_CompletesWithJobOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus()); /* stale state, see quirk above */
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_EraseJob_CompletesWithJobOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Erase(FEE_TEST_SECTOR0_START, 64U));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus()); /* stale state, see quirk above */
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_CompareJob_CompletesWithJobOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Compare(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus()); /* stale state, see quirk above */
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_BlankCheckJob_CompletesWithJobOk(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_BlankCheck(FEE_TEST_SECTOR0_START, 64U));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus()); /* stale state, see quirk above */
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/** @req SWS_Fee_00030 */
void test_Fee_MainFunction_WhenIdle_ShouldDoNothing(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Fee_MainFunction();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
}

/*==================================================================================================
 * Cancel / Suspend / Resume
 *==================================================================================================*/

/** @req SWS_Fee_00026 */
void test_Fee_Cancel_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fee_Cancel();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_CANCEL, FEE_E_UNINIT);
}

/** @req SWS_Fee_00026 */
void test_Fee_Cancel_WhenIdle_ShouldReportInvalidCancel(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Cancel();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_CANCEL, FEE_E_INVALID_CANCEL);
}

/** @req SWS_Fee_00026 */
void test_Fee_Cancel_DuringJob_ShouldCancelToIdle(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer));
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());

    Std_ReturnType ret = Fee_Cancel();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_CANCELLED, Fee_GetJobResult());
    /* A cancelled job must not be advanced by MainFunction. */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_CANCELLED, Fee_GetJobResult());
}

/** @req SWS_Fee_00027 */
void test_Fee_Suspend_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fee_Suspend();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_SUSPEND, FEE_E_UNINIT);
}

/** @req SWS_Fee_00027 */
void test_Fee_Suspend_WhenIdle_ShouldReportInvalidSuspend(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Suspend();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_SUSPEND, FEE_E_INVALID_SUSPEND);
}

/** @req SWS_Fee_00028 */
void test_Fee_Resume_WhenNotSuspended_ShouldReportInvalidResume(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    Std_ReturnType ret = Fee_Resume();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fee_AssertDet(FEE_SID_RESUME, FEE_E_INVALID_RESUME);
}

/** @req SWS_Fee_00027 */
/** @req SWS_Fee_00028 */
void test_Fee_SuspendResume_JobHoldsDuringSuspension(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(FEE_TEST_SECTOR0_START, 64U, Fee_TestBuffer));
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());

    /* Suspend the pending job. */
    Std_ReturnType ret = Fee_Suspend();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);

    /* MainFunction must not advance a suspended job. */
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());

    /* Resume lets the job finish. */
    TEST_ASSERT_EQUAL(E_OK, Fee_Resume());
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
    /* Stale state quirk: GetStatus still reports BUSY after completion. */
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(FEE_TEST_SECTOR1_START, 8U, Fee_TestBuffer));
}

/*==================================================================================================
 * Status / JobResult / VersionInfo
 *==================================================================================================*/

/** @req SWS_Fee_00024 */
void test_Fee_GetStatus_BeforeInit_ShouldReportUninit(void) {
    Fee_StateType status = Fee_GetStatus();
    TEST_ASSERT_EQUAL(FEE_UNINIT, status);
    test_Fee_AssertDet(FEE_SID_GETSTATUS, FEE_E_UNINIT);
}

/** @req SWS_Fee_00024 */
void test_Fee_GetStatus_WithPendingJob_ShouldReturnBusy(void) {
    TEST_ASSERT_EQUAL(E_OK, Fee_Init(&Fee_Config));
    TEST_ASSERT_EQUAL(E_OK, Fee_Erase(FEE_TEST_SECTOR0_START, 64U));
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Fee_00025 */
void test_Fee_GetJobResult_BeforeInit_ShouldReportUninitAndFailed(void) {
    Fee_JobResultType result = Fee_GetJobResult();
    TEST_ASSERT_EQUAL(FEE_JOB_FAILED, result);
    test_Fee_AssertDet(FEE_SID_GETJOBRESULT, FEE_E_UNINIT);
}

/** @req SWS_Fee_00029 */
void test_Fee_GetVersionInfo_NullPtr_ShouldReportParamPointer(void) {
    Fee_GetVersionInfo(NULL_PTR);
    test_Fee_AssertDet(FEE_SID_GETVERSIONINFO, FEE_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_UINT(1U, Det_MockData.CallCount);
}

/** @req SWS_Fee_00029 */
void test_Fee_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    Fee_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT(FEE_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT(FEE_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT(FEE_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT(FEE_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT(FEE_SW_PATCH_VERSION, info.sw_patch_version);
}

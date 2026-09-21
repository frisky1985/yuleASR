/**
 * @file test_eep.c
 * @brief Eep Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/eep/src/Eep.c  @tests src/bsw/mcal/eep/include/Eep.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"
#include "Eep.h"

/*==================================================================================================
 * Test support
 *==================================================================================================*/

/*
 * Host-safe configuration:
 *  - PollingMode = FALSE keeps Read/Write/Erase asynchronous (the job stays
 *    EEP_BUSY instead of touching the backing store through the raw
 *    BaseAddress pointer, which cannot be dereferenced on the host).
 *  - Eep_DeInit() fully restores the uninitialized state, so every test can
 *    run from a genuine UNINIT state without a one-shot-init restriction.
 */
static const Eep_ConfigType Eep_TestConfig = {
    0x00A00000U, /* BaseAddress       */
    4096U,       /* Size              */
    10U,         /* JobCallCycle      */
    8U,          /* PageSize          */
    5U,          /* WriteCycleTimeMs  */
    10U,         /* EraseCycleTimeMs  */
    FALSE        /* PollingMode       */
};

static uint8 Eep_TestBuffer[64U];

void setUp(void) {
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {
    /* Restore genuine uninitialized state for the next test. */
    Eep_DeInit();
}

/* Verifies the last DET report matches the expected module/API/error triple. */
static void test_Eep_AssertDet(uint8 expectedApiId, uint8 expectedErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid); /* expected DET report */
    TEST_ASSERT_EQUAL_UINT(EEP_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT(expectedApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT(expectedErrorId, Det_MockData.ErrorId);
}

/*==================================================================================================
 * Init / DeInit
 *==================================================================================================*/

/** @req SWS_Eep_00001 */
void test_Eep_Init_NullPtr_ShouldReportDetError(void) {
    Eep_Init(NULL_PTR);
    test_Eep_AssertDet(EEP_SID_INIT, EEP_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_UINT(1U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

/** @req SWS_Eep_00001 */
void test_Eep_Init_ValidConfig_ShouldReachIdle(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_OK, Eep_GetJobResult());
}

/** @req SWS_Eep_00001 */
void test_Eep_Init_RepeatedInit_ShouldReinitializeSilently(void) {
    /* Implementation has no double-init protection: a second Init resets the
     * module silently (documented actual behavior). */
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(E_OK, Eep_Write(0U, Eep_TestBuffer, 8U));
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());

    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_OK, Eep_GetJobResult());
}

/** @req SWS_Eep_00002 */
void test_Eep_DeInit_WhenInitialized_ShouldReturnToUninit(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(E_OK, Eep_Write(0U, Eep_TestBuffer, 8U));

    Eep_DeInit();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_OK, Eep_GetJobResult());
}

/*==================================================================================================
 * Erase
 *==================================================================================================*/

/** @req SWS_Eep_00002 */
void test_Eep_Erase_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Eep_Erase(0U, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_ERASE, EEP_E_UNINIT);
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_InvalidAddress_ShouldReportParamAddress(void) {
    Eep_Init(&Eep_TestConfig);
    /* 0xFFFF is beyond the configured Size of 4096 bytes. */
    Std_ReturnType ret = Eep_Erase(0xFFFFU, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_ERASE, EEP_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_LengthBeyondSize_ShouldReportParamAddress(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Erase(4090U, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_ERASE, EEP_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_ValidRegion_ShouldStartPendingJob(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Erase(0U, 64U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_PENDING, Eep_GetJobResult());
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_WhileBusy_ShouldReturnNotOkSilently(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(E_OK, Eep_Erase(0U, 64U));
    /* Busy rejection path reports no DET error. */
    Std_ReturnType ret = Eep_Erase(64U, 64U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
}

/*==================================================================================================
 * Write
 *==================================================================================================*/

/** @req SWS_Eep_00003 */
void test_Eep_Write_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Eep_Write(0U, Eep_TestBuffer, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_WRITE, EEP_E_UNINIT);
}

/** @req SWS_Eep_00003 */
void test_Eep_Write_InvalidAddress_ShouldReportParamAddress(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Write(0xFFFFU, Eep_TestBuffer, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_WRITE, EEP_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00003 */
void test_Eep_Write_ValidData_ShouldStartPendingJob(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Write(64U, Eep_TestBuffer, 16U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_PENDING, Eep_GetJobResult());
}

/** @req SWS_Eep_00005 */
void test_Eep_Write_NullDataPtr_ShouldReportParamPointer(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Write(0U, NULL_PTR, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_WRITE, EEP_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00005 */
void test_Eep_Write_ZeroLength_ShouldReportParamLength(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Write(0U, Eep_TestBuffer, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_WRITE, EEP_E_PARAM_LENGTH);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00003 */
void test_Eep_Write_WhileBusy_ShouldReturnNotOkSilently(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(E_OK, Eep_Write(0U, Eep_TestBuffer, 8U));
    Std_ReturnType ret = Eep_Write(16U, Eep_TestBuffer, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
}

/*==================================================================================================
 * Read
 *==================================================================================================*/

/** @req SWS_Eep_00004 */
void test_Eep_Read_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Eep_Read(0U, Eep_TestBuffer, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_READ, EEP_E_UNINIT);
}

/** @req SWS_Eep_00004 */
void test_Eep_Read_InvalidAddress_ShouldReportParamAddress(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Read(0xFFFFU, Eep_TestBuffer, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_READ, EEP_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00004 */
void test_Eep_Read_ValidBuffer_ShouldStartPendingJob(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Read(128U, Eep_TestBuffer, 32U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_PENDING, Eep_GetJobResult());
}

/** @req SWS_Eep_00005 */
void test_Eep_Read_NullDataPtr_ShouldReportParamPointer(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Read(0U, NULL_PTR, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_READ, EEP_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00005 */
void test_Eep_Read_ZeroLength_ShouldReportParamLength(void) {
    Eep_Init(&Eep_TestConfig);
    Std_ReturnType ret = Eep_Read(0U, Eep_TestBuffer, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Eep_AssertDet(EEP_SID_READ, EEP_E_PARAM_LENGTH);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/** @req SWS_Eep_00004 */
void test_Eep_Read_WhileBusy_ShouldReturnNotOkSilently(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(E_OK, Eep_Read(0U, Eep_TestBuffer, 8U));
    Std_ReturnType ret = Eep_Read(16U, Eep_TestBuffer, 8U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
}

/*==================================================================================================
 * Cancel
 *==================================================================================================*/

/** @req SWS_Eep_00006 */
void test_Eep_Cancel_BeforeInit_ShouldBeSilent(void) {
    /* Cancel does not report DET in any state (documented actual behavior). */
    Eep_Cancel();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

/** @req SWS_Eep_00006 */
void test_Eep_Cancel_DuringJob_ShouldCancelToIdle(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(E_OK, Eep_Write(0U, Eep_TestBuffer, 8U));
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());

    Eep_Cancel();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_CANCELED, Eep_GetJobResult());
}

/*==================================================================================================
 * Status / JobResult / VersionInfo
 *==================================================================================================*/

/** @req SWS_Eep_00007 */
void test_Eep_GetStatus_BeforeInit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Eep_00007 */
void test_Eep_GetStatus_AfterInitAndJob_ShouldTrackState(void) {
    Eep_Init(&Eep_TestConfig);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQUAL(E_OK, Eep_Read(0U, Eep_TestBuffer, 8U));
    TEST_ASSERT_EQUAL(EEP_BUSY, Eep_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Eep_00008 */
void test_Eep_GetVersionInfo_NullPtr_ShouldReportDetError(void) {
    Eep_GetVersionInfo(NULL_PTR);
    test_Eep_AssertDet(EEP_SID_GET_VERSION_INFO, EEP_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_UINT(1U, Det_MockData.CallCount);
}

/** @req SWS_Eep_00008 */
void test_Eep_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    Eep_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT(EEP_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT(EEP_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT(EEP_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT(EEP_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT(EEP_SW_PATCH_VERSION, info.sw_patch_version);
}

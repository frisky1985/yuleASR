/**
 * @file test_fls.c
 * @brief Fls Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/fls/src/Fls.c  @tests src/bsw/mcal/fls/include/Fls.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"
#include "Fls.h"

/*==================================================================================================
 * Test support
 *==================================================================================================*/

/*
 * Fls has no DeInit API, but Fls_ConfigPtr and Fls_Status are mutable extern
 * globals, so the genuine UNINIT state is restored before every test. A
 * successful Fls_Init() then re-initializes all internal statics (job control,
 * mode, timeout counter).
 *
 * Fls_WritePage/Fls_ReadData route through REG_WRITE8/REG_READ8, which the
 * mock register table backs, so write/read-back and compare scenarios are
 * verifiable on the host (footprint kept below the 256-entry table limit).
 */
#define FLS_TEST_SECTOR0_START   (0x08000000U)
#define FLS_TEST_SECTOR0_SIZE    (65536U)
#define FLS_TEST_SECTOR1_START   (0x08010000U)
#define FLS_TEST_SECTOR1_SIZE    (65536U)

static const Fls_SectorType Fls_TestSectors[2] = {
    { FLS_TEST_SECTOR0_START, FLS_TEST_SECTOR0_SIZE, 4U, 0U, TRUE, TRUE },
    { FLS_TEST_SECTOR1_START, FLS_TEST_SECTOR1_SIZE, 4U, 0U, TRUE, TRUE }
};

static const Fls_ConfigType Fls_TestConfig = {
    Fls_TestSectors,           /* sectorList                  */
    2U,                        /* sectorCount                 */
    0U,                        /* defaultMode (slow/normal)   */
    512U,                      /* maxReadFastMode             */
    256U,                      /* maxReadNormalMode           */
    64U,                       /* maxWriteFastMode            */
    32U,                       /* maxWriteNormalMode          */
    TRUE,                      /* jobEndNotificationEnabled   */
    TRUE                       /* jobErrorNotificationEnabled */
};

static uint8 Fls_TestBuffer[64U];
static uint8 Fls_TestBigBuffer[32001U];

/* Notification stubs: Fls.c requires these symbols (FLS_JOB_*_NOTIFICATION
 * are STD_ON) and they let tests observe job completion/failure. */
static uint8 Fls_EndNotificationCount = 0U;
static uint8 Fls_ErrorNotificationCount = 0U;

void Fls_JobEndNotification(void)
{
    Fls_EndNotificationCount++;
}

void Fls_JobErrorNotification(void)
{
    Fls_ErrorNotificationCount++;
}

void setUp(void) {
    MockRegisters_Reset();
    Det_Mock_Reset();
    Fls_EndNotificationCount = 0U;
    Fls_ErrorNotificationCount = 0U;
    /* Restore the genuine uninitialized state (no DeInit API exists). */
    Fls_ConfigPtr = NULL_PTR;
    Fls_Status = FLS_UNINIT;
}

void tearDown(void) {
}

/* Initializes the module unless it already is (state was reset in setUp). */
static void test_Fls_EnsureInitialized(void)
{
    if (Fls_Status == FLS_UNINIT)
    {
        Fls_Init(&Fls_TestConfig);
    }
}

/* Verifies the last DET report matches the expected module/API/error triple. */
static void test_Fls_AssertDet(uint8 expectedApiId, uint8 expectedErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid); /* expected DET report */
    TEST_ASSERT_EQUAL_UINT(FLS_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT(FLS_INSTANCE_ID, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT(expectedApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT(expectedErrorId, Det_MockData.ErrorId);
}

/* Verifies count bytes at address match the expected pattern via the mock
 * register table (i.e. the data really landed in REG_WRITE8 territory). */
static void test_Fls_AssertRegisterPattern(Fls_AddressType address, const uint8* expected, uint32 count)
{
    uint32 i;
    for (i = 0U; i < count; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(expected[i], MockRegisters_Read8(address + i));
    }
}

/*==================================================================================================
 * Init
 *==================================================================================================*/

/** @req SWS_Fls_00001 */
void test_Fls_Init_BeforeInit_NullPtr_ShouldReportParamConfig(void) {
    Fls_Init(NULL_PTR);
    test_Fls_AssertDet(FLS_SID_INIT, FLS_E_PARAM_CONFIG);
    TEST_ASSERT_EQUAL(FLS_UNINIT, Fls_GetStatus());
}

/** @req SWS_Fls_00001 */
void test_Fls_Init_ValidConfig_ShouldReachIdle(void) {
    Fls_Init(&Fls_TestConfig);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00001 */
void test_Fls_Init_AlreadyInitialized_ShouldReportDetError(void) {
    Fls_Init(&Fls_TestConfig);
    Det_Mock_Reset();

    Fls_Init(&Fls_TestConfig);
    test_Fls_AssertDet(FLS_SID_INIT, FLS_E_ALREADY_INITIALIZED);
    /* The module stays initialized after the rejected re-init. */
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/*==================================================================================================
 * Erase
 *==================================================================================================*/

/** @req SWS_Fls_00002 */
void test_Fls_Erase_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fls_Erase(FLS_TEST_SECTOR0_START, FLS_TEST_SECTOR0_SIZE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_ERASE, FLS_E_UNINIT);
    TEST_ASSERT_EQUAL(FLS_UNINIT, Fls_GetStatus());
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_InvalidAddress_ShouldReportParamAddress(void) {
    test_Fls_EnsureInitialized();
    /* 0x09000000 is outside the configured flash window. */
    Std_ReturnType ret = Fls_Erase(0x09000000U, 256U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_ERASE, FLS_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_NotSectorAligned_ShouldReportInvalidAddress(void) {
    test_Fls_EnsureInitialized();
    /* In-range and valid length, but 0x100 is not 64K-sector aligned. */
    Std_ReturnType ret = Fls_Erase(FLS_TEST_SECTOR0_START + 0x100U, 256U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_ERASE, FLS_E_INVALID_ADDRESS);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_WhileBusy_ShouldReportBusy(void) {
    test_Fls_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Fls_Erase(FLS_TEST_SECTOR0_START, FLS_TEST_SECTOR0_SIZE));

    Std_ReturnType ret = Fls_Erase(FLS_TEST_SECTOR1_START, FLS_TEST_SECTOR1_SIZE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_ERASE, FLS_E_BUSY);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_ValidSector_ShouldCompleteJob(void) {
    test_Fls_EnsureInitialized();
    Std_ReturnType ret = Fls_Erase(FLS_TEST_SECTOR0_START, FLS_TEST_SECTOR0_SIZE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());

    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(1U, Fls_EndNotificationCount);
    TEST_ASSERT_EQUAL_UINT(0U, Fls_ErrorNotificationCount);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_SpanningTwoSectors_ShouldCompleteInTwoCycles(void) {
    test_Fls_EnsureInitialized();
    /* 64K + 1 byte: first cycle erases sector 0, second cycle sector 1. */
    Std_ReturnType ret = Fls_Erase(FLS_TEST_SECTOR0_START, FLS_TEST_SECTOR0_SIZE + 1U);
    TEST_ASSERT_EQUAL(E_OK, ret);

    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(0U, Fls_EndNotificationCount);

    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(1U, Fls_EndNotificationCount);
}

/*==================================================================================================
 * Write
 *==================================================================================================*/

/** @req SWS_Fls_00003 */
void test_Fls_Write_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_WRITE, FLS_E_UNINIT);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_NullSource_ShouldReportParamData(void) {
    test_Fls_EnsureInitialized();
    Std_ReturnType ret = Fls_Write(FLS_TEST_SECTOR0_START, NULL_PTR, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_WRITE, FLS_E_PARAM_DATA);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_InvalidAddress_ShouldReportParamAddress(void) {
    test_Fls_EnsureInitialized();
    Std_ReturnType ret = Fls_Write(0x09000000U, Fls_TestBuffer, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_WRITE, FLS_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_ZeroLength_ShouldReportParamAddress(void) {
    test_Fls_EnsureInitialized();
    Std_ReturnType ret = Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_WRITE, FLS_E_PARAM_ADDRESS);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_WhileBusy_ShouldReportBusy(void) {
    test_Fls_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U));

    Std_ReturnType ret = Fls_Write(FLS_TEST_SECTOR0_START + 16U, Fls_TestBuffer, 16U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Fls_AssertDet(FLS_SID_WRITE, FLS_E_BUSY);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_ValidData_ShouldStoreBytesAndComplete(void) {
    uint32 i;
    for (i = 0U; i < 16U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(0xA0U + i);
    }
    test_Fls_EnsureInitialized();

    Std_ReturnType ret = Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());

    /* 16 bytes fit within the 32-byte normal-mode write chunk. */
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(1U, Fls_EndNotificationCount);
    TEST_ASSERT_EQUAL_UINT(0U, Fls_ErrorNotificationCount);

    /* The bytes really reached the (mocked) flash address space. */
    test_Fls_AssertRegisterPattern(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_LargeData_ShouldSpanMultipleCycles(void) {
    uint32 i;
    for (i = 0U; i < 33U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(i * 3U);
    }
    test_Fls_EnsureInitialized();

    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 33U));

    /* First cycle writes at most 32 bytes (normal mode chunk). */
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(0U, Fls_EndNotificationCount);

    /* Second cycle writes the remaining byte and completes. */
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(1U, Fls_EndNotificationCount);

    test_Fls_AssertRegisterPattern(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 33U);
}

/** @req SWS_Fls_00007 */
void test_Fls_SetMode_FastMode_ShouldEnlargeWriteChunk(void) {
    uint32 i;
    for (i = 0U; i < 40U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(0x40U + i);
    }
    test_Fls_EnsureInitialized();

    Fls_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);

    /* 40 bytes exceed the 32-byte normal chunk but fit the 64-byte fast
     * chunk, so a single MainFunction cycle completes the job. */
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 40U));
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(1U, Fls_EndNotificationCount);

    test_Fls_AssertRegisterPattern(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 40U);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_JobExceedingTimeout_ShouldFailWithRuntimeError(void) {
    uint32 cycle;
    test_Fls_EnsureInitialized();

    /* 32001 bytes need 1001 cycles at 32 bytes/cycle, but the timeout
     * expires after 1000 cycles (FLS_TIMEOUT_VALUE). */
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBigBuffer, 32001U));

    for (cycle = 0U; cycle < 1000U; cycle++)
    {
        Fls_MainFunction();
    }

    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(0U, Fls_EndNotificationCount);
    TEST_ASSERT_EQUAL_UINT(1U, Fls_ErrorNotificationCount);
    /* The runtime error is routed through Det_ReportError by the mock. */
    test_Fls_AssertDet(0U, FLS_E_ERASE_FAILED);

    /* Further MainFunction calls must not resurrect the failed job. */
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, Fls_GetJobResult());
}

/*==================================================================================================
 * Read
 *==================================================================================================*/

/** @req SWS_Fls_00004 */
void test_Fls_Read_BeforeInit_ShouldReportUninit(void) {
    Fls_Read(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
    test_Fls_AssertDet(FLS_SID_READ, FLS_E_UNINIT);
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_NullTarget_ShouldReportParamData(void) {
    test_Fls_EnsureInitialized();
    Fls_Read(FLS_TEST_SECTOR0_START, NULL_PTR, 16U);
    test_Fls_AssertDet(FLS_SID_READ, FLS_E_PARAM_DATA);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_InvalidAddress_ShouldReportParamAddress(void) {
    test_Fls_EnsureInitialized();
    Fls_Read(0x09000000U, Fls_TestBuffer, 16U);
    test_Fls_AssertDet(FLS_SID_READ, FLS_E_PARAM_ADDRESS);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_WhileBusy_ShouldBeIgnoredSilently(void) {
    uint32 i;
    for (i = 0U; i < 16U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(0x11U + i);
    }
    test_Fls_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U));

    /* Fls_Read performs no busy check: the request is silently ignored. */
    Fls_Read(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());

    /* The original write job still completes normally. */
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(1U, Fls_EndNotificationCount);
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_ValidBuffer_ShouldReadBackWrittenData(void) {
    uint8 readBack[64];
    uint32 i;
    for (i = 0U; i < 64U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(0x80U + i);
        readBack[i] = 0U;
    }
    test_Fls_EnsureInitialized();

    /* Store a known pattern through a write job (two 32-byte chunks). */
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 64U));
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());

    /* Read it back through the asynchronous read job. */
    Fls_Read(FLS_TEST_SECTOR0_START, readBack, 64U);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());

    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(2U, Fls_EndNotificationCount); /* write + read */

    for (i = 0U; i < 64U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(Fls_TestBuffer[i], readBack[i]);
    }
}

/** @req SWS_Fls_00005 */
void test_Fls_ReadSync_AfterWrite_ShouldReturnStoredData(void) {
    uint8 readBack[16];
    uint32 i;
    for (i = 0U; i < 16U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(0xC0U + i);
        readBack[i] = 0U;
    }
    test_Fls_EnsureInitialized();

    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U));
    Fls_MainFunction();

    /* Synchronous read must work while the driver is idle. */
    Std_ReturnType ret = Fls_ReadSync(FLS_TEST_SECTOR0_START, readBack, 16U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    for (i = 0U; i < 16U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(Fls_TestBuffer[i], readBack[i]);
    }
}

/*==================================================================================================
 * Compare
 *==================================================================================================*/

/** @req SWS_Fls_00006 */
void test_Fls_Compare_BeforeInit_ShouldReportUninit(void) {
    Fls_Compare(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
    test_Fls_AssertDet(FLS_SID_COMPARE, FLS_E_UNINIT);
}

/** @req SWS_Fls_00006 */
void test_Fls_Compare_NullTarget_ShouldReportParamData(void) {
    test_Fls_EnsureInitialized();
    Fls_Compare(FLS_TEST_SECTOR0_START, NULL_PTR, 16U);
    test_Fls_AssertDet(FLS_SID_COMPARE, FLS_E_PARAM_DATA);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
}

/** @req SWS_Fls_00006 */
void test_Fls_Compare_Match_ShouldReturnJobOk(void) {
    uint32 i;
    for (i = 0U; i < 16U; i++)
    {
        Fls_TestBuffer[i] = (uint8)(0x60U + i);
    }
    test_Fls_EnsureInitialized();

    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U));
    Fls_MainFunction();

    /* Compare flash content against the identical buffer. */
    Fls_Compare(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());

    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(2U, Fls_EndNotificationCount);   /* write + compare */
    TEST_ASSERT_EQUAL_UINT(0U, Fls_ErrorNotificationCount);
}

/** @req SWS_Fls_00006 */
void test_Fls_Compare_Mismatch_ShouldReturnBlockInconsistent(void) {
    uint8 compareBuffer[16];
    uint32 i;
    for (i = 0U; i < 16U; i++)
    {
        Fls_TestBuffer[i] = 0xA5U;
        compareBuffer[i] = 0x5AU; /* differs in every byte */
    }
    test_Fls_EnsureInitialized();

    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U));
    Fls_MainFunction();

    Fls_Compare(FLS_TEST_SECTOR0_START, compareBuffer, 16U);
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());

    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_BLOCK_INCONSISTENT, Fls_GetJobResult());
    /* Source quirk: MEMIF_BLOCK_INCONSISTENT is 0x00, the same value as
     * MEMIF_JOB_OK, so the end (not error) notification fires on mismatch. */
    TEST_ASSERT_EQUAL_UINT(2U, Fls_EndNotificationCount);
    TEST_ASSERT_EQUAL_UINT(0U, Fls_ErrorNotificationCount);
}

/*==================================================================================================
 * SetMode / Cancel
 *==================================================================================================*/

/** @req SWS_Fls_00007 */
void test_Fls_SetMode_BeforeInit_ShouldReportUninit(void) {
    Fls_SetMode(MEMIF_MODE_FAST);
    test_Fls_AssertDet(FLS_SID_SETMODE, FLS_E_UNINIT);
}

/** @req SWS_Fls_00010 */
void test_Fls_Cancel_BeforeInit_ShouldReportUninit(void) {
    Fls_Cancel();
    test_Fls_AssertDet(FLS_SID_CANCEL, FLS_E_UNINIT);
}

/** @req SWS_Fls_00010 */
void test_Fls_Cancel_WhenIdle_ShouldBeSilent(void) {
    test_Fls_EnsureInitialized();
    Fls_Cancel();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Fls_EndNotificationCount);
    TEST_ASSERT_EQUAL_UINT(0U, Fls_ErrorNotificationCount);
}

/** @req SWS_Fls_00010 */
void test_Fls_Cancel_DuringJob_ShouldCancelToIdle(void) {
    test_Fls_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBigBuffer, 32001U));
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());

    Fls_Cancel();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fls_GetJobResult());
    /* Cancelled jobs trigger neither notification. */
    TEST_ASSERT_EQUAL_UINT(0U, Fls_EndNotificationCount);
    TEST_ASSERT_EQUAL_UINT(0U, Fls_ErrorNotificationCount);

    /* MainFunction must not advance a cancelled job. */
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_CANCELED, Fls_GetJobResult());
}

/*==================================================================================================
 * Status / JobResult / MainFunction / VersionInfo
 *==================================================================================================*/

/** @req SWS_Fls_00008 */
void test_Fls_GetStatus_BeforeInit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL(FLS_UNINIT, Fls_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Fls_00008 */
void test_Fls_GetStatus_WithPendingJob_ShouldReturnBusy(void) {
    test_Fls_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Fls_Erase(FLS_TEST_SECTOR0_START, FLS_TEST_SECTOR0_SIZE));
    TEST_ASSERT_EQUAL(FLS_BUSY, Fls_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Fls_00009 */
void test_Fls_GetJobResult_BeforeInit_ShouldReportUninit(void) {
    Fls_JobResultType result = Fls_GetJobResult();
    TEST_ASSERT_EQUAL(MEMIF_JOB_FAILED, result);
    test_Fls_AssertDet(FLS_SID_GETJOBRESULT, FLS_E_UNINIT);
}

/** @req SWS_Fls_00009 */
void test_Fls_GetJobResult_WithPendingJob_ShouldReturnPending(void) {
    test_Fls_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(FLS_TEST_SECTOR0_START, Fls_TestBuffer, 16U));
    TEST_ASSERT_EQUAL(MEMIF_JOB_PENDING, Fls_GetJobResult());
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
}

/** @req SWS_Fls_00011 */
void test_Fls_MainFunction_WhenIdle_ShouldDoNothing(void) {
    test_Fls_EnsureInitialized();
    Fls_MainFunction();
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(FLS_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL_UINT(0U, Fls_EndNotificationCount);
}

/** @req SWS_Fls_00012 */
void test_Fls_GetVersionInfo_NullPtr_ShouldReportDetError(void) {
    Fls_GetVersionInfo(NULL_PTR);
    test_Fls_AssertDet(FLS_SID_GETVERSIONINFO, FLS_E_PARAM_POINTER);
    TEST_ASSERT_EQUAL_UINT(1U, Det_MockData.CallCount);
}

/** @req SWS_Fls_00012 */
void test_Fls_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    Fls_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT(FLS_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT(FLS_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT(FLS_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT(FLS_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT(FLS_SW_PATCH_VERSION, info.sw_patch_version);
}

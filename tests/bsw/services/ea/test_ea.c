/**
 * @file test_ea.c
 * @brief Ea (EEPROM Abstraction) unit tests, substantiated against the
 *        production implementation in src/bsw/ecual/ea/src/Ea.c.
 * @version 1.0.0
 *
 * @tests src/bsw/ecual/ea/src/Ea.c
 * @tests src/bsw/ecual/ea/include/Ea.h
 *
 * Behavioural notes (assertions encode the real SUT behaviour):
 *  - The legacy placeholder tests called APIs that do not exist (Ea_Read(),
 *    Ea_EraseImmediate(), ...). All tests now use the real Ea.h signatures.
 *  - Ea_MainFunction() silently returns when the module is uninitialized
 *    (no DET), and Ea_GetStatus()/Ea_GetJobResult() DO report EA_E_UNINIT.
 *  - Block N is mapped to EEPROM address N * EA_MAX_BLOCK_SIZE by
 *    Ea_CalculateBlockAddress().
 *  - There is no Ea_DeInit; "uninit" tests run first in the runner while the
 *    static Ea_Initialized flag is still FALSE.
 */

#include "unity.h"
#include "Ea.h"
#include "MemIf.h"
#include "Std_Types.h"
#include <string.h>

/* MemIf_Cfg.h maps the Ea_* names to legacy 2-argument function-like macros;
 * undo that so calls bind to the real 4-argument Ea.h API. */
#undef Ea_Read
#undef Ea_Write
#undef Ea_Cancel
#undef Ea_GetStatus
#undef Ea_GetJobResult

/* ------------------------------------------------------------------ */
/* Own DET mock: records every Det_ReportError call                    */
/* ------------------------------------------------------------------ */
static uint32 mock_DetCalls;
static uint16 mock_lastModuleId;
static uint8  mock_lastApiId;
static uint8  mock_lastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)InstanceId;
    mock_DetCalls++;
    mock_lastModuleId = ModuleId;
    mock_lastApiId = ApiId;
    mock_lastErrorId = ErrorId;
    return E_OK;
}

/* ------------------------------------------------------------------ */
/* Eep driver stub recorders (implemented in eep_stubs.c)            */
/* ------------------------------------------------------------------ */
extern uint32 stub_Eep_Read_calls;
extern uint32 stub_Eep_Write_calls;
extern uint32 stub_Eep_Erase_calls;
extern uint32 stub_Eep_Cancel_calls;
extern uint32 stub_Eep_GetJobResult_calls;
extern uint32 stub_Eep_last_Address;
extern uint16 stub_Eep_last_Length;
extern uint8* stub_Eep_last_ReadPtr;
extern const uint8* stub_Eep_last_WritePtr;
extern Std_ReturnType stub_Eep_Read_ret;
extern Std_ReturnType stub_Eep_Write_ret;
extern Std_ReturnType stub_Eep_Erase_ret;
extern MemIf_JobResultType stub_Eep_JobResult;

static uint8 testBuf[32];

/* ------------------------------------------------------------------ */
/* Own test configuration (never uses the Ea_Config symbol from      */
/* Ea_Lcfg.c, so its object is never pulled from the static library)  */
/* ------------------------------------------------------------------ */
#define EA_TEST_BLOCK_1    (1u)
#define EA_TEST_BLOCK_2    (2u)
#define EA_TEST_BLOCK_SIZE (16u)

static Ea_BlockConfigType testBlockConfig[2];
static Ea_ConfigType testConfig;

static void init_config(void)
{
    (void)memset(testBlockConfig, 0, sizeof(testBlockConfig));
    (void)memset(&testConfig, 0, sizeof(testConfig));

    testBlockConfig[0].BlockId = EA_TEST_BLOCK_1;
    testBlockConfig[0].BlockSize = EA_TEST_BLOCK_SIZE;
    testBlockConfig[1].BlockId = EA_TEST_BLOCK_2;
    testBlockConfig[1].BlockSize = 32u;

    testConfig.BlockConfig = testBlockConfig;
    testConfig.NumBlocks = 2u;
    testConfig.EaSectorSize = 4096u;
    testConfig.EaNumberOfSectors = 8u;
    testConfig.EaSetModeSupported = TRUE;
    testConfig.EaVersionInfoApi = TRUE;
}

void setUp(void)
{
    mock_DetCalls = 0u;
    mock_lastModuleId = 0u;
    mock_lastApiId = 0u;
    mock_lastErrorId = 0u;

    stub_Eep_Read_calls = 0u;
    stub_Eep_Write_calls = 0u;
    stub_Eep_Erase_calls = 0u;
    stub_Eep_Cancel_calls = 0u;
    stub_Eep_GetJobResult_calls = 0u;
    stub_Eep_last_Address = 0u;
    stub_Eep_last_Length = 0u;
    stub_Eep_last_ReadPtr = (uint8*)0;
    stub_Eep_last_WritePtr = (const uint8*)0;
    stub_Eep_Read_ret = E_OK;
    stub_Eep_Write_ret = E_OK;
    stub_Eep_Erase_ret = E_OK;
    stub_Eep_JobResult = MEMIF_JOB_OK;

    (void)memset(testBuf, 0xA5, sizeof(testBuf));
    init_config();
}

void tearDown(void)
{
}

/* ================================================================== */
/* Uninitialized guard tests (must run first)                         */
/* ================================================================== */

/** @req SWS_Ea_00001 */
void test_Ea_Init_NullPtr_ShouldReportDet(void)
{
    Ea_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX16(EA_MODULE_ID, mock_lastModuleId);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_INIT, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_CFG, mock_lastErrorId);
}

/** @req SWS_Ea_00004 */
void test_Ea_SetMode_Uninit_ShouldReportDet(void)
{
    Ea_SetMode(EA_MODE_SLOW);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_SETMODE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00005 */
void test_Ea_Read_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_READ, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00006 */
void test_Ea_Write_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_Write(EA_TEST_BLOCK_1, testBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_WRITE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00007 */
void test_Ea_Cancel_Uninit_ShouldReportError(void)
{
    Ea_Cancel();
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_CANCEL, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00007 */
void test_Ea_GetStatus_Uninit_ShouldReportDet(void)
{
    Ea_StatusType status = Ea_GetStatus();
    TEST_ASSERT_EQUAL(EA_IDLE, status);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_GETSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00007 */
void test_Ea_GetJobResult_Uninit_ShouldReportDet(void)
{
    Ea_JobResultType result = Ea_GetJobResult();
    TEST_ASSERT_EQUAL(EA_JOB_FAILED, result);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_GETJOBRESULT, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00008 */
void test_Ea_InvalidateBlock_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_InvalidateBlock(EA_TEST_BLOCK_1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_INVALIDATEBLOCK, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00009 */
void test_Ea_EraseImmediateBlock_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_EraseImmediateBlock(EA_TEST_BLOCK_1);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_ERASEIMMEDIATEBLOCK, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00012 */
void test_Ea_GetEraseCycleCount_Uninit_ShouldReportDet(void)
{
    uint32 count = Ea_GetEraseCycleCount();
    TEST_ASSERT_EQUAL_UINT32(0u, count);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_GETERASECYCLECOUNT, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_Ea_00003 */
void test_Ea_MainFunction_Uninit_ShouldBeSilent(void)
{
    /* SUT returns silently without any DET when uninitialized */
    Ea_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_Eep_GetJobResult_calls);
}

/* ================================================================== */
/* Initialization                                                    */
/* ================================================================== */

/** @req SWS_Ea_00001 */
void test_Ea_Init_ValidConfig_ShouldSucceed(void)
{
    Ea_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/** @req SWS_Ea_00001 */
void test_Ea_Init_DoubleInit_ShouldSucceed(void)
{
    Ea_Init(&testConfig);
    Ea_Init(&testConfig);
    /* SUT has no double-init DET check */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
}

/* ================================================================== */
/* Mode and version                                                   */
/* ================================================================== */

/** @req SWS_Ea_00004 */
void test_Ea_SetMode_ValidAndInvalidMode_ShouldValidate(void)
{
    Ea_Init(&testConfig);
    Ea_SetMode(EA_MODE_SLOW);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    Ea_SetMode((Ea_ModeType)9u);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_SETMODE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_MODE, mock_lastErrorId);
}

/** @req SWS_Ea_00002 */
void test_Ea_GetVersionInfo_NullPtr_ShouldReportError(void)
{
    Ea_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_GETVERSIONINFO, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_DATA_PTR, mock_lastErrorId);
}

/** @req SWS_Ea_00002 */
void test_Ea_GetVersionInfo_ValidPtr_ShouldSucceed(void)
{
    Std_VersionInfoType info;
    (void)memset(&info, 0, sizeof(info));
    Ea_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_HEX16(EA_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_HEX16(EA_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_HEX8(EA_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_HEX8(EA_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_HEX8(EA_SW_PATCH_VERSION, info.sw_patch_version);
}

/* ================================================================== */
/* Read                                                               */
/* ================================================================== */

/** @req SWS_Ea_00005 */
void test_Ea_Read_InvalidBlock_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Read((Ea_BlockIdType)EA_NUM_BLOCKS, 0u, testBuf, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_READ, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_NO, mock_lastErrorId);
}

/** @req SWS_Ea_00005 */
void test_Ea_Read_InvalidOffset_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Read(EA_TEST_BLOCK_1, (uint16)EA_MAX_BLOCK_SIZE, testBuf, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_READ, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_OFS, mock_lastErrorId);
}

/** @req SWS_Ea_00005 */
void test_Ea_Read_NullPtr_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Read(EA_TEST_BLOCK_1, 0u, NULL_PTR, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_READ, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_DATA_PTR, mock_lastErrorId);
}

/** @req SWS_Ea_00005 */
void test_Ea_Read_InvalidLength_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    /* Length == 0 is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 0u));
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_LEN, mock_lastErrorId);
    /* Length > EA_MAX_BLOCK_SIZE is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, (uint16)(EA_MAX_BLOCK_SIZE + 1u)));
    TEST_ASSERT_EQUAL_UINT32(2u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_LEN, mock_lastErrorId);
}

/** @req SWS_Ea_00005 */
void test_Ea_Read_UnconfiguredBlock_ShouldFailSilently(void)
{
    Ea_Init(&testConfig);
    /* block 5 has no descriptor: rejected without DET, job result EA_BLOCK_INVALID */
    Std_ReturnType ret = Ea_Read(5u, 0u, testBuf, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_BLOCK_INVALID, Ea_GetJobResult());
}

/** @req SWS_Ea_00005 */
void test_Ea_Read_ValidCall_ShouldStartJob(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 8u);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Read_calls);
    TEST_ASSERT_EQUAL_UINT32((uint32)EA_TEST_BLOCK_1 * (uint32)EA_MAX_BLOCK_SIZE, stub_Eep_last_Address);
    TEST_ASSERT_EQUAL_UINT16(8u, stub_Eep_last_Length);
    TEST_ASSERT_TRUE(testBuf == stub_Eep_last_ReadPtr);
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_PENDING, Ea_GetJobResult());
}

/** @req SWS_Ea_00003 */
/** @req SWS_Ea_00005 */
void test_Ea_Read_JobCompletesViaMainFunction(void)
{
    Ea_Init(&testConfig);
    (void)Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 8u);

    stub_Eep_JobResult = MEMIF_JOB_PENDING;
    Ea_MainFunction();
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_PENDING, Ea_GetJobResult());

    stub_Eep_JobResult = MEMIF_JOB_OK;
    Ea_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(2u, stub_Eep_GetJobResult_calls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/* ================================================================== */
/* Write                                                              */
/* ================================================================== */

/** @req SWS_Ea_00006 */
void test_Ea_Write_InvalidBlock_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Write((Ea_BlockIdType)EA_NUM_BLOCKS, testBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_WRITE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_NO, mock_lastErrorId);
}

/** @req SWS_Ea_00006 */
void test_Ea_Write_NullPtr_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Write(EA_TEST_BLOCK_1, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_WRITE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_DATA_PTR, mock_lastErrorId);
}

/** @req SWS_Ea_00006 */
void test_Ea_Write_ValidCall_ShouldStartJob(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_Write(EA_TEST_BLOCK_1, testBuf);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Write_calls);
    TEST_ASSERT_EQUAL_UINT32((uint32)EA_TEST_BLOCK_1 * (uint32)EA_MAX_BLOCK_SIZE, stub_Eep_last_Address);
    TEST_ASSERT_EQUAL_UINT16(EA_TEST_BLOCK_SIZE, stub_Eep_last_Length);
    TEST_ASSERT_TRUE(testBuf == stub_Eep_last_WritePtr);
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_PENDING, Ea_GetJobResult());

    Ea_MainFunction();
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/** @req SWS_Ea_00006 */
void test_Ea_Write_WhileBusy_ShouldFail(void)
{
    Ea_Init(&testConfig);
    (void)Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 8u); /* module now EA_BUSY */
    Std_ReturnType ret = Ea_Write(EA_TEST_BLOCK_2, testBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_Eep_Write_calls);
}

/* ================================================================== */
/* Invalidate / erase / cancel                                        */
/* ================================================================== */

/** @req SWS_Ea_00008 */
void test_Ea_InvalidateBlock_InvalidBlock_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_InvalidateBlock((Ea_BlockIdType)EA_NUM_BLOCKS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_INVALIDATEBLOCK, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_NO, mock_lastErrorId);
}

/** @req SWS_Ea_00008 */
void test_Ea_InvalidateBlock_ValidCall_ShouldInvalidateAndBlockRead(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_InvalidateBlock(EA_TEST_BLOCK_1);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    /* invalidate completes immediately in this implementation */
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());

    /* an invalidated block can no longer be read (no DET, EA_BLOCK_INVALID) */
    TEST_ASSERT_EQUAL(E_NOT_OK, Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 4u));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_BLOCK_INVALID, Ea_GetJobResult());
}

/** @req SWS_Ea_00009 */
void test_Ea_EraseImmediateBlock_InvalidBlock_ShouldReportError(void)
{
    Ea_Init(&testConfig);
    Std_ReturnType ret = Ea_EraseImmediateBlock((Ea_BlockIdType)EA_NUM_BLOCKS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_ERASEIMMEDIATEBLOCK, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_NO, mock_lastErrorId);
}

/** @req SWS_Ea_00009 */
/** @req SWS_Ea_00012 */
void test_Ea_EraseImmediateBlock_ValidCall_ShouldEraseAndCount(void)
{
    uint32 countBefore;

    Ea_Init(&testConfig);
    countBefore = Ea_GetEraseCycleCount();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);

    Std_ReturnType ret = Ea_EraseImmediateBlock(EA_TEST_BLOCK_1);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Erase_calls);
    TEST_ASSERT_EQUAL_UINT32((uint32)EA_TEST_BLOCK_1 * (uint32)EA_MAX_BLOCK_SIZE, stub_Eep_last_Address);
    TEST_ASSERT_EQUAL_UINT16(EA_TEST_BLOCK_SIZE, stub_Eep_last_Length);
    TEST_ASSERT_EQUAL_UINT32(countBefore + 1u, Ea_GetEraseCycleCount());

    /* erase stays pending until the driver job completes */
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_PENDING, Ea_GetJobResult());
    Ea_MainFunction();
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/** @req SWS_Ea_00007 */
void test_Ea_Cancel_WhileBusy_ShouldCancelJob(void)
{
    Ea_Init(&testConfig);
    (void)Ea_Read(EA_TEST_BLOCK_1, 0u, testBuf, 8u);
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());

    Ea_Cancel();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Cancel_calls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_CANCELLED, Ea_GetJobResult());
}

/** @req SWS_Ea_00003 */
void test_Ea_MainFunction_NoPendingJob_ShouldBeSilent(void)
{
    Ea_Init(&testConfig);
    Ea_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_Eep_GetJobResult_calls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
}

/* ================================================================== */
/* Runner — uninit tests must run first (module flag still FALSE)     */
/* ================================================================== */
int main(void)
{
    UNITY_BEGIN();

    /* guard tests that require the module to be uninitialized */
    RUN_TEST(test_Ea_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_Ea_SetMode_Uninit_ShouldReportDet);
    RUN_TEST(test_Ea_Read_Uninit_ShouldReportError);
    RUN_TEST(test_Ea_Write_Uninit_ShouldReportError);
    RUN_TEST(test_Ea_Cancel_Uninit_ShouldReportError);
    RUN_TEST(test_Ea_GetStatus_Uninit_ShouldReportDet);
    RUN_TEST(test_Ea_GetJobResult_Uninit_ShouldReportDet);
    RUN_TEST(test_Ea_InvalidateBlock_Uninit_ShouldReportError);
    RUN_TEST(test_Ea_EraseImmediateBlock_Uninit_ShouldReportError);
    RUN_TEST(test_Ea_GetEraseCycleCount_Uninit_ShouldReportDet);
    RUN_TEST(test_Ea_MainFunction_Uninit_ShouldBeSilent);

    /* initialized operation */
    RUN_TEST(test_Ea_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Ea_Init_DoubleInit_ShouldSucceed);
    RUN_TEST(test_Ea_SetMode_ValidAndInvalidMode_ShouldValidate);
    RUN_TEST(test_Ea_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Ea_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_Ea_Read_InvalidBlock_ShouldReportError);
    RUN_TEST(test_Ea_Read_InvalidOffset_ShouldReportError);
    RUN_TEST(test_Ea_Read_NullPtr_ShouldReportError);
    RUN_TEST(test_Ea_Read_InvalidLength_ShouldReportError);
    RUN_TEST(test_Ea_Read_UnconfiguredBlock_ShouldFailSilently);
    RUN_TEST(test_Ea_Read_ValidCall_ShouldStartJob);
    RUN_TEST(test_Ea_Read_JobCompletesViaMainFunction);
    RUN_TEST(test_Ea_Write_InvalidBlock_ShouldReportError);
    RUN_TEST(test_Ea_Write_NullPtr_ShouldReportError);
    RUN_TEST(test_Ea_Write_ValidCall_ShouldStartJob);
    RUN_TEST(test_Ea_Write_WhileBusy_ShouldFail);
    RUN_TEST(test_Ea_InvalidateBlock_InvalidBlock_ShouldReportError);
    RUN_TEST(test_Ea_InvalidateBlock_ValidCall_ShouldInvalidateAndBlockRead);
    RUN_TEST(test_Ea_EraseImmediateBlock_InvalidBlock_ShouldReportError);
    RUN_TEST(test_Ea_EraseImmediateBlock_ValidCall_ShouldEraseAndCount);
    RUN_TEST(test_Ea_Cancel_WhileBusy_ShouldCancelJob);
    RUN_TEST(test_Ea_MainFunction_NoPendingJob_ShouldBeSilent);

    return UnityEnd();
}

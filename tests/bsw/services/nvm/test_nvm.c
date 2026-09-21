/**
 * @file test_nvm.c
 * @brief NvM (NVRAM Manager) unit tests, substantiated against the production
 *        implementation in src/bsw/services/nvm/src/NvM.c.
 * @req SWS_NvM
 *
 * @tests src/bsw/services/nvm/src/NvM.c
 * @tests src/bsw/services/nvm/include/NvM.h
 *
 * Behavioural notes (assertions encode the real SUT behaviour):
 *  - The DET ApiIds used inside NvM.c are numeric literals (Init=0x01,
 *    SetDataIndex=0x01, ReadBlock=0x04, WriteBlock=0x05,
 *    RestoreBlockDefaults=0x06, EraseNvBlock=0x09,
 *    InvalidateNvBlock=0x0A, GetErrorStatus=0x0B, SetRamBlockStatus=0x0F,
 *    SetBlockLockStatus=0x13, SetBlockProtection=0x14) which intentionally
 *    differ from the NVM_SID_* macros in NvM.h.
 *  - Likewise NvM.c #defines its own NVM_E_* error codes that shadow the
 *    header: PARAM_POINTER=0x01, PARAM_BLOCK_ID=0x02, NOT_INITIALIZED=0x03,
 *    BLOCK_PENDING=0x04. The remaining codes (BLOCK_TYPE=0x0C,
 *    DATA_IDX=0x0D, WRITE_PROTECTED=0x12) come from NvM.h and match.
 *  - NvM_GetVersionInfo() silently ignores a NULL pointer (no DET call).
 *  - There is no NvM_DeInit; the "before init" tests are run first in the
 *    runner while the static NvM_InternalState is still NVM_STATE_UNINIT.
 *  - The test uses its own configuration symbol (testConfig); the legacy
 *    NvM_Config from the SUT-directory self-test NvM_test.c is never
 *    referenced, so its duplicate Det/MemIf mocks are never pulled in.
 */

#include "unity.h"
#include "NvM.h"
#include "MemIf.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Own DET mock: records every Det_ReportError call                    */
/* ------------------------------------------------------------------ */
static uint32 mock_DetCalls;
static uint16 mock_lastModuleId;
static uint8  mock_lastInstanceId;
static uint8  mock_lastApiId;
static uint8  mock_lastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    mock_DetCalls++;
    mock_lastModuleId = ModuleId;
    mock_lastInstanceId = InstanceId;
    mock_lastApiId = ApiId;
    mock_lastErrorId = ErrorId;
    return E_OK;
}

/* ------------------------------------------------------------------ */
/* MemIf stub recorders (implemented in stubs.c)                     */
/* ------------------------------------------------------------------ */
extern uint32 stub_MemIf_Read_calls;
extern uint32 stub_MemIf_Write_calls;
extern uint32 stub_MemIf_Erase_calls;
extern uint32 stub_MemIf_Invalidate_calls;
extern uint8  stub_last_Device;
extern uint16 stub_last_BlockNumber;
extern uint16 stub_last_Offset;
extern uint16 stub_last_Length;
extern uint8* stub_last_ReadPtr;
extern const uint8* stub_last_WritePtr;
extern Std_ReturnType stub_MemIf_Read_ret;
extern Std_ReturnType stub_MemIf_Write_ret;
extern Std_ReturnType stub_MemIf_Erase_ret;
extern Std_ReturnType stub_MemIf_Invalidate_ret;
extern MemIf_StatusType stub_MemIf_Status;
extern MemIf_JobResultType stub_MemIf_JobResult;

static uint8 srcBuf[8];
static uint8 dstBuf[8];

/* ------------------------------------------------------------------ */
/* Own test configuration (never uses the NvM_Config symbol)         */
/* ------------------------------------------------------------------ */
#define NVM_TEST_BLOCK_NATIVE_ID     (1u)
#define NVM_TEST_BLOCK_PROTECTED_ID  (2u)
#define NVM_TEST_BLOCK_DATASET_ID    (3u)
#define NVM_TEST_BLOCK_LENGTH        (8u)

/* DET error codes as compiled into NvM.c (its private NVM_E_* macros shadow
 * the NvM.h header values; see file header note). */
#define NVM_TEST_E_PARAM_POINTER     (0x01u)
#define NVM_TEST_E_PARAM_BLOCK_ID    (0x02u)
#define NVM_TEST_E_NOT_INITIALIZED   (0x03u)
#define NVM_TEST_E_BLOCK_PENDING     (0x04u)

static NvM_BlockDescriptorType testBlockDescriptors[3];
static NvM_ConfigType testConfig;

static void init_config(void)
{
    (void)memset(testBlockDescriptors, 0, sizeof(testBlockDescriptors));
    (void)memset(&testConfig, 0, sizeof(testConfig));

    /* Block 1: native, writable, no CRC, 8 bytes */
    testBlockDescriptors[0].BlockId = NVM_TEST_BLOCK_NATIVE_ID;
    testBlockDescriptors[0].DeviceId = 0u;
    testBlockDescriptors[0].BlockBaseNumber = 0x0001u;
    testBlockDescriptors[0].ManagementType = NVM_BLOCK_NATIVE;
    testBlockDescriptors[0].NumberOfNvBlocks = 1u;
    testBlockDescriptors[0].NvBlockLength = NVM_TEST_BLOCK_LENGTH;
    testBlockDescriptors[0].CrcType = NVM_CRC_NONE;
    testBlockDescriptors[0].BlockUseCrc = FALSE;

    /* Block 2: native, write-protected */
    testBlockDescriptors[1].BlockId = NVM_TEST_BLOCK_PROTECTED_ID;
    testBlockDescriptors[1].DeviceId = 0u;
    testBlockDescriptors[1].BlockBaseNumber = 0x0002u;
    testBlockDescriptors[1].ManagementType = NVM_BLOCK_NATIVE;
    testBlockDescriptors[1].NumberOfNvBlocks = 1u;
    testBlockDescriptors[1].NvBlockLength = NVM_TEST_BLOCK_LENGTH;
    testBlockDescriptors[1].CrcType = NVM_CRC_NONE;
    testBlockDescriptors[1].BlockUseCrc = FALSE;
    testBlockDescriptors[1].BlockWriteProt = TRUE;

    /* Block 3: dataset with 2 NV blocks / 2 datasets */
    testBlockDescriptors[2].BlockId = NVM_TEST_BLOCK_DATASET_ID;
    testBlockDescriptors[2].DeviceId = 0u;
    testBlockDescriptors[2].BlockBaseNumber = 0x0010u;
    testBlockDescriptors[2].ManagementType = NVM_BLOCK_DATASET;
    testBlockDescriptors[2].NumberOfNvBlocks = 2u;
    testBlockDescriptors[2].NumberOfDataSets = 2u;
    testBlockDescriptors[2].NvBlockLength = NVM_TEST_BLOCK_LENGTH;
    testBlockDescriptors[2].CrcType = NVM_CRC_NONE;
    testBlockDescriptors[2].BlockUseCrc = FALSE;

    testConfig.BlockDescriptors = testBlockDescriptors;
    testConfig.NumBlockDescriptors = 3u;
    testConfig.NumOfNvBlocks = 4u;
    testConfig.NumOfDataSets = 2u;
    testConfig.MaxNumberOfWriteRetries = 3u;
    testConfig.MaxNumberOfReadRetries = 3u;
    testConfig.MainFunctionPeriod = 10u;
}

void setUp(void)
{
    mock_DetCalls = 0u;
    mock_lastModuleId = 0u;
    mock_lastInstanceId = 0u;
    mock_lastApiId = 0u;
    mock_lastErrorId = 0u;

    stub_MemIf_Read_calls = 0u;
    stub_MemIf_Write_calls = 0u;
    stub_MemIf_Erase_calls = 0u;
    stub_MemIf_Invalidate_calls = 0u;
    stub_last_Device = 0u;
    stub_last_BlockNumber = 0u;
    stub_last_Offset = 0u;
    stub_last_Length = 0u;
    stub_last_ReadPtr = (uint8*)0;
    stub_last_WritePtr = (const uint8*)0;
    stub_MemIf_Read_ret = E_OK;
    stub_MemIf_Write_ret = E_OK;
    stub_MemIf_Erase_ret = E_OK;
    stub_MemIf_Invalidate_ret = E_OK;
    stub_MemIf_Status = MEMIF_IDLE;
    stub_MemIf_JobResult = MEMIF_JOB_OK;

    (void)memset(srcBuf, 0, sizeof(srcBuf));
    (void)memset(dstBuf, 0, sizeof(dstBuf));

    init_config();
}

void tearDown(void)
{
}

/* ================================================================== */
/* Before-init guard tests (must run first: state is still UNINIT)    */
/* ================================================================== */

/** @req SWS_NvM_00001 */
void test_NvM_Init_NullPtr_ShouldReportDet(void)
{
    NvM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX16(NVM_MODULE_ID, mock_lastModuleId);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_PARAM_POINTER, mock_lastErrorId);
}

/** @req SWS_NvM_00002 */
void test_NvM_ReadBlock_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_ReadBlock(NVM_TEST_BLOCK_NATIVE_ID, dstBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x04u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00003 */
void test_NvM_WriteBlock_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_WriteBlock(NVM_TEST_BLOCK_NATIVE_ID, srcBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x05u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00004 */
void test_NvM_RestoreBlockDefaults_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_RestoreBlockDefaults(NVM_TEST_BLOCK_NATIVE_ID, dstBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x06u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00013 */
void test_NvM_EraseNvBlock_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_EraseNvBlock(NVM_TEST_BLOCK_NATIVE_ID);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x09u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00014 */
void test_NvM_InvalidateNvBlock_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_InvalidateNvBlock(NVM_TEST_BLOCK_NATIVE_ID);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x0Au, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00005 */
void test_NvM_SetDataIndex_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_SetDataIndex(NVM_TEST_BLOCK_DATASET_ID, 0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00007 */
void test_NvM_SetBlockLockStatus_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_SetBlockLockStatus(NVM_TEST_BLOCK_NATIVE_ID, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x13u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00008 */
void test_NvM_SetBlockProtection_BeforeInit_ShouldFail(void)
{
    Std_ReturnType ret = NvM_SetBlockProtection(NVM_TEST_BLOCK_NATIVE_ID, TRUE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x14u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/** @req SWS_NvM_00011 */
void test_NvM_GetErrorStatus_BeforeInit_ShouldFail(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;
    Std_ReturnType ret = NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, &res);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x0Bu, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_NOT_INITIALIZED, mock_lastErrorId);
}

/* ================================================================== */
/* Initialisation                                                    */
/* ================================================================== */

/** @req SWS_NvM_00001 */
void test_NvM_Init_ValidConfig_ShouldSucceed(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;
    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
}

/** @req SWS_NvM_00001 */
void test_NvM_Init_DoubleInit_ShouldRemainSilent(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;
    NvM_Init(&testConfig);
    NvM_Init(&testConfig);
    /* SUT has no E_ALREADY_INITIALIZED check: second init must not report DET */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_PROTECTED_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
}

/* ================================================================== */
/* Parameter validation after init                                    */
/* ================================================================== */

/** @req SWS_NvM_00011 */
void test_NvM_GetErrorStatus_NullPtr_ShouldReportDet(void)
{
    NvM_Init(&testConfig);
    Std_ReturnType ret = NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x0Bu, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_PARAM_POINTER, mock_lastErrorId);
}

/** @req SWS_NvM_00011 */
void test_NvM_GetErrorStatus_MultiBlockId_ShouldReturnOk(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;
    NvM_Init(&testConfig);
    Std_ReturnType ret = NvM_GetErrorStatus(0xFFFFu, &res);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
}

/** @req SWS_NvM_00003 */
void test_NvM_WriteBlock_NullSrcPtr_ShouldReportDet(void)
{
    NvM_Init(&testConfig);
    Std_ReturnType ret = NvM_WriteBlock(NVM_TEST_BLOCK_NATIVE_ID, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x05u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_PARAM_POINTER, mock_lastErrorId);
}

/** @req SWS_NvM_00003 */
void test_NvM_WriteBlock_InvalidBlockId_ShouldReportDet(void)
{
    NvM_Init(&testConfig);
    /* BlockId 0 is reserved, BlockId 200 exceeds NVM_NUM_OF_NVRAM_BLOCKS */
    TEST_ASSERT_EQUAL(E_NOT_OK, NvM_WriteBlock(0u, srcBuf));
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x05u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_PARAM_BLOCK_ID, mock_lastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, NvM_WriteBlock(200u, srcBuf));
    TEST_ASSERT_EQUAL_UINT32(2u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_PARAM_BLOCK_ID, mock_lastErrorId);
}

/** @req SWS_NvM_00003 */
void test_NvM_WriteBlock_WriteProtectedBlock_ShouldReportDet(void)
{
    NvM_RequestResultType res = NVM_REQ_OK;
    NvM_Init(&testConfig);
    Std_ReturnType ret = NvM_WriteBlock(NVM_TEST_BLOCK_PROTECTED_ID, srcBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x05u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_E_WRITE_PROTECTED, mock_lastErrorId);
    /* failed write request must be visible in the per-block error status */
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_PROTECTED_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_NOT_OK, res);
}

/** @req SWS_NvM_00007 */
void test_NvM_SetBlockLockStatus_LockedWrite_ShouldFail(void)
{
    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, NvM_SetBlockLockStatus(NVM_TEST_BLOCK_NATIVE_ID, TRUE));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, NvM_WriteBlock(NVM_TEST_BLOCK_NATIVE_ID, srcBuf));
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x05u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_E_WRITE_PROTECTED, mock_lastErrorId);
}

/** @req SWS_NvM_00005 */
void test_NvM_SetDataIndex_NativeBlock_ShouldReportDet(void)
{
    NvM_Init(&testConfig);
    Std_ReturnType ret = NvM_SetDataIndex(NVM_TEST_BLOCK_NATIVE_ID, 0u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_E_PARAM_BLOCK_TYPE, mock_lastErrorId);
}

/** @req SWS_NvM_00005 */
void test_NvM_SetDataIndex_DatasetBlock_ShouldValidateIndex(void)
{
    NvM_Init(&testConfig);
    /* index within NumberOfDataSets (=2) is accepted */
    TEST_ASSERT_EQUAL(E_OK, NvM_SetDataIndex(NVM_TEST_BLOCK_DATASET_ID, 1u));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    /* index beyond NumberOfDataSets is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, NvM_SetDataIndex(NVM_TEST_BLOCK_DATASET_ID, 2u));
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_E_PARAM_DATA_IDX, mock_lastErrorId);
}

/** @req SWS_NvM_00012 */
void test_NvM_SetRamBlockStatus_AfterInit_ShouldSucceed(void)
{
    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, NvM_SetRamBlockStatus(NVM_TEST_BLOCK_NATIVE_ID, TRUE));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
}

/* ================================================================== */
/* End-to-end job processing via NvM_MainFunction()                   */
/* ================================================================== */

/** @req SWS_NvM_00003 */
/** @req SWS_NvM_00015 */
void test_NvM_WriteBlock_JobFlow_ShouldCompleteOk(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;

    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, NvM_WriteBlock(NVM_TEST_BLOCK_NATIVE_ID, srcBuf));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);

    /* a second request while the job is queued must be rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, NvM_WriteBlock(NVM_TEST_BLOCK_NATIVE_ID, srcBuf));
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x05u, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(NVM_TEST_E_BLOCK_PENDING, mock_lastErrorId);

    /* 1st MainFunction: job is popped from the queue and passed to MemIf */
    NvM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1u, stub_MemIf_Write_calls);
    TEST_ASSERT_EQUAL_UINT8(0u, stub_last_Device);
    TEST_ASSERT_EQUAL_HEX16(0x0001u, stub_last_BlockNumber);
    TEST_ASSERT_TRUE(srcBuf == stub_last_WritePtr);

    /* 2nd MainFunction: MemIf reports IDLE/JOB_OK, NvM completes the job */
    NvM_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls); /* only the E_BLOCK_PENDING above */

    /* job finished: a new write request is accepted again */
    TEST_ASSERT_EQUAL(E_OK, NvM_WriteBlock(NVM_TEST_BLOCK_NATIVE_ID, srcBuf));
}

/** @req SWS_NvM_00002 */
/** @req SWS_NvM_00015 */
void test_NvM_ReadBlock_JobFlow_ShouldCompleteOk(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;

    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, NvM_ReadBlock(NVM_TEST_BLOCK_NATIVE_ID, dstBuf));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);

    NvM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1u, stub_MemIf_Read_calls);
    TEST_ASSERT_EQUAL_UINT8(0u, stub_last_Device);
    TEST_ASSERT_EQUAL_HEX16(0x0001u, stub_last_BlockNumber);
    TEST_ASSERT_EQUAL_HEX16(0u, stub_last_Offset);
    TEST_ASSERT_EQUAL_HEX16(NVM_TEST_BLOCK_LENGTH, stub_last_Length);
    TEST_ASSERT_TRUE(dstBuf == stub_last_ReadPtr);

    NvM_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
}

/** @req SWS_NvM_00013 */
/** @req SWS_NvM_00015 */
void test_NvM_EraseNvBlock_JobFlow_ShouldCompleteOk(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;

    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, NvM_EraseNvBlock(NVM_TEST_BLOCK_NATIVE_ID));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);

    NvM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1u, stub_MemIf_Erase_calls);
    TEST_ASSERT_EQUAL_HEX16(0x0001u, stub_last_BlockNumber);

    NvM_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
}

/** @req SWS_NvM_00014 */
/** @req SWS_NvM_00015 */
void test_NvM_InvalidateNvBlock_JobFlow_ShouldCompleteOk(void)
{
    NvM_RequestResultType res = NVM_REQ_NOT_OK;

    NvM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, NvM_InvalidateNvBlock(NVM_TEST_BLOCK_NATIVE_ID));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);

    NvM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1u, stub_MemIf_Invalidate_calls);
    TEST_ASSERT_EQUAL_HEX16(0x0001u, stub_last_BlockNumber);

    NvM_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, NvM_GetErrorStatus(NVM_TEST_BLOCK_NATIVE_ID, &res));
    TEST_ASSERT_EQUAL(NVM_REQ_OK, res);
}

/** @req SWS_NvM_00015 */
void test_NvM_MainFunction_NoPendingJob_ShouldBeSilent(void)
{
    NvM_Init(&testConfig);
    NvM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_MemIf_Read_calls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_MemIf_Write_calls);
}

/* ================================================================== */
/* Version information                                               */
/* ================================================================== */

/** @req SWS_NvM_00010 */
void test_NvM_GetVersionInfo_ValidPtr_ShouldReturnVersions(void)
{
    Std_VersionInfoType info;
    (void)memset(&info, 0, sizeof(info));
    NvM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_HEX16(NVM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_HEX16(NVM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_HEX8(NVM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_HEX8(NVM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_HEX8(NVM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_NvM_00010 */
void test_NvM_GetVersionInfo_NullPtr_ShouldBeSilent(void)
{
    /* SUT has no NULL check / no DET in NvM_GetVersionInfo */
    NvM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
}

/* ================================================================== */
/* Runner — before-init tests must run first (module is UNINIT)      */
/* ================================================================== */
int main(void)
{
    UNITY_BEGIN();

    /* guard tests that require the module to be uninitialized */
    RUN_TEST(test_NvM_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_NvM_ReadBlock_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_WriteBlock_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_RestoreBlockDefaults_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_EraseNvBlock_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_InvalidateNvBlock_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_SetDataIndex_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_SetBlockLockStatus_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_SetBlockProtection_BeforeInit_ShouldFail);
    RUN_TEST(test_NvM_GetErrorStatus_BeforeInit_ShouldFail);

    /* initialized operation */
    RUN_TEST(test_NvM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_NvM_Init_DoubleInit_ShouldRemainSilent);
    RUN_TEST(test_NvM_GetErrorStatus_NullPtr_ShouldReportDet);
    RUN_TEST(test_NvM_GetErrorStatus_MultiBlockId_ShouldReturnOk);
    RUN_TEST(test_NvM_WriteBlock_NullSrcPtr_ShouldReportDet);
    RUN_TEST(test_NvM_WriteBlock_InvalidBlockId_ShouldReportDet);
    RUN_TEST(test_NvM_WriteBlock_WriteProtectedBlock_ShouldReportDet);
    RUN_TEST(test_NvM_SetBlockLockStatus_LockedWrite_ShouldFail);
    RUN_TEST(test_NvM_SetDataIndex_NativeBlock_ShouldReportDet);
    RUN_TEST(test_NvM_SetDataIndex_DatasetBlock_ShouldValidateIndex);
    RUN_TEST(test_NvM_SetRamBlockStatus_AfterInit_ShouldSucceed);
    RUN_TEST(test_NvM_WriteBlock_JobFlow_ShouldCompleteOk);
    RUN_TEST(test_NvM_ReadBlock_JobFlow_ShouldCompleteOk);
    RUN_TEST(test_NvM_EraseNvBlock_JobFlow_ShouldCompleteOk);
    RUN_TEST(test_NvM_InvalidateNvBlock_JobFlow_ShouldCompleteOk);
    RUN_TEST(test_NvM_MainFunction_NoPendingJob_ShouldBeSilent);
    RUN_TEST(test_NvM_GetVersionInfo_ValidPtr_ShouldReturnVersions);
    RUN_TEST(test_NvM_GetVersionInfo_NullPtr_ShouldBeSilent);

    return UnityEnd();
}

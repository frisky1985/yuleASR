/**
 * @file test_ea_svc.c
 * @brief EaSvc service-façade unit tests.
 * @version 1.0.0
 *
 * @tests src/bsw/ecual/ea/src/Ea.c
 * @tests src/bsw/ecual/ea/include/Ea.h
 *
 * EaSvc is the service-layer façade of the ECUAL EEPROM Abstraction. This
 * project has no separate EaSvc.h/EaSvc.c — the façade responsibilities are
 * implemented by the Ea module (library target ecual_ea). The original
 * SWS_EaSvc_* traceability tags are preserved and mapped onto the real Ea.h
 * public API as follows:
 *   SWS_EaSvc_00001 Init          -> Ea_Init()
 *   SWS_EaSvc_00002 DeInit        -> Ea_SetMode()/state management (Ea has no
 *                                    DeInit; mode handling is the closest
 *                                    runtime service-management operation)
 *   SWS_EaSvc_00003 GetVersionInfo-> Ea_GetVersionInfo()
 *   SWS_EaSvc_00004 MainFunction  -> Ea_MainFunction()
 *   SWS_EaSvc_00005 Read          -> Ea_Read()
 *   SWS_EaSvc_00006 Write         -> Ea_Write()
 *   SWS_EaSvc_00007 Erase         -> Ea_EraseImmediateBlock()
 *   SWS_EaSvc_00008 GetStatus     -> Ea_GetStatus()/Ea_GetJobResult()
 *
 * Behavioural notes (assertions encode the real SUT behaviour):
 *  - Ea_MainFunction() is silent when uninitialized; Ea_GetStatus() reports
 *    EA_E_UNINIT via DET when uninitialized.
 *  - Block N maps to EEPROM address N * EA_MAX_BLOCK_SIZE.
 */

#include "unity.h"
#include "Ea.h"
#include "MemIf.h"
#include "Std_Types.h"
#include <string.h>

/* MemIf_Cfg.h maps the Ea_* names to legacy 2-argument function-like macros;
 * undo that so calls bind to the real Ea.h API. */
#undef Ea_Read
#undef Ea_Write
#undef Ea_Cancel
#undef Ea_GetStatus
#undef Ea_GetJobResult

/* ------------------------------------------------------------------ */
/* Own DET mock: records every Det_ReportError call                    */
/* ------------------------------------------------------------------ */
static uint32 mock_DetCalls;
static uint8  mock_lastApiId;
static uint8  mock_lastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    mock_DetCalls++;
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
extern uint32 stub_Eep_GetJobResult_calls;
extern uint32 stub_Eep_last_Address;
extern uint16 stub_Eep_last_Length;
extern const uint8* stub_Eep_last_WritePtr;
extern MemIf_JobResultType stub_Eep_JobResult;

static uint8 svcBuf[32];

/* ------------------------------------------------------------------ */
/* Own test configuration                                             */
/* ------------------------------------------------------------------ */
static Ea_BlockConfigType svcBlockConfig[1];
static Ea_ConfigType svcConfig;

static void init_config(void)
{
    (void)memset(svcBlockConfig, 0, sizeof(svcBlockConfig));
    (void)memset(&svcConfig, 0, sizeof(svcConfig));

    svcBlockConfig[0].BlockId = 1u;
    svcBlockConfig[0].BlockSize = 16u;

    svcConfig.BlockConfig = svcBlockConfig;
    svcConfig.NumBlocks = 1u;
}

void setUp(void)
{
    mock_DetCalls = 0u;
    mock_lastApiId = 0u;
    mock_lastErrorId = 0u;

    stub_Eep_Read_calls = 0u;
    stub_Eep_Write_calls = 0u;
    stub_Eep_Erase_calls = 0u;
    stub_Eep_GetJobResult_calls = 0u;
    stub_Eep_last_Address = 0u;
    stub_Eep_last_Length = 0u;
    stub_Eep_last_WritePtr = (const uint8*)0;
    stub_Eep_JobResult = MEMIF_JOB_OK;

    (void)memset(svcBuf, 0x5A, sizeof(svcBuf));
    init_config();
}

void tearDown(void)
{
}

/* ================================================================== */
/* SWS_EaSvc_00001 — Init                                            */
/* ================================================================== */

/** @req SWS_EaSvc_00001 */
void test_EaSvc_Init_NullPtr_ShouldReportDet(void)
{
    Ea_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_INIT, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_CFG, mock_lastErrorId);
}

/** @req SWS_EaSvc_00001 */
void test_EaSvc_Init_ValidConfig_ShouldSucceed(void)
{
    Ea_Init(&svcConfig);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/** @req SWS_EaSvc_00001 */
void test_EaSvc_Init_DoubleInit_ShouldSucceed(void)
{
    Ea_Init(&svcConfig);
    Ea_Init(&svcConfig);
    /* no double-init DET check in the SUT */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
}

/* ================================================================== */
/* SWS_EaSvc_00002 — DeInit / service mode management                */
/* ================================================================== */

/** @req SWS_EaSvc_00002 */
void test_EaSvc_SetMode_Uninit_ShouldReportError(void)
{
    /* Ea provides no DeInit; mode management is the closest service operation */
    Ea_SetMode(EA_MODE_SLOW);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_SETMODE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_EaSvc_00002 */
void test_EaSvc_SetMode_ValidCall_ShouldSucceed(void)
{
    Ea_Init(&svcConfig);
    Ea_SetMode(EA_MODE_SLOW);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
}

/* ================================================================== */
/* SWS_EaSvc_00003 — GetVersionInfo                                  */
/* ================================================================== */

/** @req SWS_EaSvc_00003 */
void test_EaSvc_GetVersionInfo_NullPtr_ShouldReportError(void)
{
    Ea_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_GETVERSIONINFO, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_DATA_PTR, mock_lastErrorId);
}

/** @req SWS_EaSvc_00003 */
void test_EaSvc_GetVersionInfo_ValidPtr_ShouldSucceed(void)
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
/* SWS_EaSvc_00004 — MainFunction                                    */
/* ================================================================== */

/** @req SWS_EaSvc_00004 */
void test_EaSvc_MainFunction_Uninit_ShouldBeSilent(void)
{
    /* SUT returns silently when uninitialized (no DET) */
    Ea_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_Eep_GetJobResult_calls);
}

/** @req SWS_EaSvc_00004 */
void test_EaSvc_MainFunction_ValidCall_ShouldSucceed(void)
{
    Ea_Init(&svcConfig);
    Ea_MainFunction(); /* no pending job: driver must not be polled */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, stub_Eep_GetJobResult_calls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
}

/* ================================================================== */
/* SWS_EaSvc_00005 — Read (end-to-end via MainFunction)              */
/* ================================================================== */

/** @req SWS_EaSvc_00005 */
void test_EaSvc_Read_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_Read(1u, 0u, svcBuf, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_READ, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_EaSvc_00005 */
void test_EaSvc_Read_NullPtr_ShouldReportError(void)
{
    Ea_Init(&svcConfig);
    Std_ReturnType ret = Ea_Read(1u, 0u, NULL_PTR, 4u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_READ, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_DATA_PTR, mock_lastErrorId);
}

/** @req SWS_EaSvc_00005 */
void test_EaSvc_Read_ValidCall_ShouldCompleteJob(void)
{
    Ea_Init(&svcConfig);
    TEST_ASSERT_EQUAL(E_OK, Ea_Read(1u, 0u, svcBuf, 4u));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Read_calls);
    TEST_ASSERT_EQUAL_UINT32(1u * (uint32)EA_MAX_BLOCK_SIZE, stub_Eep_last_Address);
    TEST_ASSERT_EQUAL_UINT16(4u, stub_Eep_last_Length);
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());

    /* driver still busy: MainFunction keeps the job pending */
    stub_Eep_JobResult = MEMIF_JOB_PENDING;
    Ea_MainFunction();
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_PENDING, Ea_GetJobResult());

    /* driver finished: MainFunction completes the job */
    stub_Eep_JobResult = MEMIF_JOB_OK;
    Ea_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(2u, stub_Eep_GetJobResult_calls);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
}

/* ================================================================== */
/* SWS_EaSvc_00006 — Write (end-to-end via MainFunction)             */
/* ================================================================== */

/** @req SWS_EaSvc_00006 */
void test_EaSvc_Write_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_Write(1u, svcBuf);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_WRITE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_EaSvc_00006 */
void test_EaSvc_Write_NullPtr_ShouldReportError(void)
{
    Ea_Init(&svcConfig);
    Std_ReturnType ret = Ea_Write(1u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_WRITE, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_DATA_PTR, mock_lastErrorId);
}

/** @req SWS_EaSvc_00006 */
void test_EaSvc_Write_ValidCall_ShouldCompleteJob(void)
{
    Ea_Init(&svcConfig);
    TEST_ASSERT_EQUAL(E_OK, Ea_Write(1u, svcBuf));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Write_calls);
    TEST_ASSERT_EQUAL_UINT32(1u * (uint32)EA_MAX_BLOCK_SIZE, stub_Eep_last_Address);
    /* Ea writes the full configured block size */
    TEST_ASSERT_EQUAL_UINT16(16u, stub_Eep_last_Length);
    TEST_ASSERT_TRUE(svcBuf == stub_Eep_last_WritePtr);
    TEST_ASSERT_EQUAL(EA_BUSY, Ea_GetStatus());

    Ea_MainFunction();
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/* ================================================================== */
/* SWS_EaSvc_00007 — Erase                                            */
/* ================================================================== */

/** @req SWS_EaSvc_00007 */
void test_EaSvc_Erase_Uninit_ShouldReportError(void)
{
    Std_ReturnType ret = Ea_EraseImmediateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_ERASEIMMEDIATEBLOCK, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_EaSvc_00007 */
void test_EaSvc_Erase_InvalidBlock_ShouldReportError(void)
{
    Ea_Init(&svcConfig);
    Std_ReturnType ret = Ea_EraseImmediateBlock((Ea_BlockIdType)EA_NUM_BLOCKS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_ERASEIMMEDIATEBLOCK, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_INVALID_BLOCK_NO, mock_lastErrorId);
}

/** @req SWS_EaSvc_00007 */
void test_EaSvc_Erase_ValidCall_ShouldSucceed(void)
{
    Ea_Init(&svcConfig);
    TEST_ASSERT_EQUAL(E_OK, Ea_EraseImmediateBlock(1u));
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT32(1u, stub_Eep_Erase_calls);
    TEST_ASSERT_EQUAL_UINT32(1u * (uint32)EA_MAX_BLOCK_SIZE, stub_Eep_last_Address);
    TEST_ASSERT_EQUAL_UINT16(16u, stub_Eep_last_Length);
    /* erase cycle count is incremented as soon as the driver accepts the job */
    TEST_ASSERT_EQUAL_UINT32(1u, Ea_GetEraseCycleCount());
    Ea_MainFunction();
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
}

/* ================================================================== */
/* SWS_EaSvc_00008 — GetStatus                                        */
/* ================================================================== */

/** @req SWS_EaSvc_00008 */
void test_EaSvc_GetStatus_Uninit_ShouldReportDet(void)
{
    /* SUT reports EA_E_UNINIT via DET when uninitialized and returns EA_IDLE */
    Ea_StatusType status = Ea_GetStatus();
    TEST_ASSERT_EQUAL(EA_IDLE, status);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(EA_SID_GETSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL_HEX8(EA_E_UNINIT, mock_lastErrorId);
}

/** @req SWS_EaSvc_00008 */
void test_EaSvc_GetStatus_ValidCall_ShouldReturnStatus(void)
{
    Ea_Init(&svcConfig);
    TEST_ASSERT_EQUAL(EA_IDLE, Ea_GetStatus());
    TEST_ASSERT_EQUAL(EA_JOB_OK, Ea_GetJobResult());
    TEST_ASSERT_EQUAL_UINT32(0u, mock_DetCalls);
}

/* ================================================================== */
/* Runner — uninit tests must run first (module flag still FALSE)     */
/* ================================================================== */
int main(void)
{
    UNITY_BEGIN();

    /* guard tests that require the module to be uninitialized */
    RUN_TEST(test_EaSvc_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_EaSvc_SetMode_Uninit_ShouldReportError);
    RUN_TEST(test_EaSvc_MainFunction_Uninit_ShouldBeSilent);
    RUN_TEST(test_EaSvc_Read_Uninit_ShouldReportError);
    RUN_TEST(test_EaSvc_Write_Uninit_ShouldReportError);
    RUN_TEST(test_EaSvc_Erase_Uninit_ShouldReportError);
    RUN_TEST(test_EaSvc_GetStatus_Uninit_ShouldReportDet);

    /* initialized operation */
    RUN_TEST(test_EaSvc_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_EaSvc_Init_DoubleInit_ShouldSucceed);
    RUN_TEST(test_EaSvc_SetMode_ValidCall_ShouldSucceed);
    RUN_TEST(test_EaSvc_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_EaSvc_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_EaSvc_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_EaSvc_Read_NullPtr_ShouldReportError);
    RUN_TEST(test_EaSvc_Read_ValidCall_ShouldCompleteJob);
    RUN_TEST(test_EaSvc_Write_NullPtr_ShouldReportError);
    RUN_TEST(test_EaSvc_Write_ValidCall_ShouldCompleteJob);
    RUN_TEST(test_EaSvc_Erase_InvalidBlock_ShouldReportError);
    RUN_TEST(test_EaSvc_Erase_ValidCall_ShouldSucceed);
    RUN_TEST(test_EaSvc_GetStatus_ValidCall_ShouldReturnStatus);

    return UnityEnd();
}

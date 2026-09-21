/**
 * @file test_mem.c
 * @brief Mem Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/mem/src/Mem.c  @tests src/bsw/services/mem/include/Mem.h

#include "unity.h"
#include "Mem.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Runtime error mock (Mem_CheckIntegrity reports runtime errors via Det) */
static uint8 mock_DetRuntimeCallCount = 0U;

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    mock_DetRuntimeCallCount++;
    return E_OK;
}

/* Test config: single pool-less configuration is sufficient — Mem_Init()
 * ignores the pool array and uses its internal static pools. */
static const Mem_ConfigType testConfig =
{
    NULL_PTR,           /* pools (not used by SUT) */
    MEM_NUM_POOLS,      /* numPools */
    30U,                /* defragThreshold */
    TRUE,               /* enableChecksum */
    TRUE                /* enableMonitoring */
};

void setUp(void) {
    mock_Det_Reset();
    mock_DetRuntimeCallCount = 0U;
    /* Force a clean module state for every test (SUT has no re-init guard
     * bypass; Mem_Initialized is reset only via DeInit after a valid Init) */
    Mem_Init(&testConfig);
    Mem_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
    Mem_DeInit();
}


/** @req SWS_Mem_00001 */
void test_Mem_Init_NullPtr_ShouldReportError(void) {
    Mem_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Module stays uninitialized */
    TEST_ASSERT_EQUAL_INT(MEM_UNINIT, Mem_GetStatus());
}

/** @req SWS_Mem_00001 */
void test_Mem_Init_ValidConfig_ShouldSucceed(void) {
    Mem_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_INT(MEM_IDLE, Mem_GetStatus());
    TEST_ASSERT_EQUAL_PTR(&testConfig, Mem_ConfigPtr);
    /* The internal static pools must be usable right away */
    Mem_HandleType h = Mem_Allocate(16U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);
}

/** @req SWS_Mem_00001 */
void test_Mem_Init_DoubleInit_ShouldReportError(void) {
    Mem_Init(&testConfig);
    Mem_Init(&testConfig);
    /* SUT: second Init reports MEM_E_ALREADY_INITIALIZED and returns */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
    /* Module remains initialized and operational */
    TEST_ASSERT_EQUAL_INT(MEM_IDLE, Mem_GetStatus());
    Mem_HandleType h = Mem_Allocate(16U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);
}

/** @req SWS_Mem_00002 */
void test_Mem_DeInit_Uninit_ShouldReportError(void) {
    Mem_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mem_00002 */
void test_Mem_DeInit_ValidCall_ShouldResetState(void) {
    Mem_Init(&testConfig);
    Mem_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_INT(MEM_UNINIT, Mem_GetStatus());
    TEST_ASSERT_NULL(Mem_ConfigPtr);
    /* Post-DeInit allocations are rejected with MEM_E_UNINIT */
    Mem_HandleType h = Mem_Allocate(16U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mem_00003 */
void test_Mem_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Mem_Init(&testConfig);
    Mem_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Mem_00003 */
void test_Mem_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType vi;
    Mem_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(MEM_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(MEM_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(MEM_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(MEM_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(MEM_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_Mem_00004 */
void test_Mem_MainFunction_Uninit_ShouldReportError(void) {
    Mem_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_MAINFUNCTION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mem_00004 */
void test_Mem_MainFunction_ValidCall_ShouldSucceed(void) {
    Mem_Init(&testConfig);
    Mem_MainFunction();
    /* Monitoring enabled (MEM_ENABLE_MONITORING == STD_ON): MainFunction
     * runs the integrity check without corrupting pools or reporting DET */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetRuntimeCallCount);
    TEST_ASSERT_EQUAL_INT(MEM_IDLE, Mem_GetStatus());
    TEST_ASSERT_EQUAL(E_OK, Mem_CheckIntegrity());
}

/** @req SWS_Mem_00005 */
void test_Mem_Allocate_Uninit_ShouldReportError(void) {
    Mem_HandleType h = Mem_Allocate(16U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_ALLOCATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mem_00005 */
void test_Mem_Allocate_InvalidParams_ShouldReportError(void) {
    Mem_Init(&testConfig);
    /* Size 0 */
    Mem_HandleType h = Mem_Allocate(0U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_ALLOCATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_PARAM_SIZE, mock_DetLastErrorId);
    /* Alignment not power of 2 / below minimum */
    h = Mem_Allocate(16U, 3U);
    TEST_ASSERT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL_UINT32(2U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_PARAM_ALIGN, mock_DetLastErrorId);
}

/** @req SWS_Mem_00005 */
void test_Mem_Allocate_ValidCall_ShouldReturnUsablePointer(void) {
    Mem_Init(&testConfig);
    Mem_HandleType h = Mem_Allocate(32U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    uint8* p = (uint8*)Mem_GetPointer(h);
    TEST_ASSERT_NOT_NULL(p);
    /* Read/write access through the returned pointer must be safe */
    p[0] = 0xA5U;
    p[31] = 0x5AU;
    TEST_ASSERT_EQUAL_UINT8(0xA5U, p[0]);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, p[31]);
    /* Handles are unique: second allocation differs from the first */
    Mem_HandleType h2 = Mem_Allocate(32U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(h, h2);
    TEST_ASSERT_EQUAL(E_OK, Mem_Free(h));
    TEST_ASSERT_EQUAL(E_OK, Mem_Free(h2));
}

/** @req SWS_Mem_00006 */
void test_Mem_Free_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = Mem_Free(1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_FREE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mem_00006 */
void test_Mem_Free_InvalidHandle_ShouldReportError(void) {
    Mem_Init(&testConfig);
    Std_ReturnType ret = Mem_Free(MEM_INVALID_HANDLE);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_FREE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_INVALID_HANDLE, mock_DetLastErrorId);
}

/** @req SWS_Mem_00006 */
void test_Mem_Free_ValidCall_ShouldReleaseBlock(void) {
    Mem_Init(&testConfig);
    Mem_HandleType h = Mem_Allocate(64U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL(E_OK, Mem_Free(h));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Freed handle is no longer resolvable */
    TEST_ASSERT_NULL(Mem_GetPointer(h));
    /* Double free of the same handle is rejected (block is free again) */
    TEST_ASSERT_EQUAL(E_NOT_OK, Mem_Free(h));
    TEST_ASSERT_EQUAL_UINT8(MEM_E_INVALID_HANDLE, mock_DetLastErrorId);
}

/** @req SWS_Mem_00007 */
void test_Mem_Reallocate_Uninit_ShouldReportError(void) {
    Mem_HandleType h = Mem_Reallocate(1U, 32U);
    TEST_ASSERT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_REALLOCATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mem_00007 */
void test_Mem_Reallocate_ValidCall_ShouldPreserveData(void) {
    Mem_Init(&testConfig);
    Mem_HandleType h = Mem_Allocate(32U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);
    uint8* p = (uint8*)Mem_GetPointer(h);
    TEST_ASSERT_NOT_NULL(p);
    p[0] = 0x11U;
    p[1] = 0x22U;

    Mem_HandleType h2 = Mem_Reallocate(h, 128U);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h2);
    TEST_ASSERT_NOT_EQUAL(h, h2);
    uint8* p2 = (uint8*)Mem_GetPointer(h2);
    TEST_ASSERT_NOT_NULL(p2);
    /* Old content must be copied to the new block */
    TEST_ASSERT_EQUAL_UINT8(0x11U, p2[0]);
    TEST_ASSERT_EQUAL_UINT8(0x22U, p2[1]);
    /* Old block was freed during reallocation */
    TEST_ASSERT_NULL(Mem_GetPointer(h));
}

/** @req SWS_Mem_00008 */
void test_Mem_GetStatus_Uninit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL_INT(MEM_UNINIT, Mem_GetStatus());
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_Mem_00008 */
void test_Mem_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Mem_Init(&testConfig);
    TEST_ASSERT_EQUAL_INT(MEM_IDLE, Mem_GetStatus());
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    Mem_DeInit();
    TEST_ASSERT_EQUAL_INT(MEM_UNINIT, Mem_GetStatus());
}

/** @req SWS_Mem_00008 */
void test_Mem_GetMemInfo_InvalidPool_ShouldReportError(void) {
    Mem_Init(&testConfig);
    Mem_InfoType info;
    Std_ReturnType ret = Mem_GetMemInfo(MEM_NUM_POOLS, &info);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MEM_SID_GETMEMINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MEM_E_PARAM_SIZE, mock_DetLastErrorId);
}

/** @req SWS_Mem_00008 */
void test_Mem_GetMemInfo_ValidCall_ShouldReflectAllocation(void) {
    Mem_Init(&testConfig);
    Mem_InfoType info;
    TEST_ASSERT_EQUAL(E_OK, Mem_GetMemInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT32(MEM_FAST_POOL_SIZE, info.totalSize);

    Mem_HandleType h = Mem_Allocate(64U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);

    Mem_InfoType infoAfter;
    TEST_ASSERT_EQUAL(E_OK, Mem_GetMemInfo(0U, &infoAfter));
    /* usedSize grows by the aligned header+payload block size */
    TEST_ASSERT_TRUE(infoAfter.usedSize > info.usedSize);
    TEST_ASSERT_TRUE(infoAfter.freeSize < info.freeSize);
    TEST_ASSERT_EQUAL(E_OK, Mem_Free(h));
}

/** @req SWS_Mem_00007 */
void test_Mem_CheckIntegrity_ValidState_ShouldReturnOk(void) {
    Mem_Init(&testConfig);
    Mem_HandleType h = Mem_Allocate(48U, MEM_DEFAULT_ALIGNMENT);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, h);
    TEST_ASSERT_EQUAL(E_OK, Mem_CheckIntegrity());
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetRuntimeCallCount);
    TEST_ASSERT_EQUAL(E_OK, Mem_Free(h));
}

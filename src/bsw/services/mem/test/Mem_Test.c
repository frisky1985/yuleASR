/*==================================================================================================
 *                                      MEMORY SERVICE (Mem)
 *==================================================================================================
 * FILENAME: Mem_Test.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Memory Service module
 *==================================================================================================
 */

#include "unity.h"
#include "Mem.h"
#include "Mem_Cfg.h"
#include "Det.h"
#include "mock_Det.h"
#include "mock_SchM_Mem.h"

/*==================================================================================================
 *                                    TEST SETUP
 *==================================================================================================*/
void setUp(void)
{
    /* Reset module state before each test */
    Mem_DeInit();
}

void tearDown(void)
{
    /* Cleanup after each test */
    Mem_DeInit();
}

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
static const Mem_PoolConfigType TestPoolConfigs[MEM_NUM_POOLS] = {
    { NULL, MEM_FAST_POOL_SIZE, MEM_MIN_BLOCK_SIZE, MEM_FAST_POOL_MAX_BLOCK, 4u, MEM_ALLOC_FIRST_FIT },
    { NULL, MEM_STANDARD_POOL_SIZE, MEM_MIN_BLOCK_SIZE, MEM_STANDARD_POOL_MAX_BLOCK, 4u, MEM_ALLOC_BEST_FIT },
    { NULL, MEM_LARGE_POOL_SIZE, MEM_MIN_BLOCK_SIZE, MEM_LARGE_POOL_MAX_BLOCK, 8u, MEM_ALLOC_FIRST_FIT }
};

static const Mem_ConfigType TestConfig = {
    TestPoolConfigs,
    MEM_NUM_POOLS,
    MEM_DEFRAG_THRESHOLD,
    (MEM_ENABLE_CHECKSUM == STD_ON),
    (MEM_ENABLE_MONITORING == STD_ON)
};

/*==================================================================================================
 *                                    TEST CASES - Init/DeInit
 *==================================================================================================*/
void test_Mem_Init_ShouldInitializeModule(void)
{
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Expect();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Expect();
    
    Mem_Init(&TestConfig);
    
    TEST_ASSERT_EQUAL(MEM_IDLE, Mem_GetStatus());
}

void test_Mem_Init_ShouldReportError_WhenAlreadyInitialized(void)
{
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_INIT, MEM_E_ALREADY_INITIALIZED, E_OK);
    
    Mem_Init(&TestConfig);
}

void test_Mem_Init_ShouldReportError_WhenConfigNull(void)
{
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_INIT, MEM_E_PARAM_POINTER, E_OK);
    
    Mem_Init(NULL);
    
    TEST_ASSERT_EQUAL(MEM_UNINIT, Mem_GetStatus());
}

void test_Mem_DeInit_ShouldDeinitializeModule(void)
{
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    Mem_DeInit();
    
    TEST_ASSERT_EQUAL(MEM_UNINIT, Mem_GetStatus());
}

void test_Mem_DeInit_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_DEINIT, MEM_E_UNINIT, E_OK);
    
    Mem_DeInit();
}

/*==================================================================================================
 *                                    TEST CASES - Allocate/Free
 *==================================================================================================*/
void test_Mem_Allocate_ShouldReturnValidHandle(void)
{
    Mem_HandleType handle;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    handle = Mem_Allocate(100, 4);
    
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, handle);
}

void test_Mem_Allocate_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_ALLOCATE, MEM_E_UNINIT, E_OK);
    
    Mem_Allocate(100, 4);
}

void test_Mem_Allocate_ShouldReportError_WhenSizeZero(void)
{
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_ALLOCATE, MEM_E_PARAM_SIZE, E_OK);
    
    Mem_Allocate(0, 4);
}

void test_Mem_Allocate_ShouldReportError_WhenAlignmentInvalid(void)
{
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_ALLOCATE, MEM_E_PARAM_ALIGN, E_OK);
    
    Mem_Allocate(100, 3);  /* Not power of 2 */
}

void test_Mem_Free_ShouldReturnOK_WhenValidHandle(void)
{
    Mem_HandleType handle;
    Std_ReturnType result;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    handle = Mem_Allocate(100, 4);
    
    result = Mem_Free(handle);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Mem_Free_ShouldReturnNotOK_WhenInvalidHandle(void)
{
    Std_ReturnType result;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    result = Mem_Free(MEM_INVALID_HANDLE);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_Mem_Free_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_FREE, MEM_E_UNINIT, E_OK);
    
    Mem_Free(1);
}

/*==================================================================================================
 *                                    TEST CASES - Reallocate
 *==================================================================================================*/
void test_Mem_Reallocate_ShouldReturnNewHandle(void)
{
    Mem_HandleType oldHandle, newHandle;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    oldHandle = Mem_Allocate(100, 4);
    
    newHandle = Mem_Reallocate(oldHandle, 200);
    
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, newHandle);
    TEST_ASSERT_NOT_EQUAL(oldHandle, newHandle);
}

void test_Mem_Reallocate_ShouldCopyData(void)
{
    Mem_HandleType oldHandle, newHandle;
    uint8* oldPtr;
    uint8* newPtr;
    uint8 i;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    oldHandle = Mem_Allocate(100, 4);
    oldPtr = (uint8*)Mem_GetPointer(oldHandle);
    
    /* Write test data */
    for (i = 0; i < 100; i++) {
        oldPtr[i] = i;
    }
    
    newHandle = Mem_Reallocate(oldHandle, 200);
    newPtr = (uint8*)Mem_GetPointer(newHandle);
    
    /* Verify data copied */
    for (i = 0; i < 100; i++) {
        TEST_ASSERT_EQUAL(i, newPtr[i]);
    }
}

/*==================================================================================================
 *                                    TEST CASES - GetPointer
 *==================================================================================================*/
void test_Mem_GetPointer_ShouldReturnValidPointer(void)
{
    Mem_HandleType handle;
    void* ptr;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    handle = Mem_Allocate(100, 4);
    
    ptr = Mem_GetPointer(handle);
    
    TEST_ASSERT_NOT_NULL(ptr);
}

void test_Mem_GetPointer_ShouldReturnNull_WhenInvalidHandle(void)
{
    void* ptr;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    ptr = Mem_GetPointer(MEM_INVALID_HANDLE);
    
    TEST_ASSERT_NULL(ptr);
}

/*==================================================================================================
 *                                    TEST CASES - GetMemInfo
 *==================================================================================================*/
void test_Mem_GetMemInfo_ShouldReturnInfo(void)
{
    Mem_InfoType info;
    Std_ReturnType result;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    result = Mem_GetMemInfo(0, &info);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(MEM_FAST_POOL_SIZE, info.totalSize);
    TEST_ASSERT_TRUE(info.freeSize > 0);
}

void test_Mem_GetMemInfo_ShouldReportError_WhenNullPointer(void)
{
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_GETMEMINFO, MEM_E_PARAM_POINTER, E_OK);
    
    Mem_GetMemInfo(0, NULL);
}

/*==================================================================================================
 *                                    TEST CASES - Version Info
 *==================================================================================================*/
#if (MEM_VERSION_INFO_API == STD_ON)
void test_Mem_GetVersionInfo_ShouldReturnVersion(void)
{
    Std_VersionInfoType versionInfo;
    
    Mem_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT_EQUAL(MEM_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(MEM_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(MEM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(MEM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(MEM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

void test_Mem_GetVersionInfo_ShouldReportError_WhenNullPointer(void)
{
    Det_ReportError_ExpectAndReturn(MEM_MODULE_ID, MEM_INSTANCE_ID, MEM_SID_GETVERSIONINFO, MEM_E_PARAM_POINTER, E_OK);
    
    Mem_GetVersionInfo(NULL);
}
#endif

/*==================================================================================================
 *                                    TEST CASES - CheckIntegrity
 *==================================================================================================*/
void test_Mem_CheckIntegrity_ShouldReturnOK_WhenValid(void)
{
    Std_ReturnType result;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    result = Mem_CheckIntegrity();
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/*==================================================================================================
 *                                    TEST CASES - Multiple Allocations
 *==================================================================================================*/
void test_Mem_MultipleAllocations_ShouldWork(void)
{
    Mem_HandleType handles[10];
    uint8 i;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    /* Allocate multiple blocks */
    for (i = 0; i < 10; i++) {
        handles[i] = Mem_Allocate(64, 4);
        TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, handles[i]);
    }
    
    /* Free every other block */
    for (i = 0; i < 10; i += 2) {
        TEST_ASSERT_EQUAL(E_OK, Mem_Free(handles[i]));
    }
    
    /* Free remaining blocks */
    for (i = 1; i < 10; i += 2) {
        TEST_ASSERT_EQUAL(E_OK, Mem_Free(handles[i]));
    }
}

void test_Mem_AllocationInDifferentPools_ShouldWork(void)
{
    Mem_HandleType smallHandle, mediumHandle, largeHandle;
    
    SchM_Enter_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_Mem_MEM_EXCLUSIVE_AREA_0_Ignore();
    
    Mem_Init(&TestConfig);
    
    /* Allocate in different pools */
    smallHandle = Mem_Allocate(64, 4);       /* Fast pool */
    mediumHandle = Mem_Allocate(512, 4);     /* Standard pool */
    largeHandle = Mem_Allocate(4096, 8);     /* Large pool */
    
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, smallHandle);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, mediumHandle);
    TEST_ASSERT_NOT_EQUAL(MEM_INVALID_HANDLE, largeHandle);
    
    /* Verify all handles are unique */
    TEST_ASSERT_TRUE(smallHandle != mediumHandle);
    TEST_ASSERT_TRUE(smallHandle != largeHandle);
    TEST_ASSERT_TRUE(mediumHandle != largeHandle);
    
    /* Cleanup */
    Mem_Free(smallHandle);
    Mem_Free(mediumHandle);
    Mem_Free(largeHandle);
}

/*==================================================================================================
 *                                    TEST RUNNER
 *==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();
    
    /* Init/DeInit tests */
    RUN_TEST(test_Mem_Init_ShouldInitializeModule);
    RUN_TEST(test_Mem_Init_ShouldReportError_WhenAlreadyInitialized);
    RUN_TEST(test_Mem_Init_ShouldReportError_WhenConfigNull);
    RUN_TEST(test_Mem_DeInit_ShouldDeinitializeModule);
    RUN_TEST(test_Mem_DeInit_ShouldReportError_WhenNotInitialized);
    
    /* Allocate/Free tests */
    RUN_TEST(test_Mem_Allocate_ShouldReturnValidHandle);
    RUN_TEST(test_Mem_Allocate_ShouldReportError_WhenNotInitialized);
    RUN_TEST(test_Mem_Allocate_ShouldReportError_WhenSizeZero);
    RUN_TEST(test_Mem_Allocate_ShouldReportError_WhenAlignmentInvalid);
    RUN_TEST(test_Mem_Free_ShouldReturnOK_WhenValidHandle);
    RUN_TEST(test_Mem_Free_ShouldReturnNotOK_WhenInvalidHandle);
    RUN_TEST(test_Mem_Free_ShouldReportError_WhenNotInitialized);
    
    /* Reallocate tests */
    RUN_TEST(test_Mem_Reallocate_ShouldReturnNewHandle);
    RUN_TEST(test_Mem_Reallocate_ShouldCopyData);
    
    /* GetPointer tests */
    RUN_TEST(test_Mem_GetPointer_ShouldReturnValidPointer);
    RUN_TEST(test_Mem_GetPointer_ShouldReturnNull_WhenInvalidHandle);
    
    /* GetMemInfo tests */
    RUN_TEST(test_Mem_GetMemInfo_ShouldReturnInfo);
    RUN_TEST(test_Mem_GetMemInfo_ShouldReportError_WhenNullPointer);
    
    /* Version Info tests */
#if (MEM_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_Mem_GetVersionInfo_ShouldReturnVersion);
    RUN_TEST(test_Mem_GetVersionInfo_ShouldReportError_WhenNullPointer);
#endif
    
    /* Integrity tests */
    RUN_TEST(test_Mem_CheckIntegrity_ShouldReturnOK_WhenValid);
    
    /* Multiple allocation tests */
    RUN_TEST(test_Mem_MultipleAllocations_ShouldWork);
    RUN_TEST(test_Mem_AllocationInDifferentPools_ShouldWork);
    
    return UNITY_END();
}

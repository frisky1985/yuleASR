/**
 * @file test_mem_runner.c
 * @brief Unity runner for the substantiated Mem unit tests.
 *
 * The test file only provides setUp/tearDown and the test_Mem_* functions, so
 * this runner supplies main() and the RUN_TEST list. setUp() forces a clean
 * module state (Init + DeInit) before every test, so the tests are order
 * independent and can run in declaration order.
 *
 * @tests src/bsw/services/mem/src/Mem.c
 */
#include "unity.h"

extern void setUp(void);
extern void tearDown(void);

/* Lifecycle & version */
extern void test_Mem_Init_NullPtr_ShouldReportError(void);
extern void test_Mem_Init_ValidConfig_ShouldSucceed(void);
extern void test_Mem_Init_DoubleInit_ShouldReportError(void);
extern void test_Mem_DeInit_Uninit_ShouldReportError(void);
extern void test_Mem_DeInit_ValidCall_ShouldResetState(void);
extern void test_Mem_GetVersionInfo_NullPtr_ShouldReportError(void);
extern void test_Mem_GetVersionInfo_ValidPtr_ShouldReturnVersion(void);
extern void test_Mem_MainFunction_Uninit_ShouldReportError(void);
extern void test_Mem_MainFunction_ValidCall_ShouldSucceed(void);

/* Allocation */
extern void test_Mem_Allocate_Uninit_ShouldReportError(void);
extern void test_Mem_Allocate_InvalidParams_ShouldReportError(void);
extern void test_Mem_Allocate_ValidCall_ShouldReturnUsablePointer(void);
extern void test_Mem_Free_Uninit_ShouldReportError(void);
extern void test_Mem_Free_InvalidHandle_ShouldReportError(void);
extern void test_Mem_Free_ValidCall_ShouldReleaseBlock(void);
extern void test_Mem_Reallocate_Uninit_ShouldReportError(void);
extern void test_Mem_Reallocate_ValidCall_ShouldPreserveData(void);

/* Status, pool info & integrity */
extern void test_Mem_GetStatus_Uninit_ShouldReturnUninit(void);
extern void test_Mem_GetStatus_ValidCall_ShouldReturnStatus(void);
extern void test_Mem_GetMemInfo_InvalidPool_ShouldReportError(void);
extern void test_Mem_GetMemInfo_ValidCall_ShouldReflectAllocation(void);
extern void test_Mem_CheckIntegrity_ValidState_ShouldReturnOk(void);

/* New branch coverage tests */
extern void test_Mem_Allocate_SmallBlock_ShouldSplitFreeRegion(void);
extern void test_Mem_Free_AdjacentBlocks_ShouldMergeFreeBlocks(void);
extern void test_Mem_Allocate_StandardPool_ShouldAllocateFromStandardPool(void);
extern void test_Mem_Allocate_LargePool_ShouldAllocateFromLargePool(void);
extern void test_Mem_Reallocate_InvalidHandle_ShouldReportError(void);
extern void test_Mem_Reallocate_NewSizeZero_ShouldFreeAndReturnInvalid(void);
extern void test_Mem_GetPointer_InvalidHandle_ShouldReturnNull(void);
extern void test_Mem_GetMemInfo_NullPtr_ShouldReportError(void);
extern void test_Mem_GetMemInfo_Uninit_ShouldReportError(void);
extern void test_Mem_CheckIntegrity_Uninit_ShouldReportError(void);
extern void test_Mem_Defragment_Uninit_ShouldReportError(void);
extern void test_Mem_Defragment_InvalidPool_ShouldReportError(void);

int main(void) {
    UNITY_BEGIN();

    /* Lifecycle & version */
    RUN_TEST(test_Mem_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_Mem_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Mem_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_Mem_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_DeInit_ValidCall_ShouldResetState);
    RUN_TEST(test_Mem_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Mem_GetVersionInfo_ValidPtr_ShouldReturnVersion);
    RUN_TEST(test_Mem_MainFunction_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_MainFunction_ValidCall_ShouldSucceed);

    /* Allocation */
    RUN_TEST(test_Mem_Allocate_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_Allocate_InvalidParams_ShouldReportError);
    RUN_TEST(test_Mem_Allocate_ValidCall_ShouldReturnUsablePointer);
    RUN_TEST(test_Mem_Free_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_Free_InvalidHandle_ShouldReportError);
    RUN_TEST(test_Mem_Free_ValidCall_ShouldReleaseBlock);
    RUN_TEST(test_Mem_Reallocate_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_Reallocate_ValidCall_ShouldPreserveData);

    /* Status, pool info & integrity */
    RUN_TEST(test_Mem_GetStatus_Uninit_ShouldReturnUninit);
    RUN_TEST(test_Mem_GetStatus_ValidCall_ShouldReturnStatus);
    RUN_TEST(test_Mem_GetMemInfo_InvalidPool_ShouldReportError);
    RUN_TEST(test_Mem_GetMemInfo_ValidCall_ShouldReflectAllocation);
    RUN_TEST(test_Mem_CheckIntegrity_ValidState_ShouldReturnOk);

    /* New branch coverage tests */
    RUN_TEST(test_Mem_Allocate_SmallBlock_ShouldSplitFreeRegion);
    RUN_TEST(test_Mem_Free_AdjacentBlocks_ShouldMergeFreeBlocks);
    RUN_TEST(test_Mem_Allocate_StandardPool_ShouldAllocateFromStandardPool);
    RUN_TEST(test_Mem_Allocate_LargePool_ShouldAllocateFromLargePool);
    RUN_TEST(test_Mem_Reallocate_InvalidHandle_ShouldReportError);
    RUN_TEST(test_Mem_Reallocate_NewSizeZero_ShouldFreeAndReturnInvalid);
    RUN_TEST(test_Mem_GetPointer_InvalidHandle_ShouldReturnNull);
    RUN_TEST(test_Mem_GetMemInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Mem_GetMemInfo_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_CheckIntegrity_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_Defragment_Uninit_ShouldReportError);
    RUN_TEST(test_Mem_Defragment_InvalidPool_ShouldReportError);

    return UnityEnd();
}

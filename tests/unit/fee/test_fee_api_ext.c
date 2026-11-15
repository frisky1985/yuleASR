/**
 * @file test_fee_api_ext.c
 * @brief Fee API 扩展测试：InvalidateBlock / EraseImmediateBlock / Cancel /
 *        GetVersionInfo / SetMode / GetCycleCount 等
 *
 * 现有 test_fee_init/read/write 各 3 个用例, 只覆盖 Happy Path + 基本 DET guard.
 * 本文件补完所有剩余公开 API 的 DET guard + 正常路径.
 */

#include "unity.h"
#include "test_framework.h"
#include "Fee.h"
#include "Det.h"
#include "Fls.h"

/* 从 fee_test_support.c 引入的 stub hook */
extern uint16 FeeTest_DetCallCount;
extern uint8  FeeTest_DetLastApiId;
extern uint8  FeeTest_DetLastErrorId;

static const Fee_ConfigType* g_config;

void setUp(void) {
    extern const Fee_ConfigType Fee_Config;
    g_config = &Fee_Config;
    FeeTest_DetCallCount = 0u;
    FeeTest_DetLastApiId = 0xFFu;
    FeeTest_DetLastErrorId = 0xFFu;
    Fee_Init(g_config);
}

void tearDown(void) {
    Fee_DeInit();
}

/*===========================================================================
 * InvalidteBlock API
 *=========================================================================*/

void test_InvalidateBlock_Valid(void) {
    Std_ReturnType r = Fee_InvalidateBlock(1u);
    TEST_ASSERT_EQUAL(E_OK, r);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
}

void test_InvalidateBlock_InvalidBlockNumber(void) {
    Std_ReturnType r = Fee_InvalidateBlock(9999u);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

void test_InvalidateBlock_Busy(void) {
    /* 先启动一个 write job 让模块进入 BUSY */
    uint8 data[8] = {1};
    (void)Fee_Write(0u, data);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());

    Std_ReturnType r = Fee_InvalidateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

/*===========================================================================
 * EraseImmediateBlock API
 *=========================================================================*/

void test_EraseImmediateBlock_Valid(void) {
    Std_ReturnType r = Fee_EraseImmediateBlock(1u);
    TEST_ASSERT_EQUAL(E_OK, r);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
}

void test_EraseImmediateBlock_InvalidBlockNumber(void) {
    Std_ReturnType r = Fee_EraseImmediateBlock(9999u);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

void test_EraseImmediateBlock_Busy(void) {
    uint8 data[8] = {1};
    (void)Fee_Write(0u, data);
    Std_ReturnType r = Fee_EraseImmediateBlock(1u);
    TEST_ASSERT_EQUAL(E_NOT_OK, r);
}

/*===========================================================================
 * Cancel API
 *=========================================================================*/

void test_Cancel_IdleNoOp(void) {
    /* Cancel on IDLE module — should not crash, status stays IDLE */
    Fee_Cancel();
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

void test_Cancel_BusyClearsState(void) {
    uint8 data[8] = {1};
    (void)Fee_Write(0u, data);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());

    Fee_Cancel();
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_CANCELLED, Fee_GetJobResult());
}

/*===========================================================================
 * SetMode API
 *=========================================================================*/

void test_SetMode_Slow(void) {
    Fee_SetMode(FEE_MODE_SLOW);
    /* SetMode 走 DET guard 就返回, 不真切 mode — 至少不 crash */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

void test_SetMode_Fast(void) {
    Fee_SetMode(FEE_MODE_FAST);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/*===========================================================================
 * GetStatus / GetJobResult API
 *=========================================================================*/

void test_GetStatus_Init(void) {
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

void test_GetStatus_BusyAfterWrite(void) {
    uint8 data[8] = {1};
    (void)Fee_Write(0u, data);
    TEST_ASSERT_EQUAL(FEE_BUSY, Fee_GetStatus());
}

void test_GetJobResult_Init(void) {
    /* Idle 状态下 JobResult 初始值是 FEE_JOB_OK */
    Fee_JobResultType r = Fee_GetJobResult();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, r);
}

void test_GetJobResult_PendingAfterWrite(void) {
    uint8 data[8] = {1};
    (void)Fee_Write(0u, data);
    TEST_ASSERT_EQUAL(FEE_JOB_PENDING, Fee_GetJobResult());
}

/*===========================================================================
 * GetVersionInfo API
 *=========================================================================*/

void test_GetVersionInfo_ValidPtr(void) {
    Std_VersionInfoType vi = {0};
    Fee_GetVersionInfo(&vi);
    TEST_ASSERT_NOT_EQUAL(0u, vi.moduleID);
}

void test_GetVersionInfo_NullPtrTriggersDet(void) {
    FeeTest_DetCallCount = 0u;
    Fee_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);
}

/*===========================================================================
 * GetCycleCount / GetEraseCycleCount / GetWriteCycleCount API
 *=========================================================================*/

void test_GetCycleCount_Init(void) {
    TEST_ASSERT_EQUAL(0u, Fee_GetCycleCount());
    TEST_ASSERT_EQUAL(0u, Fee_GetEraseCycleCount());
    TEST_ASSERT_EQUAL(0u, Fee_GetWriteCycleCount());
}

/*===========================================================================
 * Uninitialized DET guards
 *=========================================================================*/

void test_AllApis_UninitTriggerDet(void) {
    /* DeInit twice: second time should trigger FEE_E_UNINIT */
    Fee_DeInit();
    FeeTest_DetCallCount = 0u;
    Fee_DeInit();
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    /* Read on uninit */
    FeeTest_DetCallCount = 0u;
    uint8 buf[16] = {0};
    (void)Fee_Read(0u, 0u, buf, 16u);
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    /* Write on uninit */
    FeeTest_DetCallCount = 0u;
    (void)Fee_Write(0u, buf);
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    /* SetMode on uninit */
    FeeTest_DetCallCount = 0u;
    Fee_SetMode(FEE_MODE_FAST);
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    /* Invalidate on uninit */
    FeeTest_DetCallCount = 0u;
    (void)Fee_InvalidateBlock(0u);
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    /* Cancel on uninit */
    FeeTest_DetCallCount = 0u;
    Fee_Cancel();
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    /* GetCycleCount on uninit */
    FeeTest_DetCallCount = 0u;
    (void)Fee_GetCycleCount();
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);
}

/*===========================================================================
 * Det_ReportError call count — 每个非法参数路径应触发一次 DET
 *=========================================================================*/

void test_WriteDetGuards(void) {
    FeeTest_DetCallCount = 0u;
    (void)Fee_Write(0u, NULL_PTR);   /* FEE_E_INVALID_DATA_PTR */
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    FeeTest_DetCallCount = 0u;
    (void)Fee_Write(9999u, (uint8[]){0});  /* FEE_E_INVALID_BLOCK_NO */
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);
}

void test_ReadDetGuards(void) {
    FeeTest_DetCallCount = 0u;
    uint8 buf[16] = {0};

    (void)Fee_Read(0u, 0xFFFFu, buf, 16u);  /* FEE_E_INVALID_BLOCK_OFS */
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    FeeTest_DetCallCount = 0u;
    (void)Fee_Read(0u, 0u, buf, 0u);  /* FEE_E_INVALID_BLOCK_LEN */
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);

    FeeTest_DetCallCount = 0u;
    (void)Fee_Read(0u, 0u, NULL_PTR, 16u);  /* FEE_E_INVALID_DATA_PTR */
    TEST_ASSERT_TRUE(FeeTest_DetCallCount >= 1u);
}

/*===========================================================================
 * Main entry
 *=========================================================================*/

int main(void) {
    UNITY_BEGIN();

    /* InvalidateBlock */
    RUN_TEST(test_InvalidateBlock_Valid);
    RUN_TEST(test_InvalidateBlock_InvalidBlockNumber);
    RUN_TEST(test_InvalidateBlock_Busy);

    /* EraseImmediateBlock */
    RUN_TEST(test_EraseImmediateBlock_Valid);
    RUN_TEST(test_EraseImmediateBlock_InvalidBlockNumber);
    RUN_TEST(test_EraseImmediateBlock_Busy);

    /* Cancel */
    RUN_TEST(test_Cancel_IdleNoOp);
    RUN_TEST(test_Cancel_BusyClearsState);

    /* SetMode */
    RUN_TEST(test_SetMode_Slow);
    RUN_TEST(test_SetMode_Fast);

    /* GetStatus / GetJobResult */
    RUN_TEST(test_GetStatus_Init);
    RUN_TEST(test_GetStatus_BusyAfterWrite);
    RUN_TEST(test_GetJobResult_Init);
    RUN_TEST(test_GetJobResult_PendingAfterWrite);

    /* GetVersionInfo */
    RUN_TEST(test_GetVersionInfo_ValidPtr);
    RUN_TEST(test_GetVersionInfo_NullPtrTriggersDet);

    /* Cycle counters */
    RUN_TEST(test_GetCycleCount_Init);

    /* DET guards (uninit + invalid param) */
    RUN_TEST(test_AllApis_UninitTriggerDet);
    RUN_TEST(test_WriteDetGuards);
    RUN_TEST(test_ReadDetGuards);

    return UNITY_END();
}

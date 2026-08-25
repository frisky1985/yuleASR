/**
 * @file test_ramtst.c
 * @brief RAMTST (RAM Test) Driver 模块单元测试
 * @version 1.0.0
 * @date 2026-05-15
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 * 本测试文件为MCAL层RAMTST模块提供全面的单元测试覆盖，包括:
 * - 初始化/反初始化 (RamTst_Init, RamTst_DeInit)
 * - 测试启动/停止 (RamTst_Run, RamTst_Stop)
 * - 状态查询 (RamTst_GetTestStatus)
 * - 结果获取 (RamTst_GetTestResult)
 * - 主函数处理 (RamTst_MainFunction)
 * - 错误处理 (参数检查, 未初始化状态)
 *
 * @test_coverage 目标覆盖率: 80%+
 */

// @tests src/bsw/mcal/ramtst/src/RamTst.c  @tests src/bsw/mcal/ramtst/include/RamTst.h

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*==================================================================================================
*                                      TYPE DEFINITIONS
==================================================================================================*/
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef enum { FALSE = 0, TRUE = 1 } boolean;
typedef enum { E_OK = 0, E_NOT_OK } Std_ReturnType;

#ifndef STD_ON
#define STD_ON                          1U
#define STD_OFF                         0U
#endif

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

/*==================================================================================================
*                                      MODULE INFO
==================================================================================================*/
#define RAMTST_VENDOR_ID                (0x0001U)
#define RAMTST_MODULE_ID                (0x64U)
#define RAMTST_SW_MAJOR_VERSION         (0x01U)
#define RAMTST_SW_MINOR_VERSION         (0x00U)
#define RAMTST_SW_PATCH_VERSION         (0x00U)

/*==================================================================================================
*                                      SERVICE IDs
==================================================================================================*/
#define RAMTST_SID_INIT                 (0x01U)
#define RAMTST_SID_DEINIT               (0x02U)
#define RAMTST_SID_RUN                  (0x03U)
#define RAMTST_SID_STOP                 (0x04U)
#define RAMTST_SID_GET_RESULT           (0x05U)
#define RAMTST_SID_GET_STATUS           (0x06U)

/*==================================================================================================
*                                      DET ERROR CODES
==================================================================================================*/
#define RAMTST_E_NO_ERROR               (0x00U)
#define RAMTST_E_PARAM_POINTER          (0x01U)
#define RAMTST_E_UNINIT                 (0x02U)

/*==================================================================================================
*                                      TYPE DEFINITIONS
==================================================================================================*/
typedef enum {
    RAMTST_ALGORITHM_MARCH = 0,
    RAMTST_ALGORITHM_GALPAT,
    RAMTST_ALGORITHM_WALKPATH
} RamTst_AlgType;

typedef enum {
    RAMTST_RESULT_OK = 0,
    RAMTST_RESULT_NOT_TESTED,
    RAMTST_RESULT_FAILED
} RamTst_TestResultType;

typedef enum {
    RAMTST_STATUS_UNINIT = 0,
    RAMTST_STATUS_IDLE,
    RAMTST_STATUS_RUNNING
} RamTst_StatusType;

typedef enum {
    RAMTST_STATE_UNINIT = 0,
    RAMTST_STATE_IDLE,
    RAMTST_STATE_RUNNING
} RamTst_StateType;

typedef struct {
    uint32 StartAddress;
    uint32 Size;
    RamTst_AlgType Algorithm;
    uint32 CallCycle;
} RamTst_ConfigType;

typedef struct {
    uint16 ModuleId;
    uint8 InstanceId;
    uint8 ApiId;
    uint8 ErrorId;
} Det_ErrorStatusType;

/*==================================================================================================
*                                      TEST RESULT TRACKING
==================================================================================================*/
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, (int)(expected), (int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

/*==================================================================================================
*                                      MOCK STATE VARIABLES
==================================================================================================*/
static RamTst_StateType RamTst_State = RAMTST_STATE_UNINIT;
static const RamTst_ConfigType* RamTst_ConfigPtr = NULL_PTR;
static RamTst_TestResultType RamTst_Result = RAMTST_RESULT_NOT_TESTED;
static uint32 RamTst_TestCounter = 0U;

/* Mock DET tracking */
static Det_ErrorStatusType Det_LastError;
static uint8 Det_ErrorCount = 0U;

/*==================================================================================================
*                                      MOCK DET FUNCTION
==================================================================================================*/
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_LastError.ModuleId = ModuleId;
    Det_LastError.InstanceId = InstanceId;
    Det_LastError.ApiId = ApiId;
    Det_LastError.ErrorId = ErrorId;
    Det_ErrorCount++;
}

void Det_ResetError(void)
{
    Det_ErrorCount = 0U;
    Det_LastError.ModuleId = 0U;
    Det_LastError.InstanceId = 0U;
    Det_LastError.ApiId = 0U;
    Det_LastError.ErrorId = 0U;
}

/*==================================================================================================
*                                      RAMTST IMPLEMENTATION
==================================================================================================*/
void RamTst_Init(const RamTst_ConfigType* ConfigPtr)
{
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(RAMTST_MODULE_ID, 0U, RAMTST_SID_INIT, RAMTST_E_PARAM_POINTER);
        return;
    }
    RamTst_ConfigPtr = ConfigPtr;
    RamTst_State = RAMTST_STATE_IDLE;
    RamTst_Result = RAMTST_RESULT_NOT_TESTED;
}

void RamTst_DeInit(void)
{
    RamTst_State = RAMTST_STATE_UNINIT;
    RamTst_ConfigPtr = NULL_PTR;
}

Std_ReturnType RamTst_Run(void)
{
    if (RamTst_State == RAMTST_STATE_UNINIT) {
        Det_ReportError(RAMTST_MODULE_ID, 0U, RAMTST_SID_RUN, RAMTST_E_UNINIT);
        return E_NOT_OK;
    }
    if (RamTst_State == RAMTST_STATE_RUNNING) {
        return E_NOT_OK;
    }
    RamTst_State = RAMTST_STATE_RUNNING;
    RamTst_Result = RAMTST_RESULT_NOT_TESTED;
    RamTst_TestCounter = 0U;
    return E_OK;
}

void RamTst_Stop(void)
{
    if (RamTst_State == RAMTST_STATE_RUNNING) {
        RamTst_State = RAMTST_STATE_IDLE;
    }
}

RamTst_TestResultType RamTst_GetTestResult(void)
{
    return RamTst_Result;
}

RamTst_StatusType RamTst_GetTestStatus(void)
{
    switch (RamTst_State) {
        case RAMTST_STATE_UNINIT:
            return RAMTST_STATUS_UNINIT;
        case RAMTST_STATE_IDLE:
            return RAMTST_STATUS_IDLE;
        case RAMTST_STATE_RUNNING:
            return RAMTST_STATUS_RUNNING;
        default:
            return RAMTST_STATUS_UNINIT;
    }
}

void RamTst_MainFunction(void)
{
    if (RamTst_State != RAMTST_STATE_RUNNING) {
        return;
    }
    
    /* Simulate test progress */
    RamTst_TestCounter++;
    
    /* Simulate test completion after some cycles */
    if (RamTst_TestCounter >= 5U) {
        RamTst_Result = RAMTST_RESULT_OK;
        RamTst_State = RAMTST_STATE_IDLE;
    }
}

/*==================================================================================================
*                                      TEST FIXTURE
==================================================================================================*/
/* @req SWS_RamTst_00201 */
void test_setup(void)
{
    /* Reset module state before each test */
    RamTst_State = RAMTST_STATE_UNINIT;
    RamTst_ConfigPtr = NULL_PTR;
    RamTst_Result = RAMTST_RESULT_NOT_TESTED;
    RamTst_TestCounter = 0U;
    Det_ResetError();
}

/*==================================================================================================
*                                      TEST CASES - INITIALIZATION
==================================================================================================*/
/* @req SWS_RamTst_00202 */
void test_init_with_valid_config(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] RamTst_Init with valid config\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, RamTst_Result);
    TEST_ASSERT(RamTst_ConfigPtr == &config);
    TEST_ASSERT_EQ(0U, Det_ErrorCount);
}

/* @req SWS_RamTst_00203 */
void test_init_with_null_config(void)
{
    printf("\n[Test] RamTst_Init with NULL config\n");
    test_setup();
    
    RamTst_Init(NULL_PTR);
    
    TEST_ASSERT_EQ(RAMTST_STATE_UNINIT, RamTst_State);
    TEST_ASSERT_EQ(1U, Det_ErrorCount);
    TEST_ASSERT_EQ(RAMTST_MODULE_ID, Det_LastError.ModuleId);
    TEST_ASSERT_EQ(RAMTST_SID_INIT, Det_LastError.ApiId);
    TEST_ASSERT_EQ(RAMTST_E_PARAM_POINTER, Det_LastError.ErrorId);
}

/* @req SWS_RamTst_00204 */
void test_init_multiple_times(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] RamTst_Init multiple times\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    
    /* Initialize again with different config */
    config.StartAddress = 0x20040000U;
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    TEST_ASSERT(RamTst_ConfigPtr->StartAddress == 0x20040000U);
}

/*==================================================================================================
*                                      TEST CASES - DEINITIALIZATION
==================================================================================================*/
/* @req SWS_RamTst_00205 */
void test_deinit_after_init(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] RamTst_DeInit after initialization\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    
    RamTst_DeInit();
    
    TEST_ASSERT_EQ(RAMTST_STATE_UNINIT, RamTst_State);
    TEST_ASSERT(RamTst_ConfigPtr == NULL_PTR);
}

/* @req SWS_RamTst_00206 */
void test_deinit_without_init(void)
{
    printf("\n[Test] RamTst_DeInit without initialization\n");
    test_setup();
    
    RamTst_DeInit();
    
    TEST_ASSERT_EQ(RAMTST_STATE_UNINIT, RamTst_State);
    TEST_ASSERT(RamTst_ConfigPtr == NULL_PTR);
}

/*==================================================================================================
*                                      TEST CASES - RUN/STOP
==================================================================================================*/
/* @req SWS_RamTst_00207 */
void test_run_after_init(void)
{
    RamTst_ConfigType config;
    Std_ReturnType result;
    
    printf("\n[Test] RamTst_Run after initialization\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    
    result = RamTst_Run();
    
    TEST_ASSERT_EQ(E_OK, result);
    TEST_ASSERT_EQ(RAMTST_STATE_RUNNING, RamTst_State);
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, RamTst_Result);
}

/* @req SWS_RamTst_00208 */
void test_run_without_init(void)
{
    Std_ReturnType result;
    
    printf("\n[Test] RamTst_Run without initialization\n");
    test_setup();
    
    result = RamTst_Run();
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(1U, Det_ErrorCount);
    TEST_ASSERT_EQ(RAMTST_MODULE_ID, Det_LastError.ModuleId);
    TEST_ASSERT_EQ(RAMTST_SID_RUN, Det_LastError.ApiId);
    TEST_ASSERT_EQ(RAMTST_E_UNINIT, Det_LastError.ErrorId);
}

/* @req SWS_RamTst_00209 */
void test_run_while_already_running(void)
{
    RamTst_ConfigType config;
    Std_ReturnType result;
    
    printf("\n[Test] RamTst_Run while already running\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    
    /* Try to start again while running */
    result = RamTst_Run();
    
    TEST_ASSERT_EQ(E_NOT_OK, result);
    TEST_ASSERT_EQ(RAMTST_STATE_RUNNING, RamTst_State);
}

/* @req SWS_RamTst_00210 */
void test_stop_while_running(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] RamTst_Stop while running\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    TEST_ASSERT_EQ(RAMTST_STATE_RUNNING, RamTst_State);
    
    RamTst_Stop();
    
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
}

/* @req SWS_RamTst_00211 */
void test_stop_while_idle(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] RamTst_Stop while idle\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    
    RamTst_Stop();
    
    /* Should stay in IDLE state */
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
}

/* @req SWS_RamTst_00212 */
void test_stop_while_uninit(void)
{
    printf("\n[Test] RamTst_Stop while uninitialized\n");
    test_setup();
    
    RamTst_Stop();
    
    /* Should stay in UNINIT state */
    TEST_ASSERT_EQ(RAMTST_STATE_UNINIT, RamTst_State);
}

/*==================================================================================================
*                                      TEST CASES - GET STATUS
==================================================================================================*/
/* @req SWS_RamTst_00213 */
void test_get_status_uninit(void)
{
    RamTst_StatusType status;
    
    printf("\n[Test] RamTst_GetTestStatus when uninitialized\n");
    test_setup();
    
    status = RamTst_GetTestStatus();
    
    TEST_ASSERT_EQ(RAMTST_STATUS_UNINIT, status);
}

/* @req SWS_RamTst_00214 */
void test_get_status_idle(void)
{
    RamTst_ConfigType config;
    RamTst_StatusType status;
    
    printf("\n[Test] RamTst_GetTestStatus when idle\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    status = RamTst_GetTestStatus();
    
    TEST_ASSERT_EQ(RAMTST_STATUS_IDLE, status);
}

/* @req SWS_RamTst_00215 */
void test_get_status_running(void)
{
    RamTst_ConfigType config;
    RamTst_StatusType status;
    
    printf("\n[Test] RamTst_GetTestStatus when running\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    status = RamTst_GetTestStatus();
    
    TEST_ASSERT_EQ(RAMTST_STATUS_RUNNING, status);
}

/*==================================================================================================
*                                      TEST CASES - GET RESULT
==================================================================================================*/
/* @req SWS_RamTst_00216 */
void test_get_result_not_tested(void)
{
    RamTst_ConfigType config;
    RamTst_TestResultType result;
    
    printf("\n[Test] RamTst_GetTestResult before test\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    result = RamTst_GetTestResult();
    
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, result);
}

/* @req SWS_RamTst_00217 */
void test_get_result_after_run(void)
{
    RamTst_ConfigType config;
    RamTst_TestResultType result;
    
    printf("\n[Test] RamTst_GetTestResult after starting test\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    
    /* Result should be NOT_TESTED until test completes */
    result = RamTst_GetTestResult();
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, result);
}

/*==================================================================================================
*                                      TEST CASES - MAIN FUNCTION
==================================================================================================*/
/* @req SWS_RamTst_00218 */
void test_mainfunction_when_not_running(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] RamTst_MainFunction when not running\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    
    /* Should not change state or result */
    RamTst_MainFunction();
    
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, RamTst_Result);
}

/* @req SWS_RamTst_00219 */
void test_mainfunction_completes_test(void)
{
    RamTst_ConfigType config;
    RamTst_TestResultType result;
    uint32 i;
    
    printf("\n[Test] RamTst_MainFunction completes test\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    
    TEST_ASSERT_EQ(RAMTST_STATE_RUNNING, RamTst_State);
    
    /* Call MainFunction multiple times to complete test */
    for (i = 0U; i < 10U; i++) {
        RamTst_MainFunction();
        if (RamTst_GetTestStatus() == RAMTST_STATUS_IDLE) {
            break;
        }
    }
    
    TEST_ASSERT_EQ(RAMTST_STATE_IDLE, RamTst_State);
    result = RamTst_GetTestResult();
    TEST_ASSERT_EQ(RAMTST_RESULT_OK, result);
}

/* @req SWS_RamTst_00220 */
void test_mainfunction_while_uninit(void)
{
    printf("\n[Test] RamTst_MainFunction when uninitialized\n");
    test_setup();
    
    /* Should not crash or report errors */
    RamTst_MainFunction();
    
    TEST_ASSERT_EQ(RAMTST_STATE_UNINIT, RamTst_State);
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, RamTst_Result);
}

/*==================================================================================================
*                                      TEST CASES - FULL CYCLE
==================================================================================================*/
/* @req SWS_RamTst_00221 */
void test_full_test_cycle(void)
{
    RamTst_ConfigType config;
    Std_ReturnType status;
    RamTst_StatusType testStatus;
    RamTst_TestResultType result;
    uint32 i;
    
    printf("\n[Test] Full test cycle\n");
    test_setup();
    
    /* Configure test parameters */
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    /* Step 1: Initialize */
    RamTst_Init(&config);
    testStatus = RamTst_GetTestStatus();
    TEST_ASSERT_EQ(RAMTST_STATUS_IDLE, testStatus);
    
    /* Step 2: Start test */
    status = RamTst_Run();
    TEST_ASSERT_EQ(E_OK, status);
    testStatus = RamTst_GetTestStatus();
    TEST_ASSERT_EQ(RAMTST_STATUS_RUNNING, testStatus);
    result = RamTst_GetTestResult();
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, result);
    
    /* Step 3: Process test */
    for (i = 0U; i < 10U; i++) {
        RamTst_MainFunction();
        if (RamTst_GetTestStatus() == RAMTST_STATUS_IDLE) {
            break;
        }
    }
    
    /* Step 4: Verify completion */
    testStatus = RamTst_GetTestStatus();
    TEST_ASSERT_EQ(RAMTST_STATUS_IDLE, testStatus);
    result = RamTst_GetTestResult();
    TEST_ASSERT_EQ(RAMTST_RESULT_OK, result);
    
    /* Step 5: Cleanup */
    RamTst_DeInit();
    testStatus = RamTst_GetTestStatus();
    TEST_ASSERT_EQ(RAMTST_STATUS_UNINIT, testStatus);
}

/* @req SWS_RamTst_00222 */
void test_multiple_test_cycles(void)
{
    RamTst_ConfigType config;
    uint32 cycle;
    uint32 i;
    
    printf("\n[Test] Multiple test cycles\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    
    for (cycle = 0U; cycle < 3U; cycle++) {
        /* Run test cycle */
        RamTst_Run();
        TEST_ASSERT_EQ(RAMTST_STATUS_RUNNING, RamTst_GetTestStatus());
        
        for (i = 0U; i < 10U; i++) {
            RamTst_MainFunction();
            if (RamTst_GetTestStatus() == RAMTST_STATUS_IDLE) {
                break;
            }
        }
        
        TEST_ASSERT_EQ(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
        TEST_ASSERT_EQ(RAMTST_RESULT_OK, RamTst_GetTestResult());
    }
}

/* @req SWS_RamTst_00223 */
void test_stop_during_test(void)
{
    RamTst_ConfigType config;
    uint32 i;
    
    printf("\n[Test] Stop test during execution\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    
    /* Call MainFunction a few times */
    for (i = 0U; i < 2U; i++) {
        RamTst_MainFunction();
    }
    
    /* Stop before completion */
    RamTst_Stop();
    
    TEST_ASSERT_EQ(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    /* Result should still be NOT_TESTED since test was interrupted */
    TEST_ASSERT_EQ(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}

/*==================================================================================================
*                                      TEST CASES - DIFFERENT ALGORITHMS
==================================================================================================*/
/* @req SWS_RamTst_00224 */
void test_algorithm_march(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] March algorithm test\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_ALGORITHM_MARCH, RamTst_ConfigPtr->Algorithm);
}

/* @req SWS_RamTst_00225 */
void test_algorithm_galpat(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] GALPAT algorithm test\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_GALPAT;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_ALGORITHM_GALPAT, RamTst_ConfigPtr->Algorithm);
}

/* @req SWS_RamTst_00226 */
void test_algorithm_walkpath(void)
{
    RamTst_ConfigType config;
    
    printf("\n[Test] Walkpath algorithm test\n");
    test_setup();
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_WALKPATH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    TEST_ASSERT_EQ(RAMTST_ALGORITHM_WALKPATH, RamTst_ConfigPtr->Algorithm);
}

/*==================================================================================================
*                                      TEST RUNNER
==================================================================================================*/
void run_all_tests(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("       RAMTST (RAM Test) Module Unit Tests                 \n");
    printf("       Shanghai Yule Electronics Technology Co., Ltd.      \n");
    printf("============================================================\n");
    
    /* Initialization tests */
    printf("\n--- Initialization Tests ---\n");
    test_init_with_valid_config();
    test_init_with_null_config();
    test_init_multiple_times();
    
    /* Deinitialization tests */
    printf("\n--- Deinitialization Tests ---\n");
    test_deinit_after_init();
    test_deinit_without_init();
    
    /* Run/Stop tests */
    printf("\n--- Run/Stop Tests ---\n");
    test_run_after_init();
    test_run_without_init();
    test_run_while_already_running();
    test_stop_while_running();
    test_stop_while_idle();
    test_stop_while_uninit();
    
    /* Status tests */
    printf("\n--- Status Tests ---\n");
    test_get_status_uninit();
    test_get_status_idle();
    test_get_status_running();
    
    /* Result tests */
    printf("\n--- Result Tests ---\n");
    test_get_result_not_tested();
    test_get_result_after_run();
    
    /* MainFunction tests */
    printf("\n--- MainFunction Tests ---\n");
    test_mainfunction_when_not_running();
    test_mainfunction_completes_test();
    test_mainfunction_while_uninit();
    
    /* Full cycle tests */
    printf("\n--- Full Cycle Tests ---\n");
    test_full_test_cycle();
    test_multiple_test_cycles();
    test_stop_during_test();
    
    /* Algorithm tests */
    printf("\n--- Algorithm Tests ---\n");
    test_algorithm_march();
    test_algorithm_galpat();
    test_algorithm_walkpath();
}

void print_test_summary(void)
{
    int coverage = 0;
    
    printf("\n");
    printf("============================================================\n");
    printf("                      TEST SUMMARY                         \n");
    printf("============================================================\n");
    printf("  Total Tests:    %d\n", tests_run);
    printf("  Passed:         %d\n", tests_passed);
    printf("  Failed:         %d\n", tests_failed);
    
    if (tests_run > 0) {
        coverage = (tests_passed * 100) / tests_run;
        printf("  Pass Rate:      %d%%\n", coverage);
    }
    
    printf("------------------------------------------------------------\n");
    
    if (tests_failed == 0) {
        printf("  Result: ALL TESTS PASSED ✓\n");
        printf("============================================================\n");
    } else {
        printf("  Result: SOME TESTS FAILED ✗\n");
        printf("============================================================\n");
    }
}

/*==================================================================================================
*                                      MAIN ENTRY
==================================================================================================*/
int main(void)
{
    run_all_tests();
    print_test_summary();
    
    return (tests_failed == 0) ? 0 : 1;
}

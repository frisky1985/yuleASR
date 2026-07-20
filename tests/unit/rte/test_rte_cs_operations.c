/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : RTE Client-Server Operations Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-27
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* Description: 单元测试 - RTE 客户端-服务端调用操作
*              覆盖 Rte_Call, Rte_Read, Rte_Write 等 CS 接口
*
* 注意: 本测试文件是自包含的，定义了所需的类型和桩函数，
*       以避免在桌面 Linux 上编译嵌入式 AutoSAR 头文件时的冲突。
*==================================================================================================*/

#include "../test_framework.h"
#include <stdint.h>
#include <stddef.h>

/*==================================================================================================
*                                     TYPE DEFINITIONS
*==================================================================================================*/

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef STD_ON
#define STD_ON 1
#endif

#ifndef STD_OFF
#define STD_OFF 0
#endif

/* AutoSAR standard types */
typedef unsigned char   boolean;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef signed char     sint8;
typedef signed short    sint16;
typedef signed long     sint32;

typedef uint8  Std_ReturnType;

#ifndef E_OK
#define E_OK            ((Std_ReturnType)0U)
#endif
#ifndef E_NOT_OK
#define E_NOT_OK        ((Std_ReturnType)1U)
#endif

/* RTE Status Type */
typedef enum {
    RTE_E_OK = 0,
    RTE_E_NOK,
    RTE_E_INVALID,
    RTE_E_TIMEOUT,
    RTE_E_LIMIT,
    RTE_E_NO_DATA,
    RTE_E_TRANSMIT_ACK,
    RTE_E_UNCONNECTED,
    RTE_E_SEG_FAULT,
    RTE_E_OUT_OF_RANGE,
    RTE_E_OK_PENDING
} Rte_StatusType;

/*==================================================================================================
*                                     MOCK / STUB VARIABLES
*==================================================================================================*/

/* --- Stub: Rte_Call_EngineControl_SetTargetRPM --- */
static uint16 g_stub_targetRPM = 0U;
static Rte_StatusType g_stub_setTargetRPM_result = RTE_E_OK;
static uint32 g_stub_setTargetRPM_callCount = 0U;

/* --- Stub: Rte_Call_DiagnosticManager_ReadDTC --- */
static uint32 g_stub_readDTC_dtcCode = 0U;
static uint8  g_stub_readDTC_statusByte = 0U;
static Rte_StatusType g_stub_readDTC_result = RTE_E_OK;
static uint32 g_stub_readDTC_callCount = 0U;

/* --- Stub: Rte_Read_EngineControl_RPM --- */
static uint16 g_stub_engineRPM = 0U;
static Rte_StatusType g_stub_readRPM_result = RTE_E_OK;
static uint32 g_stub_readRPM_callCount = 0U;

/* --- Stub: Rte_Write_EngineControl_Throttle --- */
static uint16 g_stub_throttleValue = 0U;
static Rte_StatusType g_stub_writeThrottle_result = RTE_E_OK;
static uint32 g_stub_writeThrottle_callCount = 0U;

/* --- Stub: Rte_Call_WatchdogManager_Reset --- */
static Rte_StatusType g_stub_wdgReset_result = RTE_E_OK;
static uint32 g_stub_wdgReset_callCount = 0U;

/* --- Stub: Rte_GetAsyncResult --- */
typedef struct {
    boolean     isPending;
    Rte_StatusType result;
    uint32      operationId;
    uint32      elapsedTimeMs;
} Rte_AsyncResultType;

static Rte_AsyncResultType g_stub_asyncResult;
static uint32 g_stub_asyncGetResult_callCount = 0U;

/*==================================================================================================
*                                     STUB IMPLEMENTATIONS
*==================================================================================================*/

/**
 * @brief 模拟 Rte_Call_EngineControl_SetTargetRPM
 *        设置目标发动机转速
 */
Std_ReturnType Rte_Call_EngineControl_SetTargetRPM(uint16 targetRPM)
{
    g_stub_setTargetRPM_callCount++;

    if (g_stub_setTargetRPM_result == RTE_E_OK)
    {
        g_stub_targetRPM = targetRPM;
    }

    return (Std_ReturnType)g_stub_setTargetRPM_result;
}

/**
 * @brief 模拟 Rte_Call_DiagnosticManager_ReadDTC
 *        读取指定 DTC 的状态
 */
Std_ReturnType Rte_Call_DiagnosticManager_ReadDTC(uint32 dtcCode, uint8* statusByte)
{
    g_stub_readDTC_callCount++;

    if (statusByte == NULL_PTR)
    {
        return E_NOT_OK;
    }

    if (g_stub_readDTC_result == RTE_E_OK)
    {
        g_stub_readDTC_dtcCode = dtcCode;
        *statusByte = g_stub_readDTC_statusByte;
    }

    return (Std_ReturnType)g_stub_readDTC_result;
}

/**
 * @brief 模拟 Rte_Read_EngineControl_RPM
 *        从 EngineControl 组件读取当前发动机转速
 */
Rte_StatusType Rte_Read_EngineControl_RPM(uint16* rpm)
{
    g_stub_readRPM_callCount++;

    if (rpm == NULL_PTR)
    {
        return RTE_E_SEG_FAULT;
    }

    if (g_stub_readRPM_result == RTE_E_OK)
    {
        *rpm = g_stub_engineRPM;
    }

    return g_stub_readRPM_result;
}

/**
 * @brief 模拟 Rte_Write_EngineControl_Throttle
 *        向 EngineControl 组件写入节气门位置值
 */
Rte_StatusType Rte_Write_EngineControl_Throttle(uint16 throttlePosition)
{
    g_stub_writeThrottle_callCount++;

    if (g_stub_writeThrottle_result == RTE_E_OK)
    {
        g_stub_throttleValue = throttlePosition;
    }

    return g_stub_writeThrottle_result;
}

/**
 * @brief 模拟 Rte_Call_WatchdogManager_Reset
 *        执行看门狗复位操作
 */
Rte_StatusType Rte_Call_WatchdogManager_Reset(void)
{
    g_stub_wdgReset_callCount++;
    return g_stub_wdgReset_result;
}

/**
 * @brief 模拟 Rte_GetAsyncResult
 *        获取异步操作的结果
 */
Rte_StatusType Rte_GetAsyncResult(uint32 operationId, Rte_StatusType* result, uint32 timeoutMs)
{
    g_stub_asyncGetResult_callCount++;
    (void)timeoutMs;

    if (result == NULL_PTR)
    {
        return RTE_E_SEG_FAULT;
    }

    if (operationId != g_stub_asyncResult.operationId)
    {
        return RTE_E_INVALID;
    }

    if (g_stub_asyncResult.isPending)
    {
        return RTE_E_OK_PENDING;
    }

    *result = g_stub_asyncResult.result;
    return RTE_E_OK;
}

/*==================================================================================================
*                                     MOCK RESET HELPERS
*==================================================================================================*/

static void Mock_Reset_All(void)
{
    /* EngineControl_SetTargetRPM */
    g_stub_targetRPM = 0U;
    g_stub_setTargetRPM_result = RTE_E_OK;
    g_stub_setTargetRPM_callCount = 0U;

    /* DiagnosticManager_ReadDTC */
    g_stub_readDTC_dtcCode = 0U;
    g_stub_readDTC_statusByte = 0U;
    g_stub_readDTC_result = RTE_E_OK;
    g_stub_readDTC_callCount = 0U;

    /* Rte_Read_EngineControl_RPM */
    g_stub_engineRPM = 0U;
    g_stub_readRPM_result = RTE_E_OK;
    g_stub_readRPM_callCount = 0U;

    /* Rte_Write_EngineControl_Throttle */
    g_stub_throttleValue = 0U;
    g_stub_writeThrottle_result = RTE_E_OK;
    g_stub_writeThrottle_callCount = 0U;

    /* Rte_Call_WatchdogManager_Reset */
    g_stub_wdgReset_result = RTE_E_OK;
    g_stub_wdgReset_callCount = 0U;

    /* Rte_GetAsyncResult */
    g_stub_asyncResult.isPending = FALSE;
    g_stub_asyncResult.result = RTE_E_OK;
    g_stub_asyncResult.operationId = 0U;
    g_stub_asyncResult.elapsedTimeMs = 0U;
    g_stub_asyncGetResult_callCount = 0U;
}

/*==================================================================================================
*                                     TEST CASES
*==================================================================================================*/

/* ===== 1. Rte_Call_EngineControl_SetTargetRPM ===== */

TEST_CASE(Rte_Call_EngineControl_SetTargetRPM_normal)
{
    Mock_Reset_All();
    uint16 setRPM = 3000U;

    Std_ReturnType ret = Rte_Call_EngineControl_SetTargetRPM(setRPM);

    ASSERT_EQ((int)E_OK, (int)ret);
    ASSERT_EQ(1U, g_stub_setTargetRPM_callCount);
    ASSERT_EQ(setRPM, g_stub_targetRPM);
}

TEST_CASE(Rte_Call_EngineControl_SetTargetRPM_zero)
{
    Mock_Reset_All();
    uint16 setRPM = 0U;

    Std_ReturnType ret = Rte_Call_EngineControl_SetTargetRPM(setRPM);

    ASSERT_EQ((int)E_OK, (int)ret);
    ASSERT_EQ(0U, g_stub_targetRPM);
}

TEST_CASE(Rte_Call_EngineControl_SetTargetRPM_max)
{
    Mock_Reset_All();
    uint16 setRPM = 8000U;

    Std_ReturnType ret = Rte_Call_EngineControl_SetTargetRPM(setRPM);

    ASSERT_EQ((int)E_OK, (int)ret);
    ASSERT_EQ(8000U, g_stub_targetRPM);
}

/* ===== 2. Rte_Call_DiagnosticManager_ReadDTC ===== */

TEST_CASE(Rte_Call_DiagnosticManager_ReadDTC_normal)
{
    Mock_Reset_All();
    uint32 dtcCode = 0x123456U;
    uint8 statusByte = 0x9FU;
    uint8 resultByte = 0U;
    g_stub_readDTC_statusByte = statusByte;

    Std_ReturnType ret = Rte_Call_DiagnosticManager_ReadDTC(dtcCode, &resultByte);

    ASSERT_EQ((int)E_OK, (int)ret);
    ASSERT_EQ(1U, g_stub_readDTC_callCount);
    ASSERT_EQ(dtcCode, g_stub_readDTC_dtcCode);
    ASSERT_EQ(statusByte, resultByte);
}

TEST_CASE(Rte_Call_DiagnosticManager_ReadDTC_all_dtc)
{
    Mock_Reset_All();
    uint32 dtcCode = 0xFFFFFFU;
    uint8 statusByte = 0x00U;
    uint8 resultByte = 0xFFU;
    g_stub_readDTC_statusByte = statusByte;

    Std_ReturnType ret = Rte_Call_DiagnosticManager_ReadDTC(dtcCode, &resultByte);

    ASSERT_EQ((int)E_OK, (int)ret);
    ASSERT_EQ(0xFFFFFFU, g_stub_readDTC_dtcCode);
    ASSERT_EQ(0x00U, resultByte);
}

TEST_CASE(Rte_Call_DiagnosticManager_ReadDTC_multi_call)
{
    Mock_Reset_All();
    uint8 resultByte = 0U;

    /* 第一次调用 */
    g_stub_readDTC_statusByte = 0xA0U;
    Std_ReturnType ret1 = Rte_Call_DiagnosticManager_ReadDTC(0x001U, &resultByte);
    ASSERT_EQ((int)E_OK, (int)ret1);
    ASSERT_EQ(0xA0U, resultByte);

    /* 第二次调用 - 不同 DTC */
    g_stub_readDTC_statusByte = 0x50U;
    Std_ReturnType ret2 = Rte_Call_DiagnosticManager_ReadDTC(0x002U, &resultByte);
    ASSERT_EQ((int)E_OK, (int)ret2);
    ASSERT_EQ(0x50U, resultByte);

    /* 验证调用计数和最后写入的 DTC 码 */
    ASSERT_EQ(2U, g_stub_readDTC_callCount);
    ASSERT_EQ(0x002U, g_stub_readDTC_dtcCode);
}

/* ===== 3. Rte_Read_EngineControl_RPM ===== */

TEST_CASE(Rte_Read_EngineControl_RPM_normal)
{
    Mock_Reset_All();
    uint16 expectedRPM = 1500U;
    uint16 actualRPM = 0U;
    g_stub_engineRPM = expectedRPM;

    Rte_StatusType ret = Rte_Read_EngineControl_RPM(&actualRPM);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ(1U, g_stub_readRPM_callCount);
    ASSERT_EQ(expectedRPM, actualRPM);
}

TEST_CASE(Rte_Read_EngineControl_RPM_idle)
{
    Mock_Reset_All();
    uint16 actualRPM = 9999U;
    g_stub_engineRPM = 800U;  /* 怠速转速 */

    Rte_StatusType ret = Rte_Read_EngineControl_RPM(&actualRPM);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ(800U, actualRPM);
}

TEST_CASE(Rte_Read_EngineControl_RPM_zero)
{
    Mock_Reset_All();
    uint16 actualRPM = 0U;

    Rte_StatusType ret = Rte_Read_EngineControl_RPM(&actualRPM);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
}

/* ===== 4. Rte_Write_EngineControl_Throttle ===== */

TEST_CASE(Rte_Write_EngineControl_Throttle_normal)
{
    Mock_Reset_All();
    uint16 throttlePos = 50U;  /* 50% 节气门开度 */

    Rte_StatusType ret = Rte_Write_EngineControl_Throttle(throttlePos);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ(1U, g_stub_writeThrottle_callCount);
    ASSERT_EQ(throttlePos, g_stub_throttleValue);
}

TEST_CASE(Rte_Write_EngineControl_Throttle_full_open)
{
    Mock_Reset_All();

    Rte_StatusType ret = Rte_Write_EngineControl_Throttle(100U);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ(100U, g_stub_throttleValue);
}

TEST_CASE(Rte_Write_EngineControl_Throttle_closed)
{
    Mock_Reset_All();

    Rte_StatusType ret = Rte_Write_EngineControl_Throttle(0U);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ(0U, g_stub_throttleValue);
}

/* ===== 5. Rte_Call_WatchdogManager_Reset ===== */

TEST_CASE(Rte_Call_WatchdogManager_Reset_normal)
{
    Mock_Reset_All();

    Rte_StatusType ret = Rte_Call_WatchdogManager_Reset();

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ(1U, g_stub_wdgReset_callCount);
}

TEST_CASE(Rte_Call_WatchdogManager_Reset_multiple)
{
    Mock_Reset_All();

    /* 连续多次复位 */
    for (uint32 i = 0U; i < 10U; i++)
    {
        Rte_StatusType ret = Rte_Call_WatchdogManager_Reset();
        ASSERT_EQ((int)RTE_E_OK, (int)ret);
    }

    ASSERT_EQ(10U, g_stub_wdgReset_callCount);
}

/* ===== 6. 异步调用结果获取 ===== */

TEST_CASE(Rte_GetAsyncResult_completed)
{
    Mock_Reset_All();
    g_stub_asyncResult.isPending = FALSE;
    g_stub_asyncResult.result = RTE_E_OK;
    g_stub_asyncResult.operationId = 1U;
    Rte_StatusType result = RTE_E_NOK;

    Rte_StatusType ret = Rte_GetAsyncResult(1U, &result, 100U);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ((int)RTE_E_OK, (int)result);
}

TEST_CASE(Rte_GetAsyncResult_pending)
{
    Mock_Reset_All();
    g_stub_asyncResult.isPending = TRUE;
    g_stub_asyncResult.operationId = 2U;
    Rte_StatusType result = RTE_E_NOK;

    Rte_StatusType ret = Rte_GetAsyncResult(2U, &result, 100U);

    ASSERT_EQ((int)RTE_E_OK_PENDING, (int)ret);
}

TEST_CASE(Rte_GetAsyncResult_completed_with_error)
{
    Mock_Reset_All();
    g_stub_asyncResult.isPending = FALSE;
    g_stub_asyncResult.result = RTE_E_TIMEOUT;
    g_stub_asyncResult.operationId = 3U;
    Rte_StatusType result = RTE_E_OK;

    Rte_StatusType ret = Rte_GetAsyncResult(3U, &result, 100U);

    ASSERT_EQ((int)RTE_E_OK, (int)ret);
    ASSERT_EQ((int)RTE_E_TIMEOUT, (int)result);
}

/* ===== 7. 超时处理 ===== */

TEST_CASE(Rte_GetAsyncResult_timeout)
{
    Mock_Reset_All();
    g_stub_asyncResult.isPending = TRUE;  /* 仍然处于 pending 状态表示超时未返回 */
    g_stub_asyncResult.operationId = 10U;
    Rte_StatusType result = RTE_E_OK;

    Rte_StatusType ret = Rte_GetAsyncResult(10U, &result, 0U);

    ASSERT_EQ((int)RTE_E_OK_PENDING, (int)ret);
}

/* ===== 8. 错误注入测试 ===== */

/* 8a. 无效句柄 / 无效操作 ID */
TEST_CASE(Rte_GetAsyncResult_invalid_operation_id)
{
    Mock_Reset_All();
    g_stub_asyncResult.isPending = FALSE;
    g_stub_asyncResult.result = RTE_E_OK;
    g_stub_asyncResult.operationId = 1U;  /* 只有 operationId=1 是有效的 */
    Rte_StatusType result = RTE_E_NOK;

    Rte_StatusType ret = Rte_GetAsyncResult(999U, &result, 100U);

    ASSERT_EQ((int)RTE_E_INVALID, (int)ret);
}

/* 8b. 空指针测试 */
TEST_CASE(Rte_Call_DiagnosticManager_ReadDTC_null_pointer)
{
    Mock_Reset_All();

    Std_ReturnType ret = Rte_Call_DiagnosticManager_ReadDTC(0x123456U, NULL_PTR);

    ASSERT_EQ((int)E_NOT_OK, (int)ret);
}

TEST_CASE(Rte_Read_EngineControl_RPM_null_pointer)
{
    Mock_Reset_All();

    Rte_StatusType ret = Rte_Read_EngineControl_RPM(NULL_PTR);

    ASSERT_EQ((int)RTE_E_SEG_FAULT, (int)ret);
}

TEST_CASE(Rte_GetAsyncResult_null_pointer)
{
    Mock_Reset_All();
    g_stub_asyncResult.isPending = FALSE;
    g_stub_asyncResult.result = RTE_E_OK;
    g_stub_asyncResult.operationId = 1U;

    Rte_StatusType ret = Rte_GetAsyncResult(1U, NULL_PTR, 100U);

    ASSERT_EQ((int)RTE_E_SEG_FAULT, (int)ret);
}

/* 8c. 服务端返回错误 - 验证错误透传 */
TEST_CASE(Rte_Call_EngineControl_SetTargetRPM_server_error)
{
    Mock_Reset_All();
    g_stub_setTargetRPM_result = RTE_E_NOK;

    Std_ReturnType ret = Rte_Call_EngineControl_SetTargetRPM(2000U);

    ASSERT_EQ((int)RTE_E_NOK, (int)ret);
    /* 返回错误时 targetRPM 不应被修改 */
    ASSERT_EQ(0U, g_stub_targetRPM);
}

TEST_CASE(Rte_Call_WatchdogManager_Reset_server_error)
{
    Mock_Reset_All();
    g_stub_wdgReset_result = RTE_E_NOK;

    Rte_StatusType ret = Rte_Call_WatchdogManager_Reset();

    ASSERT_EQ((int)RTE_E_NOK, (int)ret);
    ASSERT_EQ(1U, g_stub_wdgReset_callCount);
}

/* 8d. 组合操作 - 设置目标转速后验证读取值的一致性 */
TEST_CASE(Rte_Read_EngineControl_RPM_after_SetTargetRPM)
{
    Mock_Reset_All();

    /* Step 1: 设置目标转速 */
    uint16 setRPM = 3500U;
    Std_ReturnType retSet = Rte_Call_EngineControl_SetTargetRPM(setRPM);
    ASSERT_EQ((int)E_OK, (int)retSet);
    ASSERT_EQ(setRPM, g_stub_targetRPM);

    /* Step 2: 模拟服务端更新 RPM 值后读取 */
    g_stub_engineRPM = 3480U;  /* 实际转速可能略有偏差 */
    uint16 actualRPM = 0U;
    Rte_StatusType retRead = Rte_Read_EngineControl_RPM(&actualRPM);
    ASSERT_EQ((int)RTE_E_OK, (int)retRead);
    ASSERT_EQ(3480U, actualRPM);

}

/*==================================================================================================
*                                   TEST RUNNER
*==================================================================================================*/

TEST_MAIN_BEGIN()
{
    printf("\n--- RTE Client-Server Operations Tests ---\n");

    /* 1. Rte_Call_EngineControl_SetTargetRPM */
    RUN_TEST(Rte_Call_EngineControl_SetTargetRPM_normal);
    RUN_TEST(Rte_Call_EngineControl_SetTargetRPM_zero);
    RUN_TEST(Rte_Call_EngineControl_SetTargetRPM_max);

    /* 2. Rte_Call_DiagnosticManager_ReadDTC */
    RUN_TEST(Rte_Call_DiagnosticManager_ReadDTC_normal);
    RUN_TEST(Rte_Call_DiagnosticManager_ReadDTC_all_dtc);
    RUN_TEST(Rte_Call_DiagnosticManager_ReadDTC_multi_call);

    /* 3. Rte_Read_EngineControl_RPM */
    RUN_TEST(Rte_Read_EngineControl_RPM_normal);
    RUN_TEST(Rte_Read_EngineControl_RPM_idle);
    RUN_TEST(Rte_Read_EngineControl_RPM_zero);

    /* 4. Rte_Write_EngineControl_Throttle */
    RUN_TEST(Rte_Write_EngineControl_Throttle_normal);
    RUN_TEST(Rte_Write_EngineControl_Throttle_full_open);
    RUN_TEST(Rte_Write_EngineControl_Throttle_closed);

    /* 5. Rte_Call_WatchdogManager_Reset */
    RUN_TEST(Rte_Call_WatchdogManager_Reset_normal);
    RUN_TEST(Rte_Call_WatchdogManager_Reset_multiple);

    /* 6. 异步调用结果获取 */
    RUN_TEST(Rte_GetAsyncResult_completed);
    RUN_TEST(Rte_GetAsyncResult_pending);
    RUN_TEST(Rte_GetAsyncResult_completed_with_error);

    /* 7. 超时处理 */
    RUN_TEST(Rte_GetAsyncResult_timeout);

    /* 8. 错误注入测试 */
    RUN_TEST(Rte_GetAsyncResult_invalid_operation_id);
    RUN_TEST(Rte_Call_DiagnosticManager_ReadDTC_null_pointer);
    RUN_TEST(Rte_Read_EngineControl_RPM_null_pointer);
    RUN_TEST(Rte_GetAsyncResult_null_pointer);
    RUN_TEST(Rte_Call_EngineControl_SetTargetRPM_server_error);
    RUN_TEST(Rte_Call_WatchdogManager_Reset_server_error);
    RUN_TEST(Rte_Read_EngineControl_RPM_after_SetTargetRPM);
}
TEST_MAIN_END()

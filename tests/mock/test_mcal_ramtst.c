/**
 * @file test_mcal_ramtst.c
 * @brief RamTst unit test — links real RamTst.c + RamTst_Lcfg.c production code with MockHAL
 *
 * AUTOSAR RAM Test (RamTst) — MCAL Layer
 * Tests: Init/DeInit/Run/Stop/GetResult/GetStatus/MainFunction/GetVersionInfo/SetMode
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "RamTst.h"

static RamTst_ConfigType g_test_cfg;

void setUp(void) { mock_hal_reset(); memset(&g_test_cfg, 0, sizeof(g_test_cfg)); }
void tearDown(void) {}

/* ========= RamTst_Init ========= */
void test_RamTst_Init_NullConfig(void)
{
    RamTst_Init(NULL);
    TEST_ASSERT_EQUAL(RAMTST_STATUS_UNINIT, RamTst_GetStatus());
}

void test_RamTst_Init_Valid(void)
{
    RamTst_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetStatus());
}

void test_RamTst_Init_DoubleInit(void)
{
    RamTst_Init(&g_test_cfg);
    RamTst_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetStatus());
}

/* ========= RamTst_DeInit ========= */
void test_RamTst_DeInit_BeforeInit(void)
{
    RamTst_DeInit();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_UNINIT, RamTst_GetStatus());
}

void test_RamTst_DeInit_AfterInit(void)
{
    RamTst_Init(&g_test_cfg);
    RamTst_DeInit();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_UNINIT, RamTst_GetStatus());
}

/* ========= RamTst_Run ========= */
/* Note: RamTst_Run(void) uses config's test area, so we need proper cfg */
void test_RamTst_Run_BeforeInit(void)
{
    Std_ReturnType ret = RamTst_Run();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_RamTst_Run_Valid(void)
{
    RamTst_Init(&g_test_cfg);
    Std_ReturnType ret = RamTst_Run();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* ========= RamTst_Stop ========= */
void test_RamTst_Stop_BeforeInit(void)
{
    RamTst_Stop();
    /* Should not crash */
    TEST_ASSERT_EQUAL(RAMTST_STATUS_UNINIT, RamTst_GetStatus());
}

void test_RamTst_Stop_AfterInit(void)
{
    RamTst_Init(&g_test_cfg);
    RamTst_Stop();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetStatus());
}

/* ========= RamTst_GetResult ========= */
void test_RamTst_GetResult_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(RAMTST_RESULT_NOT_TESTED, RamTst_GetResult());
}

void test_RamTst_GetResult_AfterRun(void)
{
    RamTst_Init(&g_test_cfg);
    RamTst_Run();
    RamTst_TestResultType result = RamTst_GetResult();
    TEST_ASSERT_TRUE(result == RAMTST_RESULT_OK || result == RAMTST_RESULT_FAILED || result == RAMTST_RESULT_NOT_TESTED);
}

/* ========= RamTst_GetStatus ========= */
void test_RamTst_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(RAMTST_STATUS_UNINIT, RamTst_GetStatus());
}

/* ========= RamTst_SetMode / GetMode ========= */
void test_RamTst_SetMode_Valid(void)
{
    RamTst_Init(&g_test_cfg);
    Std_ReturnType ret = RamTst_SetMode(0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    RamTst_ModeType mode = RamTst_GetMode();
    TEST_ASSERT_EQUAL(0, mode);
}

void test_RamTst_SetMode_BeforeInit(void)
{
    Std_ReturnType ret = RamTst_SetMode(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ========= RamTst_MainFunction ========= */
void test_RamTst_MainFunction_Idle(void)
{
    RamTst_Init(&g_test_cfg);
    RamTst_MainFunction();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetStatus());
}

void test_RamTst_MainFunction_Uninit(void)
{
    RamTst_MainFunction();
}

/* ========= RamTst_GetVersionInfo ========= */
void test_RamTst_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    RamTst_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(RAMTST_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(RAMTST_MODULE_ID, vi.moduleID);
}

void test_RamTst_GetVersionInfo_Null(void)
{
    RamTst_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_RamTst_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_RamTst_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_RamTst_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_RamTst_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_RamTst_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_RamTst_Run_BeforeInit, "Run before init", __LINE__);
    UnityRunTest(test_RamTst_Run_Valid, "Run valid", __LINE__);
    UnityRunTest(test_RamTst_Stop_BeforeInit, "Stop before init", __LINE__);
    UnityRunTest(test_RamTst_Stop_AfterInit, "Stop after init", __LINE__);
    UnityRunTest(test_RamTst_GetResult_BeforeInit, "GetResult before init", __LINE__);
    UnityRunTest(test_RamTst_GetResult_AfterRun, "GetResult after run", __LINE__);
    UnityRunTest(test_RamTst_GetStatus_Uninit, "GetStatus uninit", __LINE__);
    UnityRunTest(test_RamTst_SetMode_Valid, "SetMode valid", __LINE__);
    UnityRunTest(test_RamTst_SetMode_BeforeInit, "SetMode before init", __LINE__);
    UnityRunTest(test_RamTst_MainFunction_Idle, "MainFunction idle", __LINE__);
    UnityRunTest(test_RamTst_MainFunction_Uninit, "MainFunction uninit", __LINE__);
    UnityRunTest(test_RamTst_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_RamTst_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

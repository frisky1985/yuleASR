/**
 * @file test_mcal_eep.c
 * @brief Eep unit test — links real Eep.c with MockHAL
 *
 * AUTOSAR EEPROM Driver (Eep) — MCAL Layer
 * Tests: Init/DeInit/Read/Write/Erase/GetStatus/GetJobResult/Cancel/MainFunction/VersionInfo
 *
 * Note: Read/Write/Erase VALID paths access physical memory (hardware-dependent).
 * Only error paths are tested on host to avoid segfaults.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Eep.h"

static Eep_ConfigType g_cfg;

void setUp(void) {
    mock_hal_reset();
    /* DeInit first to reset Eep_State between tests */
    Eep_DeInit();
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.BaseAddress = 0;
    g_cfg.Size = 4096;
    g_cfg.PageSize = 16;
    g_cfg.JobCallCycle = 1;
    g_cfg.PollingMode = FALSE;  /* Don't auto-process (avoids memory deref) */
}
void tearDown(void) {
    /* Reset Eep_State after each test */
    Eep_DeInit();
}

/* ========= Eep_Init ========= */
void test_Eep_Init_NullConfig(void)
{
    Eep_Init(NULL);
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

void test_Eep_Init_Valid(void)
{
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
    TEST_ASSERT_EQUAL(EEP_JOB_OK, Eep_GetJobResult());
}

void test_Eep_Init_DoubleInit(void)
{
    Eep_Init(&g_cfg);
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/* ========= Eep_DeInit ========= */
void test_Eep_DeInit_BeforeInit(void)
{
    Eep_DeInit();
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

void test_Eep_DeInit_AfterInit(void)
{
    Eep_Init(&g_cfg);
    Eep_DeInit();
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

/* ========= Eep_Read (error paths only) ========= */
void test_Eep_Read_NullBuffer(void)
{
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Eep_Read(0, NULL, 16));
}

void test_Eep_Read_InvalidAddress(void)
{
    Eep_Init(&g_cfg);
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Eep_Read(4096, buf, 16));
}

void test_Eep_Read_ZeroLength(void)
{
    Eep_Init(&g_cfg);
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Eep_Read(0, buf, 0));
}

/* ========= Eep_Write (error paths only) ========= */
void test_Eep_Write_NullData(void)
{
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Eep_Write(0, NULL, 4));
}

void test_Eep_Write_OutOfBounds(void)
{
    Eep_Init(&g_cfg);
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_NOT_OK, Eep_Write(4096, data, 1));
}

/* ========= Eep_Erase (error paths only) ========= */
void test_Eep_Erase_Invalid(void)
{
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Eep_Erase(0xFFFFFFFF, 1));
}

/* ========= Eep_Cancel ========= */
void test_Eep_Cancel_AfterInit(void)
{
    Eep_Init(&g_cfg);
#if (EEP_CANCEL_API == STD_ON)
    Eep_Cancel();
#endif
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

/* ========= Eep_GetStatus / Eep_GetJobResult ========= */
void test_Eep_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(EEP_UNINIT, Eep_GetStatus());
}

void test_Eep_GetJobResult_Uninit(void)
{
    TEST_ASSERT_EQUAL(EEP_JOB_OK, Eep_GetJobResult());
}

void test_Eep_GetStatus_Idle(void)
{
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

void test_Eep_GetJobResult_AfterInit(void)
{
    Eep_Init(&g_cfg);
    TEST_ASSERT_EQUAL(EEP_JOB_OK, Eep_GetJobResult());
}

/* ========= Eep_MainFunction ========= */
void test_Eep_MainFunction_Idle(void)
{
    Eep_Init(&g_cfg);
    Eep_MainFunction();
    TEST_ASSERT_EQUAL(EEP_IDLE, Eep_GetStatus());
}

void test_Eep_MainFunction_Uninit(void)
{
    Eep_MainFunction();
}

/* ========= Eep_GetVersionInfo ========= */
void test_Eep_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Eep_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(EEP_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(EEP_MODULE_ID, vi.moduleID);
}

void test_Eep_GetVersionInfo_Null(void)
{
    Eep_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Eep_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Eep_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Eep_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Eep_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_Eep_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Eep_Read_NullBuffer, "Read null buffer", __LINE__);
    UnityRunTest(test_Eep_Read_InvalidAddress, "Read invalid address", __LINE__);
    UnityRunTest(test_Eep_Read_ZeroLength, "Read zero length", __LINE__);
    UnityRunTest(test_Eep_Write_NullData, "Write null data", __LINE__);
    UnityRunTest(test_Eep_Write_OutOfBounds, "Write out of bounds", __LINE__);
    UnityRunTest(test_Eep_Erase_Invalid, "Erase invalid", __LINE__);
    UnityRunTest(test_Eep_Cancel_AfterInit, "Cancel after init", __LINE__);
    UnityRunTest(test_Eep_GetStatus_Uninit, "GetStatus uninit", __LINE__);
    UnityRunTest(test_Eep_GetJobResult_Uninit, "GetJobResult uninit", __LINE__);
    UnityRunTest(test_Eep_GetStatus_Idle, "GetStatus idle", __LINE__);
    UnityRunTest(test_Eep_GetJobResult_AfterInit, "GetJobResult after init", __LINE__);
    UnityRunTest(test_Eep_MainFunction_Idle, "MainFunction idle", __LINE__);
    UnityRunTest(test_Eep_MainFunction_Uninit, "MainFunction uninit", __LINE__);
    UnityRunTest(test_Eep_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Eep_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

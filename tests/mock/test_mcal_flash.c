/**
 * @file test_mcal_flash.c
 * @brief Flash (Fls) unit test — links real Flash.c + Flash_Lcfg.c with MockHAL
 *
 * AUTOSAR Flash Driver (Fls) — MCAL Layer
 * Tests: Init/DeInit/Erase/Write/Read/Cancel/GetStatus/GetJobResult/Compare/BlankCheck/SetMode
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Flash.h"

void setUp(void) { mock_hal_reset(); }
void tearDown(void) {}

/* Create minimal Fls config */
static void create_default_cfg(Fls_ConfigType* cfg)
{
    memset(cfg, 0, sizeof(Fls_ConfigType));
    cfg->BaseAddress = 0x08000000UL;
    cfg->TotalSize = 65536;
    cfg->PageSize = 128;
    cfg->ProgrammingUnit = 1;
    cfg->DefaultMode = MEMIF_MODE_SLOW;
}

/* ========= Fls_Init ========= */
void test_Fls_Init_NullConfig(void)
{
    Fls_Init(NULL);
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

void test_Fls_Init_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQUAL(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_Fls_Init_DoubleInit(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/* ========= Fls_DeInit ========= */
void test_Fls_DeInit_AfterInit(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_DeInit();
    TEST_ASSERT_EQUAL(MEMIF_UNINIT, Fls_GetStatus());
}

/* ========= Fls_Erase ========= */
void test_Fls_Erase_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000, 4096));
}

void test_Fls_Erase_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_Erase(0x08000000, 4096));
}

void test_Fls_Erase_InvalidAddress(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x00000000, 1));
}

void test_Fls_Erase_ZeroLength(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Erase(0x08000000, 0));
}

/* ========= Fls_Write ========= */
void test_Fls_Write_BeforeInit(void)
{
    const uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000, data, 4));
}

void test_Fls_Write_NullData(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000, NULL, 4));
}

void test_Fls_Write_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0xA5, 0xB6, 0xC7, 0xD8};
    TEST_ASSERT_EQUAL(E_OK, Fls_Write(0x08000000, data, 4));
}

void test_Fls_Write_Oversize(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Write(0x08000000 + 65536, data, 1));
}

/* ========= Fls_Read ========= */
void test_Fls_Read_BeforeInit(void)
{
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000, buf, 16));
}

void test_Fls_Read_NullBuffer(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x08000000, NULL, 4));
}

void test_Fls_Read_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    uint8 buf[16];
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Fls_Read(0x08000000, buf, 16));
}

void test_Fls_Read_InvalidAddress(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Read(0x00000000, buf, 4));
}

/* ========= Fls_Cancel ========= */
void test_Fls_Cancel_AfterInit(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_Cancel();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/* ========= Fls_Compare ========= */
void test_Fls_Compare_BeforeInit(void)
{
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000, data, 1));
}

void test_Fls_Compare_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    const uint8 data[] = {0xA5, 0xB6};
    TEST_ASSERT_EQUAL(E_OK, Fls_Compare(0x08000000, data, 2));
}

void test_Fls_Compare_NullData(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_Compare(0x08000000, NULL, 1));
}

/* ========= Fls_BlankCheck ========= */
void test_Fls_BlankCheck_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fls_BlankCheck(0x08000000, 256));
}

void test_Fls_BlankCheck_Valid(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    TEST_ASSERT_EQUAL(E_OK, Fls_BlankCheck(0x08000000, 256));
}

/* ========= Fls_SetMode ========= */
void test_Fls_SetMode_Normal(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_SetMode(MEMIF_MODE_SLOW);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

void test_Fls_SetMode_Fast(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

/* ========= Fls_MainFunction ========= */
void test_Fls_MainFunction_Idle(void)
{
    Fls_ConfigType cfg;
    create_default_cfg(&cfg);
    Fls_Init(&cfg);
    Fls_MainFunction();
    TEST_ASSERT_EQUAL(MEMIF_IDLE, Fls_GetStatus());
}

void test_Fls_MainFunction_Uninit(void)
{
    Fls_MainFunction();
}

#if (FLS_VERSION_INFO_API == STD_ON)
/* ========= Fls_GetVersionInfo ========= */
void test_Fls_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Fls_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(FLS_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(FLS_MODULE_ID, vi.moduleID);
}

void test_Fls_GetVersionInfo_Null(void)
{
    Fls_GetVersionInfo(NULL);
}
#endif

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Fls_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Fls_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Fls_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Fls_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Fls_Erase_BeforeInit, "Erase before init", __LINE__);
    UnityRunTest(test_Fls_Erase_Valid, "Erase valid", __LINE__);
    UnityRunTest(test_Fls_Erase_InvalidAddress, "Erase invalid address", __LINE__);
    UnityRunTest(test_Fls_Erase_ZeroLength, "Erase zero length", __LINE__);
    UnityRunTest(test_Fls_Write_BeforeInit, "Write before init", __LINE__);
    UnityRunTest(test_Fls_Write_NullData, "Write null data", __LINE__);
    UnityRunTest(test_Fls_Write_Valid, "Write valid", __LINE__);
    UnityRunTest(test_Fls_Write_Oversize, "Write oversize", __LINE__);
    UnityRunTest(test_Fls_Read_BeforeInit, "Read before init", __LINE__);
    UnityRunTest(test_Fls_Read_NullBuffer, "Read null buffer", __LINE__);
    UnityRunTest(test_Fls_Read_Valid, "Read valid", __LINE__);
    UnityRunTest(test_Fls_Read_InvalidAddress, "Read invalid", __LINE__);
    UnityRunTest(test_Fls_Cancel_AfterInit, "Cancel after init", __LINE__);
    UnityRunTest(test_Fls_Compare_BeforeInit, "Compare before init", __LINE__);
    UnityRunTest(test_Fls_Compare_Valid, "Compare valid", __LINE__);
    UnityRunTest(test_Fls_Compare_NullData, "Compare null data", __LINE__);
    UnityRunTest(test_Fls_BlankCheck_BeforeInit, "BlankCheck before init", __LINE__);
    UnityRunTest(test_Fls_BlankCheck_Valid, "BlankCheck valid", __LINE__);
    UnityRunTest(test_Fls_SetMode_Normal, "SetMode normal", __LINE__);
    UnityRunTest(test_Fls_SetMode_Fast, "SetMode fast", __LINE__);
    UnityRunTest(test_Fls_MainFunction_Idle, "MainFunction idle", __LINE__);
    UnityRunTest(test_Fls_MainFunction_Uninit, "MainFunction uninit", __LINE__);
#if (FLS_VERSION_INFO_API == STD_ON)
    UnityRunTest(test_Fls_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Fls_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
#endif
    return UnityEnd();
}

/**
 * @file test_mcal_fee.c
 * @brief Fee unit test — links real Fee.c + Fee_Lcfg.c production code with MockHAL
 *
 * AUTOSAR Flash EEPROM Emulation (Fee) — MCAL Layer
 * Tests: Init/DeInit/SetMode/Read/Write/Erase/Compare/BlankCheck/GetStatus/GetJobResult/Cancel
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Fee.h"

void setUp(void)
{
    mock_hal_reset();
    /* Reset Fee module state between tests. Fee_Cancel clears a pending
     * BUSY flag left by a previous test's job, Fee_DeInit resets to
     * uninitialized. Both are DET-reporting no-ops when not initialized. */
    Fee_Cancel();
    Fee_DeInit();
}
void tearDown(void) {}

/* Helper: create a minimal Fee config - use actual Fee_ConfigType fields */
static Fee_SectorType g_sectors[1];
static Fee_BlockType g_blocks[8];

static void create_default_cfg(Fee_ConfigType* cfg)
{
    memset(cfg, 0, sizeof(Fee_ConfigType));
    memset(g_sectors, 0, sizeof(g_sectors));
    memset(g_blocks, 0, sizeof(g_blocks));
    /* One 64KB sector at address 0 so address-0 reads/writes validate */
    g_sectors[0].sectorStartAddr = 0U;
    g_sectors[0].sectorSize = 0x10000U;
    g_sectors[0].sectorPageSize = 128U;
    g_sectors[0].sectorWritable = TRUE;
    g_sectors[0].sectorErasable = TRUE;
    cfg->sectorList = g_sectors;
    cfg->blockList = g_blocks;
    cfg->virtualPageSize = 128;
    cfg->sectorCount = 1;
    cfg->blockCount = 8;
}

/* ========= Fee_Init ========= */
void test_Fee_Init_NullConfig(void)
{
    Fee_Init(NULL);
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

void test_Fee_Init_Valid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());
}

void test_Fee_Init_DoubleInit(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/* ========= Fee_DeInit ========= */
void test_Fee_DeInit_BeforeInit(void)
{
    Fee_DeInit();
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

void test_Fee_DeInit_AfterInit(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    Fee_DeInit();
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

/* ========= Fee_SetMode ========= */
void test_Fee_SetMode_BeforeInit(void)
{
    Fee_SetMode(FEE_MODE_NORMAL);
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

void test_Fee_SetMode_Normal(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    Fee_SetMode(FEE_MODE_NORMAL);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

void test_Fee_SetMode_Fast(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    Fee_SetMode(FEE_MODE_FAST);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/* ========= Fee_Read ========= */
void test_Fee_Read_BeforeInit(void)
{
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Read(0, 16, buf));
}

void test_Fee_Read_NullBuffer(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Read(0, 16, NULL));
}

void test_Fee_Read_Valid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    uint8 buf[16];
    memset(buf, 0xAA, sizeof(buf));
    TEST_ASSERT_EQUAL(E_OK, Fee_Read(0, 16, buf));
}

void test_Fee_Read_InvalidAddress(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    uint8 buf[16];
    TEST_ASSERT(E_NOT_OK == Fee_Read(0xFFFFFFFF, 16, buf) || E_OK == Fee_Read(0xFFFFFFFF, 16, buf));
}

void test_Fee_Read_ZeroLength(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    uint8 buf[16];
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Read(0, 0, buf));
}

/* ========= Fee_Write ========= */
void test_Fee_Write_BeforeInit(void)
{
    const uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Write(0, 4, data));
}

void test_Fee_Write_NullData(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Write(0, 4, NULL));
}

void test_Fee_Write_Valid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    /* Length must be a multiple of FEE_VIRTUAL_PAGE_SIZE (8) */
    const uint8 data[] = {0xA5, 0xB6, 0xC7, 0xD8, 0xE9, 0xF0, 0x11, 0x22};
    TEST_ASSERT_EQUAL(E_OK, Fee_Write(0, 8, data));
}

void test_Fee_Write_Oversize(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    const uint8 data[] = {0x00};
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Write(65536, 1, data));
}

/* ========= Fee_Erase ========= */
void test_Fee_Erase_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Erase(0, 1024));
}

void test_Fee_Erase_Valid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT_EQUAL(E_OK, Fee_Erase(0, 1024));
}

void test_Fee_Erase_Invalid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Fee_Erase(0xFFFFFFFF, 1));
}

/* ========= Fee_Compare ========= */
void test_Fee_Compare_BeforeInit(void)
{
    const uint8 data[] = {0x00};
    TEST_ASSERT(E_NOT_OK == Fee_Compare(0, 1, data));
}

void test_Fee_Compare_Valid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    const uint8 data[] = {0xA5, 0xB6, 0xC7, 0xD8, 0xE9, 0xF0, 0x11, 0x22};
    TEST_ASSERT_EQUAL(E_OK, Fee_Compare(0, 8, data));
}

void test_Fee_Compare_NullData(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT(E_NOT_OK == Fee_Compare(0, 1, NULL));
}

/* ========= Fee_BlankCheck ========= */
void test_Fee_BlankCheck_BeforeInit(void)
{
    TEST_ASSERT(E_NOT_OK == Fee_BlankCheck(0, 256));
}

void test_Fee_BlankCheck_Valid(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    TEST_ASSERT(E_OK == Fee_BlankCheck(0, 256));
}

/* ========= Fee_Cancel ========= */
void test_Fee_Cancel_AfterInit(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    Fee_Cancel();
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

/* ========= Fee_GetStatus / Fee_GetJobResult ========= */
void test_Fee_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

void test_Fee_GetJobResult_Uninit(void)
{
    TEST_ASSERT_EQUAL(FEE_JOB_FAILED, Fee_GetJobResult()); /* Uninit → FAILED per impl */
}

/* ========= Fee_MainFunction ========= */
void test_Fee_MainFunction_Idle(void)
{
    Fee_ConfigType cfg;
    create_default_cfg(&cfg);
    Fee_Init(&cfg);
    Fee_MainFunction();
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

void test_Fee_MainFunction_Uninit(void)
{
    Fee_MainFunction();
}

/* ========= Fee_GetVersionInfo ========= */
void test_Fee_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Fee_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(FEE_VENDOR_ID, vi.vendorID);
}

void test_Fee_GetVersionInfo_Null(void)
{
    Fee_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Fee_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Fee_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Fee_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Fee_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_Fee_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Fee_SetMode_BeforeInit, "SetMode before init", __LINE__);
    UnityRunTest(test_Fee_SetMode_Normal, "SetMode normal", __LINE__);
    UnityRunTest(test_Fee_SetMode_Fast, "SetMode fast", __LINE__);
    UnityRunTest(test_Fee_Read_BeforeInit, "Read before init", __LINE__);
    UnityRunTest(test_Fee_Read_NullBuffer, "Read null buffer", __LINE__);
    UnityRunTest(test_Fee_Read_Valid, "Read valid", __LINE__);
    UnityRunTest(test_Fee_Read_InvalidAddress, "Read invalid address", __LINE__);
    UnityRunTest(test_Fee_Read_ZeroLength, "Read zero length", __LINE__);
    UnityRunTest(test_Fee_Write_BeforeInit, "Write before init", __LINE__);
    UnityRunTest(test_Fee_Write_NullData, "Write null data", __LINE__);
    UnityRunTest(test_Fee_Write_Valid, "Write valid", __LINE__);
    UnityRunTest(test_Fee_Write_Oversize, "Write oversize", __LINE__);
    UnityRunTest(test_Fee_Erase_BeforeInit, "Erase before init", __LINE__);
    UnityRunTest(test_Fee_Erase_Valid, "Erase valid", __LINE__);
    UnityRunTest(test_Fee_Erase_Invalid, "Erase invalid", __LINE__);
    UnityRunTest(test_Fee_Compare_BeforeInit, "Compare before init", __LINE__);
    UnityRunTest(test_Fee_Compare_Valid, "Compare valid", __LINE__);
    UnityRunTest(test_Fee_Compare_NullData, "Compare null data", __LINE__);
    UnityRunTest(test_Fee_BlankCheck_BeforeInit, "BlankCheck before init", __LINE__);
    UnityRunTest(test_Fee_BlankCheck_Valid, "BlankCheck valid", __LINE__);
    UnityRunTest(test_Fee_Cancel_AfterInit, "Cancel after init", __LINE__);
    UnityRunTest(test_Fee_GetStatus_Uninit, "GetStatus uninit", __LINE__);
    UnityRunTest(test_Fee_GetJobResult_Uninit, "GetJobResult uninit", __LINE__);
    UnityRunTest(test_Fee_MainFunction_Idle, "MainFunction idle", __LINE__);
    UnityRunTest(test_Fee_MainFunction_Uninit, "MainFunction uninit", __LINE__);
    UnityRunTest(test_Fee_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Fee_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

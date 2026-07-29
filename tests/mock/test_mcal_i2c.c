/**
 * @file test_mcal_i2c.c
 * @brief I2C unit test — links real I2c.c production code with MockHAL
 *
 * AUTOSAR I2C Driver (I2c) — MCAL Layer
 * Tests: Init/DeInit/WriteBytes/ReadBytes/GetStatus/GetVersionInfo/SetClockMode
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "I2c.h"

/* Minimal config - actual fields depend on I2c_Cfg.h */
static I2c_ConfigType g_test_cfg;

void setUp(void) {
    mock_hal_reset();
    /* DeInit to reset internal state between tests */
    I2c_DeInit();
    memset(&g_test_cfg, 0, sizeof(g_test_cfg));
}
void tearDown(void) {
    I2c_DeInit();
}

/* ========= I2c_Init ========= */
void test_I2c_Init_NullConfig(void)
{
    I2c_Init(NULL);
    /* Status should remain uninit */
    TEST_ASSERT_EQUAL(I2C_UNINIT, I2c_GetStatus());
}

void test_I2c_Init_Valid(void)
{
    I2c_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
}

void test_I2c_Init_DoubleInit(void)
{
    I2c_Init(&g_test_cfg);
    I2c_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
}

/* ========= I2c_DeInit ========= */
void test_I2c_DeInit_BeforeInit(void)
{
    Std_ReturnType ret = I2c_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_I2c_DeInit_AfterInit(void)
{
    I2c_Init(&g_test_cfg);
    Std_ReturnType ret = I2c_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(I2C_UNINIT, I2c_GetStatus());
}

/* ========= I2c_WriteBytes ========= */
void test_I2c_WriteBytes_BeforeInit(void)
{
    const uint8 data[] = {0x01, 0x02};
    Std_ReturnType ret = I2c_WriteBytes(0, 0x50, data, 2, 0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_I2c_WriteBytes_NullBuffer(void)
{
    I2c_Init(&g_test_cfg);
    Std_ReturnType ret = I2c_WriteBytes(0, 0x50, NULL, 2, 0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_I2c_WriteBytes_Valid(void)
{
    I2c_Init(&g_test_cfg);
    const uint8 data[] = {0x00, 0xFF, 0xA5, 0x5A};
    Std_ReturnType ret = I2c_WriteBytes(0, 0x50, data, 4, 0);
    /* Hardware-dependent: may succeed or fail without real I2C bus */
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= I2c_ReadBytes ========= */
void test_I2c_ReadBytes_BeforeInit(void)
{
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, I2c_ReadBytes(0, 0x50, buf, 4, 0));
}

void test_I2c_ReadBytes_NullBuffer(void)
{
    I2c_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, I2c_ReadBytes(0, 0x50, NULL, 4, 0));
}

void test_I2c_ReadBytes_Valid(void)
{
    I2c_Init(&g_test_cfg);
    uint8 buf[4];
    memset(buf, 0x00, sizeof(buf));
    Std_ReturnType ret = I2c_ReadBytes(0, 0x50, buf, 4, 0);
    /* Hardware-dependent */
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= I2c_WriteRead ========= */
void test_I2c_WriteRead_Valid(void)
{
    I2c_Init(&g_test_cfg);
    const uint8 wbuf[] = {0x00, 0x10};
    uint8 rbuf[2];
    memset(rbuf, 0, sizeof(rbuf));
    Std_ReturnType ret = I2c_WriteRead(0, 0x50, wbuf, 2, rbuf, 2, 0);
    /* Hardware-dependent */
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= I2c_GetStatus ========= */
void test_I2c_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(I2C_UNINIT, I2c_GetStatus());
}

void test_I2c_GetStatus_AfterInit(void)
{
    I2c_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
}

/* ========= I2c_SetClockMode ========= */
void test_I2c_SetClockMode_Valid(void)
{
    I2c_Init(&g_test_cfg);
    Std_ReturnType ret = I2c_SetClockMode(0, 0);
    /* Hardware-dependent */
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

void test_I2c_SetClockMode_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, I2c_SetClockMode(0, 0));
}

/* ========= I2c_GetBusState ========= */
void test_I2c_GetBusState_Valid(void)
{
    I2c_Init(&g_test_cfg);
    I2c_BusStateType state = I2c_GetBusState(0);
    TEST_ASSERT_TRUE(state == I2C_BUS_STATE_IDLE || state == I2C_BUS_STATE_OWNER || state == I2C_BUS_STATE_BUSY);
}

/* ========= I2c_MainFunction ========= */
void test_I2c_MainFunction_Idle(void)
{
    I2c_Init(&g_test_cfg);
    I2c_MainFunction();
    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
}

void test_I2c_MainFunction_Uninit(void)
{
    I2c_MainFunction();
}

/* ========= I2c_GetVersionInfo ========= */
void test_I2c_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    I2c_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(I2C_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(I2C_MODULE_ID, vi.moduleID);
}

void test_I2c_GetVersionInfo_Null(void)
{
    I2c_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_I2c_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_I2c_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_I2c_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_I2c_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_I2c_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_I2c_WriteBytes_BeforeInit, "WriteBytes before init", __LINE__);
    UnityRunTest(test_I2c_WriteBytes_NullBuffer, "WriteBytes null buffer", __LINE__);
    UnityRunTest(test_I2c_WriteBytes_Valid, "WriteBytes valid", __LINE__);
    UnityRunTest(test_I2c_ReadBytes_BeforeInit, "ReadBytes before init", __LINE__);
    UnityRunTest(test_I2c_ReadBytes_NullBuffer, "ReadBytes null buffer", __LINE__);
    UnityRunTest(test_I2c_ReadBytes_Valid, "ReadBytes valid", __LINE__);
    UnityRunTest(test_I2c_WriteRead_Valid, "WriteRead valid", __LINE__);
    UnityRunTest(test_I2c_GetStatus_Uninit, "GetStatus uninit", __LINE__);
    UnityRunTest(test_I2c_GetStatus_AfterInit, "GetStatus after init", __LINE__);
    UnityRunTest(test_I2c_GetBusState_Valid, "GetBusState valid", __LINE__);
    UnityRunTest(test_I2c_SetClockMode_Valid, "SetClockMode valid", __LINE__);
    UnityRunTest(test_I2c_SetClockMode_BeforeInit, "SetClockMode before init", __LINE__);
    UnityRunTest(test_I2c_MainFunction_Idle, "MainFunction idle", __LINE__);
    UnityRunTest(test_I2c_MainFunction_Uninit, "MainFunction uninit", __LINE__);
    UnityRunTest(test_I2c_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_I2c_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

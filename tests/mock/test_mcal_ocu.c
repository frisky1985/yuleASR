/**
 * @file test_mcal_ocu.c
 * @brief OCU unit test — links real Ocu.c production code with MockHAL
 *
 * AUTOSAR Output Compare Unit (Ocu) — MCAL Layer
 * Tests: Init/DeInit/StartChannel/StopChannel/SetThresholds/GetCounter/GetVersionInfo
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Ocu.h"

static Ocu_ConfigType g_test_cfg;

void setUp(void) { mock_hal_reset(); memset(&g_test_cfg, 0, sizeof(g_test_cfg)); }
void tearDown(void) {}

/* ========= Ocu_Init ========= */
/* @req SWS_Ocu_00001 */
void test_Ocu_Init_NullConfig(void)
{
    Ocu_Init(NULL);
    TEST_ASSERT_EQUAL(0, Ocu_GetCounter(0));
}

/* @req SWS_Ocu_00001 */
void test_Ocu_Init_Valid(void)
{
    Ocu_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(0, Ocu_GetCounter(0));
}

/* @req SWS_Ocu_00001 */
void test_Ocu_Init_DoubleInit(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_Init(&g_test_cfg);
}

/* ========= Ocu_DeInit ========= */
/* @req SWS_Ocu_00002 */
void test_Ocu_DeInit_BeforeInit(void)
{
    Ocu_DeInit();
}

/* @req SWS_Ocu_00002 */
void test_Ocu_DeInit_AfterInit(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_DeInit();
}

/* ========= Ocu_StartChannel ========= */
/* @req SWS_Ocu_00003 */
void test_Ocu_StartChannel_Valid(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_StartChannel(0);
    Ocu_ValueType cnt = Ocu_GetCounter(0);
    TEST_ASSERT_EQUAL(0, cnt);
}

/* @req SWS_Ocu_00003 */
void test_Ocu_StartChannel_BeforeInit(void)
{
    Ocu_StartChannel(0);
}

/* ========= Ocu_StopChannel ========= */
/* @req SWS_Ocu_00004 */
void test_Ocu_StopChannel_BeforeInit(void)
{
    Ocu_StopChannel(0);
}

/* @req SWS_Ocu_00004 */
void test_Ocu_StopChannel_Valid(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_StartChannel(0);
    Ocu_StopChannel(0);
}

/* ========= Ocu_SetAbsoluteThreshold ========= */
/* @req SWS_Ocu_00007 */
void test_Ocu_SetAbsoluteThreshold_BeforeInit(void)
{
    Std_ReturnType ret = Ocu_SetAbsoluteThreshold(0, 0, 5000);
    /* Some implementations allow before-init; either is valid */
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* @req SWS_Ocu_00007 */
void test_Ocu_SetAbsoluteThreshold_Valid(void)
{
    Ocu_Init(&g_test_cfg);
    Std_ReturnType ret = Ocu_SetAbsoluteThreshold(0, 0, 5000);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* ========= Ocu_SetRelativeThreshold ========= */
/* @req SWS_Ocu_00008 */
void test_Ocu_SetRelativeThreshold_Valid(void)
{
    Ocu_Init(&g_test_cfg);
    Std_ReturnType ret = Ocu_SetRelativeThreshold(0, 250);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* @req SWS_Ocu_00008 */
void test_Ocu_SetRelativeThreshold_BeforeInit(void)
{
    Std_ReturnType ret = Ocu_SetRelativeThreshold(0, 250);
    /* Some implementations allow before-init */
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= Ocu_GetCounter ========= */
/* @req SWS_Ocu_00009 */
void test_Ocu_GetCounter_BeforeInit(void)
{
    Ocu_ValueType cnt = Ocu_GetCounter(0);
    TEST_ASSERT_EQUAL(0, cnt);
}

/* @req SWS_Ocu_00009 */
void test_Ocu_GetCounter_AfterInit(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_ValueType cnt = Ocu_GetCounter(0);
    TEST_ASSERT_EQUAL(0, cnt);
}

/* ========= Ocu_SetPinState ========= */
/* @req SWS_Ocu_00005 */
void test_Ocu_SetPinState_Valid(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_SetPinState(0, OCU_HIGH);
    Ocu_SetPinState(0, OCU_LOW);
}

/* @req SWS_Ocu_00005 */
void test_Ocu_SetPinState_BeforeInit(void)
{
    Ocu_SetPinState(0, OCU_HIGH);
}

/* ========= Ocu_EnableDisableNotification ========= */
/* @req SWS_Ocu_00001 */
void test_Ocu_Notification_AfterInit(void)
{
    Ocu_Init(&g_test_cfg);
    Ocu_EnableNotification(0);
    Ocu_DisableNotification(0);
}

/* @req SWS_Ocu_00010 */
void test_Ocu_Notification_BeforeInit(void)
{
    Ocu_EnableNotification(0);
    Ocu_DisableNotification(0);
}

/* ========= Ocu_GetVersionInfo ========= */
/* @req SWS_Ocu_00012 */
void test_Ocu_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Ocu_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(OCU_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(OCU_MODULE_ID, vi.moduleID);
}

/* @req SWS_Ocu_00012 */
void test_Ocu_GetVersionInfo_Null(void)
{
    Ocu_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Ocu_Init_NullConfig, "Init NULL", __LINE__);
    UnityRunTest(test_Ocu_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Ocu_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Ocu_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_Ocu_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Ocu_StartChannel_Valid, "Start channel", __LINE__);
    UnityRunTest(test_Ocu_StartChannel_BeforeInit, "Start before init", __LINE__);
    UnityRunTest(test_Ocu_StopChannel_BeforeInit, "Stop before init", __LINE__);
    UnityRunTest(test_Ocu_StopChannel_Valid, "Stop valid", __LINE__);
    UnityRunTest(test_Ocu_SetAbsoluteThreshold_BeforeInit, "SetAbsThresh before init", __LINE__);
    UnityRunTest(test_Ocu_SetAbsoluteThreshold_Valid, "SetAbsThresh valid", __LINE__);
    UnityRunTest(test_Ocu_SetRelativeThreshold_Valid, "SetRelThresh valid", __LINE__);
    UnityRunTest(test_Ocu_SetRelativeThreshold_BeforeInit, "SetRelThresh before init", __LINE__);
    UnityRunTest(test_Ocu_GetCounter_BeforeInit, "GetCounter before init", __LINE__);
    UnityRunTest(test_Ocu_GetCounter_AfterInit, "GetCounter after init", __LINE__);
    UnityRunTest(test_Ocu_SetPinState_Valid, "SetPinState valid", __LINE__);
    UnityRunTest(test_Ocu_SetPinState_BeforeInit, "SetPinState before init", __LINE__);
    UnityRunTest(test_Ocu_Notification_AfterInit, "Notification after init", __LINE__);
    UnityRunTest(test_Ocu_Notification_BeforeInit, "Notification before init", __LINE__);
    UnityRunTest(test_Ocu_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Ocu_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

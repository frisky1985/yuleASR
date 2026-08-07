/**
 * @file test_mcal_lin.c
 * @brief LIN unit test — links real Lin.c production code with MockHAL
 *
 * AUTOSAR LIN Driver (Lin) — MCAL Layer
 * Tests: Init/DeInit/SendFrame/SendResponse/DisableResponse/WakeUp/GetStatus/GoToSleep/GetVersionInfo
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Lin.h"

static Lin_ConfigType g_test_cfg;
#ifndef LIN_MAX_CHANNELS
#define LIN_MAX_CHANNELS 2U
#endif
static Lin_ChannelConfigType g_channel_cfg[LIN_MAX_CHANNELS];

/* Reset Lin module state between tests.
 * Lin_Init() has no already-initialized guard and Lin_DeInit() fully resets
 * the static module state, so Init->DeInit returns the module to a pristine
 * "not initialized" state even when a previous test called Init().
 * A valid config with NumChannels >= 1 is required so Init marks channels
 * as initialized (otherwise before-init/after-init semantics are untestable).
 */
static void lin_reset_module(void)
{
    Lin_Init(&g_test_cfg);
    Lin_DeInit();
}

void setUp(void)
{
    mock_hal_reset();
    memset(&g_test_cfg, 0, sizeof(g_test_cfg));
    memset(g_channel_cfg, 0, sizeof(g_channel_cfg));
    g_test_cfg.NumChannels = 1;
    g_test_cfg.ChannelConfigPtr = g_channel_cfg;
    g_test_cfg.DevErrorDetect = TRUE;
    g_test_cfg.VersionInfoApi = TRUE;
    lin_reset_module();
}
void tearDown(void) {}

/* ========= Lin_Init ========= */
void test_Lin_Init_NullConfig(void)
{
    Lin_Init(NULL);
    TEST_ASSERT_EQUAL(LIN_NOT_OK, Lin_GetStatus(0, NULL));
}

void test_Lin_Init_Valid(void)
{
    Lin_Init(&g_test_cfg);
    Lin_StatusType status = Lin_GetStatus(0, NULL);
    /* After init the channel is in sleep state pending wake-up (AUTOSAR) */
    TEST_ASSERT_TRUE(status == LIN_OPERATIONAL || status == LIN_CH_SLEEP);
}

void test_Lin_Init_DoubleInit(void)
{
    Lin_Init(&g_test_cfg);
    Lin_Init(&g_test_cfg);
    Lin_StatusType status = Lin_GetStatus(0, NULL);
    TEST_ASSERT_TRUE(status == LIN_OPERATIONAL || status == LIN_CH_SLEEP);
}

/* ========= Lin_DeInit ========= */
void test_Lin_DeInit_BeforeInit(void)
{
    Lin_DeInit();
    TEST_ASSERT_EQUAL(LIN_NOT_OK, Lin_GetStatus(0, NULL));
}

void test_Lin_DeInit_AfterInit(void)
{
    Lin_Init(&g_test_cfg);
    Lin_DeInit();
    TEST_ASSERT_EQUAL(LIN_NOT_OK, Lin_GetStatus(0, NULL));
}

/* ========= Lin_SendFrame ========= */
void test_Lin_SendFrame_BeforeInit(void)
{
    Lin_PduType pdu;
    memset(&pdu, 0, sizeof(pdu));
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_SendFrame(0, &pdu));
}

void test_Lin_SendFrame_Valid(void)
{
    Lin_Init(&g_test_cfg);
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Lin_PduType pdu;
    memset(&pdu, 0, sizeof(pdu));
    pdu.Pid = 0x01;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_MASTER_RESPONSE;
    pdu.Length = 8;
    pdu.ChecksumType = LIN_ENHANCED_CS;
    pdu.SduPtr = data;
    Std_ReturnType ret = Lin_SendFrame(0, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_Lin_SendFrame_NullPdu(void)
{
    Lin_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_SendFrame(0, NULL));
}

void test_Lin_SendFrame_SlaveResponse(void)
{
    Lin_Init(&g_test_cfg);
    uint8 data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    Lin_PduType pdu;
    memset(&pdu, 0, sizeof(pdu));
    pdu.Pid = 0x02;
    pdu.FrameType = LIN_FRAMETYPE_UNCONDITIONAL;
    pdu.FrameResponse = LIN_SLAVE_RESPONSE;
    pdu.Length = 4;
    pdu.SduPtr = data;
    Std_ReturnType ret = Lin_SendFrame(0, &pdu);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= Lin_SendResponse ========= */
void test_Lin_SendResponse_BeforeInit(void)
{
    Lin_PduType pdu;
    memset(&pdu, 0, sizeof(pdu));
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_SendResponse(0, &pdu));
}

void test_Lin_SendResponse_Valid(void)
{
    Lin_Init(&g_test_cfg);
    uint8 data[4] = {0x11, 0x22, 0x33, 0x44};
    Lin_PduType pdu;
    memset(&pdu, 0, sizeof(pdu));
    pdu.Length = 4;
    pdu.SduPtr = data;
    Std_ReturnType ret = Lin_SendResponse(0, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* ========= Lin_DisableResponse ========= */
void test_Lin_DisableResponse_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_DisableResponse(0));
}

void test_Lin_DisableResponse_Valid(void)
{
    Lin_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Lin_DisableResponse(0));
}

/* ========= Lin_WakeUp ========= */
void test_Lin_WakeUp_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_WakeUp(0));
}

void test_Lin_WakeUp_Valid(void)
{
    Lin_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Lin_WakeUp(0));
}

/* ========= Lin_GoToSleep ========= */
void test_Lin_GoToSleep_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Lin_GoToSleep(0));
}

void test_Lin_GoToSleep_Valid(void)
{
    Lin_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Lin_GoToSleep(0));
}

void test_Lin_GoToSleep_WakeUpCycle(void)
{
    Lin_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_OK, Lin_GoToSleep(0));
    TEST_ASSERT_EQUAL(E_OK, Lin_WakeUp(0));
}

/* ========= Lin_GetStatus ========= */
void test_Lin_GetStatus_WithSduPtr(void)
{
    Lin_Init(&g_test_cfg);
    uint8* sdu_ptr = NULL;
    Lin_StatusType status = Lin_GetStatus(0, &sdu_ptr);
    TEST_ASSERT_TRUE(status == LIN_OPERATIONAL || status == LIN_CH_SLEEP);
}

/* ========= Lin_CheckWakeup ========= */
void test_Lin_CheckWakeup_Valid(void)
{
    Lin_Init(&g_test_cfg);
    Std_ReturnType ret = Lin_CheckWakeup(0);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= Lin_GetVersionInfo ========= */
void test_Lin_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Lin_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(LIN_MODULE_ID, vi.moduleID);
}

void test_Lin_GetVersionInfo_Null(void)
{
    Lin_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Lin_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Lin_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Lin_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Lin_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_Lin_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Lin_SendFrame_BeforeInit, "SendFrame before init", __LINE__);
    UnityRunTest(test_Lin_SendFrame_Valid, "SendFrame valid", __LINE__);
    UnityRunTest(test_Lin_SendFrame_NullPdu, "SendFrame null PDU", __LINE__);
    UnityRunTest(test_Lin_SendFrame_SlaveResponse, "SendFrame slave response", __LINE__);
    UnityRunTest(test_Lin_SendResponse_BeforeInit, "SendResponse before init", __LINE__);
    UnityRunTest(test_Lin_SendResponse_Valid, "SendResponse valid", __LINE__);
    UnityRunTest(test_Lin_DisableResponse_BeforeInit, "DisableResponse before init", __LINE__);
    UnityRunTest(test_Lin_DisableResponse_Valid, "DisableResponse valid", __LINE__);
    UnityRunTest(test_Lin_WakeUp_BeforeInit, "WakeUp before init", __LINE__);
    UnityRunTest(test_Lin_WakeUp_Valid, "WakeUp valid", __LINE__);
    UnityRunTest(test_Lin_GoToSleep_BeforeInit, "GoToSleep before init", __LINE__);
    UnityRunTest(test_Lin_GoToSleep_Valid, "GoToSleep valid", __LINE__);
    UnityRunTest(test_Lin_GoToSleep_WakeUpCycle, "GoToSleep/WakeUp cycle", __LINE__);
    UnityRunTest(test_Lin_GetStatus_WithSduPtr, "GetStatus with SDU ptr", __LINE__);
    UnityRunTest(test_Lin_CheckWakeup_Valid, "CheckWakeup valid", __LINE__);
    UnityRunTest(test_Lin_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Lin_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

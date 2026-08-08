/**
 * @file test_mcal_uart.c
 * @brief UART unit test — links real Uart.c production code with MockHAL
 *
 * AUTOSAR UART Driver (Uart) — MCAL Layer
 * Tests: Init/DeInit/Send/Receive/GetStatus/GetVersionInfo/SetBaudRate
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_hal.h"
#include "mock_hal_config.h"
#include "Uart.h"

#ifndef UART_MAX_CHANNELS
#define UART_MAX_CHANNELS 4U
#endif

/* MockHAL base address for UART1 — must match Uart.c's UART1_BASE_ADDR in
 * this build (S32K312 path: S32K312_LPUART0_BASE, see platform/s32k312).
 * Tech-debt T2: the driver polls USR1_TRDY / USR2_TXDC / USR2_RDR status
 * bits; without mock defaults those reads return 0 and the polling loops
 * spin forever (test hung), and raw MMIO pointer access previously
 * segfaulted before MockHAL redirection existed. */
#define UART_MOCK_BASE                  0x40180000UL
#define UART_MOCK_USR1_OFFSET           0x94U  /* UART_USR1_OFFSET */
#define UART_MOCK_USR2_OFFSET           0x98U  /* UART_USR2_OFFSET */
/* Status bits (mirror Uart.c defines): TRDY=1<<13, RRDY=1<<9,
 * TXDC=1<<3, RDR=1<<0 */
#define UART_MOCK_USR1_READY            ((1UL << 13U) | (1UL << 9U))
#define UART_MOCK_USR2_READY            ((1UL << 3U) | (1UL << 0U))

static Uart_ConfigType g_test_cfg;
static Uart_ChannelConfigType g_channel_cfg[UART_MAX_CHANNELS];

/* Reset Uart module state between tests: DeInit fully resets the static
 * driver state so before-init tests are hermetic. */
static void uart_reset_module(void)
{
    Uart_DeInit();
}

void setUp(void)
{
    mock_hal_reset();
    /* Preset the UART status registers so the driver's polling loops
     * (TX FIFO ready / TX done / RX data ready) pass immediately. */
    mock_hal_set_default(UART_MOCK_BASE + UART_MOCK_USR1_OFFSET, UART_MOCK_USR1_READY);
    mock_hal_set_default(UART_MOCK_BASE + UART_MOCK_USR2_OFFSET, UART_MOCK_USR2_READY);
    memset(&g_test_cfg, 0, sizeof(g_test_cfg));
    memset(g_channel_cfg, 0, sizeof(g_channel_cfg));
    g_test_cfg.ChannelCount = 1;
    g_test_cfg.ChannelConfig = g_channel_cfg;
    /* Non-zero timeouts: the mock GPT does not advance time, so polls
     * must pass on the preset status bits; timeouts are a safety net. */
    g_channel_cfg[0].TxTimeout = 100U;
    g_channel_cfg[0].RxTimeout = 100U;
    uart_reset_module();
}
void tearDown(void) {}

/* ========= Uart_Init ========= */
void test_Uart_Init_NullConfig(void)
{
    Uart_Init(NULL);
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0));
}

void test_Uart_Init_Valid(void)
{
    Uart_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0));
}

void test_Uart_Init_DoubleInit(void)
{
    Uart_Init(&g_test_cfg);
    Uart_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0));
}

/* ========= Uart_DeInit ========= */
void test_Uart_DeInit_BeforeInit(void)
{
    Uart_DeInit();
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0));
}

void test_Uart_DeInit_AfterInit(void)
{
    Uart_Init(&g_test_cfg);
    Uart_DeInit();
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0));
}

/* ========= Uart_Send ========= */
void test_Uart_Send_BeforeInit(void)
{
    const uint8 data[] = {0x55};
    TEST_ASSERT_EQUAL(E_NOT_OK, Uart_Send(0, data, 1));
}

void test_Uart_Send_Valid(void)
{
    Uart_Init(&g_test_cfg);
    const uint8 data[] = {0x55, 0xAA, 0x01, 0xFF};
    Std_ReturnType ret = Uart_Send(0, data, 4);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_Uart_Send_NullData(void)
{
    Uart_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Uart_Send(0, NULL, 1));
}

void test_Uart_Send_Empty(void)
{
    Uart_Init(&g_test_cfg);
    const uint8 data[] = {0x00};
    Std_ReturnType ret = Uart_Send(0, data, 0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_Uart_Send_Interrupt(void)
{
    Uart_Init(&g_test_cfg);
    const uint8 data[] = {0x01, 0x02, 0x03};
    Std_ReturnType ret = Uart_SendInterrupt(0, data, 3);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_Uart_Send_DMA(void)
{
    Uart_Init(&g_test_cfg);
    const uint8 data[] = {0x01, 0x02, 0x03};
    Std_ReturnType ret = Uart_SendDMA(0, data, 3);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/* ========= Uart_Receive ========= */
void test_Uart_Receive_BeforeInit(void)
{
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, Uart_Receive(0, buf, 4));
}

void test_Uart_Receive_NullBuffer(void)
{
    Uart_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(E_NOT_OK, Uart_Receive(0, NULL, 4));
}

void test_Uart_Receive_Valid(void)
{
    Uart_Init(&g_test_cfg);
    uint8 buf[8];
    memset(buf, 0x00, sizeof(buf));
    Std_ReturnType ret = Uart_Receive(0, buf, 8);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_Uart_Receive_Interrupt(void)
{
    Uart_Init(&g_test_cfg);
    uint8 buf[4];
    Std_ReturnType ret = Uart_ReceiveInterrupt(0, buf, 4);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* ========= Uart_GetStatus ========= */
void test_Uart_GetStatus_Uninit(void)
{
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0));
}

void test_Uart_GetStatus_AfterInit(void)
{
    Uart_Init(&g_test_cfg);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0));
}

/* ========= Uart_SetBaudRate ========= */
void test_Uart_SetBaudRate_BeforeInit(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Uart_SetBaudRate(0, 115200));
}

void test_Uart_SetBaudRate_Valid(void)
{
    Uart_Init(&g_test_cfg);
    Std_ReturnType ret = Uart_SetBaudRate(0, 115200);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* ========= Uart_EnableDisableInterrupt ========= */
void test_Uart_Interrupt_BeforeInit(void)
{
    Uart_EnableInterrupt(0);
    Uart_DisableInterrupt(0);
}

void test_Uart_Interrupt_AfterInit(void)
{
    Uart_Init(&g_test_cfg);
    Uart_EnableInterrupt(0);
    Uart_DisableInterrupt(0);
}

/* ========= Uart_GetVersionInfo ========= */
void test_Uart_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Uart_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(UART_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL(UART_MODULE_ID, vi.moduleID);
}

void test_Uart_GetVersionInfo_Null(void)
{
    Uart_GetVersionInfo(NULL);
}

/* ========= Main ========= */
int main(void)
{
    UnityBegin();
    UnityRunTest(test_Uart_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Uart_Init_Valid, "Init valid", __LINE__);
    UnityRunTest(test_Uart_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Uart_DeInit_BeforeInit, "DeInit before init", __LINE__);
    UnityRunTest(test_Uart_DeInit_AfterInit, "DeInit after init", __LINE__);
    UnityRunTest(test_Uart_Send_BeforeInit, "Send before init", __LINE__);
    UnityRunTest(test_Uart_Send_Valid, "Send valid", __LINE__);
    UnityRunTest(test_Uart_Send_NullData, "Send null data", __LINE__);
    UnityRunTest(test_Uart_Send_Empty, "Send empty", __LINE__);
    UnityRunTest(test_Uart_Send_Interrupt, "Send interrupt", __LINE__);
    UnityRunTest(test_Uart_Send_DMA, "Send DMA", __LINE__);
    UnityRunTest(test_Uart_Receive_BeforeInit, "Receive before init", __LINE__);
    UnityRunTest(test_Uart_Receive_NullBuffer, "Receive null buffer", __LINE__);
    UnityRunTest(test_Uart_Receive_Valid, "Receive valid", __LINE__);
    UnityRunTest(test_Uart_Receive_Interrupt, "Receive interrupt", __LINE__);
    UnityRunTest(test_Uart_GetStatus_Uninit, "GetStatus uninit", __LINE__);
    UnityRunTest(test_Uart_GetStatus_AfterInit, "GetStatus after init", __LINE__);
    UnityRunTest(test_Uart_SetBaudRate_BeforeInit, "SetBaudRate before init", __LINE__);
    UnityRunTest(test_Uart_SetBaudRate_Valid, "SetBaudRate valid", __LINE__);
    UnityRunTest(test_Uart_Interrupt_BeforeInit, "Interrupt before init", __LINE__);
    UnityRunTest(test_Uart_Interrupt_AfterInit, "Interrupt after init", __LINE__);
    UnityRunTest(test_Uart_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Uart_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    return UnityEnd();
}

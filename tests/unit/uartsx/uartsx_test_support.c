/**
 * @file uartsx_test_support.c
 * @brief UartSx 主机侧测试桩 (Uart / OS / Interrupt)
 */

#define UNIT_TEST
#include "UartSx.h"
#include "uartsx_test_support.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/*==================================================================================================
 *                                Global Stubs (供 UartSx.c extern 引用)
 *================================================================================================*/
pthread_mutex_t  g_uartsx_crit_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int     g_uartsx_event_flag = 0;
volatile int     g_uartsx_tx_pending = 0;
volatile int     g_uartsx_tx_busy    = 0;

/* Event 唤醒: 在 Print 后 set, SenderTask MainFunction 之前检查 */
void uartsx_test_signal_event(void)  { g_uartsx_event_flag = 1; }
void uartsx_test_clear_event(void)   { g_uartsx_event_flag = 0; }

/*==================================================================================================
 *                                Uart Send Stub
 *================================================================================================*/
UartSx_TxEntry  g_tx_log[UARTSX_TEST_MAX_LOGS];
uint32          g_tx_count    = 0U;
uint32          g_tx_read_idx = 0U;

void UartSx_TestStubReset(void)
{
    (void)memset(g_tx_log, 0, sizeof(g_tx_log));
    g_tx_count    = 0U;
    g_tx_read_idx = 0U;
    g_uartsx_event_flag = 0;
}

Std_ReturnType Uart_Send(Uart_ChannelType Channel, const uint8* Data, uint32 Length)
{
    (void)Channel;

    if (g_tx_count < UARTSX_TEST_MAX_LOGS) {
        uint32 copy_len = Length;
        if (copy_len >= UARTSX_TEST_MAX_LEN) {
            copy_len = UARTSX_TEST_MAX_LEN - 1U;
        }
        (void)memcpy(g_tx_log[g_tx_count].data, Data, copy_len);
        g_tx_log[g_tx_count].data[copy_len] = '\0';
        g_tx_log[g_tx_count].len = copy_len;
        g_tx_count++;
    }
    return E_OK;
}

Std_ReturnType Uart_Send_Stub(Uart_ChannelType c, const uint8* d, uint32 l)
{
    return Uart_Send(c, d, l);  /* 别名 — UartSx.c 内部直接调 Uart_Send */
}

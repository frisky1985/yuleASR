#ifndef UARTSX_TEST_SUPPORT_H
#define UARTSX_TEST_SUPPORT_H

#include "UartSx.h"
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>

/*==================================================================================================
 *                                Uart_Send Stub
 *  记录所有发送的日志到 g_tx_log[], 由测试断言
 *================================================================================================*/
#define UARTSX_TEST_MAX_LOGS  (256U)
#define UARTSX_TEST_MAX_LEN   (256U)

typedef struct {
    unsigned char data[UARTSX_TEST_MAX_LEN];
    uint32 len;
} UartSx_TxEntry;

extern UartSx_TxEntry  g_tx_log[UARTSX_TEST_MAX_LOGS];
extern uint32          g_tx_count;     /* 累计发送次数 */
extern uint32          g_tx_read_idx;  /* 被测试取出的序号 */

/** 重置 stub 状态 */
void UartSx_TestStubReset(void);

/*==================================================================================================
 *                                Interrupt/OS stubs
 *  关中断 = pthread_mutex_lock, 开中断 = pthread_mutex_unlock
 *  Event flag = volatile int + 忙等待
 *================================================================================================*/
extern pthread_mutex_t  g_uartsx_crit_mutex;
extern volatile int     g_uartsx_event_flag;

#endif /* UARTSX_TEST_SUPPORT_H */

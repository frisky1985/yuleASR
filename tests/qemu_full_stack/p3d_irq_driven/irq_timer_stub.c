/*
 * irq_timer_stub.c - C7: FreeRTOS-based IRQ verification stub
 *
 * Uses FreeRTOS software timer + task priority preemption instead of
 * CMSDK APB Timer (not reliably available in QEMU mps2-an521).
 */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "Uart_Cfg.h"

static volatile uint32_t s_irq_count = 0UL;
static TaskHandle_t      s_notify_task = NULL;
static TimerHandle_t     s_timer = NULL;

void IrqTimer_SetNotifyTask(TaskHandle_t t) { s_notify_task = t; }
uint32_t IrqTimer_GetCount(void) { return s_irq_count; }

static void timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    s_irq_count++;
    if (s_notify_task != NULL) {
        xTaskNotifyGive(s_notify_task);
    }
}

void IrqTimer_Init(uint32_t reload)
{
    (void)reload;
    s_irq_count = 0UL;
    if (s_timer == NULL) {
        s_timer = xTimerCreate("IrqTmr", pdMS_TO_TICKS(10), pdTRUE, NULL, timer_callback);
    }
}

void IrqTimer_Start(void)
{
    if (s_timer != NULL) {
        xTimerStart(s_timer, 0);
    }
}

void IrqTimer_Stop(void)
{
    if (s_timer != NULL) {
        xTimerStop(s_timer, 0);
    }
}


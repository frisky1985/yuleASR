/*
 * main_irq_driven.c - C7: QEMU IRQ Driven Verification
 *
 * Uses FreeRTOS software timers (not CMSDK APB Timer) for timer-driven
 * notification, and task-priority preemption for S7.4.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

extern void IrqTimer_Init(uint32_t reload);
extern void IrqTimer_Start(void);
extern void IrqTimer_Stop(void);
extern uint32_t IrqTimer_GetCount(void);
extern void IrqTimer_SetNotifyTask(TaskHandle_t t);

static volatile uint32_t s_preempt_order = 0UL;

static void vHighPrioTask(void *pv)
{
    (void)pv;
    s_preempt_order = (s_preempt_order << 4) | 0xBUL;
    vTaskDelete(NULL);
}

static void vIrqTask(void *pv)
{
    (void)pv;
    TaskHandle_t self = xTaskGetCurrentTaskHandle();

    Uart_WriteString("=== C7 IRQ Driven ===\n");

    /* S7.1 + S7.2: software timer fires 10 times and notifies this task */
    {
        IrqTimer_SetNotifyTask(self);
        IrqTimer_Init(0UL);
        IrqTimer_Start();

        uint32_t notified = 0UL;
        while (IrqTimer_GetCount() < 10UL && notified < 50UL) {
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0) {
                notified++;
            }
        }
        IrqTimer_Stop();

        Uart_WriteString("S7.1 irq_count=");
        Uart_WriteDec(IrqTimer_GetCount());
        Uart_WriteString("\n");
        Qemu_Assert(IrqTimer_GetCount() >= 10UL, "S7.1: timer did not fire 10 times");
    }

    /* S7.3: check stack watermark after timer load */
    {
        UBaseType_t mark = uxTaskGetStackHighWaterMark(NULL);
        Uart_WriteString("S7.3 stack_watermark=");
        Uart_WriteDec((uint32_t)mark);
        Uart_WriteString("\n");
        Qemu_Assert(mark > 0U, "S7.3: stack watermark is zero");
    }

    /* S7.4: higher-priority task preempts this task */
    {
        s_preempt_order = 0xAUL;
        xTaskCreate(vHighPrioTask, "High", 256, NULL,
                    configMAX_PRIORITIES - 1, NULL);
        taskYIELD();

        Uart_WriteString("S7.4 preempt_order=0x");
        Uart_WriteDec(s_preempt_order);
        Uart_WriteString("\n");
        Qemu_Assert(s_preempt_order == 0xABUL,
                    "S7.4: high-priority task did not preempt");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("IRQ_DRIVEN_START\n");
    xTaskCreate(vIrqTask, "Irq", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}

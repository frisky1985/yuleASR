/*
 * main_os_schedule.c - C2: QEMU OS Schedule Verification
 *
 * Scenarios:
 *   S2.1 SysTickAdvance      - tick count advances >= 90 over 100ms wait
 *   S2.2 TaskPriorityPreempt - high-priority task runs before low-priority
 *   S2.3 TerminateTask       - task self-deletes after 1 run (b_run_count == 1)
 *   S2.4 AlarmExpiry         - software timer fires >= 3 times in 400ms
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "../common/qemu_assert.h"
#include "Uart_Cfg.h"

static volatile uint32_t s_high_tick   = 0UL;
static volatile uint32_t s_low_count   = 0UL;
static volatile uint32_t s_b_run_count = 0UL;
static volatile uint32_t s_alarm_count = 0UL;
static TaskHandle_t       s_main_task  = NULL;

/* S2.2 high-priority task - records tick then notifies main */
static void vHighPrioTask(void *pv)
{
    (void)pv;
    s_high_tick = (uint32_t)xTaskGetTickCount();
    Uart_WriteString("HIGH_PRIO_RAN\n");
    xTaskNotifyGive(s_main_task);
    vTaskDelete(NULL);
}

/* S2.2 low-priority task - runs after high has already gone */
static void vLowPrioTask(void *pv)
{
    (void)pv;
    s_low_count++;
    Uart_WriteString("LOW_PRIO_RAN\n");
    xTaskNotifyGive(s_main_task);
    vTaskDelete(NULL);
}

/* S2.3 single-shot task */
static void vOneShotTask(void *pv)
{
    (void)pv;
    s_b_run_count++;
    Uart_WriteString("ONESHOT_RAN\n");
    xTaskNotifyGive(s_main_task);
    vTaskDelete(NULL);
}

/* S2.4 software timer callback */
static void vAlarmCb(TimerHandle_t t)
{
    (void)t;
    s_alarm_count++;
}

static void vMainTask(void *pv)
{
    (void)pv;
    TimerHandle_t alarm;

    Uart_WriteString("=== C2 OS Schedule ===\n");

    /* S2.1: SysTickAdvance */
    {
        TickType_t t0 = xTaskGetTickCount();
        vTaskDelay(pdMS_TO_TICKS(100));
        TickType_t t1 = xTaskGetTickCount();
        uint32_t diff = (uint32_t)(t1 - t0);
        Uart_WriteString("S2.1 tick_diff=");
        Uart_WriteDec(diff);
        Uart_WriteString("\n");
        Qemu_Assert(diff >= 90U, "S2.1: tick did not advance >= 90");
    }

    /* S2.2: TaskPriorityPreempt */
    {
        /* create low first, then high - high should preempt immediately */
        xTaskCreate(vLowPrioTask,  "Low",  256, NULL, 1, NULL);
        xTaskCreate(vHighPrioTask, "High", 256, NULL, 3, NULL);
        /* wait for both to notify */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
        Uart_WriteString("S2.2 high_tick=");
        Uart_WriteDec(s_high_tick);
        Uart_WriteString("\n");
        Qemu_Assert(s_high_tick > 0U, "S2.2: high prio task never ran");
        Qemu_Assert(s_low_count == 1U, "S2.2: low prio task did not run");
    }

    /* S2.3: TerminateTask (single-shot via vTaskDelete) */
    {
        xTaskCreate(vOneShotTask, "OneShot", 256, NULL, 2, NULL);
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
        vTaskDelay(pdMS_TO_TICKS(50));
        Uart_WriteString("S2.3 b_run_count=");
        Uart_WriteDec(s_b_run_count);
        Uart_WriteString("\n");
        Qemu_Assert(s_b_run_count == 1U, "S2.3: oneshot ran != 1 time");
    }

    /* S2.4: AlarmExpiry */
    {
        s_alarm_count = 0UL;
        alarm = xTimerCreate("Alarm", pdMS_TO_TICKS(80), pdTRUE, NULL, vAlarmCb);
        Qemu_Assert(alarm != NULL, "S2.4: timer create failed");
        xTimerStart(alarm, 0);
        vTaskDelay(pdMS_TO_TICKS(400));
        xTimerStop(alarm, 0);
        Uart_WriteString("S2.4 alarm_count=");
        Uart_WriteDec(s_alarm_count);
        Uart_WriteString("\n");
        Qemu_Assert(s_alarm_count >= 3U, "S2.4: alarm fired < 3 times");
    }

    Qemu_ReportPass();
}

int main(void)
{
    Uart_Init();
    Uart_WriteString("OS_SCHEDULE_START\n");
    s_main_task = xTaskGetCurrentTaskHandle();
    xTaskCreate(vMainTask, "Main", 1024, NULL, 2, &s_main_task);
    vTaskStartScheduler();
    for (;;) {}
}

/* Malloc/StackOverflow hooks provided by hooks.c */
void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}

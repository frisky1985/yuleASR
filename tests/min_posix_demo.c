/*==================================================================================================
 * min_posix_demo.c - Minimal FreeRTOS Posix port test (Linux)
 *
 * Pure official-port test, no yuleASR Os wrapper involved:
 *   - one task prints "task alive" every 500ms
 *   - software timer stops the scheduler after 3s
 * If this hangs, the Posix port itself is broken on this platform.
 *================================================================================================*/
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t g_exitTimer = NULL;

static void vTaskPrint(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        printf("[DEMO] task alive, tick=%lu\n", (unsigned long)xTaskGetTickCount());
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vExitTimerCallback(TimerHandle_t xTimer)
{
    static int count = 0;
    (void)xTimer;
    count++;
    printf("[DEMO] timer %d\n", count);
    fflush(stdout);
    if (count >= 3)
    {
        printf("[DEMO] calling vTaskEndScheduler()\n");
        fflush(stdout);
        vTaskEndScheduler();
    }
}

int main(void)
{
    printf("[DEMO] === minimal Posix port test ===\n");
    fflush(stdout);

    g_exitTimer = xTimerCreate("Exit", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, vExitTimerCallback);
    if (g_exitTimer == NULL) { printf("[DEMO] xTimerCreate failed\n"); return 1; }
    if (xTimerStart(g_exitTimer, 0) != pdPASS) { printf("[DEMO] xTimerStart failed\n"); return 1; }

    if (xTaskCreate(vTaskPrint, "Print", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL) != pdPASS)
    {
        printf("[DEMO] xTaskCreate failed\n");
        return 1;
    }

    printf("[DEMO] calling vTaskStartScheduler()\n");
    fflush(stdout);
    vTaskStartScheduler();

    printf("[DEMO] scheduler returned - DONE\n");
    fflush(stdout);
    return 0;
}

/**
 * @file FreeRTOS.h
 * @brief FreeRTOS Header Stub for AutoSAR OS Integration
 * @version 10.6.0
 * @date 2026-04-21
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef FREERTOS_H
#define FREERTOS_H

#include "Std_Types.h"

/* FreeRTOS configuration stub */
#define configMAX_PRIORITIES            (32)
#define configMAX_TASK_NAME_LEN         (16)
#define configTIMER_TASK_PRIORITY       (2)
#define configTIMER_QUEUE_LENGTH        (10)
#define configTIMER_TASK_STACK_DEPTH    (256)
#define configUSE_16_BIT_TICKS          (0)
#define portBASE_TYPE                   long
#define pdFALSE                         (0)
#define pdTRUE                          (1)
#define pdPASS                          (pdTRUE)
#define pdFAIL                          (pdFALSE)
#define errQUEUE_EMPTY                  (0)
#define errQUEUE_FULL                   (0)
#define portMAX_DELAY                   (0xFFFFFFFFUL)
#define pdMS_TO_TICKS(xTimeInMs)        ((TickType_t)((xTimeInMs) / 10))

/* Basic FreeRTOS types */
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;

/* Task handle */
struct tskTaskControlBlock;
typedef struct tskTaskControlBlock* TaskHandle_t;

/* Timer handle */
struct tmrTimerControl;
typedef struct tmrTimerControl* TimerHandle_t;

/* Event group handle */
struct EventGroupDef_t;
typedef struct EventGroupDef_t* EventGroupHandle_t;

/* Semaphore handle */
struct QueueDefinition;
typedef struct QueueDefinition* QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;

/* Kernel control */
void vTaskStartScheduler(void);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);
TickType_t xTaskGetTickCount(void);

/* Memory allocation */
void* pvPortMalloc(size_t xWantedSize);
void vPortFree(void* pv);

/* Critical sections */
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);
#define taskENTER_CRITICAL()            vPortEnterCritical()
#define taskEXIT_CRITICAL()             vPortExitCritical()
#define taskDISABLE_INTERRUPTS()
#define taskENABLE_INTERRUPTS()

#endif /* FREERTOS_H */

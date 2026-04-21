/**
 * @file task.h
 * @brief FreeRTOS Task API Header Stub
 * @version 10.6.0
 * @date 2026-04-21
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"

/* Task states */
#define eRunning                        (0)
#define eReady                          (1)
#define eBlocked                        (2)
#define eSuspended                      (3)
#define eDeleted                        (4)

typedef enum {
    eNoAction = 0,
    eSetBits,
    eIncrement,
    eSetValueWithOverwrite,
    eSetValueWithoutOverwrite
} eNotifyAction;

/* Task creation */
BaseType_t xTaskCreate(
    void (*pvTaskCode)(void*),
    const char* const pcName,
    const uint32_t usStackDepth,
    void* const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* const pxCreatedTask);

/* Task deletion */
void vTaskDelete(TaskHandle_t xTask);

/* Task control */
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t* const pxPreviousWakeTime, const TickType_t xTimeIncrement);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);
BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume);

/* Task priority */
UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);

/* Task yield */
void taskYIELD(void);

/* Notifications */
BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t* pulNotificationValue, TickType_t xTicksToWait);
BaseType_t xTaskNotify(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction);
BaseType_t xTaskNotifyFromISR(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, BaseType_t* pxHigherPriorityTaskWoken);

/* Current task handle */
TaskHandle_t xTaskGetCurrentTaskHandle(void);

/* Task state */
eNotifyAction eTaskGetState(TaskHandle_t xTask);

/* Idle hook */
extern void vApplicationIdleHook(void);

/* Stack overflow hook */
extern void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName);

/* Tick hook */
extern void vApplicationTickHook(void);

/* Malloc failed hook */
extern void vApplicationMallocFailedHook(void);

#endif /* TASK_H */

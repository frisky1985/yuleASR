/* FreeRTOS Task API Stub — provided by yuleDKCS port layer */
#ifndef TASK_H
#define TASK_H

/* All FreeRTOS types come via FreeRTOS.h → portmacro.h + projdefs.h */
#include "FreeRTOS.h"

/* Task handle type */
struct tskTaskControlBlock;
typedef struct tskTaskControlBlock* TaskHandle_t;

/* Task state enumeration for eTaskGetState() */
typedef enum {
    eRunning   = 0,
    eReady     = 1,
    eBlocked   = 2,
    eSuspended = 3,
    eDeleted   = 4,
    eInvalid   = 5
} eTaskState;

/* Notification action type */
typedef enum {
    eNoAction                = 0,
    eSetBits                 = 1,
    eIncrement               = 2,
    eSetValueWithOverwrite   = 3,
    eSetValueWithoutOverwrite = 4
} eNotifyAction;

/* =========================================================================
 * Task API
 * ========================================================================= */
BaseType_t xTaskCreate(TaskFunction_t pvTaskCode, const char* pcName, unsigned long usStackDepth, void* pvParameters, UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask);
void vTaskDelete(TaskHandle_t xTask);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t* pxPreviousWakeTime, const TickType_t xTimeIncrement);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);
BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume);
UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
void taskYIELD(void);
BaseType_t xTaskNotifyWait(unsigned long ulBitsToClearOnEntry, unsigned long ulBitsToClearOnExit, unsigned long* pulNotificationValue, TickType_t xTicksToWait);
BaseType_t xTaskNotify(TaskHandle_t xTaskToNotify, unsigned long ulValue, eNotifyAction eAction);
BaseType_t xTaskNotifyFromISR(TaskHandle_t xTaskToNotify, unsigned long ulValue, eNotifyAction eAction, BaseType_t* pxHigherPriorityTaskWoken);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
eTaskState eTaskGetState(TaskHandle_t xTask);
TickType_t xTaskGetTickCount(void);

/* =========================================================================
 * Hooks (weak, provided by application)
 * ========================================================================= */
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName);
void vApplicationTickHook(void);
void vApplicationMallocFailedHook(void);

/* =========================================================================
 * Scheduler control
 * ========================================================================= */
void vTaskStartScheduler(void);
void vTaskEndScheduler(void);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);

#endif /* TASK_H */

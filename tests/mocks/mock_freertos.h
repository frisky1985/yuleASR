/**
 * @file mock_freertos.h
 * @brief FreeRTOS模拟头文件 - 用于单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 基本类型定义
 *==============================================================================*/

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;
typedef void* TimerHandle_t;

/* 任务回调函数类型 */
typedef void (*TaskFunction_t)(void*);

/* 基本类型别名 */
typedef uint32_t TickType_t;
typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t StackType_t;

/* 堆大小 */
#define configTOTAL_HEAP_SIZE           (64 * 1024)
#define configMAX_TASK_NAME_LEN         16

/* 任务堆栈深度 */
#define configSTACK_DEPTH_TYPE          uint32_t
/* Tick转换宏 */
#define portTICK_PERIOD_MS              (1000 / configTICK_RATE_HZ)
#define pdMS_TO_TICKS(xTimeInMs)        ((TickType_t)((xTimeInMs) / portTICK_PERIOD_MS))
#define portMAX_DELAY                   0xFFFFFFFF

/*==============================================================================
 * 返回值定义
 *==============================================================================*/

#define pdTRUE                          1
#define pdFALSE                         0
#define pdPASS                          pdTRUE
#define pdFAIL                          pdFALSE

#define errQUEUE_EMPTY                  0
#define errQUEUE_FULL                   0
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY  (-1)

/*==============================================================================
 * 任务API声明
 *==============================================================================*/

BaseType_t xTaskCreate(TaskFunction_t pvTaskCode,
                       const char* const pcName,
                       const configSTACK_DEPTH_TYPE usStackDepth,
                       void* const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t* const pxCreatedTask);

void vTaskDelete(TaskHandle_t xTask);
void vTaskDelay(const TickType_t xTicksToDelay);
BaseType_t xTaskDelayUntil(TickType_t* const pxPreviousWakeTime,
                           const TickType_t xTimeIncrement);
TickType_t xTaskGetTickCount(void);
UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
void vTaskSuspend(TaskHandle_t xTask);
void vTaskResume(TaskHandle_t xTask);
BaseType_t xTaskResumeFromISR(TaskHandle_t xTask);
void vTaskStartScheduler(void);
void vTaskEndScheduler(void);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);

/*==============================================================================
 * 队列API声明
 *==============================================================================*/

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize);
void vQueueDelete(QueueHandle_t xQueue);
BaseType_t xQueueSend(QueueHandle_t xQueue,
                      const void* const pvItemToQueue,
                      const TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue,
                         void* const pvBuffer,
                         const TickType_t xTicksToWait);
UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue);
BaseType_t xQueueSendFromISR(QueueHandle_t xQueue,
                              const void* const pvItemToQueue,
                              BaseType_t* const pxHigherPriorityTaskWoken);
BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue,
                                 void* const pvBuffer,
                                 BaseType_t* const pxHigherPriorityTaskWoken);

/*==============================================================================
 * 信号量API声明
 *==============================================================================*/

SemaphoreHandle_t xSemaphoreCreateBinary(void);
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount,
                                           UBaseType_t uxInitialCount);
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);
void vSemaphoreDelete(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore,
                          TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore,
                                  BaseType_t* const pxHigherPriorityTaskWoken);
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,
                                  BaseType_t* const pxHigherPriorityTaskWoken);

/*==============================================================================
 * 内存API声明
 *==============================================================================*/

void* pvPortMalloc(size_t xWantedSize);
void vPortFree(void* pv);
void* pvPortCalloc(size_t xNum, size_t xSize);
size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);

/*==============================================================================
 * 中断API声明
 *==============================================================================*/

void vPortEnterCritical(void);
void vPortExitCritical(void);
void vPortDisableInterrupts(void);
void vPortEnableInterrupts(void);

/*==============================================================================
 * 调试API声明
 *==============================================================================*/

void mock_freertos_reset(void);
uint32_t mock_freertos_get_task_count(void);
uint32_t mock_freertos_get_queue_count(void);
uint32_t mock_freertos_get_tick_count(void);
uint64_t mock_freertos_get_heap_used(void);
void mock_freertos_print_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_FREERTOS_H */

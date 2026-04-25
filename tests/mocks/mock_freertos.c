/**
 * @file mock_freertos.c
 * @brief FreeRTOS模拟实现 - 用于单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "mock_freertos.h"

/*==============================================================================
 * 内部状态和数据结构
 *==============================================================================*/

typedef struct mock_task {
    uint32_t id;
    char name[configMAX_TASK_NAME_LEN];
    pthread_t thread;
    TaskFunction_t func;
    void* params;
    uint32_t priority;
    uint32_t stack_size;
    bool active;
    bool suspended;
    struct mock_task* next;
} mock_task_t;

typedef struct mock_queue {
    uint32_t id;
    uint32_t length;
    uint32_t item_size;
    uint8_t* buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    bool active;
    struct mock_queue* next;
} mock_queue_t;

typedef struct mock_semaphore {
    uint32_t id;
    uint32_t count;
    uint32_t max_count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool active;
    struct mock_semaphore* next;
} mock_semaphore_t;

typedef struct mock_mutex {
    uint32_t id;
    pthread_mutex_t pmutex;
    pthread_mutexattr_t attr;
    bool active;
    struct mock_mutex* next;
} mock_mutex_t;

/* 全局状态 */
static struct {
    mock_task_t* tasks;
    mock_queue_t* queues;
    mock_semaphore_t* semaphores;
    mock_mutex_t* mutexes;
    mock_task_t* current_task;
    uint32_t tick_count;
    pthread_mutex_t global_mutex;
    bool initialized;
    uint32_t next_task_id;
    uint32_t next_queue_id;
    uint32_t next_sem_id;
    uint32_t next_mutex_id;
    uint64_t total_heap_free;
    uint64_t total_heap_used;
    uint32_t context_switch_count;
    uint32_t critical_nesting;
} mock_freertos_state = {0};

/*==============================================================================
 * 辅助函数
 *==============================================================================*/

static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void mock_freertos_init(void) {
    if (!mock_freertos_state.initialized) {
        pthread_mutex_init(&mock_freertos_state.global_mutex, NULL);
        mock_freertos_state.tick_count = 0;
        mock_freertos_state.next_task_id = 1;
        mock_freertos_state.next_queue_id = 1;
        mock_freertos_state.next_sem_id = 1;
        mock_freertos_state.next_mutex_id = 1;
        mock_freertos_state.total_heap_free = configTOTAL_HEAP_SIZE;
        mock_freertos_state.total_heap_used = 0;
        mock_freertos_state.context_switch_count = 0;
        mock_freertos_state.critical_nesting = 0;
        mock_freertos_state.initialized = true;
    }
}

/*==============================================================================
 * 任务管理
 *==============================================================================*/

static void* task_wrapper(void* arg) {
    mock_task_t* task = (mock_task_t*)arg;
    mock_freertos_state.current_task = task;
    task->func(task->params);
    task->active = false;
    return NULL;
}

BaseType_t xTaskCreate(TaskFunction_t pvTaskCode,
                       const char* const pcName,
                       const configSTACK_DEPTH_TYPE usStackDepth,
                       void* const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t* const pxCreatedTask) {
    mock_freertos_init();
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    mock_task_t* task = (mock_task_t*)malloc(sizeof(mock_task_t));
    if (task == NULL) {
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    }
    
    task->id = mock_freertos_state.next_task_id++;
    strncpy(task->name, pcName, configMAX_TASK_NAME_LEN - 1);
    task->name[configMAX_TASK_NAME_LEN - 1] = '\0';
    task->func = pvTaskCode;
    task->params = pvParameters;
    task->priority = uxPriority;
    task->stack_size = usStackDepth * sizeof(StackType_t);
    task->active = true;
    task->suspended = false;
    task->next = mock_freertos_state.tasks;
    mock_freertos_state.tasks = task;
    
    if (pthread_create(&task->thread, NULL, task_wrapper, task) != 0) {
        mock_freertos_state.tasks = task->next;
        free(task);
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    }
    
    if (pxCreatedTask != NULL) {
        *pxCreatedTask = task;
    }
    
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return pdPASS;
}

void vTaskDelete(TaskHandle_t xTask) {
    mock_task_t* task = (mock_task_t*)xTask;
    
    if (task == NULL) {
        task = mock_freertos_state.current_task;
    }
    
    if (task != NULL) {
        task->active = false;
        pthread_detach(task->thread);
    }
}

void vTaskDelay(const TickType_t xTicksToDelay) {
    uint32_t delay_ms = xTicksToDelay * portTICK_PERIOD_MS;
    usleep(delay_ms * 1000);
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    mock_freertos_state.tick_count += xTicksToDelay;
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
}

BaseType_t xTaskDelayUntil(TickType_t* const pxPreviousWakeTime,
                           const TickType_t xTimeIncrement) {
    TickType_t current = xTaskGetTickCount();
    TickType_t next = *pxPreviousWakeTime + xTimeIncrement;
    
    if (next > current) {
        vTaskDelay(next - current);
    }
    
    *pxPreviousWakeTime = next;
    return pdTRUE;
}

TickType_t xTaskGetTickCount(void) {
    mock_freertos_init();
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    TickType_t ticks = mock_freertos_state.tick_count;
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return ticks;
}

UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask) {
    mock_task_t* task = (mock_task_t*)xTask;
    if (task == NULL) {
        task = mock_freertos_state.current_task;
    }
    return task ? task->priority : 0;
}

void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority) {
    mock_task_t* task = (mock_task_t*)xTask;
    if (task == NULL) {
        task = mock_freertos_state.current_task;
    }
    if (task != NULL) {
        task->priority = uxNewPriority;
    }
}

void vTaskSuspend(TaskHandle_t xTask) {
    mock_task_t* task = (mock_task_t*)xTask;
    if (task == NULL) {
        task = mock_freertos_state.current_task;
    }
    if (task != NULL) {
        task->suspended = true;
    }
}

void vTaskResume(TaskHandle_t xTask) {
    mock_task_t* task = (mock_task_t*)xTask;
    if (task != NULL) {
        task->suspended = false;
    }
}

BaseType_t xTaskResumeFromISR(TaskHandle_t xTask) {
    vTaskResume(xTask);
    return pdTRUE;
}

/*==============================================================================
 * 队列管理
 *==============================================================================*/

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize) {
    mock_freertos_init();
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    mock_queue_t* queue = (mock_queue_t*)malloc(sizeof(mock_queue_t));
    if (queue == NULL) {
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return NULL;
    }
    
    queue->id = mock_freertos_state.next_queue_id++;
    queue->length = uxQueueLength;
    queue->item_size = uxItemSize;
    queue->buffer = (uint8_t*)malloc(uxQueueLength * uxItemSize);
    if (queue->buffer == NULL) {
        free(queue);
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return NULL;
    }
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    queue->active = true;
    queue->next = mock_freertos_state.queues;
    mock_freertos_state.queues = queue;
    
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return queue;
}

void vQueueDelete(QueueHandle_t xQueue) {
    mock_queue_t* queue = (mock_queue_t*)xQueue;
    if (queue != NULL) {
        queue->active = false;
        pthread_cond_destroy(&queue->not_empty);
        pthread_cond_destroy(&queue->not_full);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->buffer);
        free(queue);
    }
}

BaseType_t xQueueSend(QueueHandle_t xQueue,
                      const void* const pvItemToQueue,
                      const TickType_t xTicksToWait) {
    mock_queue_t* queue = (mock_queue_t*)xQueue;
    if (queue == NULL || !queue->active) {
        return errQUEUE_FULL;
    }
    
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += xTicksToWait / configTICK_RATE_HZ;
    timeout.tv_nsec += (xTicksToWait % configTICK_RATE_HZ) * 1000000000 / configTICK_RATE_HZ;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->count >= queue->length) {
        if (xTicksToWait == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return errQUEUE_FULL;
        }
        int ret = pthread_cond_timedwait(&queue->not_full, &queue->mutex, &timeout);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return errQUEUE_FULL;
        }
    }
    
    uint8_t* dest = queue->buffer + (queue->tail * queue->item_size);
    memcpy(dest, pvItemToQueue, queue->item_size);
    queue->tail = (queue->tail + 1) % queue->length;
    queue->count++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue,
                         void* const pvBuffer,
                         const TickType_t xTicksToWait) {
    mock_queue_t* queue = (mock_queue_t*)xQueue;
    if (queue == NULL || !queue->active) {
        return errQUEUE_EMPTY;
    }
    
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += xTicksToWait / configTICK_RATE_HZ;
    timeout.tv_nsec += (xTicksToWait % configTICK_RATE_HZ) * 1000000000 / configTICK_RATE_HZ;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->count == 0) {
        if (xTicksToWait == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return errQUEUE_EMPTY;
        }
        int ret = pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &timeout);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return errQUEUE_EMPTY;
        }
    }
    
    uint8_t* src = queue->buffer + (queue->head * queue->item_size);
    memcpy(pvBuffer, src, queue->item_size);
    queue->head = (queue->head + 1) % queue->length;
    queue->count--;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return pdPASS;
}

UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue) {
    mock_queue_t* queue = (mock_queue_t*)xQueue;
    if (queue == NULL) {
        return 0;
    }
    return queue->count;
}

BaseType_t xQueueSendFromISR(QueueHandle_t xQueue,
                              const void* const pvItemToQueue,
                              BaseType_t* const pxHigherPriorityTaskWoken) {
    return xQueueSend(xQueue, pvItemToQueue, 0);
}

BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue,
                                 void* const pvBuffer,
                                 BaseType_t* const pxHigherPriorityTaskWoken) {
    return xQueueReceive(xQueue, pvBuffer, 0);
}

/*==============================================================================
 * 信号量管理
 *==============================================================================*/

SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    return xSemaphoreCreateCounting(1, 0);
}

SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount,
                                           UBaseType_t uxInitialCount) {
    mock_freertos_init();
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    mock_semaphore_t* sem = (mock_semaphore_t*)malloc(sizeof(mock_semaphore_t));
    if (sem == NULL) {
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return NULL;
    }
    
    sem->id = mock_freertos_state.next_sem_id++;
    sem->count = uxInitialCount;
    sem->max_count = uxMaxCount;
    pthread_mutex_init(&sem->mutex, NULL);
    pthread_cond_init(&sem->cond, NULL);
    sem->active = true;
    sem->next = mock_freertos_state.semaphores;
    mock_freertos_state.semaphores = sem;
    
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return sem;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    return (SemaphoreHandle_t)xSemaphoreCreateCounting(1, 1);
}

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {
    mock_semaphore_t* sem = (mock_semaphore_t*)xSemaphore;
    if (sem != NULL) {
        sem->active = false;
        pthread_cond_destroy(&sem->cond);
        pthread_mutex_destroy(&sem->mutex);
        free(sem);
    }
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore,
                          TickType_t xTicksToWait) {
    mock_semaphore_t* sem = (mock_semaphore_t*)xSemaphore;
    if (sem == NULL || !sem->active) {
        return pdFALSE;
    }
    
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += xTicksToWait / configTICK_RATE_HZ;
    timeout.tv_nsec += (xTicksToWait % configTICK_RATE_HZ) * 1000000000 / configTICK_RATE_HZ;
    
    pthread_mutex_lock(&sem->mutex);
    
    while (sem->count == 0) {
        if (xTicksToWait == 0) {
            pthread_mutex_unlock(&sem->mutex);
            return pdFALSE;
        }
        int ret = pthread_cond_timedwait(&sem->cond, &sem->mutex, &timeout);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&sem->mutex);
            return pdFALSE;
        }
    }
    
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);
    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    mock_semaphore_t* sem = (mock_semaphore_t*)xSemaphore;
    if (sem == NULL || !sem->active) {
        return pdFALSE;
    }
    
    pthread_mutex_lock(&sem->mutex);
    if (sem->count < sem->max_count) {
        sem->count++;
        pthread_cond_signal(&sem->cond);
    }
    pthread_mutex_unlock(&sem->mutex);
    return pdTRUE;
}

BaseType_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore,
                                  BaseType_t* const pxHigherPriorityTaskWoken) {
    return xSemaphoreTake(xSemaphore, 0);
}

BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore,
                                  BaseType_t* const pxHigherPriorityTaskWoken) {
    return xSemaphoreGive(xSemaphore);
}

/*==============================================================================
 * 互斥锁管理
 *==============================================================================*/

SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void) {
    mock_freertos_init();
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    mock_mutex_t* mutex = (mock_mutex_t*)malloc(sizeof(mock_mutex_t));
    if (mutex == NULL) {
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return NULL;
    }
    
    mutex->id = mock_freertos_state.next_mutex_id++;
    pthread_mutexattr_init(&mutex->attr);
    pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex->pmutex, &mutex->attr);
    mutex->active = true;
    mutex->next = mock_freertos_state.mutexes;
    mock_freertos_state.mutexes = mutex;
    
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return mutex;
}

/*==============================================================================
 * 内存管理
 *==============================================================================*/

void* pvPortMalloc(size_t xWantedSize) {
    mock_freertos_init();
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    if (mock_freertos_state.total_heap_used + xWantedSize > 
        mock_freertos_state.total_heap_free) {
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        return NULL;
    }
    
    void* ptr = malloc(xWantedSize);
    if (ptr != NULL) {
        mock_freertos_state.total_heap_used += xWantedSize;
    }
    
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return ptr;
}

void vPortFree(void* pv) {
    if (pv != NULL) {
        pthread_mutex_lock(&mock_freertos_state.global_mutex);
        mock_freertos_state.total_heap_used -= sizeof(size_t);
        pthread_mutex_unlock(&mock_freertos_state.global_mutex);
        free(pv);
    }
}

void* pvPortCalloc(size_t xNum, size_t xSize) {
    size_t total = xNum * xSize;
    void* ptr = pvPortMalloc(total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

size_t xPortGetFreeHeapSize(void) {
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    size_t free_size = (size_t)mock_freertos_state.total_heap_free - 
                       mock_freertos_state.total_heap_used;
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return free_size;
}

size_t xPortGetMinimumEverFreeHeapSize(void) {
    return xPortGetFreeHeapSize();
}

/*==============================================================================
 * 中断管理
 *==============================================================================*/

void vPortEnterCritical(void) {
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    mock_freertos_state.critical_nesting++;
}

void vPortExitCritical(void) {
    if (mock_freertos_state.critical_nesting > 0) {
        mock_freertos_state.critical_nesting--;
    }
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
}

void vPortDisableInterrupts(void) {
    /* 在POSIX环境下模拟 - 仅增加关键区嵌套计数 */
    vPortEnterCritical();
}

void vPortEnableInterrupts(void) {
    vPortExitCritical();
}

/*==============================================================================
 * 调度器支持
 *==============================================================================*/

void vTaskStartScheduler(void) {
    mock_freertos_init();
    /* 在模拟环境中不需要启动调度器 */
}

void vTaskEndScheduler(void) {
    /* 清理所有任务 */
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    mock_task_t* task = mock_freertos_state.tasks;
    while (task != NULL) {
        if (task->active) {
            pthread_cancel(task->thread);
        }
        mock_task_t* next = task->next;
        free(task);
        task = next;
    }
    mock_freertos_state.tasks = NULL;
    
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
}

void vTaskSuspendAll(void) {
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
}

BaseType_t xTaskResumeAll(void) {
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return pdFALSE;
}

/*==============================================================================
 * 调试和统计接口
 *==============================================================================*/

void mock_freertos_reset(void) {
    vTaskEndScheduler();
    
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    
    /* 清理队列 */
    mock_queue_t* queue = mock_freertos_state.queues;
    while (queue != NULL) {
        mock_queue_t* next = queue->next;
        vQueueDelete(queue);
        queue = next;
    }
    mock_freertos_state.queues = NULL;
    
    /* 清理信号量 */
    mock_semaphore_t* sem = mock_freertos_state.semaphores;
    while (sem != NULL) {
        mock_semaphore_t* next = sem->next;
        vSemaphoreDelete(sem);
        sem = next;
    }
    mock_freertos_state.semaphores = NULL;
    
    /* 清理互斥锁 */
    mock_mutex_t* mutex = mock_freertos_state.mutexes;
    while (mutex != NULL) {
        mock_mutex_t* next = mutex->next;
        pthread_mutex_destroy(&mutex->pmutex);
        pthread_mutexattr_destroy(&mutex->attr);
        free(mutex);
        mutex = next;
    }
    mock_freertos_state.mutexes = NULL;
    
    mock_freertos_state.initialized = false;
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    pthread_mutex_destroy(&mock_freertos_state.global_mutex);
}

uint32_t mock_freertos_get_task_count(void) {
    uint32_t count = 0;
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    mock_task_t* task = mock_freertos_state.tasks;
    while (task != NULL) {
        if (task->active) count++;
        task = task->next;
    }
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return count;
}

uint32_t mock_freertos_get_queue_count(void) {
    uint32_t count = 0;
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    mock_queue_t* queue = mock_freertos_state.queues;
    while (queue != NULL) {
        if (queue->active) count++;
        queue = queue->next;
    }
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return count;
}

uint32_t mock_freertos_get_tick_count(void) {
    return xTaskGetTickCount();
}

uint64_t mock_freertos_get_heap_used(void) {
    pthread_mutex_lock(&mock_freertos_state.global_mutex);
    uint64_t used = mock_freertos_state.total_heap_used;
    pthread_mutex_unlock(&mock_freertos_state.global_mutex);
    return used;
}

void mock_freertos_print_stats(void) {
    printf("\n========== FreeRTOS Mock Statistics ==========\n");
    printf("Tasks: %u\n", mock_freertos_get_task_count());
    printf("Queues: %u\n", mock_freertos_get_queue_count());
    printf("Tick Count: %u\n", mock_freertos_get_tick_count());
    printf("Heap Used: %lu bytes\n", (unsigned long)mock_freertos_get_heap_used());
    printf("=============================================\n");
}

/**
 * @file FreeRTOS.h
 * @brief Mock FreeRTOS header for OS unit testing
 *
 * Provides all type definitions, constants, and function prototypes
 * needed to compile Os.c in a hosted test environment.
 * All FreeRTOS API functions are stubbed in the test .c files.
 */
#ifndef FREERTOS_MOCK_H
#define FREERTOS_MOCK_H

#include <stdint.h>
#include <stddef.h>

/* ================================================================
 *  Basic FreeRTOS types
 * ================================================================ */
typedef void*    TaskHandle_t;
typedef void*    SemaphoreHandle_t;
typedef void*    EventGroupHandle_t;
typedef void*    TimerHandle_t;
typedef uint32_t UBaseType_t;
typedef int32_t  BaseType_t;
typedef uint32_t TickType_t;
typedef uint16_t EventBits_t;
typedef uint32_t StackType_t;
typedef struct { int dummy; } StaticTask_t;
typedef int32_t  configSTACK_DEPTH_TYPE;

/* ================================================================
 *  Constants
 * ================================================================ */
#define pdTRUE                  ((BaseType_t)1)
#define pdFALSE                 ((BaseType_t)0)
#define pdPASS                  (pdTRUE)
#define pdFAIL                  (pdFALSE)
#define portMAX_DELAY           ((TickType_t)0xFFFFFFFFU)

#define taskSCHEDULER_NOT_STARTED   ((BaseType_t)0)
#define taskSCHEDULER_RUNNING       ((BaseType_t)2)
#define taskSCHEDULER_SUSPENDED     ((BaseType_t)1)

#define eRunning    ((eTaskState)0)
#define eReady      ((eTaskState)1)
#define eBlocked    ((eTaskState)2)
#define eSuspended  ((eTaskState)3)

typedef enum { eTaskStatesPlaceholder = 0 } eTaskStatePlaceholder;
/* Real enum needed by eTaskGetState return type */
typedef enum {
    eRunning_real  = 0,
    eReady_real    = 1,
    eBlocked_real  = 2,
    eSuspended_real= 3,
    eDeleted_real  = 4,
    eInvalid_real  = 5
} eTaskStateEnum;

/* Use a define-based approach so eTaskState works as a type */
#undef eRunning
#undef eReady
#undef eBlocked
#undef eSuspended

typedef enum {
    eRunning   = 0,
    eReady     = 1,
    eBlocked   = 2,
    eSuspended = 3,
    eDeleted   = 4,
    eInvalid   = 5
} eTaskState;

/* ================================================================
 *  Configuration defaults
 * ================================================================ */
#ifndef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE    ((uint16_t)128)
#endif

#ifndef configTIMER_TASK_STACK_DEPTH
#define configTIMER_TASK_STACK_DEPTH ((uint16_t)256)
#endif

#ifndef configUSE_PREEMPTION
#define configUSE_PREEMPTION        1
#endif

/* ================================================================
 *  Macro stubs for interrupt control
 * ================================================================ */
#define portENABLE_INTERRUPTS()     do {} while(0)
#define portDISABLE_INTERRUPTS()    do {} while(0)
#define taskYIELD()                 do {} while(0)

/* ================================================================
 *  Tick conversion macros
 * ================================================================ */
#define pdMS_TO_TICKS(xTimeInMs)    ((TickType_t)(xTimeInMs))

/* ================================================================
 *  Task API prototypes
 * ================================================================ */
BaseType_t xTaskCreate(void (*pvTaskCode)(void*),
                       const char *pcName,
                       configSTACK_DEPTH_TYPE usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask);

void vTaskDelete(TaskHandle_t xTask);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
eTaskState eTaskGetState(TaskHandle_t xTask);
void vTaskStartScheduler(void);
void vTaskEndScheduler(void);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask);
BaseType_t xTaskGetSchedulerState(void);
TickType_t xTaskGetTickCount(void);
void vTaskDelay(TickType_t xTicksToDelay);

/* ================================================================
 *  Semaphore API prototypes
 * ================================================================ */
SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

/* ================================================================
 *  Timer API prototypes
 * ================================================================ */
TimerHandle_t xTimerCreate(const char *pcTimerName,
                           TickType_t xTimerPeriodInTicks,
                           UBaseType_t uxAutoReload,
                           void *pvTimerID,
                           void (*pxCallBack)(TimerHandle_t));

BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerChangePeriod(TimerHandle_t xTimer,
                              TickType_t xNewPeriod,
                              TickType_t xTicksToWait);
void *pvTimerGetTimerID(TimerHandle_t xTimer);

/* ================================================================
 *  Event Group API prototypes
 * ================================================================ */
EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup,
                                EventBits_t uxBitsToSet);
EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup,
                                  EventBits_t uxBitsToClear);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                 EventBits_t uxBitsToWaitFor,
                                 BaseType_t xClearOnExit,
                                 BaseType_t xWaitForAllBits,
                                 TickType_t xTicksToWait);
EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup);

#endif /* FREERTOS_MOCK_H */

/******************************************************************************
 * @file    os_port.c
 * @brief   FreeRTOS Platform Abstraction Layer
 *
 * This file implements the platform-specific OS abstraction layer for FreeRTOS.
 * It wraps FreeRTOS API calls to provide a unified interface for the AUTOSAR
 * OS module.
 *
 * @version 1.0.0
 * @date    2024
 ******************************************************************************/

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* AUTOSAR types */
#include "autosar_types.h"

/******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* Maximum number of supported cores */
#define OS_MAX_CORE_ID          (1U)

/* Default task stack size */
#define OS_DEFAULT_STACK_SIZE   (configMINIMAL_STACK_SIZE)

/* OS state tracking */
typedef enum {
    OS_STATE_UNINITIALIZED = 0,
    OS_STATE_INITIALIZED,
    OS_STATE_RUNNING,
    OS_STATE_ERROR
} Os_StateType;

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static Os_StateType os_state = OS_STATE_UNINITIALIZED;
static TaskHandle_t os_main_task = NULL;

/******************************************************************************
 * Public Functions - Core OS Operations
 ******************************************************************************/

/**
 * @brief Initialize the OS port layer
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Os_Port_Init(void)
{
    if (os_state != OS_STATE_UNINITIALIZED) {
        return E_NOT_OK;
    }

    /* Initialize critical nesting count if needed */
    #ifdef PORT_CRITICAL_NESTING_IN_TCB
    /* FreeRTOS manages critical nesting in TCB */
    #endif

    os_state = OS_STATE_INITIALIZED;
    return E_OK;
}

/**
 * @brief Start the OS scheduler
 * 
 * This function starts the FreeRTOS scheduler. It does not return unless
 * there is insufficient RAM.
 * 
 * @return E_OK if scheduler started successfully, otherwise does not return
 */
Std_ReturnType Os_Port_StartOS(void)
{
    if (os_state != OS_STATE_INITIALIZED) {
        return E_NOT_OK;
    }

    /* Store the current task handle (should be NULL before scheduler starts) */
    os_main_task = xTaskGetCurrentTaskHandle();

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here if successful */
    os_state = OS_STATE_ERROR;
    return E_NOT_OK;
}

/**
 * @brief Get the current core ID
 * 
 * For single-core FreeRTOS, always returns 0.
 * For multi-core (SMP) configurations, returns the current core ID.
 * 
 * @return Current core ID (0 for single-core)
 */
uint32 Os_Port_GetCoreID(void)
{
    #ifdef configNUM_CORES
        /* FreeRTOS SMP support */
        #if (configNUM_CORES > 1)
            return (uint32)xPortGetCoreID();
        #else
            return 0U;
        #endif
    #else
        /* Single-core FreeRTOS */
        return 0U;
    #endif
}

/**
 * @brief Get the current task handle
 * 
 * Maps to FreeRTOS xTaskGetCurrentTaskHandle()
 * 
 * @return Current task handle, or NULL if called before scheduler starts
 */
void* Os_Port_GetCurrentTask(void)
{
    return (void*)xTaskGetCurrentTaskHandle();
}

/******************************************************************************
 * Public Functions - Critical Section and Interrupt Management
 ******************************************************************************/

/**
 * @brief Enter a critical section
 * 
 * Disables interrupts up to the configured priority level.
 * Supports nested critical sections.
 */
void Os_Port_EnterCritical(void)
{
    taskENTER_CRITICAL();
}

/**
 * @brief Exit a critical section
 * 
 * Re-enables interrupts if this is the outermost critical section.
 */
void Os_Port_ExitCritical(void)
{
    taskEXIT_CRITICAL();
}

/**
 * @brief Enter a critical section from ISR context
 * 
 * @return Previous interrupt state (for nesting)
 */
uint32 Os_Port_EnterCriticalFromISR(void)
{
    UBaseType_t uxSavedInterruptStatus;
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    return (uint32)uxSavedInterruptStatus;
}

/**
 * @brief Exit a critical section from ISR context
 * 
 * @param saved_status The value returned by Os_Port_EnterCriticalFromISR()
 */
void Os_Port_ExitCriticalFromISR(uint32 saved_status)
{
    taskEXIT_CRITICAL_FROM_ISR((UBaseType_t)saved_status);
}

/**
 * @brief Disable all interrupts
 */
void Os_Port_DisableInterrupts(void)
{
    taskDISABLE_INTERRUPTS();
}

/**
 * @brief Enable all interrupts
 */
void Os_Port_EnableInterrupts(void)
{
    taskENABLE_INTERRUPTS();
}

/******************************************************************************
 * Public Functions - Task Management
 ******************************************************************************/

/**
 * @brief Yield the current task
 */
void Os_Port_TaskYield(void)
{
    taskYIELD();
}

/**
 * @brief Get the current tick count
 * 
 * @return Current tick count
 */
TickType_t Os_Port_GetTickCount(void)
{
    return xTaskGetTickCount();
}

/**
 * @brief Get the current tick count from ISR context
 * 
 * @return Current tick count
 */
TickType_t Os_Port_GetTickCountFromISR(void)
{
    return xTaskGetTickCountFromISR();
}

/**
 * @brief Convert milliseconds to ticks
 * 
 * @param ms Time in milliseconds
 * @return Equivalent ticks
 */
TickType_t Os_Port_MsToTicks(uint32 ms)
{
    return pdMS_TO_TICKS(ms);
}

/******************************************************************************
 * Public Functions - Delay and Sleep
 ******************************************************************************/

/**
 * @brief Delay the current task for a number of ticks
 * 
 * @param ticks Number of ticks to delay
 */
void Os_Port_Delay(TickType_t ticks)
{
    vTaskDelay(ticks);
}

/**
 * @brief Delay the current task until a specific tick count
 * 
 * @param prev_wake_time Pointer to the previous wake time
 * @param ticks Number of ticks to delay
 */
void Os_Port_DelayUntil(TickType_t* prev_wake_time, TickType_t ticks)
{
    vTaskDelayUntil(prev_wake_time, ticks);
}

/******************************************************************************
 * Public Functions - Scheduler Control
 ******************************************************************************/

/**
 * @brief Suspend the scheduler
 * 
 * Prevents context switches while still allowing interrupts.
 * Supports nesting.
 */
void Os_Port_SuspendScheduler(void)
{
    vTaskSuspendAll();
}

/**
 * @brief Resume the scheduler
 * 
 * @return TRUE if a context switch occurred, FALSE otherwise
 */
boolean Os_Port_ResumeScheduler(void)
{
    return (xTaskResumeAll() == pdTRUE) ? TRUE : FALSE;
}

/**
 * @brief Check if the scheduler is running
 * 
 * @return TRUE if scheduler is running, FALSE otherwise
 */
boolean Os_Port_IsSchedulerRunning(void)
{
    return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) ? TRUE : FALSE;
}

/******************************************************************************
 * Public Functions - System Control
 ******************************************************************************/

/**
 * @brief Request a system reset
 */
void Os_Port_SystemReset(void)
{
    /* Request a context switch to trigger watchdog or reset */
    taskDISABLE_INTERRUPTS();
    
    /* Infinite loop to trigger watchdog */
    for (;;) {
        /* Wait for watchdog reset */
    }
}

/**
 * @brief Get the OS state
 * 
 * @return Current OS state
 */
uint8 Os_Port_GetState(void)
{
    return (uint8)os_state;
}

/******************************************************************************
 * Hook Functions (Weak implementations)
 ******************************************************************************/

/**
 * @brief Idle task hook - called repeatedly when idle
 * 
 * Application can override this weak function.
 */
__attribute__((weak)) void Os_Port_IdleHook(void)
{
    /* Default: do nothing */
}

/**
 * @brief Tick hook - called on each tick interrupt
 * 
 * Application can override this weak function.
 */
__attribute__((weak)) void Os_Port_TickHook(void)
{
    /* Default: do nothing */
}

/**
 * @brief Stack overflow hook - called when a task stack overflows
 * 
 * @param task Handle of the task that overflowed
 * @param name Name of the task that overflowed
 */
__attribute__((weak)) void Os_Port_StackOverflowHook(void* task, char* name)
{
    (void)task;
    (void)name;
    
    /* Disable interrupts and halt */
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Halt - wait for debugger or watchdog */
    }
}

/******************************************************************************
 * FreeRTOS Hook Functions (called by FreeRTOS kernel)
 ******************************************************************************/

#ifdef configUSE_IDLE_HOOK
void vApplicationIdleHook(void)
{
    Os_Port_IdleHook();
}
#endif

#ifdef configUSE_TICK_HOOK
void vApplicationTickHook(void)
{
    Os_Port_TickHook();
}
#endif

#ifdef configCHECK_FOR_STACK_OVERFLOW
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    Os_Port_StackOverflowHook((void*)xTask, pcTaskName);
}
#endif

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
        /* Halt - memory allocation failed */
    }
}

void vAssertCalled(const char* pcFile, unsigned long ulLine)
{
    volatile uint32 ulSetToNonZeroInDebuggerToContinue = 0;

    /* Parameters are not used but need to be present */
    (void)pcFile;
    (void)ulLine;

    taskENTER_CRITICAL();
    {
        /* Set ulSetToNonZeroInDebuggerToContinue to a non-zero value in
         * the debugger to step out of this function. */
        while (ulSetToNonZeroInDebuggerToContinue == 0) {
            /* Wait for debugger intervention */
        }
    }
    taskEXIT_CRITICAL();
}

/******************************************************************************
 * Startup and Initialization
 ******************************************************************************/

/**
 * @brief Initialize hardware before scheduler starts
 * 
 * This function is called by the startup code before the scheduler starts.
 * Application can override this weak function.
 */
__attribute__((weak)) void Os_Port_InitHardware(void)
{
    /* Default: no hardware initialization */
}

/**
 * @brief Early initialization - called before main()
 */
__attribute__((constructor)) void Os_Port_EarlyInit(void)
{
    /* Called before main() - can be used for early hardware setup */
}

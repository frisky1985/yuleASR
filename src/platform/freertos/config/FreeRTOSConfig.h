/******************************************************************************
 * @file    FreeRTOSConfig.h
 * @brief   FreeRTOS Configuration for ETH-DDS Integration
 *
 * @version 1.0.0
 * @date    2024
 ******************************************************************************/
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/******************************************************************************
 * Application Specific Definitions
 ******************************************************************************/

/* Target architecture - POSIX for simulation, ARM for embedded */
#ifdef __linux__
    #define PORTABLE_GCC_POSIX
#else
    #define PORTABLE_GCC_ARM_CM4F
#endif

/******************************************************************************
 * Scheduler Configuration
 ******************************************************************************/

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 168000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 8 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 256 )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_ALTERNATIVE_API               0
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configSTACK_DEPTH_TYPE                  uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/******************************************************************************
 * Memory Allocation Configuration
 ******************************************************************************/

#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 64 * 1024 ) )
#define configAPPLICATION_ALLOCATED_HEAP        0

/******************************************************************************
 * Hook Function Configuration
 ******************************************************************************/

#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/******************************************************************************
 * Run Time and Task Stats Configuration
 ******************************************************************************/

#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/******************************************************************************
 * Co-routine Configuration
 ******************************************************************************/

#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

/******************************************************************************
 * Software Timer Configuration
 ******************************************************************************/

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/******************************************************************************
 * Interrupt Configuration
 ******************************************************************************/

#ifdef PORTABLE_GCC_POSIX
    /* POSIX port specific */
    #define configPOSIX_STACK_SIZE              ( ( unsigned long ) 8192 )
    #define configPOSIX_MESSAGE_QUEUE_JOBS_MAX  10
#else
    /* ARM Cortex-M4 specific */
    #define configPRIO_BITS                     4
    #define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
    #define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
    #define configKERNEL_INTERRUPT_PRIORITY     ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
    #define configMAX_SYSCALL_INTERRUPT_PRIORITY ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#endif

/******************************************************************************
 * Assert Configuration
 ******************************************************************************/

#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/******************************************************************************
 * Definitions for Tracealyzer
 ******************************************************************************/

#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 0

/******************************************************************************
 * Multi-core Configuration (SMP)
 ******************************************************************************/

#define configNUM_CORES                         1
#define configUSE_CORE_AFFINITY                 0
#define configRUN_MULTIPLE_PRIORITIES           0

/******************************************************************************
 * Include C Standard Library Functions
 ******************************************************************************/

#define configUSE_TIMERS_LIB                    1

/******************************************************************************
 * Optional Functions - Comment out to disable
 ******************************************************************************/

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_uxTaskGetStackHighWaterMark2    0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1

/******************************************************************************
 * Platform-specific settings
 ******************************************************************************/

#ifdef __linux__
    /* Linux-specific settings for POSIX port */
    #define configUSE_POSIX_SIGNALS             1
    #define configUSE_POSIX_PTHREAD_MUTEX       1
#endif

/******************************************************************************
 * OS Port Interface
 ******************************************************************************/

/* Allow application to provide port-specific definitions */
#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes for hooks */
void vApplicationIdleHook( void );
void vApplicationTickHook( void );
void vApplicationMallocFailedHook( void );
void vApplicationDaemonTaskStartupHook( void );
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );
void vAssertCalled( const char * const pcFileName, unsigned long ulLine );

/* Platform init functions */
void Os_Port_InitHardware( void );
Std_ReturnType Os_Port_Init( void );
Std_ReturnType Os_Port_StartOS( void );

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_H */

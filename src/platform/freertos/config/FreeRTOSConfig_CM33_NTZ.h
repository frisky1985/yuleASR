/******************************************************************************
 * @file    FreeRTOSConfig_CM33_NTZ.h
 * @brief   FreeRTOS Configuration for ARM Cortex-M33 without TrustZone
 *
 * @version 1.0.0
 * @date    2024
 *
 * @note    Configuration for Cortex-M33 without TrustZone support (NTZ).
 *          Simplified configuration for non-secure applications.
 ******************************************************************************/
#ifndef FREERTOS_CONFIG_CM33_NTZ_H
#define FREERTOS_CONFIG_CM33_NTZ_H

/******************************************************************************
 * Target Architecture
 ******************************************************************************/
#define PORTABLE_GCC_ARM_CM33_NTZ

/******************************************************************************
 * Scheduler Configuration
 ******************************************************************************/

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 1
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 160000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 8 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 256 )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
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
 * Cortex-M33 Specific Configuration
 ******************************************************************************/

/* CM33 has 4 bits of interrupt priority (configurable, 3-8 bits) */
#define configPRIO_BITS                         4

/* The lowest interrupt priority that can be used in a call to a "set priority" function */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* Interrupt priorities used by the kernel port layer itself */
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* configMAX_SYSCALL_INTERRUPT_PRIORITY sets the interrupt priority from which
kernel aware interrupt handlers can call FreeRTOS API functions. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS standard names */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

/******************************************************************************
 * Cortex-M33 TrustZone and Security Configuration (NTZ - No TrustZone)
 ******************************************************************************/

/* Disable TrustZone support - running without TrustZone */
#define configENABLE_TRUSTZONE                  0

/* No secure context management */
#define configENABLE_SECURE_CONTEXT             0

/* Run all tasks in privileged mode */
#define configRUN_FREERTOS_SECURE_ONLY          1

/******************************************************************************
 * Cortex-M33 MPU and FPU Configuration
 ******************************************************************************/

/* Disable MPU support (optional - enable if needed) */
#define configENABLE_MPU                        0

/* Enable hardware FPU support (single-precision for CM33) */
#define configENABLE_FPU                        1

/* Disable Memory Protection Unit wrappers */
#define configUSE_MPU_WRAPPERS_V1               0
#define configUSE_MPU_WRAPPERS_V2               0

/* Allow unprivileged critical sections when MPU is disabled */
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS  1

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

#endif /* FREERTOS_CONFIG_CM33_NTZ_H */

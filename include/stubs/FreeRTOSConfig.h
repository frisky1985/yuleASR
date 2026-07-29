/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS configuration - stub for compilation
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Config options */
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 160000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 130 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 65536 ) )
#define configMAX_TASK_NAME_LEN                 ( 10 )
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configQUEUE_REGISTRY_SIZE               10
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* Software timer */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* Co-routine */
#define configUSE_CO_ROUTINES                   0

/* Memory allocation */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         1

/* Hook function */
#define configUSE_MALLOC_FAILED_HOOK            0

/* Interrupt nesting */
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191

/* Set configASSERT */
#ifndef configASSERT
#define configASSERT( x ) ((void)(x))
#endif

/* Include platform-specific configuration */
#ifdef __arm__
    #include "FreeRTOS_ARM_CM7.h"
#endif

#endif /* FREERTOS_CONFIG_H */

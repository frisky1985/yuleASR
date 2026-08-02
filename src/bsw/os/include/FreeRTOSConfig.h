/*==================================================================================================
 * FreeRTOSConfig.h - FreeRTOS configuration for yuleASR
 *
 * Full configuration for FreeRTOS-Kernel V11.1.0.
 * - Native/host builds use the Posix port (portable/posix).
 * - ARM targets (S32K312 / Cortex-M33) use GCC/ARM_CM33.
 * Tick rate 1000 Hz => 1 tick = 1 ms (matches OS_TICKS_PER_MS in Os_Internal.h).
 *================================================================================================*/
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

/*-----------------------------------------------------------
 * Application specific definitions
 *-----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 160000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 130 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 64 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* Event groups are required by the yuleASR OS abstraction (xEventGroupCreate) */
#define configUSE_EVENT_GROUPS                  1

/* Co-routine related definitions */
#define configUSE_CO_ROUTINES                   0

/* Software timer definitions (used by Os_InitAlarms / SetRelAlarm) */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE )

/* Memory allocation related definitions */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Hook function related definitions */
#define configCHECK_FOR_STACK_OVERFLOW          0

/* Assert (required by Posix port.c) */
#define configASSERT( x )                       assert( ( x ) )

/* ARMv8-M (Cortex-M33) port requirements - S32K312 */
#define configENABLE_FPU                        1
#define configENABLE_MPU                        0
#define configENABLE_TRUSTZONE                  0
#define configENABLE_ARM_MPU                    0
#define configENABLE_ARM_FPU                    1
#define configRUN_FREERTOS_SECURE_ONLY          1

/* ARMv8-M interrupt priorities - S32K312 NVIC has 8 priority bits */
#define configPRIO_BITS                         8
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      0xff
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* Optional functions */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetTickCount               1
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_H */

/*==================================================================================================
 * portmacro.h - FreeRTOS port macros for yuleASR (Cortex-M7, ARM GCC)
 * Provides the base port types and critical-section primitives.
 *================================================================================================*/
#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*-----------------------------------------------------------
 * Port specific types.
 *-----------------------------------------------------------*/
typedef int32_t  BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;

#define portBASE_TYPE                       BaseType_t

/* Maximum delay constant. */
#define portMAX_DELAY                       ( ( TickType_t ) 0xffffffffUL )

/*-----------------------------------------------------------
 * Critical section control.
 *-----------------------------------------------------------*/
#if defined(__aarch64__)
#define portDISABLE_INTERRUPTS()            __asm volatile ( "msr daifset, #2" ::: "memory" )
#define portENABLE_INTERRUPTS()             __asm volatile ( "msr daifclr, #2" ::: "memory" )
#else
#define portDISABLE_INTERRUPTS()            __asm volatile ( "cpsid i" ::: "memory" )
#define portENABLE_INTERRUPTS()             __asm volatile ( "cpsie i" ::: "memory" )
#endif

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portENTER_CRITICAL()                vPortEnterCritical()
#define portEXIT_CRITICAL()                 vPortExitCritical()

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */

/**
 * @file portmacro.h
 * @brief FreeRTOS port macro - stub for compilation
 */
#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "projdefs.h"

/* Type definitions */
typedef uint8_t     StackType_t;
typedef int32_t     BaseType_t;
typedef uint32_t    UBaseType_t;
typedef uint32_t    TickType_t;

/* Critical section macros */
#define portDISABLE_INTERRUPTS()
#define portENABLE_INTERRUPTS()
#define portENTER_CRITICAL()
#define portEXIT_CRITICAL()
#define portSET_INTERRUPT_MASK_FROM_ISR()      0
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)   ((void)(x))

/* Yield */
#define portYIELD()
#define portYIELD_FROM_ISR(x)
#define portEND_SWITCHING_ISR(x)
#define portTICK_TYPE_ENTER_CRITICAL()
#define portTICK_TYPE_EXIT_CRITICAL()

/* Stack macros */
#define portSTACK_TYPE              StackType_t
#define portBASE_TYPE               BaseType_t
#define portPOINTER_SIZE_TYPE       uint32_t

/* Tick */
#define portTICK_PERIOD_MS          ((TickType_t) 1)
#define portTICK_RATE_HZ            ((TickType_t) 1000)
#define portNVIC_INT_CTRL_REG       0xE000ED04
#define portNVIC_PENDSVSET_BIT      (1UL << 28UL)
#define portYIELD_WITHIN_API()

/* Memory barrier */
#define portMEMORY_BARRIER()
#define portNOP()

#endif /* PORTMACRO_H */

/**
 * @file projdefs.h
 * @brief FreeRTOS project definitions - stub for compilation
 */
#ifndef PROJDEFS_H
#define PROJDEFS_H

/* FreeRTOS task notification */
typedef uint32_t TickType_t;
typedef unsigned long BaseType_t;
typedef unsigned long UBaseType_t;

/* Priority definitions */
#define tskIDLE_PRIORITY            ((BaseType_t) 0)
#define tskKERNEL_HIGH_PRIORITY     ((BaseType_t) 1)

/* Timeout definitions */
#define portMAX_DELAY               ((TickType_t) 0xFFFFFFFFUL)
#define portTICK_PERIOD_MS          ((TickType_t) 1)

/* Boolean */
#define pdTRUE                      ((BaseType_t) 1)
#define pdFALSE                     ((BaseType_t) 0)
#define pdPASS                      ((BaseType_t) 1)
#define pdFAIL                      ((BaseType_t) 0)

/* Task states */
#define eRunning                    0
#define eReady                      1
#define eBlocked                    2
#define eSuspended                  3
#define eDeleted                    4
#define eInvalid                    5

#endif /* PROJDEFS_H */

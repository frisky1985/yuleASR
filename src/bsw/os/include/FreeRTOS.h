/* yuleDKCS FreeRTOS port header — delegates to port layer definitions */
#ifndef FREERTOS_H
#define FREERTOS_H

/* Configuration from yuleDKCS port layer */
#include "FreeRTOSConfig.h"

#include <stddef.h>

/* AUTOSAR standard types (provides uint8, uint16, uint32, etc.) */
#include "Std_Types.h"

/* Project definitions (pdTRUE, pdFALSE, TaskFunction_t, etc.) */
#include "projdefs.h"

/* Port-specific macros (StackType_t, TickType_t, etc.) */
#include "portmacro.h"

/* FreeRTOS API headers */
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* Backward compatibility types */
#ifndef portBASE_TYPE
    #define portBASE_TYPE   long
#endif

/* Memory allocation */
void* pvPortMalloc(size_t xWantedSize);
void vPortFree(void* pv);

/* Critical section entry/exit */
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);

#endif /* FREERTOS_H */

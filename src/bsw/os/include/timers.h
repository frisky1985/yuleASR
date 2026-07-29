#ifndef TIMERS_H
#define TIMERS_H

#include "FreeRTOS.h"

/* Timer handle type */
struct tmrTimerControl;
typedef struct tmrTimerControl *TimerHandle_t;

typedef void (*TimerCallbackFunction_t)(TimerHandle_t xTimer);

/* Timer API */
TimerHandle_t xTimerCreate(const char *pcTimerName, TickType_t xTimerPeriodInTicks, UBaseType_t uxAutoReload, void *pvTimerID, TimerCallbackFunction_t pxCallbackFunction);
BaseType_t xTimerIsTimerActive(TimerHandle_t xTimer);
BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerChangePeriod(TimerHandle_t xTimer, TickType_t xNewPeriod, TickType_t xTicksToWait);
BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerStartFromISR(TimerHandle_t xTimer, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xTimerStopFromISR(TimerHandle_t xTimer, BaseType_t *pxHigherPriorityTaskWoken);
void *pvTimerGetTimerID(TimerHandle_t xTimer);
void vTimerSetTimerID(TimerHandle_t xTimer, void *pvNewID);

#endif

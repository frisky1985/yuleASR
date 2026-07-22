/**
 * @file Tm.h
 * @brief Time Manager (Tm) — AUTOSAR BSW Module
 *
 * AUTOSAR R21-11 §12.15: Tm provides centralized time management
 * services for BSW modules including time base synchronization,
 * time conversion, and timing event scheduling.
 */
#ifndef TM_H
#define TM_H

#include "Std_Types.h"

/* Module ID */
#define TM_MODULE_ID             0x0CUL

/* Time base types */
typedef uint64 Tm_TimeBaseType;
typedef uint32 Tm_DurationType;

/* Time base status */
typedef enum {
    TM_STATUS_RUNNING,
    TM_STATUS_STOPPED,
    TM_STATUS_SYNCHRONIZED,
    TM_STATUS_FREE_RUNNING,
    TM_STATUS_ERROR
} Tm_StatusType;

/* Time base information */
typedef struct {
    Tm_TimeBaseType currentValue;
    Tm_DurationType resolution;
    boolean isSynchronized;
    Tm_StatusType status;
} Tm_TimeBaseInfoType;

/* Global time */
typedef struct {
    uint32 secondsHigh;
    uint32 secondsLow;
    uint32 nanoseconds;
} Tm_GlobalTimeType;

/* Configuration */
typedef struct {
    uint8 numTimeBases;
    Tm_DurationType defaultResolution;
    boolean enableSync;
} Tm_ConfigType;

/* Initialization */
Std_ReturnType Tm_Init(const Tm_ConfigType* config);
void Tm_DeInit(void);

/* Main function */
void Tm_MainFunction(void);

/* Time base operations */
Std_ReturnType Tm_GetTimeBaseValue(uint8 timeBaseId, Tm_TimeBaseType* value);
Std_ReturnType Tm_SetTimeBaseValue(uint8 timeBaseId, Tm_TimeBaseType value);
Std_ReturnType Tm_GetTimeBaseInfo(uint8 timeBaseId, Tm_TimeBaseInfoType* info);

/* Global time */
Std_ReturnType Tm_GetGlobalTime(Tm_GlobalTimeType* time);
Std_ReturnType Tm_SetGlobalTime(const Tm_GlobalTimeType* time);

/* Synchronization */
Std_ReturnType Tm_SyncTimeBase(uint8 sourceId, uint8 targetId);

/* Duration */
Tm_DurationType Tm_GetElapsedDuration(uint8 timeBaseId, Tm_TimeBaseType since);

#endif /* TM_H */

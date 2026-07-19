/**
 * @file SchM.h
 * @brief Schedule Manager - AUTOSAR Services Module
 * @version 1.0.0
 * @date 2026-07-19
 * @author YuleTech
 *
 * @details AUTOSAR Schedule Manager (SchM) manages schedule tables
 *          for cyclic activation of BSW functions.
 *
 * @implements AUTOSAR_SWS_ScheduleManager.pdf
 */

#ifndef SCHM_H
#define SCHM_H

#include "Std_Types.h"

#define SCHM_MODULE_ID              0x3AU
#define SCHM_VENDOR_ID              0x0055U

typedef void (*SchM_CallbackType)(void);

typedef struct {
    uint32 TickOffset;
    SchM_CallbackType Callback;
} SchM_SchedulePointType;

typedef struct {
    uint8 TableId;
    uint32 TableDuration;
    boolean TableRepeat;
    uint8 NumSchedulePoints;
    const SchM_SchedulePointType* SchedulePoints;
} SchM_ScheduleTableType;

typedef struct {
    uint8 NumScheduleTables;
    const SchM_ScheduleTableType* ScheduleTables;
} SchM_ConfigType;

void SchM_Init(const SchM_ConfigType* ConfigPtr);
void SchM_DeInit(void);
Std_ReturnType SchM_Start(void);
Std_ReturnType SchM_Stop(void);
Std_ReturnType SchM_SetScheduleTable(uint8 ScheduleId);
uint8 SchM_GetScheduleTable(void);
void SchM_MainFunction(void);
void SchM_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* SCHM_H */
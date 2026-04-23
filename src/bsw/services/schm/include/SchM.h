/**
 * @file SchM.h
 * @brief Scheduler Manager
 * @version 1.0.0
 */

#ifndef SCHM_H
#define SCHM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define SCHM_AR_RELEASE_MAJOR_VERSION       4
#define SCHM_AR_RELEASE_MINOR_VERSION       0
#define SCHM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define SCHM_SW_MAJOR_VERSION               1
#define SCHM_SW_MINOR_VERSION               0
#define SCHM_SW_PATCH_VERSION               0

/* Service IDs */
#define SCHM_INIT_SID                       0x00
#define SCHM_DEINIT_SID                     0x01
#define SCHM_START_SID                      0x02
#define SCHM_STOP_SID                       0x03
#define SCHM_SWITCHPOINT_SID                0x04

/* Error Codes */
#define SCHM_E_NO_ERROR                     0x00
#define SCHM_E_UNINIT                       0x01
#define SCHM_E_ALREADY_INITIALIZED          0x02

/* Types */
typedef enum {
    SCHM_POINT_ZERO = 0,
    SCHM_POINT_ONE,
    SCHM_POINT_TWO,
    SCHM_POINT_MAX
} SchM_PointType;

typedef struct {
    uint16 taskId;
    uint16 priority;
    uint32 period;
    void (*taskFunction)(void);
} SchM_TaskConfigType;

/* Function Prototypes */
extern void SchM_Init(const void* ConfigPtr);
extern void SchM_Deinit(void);
extern void SchM_Start(void);
extern void SchM_Stop(void);
extern void SchM_SwitchPoint(SchM_PointType point);

#endif /* SCHM_H */

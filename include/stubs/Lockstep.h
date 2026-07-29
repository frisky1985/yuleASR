/**
 * @file Lockstep.h
 * @brief Lockstep module header - stub for compilation
 */
#ifndef LOCKSTEP_H
#define LOCKSTEP_H

#include "Std_Types.h"
#include "Platform_Types.h"

/* Lockstep mode */
typedef uint8 Lockstep_ModeType;
#define LOCKSTEP_MODE_OFF       0x00u
#define LOCKSTEP_MODE_ON        0x01u
#define LOCKSTEP_MODE_TEST      0x02u

/* Lockstep event */
typedef uint8 Lockstep_EventType;
#define LOCKSTEP_EVENT_NONE     0x00u
#define LOCKSTEP_EVENT_MISMATCH 0x01u
#define LOCKSTEP_EVENT_TIMEOUT  0x02u
#define LOCKSTEP_EVENT_ERROR    0x03u

/* Lockstep status */
typedef struct {
    boolean Initialized;
    boolean LockstepEnabled;
    boolean ErrorDetected;
    uint32  ErrorCount;
    uint32  LastErrorAddress;
} Lockstep_StatusType;

/* Lockstep configuration */
typedef struct {
    Lockstep_ModeType Mode;
    uint32 MonitorPeriod;
    uint32 ErrorThreshold;
    boolean EnableCrcCheck;
    boolean EnableFccuIntegration;
    boolean EnableDemIntegration;
} Lockstep_ConfigType;

/* API stubs */
extern Std_ReturnType Lockstep_Init(const Lockstep_ConfigType* Config);
extern Std_ReturnType Lockstep_SetMode(Lockstep_ModeType Mode);
extern Std_ReturnType Lockstep_GetStatus(Lockstep_StatusType* Status);
extern Std_ReturnType Lockstep_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* LOCKSTEP_H */

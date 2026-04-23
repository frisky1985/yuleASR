/**
 * @file EcuM.h
 * @brief ECU State Manager
 * @version 1.0.0
 */

#ifndef ECUM_H
#define ECUM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define ECUM_AR_RELEASE_MAJOR_VERSION       4
#define ECUM_AR_RELEASE_MINOR_VERSION       0
#define ECUM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define ECUM_SW_MAJOR_VERSION               1
#define ECUM_SW_MINOR_VERSION               0
#define ECUM_SW_PATCH_VERSION               0

/* Service IDs */
#define ECUM_INIT_SID                       0x00
#define ECUM_SHUTDOWN_SID                   0x01
#define ECUM_REQUESTRUN_SID                 0x02
#define ECUM_RELEASERUN_SID                 0x03
#define ECUM_SELECTSHUTDOWNTARGET_SID       0x04
#define ECUM_GETSTATE_SID                   0x05
#define ECUM_COMMODEREQUEST_SID             0x06
#define ECUM_SETWAKEUPEVENT_SID             0x07
#define ECUM_VALIDATEMCUWAKEUPEVENT_SID     0x08

/* Error Codes */
#define ECUM_E_NOT_INITIALIZED              0x10
#define ECUM_E_INVALID_PAR                  0x11
#define ECUM_E_NULL_POINTER                 0x12
#define ECUM_E_STATE_CHANGE_FAILED          0x13

/* Types */
typedef uint8 EcuM_StateType;
#define ECUM_STATE_STARTUP                  0x00
#define ECUM_STATE_RUN                      0x10
#define ECUM_STATE_POST_RUN                 0x20
#define ECUM_STATE_SLEEP                    0x30
#define ECUM_STATE_SHUTDOWN                 0x40
#define ECUM_STATE_OFF                      0x50

typedef uint8 EcuM_WakeupSourceType;
typedef uint8 EcuM_WakeupStatusType;
#define ECUM_WKSTATUS_NONE                  0x00
#define ECUM_WKSTATUS_PENDING               0x01
#define ECUM_WKSTATUS_VALIDATED             0x02
#define ECUM_WKSTATUS_EXPIRED               0x03

typedef uint8 EcuM_ShutdownTargetType;
#define ECUM_STATE_OFF                      0x00
#define ECUM_STATE_RESET                    0x01
#define ECUM_STATE_SLEEP                    0x02

typedef uint8 EcuM_UserType;

/* Function Prototypes */
extern void EcuM_Init(void);
extern void EcuM_StartupTwo(void);
extern void EcuM_Shutdown(void);
extern Std_ReturnType EcuM_RequestRUN(EcuM_UserType user);
extern Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user);
extern Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode);
extern Std_ReturnType EcuM_GetState(EcuM_StateType* state);
extern void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources);
extern EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(EcuM_WakeupSourceType sources);

#endif /* ECUM_H */

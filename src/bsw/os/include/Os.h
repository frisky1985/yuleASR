/**
 * @file Os.h
 * @brief AutoSAR OS API Header - FreeRTOS Integration
 * @version 1.0.0
 * @date 2026-04-21
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Operating System (OS)
 * Layer: OS Layer
 * Target: FreeRTOS V10.6.x / V11.x
 */

#ifndef OS_H
#define OS_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define OS_VENDOR_ID                    (0x01U) /* YuleTech Vendor ID */
#define OS_MODULE_ID                    (0x01U) /* OS Module ID */
#define OS_AR_RELEASE_MAJOR_VERSION     (0x04U)
#define OS_AR_RELEASE_MINOR_VERSION     (0x04U)
#define OS_AR_RELEASE_REVISION_VERSION  (0x00U)
#define OS_SW_MAJOR_VERSION             (0x01U)
#define OS_SW_MINOR_VERSION             (0x00U)
#define OS_SW_PATCH_VERSION             (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define OS_SID_ACTIVATETASK             (0x01U)
#define OS_SID_TERMINATETASK            (0x02U)
#define OS_SID_CHAINTASK                (0x03U)
#define OS_SID_SCHEDULE                 (0x04U)
#define OS_SID_GETTASKID                (0x05U)
#define OS_SID_GETTASKSTATE             (0x06U)
#define OS_SID_ENABLEALLINTERRUPTS      (0x07U)
#define OS_SID_DISABLEALLINTERRUPTS     (0x08U)
#define OS_SID_RESUMEALLINTERRUPTS      (0x09U)
#define OS_SID_SUSPENDALLINTERRUPTS     (0x0AU)
#define OS_SID_GETRESOURCE              (0x0BU)
#define OS_SID_RELEASERESOURCE          (0x0CU)
#define OS_SID_SETEVENT                 (0x0DU)
#define OS_SID_CLEAREVENT               (0x0EU)
#define OS_SID_GETEVENT                 (0x0FU)
#define OS_SID_WAITEVENT                (0x10U)
#define OS_SID_GETALARMBASE             (0x11U)
#define OS_SID_GETALARM                 (0x12U)
#define OS_SID_SETRELALARM              (0x13U)
#define OS_SID_SETABSALARM              (0x14U)
#define OS_SID_CANCELALARM              (0x15U)
#define OS_SID_STARTOS                  (0x16U)
#define OS_SID_SHUTDOWNOS               (0x17U)
#define OS_SID_GETVERSIONINFO           (0x18U)
#define OS_SID_GETACTIVEAPPLICATIONMODE (0x19U)

/*==================================================================================================
*                                    STATUS TYPE
==================================================================================================*/
typedef uint8 StatusType;

#define E_OS_OK                         ((StatusType)0x00U)
#define E_OS_ACCESS                     ((StatusType)0x01U)
#define E_OS_CALLEVEL                   ((StatusType)0x02U)
#define E_OS_ID                         ((StatusType)0x03U)
#define E_OS_LIMIT                      ((StatusType)0x04U)
#define E_OS_NOFUNC                     ((StatusType)0x05U)
#define E_OS_RESOURCE                   ((StatusType)0x06U)
#define E_OS_STATE                      ((StatusType)0x07U)
#define E_OS_VALUE                      ((StatusType)0x08U)
#define E_OS_SERVICEID                  ((StatusType)0x09U)
#define E_OS_ILLEGAL_ADDRESS            ((StatusType)0x0AU)
#define E_OS_MISSINGEND                 ((StatusType)0x0BU)
#define E_OS_DISABLEDINT                ((StatusType)0x0CU)
#define E_OS_STACKFAULT                 ((StatusType)0x0DU)
#define E_OS_PROTECTION_MEMORY          ((StatusType)0x0EU)
#define E_OS_PROTECTION_TIME            ((StatusType)0x0FU)
#define E_OS_PROTECTION_ARRIVAL         ((StatusType)0x10U)
#define E_OS_PROTECTION_LOCKED          ((StatusType)0x11U)
#define E_OS_PROTECTION_EXCEPTION       ((StatusType)0x12U)

/*==================================================================================================
*                                    TASK TYPES
==================================================================================================*/
typedef uint32 TaskType;
typedef TaskType* TaskRefType;

typedef enum {
    SUSPENDED = 0,
    READY,
    RUNNING,
    WAITING
} TaskStateType;

typedef TaskStateType* TaskStateRefType;

/*==================================================================================================
*                                    EVENT TYPES
==================================================================================================*/
typedef uint32 EventMaskType;
typedef EventMaskType* EventMaskRefType;

/*==================================================================================================
*                                    ALARM TYPES
==================================================================================================*/
typedef uint32 AlarmType;
typedef AlarmType* AlarmRefType;

typedef uint32 TickType;
typedef TickType* TickRefType;

typedef struct {
    TickType maxallowedvalue;
    TickType ticksperbase;
    TickType mincycle;
} AlarmBaseType;

typedef AlarmBaseType* AlarmBaseRefType;

/*==================================================================================================
*                                    RESOURCE TYPES
==================================================================================================*/
typedef uint32 ResourceType;

/*==================================================================================================
*                                    APPLICATION MODE TYPE
==================================================================================================*/
typedef uint8 AppModeType;

#define OSDEFAULTAPPMODE                ((AppModeType)0x01U)

/*==================================================================================================
*                                    COUNTER TYPES
==================================================================================================*/
typedef uint32 Os_CounterType;

/*==================================================================================================
*                                    CONFIGURATION
==================================================================================================*/
#define OS_DEV_ERROR_DETECT             (STD_ON)
#define OS_USE_GET_SERVICE_ID           (STD_ON)
#define OS_USE_PARAMETER_ACCESS         (STD_ON)
#define OS_RES_SCHEDULER                ((ResourceType)0xFFFFFFFFU)

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define OS_START_SEC_CODE
#include "MemMap.h"

/* Task Management */
StatusType ActivateTask(TaskType TaskID);
StatusType TerminateTask(void);
StatusType ChainTask(TaskType TaskID);
StatusType Schedule(void);
StatusType GetTaskID(TaskRefType TaskID);
StatusType GetTaskState(TaskType TaskID, TaskStateRefType State);

/* Interrupt Management */
void EnableAllInterrupts(void);
void DisableAllInterrupts(void);
void ResumeAllInterrupts(void);
void SuspendAllInterrupts(void);
void ResumeOSInterrupts(void);
void SuspendOSInterrupts(void);

/* Resource Management */
StatusType GetResource(ResourceType ResID);
StatusType ReleaseResource(ResourceType ResID);

/* Event Management */
StatusType SetEvent(TaskType TaskID, EventMaskType Mask);
StatusType ClearEvent(EventMaskType Mask);
StatusType GetEvent(TaskType TaskID, EventMaskRefType Event);
StatusType WaitEvent(EventMaskType Mask);

/* Alarm Management */
StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info);
StatusType GetAlarm(AlarmType AlarmID, TickRefType Tick);
StatusType SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle);
StatusType SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle);
StatusType CancelAlarm(AlarmType AlarmID);

/* OS Control */
void StartOS(AppModeType Mode);
void ShutdownOS(StatusType Error);
AppModeType GetActiveApplicationMode(void);

/* Callbacks */
void ErrorHook(StatusType Error);
void PreTaskHook(void);
void PostTaskHook(void);
void StartupHook(void);
void ShutdownHook(StatusType Error);

/* Version Info */
void Os_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* BSW Alarm Callback Dispatcher */
void Os_Callback_Alarm(AlarmType AlarmID);

#define OS_STOP_SEC_CODE
#include "MemMap.h"

#endif /* OS_H */

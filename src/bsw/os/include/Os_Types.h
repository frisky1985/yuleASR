/******************************************************************************
 * @file    Os_Types.h
 * @brief   AUTOSAR OS Module Types Definition
 *
 * AUTOSAR R22-11 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef OS_TYPES_H
#define OS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar_types.h"

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif

/******************************************************************************
 * OS Status Types
 ******************************************************************************/

typedef enum {
    E_OS_OK = 0,
    E_OS_ERROR = 1,
    E_OS_ACCESS = 2,
    E_OS_CALLEVEL = 3,
    E_OS_ID = 4,
    E_OS_LIMIT = 5,
    E_OS_NOFUNC = 6,
    E_OS_RESOURCE = 7,
    E_OS_STATE = 8,
    E_OS_VALUE = 9,
    E_OS_SERVICE_NOT_AVAILABLE = 10,
    E_OS_MISSINGEND = 11,
    E_OS_DISABLEDINT = 12,
    E_OS_STACKFAULT = 13,
    E_OS_PROTECTION_MEMORY = 14,
    E_OS_PROTECTION_TIME = 15,
    E_OS_PROTECTION_ARRIVAL = 16,
    E_OS_PROTECTION_LOCKED = 17,
    E_OS_PROTECTION_EXCEPTION = 18,
    E_OS_CORE = 19,
    E_OS_INTERFERENCE_DEADLOCK = 20,
    E_OS_NESTING_DEADLOCK = 21,
    E_OS_SPINLOCK = 22,
    E_OS_PARAM_POINTER = 23
} StatusType;

/******************************************************************************
 * Task Types
 ******************************************************************************/

typedef uint32 TaskType;
typedef TaskType* TaskRefType;

typedef uint32 TaskStateType;
typedef TaskStateType* TaskStateRefType;

typedef enum {
    SUSPENDED = 0,
    READY,
    RUNNING,
    WAITING,
    INVALID_STATE
} TaskStatusType;

/******************************************************************************
 * ISR Types
 ******************************************************************************/

typedef uint16 ISRType;
typedef uint8 ISRSourceType;

/******************************************************************************
 * Resource Types
 ******************************************************************************/

typedef uint32 ResourceType;
typedef uint32 LimitType;

/******************************************************************************
 * Event Types
 ******************************************************************************/

typedef uint32 EventMaskType;
typedef EventMaskType* EventMaskRefType;

/******************************************************************************
 * Alarm Types
 ******************************************************************************/

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

/******************************************************************************
 * Schedule Table Types
 ******************************************************************************/

typedef uint32 ScheduleTableType;
typedef ScheduleTableType* ScheduleTableRefType;

typedef enum {
    SCHEDULETABLE_STOPPED = 0,
    SCHEDULETABLE_NEXT,
    SCHEDULETABLE_WAITING,
    SCHEDULETABLE_RUNNING,
    SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS
} ScheduleTableStatusType;
typedef ScheduleTableStatusType* ScheduleTableStatusRefType;

/******************************************************************************
 * Counter Types
 ******************************************************************************/

typedef uint32 CounterType;

/******************************************************************************
 * Application Types
 ******************************************************************************/

typedef uint32 ApplicationType;
typedef uint8 AccessType;

typedef enum {
    APPLICATION_ACCESS = 0,
    OBJECT_ACCESS = 1
} ObjectAccessType;

/******************************************************************************
 * Core Types (Multi-core)
 ******************************************************************************/

typedef uint32 CoreIdType;
typedef uint32 CoreNumType;

typedef enum {
    E_OS_COREID = 0,
    E_OS_NOCORE = 1
} CoreStatusType;

/******************************************************************************
 * Spinlock Types (Multi-core)
 ******************************************************************************/

typedef uint32 SpinlockIdType;
typedef SpinlockIdType* SpinlockIdRefType;

typedef enum {
    TRYTOGETSPINLOCK_SUCCESS = 0,
    TRYTOGETSPINLOCK_NOSUCCESS
} TryToGetSpinlockType;
typedef TryToGetSpinlockType* TryToGetSpinlockRefType;

/******************************************************************************
 * Protection Types
 ******************************************************************************/

typedef enum {
    E_OS_PROTECTION_FAULT = 0,
    E_OS_PROTECTION_HOOK
} ProtectionReturnType;

typedef enum {
    PROTECTION_NONE = 0,
    PROTECTION_KILLTASKISR,
    PROTECTION_KILLAPPL,
    PROTECTION_KILLAPPL_RESTART,
    PROTECTION_SHUTDOWN
} ProtectionHookActionType;

/******************************************************************************
 * Restart Types
 ******************************************************************************/

typedef enum {
    NO_RESTART = 0,
    RESTART
} RestartType;

/******************************************************************************
 * Schedule Types
 ******************************************************************************/

typedef enum {
    FULL = 0,
    NON
} ScheduleType;

/******************************************************************************
 * Task Configuration Types
 ******************************************************************************/

typedef uint8 PriorityType;

typedef struct {
    TaskType task;
    PriorityType priority;
    uint32 stack_size;
    boolean autostart;
    boolean extended;
} TaskConfigType;

/******************************************************************************
 * OS Constants
 ******************************************************************************/

#define INVALID_TASK        ((TaskType)0xFFFFFFFFU)
#define INVALID_RESOURCE    ((ResourceType)0xFFFFFFFFU)
#define INVALID_ALARM       ((AlarmType)0xFFFFFFFFU)
#define INVALID_COUNTER     ((CounterType)0xFFFFFFFFU)
#define INVALID_SCHEDULETABLE ((ScheduleTableType)0xFFFFFFFFU)
#define INVALID_ISR         ((ISRType)0xFFFFU)
#define INVALID_CORE        ((CoreIdType)0xFFFFFFFFU)
#define INVALID_SPINLOCK    ((SpinlockIdType)0xFFFFFFFFU)
#define INVALID_APPLICATION ((ApplicationType)0xFFFFFFFFU)

#define RES_SCHEDULER       ((ResourceType)0x00000001U)

#define OSDEFAULTAPPMODE    ((AppModeType)0x00000001U)

typedef uint32 AppModeType;

/******************************************************************************
 * Callback Function Types
 ******************************************************************************/

typedef void (*TaskEntryType)(void);
typedef void (*ISRHandlerType)(void);
typedef void (*AlarmCallbackType)(void);
typedef void (*ProtectionHookType)(StatusType FatalError);
typedef void (*ErrorHookType)(StatusType Error);
typedef void (*PreTaskHookType)(void);
typedef void (*PostTaskHookType)(void);
typedef void (*StartupHookType)(void);
typedef void (*ShutdownHookType)(StatusType Error);

#ifdef __cplusplus
}
#endif

#endif /* OS_TYPES_H */

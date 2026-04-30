/******************************************************************************
 * @file Os.h
 * @brief AutoSAR OS Standard Interface Header
 * @details This file contains the standard AutoSAR OS API definitions
 *          based on AUTOSAR_SWS_OS specification.
 *          Adapted for FreeRTOS implementation.
 *
 * @author YuleTech
 * @version 1.0.0
 * @date 2026-04-30
 *
 * @copyright Copyright (c) 2026 YuleTech
 ******************************************************************************/

#ifndef OS_H
#define OS_H

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "Std_Types.h"

/*******************************************************************************
 * Compiler Abstraction
 ******************************************************************************/
#define OS_CODE             /* Code section */
#define OS_VAR_NOINIT       /* Var noinit section */
#define OS_VAR_CLEARED      /* Var cleared section */
#define OS_CONST            /* Const section */
#define OS_APPL_DATA        /* Application data */
#define OS_APPL_CODE        /* Application code */

/*******************************************************************************
 * Version Info
 ******************************************************************************/
#define OS_VENDOR_ID        (0x00U)
#define OS_MODULE_ID        (0x01U)
#define OS_SW_MAJOR_VERSION (1U)
#define OS_SW_MINOR_VERSION (0U)
#define OS_SW_PATCH_VERSION (0U)

/*******************************************************************************
 * Published Information
 ******************************************************************************/
#define OS_AR_RELEASE_MAJOR_VERSION       (4U)
#define OS_AR_RELEASE_MINOR_VERSION       (4U)
#define OS_AR_RELEASE_REVISION_VERSION    (0U)

/*******************************************************************************
 * Service IDs (for ErrorHook)
 ******************************************************************************/
#define OS_SID_INIT_SYSTEM                  (0x01U)
#define OS_SID_START_OS                     (0x02U)
#define OS_SID_SHUTDOWN_OS                  (0x03U)
#define OS_SID_GET_TASK_ID                  (0x04U)
#define OS_SID_GET_TASK_STATE               (0x05U)
#define OS_SID_ACTIVATE_TASK                (0x06U)
#define OS_SID_TERMINATE_TASK               (0x07U)
#define OS_SID_CHAIN_TASK                   (0x08U)
#define OS_SID_SCHEDULE                     (0x09U)
#define OS_SID_GET_RESOURCE                 (0x0AU)
#define OS_SID_RELEASE_RESOURCE             (0x0BU)
#define OS_SID_SET_EVENT                    (0x0CU)
#define OS_SID_CLEAR_EVENT                  (0x0DU)
#define OS_SID_GET_EVENT                    (0x0EU)
#define OS_SID_WAIT_EVENT                   (0x0FU)
#define OS_SID_GET_ALARM_BASE               (0x10U)
#define OS_SID_GET_ALARM                    (0x11U)
#define OS_SID_SET_REL_ALARM                (0x12U)
#define OS_SID_SET_ABS_ALARM                (0x13U)
#define OS_SID_CANCEL_ALARM                 (0x14U)
#define OS_SID_START_SCHEDULE_TABLE_REL     (0x15U)
#define OS_SID_START_SCHEDULE_TABLE_ABS     (0x16U)
#define OS_SID_STOP_SCHEDULE_TABLE          (0x17U)
#define OS_SID_NEXT_SCHEDULE_TABLE          (0x18U)
#define OS_SID_GET_SCHEDULE_TABLE_STATUS    (0x19U)
#define OS_SID_GET_VERSION_INFO             (0x1AU)
#define OS_SID_DISABLE_ALL_INTERRUPTS       (0x1BU)
#define OS_SID_ENABLE_ALL_INTERRUPTS        (0x1CU)
#define OS_SID_SUSPEND_ALL_INTERRUPTS       (0x1DU)
#define OS_SID_RESUME_ALL_INTERRUPTS        (0x1EU)
#define OS_SID_SUSPEND_OS_INTERRUPTS        (0x1FU)
#define OS_SID_RESUME_OS_INTERRUPTS         (0x20U)
#define OS_SID_GET_ISR_ID                   (0x21U)
#define OS_SID_CALL_TRUSTED_FUNCTION        (0x22U)
#define OS_SID_ENABLE_INTERRUPT_SOURCE      (0x23U)
#define OS_SID_DISABLE_INTERRUPT_SOURCE     (0x24U)
#define OS_SID_GET_APPLICATION_ID           (0x25U)
#define OS_SID_GET_CURRENT_APPLICATION_ID   (0x26U)
#define OS_SID_TERMINATE_APPLICATION        (0x27U)
#define OS_SID_ALLOW_ACCESS                 (0x28U)
#define OS_SID_GET_APPLICATION_STATE        (0x29U)
#define OS_SID_CALL_NON_TRUSTED_FUNCTION    (0x2AU)
#define OS_SID_CHECK_ISR_MEMORY_ACCESS      (0x2BU)
#define OS_SID_CHECK_TASK_MEMORY_ACCESS     (0x2CU)
#define OS_SID_CHECK_OBJECT_ACCESS          (0x2DU)
#define OS_SID_CHECK_OBJECT_IDENTIFICATION  (0x2EU)
#define OS_SID_SET_RELATIVE_ALARM           (0x2FU)
#define OS_SID_SYNC_SCHEDULE_TABLE          (0x30U)
#define OS_SID_START_SCHEDULE_TABLE_SYNCHRONOUS  (0x31U)
#define OS_SID_SET_SCHEDULE_TABLE_ASYNC     (0x32U)
#define OS_SID_INCREMENT_COUNTER            (0x33U)
#define OS_SID_GET_COUNTER_VALUE            (0x34U)
#define OS_SID_GET_ELAPSED_COUNTER_VALUE    (0x35U)
#define OS_SID_ACTIVATE_TASK_ASYNCHRONOUS   (0x36U)
#define OS_SID_SET_EVENT_ASYNCHRONOUS       (0x37U)
#define OS_SID_CALL_FAST_TRUSTED_FUNCTION   (0x38U)
#define OS_SID_SPINLOCK_GET                 (0x39U)
#define OS_SID_SPINLOCK_RELEASE             (0x3AU)
#define OS_SID_SPINLOCK_TRYLOCK             (0x3BU)
#define OS_SID_KILL_TASK                    (0x3CU)
#define OS_SID_CONTROL_IDLE                 (0x3DU)
#define OS_SID_GET_BARRIER_COUNTER          (0x3EU)
#define OS_SID_BARRIER_SYNC                 (0x3FU)
#define OS_SID_GET_STACK_USAGE              (0x40U)

/*******************************************************************************
 * Status Types
 ******************************************************************************/
/* Standard return type for most OS services - undefine Std_Types.h version first */
#ifdef E_OK
#undef E_OK
#endif

typedef enum
{
    E_OK                    = 0,    /* No error */
    E_OS_ACCESS             = 1,    /* Access denied */
    E_OS_CALLEVEL           = 2,    /* Called from wrong context */
    E_OS_ID                 = 3,    /* Invalid ID */
    E_OS_LIMIT              = 4,    /* Limit exceeded */
    E_OS_NOFUNC             = 5,    /* Service not used by this task */
    E_OS_RESOURCE           = 6,    /* Resource still occupied */
    E_OS_STATE              = 7,    /* State not valid for request */
    E_OS_VALUE              = 8,    /* Value outside allowed range */
    E_OS_SERVICEID          = 9,    /* Service ID invalid */
    E_OS_ILLEGAL_ADDRESS    = 10,   /* Illegal address */
    E_OS_MISSINGEND         = 11,   /* Missing EndPreTaskHook/EndPostTaskHook */
    E_OS_DISABLEDINT        = 12,   /* Interrupts disabled */
    E_OS_STACKFAULT         = 13,   /* Stack fault detected */
    E_OS_PROTECTION_MEMORY  = 14,   /* Memory protection error */
    E_OS_PROTECTION_TIME    = 15,   /* Timing protection error */
    E_OS_PROTECTION_LOCKED  = 16,   /* Locking time exceeded */
    E_OS_PROTECTION_EXCEPTION = 17, /* Exception error */
    E_OS_SPINLOCK           = 18,   /* Spinlock error */
    E_OS_INTERFERENCE_DEADLOCK = 19, /* Deadlock in cross core interference */
    E_OS_NESTING_DEADLOCK   = 20,   /* Nesting deadlock */
    E_OS_CORE               = 21,   /* Core error */
    E_OS_SYS_INIT           = 22,   /* Error during OS initialization */
    E_OS_SYS_SUSPEND_NESTING_LIMIT = 23, /* Suspend nesting limit exceeded */
    E_OS_SYS_TASK           = 24,   /* Error in task management */
    E_OS_SYS_STACK          = 25,   /* Stack overflow */
    E_OS_SYS_ACTIVATION     = 26    /* Too many task activations */
} StatusType;

/*******************************************************************************
 * Object Types
 ******************************************************************************/

/* Task Type */
typedef uint32 TaskType;
typedef TaskType *TaskRefType;

/* Task State Type */
typedef enum
{
    RUNNING     = 0,    /* Task currently running */
    WAITING     = 1,    /* Task waiting for event/resource */
    READY       = 2,    /* Task ready to run */
    SUSPENDED   = 3,    /* Task suspended/not activated */
    INVALID_TASK = 4    /* Invalid task */
} TaskStateType;
typedef TaskStateType *TaskStateRefType;

/* Task Activation Count Type */
typedef uint8 TaskActivationCountType;

/* Event Mask Type */
typedef uint32 EventMaskType;
typedef EventMaskType *EventMaskRefType;

/* Alarm Type */
typedef uint32 AlarmType;

/* Tick Type - used for timing */
typedef uint32 TickType;

/* Alarm Base Type */
typedef struct
{
    TickType maxallowedvalue;       /* Maximum allowed counter value */
    TickType ticksperbase;          /* Number of ticks per base */
    TickType mincycle;              /* Smallest allowed cycle */
} AlarmBaseType;
typedef AlarmBaseType *AlarmBaseRefType;

/* Schedule Table Type */
typedef uint32 ScheduleTableType;
typedef ScheduleTableType *ScheduleTableRefType;

/* Schedule Table Status Type */
typedef enum
{
    SCHEDULETABLE_RUNNING           = 0,
    SCHEDULETABLE_WAITING           = 1,
    SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS = 2,
    SCHEDULETABLE_STOPPED           = 3,
    SCHEDULETABLE_NEXT              = 4
} ScheduleTableStatusType;
typedef ScheduleTableStatusType *ScheduleTableStatusRefType;

/* Counter Type */
typedef uint32 CounterType;

/* ISR Type */
typedef uint16 ISRType;
typedef ISRType *ISRRefType;

/* Application Type */
typedef uint16 ApplicationType;
#define INVALID_OSAPPLICATION ((ApplicationType)0xFFU)

/* Application State Type */
typedef enum
{
    APPLICATION_ACCESSIBLE      = 0,
    APPLICATION_RESTARTING      = 1,
    APPLICATION_TERMINATED      = 2
} ApplicationStateType;
typedef ApplicationStateType *ApplicationStateRefType;

/* Resource Type */
typedef uint32 ResourceType;
#define RES_SCHEDULER ((ResourceType)0xFFFFFFFFU)

/* Spinlock Type */
typedef uint32 SpinlockIdType;
#define INVALID_SPINLOCK ((SpinlockIdType)0xFFFFFFFFU)

typedef enum
{
    TRYTOGETSPINLOCK_NOSUCCESS  = 0,
    TRYTOGETSPINLOCK_SUCCESS    = 1
} TryToGetSpinlockType;
typedef TryToGetSpinlockType *TryToGetSpinlockRefType;

/* Protection Return Type */
typedef enum
{
    PRO_KILLTASK                = 0,
    PRO_KILLAPPL                = 1,
    PRO_KILLAPPL_RESTART        = 2,
    PRO_SHUTDOWN                = 3
} ProtectionReturnType;

/* Restart Type */
typedef enum
{
    NO_RESTART                  = 0,
    RESTART                     = 1
} RestartType;

/* Idle Mode Type */
typedef enum
{
    IDLE_NO_HALT                = 0,
    IDLE_HALT                   = 1,
    IDLE_SLEEP                  = 2
} IdleModeType;

/* Barrier Id Type */
typedef uint8 BarrierIdType;

/*******************************************************************************
 * Hook Function Types
 ******************************************************************************/

/* Error Hook - Called when an error occurs */
extern void Os_ErrorHook(StatusType error);

/* Pre-Task Hook - Called before task switch */
extern void Os_PreTaskHook(void);

/* Post-Task Hook - Called after task switch */
extern void Os_PostTaskHook(void);

/* Startup Hook - Called during OS startup */
extern void Os_StartupHook(void);

/* Shutdown Hook - Called during OS shutdown */
extern void Os_ShutdownHook(StatusType error);

/* Protection Hook - Called when protection error occurs */
extern ProtectionReturnType Os_ProtectionHook(StatusType fatalError);

/* Idle Hook - Called in idle loop */
extern void Os_IdleHook(void);

/*******************************************************************************
 * Configuration Include
 ******************************************************************************/
#include "Os_Cfg.h"

/*******************************************************************************
 * Task Management API
 ******************************************************************************/

/**
 * @brief Activate a task
 * @param TaskID - Reference to the task to be activated
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ActivateTask(TaskType TaskID);

/**
 * @brief Terminate the calling task
 * @return StatusType - E_OK or error code (task doesn't terminate if error)
 */
extern StatusType Os_TerminateTask(void);

/**
 * @brief Chain task (terminate current and activate another)
 * @param TaskID - Task to activate after current terminates
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ChainTask(TaskType TaskID);

/**
 * @brief Get the identifier of the running task
 * @param TaskRef - Reference to store the task ID
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetTaskID(TaskRefType TaskRef);

/**
 * @brief Get the state of a task
 * @param TaskID - Task to query
 * @param State - Reference to store the state
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetTaskState(TaskType TaskID, TaskStateRefType State);

/**
 * @brief Request a schedule point (reschedule if higher priority task ready)
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_Schedule(void);

/**
 * @brief Create a task (optional - depends on implementation)
 * @param TaskID - Task identifier
 * @return StatusType - E_OK or error code
 * @note This is implementation specific, AutoSAR tasks are typically static
 */
extern StatusType Os_CreateTask(TaskType TaskID);

/*******************************************************************************
 * Event Management API
 ******************************************************************************/

/**
 * @brief Set an event for a task
 * @param TaskID - Task to set event for
 * @param Mask - Event mask to set
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_SetEvent(TaskType TaskID, EventMaskType Mask);

/**
 * @brief Wait for events
 * @param Mask - Events to wait for
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_WaitEvent(EventMaskType Mask);

/**
 * @brief Clear events
 * @param Mask - Events to clear
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ClearEvent(EventMaskType Mask);

/**
 * @brief Get events for a task
 * @param TaskID - Task to query
 * @param Event - Reference to store the event mask
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetEvent(TaskType TaskID, EventMaskRefType Event);

/*******************************************************************************
 * Alarm Management API
 ******************************************************************************/

/**
 * @brief Set a relative alarm
 * @param AlarmID - Alarm to set
 * @param increment - Relative offset from now
 * @param cycle - Cycle time (0 = one-shot)
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle);

/**
 * @brief Set an absolute alarm
 * @param AlarmID - Alarm to set
 * @param start - Absolute start value
 * @param cycle - Cycle time (0 = one-shot)
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle);

/**
 * @brief Cancel an alarm
 * @param AlarmID - Alarm to cancel
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_CancelAlarm(AlarmType AlarmID);

/**
 * @brief Get remaining ticks until alarm expires
 * @param AlarmID - Alarm to query
 * @param Tick - Reference to store remaining ticks
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetAlarm(AlarmType AlarmID, TickRefType Tick);

/**
 * @brief Get alarm base characteristics
 * @param AlarmID - Alarm to query
 * @param Info - Reference to store alarm base info
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info);

/*******************************************************************************
 * Resource Management API
 ******************************************************************************/

/**
 * @brief Get a resource (enter critical section with priority ceiling)
 * @param ResID - Resource to get
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetResource(ResourceType ResID);

/**
 * @brief Release a resource
 * @param ResID - Resource to release
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ReleaseResource(ResourceType ResID);

/*******************************************************************************
 * Schedule Table Management API
 ******************************************************************************/

/**
 * @brief Start a schedule table relative to current counter value
 * @param ScheduleTableID - Schedule table to start
 * @param Offset - Relative offset from now
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset);

/**
 * @brief Start a schedule table absolute (at specific counter value)
 * @param ScheduleTableID - Schedule table to start
 * @param Start - Absolute start value
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start);

/**
 * @brief Stop a schedule table
 * @param ScheduleTableID - Schedule table to stop
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_StopScheduleTable(ScheduleTableType ScheduleTableID);

/**
 * @brief Chain schedule tables (switch from one to another)
 * @param ScheduleTableID_From - Current schedule table
 * @param ScheduleTableID_To - Schedule table to switch to
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_NextScheduleTable(ScheduleTableType ScheduleTableID_From, 
                                        ScheduleTableType ScheduleTableID_To);

/**
 * @brief Get schedule table status
 * @param ScheduleTableID - Schedule table to query
 * @param ScheduleStatus - Reference to store status
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetScheduleTableStatus(ScheduleTableType ScheduleTableID,
                                             ScheduleTableStatusRefType ScheduleStatus);

/*******************************************************************************
 * System Control API
 ******************************************************************************/

/**
 * @brief Start the OS
 * @param Mode - Application mode
 */
extern void Os_StartOS(AppModeType Mode);

/**
 * @brief Shutdown the OS
 * @param Error - Error code for shutdown
 */
extern void Os_ShutdownOS(StatusType Error);

/**
 * @brief Get OS version info
 * @param versioninfo - Reference to store version info
 */
extern void Os_GetVersionInfo(Std_VersionInfoType *versioninfo);

/*******************************************************************************
 * Interrupt Management API
 ******************************************************************************/

/**
 * @brief Disable all interrupts
 */
extern void Os_DisableAllInterrupts(void);

/**
 * @brief Enable all interrupts
 */
extern void Os_EnableAllInterrupts(void);

/**
 * @brief Suspend all interrupts (nestable)
 */
extern void Os_SuspendAllInterrupts(void);

/**
 * @brief Resume all interrupts (nestable)
 */
extern void Os_ResumeAllInterrupts(void);

/**
 * @brief Suspend OS interrupts (category 2 only)
 */
extern void Os_SuspendOSInterrupts(void);

/**
 * @brief Resume OS interrupts (category 2 only)
 */
extern void Os_ResumeOSInterrupts(void);

/**
 * @brief Disable interrupt source
 * @param ISRID - ISR to disable
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_DisableInterruptSource(ISRType ISRID);

/**
 * @brief Enable interrupt source
 * @param ISRID - ISR to enable
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_EnableInterruptSource(ISRType ISRID);

/**
 * @brief Clear pending interrupt
 * @param ISRID - ISR to clear
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ClearPendingInterrupt(ISRType ISRID);

/*******************************************************************************
 * Counter Management API
 ******************************************************************************/

/**
 * @brief Increment a counter (for software counters)
 * @param CounterID - Counter to increment
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_IncrementCounter(CounterType CounterID);

/**
 * @brief Get current counter value
 * @param CounterID - Counter to query
 * @param Value - Reference to store value
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetCounterValue(CounterType CounterID, TickRefType Value);

/**
 * @brief Get elapsed counter value
 * @param CounterID - Counter to query
 * @param Value - Reference to store value (in/out)
 * @param ElapsedValue - Reference to store elapsed value
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetElapsedCounterValue(CounterType CounterID, 
                                             TickRefType Value,
                                             TickRefType ElapsedValue);

/*******************************************************************************
 * Spinlock API (for multicore)
 ******************************************************************************/

/**
 * @brief Get a spinlock
 * @param SpinlockId - Spinlock to get
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetSpinlock(SpinlockIdType SpinlockId);

/**
 * @brief Release a spinlock
 * @param SpinlockId - Spinlock to release
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ReleaseSpinlock(SpinlockIdType SpinlockId);

/**
 * @brief Try to get spinlock (non-blocking)
 * @param SpinlockId - Spinlock to try
 * @param Success - Reference to store result
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_TryToGetSpinlock(SpinlockIdType SpinlockId,
                                       TryToGetSpinlockRefType Success);

/*******************************************************************************
 * IOC (Inter-OS-Application Communication) API
 ******************************************************************************/

/* IOC interface is implementation specific */

/*******************************************************************************
 * Memory Protection API
 ******************************************************************************/

/* Memory protection is implementation specific */

/*******************************************************************************
 * Timing Protection API
 ******************************************************************************/

/**
 * @brief Get task execution time
 * @param TaskID - Task to query
 * @param Value - Reference to store execution time
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetTaskExecutionTime(TaskType TaskID, TimeInMicrosecondsType *Value);

/**
 * @brief Start execution time measurement for a task
 * @param TaskID - Task to start measurement for
 * @return StatusType - E_OK or error code
 * @details Called when task starts executing to begin timing protection monitoring
 */
extern StatusType Os_StartExecutionTimeMeasurement(TaskType TaskID);

/**
 * @brief Stop execution time measurement for a task
 * @param TaskID - Task to stop measurement for
 * @return StatusType - E_OK or error code
 * @details Called when task stops executing (suspends/terminates)
 */
extern StatusType Os_StopExecutionTimeMeasurement(TaskType TaskID);

/**
 * @brief Check if task execution time budget is exceeded
 * @param TaskID - Task to check
 * @return StatusType - E_OK if within budget, E_OS_PROTECTION_TIME if exceeded
 * @details Should be called periodically during task execution
 */
extern StatusType Os_CheckExecutionTimeBudget(TaskType TaskID);

/**
 * @brief Check task arrival time (inter-arrival time)
 * @param TaskID - Task being activated
 * @return StatusType - E_OK if activation is allowed, E_OS_PROTECTION_TIME if too frequent
 * @details Called when task is activated to check inter-arrival timing
 */
extern StatusType Os_CheckTaskArrivalTime(TaskType TaskID);

/**
 * @brief Start resource lock time measurement
 * @param ResourceID - Resource being locked
 * @return void
 * @details Called when GetResource is invoked
 */
extern void Os_StartResourceLockTimeMeasurement(ResourceType ResourceID);

/**
 * @brief Check if resource lock time budget is exceeded
 * @param ResourceID - Resource being checked
 * @return StatusType - E_OK if within budget, E_OS_PROTECTION_LOCKED if exceeded
 * @details Called periodically and before ReleaseResource
 */
extern StatusType Os_CheckResourceLockTime(ResourceType ResourceID);

/**
 * @brief Stop resource lock time measurement
 * @param ResourceID - Resource being released
 * @return void
 * @details Called when ReleaseResource is invoked
 */
extern void Os_StopResourceLockTimeMeasurement(ResourceType ResourceID);

/**
 * @brief Start all interrupts lock time measurement
 * @return void
 * @details Called when DisableAllInterrupts or SuspendAllInterrupts is invoked
 */
extern void Os_StartAllInterruptsLockMeasurement(void);

/**
 * @brief Check if all interrupts lock time budget is exceeded
 * @return StatusType - E_OK if within budget, E_OS_PROTECTION_LOCKED if exceeded
 */
extern StatusType Os_CheckAllInterruptsLockTime(void);

/**
 * @brief Stop all interrupts lock time measurement
 * @return void
 * @details Called when EnableAllInterrupts or ResumeAllInterrupts is invoked
 */
extern void Os_StopAllInterruptsLockMeasurement(void);

/**
 * @brief Start OS interrupts lock time measurement
 * @return void
 * @details Called when SuspendOSInterrupts is invoked
 */
extern void Os_StartOsInterruptsLockMeasurement(void);

/**
 * @brief Check if OS interrupts lock time budget is exceeded
 * @return StatusType - E_OK if within budget, E_OS_PROTECTION_LOCKED if exceeded
 */
extern StatusType Os_CheckOsInterruptsLockTime(void);

/**
 * @brief Stop OS interrupts lock time measurement
 * @return void
 * @details Called when ResumeOSInterrupts is invoked
 */
extern void Os_StopOsInterruptsLockMeasurement(void);

/**
 * @brief Get current timestamp in microseconds
 * @return TimeInMicrosecondsType - Current time in microseconds
 * @details Platform-specific implementation required
 */
extern TimeInMicrosecondsType Os_GetCurrentTimeInUs(void);

/**
 * @brief Handle timing protection violation
 * @param ViolationType - Type of timing violation detected
 * @param ObjectID - ID of object that caused violation (task, ISR, resource)
 * @return ProtectionReturnType - Action to take
 * @details Called internally when timing protection violation is detected
 */
extern ProtectionReturnType Os_HandleTimingViolation(Os_TimingViolationType ViolationType, uint32 ObjectID);

/**
 * @brief Initialize timing protection module
 * @return StatusType - E_OK if initialization successful
 * @details Called during OS initialization
 */
extern StatusType Os_InitTimingProtection(void);

/**
 * @brief Get timing protection state for a task
 * @param TaskID - Task to query
 * @param State - Reference to store timing protection state
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetTimingProtectionState(TaskType TaskID, Os_TimingProtectionStateType *State);

/**
 * @brief Get last timing violation information
 * @param TaskID - Task to query
 * @param ViolationType - Reference to store violation type
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_GetLastTimingViolation(TaskType TaskID, Os_TimingViolationType *ViolationType);

/*******************************************************************************
 * Control API
 ******************************************************************************/

/**
 * @brief Control idle behavior
 * @param CoreID - Core to control
 * @param IdleMode - Idle mode
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_ControlIdle(CoreIdType CoreID, IdleModeType IdleMode);

/*******************************************************************************
 * Trusted Function API
 ******************************************************************************/

/**
 * @brief Call a trusted function
 * @param FunctionIndex - Function to call
 * @param FunctionParams - Parameters
 * @return StatusType - E_OK or error code
 */
extern StatusType Os_CallTrustedFunction(TrustedFunctionIndexType FunctionIndex,
                                          TrustedFunctionParameterRefType FunctionParams);

/*******************************************************************************
 * Multi-Core API
 ******************************************************************************/

/**
 * @brief Get core ID
 * @return CoreIdType - Current core ID
 */
extern CoreIdType Os_GetCoreID(void);

/**
 * @brief Get number of cores
 * @return uint32 - Number of cores
 */
extern uint32 Os_GetNumberOfActivatedCores(void);

/**
 * @brief Start a core
 * @param CoreID - Core to start
 * @param Status - Reference to store status
 */
extern void Os_StartCore(CoreIdType CoreID, StatusType *Status);

/**
 * @brief Start non-autosar cores
 * @param CoreID - Core to start
 * @param Status - Reference to store status
 */
extern void Os_StartNonAutosarCore(CoreIdType CoreID, StatusType *Status);

/**
 * @brief Get active application mode
 * @return AppModeType - Current application mode
 */
extern AppModeType Os_GetActiveApplicationMode(void);

/*******************************************************************************
 * Macros for backward compatibility
 ******************************************************************************/

#define ActivateTask(TaskID)                    Os_ActivateTask(TaskID)
#define TerminateTask()                         Os_TerminateTask()
#define ChainTask(TaskID)                       Os_ChainTask(TaskID)
#define GetTaskID(TaskRef)                      Os_GetTaskID(TaskRef)
#define GetTaskState(TaskID, State)             Os_GetTaskState(TaskID, State)
#define Schedule()                              Os_Schedule()
#define GetResource(ResID)                      Os_GetResource(ResID)
#define ReleaseResource(ResID)                  Os_ReleaseResource(ResID)
#define SetEvent(TaskID, Mask)                  Os_SetEvent(TaskID, Mask)
#define ClearEvent(Mask)                        Os_ClearEvent(Mask)
#define GetEvent(TaskID, Event)                 Os_GetEvent(TaskID, Event)
#define WaitEvent(Mask)                         Os_WaitEvent(Mask)
#define SetRelAlarm(AlarmID, increment, cycle)  Os_SetRelAlarm(AlarmID, increment, cycle)
#define SetAbsAlarm(AlarmID, start, cycle)      Os_SetAbsAlarm(AlarmID, start, cycle)
#define CancelAlarm(AlarmID)                    Os_CancelAlarm(AlarmID)
#define GetAlarm(AlarmID, Tick)                 Os_GetAlarm(AlarmID, Tick)
#define GetAlarmBase(AlarmID, Info)             Os_GetAlarmBase(AlarmID, Info)
#define StartScheduleTableRel(ScheduleTableID, Offset)  Os_StartScheduleTableRel(ScheduleTableID, Offset)
#define StartScheduleTableAbs(ScheduleTableID, Start)   Os_StartScheduleTableAbs(ScheduleTableID, Start)
#define StopScheduleTable(ScheduleTableID)      Os_StopScheduleTable(ScheduleTableID)
#define NextScheduleTable(ScheduleTableID_From, ScheduleTableID_To) Os_NextScheduleTable(ScheduleTableID_From, ScheduleTableID_To)
#define GetScheduleTableStatus(ScheduleTableID, ScheduleStatus) Os_GetScheduleTableStatus(ScheduleTableID, ScheduleStatus)
#define StartOS(Mode)                           Os_StartOS(Mode)
#define ShutdownOS(Error)                       Os_ShutdownOS(Error)
#define DisableAllInterrupts()                  Os_DisableAllInterrupts()
#define EnableAllInterrupts()                   Os_EnableAllInterrupts()
#define SuspendAllInterrupts()                  Os_SuspendAllInterrupts()
#define ResumeAllInterrupts()                   Os_ResumeAllInterrupts()
#define SuspendOSInterrupts()                   Os_SuspendOSInterrupts()
#define ResumeOSInterrupts()                    Os_ResumeOSInterrupts()

/*******************************************************************************
 * Internal Functions (for OS implementation use only)
 ******************************************************************************/

/* These functions are implementation specific and may vary */

#endif /* OS_H */

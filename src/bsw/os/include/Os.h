/******************************************************************************
 * @file    Os.h
 * @brief   AUTOSAR OS Module Interface Header
 *
 * AUTOSAR R22-11 compliant
 * Provides standard OS services and hooks
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef OS_H
#define OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Os_Types.h"
#include "Os_Cfg.h"

/******************************************************************************
 * OS Version Information
 ******************************************************************************/

#define OS_VENDOR_ID                (0x01U)
#define OS_MODULE_ID                (0x01U)
#define OS_SW_MAJOR_VERSION         (1U)
#define OS_SW_MINOR_VERSION         (0U)
#define OS_SW_PATCH_VERSION         (0U)

/******************************************************************************
 * API Services - Task Management
 ******************************************************************************/

/**
 * @brief Activate a task
 * @param TaskID The task to be activated
 * @return StatusType E_OS_OK if successful
 */
extern StatusType ActivateTask(TaskType TaskID);

/**
 * @brief Terminate the calling task
 * @return StatusType E_OS_OK if successful
 */
extern StatusType TerminateTask(void);

/**
 * @brief Chain execution to another task
 * @param TaskID The task to chain to
 * @return StatusType E_OS_OK if successful
 */
extern StatusType ChainTask(TaskType TaskID);

/**
 * @brief Schedule the next ready task
 * @return StatusType E_OS_OK if successful
 */
extern StatusType Schedule(void);

/**
 * @brief Get the ID of the current task
 * @param TaskID Reference to store the task ID
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetTaskID(TaskRefType TaskID);

/**
 * @brief Get the state of a task
 * @param TaskID The task to query
 * @param State Reference to store the state
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetTaskState(TaskType TaskID, TaskStateRefType State);

/******************************************************************************
 * API Services - ISR Management
 ******************************************************************************/

/**
 * @brief Enable all interrupts
 */
extern void EnableAllInterrupts(void);

/**
 * @brief Disable all interrupts
 */
extern void DisableAllInterrupts(void);

/**
 * @brief Resume OS interrupts
 */
extern void ResumeOSInterrupts(void);

/**
 * @brief Suspend OS interrupts
 */
extern void SuspendOSInterrupts(void);

/**
 * @brief Resume all interrupts
 */
extern void ResumeAllInterrupts(void);

/**
 * @brief Suspend all interrupts
 */
extern void SuspendAllInterrupts(void);

/******************************************************************************
 * API Services - Resource Management
 ******************************************************************************/

/**
 * @brief Get a resource
 * @param ResID The resource to get
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetResource(ResourceType ResID);

/**
 * @brief Release a resource
 * @param ResID The resource to release
 * @return StatusType E_OS_OK if successful
 */
extern StatusType ReleaseResource(ResourceType ResID);

/******************************************************************************
 * API Services - Event Management
 ******************************************************************************/

/**
 * @brief Set an event for a task
 * @param TaskID The target task
 * @param Mask The event mask to set
 * @return StatusType E_OS_OK if successful
 */
extern StatusType SetEvent(TaskType TaskID, EventMaskType Mask);

/**
 * @brief Clear an event
 * @param Mask The event mask to clear
 * @return StatusType E_OS_OK if successful
 */
extern StatusType ClearEvent(EventMaskType Mask);

/**
 * @brief Get the current event state
 * @param TaskID The task to query
 * @param Event Reference to store the event mask
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetEvent(TaskType TaskID, EventMaskRefType Event);

/**
 * @brief Wait for events
 * @param Mask The events to wait for
 * @return StatusType E_OS_OK if successful
 */
extern StatusType WaitEvent(EventMaskType Mask);

/******************************************************************************
 * API Services - Alarm Management
 ******************************************************************************/

/**
 * @brief Get alarm base information
 * @param AlarmID The alarm to query
 * @param Info Reference to store the base information
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info);

/**
 * @brief Get the current relative value of an alarm
 * @param AlarmID The alarm to query
 * @param Tick Reference to store the tick count
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetAlarm(AlarmType AlarmID, TickRefType Tick);

/**
 * @brief Set a relative alarm
 * @param AlarmID The alarm to set
 * @param increment Relative value in ticks
 * @param cycle Cycle period (0 for one-shot)
 * @return StatusType E_OS_OK if successful
 */
extern StatusType SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle);

/**
 * @brief Set an absolute alarm
 * @param AlarmID The alarm to set
 * @param start Absolute start value
 * @param cycle Cycle period (0 for one-shot)
 * @return StatusType E_OS_OK if successful
 */
extern StatusType SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle);

/**
 * @brief Cancel an alarm
 * @param AlarmID The alarm to cancel
 * @return StatusType E_OS_OK if successful
 */
extern StatusType CancelAlarm(AlarmType AlarmID);

/******************************************************************************
 * API Services - OS Control
 ******************************************************************************/

/**
 * @brief Get the current application mode
 * @return AppModeType The current application mode
 */
extern AppModeType GetActiveApplicationMode(void);

/**
 * @brief Start the OS
 * @param Mode The application mode to start in
 */
extern void StartOS(AppModeType Mode);

/**
 * @brief Shutdown the OS
 * @param Error The error code for shutdown
 */
extern void ShutdownOS(StatusType Error);

/******************************************************************************
 * API Services - Schedule Tables
 ******************************************************************************/

/**
 * @brief Start a schedule table
 * @param ScheduleTableID The schedule table to start
 * @param Offset The offset from now to start
 * @return StatusType E_OS_OK if successful
 */
extern StatusType StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset);

/**
 * @brief Start a schedule table absolutely
 * @param ScheduleTableID The schedule table to start
 * @param Start The absolute start value
 * @return StatusType E_OS_OK if successful
 */
extern StatusType StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start);

/**
 * @brief Stop a schedule table
 * @param ScheduleTableID The schedule table to stop
 * @return StatusType E_OS_OK if successful
 */
extern StatusType StopScheduleTable(ScheduleTableType ScheduleTableID);

/**
 * @brief Get the status of a schedule table
 * @param ScheduleTableID The schedule table to query
 * @param ScheduleStatus Reference to store the status
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetScheduleTableStatus(ScheduleTableType ScheduleTableID, ScheduleTableStatusRefType ScheduleStatus);

/******************************************************************************
 * API Services - Counter
 ******************************************************************************/

/**
 * @brief Increment a counter
 * @param CounterID The counter to increment
 * @return StatusType E_OS_OK if successful
 */
extern StatusType IncrementCounter(CounterType CounterID);

/**
 * @brief Get the current counter value
 * @param CounterID The counter to query
 * @param Value Reference to store the value
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetCounterValue(CounterType CounterID, TickRefType Value);

/**
 * @brief Get the elapsed counter value
 * @param CounterID The counter to query
 * @param Value Reference to store the elapsed value
 * @return StatusType E_OS_OK if successful
 */
extern StatusType GetElapsedValue(CounterType CounterID, TickRefType Value);

/******************************************************************************
 * API Services - Multi-core
 ******************************************************************************/

/**
 * @brief Get the ID of the current core
 * @return CoreIdType The current core ID
 */
extern CoreIdType GetCoreID(void);

/**
 * @brief Get the number of cores
 * @return CoreNumType The number of cores
 */
extern CoreNumType GetNumberOfActivatedCores(void);

/**
 * @brief Start a core
 * @param CoreID The core to start
 * @param Status Reference to store the status
 */
extern void StartCore(CoreIdType CoreID, StatusType* Status);

/**
 * @brief Start non-autostart cores
 */
extern void StartNonAutosarCore(CoreIdType CoreID, StatusType* Status);

/******************************************************************************
 * Hook Functions - To be implemented by application
 ******************************************************************************/

/**
 * @brief Error hook - called on OS errors
 * @param Error The error code
 */
extern void ErrorHook(StatusType Error);

/**
 * @brief Pre-task hook - called before task activation
 */
extern void PreTaskHook(void);

/**
 * @brief Post-task hook - called after task termination
 */
extern void PostTaskHook(void);

/**
 * @brief Startup hook - called during OS startup
 */
extern void StartupHook(void);

/**
 * @brief Shutdown hook - called during OS shutdown
 * @param Error The error code
 */
extern void ShutdownHook(StatusType Error);

/**
 * @brief Protection hook - called on protection violations
 * @param FatalError The error code
 * @return ProtectionReturnType The action to take
 */
extern ProtectionReturnType ProtectionHook(StatusType FatalError);

/******************************************************************************
 * Internal Functions (Platform Port Interface)
 ******************************************************************************/

/**
 * @brief Initialize OS port layer
 * @return Std_ReturnType E_OK if successful
 */
extern Std_ReturnType Os_Port_Init(void);

/**
 * @brief Start OS scheduler (port-specific)
 * @return Std_ReturnType E_OK if successful
 */
extern Std_ReturnType Os_Port_StartOS(void);

/**
 * @brief Get core ID from port layer
 * @return uint32 Core ID
 */
extern uint32 Os_Port_GetCoreID(void);

/**
 * @brief Get current task from port layer
 * @return void* Current task handle
 */
extern void* Os_Port_GetCurrentTask(void);

/**
 * @brief Enter critical section
 */
extern void Os_Port_EnterCritical(void);

/**
 * @brief Exit critical section
 */
extern void Os_Port_ExitCritical(void);

#ifdef __cplusplus
}
#endif

#endif /* OS_H */

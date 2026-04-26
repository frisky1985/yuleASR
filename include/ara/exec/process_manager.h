/******************************************************************************
 * @file    process_manager.h
 * @brief   Process Manager Internal Header
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef EXEC_PROCESS_MANAGER_H
#define EXEC_PROCESS_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar_types.h"
#include "autosar_errors.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define EXEC_MAX_PROCESSES              32U
#define EXEC_MAX_ARGUMENTS              16U
#define EXEC_MAX_ENV_VARS               16U
#define EXEC_MAX_PATH_LENGTH            256U
#define EXEC_MAX_NAME_LENGTH            64U
#define EXEC_DEFAULT_STACK_SIZE         8192U
#define EXEC_DEFAULT_PRIORITY           5U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Process ID type */
typedef uint32_t ProcessIdType;

/* Process states */
typedef enum {
    PROCESS_STATE_IDLE = 0,
    PROCESS_STATE_STARTING,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_TERMINATING,
    PROCESS_STATE_TERMINATED,
    PROCESS_STATE_ERROR
} ProcessStateType;

/* Termination types */
typedef enum {
    TERMINATION_GRACEFUL = 0,
    TERMINATION_FORCED
} TerminationTypeType;

/* Process configuration */
typedef struct {
    char executable[EXEC_MAX_PATH_LENGTH];
    char arguments[EXEC_MAX_ARGUMENTS][EXEC_MAX_PATH_LENGTH];
    uint32_t argumentCount;
    char environment[EXEC_MAX_ENV_VARS][EXEC_MAX_PATH_LENGTH];
    uint32_t environmentCount;
    char workingDirectory[EXEC_MAX_PATH_LENGTH];
    bool autoRestart;
    uint32_t restartDelayMs;
    uint32_t startTimeoutMs;
    uint32_t stackSize;
    uint32_t priority;
} ProcessConfigType;

/* Process information */
typedef struct {
    ProcessIdType pid;
    ProcessStateType state;
    uint32_t startTime;
    uint32_t restartCount;
    uint32_t lastRestartTime;
    bool sandboxEnabled;
    void* platformHandle;
} ProcessInfoType;

/* Process table entry */
typedef struct {
    bool isUsed;
    ProcessConfigType config;
    ProcessInfoType info;
    void* healthCheckCallback;
    void* stateChangeCallback;
} ProcessEntryType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize process manager */
Std_ReturnType ProcessManager_Init(void);

/* Deinitialize process manager */
Std_ReturnType ProcessManager_Deinit(void);

/* Check if initialized */
bool ProcessManager_IsInitialized(void);

/* Create a new process configuration */
Std_ReturnType ProcessManager_CreateConfig(ProcessConfigType* config);

/* Start a process */
Std_ReturnType ProcessManager_Start(const ProcessConfigType* config, ProcessIdType* pid);

/* Stop a process */
Std_ReturnType ProcessManager_Stop(ProcessIdType pid, TerminationTypeType type);

/* Restart a process */
Std_ReturnType ProcessManager_Restart(ProcessIdType pid);

/* Kill a process immediately */
Std_ReturnType ProcessManager_Kill(ProcessIdType pid);

/* Get process state */
ProcessStateType ProcessManager_GetState(ProcessIdType pid);

/* Get process info */
Std_ReturnType ProcessManager_GetInfo(ProcessIdType pid, ProcessInfoType* info);

/* Check if process is running */
bool ProcessManager_IsRunning(ProcessIdType pid);

/* Get all process IDs */
uint32_t ProcessManager_GetAllProcesses(ProcessIdType* pids, uint32_t maxCount);

/* Register health check callback */
Std_ReturnType ProcessManager_RegisterHealthCheck(ProcessIdType pid, 
                                                   void (*callback)(ProcessIdType, uint8_t*));

/* Unregister health check callback */
Std_ReturnType ProcessManager_UnregisterHealthCheck(ProcessIdType pid);

/* Register state change callback */
Std_ReturnType ProcessManager_RegisterStateCallback(ProcessIdType pid,
                                                     void (*callback)(ProcessIdType, ProcessStateType));

/* Process manager main function (call periodically) */
void ProcessManager_MainFunction(void);

/* Get next available process ID */
ProcessIdType ProcessManager_GetNextPid(void);

#ifdef __cplusplus
}
#endif

#endif /* EXEC_PROCESS_MANAGER_H */

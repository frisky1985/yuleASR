/******************************************************************************
 * @file    process_manager.c
 * @brief   Process Manager Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/exec/process_manager.h"
#include <string.h>
#include <stdlib.h>

/******************************************************************************
 * Private Data
 ******************************************************************************/

static bool g_initialized = false;
static ProcessEntryType g_processTable[EXEC_MAX_PROCESSES];
static ProcessIdType g_nextPid = 1U;
static void (*g_globalStateCallback)(ProcessIdType, ProcessStateType) = NULL;

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static ProcessEntryType* FindProcessEntry(ProcessIdType pid) {
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (g_processTable[i].isUsed && g_processTable[i].info.pid == pid) {
            return &g_processTable[i];
        }
    }
    return NULL;
}

static ProcessEntryType* FindFreeEntry(void) {
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (!g_processTable[i].isUsed) {
            return &g_processTable[i];
        }
    }
    return NULL;
}

static void SetProcessState(ProcessEntryType* entry, ProcessStateType newState) {
    ProcessStateType oldState = entry->info.state;
    entry->info.state = newState;
    
    if (g_globalStateCallback != NULL) {
        g_globalStateCallback(entry->info.pid, newState);
    }
    
    if (entry->stateChangeCallback != NULL) {
        void (*callback)(ProcessIdType, ProcessStateType) = 
            (void (*)(ProcessIdType, ProcessStateType))entry->stateChangeCallback;
        callback(entry->info.pid, newState);
    }
}

static Std_ReturnType ValidateConfig(const ProcessConfigType* config) {
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    if (config->executable[0] == '\0') {
        return E_NOT_OK;
    }
    
    if (config->argumentCount > EXEC_MAX_ARGUMENTS) {
        return E_NOT_OK;
    }
    
    if (config->environmentCount > EXEC_MAX_ENV_VARS) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

static void CleanupProcess(ProcessEntryType* entry) {
    if (entry == NULL || !entry->isUsed) {
        return;
    }
    
    SetProcessState(entry, PROCESS_STATE_TERMINATED);
    
    /* Platform-specific cleanup would go here */
    entry->info.platformHandle = NULL;
    entry->isUsed = false;
}

static void* PlatformCreateProcess(const ProcessConfigType* config) {
    /* Platform-specific: FreeRTOS task creation or POSIX fork/exec */
    /* This is a stub - actual implementation depends on target platform */
    (void)config;
    return (void*)1U;  /* Return dummy handle */
}

static Std_ReturnType PlatformStartProcess(void* handle) {
    /* Platform-specific: Start the process/task */
    (void)handle;
    return E_OK;
}

static Std_ReturnType PlatformStopProcess(void* handle, TerminationTypeType type) {
    /* Platform-specific: Stop the process/task */
    (void)handle;
    (void)type;
    return E_OK;
}

static Std_ReturnType PlatformKillProcess(void* handle) {
    /* Platform-specific: Kill the process/task immediately */
    (void)handle;
    return E_OK;
}

static bool PlatformIsProcessRunning(void* handle) {
    /* Platform-specific: Check if process is running */
    (void)handle;
    return true;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Std_ReturnType ProcessManager_Init(void) {
    if (g_initialized) {
        return E_OK;
    }
    
    memset(g_processTable, 0, sizeof(g_processTable));
    g_nextPid = 1U;
    g_globalStateCallback = NULL;
    
    g_initialized = true;
    return E_OK;
}

Std_ReturnType ProcessManager_Deinit(void) {
    if (!g_initialized) {
        return E_OK;
    }
    
    /* Terminate all running processes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (g_processTable[i].isUsed) {
            ProcessManager_Stop(g_processTable[i].info.pid, TERMINATION_FORCED);
        }
    }
    
    memset(g_processTable, 0, sizeof(g_processTable));
    g_globalStateCallback = NULL;
    g_initialized = false;
    
    return E_OK;
}

bool ProcessManager_IsInitialized(void) {
    return g_initialized;
}

Std_ReturnType ProcessManager_CreateConfig(ProcessConfigType* config) {
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    memset(config, 0, sizeof(ProcessConfigType));
    config->startTimeoutMs = 5000U;
    config->restartDelayMs = 1000U;
    config->autoRestart = false;
    config->stackSize = EXEC_DEFAULT_STACK_SIZE;
    config->priority = EXEC_DEFAULT_PRIORITY;
    
    return E_OK;
}

Std_ReturnType ProcessManager_Start(const ProcessConfigType* config, ProcessIdType* pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    if (pid == NULL || config == NULL) {
        return E_NOT_OK;
    }
    
    if (ValidateConfig(config) != E_OK) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindFreeEntry();
    if (entry == NULL) {
        return E_NOT_OK;  /* No free slots */
    }
    
    /* Create platform-specific process */
    void* platformHandle = PlatformCreateProcess(config);
    if (platformHandle == NULL) {
        return E_NOT_OK;
    }
    
    /* Initialize entry */
    entry->isUsed = true;
    memcpy(&entry->config, config, sizeof(ProcessConfigType));
    
    entry->info.pid = g_nextPid++;
    entry->info.state = PROCESS_STATE_STARTING;
    entry->info.startTime = 0U;  /* Would use actual timestamp */
    entry->info.restartCount = 0U;
    entry->info.lastRestartTime = 0U;
    entry->info.sandboxEnabled = false;
    entry->info.platformHandle = platformHandle;
    entry->healthCheckCallback = NULL;
    entry->stateChangeCallback = NULL;
    
    *pid = entry->info.pid;
    
    /* Start the process */
    if (PlatformStartProcess(platformHandle) == E_OK) {
        SetProcessState(entry, PROCESS_STATE_RUNNING);
    } else {
        entry->isUsed = false;
        return E_NOT_OK;
    }
    
    return E_OK;
}

Std_ReturnType ProcessManager_Stop(ProcessIdType pid, TerminationTypeType type) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    if (entry->info.state == PROCESS_STATE_TERMINATED ||
        entry->info.state == PROCESS_STATE_TERMINATING) {
        return E_OK;
    }
    
    SetProcessState(entry, PROCESS_STATE_TERMINATING);
    
    if (PlatformStopProcess(entry->info.platformHandle, type) != E_OK) {
        return E_NOT_OK;
    }
    
    SetProcessState(entry, PROCESS_STATE_TERMINATED);
    entry->isUsed = false;
    
    return E_OK;
}

Std_ReturnType ProcessManager_Restart(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    /* Stop the process */
    if (ProcessManager_Stop(pid, TERMINATION_GRACEFUL) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Clean up the old entry */
    CleanupProcess(entry);
    
    /* Start a new process with the same configuration */
    ProcessIdType newPid;
    if (ProcessManager_Start(&entry->config, &newPid) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Update restart tracking */
    entry = FindProcessEntry(newPid);
    if (entry != NULL) {
        entry->info.restartCount++;
        entry->info.lastRestartTime = 0U;  /* Would use actual timestamp */
    }
    
    return E_OK;
}

Std_ReturnType ProcessManager_Kill(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    SetProcessState(entry, PROCESS_STATE_TERMINATING);
    
    if (PlatformKillProcess(entry->info.platformHandle) != E_OK) {
        return E_NOT_OK;
    }
    
    SetProcessState(entry, PROCESS_STATE_TERMINATED);
    entry->isUsed = false;
    
    return E_OK;
}

ProcessStateType ProcessManager_GetState(ProcessIdType pid) {
    if (!g_initialized) {
        return PROCESS_STATE_ERROR;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return PROCESS_STATE_ERROR;
    }
    
    return entry->info.state;
}

Std_ReturnType ProcessManager_GetInfo(ProcessIdType pid, ProcessInfoType* info) {
    if (!g_initialized || info == NULL) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(info, &entry->info, sizeof(ProcessInfoType));
    return E_OK;
}

bool ProcessManager_IsRunning(ProcessIdType pid) {
    if (!g_initialized) {
        return false;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return false;
    }
    
    return (entry->info.state == PROCESS_STATE_RUNNING) &&
           PlatformIsProcessRunning(entry->info.platformHandle);
}

uint32_t ProcessManager_GetAllProcesses(ProcessIdType* pids, uint32_t maxCount) {
    if (!g_initialized || pids == NULL || maxCount == 0U) {
        return 0U;
    }
    
    uint32_t count = 0U;
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES && count < maxCount; i++) {
        if (g_processTable[i].isUsed) {
            pids[count++] = g_processTable[i].info.pid;
        }
    }
    
    return count;
}

Std_ReturnType ProcessManager_RegisterHealthCheck(ProcessIdType pid,
                                                   void (*callback)(ProcessIdType, uint8_t*)) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    entry->healthCheckCallback = (void*)callback;
    return E_OK;
}

Std_ReturnType ProcessManager_UnregisterHealthCheck(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    entry->healthCheckCallback = NULL;
    return E_OK;
}

Std_ReturnType ProcessManager_RegisterStateCallback(ProcessIdType pid,
                                                     void (*callback)(ProcessIdType, ProcessStateType)) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ProcessEntryType* entry = FindProcessEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    entry->stateChangeCallback = (void*)callback;
    return E_OK;
}

void ProcessManager_MainFunction(void) {
    if (!g_initialized) {
        return;
    }
    
    /* Check health of all running processes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (!g_processTable[i].isUsed) {
            continue;
        }
        
        ProcessEntryType* entry = &g_processTable[i];
        
        if (entry->info.state == PROCESS_STATE_RUNNING) {
            /* Check if process is still running */
            if (!PlatformIsProcessRunning(entry->info.platformHandle)) {
                /* Process died unexpectedly */
                if (entry->config.autoRestart) {
                    ProcessManager_Restart(entry->info.pid);
                } else {
                    SetProcessState(entry, PROCESS_STATE_ERROR);
                    entry->isUsed = false;
                }
            } else if (entry->healthCheckCallback != NULL) {
                /* Run health check */
                void (*callback)(ProcessIdType, uint8_t*) = 
                    (void (*)(ProcessIdType, uint8_t*))entry->healthCheckCallback;
                uint8_t healthStatus = 0U;
                callback(entry->info.pid, &healthStatus);
                
                if (healthStatus != 0U) {
                    /* Health check failed */
                    if (entry->config.autoRestart) {
                        ProcessManager_Restart(entry->info.pid);
                    }
                }
            }
        }
    }
}

ProcessIdType ProcessManager_GetNextPid(void) {
    return g_nextPid;
}

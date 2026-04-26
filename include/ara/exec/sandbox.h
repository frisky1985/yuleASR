/******************************************************************************
 * @file    sandbox.h
 * @brief   Sandbox Manager Header
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef EXEC_SANDBOX_H
#define EXEC_SANDBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar_types.h"
#include "autosar_errors.h"
#include "process_manager.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define SANDBOX_MAX_MOUNT_POINTS        8U
#define SANDBOX_MAX_DEVICES             8U
#define SANDBOX_MAX_SYSCALL_FILTERS     64U
#define SANDBOX_MAX_PATH_LENGTH         256U
#define SANDBOX_DEFAULT_ROOT_PATH       "/var/sandbox"

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Sandbox isolation types */
typedef enum {
    SANDBOX_ISOLATION_NONE = 0,
    SANDBOX_ISOLATION_FILESYSTEM,
    SANDBOX_ISOLATION_NETWORK,
    SANDBOX_ISOLATION_PID,
    SANDBOX_ISOLATION_IPC,
    SANDBOX_ISOLATION_ALL
} SandboxIsolationType;

/* Mount point configuration */
typedef struct {
    char source[SANDBOX_MAX_PATH_LENGTH];
    char target[SANDBOX_MAX_PATH_LENGTH];
    bool readOnly;
    bool createIfMissing;
} SandboxMountPointType;

/* Device access configuration */
typedef struct {
    char device[SANDBOX_MAX_PATH_LENGTH];
    bool readAllowed;
    bool writeAllowed;
} SandboxDeviceAccessType;

/* System call filter */
typedef struct {
    uint32_t syscallNumber;
    bool allowed;
} SandboxSyscallFilterType;

/* Sandbox configuration */
typedef struct {
    char rootPath[SANDBOX_MAX_PATH_LENGTH];
    SandboxMountPointType mounts[SANDBOX_MAX_MOUNT_POINTS];
    uint32_t mountCount;
    SandboxDeviceAccessType devices[SANDBOX_MAX_DEVICES];
    uint32_t deviceCount;
    SandboxSyscallFilterType syscalls[SANDBOX_MAX_SYSCALL_FILTERS];
    uint32_t syscallCount;
    SandboxIsolationType isolationLevel;
    bool networkRestricted;
    bool capDropAll;
} SandboxConfigType;

/* Sandbox status */
typedef struct {
    ProcessIdType pid;
    bool isActive;
    SandboxIsolationType activeIsolation;
    char actualRootPath[SANDBOX_MAX_PATH_LENGTH];
    void* platformHandle;
} SandboxStatusType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize sandbox manager */
Std_ReturnType Sandbox_Init(void);

/* Deinitialize sandbox manager */
Std_ReturnType Sandbox_Deinit(void);

/* Check if initialized */
bool Sandbox_IsInitialized(void);

/* Create default sandbox configuration */
Std_ReturnType Sandbox_CreateDefaultConfig(SandboxConfigType* config);

/* Enable sandbox for a process */
Std_ReturnType Sandbox_Enable(ProcessIdType pid, const SandboxConfigType* config);

/* Disable sandbox for a process */
Std_ReturnType Sandbox_Disable(ProcessIdType pid);

/* Check if sandbox is active for a process */
bool Sandbox_IsActive(ProcessIdType pid);

/* Get sandbox status for a process */
Std_ReturnType Sandbox_GetStatus(ProcessIdType pid, SandboxStatusType* status);

/* Add mount point to sandbox configuration */
Std_ReturnType Sandbox_AddMountPoint(SandboxConfigType* config, 
                                      const char* source,
                                      const char* target,
                                      bool readOnly);

/* Add device access to sandbox configuration */
Std_ReturnType Sandbox_AddDeviceAccess(SandboxConfigType* config,
                                        const char* device,
                                        bool readAllowed,
                                        bool writeAllowed);

/* Add syscall filter to sandbox configuration */
Std_ReturnType Sandbox_AddSyscallFilter(SandboxConfigType* config,
                                         uint32_t syscallNumber,
                                         bool allowed);

/* Validate sandbox configuration */
bool Sandbox_ValidateConfig(const SandboxConfigType* config);

/* Apply filesystem isolation */
Std_ReturnType Sandbox_ApplyFilesystemIsolation(ProcessIdType pid, const SandboxConfigType* config);

/* Apply network isolation */
Std_ReturnType Sandbox_ApplyNetworkIsolation(ProcessIdType pid, bool restrict);

/* Apply syscall filtering */
Std_ReturnType Sandbox_ApplySyscallFiltering(ProcessIdType pid, const SandboxConfigType* config);

/* Sandbox main function (call periodically) */
void Sandbox_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* EXEC_SANDBOX_H */

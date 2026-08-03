/******************************************************************************
 * @file    sandbox.c
 * @brief   Sandbox Manager Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/exec/sandbox.h"
#include <string.h>
#include <stdlib.h>

/******************************************************************************
 * Private Data
 ******************************************************************************/

static bool g_initialized = false;
static SandboxStatusType g_sandboxTable[EXEC_MAX_PROCESSES];

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static SandboxStatusType* FindSandboxEntry(ProcessIdType pid) {
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (g_sandboxTable[i].pid == pid && g_sandboxTable[i].isActive) {
            return &g_sandboxTable[i];
        }
    }
    return NULL;
}

static SandboxStatusType* FindFreeEntry(void) {
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (!g_sandboxTable[i].isActive) {
            return &g_sandboxTable[i];
        }
    }
    return NULL;
}

static bool ValidatePath(const char* path) {
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    if (strlen(path) >= SANDBOX_MAX_PATH_LENGTH) {
        return false;
    }
    return true;
}

static Std_ReturnType PlatformApplyChroot(const char* rootPath) {
    /* Platform-specific: Apply chroot */
    (void)rootPath;
    return E_OK;
}

static Std_ReturnType PlatformApplyNamespace(SandboxIsolationType isolation) {
    /* Platform-specific: Apply namespace isolation */
    (void)isolation;
    return E_OK;
}

static Std_ReturnType PlatformApplyNetworkRestriction(bool restrict) {
    /* Platform-specific: Apply network restrictions */
    (void)restrict;
    return E_OK;
}

static Std_ReturnType PlatformApplySeccomp(const SandboxSyscallFilterType* filters, 
                                           uint32_t count) {
    /* Platform-specific: Apply seccomp filters */
    (void)filters;
    (void)count;
    return E_OK;
}

static Std_ReturnType PlatformCreateMount(const char* source, const char* target,
                                           bool readOnly) {
    /* Platform-specific: Create mount point */
    (void)source;
    (void)target;
    (void)readOnly;
    return E_OK;
}

static Std_ReturnType PlatformRemoveSandbox(ProcessIdType pid) {
    /* Platform-specific: Remove sandbox */
    (void)pid;
    return E_OK;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Std_ReturnType Sandbox_Init(void) {
    if (g_initialized) {
        return E_OK;
    }
    
    memset(g_sandboxTable, 0, sizeof(g_sandboxTable));
    
    g_initialized = true;
    return E_OK;
}

Std_ReturnType Sandbox_Deinit(void) {
    if (!g_initialized) {
        return E_OK;
    }
    
    /* Disable all active sandboxes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (g_sandboxTable[i].isActive) {
            Sandbox_Disable(g_sandboxTable[i].pid);
        }
    }
    
    memset(g_sandboxTable, 0, sizeof(g_sandboxTable));
    g_initialized = false;
    
    return E_OK;
}

bool Sandbox_IsInitialized(void) {
    return g_initialized;
}

Std_ReturnType Sandbox_CreateDefaultConfig(SandboxConfigType* config) {
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    memset(config, 0, sizeof(SandboxConfigType));
    
    strncpy(config->rootPath, SANDBOX_DEFAULT_ROOT_PATH, 
            SANDBOX_MAX_PATH_LENGTH - 1U);
    config->rootPath[SANDBOX_MAX_PATH_LENGTH - 1U] = '\0';
    
    config->isolationLevel = SANDBOX_ISOLATION_FILESYSTEM;
    config->networkRestricted = true;
    config->capDropAll = true;
    config->mountCount = 0U;
    config->deviceCount = 0U;
    config->syscallCount = 0U;
    
    /* Add default read-only mounts */
    Sandbox_AddMountPoint(config, "/bin", "/bin", true);
    Sandbox_AddMountPoint(config, "/lib", "/lib", true);
    Sandbox_AddMountPoint(config, "/lib64", "/lib64", true);
    Sandbox_AddMountPoint(config, "/usr", "/usr", true);
    
    /* Add writable tmp */
    Sandbox_AddMountPoint(config, "tmpfs", "/tmp", false);
    
    return E_OK;
}

Std_ReturnType Sandbox_Enable(ProcessIdType pid, const SandboxConfigType* config) {
    if (!g_initialized || config == NULL) {
        return E_NOT_OK;
    }
    
    if (!Sandbox_ValidateConfig(config)) {
        return E_NOT_OK;
    }
    
    /* Check if sandbox already active for this process */
    if (FindSandboxEntry(pid) != NULL) {
        return E_NOT_OK;
    }
    
    SandboxStatusType* entry = FindFreeEntry();
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    /* Initialize entry */
    entry->pid = pid;
    entry->isActive = true;
    entry->activeIsolation = config->isolationLevel;
    strncpy(entry->actualRootPath, config->rootPath, 
            SANDBOX_MAX_PATH_LENGTH - 1U);
    entry->actualRootPath[SANDBOX_MAX_PATH_LENGTH - 1U] = '\0';
    
    /* Apply filesystem isolation */
    if (config->isolationLevel == SANDBOX_ISOLATION_FILESYSTEM ||
        config->isolationLevel == SANDBOX_ISOLATION_ALL) {
        if (Sandbox_ApplyFilesystemIsolation(pid, config) != E_OK) {
            entry->isActive = false;
            return E_NOT_OK;
        }
    }
    
    /* Apply network isolation */
    if (config->networkRestricted || config->isolationLevel == SANDBOX_ISOLATION_ALL) {
        if (Sandbox_ApplyNetworkIsolation(pid, config->networkRestricted) != E_OK) {
            entry->isActive = false;
            return E_NOT_OK;
        }
    }
    
    /* Apply syscall filtering */
    if (config->syscallCount > 0U) {
        if (Sandbox_ApplySyscallFiltering(pid, config) != E_OK) {
            entry->isActive = false;
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

Std_ReturnType Sandbox_Disable(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    SandboxStatusType* entry = FindSandboxEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    /* Platform-specific cleanup */
    PlatformRemoveSandbox(pid);
    
    entry->isActive = false;
    entry->pid = 0U;
    
    return E_OK;
}

bool Sandbox_IsActive(ProcessIdType pid) {
    if (!g_initialized) {
        return false;
    }
    
    return FindSandboxEntry(pid) != NULL;
}

Std_ReturnType Sandbox_GetStatus(ProcessIdType pid, SandboxStatusType* status) {
    if (!g_initialized || status == NULL) {
        return E_NOT_OK;
    }
    
    SandboxStatusType* entry = FindSandboxEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(status, entry, sizeof(SandboxStatusType));
    return E_OK;
}

Std_ReturnType Sandbox_AddMountPoint(SandboxConfigType* config,
                                      const char* source,
                                      const char* target,
                                      bool readOnly) {
    if (config == NULL || source == NULL || target == NULL) {
        return E_NOT_OK;
    }
    
    if (!ValidatePath(source) || !ValidatePath(target)) {
        return E_NOT_OK;
    }
    
    if (config->mountCount >= SANDBOX_MAX_MOUNT_POINTS) {
        return E_NOT_OK;
    }
    
    SandboxMountPointType* mount = &config->mounts[config->mountCount];
    strncpy(mount->source, source, SANDBOX_MAX_PATH_LENGTH - 1U);
    mount->source[SANDBOX_MAX_PATH_LENGTH - 1U] = '\0';
    strncpy(mount->target, target, SANDBOX_MAX_PATH_LENGTH - 1U);
    mount->target[SANDBOX_MAX_PATH_LENGTH - 1U] = '\0';
    mount->readOnly = readOnly;
    mount->createIfMissing = true;
    
    config->mountCount++;
    
    return E_OK;
}

Std_ReturnType Sandbox_AddDeviceAccess(SandboxConfigType* config,
                                        const char* device,
                                        bool readAllowed,
                                        bool writeAllowed) {
    if (config == NULL || device == NULL) {
        return E_NOT_OK;
    }
    
    if (!ValidatePath(device)) {
        return E_NOT_OK;
    }
    
    if (config->deviceCount >= SANDBOX_MAX_DEVICES) {
        return E_NOT_OK;
    }
    
    SandboxDeviceAccessType* dev = &config->devices[config->deviceCount];
    strncpy(dev->device, device, SANDBOX_MAX_PATH_LENGTH - 1U);
    dev->device[SANDBOX_MAX_PATH_LENGTH - 1U] = '\0';
    dev->readAllowed = readAllowed;
    dev->writeAllowed = writeAllowed;
    
    config->deviceCount++;
    
    return E_OK;
}

Std_ReturnType Sandbox_AddSyscallFilter(SandboxConfigType* config,
                                         uint32_t syscallNumber,
                                         bool allowed) {
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    if (config->syscallCount >= SANDBOX_MAX_SYSCALL_FILTERS) {
        return E_NOT_OK;
    }
    
    SandboxSyscallFilterType* filter = &config->syscalls[config->syscallCount];
    filter->syscallNumber = syscallNumber;
    filter->allowed = allowed;
    
    config->syscallCount++;
    
    return E_OK;
}

bool Sandbox_ValidateConfig(const SandboxConfigType* config) {
    if (config == NULL) {
        return false;
    }
    
    if (!ValidatePath(config->rootPath)) {
        return false;
    }
    
    if (config->mountCount > SANDBOX_MAX_MOUNT_POINTS) {
        return false;
    }
    
    if (config->deviceCount > SANDBOX_MAX_DEVICES) {
        return false;
    }
    
    if (config->syscallCount > SANDBOX_MAX_SYSCALL_FILTERS) {
        return false;
    }
    
    /* Validate all mount points */
    for (uint32_t i = 0U; i < config->mountCount; i++) {
        if (!ValidatePath(config->mounts[i].source) ||
            !ValidatePath(config->mounts[i].target)) {
            return false;
        }
    }
    
    /* Validate all device paths */
    for (uint32_t i = 0U; i < config->deviceCount; i++) {
        if (!ValidatePath(config->devices[i].device)) {
            return false;
        }
    }
    
    return true;
}

Std_ReturnType Sandbox_ApplyFilesystemIsolation(ProcessIdType pid, 
                                                 const SandboxConfigType* config) {
    (void)pid;
    
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    /* Apply chroot */
    if (PlatformApplyChroot(config->rootPath) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Apply namespace isolation */
    if (PlatformApplyNamespace(config->isolationLevel) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Create mount points */
    for (uint32_t i = 0U; i < config->mountCount; i++) {
        if (PlatformCreateMount(config->mounts[i].source,
                                 config->mounts[i].target,
                                 config->mounts[i].readOnly) != E_OK) {
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

Std_ReturnType Sandbox_ApplyNetworkIsolation(ProcessIdType pid, bool restrict) {
    (void)pid;
    return PlatformApplyNetworkRestriction(restrict);
}

Std_ReturnType Sandbox_ApplySyscallFiltering(ProcessIdType pid, 
                                              const SandboxConfigType* config) {
    (void)pid;
    
    if (config == NULL || config->syscallCount == 0U) {
        return E_OK;
    }
    
    return PlatformApplySeccomp(config->syscalls, config->syscallCount);
}

void Sandbox_MainFunction(void) {
    if (!g_initialized) {
        return;
    }
    
    /* Monitor active sandboxes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (!g_sandboxTable[i].isActive) {
            continue;
        }
        
        /* Could check process health, resource usage, etc. */
        /* Could also verify sandbox integrity */
    }
}

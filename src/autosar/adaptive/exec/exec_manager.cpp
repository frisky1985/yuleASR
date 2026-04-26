/******************************************************************************
 * @file    exec_manager.cpp
 * @brief   ara::exec Execution Manager C++ Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/exec/exec_manager.h"
#include "ara/exec/process_manager.h"
#include "ara/exec/resource_manager.h"
#include "ara/exec/sandbox.h"
#include <cstring>
#include <algorithm>

namespace ara {
namespace exec {

/******************************************************************************
 * ExecManager Implementation
 ******************************************************************************/

class ExecManager::Impl {
public:
    Impl() : initialized_(false) {}
    
    bool initialized_;
    StateChangeCallback stateCallback_;
};

ExecManager::ExecManager() : pImpl_(new Impl()) {}

ExecManager::~ExecManager() = default;

ExecManager& ExecManager::GetInstance() {
    static ExecManager instance;
    return instance;
}

Result<void> ExecManager::Initialize() {
    if (pImpl_->initialized_) {
        return Result<void>();
    }
    
    /* Initialize sub-modules */
    if (ProcessManager_Init() != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    if (ResourceManager_Init() != E_OK) {
        ProcessManager_Deinit();
        return Result<void>(E_NOT_OK);
    }
    
    if (Sandbox_Init() != E_OK) {
        ResourceManager_Deinit();
        ProcessManager_Deinit();
        return Result<void>(E_NOT_OK);
    }
    
    pImpl_->initialized_ = true;
    return Result<void>();
}

Result<void> ExecManager::Deinitialize() {
    if (!pImpl_->initialized_) {
        return Result<void>();
    }
    
    Sandbox_Deinit();
    ResourceManager_Deinit();
    ProcessManager_Deinit();
    
    pImpl_->initialized_ = false;
    return Result<void>();
}

bool ExecManager::IsInitialized() const {
    return pImpl_->initialized_;
}

Result<ProcessId> ExecManager::StartProcess(const ExecutionConfig& config) {
    if (!pImpl_->initialized_) {
        return Result<ProcessId>(E_NOT_OK);
    }
    
    /* Convert C++ config to C config */
    ProcessConfigType cConfig;
    ProcessManager_CreateConfig(&cConfig);
    
    std::strncpy(cConfig.executable, config.executable.c_str(), 
                 EXEC_MAX_PATH_LENGTH - 1U);
    cConfig.executable[EXEC_MAX_PATH_LENGTH - 1U] = '\0';
    
    cConfig.argumentCount = std::min(static_cast<uint32_t>(config.arguments.size()),
                                     EXEC_MAX_ARGUMENTS);
    for (uint32_t i = 0U; i < cConfig.argumentCount; i++) {
        std::strncpy(cConfig.arguments[i], config.arguments[i].c_str(),
                     EXEC_MAX_PATH_LENGTH - 1U);
        cConfig.arguments[i][EXEC_MAX_PATH_LENGTH - 1U] = '\0';
    }
    
    cConfig.environmentCount = std::min(static_cast<uint32_t>(config.environment.size()),
                                        EXEC_MAX_ENV_VARS);
    for (uint32_t i = 0U; i < cConfig.environmentCount; i++) {
        std::strncpy(cConfig.environment[i], config.environment[i].c_str(),
                     EXEC_MAX_PATH_LENGTH - 1U);
        cConfig.environment[i][EXEC_MAX_PATH_LENGTH - 1U] = '\0';
    }
    
    std::strncpy(cConfig.workingDirectory, config.workingDirectory.c_str(),
                 EXEC_MAX_PATH_LENGTH - 1U);
    cConfig.workingDirectory[EXEC_MAX_PATH_LENGTH - 1U] = '\0';
    
    cConfig.autoRestart = config.autoRestart;
    cConfig.restartDelayMs = config.restartDelayMs;
    cConfig.startTimeoutMs = config.startTimeoutMs;
    
    /* Start the process */
    ProcessIdType pid;
    if (ProcessManager_Start(&cConfig, &pid) != E_OK) {
        return Result<ProcessId>(E_NOT_OK);
    }
    
    /* Enable resource monitoring */
    ResourceManager_EnableMonitoring(pid);
    
    return Result<ProcessId>(pid);
}

Result<void> ExecManager::StopProcess(ProcessId pid, TerminationType type) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    TerminationTypeType cType = (type == TerminationType::kGraceful) 
                                 ? TERMINATION_GRACEFUL 
                                 : TERMINATION_FORCED;
    
    if (ProcessManager_Stop(pid, cType) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

Result<void> ExecManager::RestartProcess(ProcessId pid) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    if (ProcessManager_Restart(pid) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

Result<void> ExecManager::KillProcess(ProcessId pid) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    if (ProcessManager_Kill(pid) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

ProcessState ExecManager::GetProcessState(ProcessId pid) const {
    if (!pImpl_->initialized_) {
        return ProcessState::kError;
    }
    
    ProcessStateType cState = ProcessManager_GetState(pid);
    
    switch (cState) {
        case PROCESS_STATE_IDLE:
            return ProcessState::kIdle;
        case PROCESS_STATE_STARTING:
            return ProcessState::kStarting;
        case PROCESS_STATE_RUNNING:
            return ProcessState::kRunning;
        case PROCESS_STATE_TERMINATING:
            return ProcessState::kTerminating;
        case PROCESS_STATE_TERMINATED:
            return ProcessState::kTerminated;
        case PROCESS_STATE_ERROR:
        default:
            return ProcessState::kError;
    }
}

bool ExecManager::IsProcessRunning(ProcessId pid) const {
    if (!pImpl_->initialized_) {
        return false;
    }
    
    return ProcessManager_IsRunning(pid);
}

Result<void> ExecManager::SetResourceLimit(ProcessId pid, const ResourceLimit& limit) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    ResourceLimitType cLimit;
    cLimit.type = static_cast<ResourceTypeType>(limit.type);
    cLimit.softLimit = limit.softLimit;
    cLimit.hardLimit = limit.hardLimit;
    cLimit.enabled = true;
    
    if (ResourceManager_SetLimit(pid, &cLimit) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

Result<ResourceUsage> ExecManager::GetResourceUsage(ProcessId pid) const {
    if (!pImpl_->initialized_) {
        return Result<ResourceUsage>(E_NOT_OK);
    }
    
    ResourceUsageType cUsage;
    if (ResourceManager_GetUsage(pid, &cUsage) != E_OK) {
        return Result<ResourceUsage>(E_NOT_OK);
    }
    
    ResourceUsage usage;
    usage.cpuTimeUs = cUsage.cpuTimeUs;
    usage.memoryBytes = cUsage.memoryBytes;
    usage.fileDescriptors = cUsage.fileDescriptors;
    usage.diskReadBytes = cUsage.diskReadBytes;
    usage.diskWriteBytes = cUsage.diskWriteBytes;
    usage.networkRxBytes = cUsage.networkRxBytes;
    usage.networkTxBytes = cUsage.networkTxBytes;
    
    return Result<ResourceUsage>(usage);
}

Result<void> ExecManager::RegisterHealthCheck(ProcessId pid, HealthCheckCallback callback) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    /* Store callback - would need proper storage for production */
    (void)pid;
    (void)callback;
    
    return Result<void>();
}

Result<void> ExecManager::UnregisterHealthCheck(ProcessId pid) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    if (ProcessManager_UnregisterHealthCheck(pid) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

Result<HealthStatus> ExecManager::CheckHealth(ProcessId pid) {
    if (!pImpl_->initialized_) {
        return Result<HealthStatus>(E_NOT_OK);
    }
    
    /* Check if process is running */
    if (!ProcessManager_IsRunning(pid)) {
        return Result<HealthStatus>(HealthStatus::kUnhealthy);
    }
    
    /* Check resource limits */
    if (!ResourceManager_IsWithinLimits(pid)) {
        return Result<HealthStatus>(HealthStatus::kDegraded);
    }
    
    return Result<HealthStatus>(HealthStatus::kHealthy);
}

void ExecManager::RegisterStateChangeCallback(StateChangeCallback callback) {
    pImpl_->stateCallback_ = callback;
}

void ExecManager::UnregisterStateChangeCallback() {
    pImpl_->stateCallback_ = nullptr;
}

Result<void> ExecManager::EnableSandbox(ProcessId pid) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    if (Sandbox_Enable(pid, &config) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

Result<void> ExecManager::DisableSandbox(ProcessId pid) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    if (Sandbox_Disable(pid) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

} // namespace exec
} // namespace ara

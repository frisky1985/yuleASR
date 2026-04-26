/******************************************************************************
 * @file    exec_manager.h
 * @brief   ara::exec Execution Manager C++ Interface
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ARA_EXEC_EXEC_MANAGER_H
#define ARA_EXEC_EXEC_MANAGER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace ara {
namespace exec {

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Process ID type */
using ProcessId = uint32_t;

/* Invalid process ID */
constexpr ProcessId INVALID_PROCESS_ID = 0U;

/* Process states */
enum class ProcessState : uint8_t {
    kIdle = 0,
    kStarting,
    kRunning,
    kTerminating,
    kTerminated,
    kError
};

/* Termination types */
enum class TerminationType : uint8_t {
    kGraceful = 0,
    kForced
};

/* Resource types */
enum class ResourceType : uint8_t {
    kCpu = 0,
    kMemory,
    kFileDescriptor,
    kDiskIO,
    kNetworkIO
};

/* Execution configuration */
struct ExecutionConfig {
    std::string executable;
    std::vector<std::string> arguments;
    std::vector<std::string> environment;
    std::string workingDirectory;
    bool autoRestart;
    uint32_t restartDelayMs;
    uint32_t startTimeoutMs;
    
    ExecutionConfig() 
        : autoRestart(false)
        , restartDelayMs(1000U)
        , startTimeoutMs(5000U) {}
};

/* Resource limits */
struct ResourceLimit {
    ResourceType type;
    uint64_t softLimit;
    uint64_t hardLimit;
    
    ResourceLimit() 
        : type(ResourceType::kCpu)
        , softLimit(0U)
        , hardLimit(0U) {}
};

/* Resource usage */
struct ResourceUsage {
    uint64_t cpuTimeUs;
    uint64_t memoryBytes;
    uint32_t fileDescriptors;
    uint64_t diskReadBytes;
    uint64_t diskWriteBytes;
    uint64_t networkRxBytes;
    uint64_t networkTxBytes;
    
    ResourceUsage()
        : cpuTimeUs(0U)
        , memoryBytes(0U)
        , fileDescriptors(0U)
        , diskReadBytes(0U)
        , diskWriteBytes(0U)
        , networkRxBytes(0U)
        , networkTxBytes(0U) {}
};

/* Health check result */
enum class HealthStatus : uint8_t {
    kHealthy = 0,
    kDegraded,
    kUnhealthy
};

/* Health check callback */
using HealthCheckCallback = std::function<HealthStatus()>;

/* State change callback */
using StateChangeCallback = std::function<void(ProcessId, ProcessState)>;

/* Result type */
template<typename T>
class Result {
public:
    Result() : hasValue_(false), error_(0) {}
    explicit Result(T value) : hasValue_(true), value_(std::move(value)), error_(0) {}
    explicit Result(int32_t error) : hasValue_(false), error_(error) {}
    
    bool HasValue() const { return hasValue_; }
    const T& Value() const { return value_; }
    int32_t Error() const { return error_; }
    bool operator bool() const { return hasValue_; }
    
private:
    bool hasValue_;
    T value_;
    int32_t error_;
};

template<>
class Result<void> {
public:
    Result() : error_(0) {}
    explicit Result(int32_t error) : error_(error) {}
    
    bool HasValue() const { return error_ == 0; }
    int32_t Error() const { return error_; }
    bool operator bool() const { return error_ == 0; }
    
private:
    int32_t error_;
};

/******************************************************************************
 * Class Definition
 ******************************************************************************/

class ExecManager {
public:
    /* Singleton accessor */
    static ExecManager& GetInstance();
    
    /* Initialize the execution manager */
    Result<void> Initialize();
    
    /* Deinitialize the execution manager */
    Result<void> Deinitialize();
    
    /* Check if initialized */
    bool IsInitialized() const;
    
    /* Process Management */
    Result<ProcessId> StartProcess(const ExecutionConfig& config);
    Result<void> StopProcess(ProcessId pid, TerminationType type);
    Result<void> RestartProcess(ProcessId pid);
    Result<void> KillProcess(ProcessId pid);
    
    /* Process State */
    ProcessState GetProcessState(ProcessId pid) const;
    bool IsProcessRunning(ProcessId pid) const;
    
    /* Resource Management */
    Result<void> SetResourceLimit(ProcessId pid, const ResourceLimit& limit);
    Result<ResourceUsage> GetResourceUsage(ProcessId pid) const;
    
    /* Health Monitoring */
    Result<void> RegisterHealthCheck(ProcessId pid, HealthCheckCallback callback);
    Result<void> UnregisterHealthCheck(ProcessId pid);
    Result<HealthStatus> CheckHealth(ProcessId pid);
    
    /* Callback Registration */
    void RegisterStateChangeCallback(StateChangeCallback callback);
    void UnregisterStateChangeCallback();
    
    /* Sandbox Configuration */
    Result<void> EnableSandbox(ProcessId pid);
    Result<void> DisableSandbox(ProcessId pid);
    
private:
    ExecManager();
    ~ExecManager();
    ExecManager(const ExecManager&) = delete;
    ExecManager& operator=(const ExecManager&) = delete;
    
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace exec
} // namespace ara

#endif /* ARA_EXEC_EXEC_MANAGER_H */

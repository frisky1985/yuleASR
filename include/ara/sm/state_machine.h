/******************************************************************************
 * @file    state_machine.h
 * @brief   ara::sm State Manager C++ Interface
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ARA_SM_STATE_MACHINE_H
#define ARA_SM_STATE_MACHINE_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace ara {
namespace sm {

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Machine states */
enum class MachineState : uint8_t {
    kOff = 0,
    kStartup,
    kDriving,
    kParking,
    kShutdown,
    kError
};

/* Function group names */
enum class FunctionGroupName : uint8_t {
    kPowerMode = 0,
    kCommunication,
    kDiagnostics,
    kLogging,
    kSafety,
    kUpdate,
    kCount
};

/* Function group states */
enum class FunctionGroupState : uint8_t {
    kOff = 0,
    kStarting,
    kOn,
    kStopping
};

/* State request result */
enum class StateRequestResult : uint8_t {
    kAccepted = 0,
    kRejected,
    kTimeout,
    kInProgress
};

/* State change callback */
using StateChangeCallback = std::function<void(MachineState, MachineState)>;  /* old, new */

/* Function group state callback */
using FGStateCallback = std::function<void(FunctionGroupName, FunctionGroupState)>;

/* Request confirmation callback */
using RequestConfirmationCallback = std::function<void(StateRequestResult)>;

/* Result type template (same as in exec) */
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

class StateManager {
public:
    /* Singleton accessor */
    static StateManager& GetInstance();
    
    /* Initialize the state manager */
    Result<void> Initialize();
    
    /* Deinitialize the state manager */
    Result<void> Deinitialize();
    
    /* Check if initialized */
    bool IsInitialized() const;
    
    /* State Machine */
    Result<StateRequestResult> RequestStateTransition(MachineState targetState);
    MachineState GetCurrentState() const;
    MachineState GetPreviousState() const;
    
    /* Check if state transition is allowed */
    bool IsStateTransitionAllowed(MachineState fromState, MachineState toState) const;
    
    /* Function Group Management */
    Result<void> SetFunctionGroupState(FunctionGroupName fg, FunctionGroupState state);
    FunctionGroupState GetFunctionGroupState(FunctionGroupName fg) const;
    
    /* Request protocol with callback */
    Result<void> RequestStateTransitionAsync(MachineState targetState,
                                              RequestConfirmationCallback callback);
    
    /* Cancel pending state request */
    Result<void> CancelStateRequest();
    
    /* Callback Registration */
    void RegisterStateChangeCallback(StateChangeCallback callback);
    void UnregisterStateChangeCallback();
    
    void RegisterFGStateCallback(FunctionGroupName fg, FGStateCallback callback);
    void UnregisterFGStateCallback(FunctionGroupName fg);
    
    /* Get function groups in a state */
    std::vector<FunctionGroupName> GetFunctionGroupsInState(FunctionGroupState state) const;
    
    /* Wait for state transition to complete */
    Result<StateRequestResult> WaitForStateTransition(std::chrono::milliseconds timeout);
    
private:
    StateManager();
    ~StateManager();
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;
    
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace sm
} // namespace ara

#endif /* ARA_SM_STATE_MACHINE_H */

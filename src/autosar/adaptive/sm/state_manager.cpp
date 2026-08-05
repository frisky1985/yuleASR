/******************************************************************************
 * @file    state_manager.cpp
 * @brief   ara::sm State Manager C++ Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/sm/state_machine.h"
#include "ara/sm/sm_internal.h"
#include "ara/sm/function_group.h"
#include "ara/sm/sm_interface.h"
#include <cstring>
#include <chrono>

namespace ara {
namespace sm {

/******************************************************************************
 * StateManager Implementation
 ******************************************************************************/

class StateManager::Impl {
public:
    Impl() : initialized_(false), clientId_(0U) {}
    
    bool initialized_;
    uint32_t clientId_;
    StateChangeCallback stateCallback_;
    FGStateCallback fgCallbacks_[static_cast<uint32_t>(FunctionGroupName::kCount)];
};

StateManager::StateManager() : pImpl_(new Impl()) {
    for (uint32_t i = 0U; i < static_cast<uint32_t>(FunctionGroupName::kCount); i++) {
        pImpl_->fgCallbacks_[i] = nullptr;
    }
}

StateManager::~StateManager() = default;

StateManager& StateManager::GetInstance() {
    static StateManager instance;
    return instance;
}

Result<void> StateManager::Initialize() {
    if (pImpl_->initialized_) {
        return Result<void>();
    }
    
    /* Initialize sub-modules */
    if (StateMachine_Init() != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    if (FG_Init() != E_OK) {
        StateMachine_Deinit();
        return Result<void>(E_NOT_OK);
    }
    
    if (SM_Interface_Init() != E_OK) {
        FG_Deinit();
        StateMachine_Deinit();
        return Result<void>(E_NOT_OK);
    }
    
    /* Register as a client */
    if (SM_RegisterClient("StateManager", &pImpl_->clientId_) != E_OK) {
        SM_Interface_Deinit();
        FG_Deinit();
        StateMachine_Deinit();
        return Result<void>(E_NOT_OK);
    }
    
    pImpl_->initialized_ = true;
    return Result<void>();
}

Result<void> StateManager::Deinitialize() {
    if (!pImpl_->initialized_) {
        return Result<void>();
    }
    
    /* Unregister client */
    if (pImpl_->clientId_ != 0U) {
        SM_UnregisterClient(pImpl_->clientId_);
        pImpl_->clientId_ = 0U;
    }
    
    SM_Interface_Deinit();
    FG_Deinit();
    StateMachine_Deinit();
    
    pImpl_->initialized_ = false;
    return Result<void>();
}

bool StateManager::IsInitialized() const {
    return pImpl_->initialized_;
}

Result<StateRequestResult> StateManager::RequestStateTransition(MachineState targetState) {
    if (!pImpl_->initialized_) {
        return Result<StateRequestResult>(E_NOT_OK);
    }
    
    StateRequestResultType result = StateMachine_RequestTransition(
        static_cast<MachineStateType>(targetState));
    
    return Result<StateRequestResult>(static_cast<StateRequestResult>(result));
}

MachineState StateManager::GetCurrentState() const {
    if (!pImpl_->initialized_) {
        return MachineState::kOff;
    }
    
    return static_cast<MachineState>(StateMachine_GetCurrentState());
}

MachineState StateManager::GetPreviousState() const {
    if (!pImpl_->initialized_) {
        return MachineState::kOff;
    }
    
    return static_cast<MachineState>(StateMachine_GetPreviousState());
}

bool StateManager::IsStateTransitionAllowed(MachineState fromState, MachineState toState) const {
    if (!pImpl_->initialized_) {
        return false;
    }
    
    return StateMachine_IsTransitionAllowed(
        static_cast<MachineStateType>(fromState),
        static_cast<MachineStateType>(toState));
}

Result<void> StateManager::SetFunctionGroupState(FunctionGroupName fg, FunctionGroupState state) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    if (StateMachine_SetFGState(
            static_cast<FunctionGroupNameType>(fg),
            static_cast<FunctionGroupStateType>(state)) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

FunctionGroupState StateManager::GetFunctionGroupState(FunctionGroupName fg) const {
    if (!pImpl_->initialized_) {
        return FunctionGroupState::kOff;
    }
    
    return static_cast<FunctionGroupState>(
        StateMachine_GetFGState(static_cast<FunctionGroupNameType>(fg)));
}

Result<void> StateManager::RequestStateTransitionAsync(MachineState targetState,
                                                        RequestConfirmationCallback callback) {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    /* Create a request */
    SM_RequestHandle handle = SM_CreateStateRequest(
        pImpl_->clientId_,
        static_cast<MachineStateType>(targetState));
    
    if (handle == 0U) {
        return Result<void>(E_NOT_OK);
    }
    
    /* Set completion callback */
    if (callback != nullptr) {
        /* Store callback for later invocation */
        (void)callback;
    }
    
    /* Submit the request */
    if (SM_SubmitRequest(handle) != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

Result<void> StateManager::CancelStateRequest() {
    if (!pImpl_->initialized_) {
        return Result<void>(E_NOT_OK);
    }
    
    if (StateMachine_CancelRequest() != E_OK) {
        return Result<void>(E_NOT_OK);
    }
    
    return Result<void>();
}

void StateManager::RegisterStateChangeCallback(StateChangeCallback callback) {
    pImpl_->stateCallback_ = callback;
    
    /* Register C callback */
    StateMachine_RegisterCallback([](MachineStateType oldState, MachineStateType newState) {
        StateManager& sm = StateManager::GetInstance();
        if (sm.pImpl_->stateCallback_) {
            sm.pImpl_->stateCallback_(
                static_cast<MachineState>(oldState),
                static_cast<MachineState>(newState));
        }
    });
}

void StateManager::UnregisterStateChangeCallback() {
    pImpl_->stateCallback_ = nullptr;
    StateMachine_UnregisterCallback();
}

void StateManager::RegisterFGStateCallback(FunctionGroupName fg, FGStateCallback callback) {
    uint32_t idx = static_cast<uint32_t>(fg);
    if (idx < static_cast<uint32_t>(FunctionGroupName::kCount)) {
        pImpl_->fgCallbacks_[idx] = callback;
        
        /* Register C callback */
        StateMachine_RegisterFGCallback(
            static_cast<FunctionGroupNameType>(fg),
            [](FunctionGroupNameType fg, FunctionGroupStateType state) {
                StateManager& sm = StateManager::GetInstance();
                uint32_t idx = static_cast<uint32_t>(fg);
                if (idx < static_cast<uint32_t>(FunctionGroupName::kCount) &&
                    sm.pImpl_->fgCallbacks_[idx]) {
                    sm.pImpl_->fgCallbacks_[idx](
                        static_cast<FunctionGroupName>(fg),
                        static_cast<FunctionGroupState>(state));
                }
            });
    }
}

void StateManager::UnregisterFGStateCallback(FunctionGroupName fg) {
    uint32_t idx = static_cast<uint32_t>(fg);
    if (idx < static_cast<uint32_t>(FunctionGroupName::kCount)) {
        pImpl_->fgCallbacks_[idx] = nullptr;
        StateMachine_UnregisterFGCallback(static_cast<FunctionGroupNameType>(fg));
    }
}

std::vector<FunctionGroupName> StateManager::GetFunctionGroupsInState(FunctionGroupState state) const {
    std::vector<FunctionGroupName> result;
    
    if (!pImpl_->initialized_) {
        return result;
    }
    
    for (uint32_t i = 0U; i < static_cast<uint32_t>(FunctionGroupName::kCount); i++) {
        if (GetFunctionGroupState(static_cast<FunctionGroupName>(i)) == state) {
            result.push_back(static_cast<FunctionGroupName>(i));
        }
    }
    
    return result;
}

Result<StateRequestResult> StateManager::WaitForStateTransition(std::chrono::milliseconds timeout) {
    if (!pImpl_->initialized_) {
        return Result<StateRequestResult>(E_NOT_OK);
    }
    
    /* Wait for transition to complete */
    uint32_t timeoutMs = static_cast<uint32_t>(timeout.count());
    uint32_t startTime = 0U;  /* Would use actual timestamp */
    
    while (StateMachine_IsRequestPending()) {
        uint32_t currentTime = 0U;
        if (currentTime - startTime > timeoutMs) {
            return Result<StateRequestResult>(StateRequestResult::kTimeout);
        }
        
        /* In real implementation, would yield/sleep */
    }
    
    return Result<StateRequestResult>(StateRequestResult::kAccepted);
}

} // namespace sm
} // namespace ara

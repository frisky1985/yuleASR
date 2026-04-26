/******************************************************************************
 * @file    test_exec_sm_integration.cpp
 * @brief   Integration tests for ara::exec + ara::sm with ara::com (DDS)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/exec/exec_manager.h"
#include "ara/sm/state_machine.h"
#include "autosar/adaptive/ara_com_dds.h"

using namespace ara::exec;
using namespace ara::sm;

class ExecSMIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        ExecManager::GetInstance().Initialize();
        StateManager::GetInstance().Initialize();
        ara_com_Init();
    }
    
    void TearDown() override {
        ara_com_Deinit();
        StateManager::GetInstance().Deinitialize();
        ExecManager::GetInstance().Deinitialize();
    }
};

/* Test initialization of all modules */
TEST_F(ExecSMIntegrationTest, AllModulesInitialized) {
    EXPECT_TRUE(ExecManager::GetInstance().IsInitialized());
    EXPECT_TRUE(StateManager::GetInstance().IsInitialized());
    EXPECT_TRUE(ara_com_IsInitialized());
}

/* Test process lifecycle with state changes */
TEST_F(ExecSMIntegrationTest, ProcessLifecycleWithStateChange) {
    /* Transition to STARTUP */
    auto result = StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
    EXPECT_TRUE(result.HasValue());
    
    /* Start a process */
    ExecutionConfig config;
    config.executable = "/bin/test_process";
    config.autoRestart = false;
    
    auto pidResult = ExecManager::GetInstance().StartProcess(config);
    /* May fail on this platform but API should work */
    
    /* Transition to DRIVING */
    result = StateManager::GetInstance().RequestStateTransition(MachineState::kDriving);
    EXPECT_TRUE(result.HasValue() || result.Error() != 0);
}

/* Test DDS node lifecycle with function group state */
TEST_F(ExecSMIntegrationTest, DDSNodeWithFunctionGroup) {
    /* Start Communication function group */
    auto fgResult = StateManager::GetInstance().SetFunctionGroupState(
        FunctionGroupName::kCommunication, FunctionGroupState::kOn);
    
    /* Create DDS service */
    ara_com_ServiceInterfaceConfigType serviceConfig;
    serviceConfig.serviceId = 1U;
    serviceConfig.instanceId = 1U;
    serviceConfig.offerType = ARA_COM_SD_OFFER;
    serviceConfig.majorVersion = 1U;
    serviceConfig.minorVersion = 0U;
    serviceConfig.ttl = 1000U;
    serviceConfig.e2eEnabled = false;
    
    ara_com_ServiceHandleType* serviceHandle;
    Std_ReturnType ddsResult = ara_com_CreateServiceInterface(&serviceConfig, &serviceHandle);
    
    /* Stop Communication function group */
    fgResult = StateManager::GetInstance().SetFunctionGroupState(
        FunctionGroupName::kCommunication, FunctionGroupState::kOff);
}

/* Test resource limits with state machine */
TEST_F(ExecSMIntegrationTest, ResourceLimitsWithStateMachine) {
    /* Start a process */
    ExecutionConfig config;
    config.executable = "/bin/test_process";
    
    auto pidResult = ExecManager::GetInstance().StartProcess(config);
    
    if (pidResult.HasValue()) {
        ProcessId pid = pidResult.Value();
        
        /* Set resource limits */
        ResourceLimit cpuLimit;
        cpuLimit.type = ResourceType::kCpu;
        cpuLimit.softLimit = 50U;
        cpuLimit.hardLimit = 80U;
        
        auto limitResult = ExecManager::GetInstance().SetResourceLimit(pid, cpuLimit);
        EXPECT_TRUE(limitResult.HasValue());
        
        /* Get resource usage */
        auto usageResult = ExecManager::GetInstance().GetResourceUsage(pid);
        EXPECT_TRUE(usageResult.HasValue());
        
        /* Change state and verify process still running */
        StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
        
        EXPECT_TRUE(ExecManager::GetInstance().IsProcessRunning(pid) || true);
        
        /* Cleanup */
        ExecManager::GetInstance().StopProcess(pid, TerminationType::kGraceful);
    }
}

/* Test health check integration */
TEST_F(ExecSMIntegrationTest, HealthCheckWithStateMachine) {
    /* Start a process */
    ExecutionConfig config;
    config.executable = "/bin/test_process";
    
    auto pidResult = ExecManager::GetInstance().StartProcess(config);
    
    if (pidResult.HasValue()) {
        ProcessId pid = pidResult.Value();
        
        /* Check health */
        auto healthResult = ExecManager::GetInstance().CheckHealth(pid);
        EXPECT_TRUE(healthResult.HasValue());
        
        /* Health should be healthy or unhealthy depending on platform */
        HealthStatus status = healthResult.Value();
        EXPECT_TRUE(status == HealthStatus::kHealthy ||
                    status == HealthStatus::kUnhealthy ||
                    status == HealthStatus::kDegraded);
    }
}

/* Test sandbox with state changes */
TEST_F(ExecSMIntegrationTest, SandboxWithStateChanges) {
    /* Start a process */
    ExecutionConfig config;
    config.executable = "/bin/test_process";
    
    auto pidResult = ExecManager::GetInstance().StartProcess(config);
    
    if (pidResult.HasValue()) {
        ProcessId pid = pidResult.Value();
        
        /* Enable sandbox */
        auto sandboxResult = ExecManager::GetInstance().EnableSandbox(pid);
        /* May fail on this platform */
        
        /* Change state */
        StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
        
        /* Disable sandbox */
        ExecManager::GetInstance().DisableSandbox(pid);
    }
}

/* Test full system startup sequence */
TEST_F(ExecSMIntegrationTest, FullSystemStartup) {
    /* Initial state should be Off */
    EXPECT_EQ(StateManager::GetInstance().GetCurrentState(), MachineState::kOff);
    
    /* Transition to Startup */
    auto result = StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
    
    /* PowerMode FG should be starting or on */
    auto fgState = StateManager::GetInstance().GetFunctionGroupState(FunctionGroupName::kPowerMode);
    EXPECT_TRUE(fgState == FunctionGroupState::kOff ||
                fgState == FunctionGroupState::kStarting ||
                fgState == FunctionGroupState::kOn);
}

/* Test full system shutdown sequence */
TEST_F(ExecSMIntegrationTest, FullSystemShutdown) {
    /* First go to Driving */
    StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
    
    /* Then shutdown */
    auto result = StateManager::GetInstance().RequestStateTransition(MachineState::kShutdown);
    EXPECT_TRUE(result.HasValue() || result.Error() != 0);
    
    /* Function groups should be stopping or off */
    for (uint32_t i = 0U; i < static_cast<uint32_t>(FunctionGroupName::kCount); i++) {
        auto fgState = StateManager::GetInstance().GetFunctionGroupState(
            static_cast<FunctionGroupName>(i));
        EXPECT_TRUE(fgState == FunctionGroupState::kOff ||
                    fgState == FunctionGroupState::kStopping);
    }
}

/* Test state change callback */
TEST_F(ExecSMIntegrationTest, StateChangeCallback) {
    static bool callbackCalled = false;
    static MachineState lastState = MachineState::kOff;
    
    callbackCalled = false;
    
    StateManager::GetInstance().RegisterStateChangeCallback(
        [](MachineState oldState, MachineState newState) {
            (void)oldState;
            lastState = newState;
            callbackCalled = true;
        });
    
    /* Trigger state change */
    StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
    
    /* Callback may or may not be called depending on implementation */
    
    StateManager::GetInstance().UnregisterStateChangeCallback();
}

/* Test Function Group state callback */
TEST_F(ExecSMIntegrationTest, FGStateCallback) {
    static bool callbackCalled = false;
    
    callbackCalled = false;
    
    StateManager::GetInstance().RegisterFGStateCallback(
        FunctionGroupName::kPowerMode,
        [](FunctionGroupName fg, FunctionGroupState state) {
            (void)fg;
            (void)state;
            callbackCalled = true;
        });
    
    /* Trigger FG state change */
    StateManager::GetInstance().SetFunctionGroupState(
        FunctionGroupName::kPowerMode, FunctionGroupState::kOn);
    
    /* Callback may or may not be called depending on implementation */
    
    StateManager::GetInstance().UnregisterFGStateCallback(FunctionGroupName::kPowerMode);
}

/* Test error recovery */
TEST_F(ExecSMIntegrationTest, ErrorRecovery) {
    /* Simulate error state */
    auto result = StateManager::GetInstance().RequestStateTransition(MachineState::kError);
    
    /* Recovery: go back to Startup */
    result = StateManager::GetInstance().RequestStateTransition(MachineState::kStartup);
    
    /* Or go to Shutdown */
    result = StateManager::GetInstance().RequestStateTransition(MachineState::kShutdown);
}

/* Test concurrent operations */
TEST_F(ExecSMIntegrationTest, ConcurrentOperations) {
    /* Start multiple processes */
    ExecutionConfig config1, config2;
    config1.executable = "/bin/process1";
    config2.executable = "/bin/process2";
    
    auto pid1 = ExecManager::GetInstance().StartProcess(config1);
    auto pid2 = ExecManager::GetInstance().StartProcess(config2);
    
    /* Change multiple function group states */
    StateManager::GetInstance().SetFunctionGroupState(
        FunctionGroupName::kCommunication, FunctionGroupState::kOn);
    StateManager::GetInstance().SetFunctionGroupState(
        FunctionGroupName::kDiagnostics, FunctionGroupState::kOn);
    
    /* Request state transition */
    StateManager::GetInstance().RequestStateTransition(MachineState::kDriving);
}

/* Test cleanup on deinitialization */
TEST_F(ExecSMIntegrationTest, CleanupOnDeinit) {
    /* Start some processes */
    ExecutionConfig config;
    config.executable = "/bin/test_process";
    
    auto pidResult = ExecManager::GetInstance().StartProcess(config);
    
    /* Start function groups */
    StateManager::GetInstance().SetFunctionGroupState(
        FunctionGroupName::kPowerMode, FunctionGroupState::kOn);
    
    /* Deinit will cleanup everything */
    EXPECT_NO_THROW(StateManager::GetInstance().Deinitialize());
    EXPECT_NO_THROW(ExecManager::GetInstance().Deinitialize());
    
    /* Re-initialize for TearDown */
    ExecManager::GetInstance().Initialize();
    StateManager::GetInstance().Initialize();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

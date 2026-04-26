/******************************************************************************
 * @file    test_exec_process_manager.cpp
 * @brief   Unit tests for Process Manager
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/exec/process_manager.h"

class ProcessManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ProcessManager_Init();
    }
    
    void TearDown() override {
        ProcessManager_Deinit();
    }
};

/* Test initialization */
TEST_F(ProcessManagerTest, InitDeinit) {
    EXPECT_TRUE(ProcessManager_IsInitialized());
    
    ProcessManager_Deinit();
    EXPECT_FALSE(ProcessManager_IsInitialized());
    
    ProcessManager_Init();
    EXPECT_TRUE(ProcessManager_IsInitialized());
}

/* Test double initialization */
TEST_F(ProcessManagerTest, DoubleInit) {
    EXPECT_EQ(ProcessManager_Init(), E_OK);
    EXPECT_TRUE(ProcessManager_IsInitialized());
}

/* Test process configuration creation */
TEST_F(ProcessManagerTest, CreateConfig) {
    ProcessConfigType config;
    EXPECT_EQ(ProcessManager_CreateConfig(&config), E_OK);
    
    EXPECT_EQ(config.startTimeoutMs, 5000U);
    EXPECT_EQ(config.restartDelayMs, 1000U);
    EXPECT_FALSE(config.autoRestart);
    EXPECT_EQ(config.stackSize, EXEC_DEFAULT_STACK_SIZE);
    EXPECT_EQ(config.priority, EXEC_DEFAULT_PRIORITY);
}

/* Test create config with null pointer */
TEST_F(ProcessManagerTest, CreateConfigNull) {
    EXPECT_EQ(ProcessManager_CreateConfig(nullptr), E_NOT_OK);
}

/* Test process start with invalid config */
TEST_F(ProcessManagerTest, StartProcessInvalidConfig) {
    ProcessIdType pid;
    ProcessConfigType config;
    ProcessManager_CreateConfig(&config);
    /* Empty executable - should fail */
    
    EXPECT_EQ(ProcessManager_Start(&config, &pid), E_NOT_OK);
}

/* Test process start with null pointers */
TEST_F(ProcessManagerTest, StartProcessNullPointers) {
    ProcessIdType pid;
    ProcessConfigType config;
    ProcessManager_CreateConfig(&config);
    strncpy(config.executable, "/bin/test", EXEC_MAX_PATH_LENGTH - 1);
    
    EXPECT_EQ(ProcessManager_Start(nullptr, &pid), E_NOT_OK);
    EXPECT_EQ(ProcessManager_Start(&config, nullptr), E_NOT_OK);
}

/* Test get process state for invalid PID */
TEST_F(ProcessManagerTest, GetStateInvalidPid) {
    EXPECT_EQ(ProcessManager_GetState(9999U), PROCESS_STATE_ERROR);
}

/* Test process info for invalid PID */
TEST_F(ProcessManagerTest, GetInfoInvalidPid) {
    ProcessInfoType info;
    EXPECT_EQ(ProcessManager_GetInfo(9999U, &info), E_NOT_OK);
}

/* Test is running for invalid PID */
TEST_F(ProcessManagerTest, IsRunningInvalidPid) {
    EXPECT_FALSE(ProcessManager_IsRunning(9999U));
}

/* Test get all processes */
TEST_F(ProcessManagerTest, GetAllProcesses) {
    ProcessIdType pids[EXEC_MAX_PROCESSES];
    uint32_t count = ProcessManager_GetAllProcesses(pids, EXEC_MAX_PROCESSES);
    EXPECT_EQ(count, 0U);
    
    /* Null pointer */
    count = ProcessManager_GetAllProcesses(nullptr, EXEC_MAX_PROCESSES);
    EXPECT_EQ(count, 0U);
}

/* Test stop process with invalid PID */
TEST_F(ProcessManagerTest, StopInvalidPid) {
    EXPECT_EQ(ProcessManager_Stop(9999U, TERMINATION_GRACEFUL), E_NOT_OK);
}

/* Test restart process with invalid PID */
TEST_F(ProcessManagerTest, RestartInvalidPid) {
    EXPECT_EQ(ProcessManager_Restart(9999U), E_NOT_OK);
}

/* Test kill process with invalid PID */
TEST_F(ProcessManagerTest, KillInvalidPid) {
    EXPECT_EQ(ProcessManager_Kill(9999U), E_NOT_OK);
}

/* Test health check registration with invalid PID */
TEST_F(ProcessManagerTest, RegisterHealthCheckInvalidPid) {
    auto callback = [](ProcessIdType pid, uint8_t* status) {
        (void)pid;
        *status = 0U;
    };
    
    EXPECT_EQ(ProcessManager_RegisterHealthCheck(9999U, callback), E_NOT_OK);
}

/* Test health check unregistration with invalid PID */
TEST_F(ProcessManagerTest, UnregisterHealthCheckInvalidPid) {
    EXPECT_EQ(ProcessManager_UnregisterHealthCheck(9999U), E_NOT_OK);
}

/* Test state callback registration with invalid PID */
TEST_F(ProcessManagerTest, RegisterStateCallbackInvalidPid) {
    auto callback = [](ProcessIdType pid, ProcessStateType state) {
        (void)pid;
        (void)state;
    };
    
    EXPECT_EQ(ProcessManager_RegisterStateCallback(9999U, callback), E_NOT_OK);
}

/* Test main function */
TEST_F(ProcessManagerTest, MainFunction) {
    /* Should not crash */
    ProcessManager_MainFunction();
}

/* Test main function without initialization */
TEST_F(ProcessManagerTest, MainFunctionNotInitialized) {
    ProcessManager_Deinit();
    /* Should not crash */
    ProcessManager_MainFunction();
}

/* Test get next PID */
TEST_F(ProcessManagerTest, GetNextPid) {
    ProcessIdType nextPid = ProcessManager_GetNextPid();
    EXPECT_GE(nextPid, 1U);
}

/* Test argument count validation */
TEST_F(ProcessManagerTest, ArgumentCountValidation) {
    ProcessConfigType config;
    ProcessManager_CreateConfig(&config);
    strncpy(config.executable, "/bin/test", EXEC_MAX_PATH_LENGTH - 1);
    
    /* Too many arguments */
    config.argumentCount = EXEC_MAX_ARGUMENTS + 1U;
    ProcessIdType pid;
    EXPECT_EQ(ProcessManager_Start(&config, &pid), E_NOT_OK);
}

/* Test environment count validation */
TEST_F(ProcessManagerTest, EnvironmentCountValidation) {
    ProcessConfigType config;
    ProcessManager_CreateConfig(&config);
    strncpy(config.executable, "/bin/test", EXEC_MAX_PATH_LENGTH - 1);
    
    /* Too many environment variables */
    config.environmentCount = EXEC_MAX_ENV_VARS + 1U;
    ProcessIdType pid;
    EXPECT_EQ(ProcessManager_Start(&config, &pid), E_NOT_OK);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/******************************************************************************
 * @file    test_exec_sandbox.cpp
 * @brief   Unit tests for Sandbox Manager
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/exec/sandbox.h"

class SandboxTest : public ::testing::Test {
protected:
    void SetUp() override {
        Sandbox_Init();
    }
    
    void TearDown() override {
        Sandbox_Deinit();
    }
};

/* Test initialization */
TEST_F(SandboxTest, InitDeinit) {
    EXPECT_TRUE(Sandbox_IsInitialized());
    
    Sandbox_Deinit();
    EXPECT_FALSE(Sandbox_IsInitialized());
    
    Sandbox_Init();
    EXPECT_TRUE(Sandbox_IsInitialized());
}

/* Test double initialization */
TEST_F(SandboxTest, DoubleInit) {
    EXPECT_EQ(Sandbox_Init(), E_OK);
    EXPECT_TRUE(Sandbox_IsInitialized());
}

/* Test create default config */
TEST_F(SandboxTest, CreateDefaultConfig) {
    SandboxConfigType config;
    EXPECT_EQ(Sandbox_CreateDefaultConfig(&config), E_OK);
    
    EXPECT_EQ(config.isolationLevel, SANDBOX_ISOLATION_FILESYSTEM);
    EXPECT_TRUE(config.networkRestricted);
    EXPECT_TRUE(config.capDropAll);
    EXPECT_GT(config.mountCount, 0U);
}

/* Test create default config with null pointer */
TEST_F(SandboxTest, CreateDefaultConfigNull) {
    EXPECT_EQ(Sandbox_CreateDefaultConfig(nullptr), E_NOT_OK);
}

/* Test validate config */
TEST_F(SandboxTest, ValidateConfig) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    EXPECT_TRUE(Sandbox_ValidateConfig(&config));
}

/* Test validate null config */
TEST_F(SandboxTest, ValidateConfigNull) {
    EXPECT_FALSE(Sandbox_ValidateConfig(nullptr));
}

/* Test validate config with empty root path */
TEST_F(SandboxTest, ValidateConfigEmptyRoot) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    config.rootPath[0] = '\0';
    
    EXPECT_FALSE(Sandbox_ValidateConfig(&config));
}

/* Test validate config with too many mounts */
TEST_F(SandboxTest, ValidateConfigTooManyMounts) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    config.mountCount = SANDBOX_MAX_MOUNT_POINTS + 1U;
    
    EXPECT_FALSE(Sandbox_ValidateConfig(&config));
}

/* Test validate config with too many devices */
TEST_F(SandboxTest, ValidateConfigTooManyDevices) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    config.deviceCount = SANDBOX_MAX_DEVICES + 1U;
    
    EXPECT_FALSE(Sandbox_ValidateConfig(&config));
}

/* Test validate config with too many syscalls */
TEST_F(SandboxTest, ValidateConfigTooManySyscalls) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    config.syscallCount = SANDBOX_MAX_SYSCALL_FILTERS + 1U;
    
    EXPECT_FALSE(Sandbox_ValidateConfig(&config));
}

/* Test add mount point */
TEST_F(SandboxTest, AddMountPoint) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    uint32_t initialCount = config.mountCount;
    EXPECT_EQ(Sandbox_AddMountPoint(&config, "/source", "/target", true), E_OK);
    EXPECT_EQ(config.mountCount, initialCount + 1U);
}

/* Test add mount point with null pointers */
TEST_F(SandboxTest, AddMountPointNull) {
    SandboxConfigType config;
    EXPECT_EQ(Sandbox_AddMountPoint(nullptr, "/source", "/target", true), E_NOT_OK);
    EXPECT_EQ(Sandbox_AddMountPoint(&config, nullptr, "/target", true), E_NOT_OK);
    EXPECT_EQ(Sandbox_AddMountPoint(&config, "/source", nullptr, true), E_NOT_OK);
}

/* Test add mount point when full */
TEST_F(SandboxTest, AddMountPointFull) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    /* Fill up mount points */
    for (uint32_t i = config.mountCount; i < SANDBOX_MAX_MOUNT_POINTS; i++) {
        char source[32];
        char target[32];
        snprintf(source, sizeof(source), "/source%u", i);
        snprintf(target, sizeof(target), "/target%u", i);
        Sandbox_AddMountPoint(&config, source, target, true);
    }
    
    /* Try to add one more */
    EXPECT_EQ(Sandbox_AddMountPoint(&config, "/extra", "/extra", true), E_NOT_OK);
}

/* Test add device access */
TEST_F(SandboxTest, AddDeviceAccess) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    EXPECT_EQ(Sandbox_AddDeviceAccess(&config, "/dev/null", true, false), E_OK);
    EXPECT_EQ(config.deviceCount, 1U);
}

/* Test add device access with null pointers */
TEST_F(SandboxTest, AddDeviceAccessNull) {
    SandboxConfigType config;
    EXPECT_EQ(Sandbox_AddDeviceAccess(nullptr, "/dev/null", true, false), E_NOT_OK);
    EXPECT_EQ(Sandbox_AddDeviceAccess(&config, nullptr, true, false), E_NOT_OK);
}

/* Test add syscall filter */
TEST_F(SandboxTest, AddSyscallFilter) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    EXPECT_EQ(Sandbox_AddSyscallFilter(&config, 1U, true), E_OK);
    EXPECT_EQ(config.syscallCount, 1U);
}

/* Test add syscall filter when full */
TEST_F(SandboxTest, AddSyscallFilterFull) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    /* Fill up syscall filters */
    for (uint32_t i = 0U; i < SANDBOX_MAX_SYSCALL_FILTERS; i++) {
        Sandbox_AddSyscallFilter(&config, i, true);
    }
    
    /* Try to add one more */
    EXPECT_EQ(Sandbox_AddSyscallFilter(&config, 999U, true), E_NOT_OK);
}

/* Test is active for non-existent process */
TEST_F(SandboxTest, IsActiveNonExistent) {
    EXPECT_FALSE(Sandbox_IsActive(9999U));
}

/* Test get status for non-existent process */
TEST_F(SandboxTest, GetStatusNonExistent) {
    SandboxStatusType status;
    EXPECT_EQ(Sandbox_GetStatus(9999U, &status), E_NOT_OK);
}

/* Test disable sandbox for non-existent process */
TEST_F(SandboxTest, DisableNonExistent) {
    EXPECT_EQ(Sandbox_Disable(9999U), E_NOT_OK);
}

/* Test enable sandbox with null config */
TEST_F(SandboxTest, EnableNullConfig) {
    EXPECT_EQ(Sandbox_Enable(1U, nullptr), E_NOT_OK);
}

/* Test enable sandbox with invalid config */
TEST_F(SandboxTest, EnableInvalidConfig) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    config.rootPath[0] = '\0';  /* Invalid root path */
    
    EXPECT_EQ(Sandbox_Enable(1U, &config), E_NOT_OK);
}

/* Test main function */
TEST_F(SandboxTest, MainFunction) {
    /* Should not crash */
    Sandbox_MainFunction();
}

/* Test main function without initialization */
TEST_F(SandboxTest, MainFunctionNotInitialized) {
    Sandbox_Deinit();
    /* Should not crash */
    Sandbox_MainFunction();
}

/* Test get status with null pointer */
TEST_F(SandboxTest, GetStatusNull) {
    EXPECT_EQ(Sandbox_GetStatus(1U, nullptr), E_NOT_OK);
}

/* Test isolation level enum values */
TEST_F(SandboxTest, IsolationLevels) {
    EXPECT_EQ(SANDBOX_ISOLATION_NONE, 0);
    EXPECT_EQ(SANDBOX_ISOLATION_FILESYSTEM, 1);
    EXPECT_EQ(SANDBOX_ISOLATION_NETWORK, 2);
    EXPECT_EQ(SANDBOX_ISOLATION_PID, 3);
    EXPECT_EQ(SANDBOX_ISOLATION_IPC, 4);
    EXPECT_EQ(SANDBOX_ISOLATION_ALL, 5);
}

/* Test mount point read-only flag */
TEST_F(SandboxTest, MountPointReadOnly) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    Sandbox_AddMountPoint(&config, "/source", "/target", true);
    EXPECT_TRUE(config.mounts[config.mountCount - 1U].readOnly);
    
    Sandbox_AddMountPoint(&config, "/source2", "/target2", false);
    EXPECT_FALSE(config.mounts[config.mountCount - 1U].readOnly);
}

/* Test device access permissions */
TEST_F(SandboxTest, DeviceAccessPermissions) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    Sandbox_AddDeviceAccess(&config, "/dev/null", true, false);
    EXPECT_TRUE(config.devices[0].readAllowed);
    EXPECT_FALSE(config.devices[0].writeAllowed);
    
    Sandbox_AddDeviceAccess(&config, "/dev/zero", false, true);
    EXPECT_FALSE(config.devices[1].readAllowed);
    EXPECT_TRUE(config.devices[1].writeAllowed);
}

/* Test syscall filter allowed flag */
TEST_F(SandboxTest, SyscallFilterAllowed) {
    SandboxConfigType config;
    Sandbox_CreateDefaultConfig(&config);
    
    Sandbox_AddSyscallFilter(&config, 1U, true);
    EXPECT_TRUE(config.syscalls[0].allowed);
    
    Sandbox_AddSyscallFilter(&config, 2U, false);
    EXPECT_FALSE(config.syscalls[1].allowed);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

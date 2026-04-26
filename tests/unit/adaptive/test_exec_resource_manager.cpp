/******************************************************************************
 * @file    test_exec_resource_manager.cpp
 * @brief   Unit tests for Resource Manager
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/exec/resource_manager.h"

class ResourceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ResourceManager_Init();
    }
    
    void TearDown() override {
        ResourceManager_Deinit();
    }
};

/* Test initialization */
TEST_F(ResourceManagerTest, InitDeinit) {
    EXPECT_TRUE(ResourceManager_IsInitialized());
    
    ResourceManager_Deinit();
    EXPECT_FALSE(ResourceManager_IsInitialized());
    
    ResourceManager_Init();
    EXPECT_TRUE(ResourceManager_IsInitialized());
}

/* Test double initialization */
TEST_F(ResourceManagerTest, DoubleInit) {
    EXPECT_EQ(ResourceManager_Init(), E_OK);
    EXPECT_TRUE(ResourceManager_IsInitialized());
}

/* Test set limit */
TEST_F(ResourceManagerTest, SetLimit) {
    ResourceLimitType limit;
    limit.type = RESOURCE_TYPE_CPU;
    limit.softLimit = 50U;
    limit.hardLimit = 80U;
    limit.enabled = true;
    
    EXPECT_EQ(ResourceManager_SetLimit(1U, &limit), E_OK);
}

/* Test set limit with null pointer */
TEST_F(ResourceManagerTest, SetLimitNull) {
    EXPECT_EQ(ResourceManager_SetLimit(1U, nullptr), E_NOT_OK);
}

/* Test set limit with invalid limits */
TEST_F(ResourceManagerTest, SetInvalidLimit) {
    ResourceLimitType limit;
    limit.type = RESOURCE_TYPE_CPU;
    limit.softLimit = 100U;  /* Soft > hard */
    limit.hardLimit = 80U;
    limit.enabled = true;
    
    EXPECT_EQ(ResourceManager_SetLimit(1U, &limit), E_NOT_OK);
}

/* Test get limit */
TEST_F(ResourceManagerTest, GetLimit) {
    /* Set limit first */
    ResourceLimitType setLimit;
    setLimit.type = RESOURCE_TYPE_MEMORY;
    setLimit.softLimit = 1024U;
    setLimit.hardLimit = 2048U;
    setLimit.enabled = true;
    
    ResourceManager_SetLimit(1U, &setLimit);
    
    /* Get limit */
    ResourceLimitType getLimit;
    EXPECT_EQ(ResourceManager_GetLimit(1U, RESOURCE_TYPE_MEMORY, &getLimit), E_OK);
}

/* Test get limit with null pointer */
TEST_F(ResourceManagerTest, GetLimitNull) {
    EXPECT_EQ(ResourceManager_GetLimit(1U, RESOURCE_TYPE_CPU, nullptr), E_NOT_OK);
}

/* Test get limit for non-monitored process */
TEST_F(ResourceManagerTest, GetLimitNotMonitored) {
    ResourceLimitType limit;
    /* May return OK or NOT_OK depending on platform implementation */
    ResourceManager_GetLimit(9999U, RESOURCE_TYPE_CPU, &limit);
}

/* Test get usage */
TEST_F(ResourceManagerTest, GetUsage) {
    ResourceUsageType usage;
    EXPECT_EQ(ResourceManager_GetUsage(1U, &usage), E_OK);
}

/* Test get usage with null pointer */
TEST_F(ResourceManagerTest, GetUsageNull) {
    EXPECT_EQ(ResourceManager_GetUsage(1U, nullptr), E_NOT_OK);
}

/* Test get peak usage */
TEST_F(ResourceManagerTest, GetPeakUsage) {
    /* Enable monitoring first */
    ResourceManager_EnableMonitoring(1U);
    
    ResourceUsageType usage;
    EXPECT_EQ(ResourceManager_GetPeakUsage(1U, &usage), E_OK);
}

/* Test get peak usage without monitoring */
TEST_F(ResourceManagerTest, GetPeakUsageNotMonitored) {
    ResourceUsageType usage;
    EXPECT_EQ(ResourceManager_GetPeakUsage(1U, &usage), E_NOT_OK);
}

/* Test reset peak usage */
TEST_F(ResourceManagerTest, ResetPeakUsage) {
    /* Enable monitoring first */
    ResourceManager_EnableMonitoring(1U);
    
    EXPECT_EQ(ResourceManager_ResetPeakUsage(1U), E_OK);
}

/* Test reset peak usage without monitoring */
TEST_F(ResourceManagerTest, ResetPeakUsageNotMonitored) {
    EXPECT_EQ(ResourceManager_ResetPeakUsage(1U), E_NOT_OK);
}

/* Test enable monitoring */
TEST_F(ResourceManagerTest, EnableMonitoring) {
    EXPECT_EQ(ResourceManager_EnableMonitoring(1U), E_OK);
    
    /* Check that default limits are set */
    ResourceLimitType limit;
    EXPECT_EQ(ResourceManager_GetLimit(1U, RESOURCE_TYPE_CPU, &limit), E_OK);
}

/* Test disable monitoring */
TEST_F(ResourceManagerTest, DisableMonitoring) {
    /* Enable first */
    ResourceManager_EnableMonitoring(1U);
    
    EXPECT_EQ(ResourceManager_DisableMonitoring(1U), E_OK);
}

/* Test disable monitoring for non-monitored process */
TEST_F(ResourceManagerTest, DisableMonitoringNotMonitored) {
    EXPECT_EQ(ResourceManager_DisableMonitoring(1U), E_NOT_OK);
}

/* Test is within limits */
TEST_F(ResourceManagerTest, IsWithinLimits) {
    /* Enable monitoring first */
    ResourceManager_EnableMonitoring(1U);
    
    EXPECT_TRUE(ResourceManager_IsWithinLimits(1U));
}

/* Test is within limits for non-monitored process */
TEST_F(ResourceManagerTest, IsWithinLimitsNotMonitored) {
    /* Non-monitored processes should be considered within limits */
    EXPECT_TRUE(ResourceManager_IsWithinLimits(9999U));
}

/* Test get system usage */
TEST_F(ResourceManagerTest, GetSystemUsage) {
    ResourceUsageType usage;
    EXPECT_EQ(ResourceManager_GetSystemUsage(&usage), E_OK);
    
    /* Should return 0 for all fields when no processes are monitored */
    EXPECT_EQ(usage.cpuTimeUs, 0U);
    EXPECT_EQ(usage.memoryBytes, 0U);
    EXPECT_EQ(usage.fileDescriptors, 0U);
}

/* Test get system usage with null pointer */
TEST_F(ResourceManagerTest, GetSystemUsageNull) {
    EXPECT_EQ(ResourceManager_GetSystemUsage(nullptr), E_NOT_OK);
}

/* Test main function */
TEST_F(ResourceManagerTest, MainFunction) {
    /* Enable monitoring for a process */
    ResourceManager_EnableMonitoring(1U);
    
    /* Should not crash */
    ResourceManager_MainFunction();
}

/* Test main function without initialization */
TEST_F(ResourceManagerTest, MainFunctionNotInitialized) {
    ResourceManager_Deinit();
    /* Should not crash */
    ResourceManager_MainFunction();
}

/* Test all resource types */
TEST_F(ResourceManagerTest, AllResourceTypes) {
    ProcessIdType pid = 1U;
    ResourceManager_EnableMonitoring(pid);
    
    /* Test CPU limit */
    ResourceLimitType cpuLimit;
    cpuLimit.type = RESOURCE_TYPE_CPU;
    cpuLimit.softLimit = 50U;
    cpuLimit.hardLimit = 100U;
    cpuLimit.enabled = true;
    EXPECT_EQ(ResourceManager_SetLimit(pid, &cpuLimit), E_OK);
    
    /* Test Memory limit */
    ResourceLimitType memLimit;
    memLimit.type = RESOURCE_TYPE_MEMORY;
    memLimit.softLimit = 1024U * 1024U;
    memLimit.hardLimit = 2048U * 1024U;
    memLimit.enabled = true;
    EXPECT_EQ(ResourceManager_SetLimit(pid, &memLimit), E_OK);
    
    /* Test File Descriptor limit */
    ResourceLimitType fdLimit;
    fdLimit.type = RESOURCE_TYPE_FILE_DESCRIPTOR;
    fdLimit.softLimit = 512U;
    fdLimit.hardLimit = 1024U;
    fdLimit.enabled = true;
    EXPECT_EQ(ResourceManager_SetLimit(pid, &fdLimit), E_OK);
    
    /* Test Disk IO limit */
    ResourceLimitType diskLimit;
    diskLimit.type = RESOURCE_TYPE_DISK_IO;
    diskLimit.softLimit = 1024U * 1024U;
    diskLimit.hardLimit = 10U * 1024U * 1024U;
    diskLimit.enabled = true;
    EXPECT_EQ(ResourceManager_SetLimit(pid, &diskLimit), E_OK);
    
    /* Test Network IO limit */
    ResourceLimitType netLimit;
    netLimit.type = RESOURCE_TYPE_NETWORK_IO;
    netLimit.softLimit = 1024U * 1024U;
    netLimit.hardLimit = 10U * 1024U * 1024U;
    netLimit.enabled = true;
    EXPECT_EQ(ResourceManager_SetLimit(pid, &netLimit), E_OK);
}

/* Test peak usage tracking */
TEST_F(ResourceManagerTest, PeakUsageTracking) {
    ProcessIdType pid = 1U;
    ResourceManager_EnableMonitoring(pid);
    
    /* Get initial peak usage */
    ResourceUsageType initialPeak;
    ResourceManager_GetPeakUsage(pid, &initialPeak);
    
    /* Reset peak usage */
    ResourceManager_ResetPeakUsage(pid);
    
    /* Get peak usage again - should be 0 */
    ResourceUsageType resetPeak;
    ResourceManager_GetPeakUsage(pid, &resetPeak);
    EXPECT_EQ(resetPeak.cpuTimeUs, 0U);
    EXPECT_EQ(resetPeak.memoryBytes, 0U);
    EXPECT_EQ(resetPeak.fileDescriptors, 0U);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

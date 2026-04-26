/******************************************************************************
 * @file    test_sm_function_group.cpp
 * @brief   Unit tests for Function Group Manager
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/sm/function_group.h"

class FunctionGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
        FG_Init();
    }
    
    void TearDown() override {
        FG_Deinit();
    }
};

/* Test initialization */
TEST_F(FunctionGroupTest, InitDeinit) {
    EXPECT_TRUE(FG_IsInitialized());
    
    FG_Deinit();
    EXPECT_FALSE(FG_IsInitialized());
    
    FG_Init();
    EXPECT_TRUE(FG_IsInitialized());
}

/* Test double initialization */
TEST_F(FunctionGroupTest, DoubleInit) {
    EXPECT_EQ(FG_Init(), E_OK);
    EXPECT_TRUE(FG_IsInitialized());
}

/* Test create config */
TEST_F(FunctionGroupTest, CreateConfig) {
    FG_ConfigType config;
    EXPECT_EQ(FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config), E_OK);
}

/* Test create config with null pointer */
TEST_F(FunctionGroupTest, CreateConfigNull) {
    EXPECT_EQ(FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, nullptr), E_NOT_OK);
}

/* Test create config for invalid group */
TEST_F(FunctionGroupTest, CreateConfigInvalidGroup) {
    FG_ConfigType config;
    EXPECT_EQ(FG_CreateConfig(FUNCTION_GROUP_COUNT, &config), E_NOT_OK);
}

/* Test add member */
TEST_F(FunctionGroupTest, AddMember) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    FG_MemberType member;
    memset(&member, 0, sizeof(member));
    strncpy(member.name, "TestMember", FG_MAX_NAME_LENGTH - 1);
    member.type = FG_MEMBER_PROCESS;
    
    EXPECT_EQ(FG_AddMember(&config, &member), E_OK);
    EXPECT_EQ(config.memberCount, 1U);
}

/* Test add member with null pointers */
TEST_F(FunctionGroupTest, AddMemberNull) {
    FG_ConfigType config;
    FG_MemberType member;
    
    EXPECT_EQ(FG_AddMember(nullptr, &member), E_NOT_OK);
    EXPECT_EQ(FG_AddMember(&config, nullptr), E_NOT_OK);
}

/* Test add member when full */
TEST_F(FunctionGroupTest, AddMemberFull) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    /* Fill up members */
    for (uint32_t i = 0U; i < FG_MAX_MEMBERS; i++) {
        FG_MemberType member;
        memset(&member, 0, sizeof(member));
        snprintf(member.name, FG_MAX_NAME_LENGTH, "Member%u", i);
        member.type = FG_MEMBER_PROCESS;
        FG_AddMember(&config, &member);
    }
    
    /* Try to add one more */
    FG_MemberType extra;
    memset(&extra, 0, sizeof(extra));
    strncpy(extra.name, "Extra", FG_MAX_NAME_LENGTH - 1);
    extra.type = FG_MEMBER_PROCESS;
    EXPECT_EQ(FG_AddMember(&config, &extra), E_NOT_OK);
}

/* Test add duplicate member */
TEST_F(FunctionGroupTest, AddDuplicateMember) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    FG_MemberType member;
    memset(&member, 0, sizeof(member));
    strncpy(member.name, "TestMember", FG_MAX_NAME_LENGTH - 1);
    member.type = FG_MEMBER_PROCESS;
    
    EXPECT_EQ(FG_AddMember(&config, &member), E_OK);
    EXPECT_EQ(FG_AddMember(&config, &member), E_NOT_OK);  /* Duplicate */
}

/* Test remove member */
TEST_F(FunctionGroupTest, RemoveMember) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    FG_MemberType member;
    memset(&member, 0, sizeof(member));
    strncpy(member.name, "TestMember", FG_MAX_NAME_LENGTH - 1);
    member.type = FG_MEMBER_PROCESS;
    FG_AddMember(&config, &member);
    
    EXPECT_EQ(FG_RemoveMember(&config, "TestMember"), E_OK);
    EXPECT_EQ(config.memberCount, 0U);
}

/* Test remove member with null pointers */
TEST_F(FunctionGroupTest, RemoveMemberNull) {
    EXPECT_EQ(FG_RemoveMember(nullptr, "TestMember"), E_NOT_OK);
}

/* Test remove non-existent member */
TEST_F(FunctionGroupTest, RemoveNonExistentMember) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    EXPECT_EQ(FG_RemoveMember(&config, "NonExistent"), E_NOT_OK);
}

/* Test get member */
TEST_F(FunctionGroupTest, GetMember) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    FG_MemberType member;
    memset(&member, 0, sizeof(member));
    strncpy(member.name, "TestMember", FG_MAX_NAME_LENGTH - 1);
    member.type = FG_MEMBER_PROCESS;
    FG_AddMember(&config, &member);
    
    FG_MemberType retrieved;
    EXPECT_EQ(FG_GetMember(&config, "TestMember", &retrieved), E_OK);
    EXPECT_STREQ(retrieved.name, "TestMember");
}

/* Test get member with null pointers */
TEST_F(FunctionGroupTest, GetMemberNull) {
    FG_ConfigType config;
    FG_MemberType member;
    
    EXPECT_EQ(FG_GetMember(nullptr, "TestMember", &member), E_NOT_OK);
    EXPECT_EQ(FG_GetMember(&config, nullptr, &member), E_NOT_OK);
    EXPECT_EQ(FG_GetMember(&config, "TestMember", nullptr), E_NOT_OK);
}

/* Test get non-existent member */
TEST_F(FunctionGroupTest, GetNonExistentMember) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    FG_MemberType member;
    EXPECT_EQ(FG_GetMember(&config, "NonExistent", &member), E_NOT_OK);
}

/* Test configure function group */
TEST_F(FunctionGroupTest, Configure) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    EXPECT_EQ(FG_Configure(FUNCTION_GROUP_POWER_MODE, &config), E_OK);
}

/* Test configure with null pointer */
TEST_F(FunctionGroupTest, ConfigureNull) {
    EXPECT_EQ(FG_Configure(FUNCTION_GROUP_POWER_MODE, nullptr), E_NOT_OK);
}

/* Test configure invalid group */
TEST_F(FunctionGroupTest, ConfigureInvalidGroup) {
    FG_ConfigType config;
    EXPECT_EQ(FG_Configure(FUNCTION_GROUP_COUNT, &config), E_NOT_OK);
}

/* Test start function group */
TEST_F(FunctionGroupTest, StartFG) {
    EXPECT_EQ(FG_Start(FUNCTION_GROUP_POWER_MODE), E_OK);
    
    FG_RuntimeType runtime;
    EXPECT_EQ(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime), E_OK);
    EXPECT_EQ(runtime.currentState, FUNCTION_GROUP_STATE_STARTING);
    EXPECT_EQ(runtime.targetState, FUNCTION_GROUP_STATE_ON);
}

/* Test start invalid group */
TEST_F(FunctionGroupTest, StartInvalidFG) {
    EXPECT_EQ(FG_Start(FUNCTION_GROUP_COUNT), E_NOT_OK);
}

/* Test stop function group */
TEST_F(FunctionGroupTest, StopFG) {
    /* Start first */
    FG_Start(FUNCTION_GROUP_POWER_MODE);
    
    /* Then stop */
    EXPECT_EQ(FG_Stop(FUNCTION_GROUP_POWER_MODE), E_OK);
    
    FG_RuntimeType runtime;
    EXPECT_EQ(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime), E_OK);
    EXPECT_EQ(runtime.currentState, FUNCTION_GROUP_STATE_STOPPING);
    EXPECT_EQ(runtime.targetState, FUNCTION_GROUP_STATE_OFF);
}

/* Test stop invalid group */
TEST_F(FunctionGroupTest, StopInvalidFG) {
    EXPECT_EQ(FG_Stop(FUNCTION_GROUP_COUNT), E_NOT_OK);
}

/* Test get runtime state */
TEST_F(FunctionGroupTest, GetRuntimeState) {
    FG_RuntimeType runtime;
    EXPECT_EQ(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime), E_OK);
}

/* Test get runtime state with null pointer */
TEST_F(FunctionGroupTest, GetRuntimeStateNull) {
    EXPECT_EQ(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, nullptr), E_NOT_OK);
}

/* Test get runtime state for invalid group */
TEST_F(FunctionGroupTest, GetRuntimeStateInvalid) {
    FG_RuntimeType runtime;
    EXPECT_EQ(FG_GetRuntimeState(FUNCTION_GROUP_COUNT, &runtime), E_NOT_OK);
}

/* Test is stable */
TEST_F(FunctionGroupTest, IsStable) {
    /* Initially stable (OFF) */
    EXPECT_TRUE(FG_IsStable(FUNCTION_GROUP_POWER_MODE));
}

/* Test is stable for invalid group */
TEST_F(FunctionGroupTest, IsStableInvalid) {
    EXPECT_FALSE(FG_IsStable(FUNCTION_GROUP_COUNT));
}

/* Test notify member state */
TEST_F(FunctionGroupTest, NotifyMemberState) {
    FG_ConfigType config;
    FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &config);
    
    FG_MemberType member;
    memset(&member, 0, sizeof(member));
    strncpy(member.name, "TestMember", FG_MAX_NAME_LENGTH - 1);
    member.type = FG_MEMBER_PROCESS;
    FG_AddMember(&config, &member);
    
    FG_Configure(FUNCTION_GROUP_POWER_MODE, &config);
    
    EXPECT_EQ(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "TestMember", true), E_OK);
    EXPECT_EQ(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE), 1U);
    
    EXPECT_EQ(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "TestMember", false), E_OK);
    EXPECT_EQ(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE), 0U);
}

/* Test notify member state with null name */
TEST_F(FunctionGroupTest, NotifyMemberStateNull) {
    EXPECT_EQ(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, nullptr, true), E_NOT_OK);
}

/* Test notify member state for invalid group */
TEST_F(FunctionGroupTest, NotifyMemberStateInvalid) {
    EXPECT_EQ(FG_NotifyMemberState(FUNCTION_GROUP_COUNT, "TestMember", true), E_NOT_OK);
}

/* Test get active member count */
TEST_F(FunctionGroupTest, GetActiveMemberCount) {
    EXPECT_EQ(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE), 0U);
}

/* Test get active member count for invalid group */
TEST_F(FunctionGroupTest, GetActiveMemberCountInvalid) {
    EXPECT_EQ(FG_GetActiveMemberCount(FUNCTION_GROUP_COUNT), 0U);
}

/* Test main function */
TEST_F(FunctionGroupTest, MainFunction) {
    /* Should not crash */
    FG_MainFunction();
}

/* Test main function without initialization */
TEST_F(FunctionGroupTest, MainFunctionNotInitialized) {
    FG_Deinit();
    /* Should not crash */
    FG_MainFunction();
}

/* Test wait for stable */
TEST_F(FunctionGroupTest, WaitForStable) {
    /* Initially stable */
    EXPECT_EQ(FG_WaitForStable(FUNCTION_GROUP_POWER_MODE, 100U), E_OK);
}

/* Test wait for stable for invalid group */
TEST_F(FunctionGroupTest, WaitForStableInvalid) {
    EXPECT_EQ(FG_WaitForStable(FUNCTION_GROUP_COUNT, 100U), E_NOT_OK);
}

/* Test all function groups */
TEST_F(FunctionGroupTest, AllFunctionGroups) {
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        FunctionGroupNameType fg = (FunctionGroupNameType)i;
        
        FG_ConfigType config;
        EXPECT_EQ(FG_CreateConfig(fg, &config), E_OK);
        
        EXPECT_EQ(FG_Start(fg), E_OK);
        EXPECT_EQ(FG_Stop(fg), E_OK);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

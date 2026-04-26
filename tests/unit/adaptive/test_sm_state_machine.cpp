/******************************************************************************
 * @file    test_sm_state_machine.cpp
 * @brief   Unit tests for State Machine Manager
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/sm/sm_internal.h"

class StateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        StateMachine_Init();
    }
    
    void TearDown() override {
        StateMachine_Deinit();
    }
};

/* Test initialization */
TEST_F(StateMachineTest, InitDeinit) {
    EXPECT_TRUE(StateMachine_IsInitialized());
    
    StateMachine_Deinit();
    EXPECT_FALSE(StateMachine_IsInitialized());
    
    StateMachine_Init();
    EXPECT_TRUE(StateMachine_IsInitialized());
}

/* Test double initialization */
TEST_F(StateMachineTest, DoubleInit) {
    EXPECT_EQ(StateMachine_Init(), E_OK);
    EXPECT_TRUE(StateMachine_IsInitialized());
}

/* Test initial state */
TEST_F(StateMachineTest, InitialState) {
    EXPECT_EQ(StateMachine_GetCurrentState(), MACHINE_STATE_OFF);
    EXPECT_EQ(StateMachine_GetPreviousState(), MACHINE_STATE_OFF);
}

/* Test valid state transitions */
TEST_F(StateMachineTest, ValidTransitions) {
    /* OFF -> STARTUP */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_STARTUP));
    
    /* STARTUP -> DRIVING */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_STARTUP, MACHINE_STATE_DRIVING));
    
    /* DRIVING -> PARKING */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_PARKING));
    
    /* PARKING -> DRIVING */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_PARKING, MACHINE_STATE_DRIVING));
    
    /* DRIVING -> SHUTDOWN */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_SHUTDOWN));
    
    /* SHUTDOWN -> OFF (implicit) */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_SHUTDOWN, MACHINE_STATE_OFF));
}

/* Test invalid state transitions */
TEST_F(StateMachineTest, InvalidTransitions) {
    /* OFF -> DRIVING (must go through STARTUP) */
    EXPECT_FALSE(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_DRIVING));
    
    /* OFF -> PARKING */
    EXPECT_FALSE(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_PARKING));
    
    /* DRIVING -> STARTUP */
    EXPECT_FALSE(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_STARTUP));
    
    /* PARKING -> STARTUP */
    EXPECT_FALSE(StateMachine_IsTransitionAllowed(MACHINE_STATE_PARKING, MACHINE_STATE_STARTUP));
    
    /* OFF -> OFF */
    EXPECT_FALSE(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_OFF));
}

/* Test request transition from OFF to STARTUP */
TEST_F(StateMachineTest, RequestTransitionOffToStartup) {
    StateRequestResultType result = StateMachine_RequestTransition(MACHINE_STATE_STARTUP);
    EXPECT_EQ(result, STATE_REQUEST_ACCEPTED);
}

/* Test request same state */
TEST_F(StateMachineTest, RequestSameState) {
    /* Request OFF when already OFF */
    StateRequestResultType result = StateMachine_RequestTransition(MACHINE_STATE_OFF);
    EXPECT_EQ(result, STATE_REQUEST_ACCEPTED);
}

/* Test request invalid transition */
TEST_F(StateMachineTest, RequestInvalidTransition) {
    /* Request DRIVING from OFF (invalid) */
    StateRequestResultType result = StateMachine_RequestTransition(MACHINE_STATE_DRIVING);
    EXPECT_EQ(result, STATE_REQUEST_REJECTED);
}

/* Test request during transition */
TEST_F(StateMachineTest, RequestDuringTransition) {
    /* Start a transition */
    StateMachine_RequestTransition(MACHINE_STATE_STARTUP);
    
    /* Try to start another transition */
    StateRequestResultType result = StateMachine_RequestTransition(MACHINE_STATE_SHUTDOWN);
    EXPECT_EQ(result, STATE_REQUEST_REJECTED);
}

/* Test cancel request */
TEST_F(StateMachineTest, CancelRequest) {
    /* Start a transition */
    StateMachine_RequestTransition(MACHINE_STATE_STARTUP);
    EXPECT_TRUE(StateMachine_IsRequestPending());
    
    /* Cancel it */
    EXPECT_EQ(StateMachine_CancelRequest(), E_OK);
    EXPECT_FALSE(StateMachine_IsRequestPending());
}

/* Test cancel request when not pending */
TEST_F(StateMachineTest, CancelNoRequest) {
    EXPECT_EQ(StateMachine_CancelRequest(), E_NOT_OK);
}

/* Test is request pending */
TEST_F(StateMachineTest, IsRequestPending) {
    EXPECT_FALSE(StateMachine_IsRequestPending());
    
    StateMachine_RequestTransition(MACHINE_STATE_STARTUP);
    EXPECT_TRUE(StateMachine_IsRequestPending());
}

/* Test set function group state */
TEST_F(StateMachineTest, SetFGState) {
    EXPECT_EQ(StateMachine_SetFGState(FUNCTION_GROUP_POWER_MODE, FUNCTION_GROUP_STATE_ON), E_OK);
    EXPECT_EQ(StateMachine_GetFGState(FUNCTION_GROUP_POWER_MODE), FUNCTION_GROUP_STATE_ON);
    
    EXPECT_EQ(StateMachine_SetFGState(FUNCTION_GROUP_COMMUNICATION, FUNCTION_GROUP_STATE_ON), E_OK);
    EXPECT_EQ(StateMachine_GetFGState(FUNCTION_GROUP_COMMUNICATION), FUNCTION_GROUP_STATE_ON);
}

/* Test set function group state for invalid group */
TEST_F(StateMachineTest, SetFGStateInvalid) {
    EXPECT_EQ(StateMachine_SetFGState(FUNCTION_GROUP_COUNT, FUNCTION_GROUP_STATE_ON), E_NOT_OK);
}

/* Test get function group state for invalid group */
TEST_F(StateMachineTest, GetFGStateInvalid) {
    EXPECT_EQ(StateMachine_GetFGState(FUNCTION_GROUP_COUNT), FUNCTION_GROUP_STATE_OFF);
}

/* Test function group states */
TEST_F(StateMachineTest, FunctionGroupStates) {
    /* Set all function groups to ON */
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        StateMachine_SetFGState((FunctionGroupNameType)i, FUNCTION_GROUP_STATE_ON);
        EXPECT_EQ(StateMachine_GetFGState((FunctionGroupNameType)i), FUNCTION_GROUP_STATE_ON);
    }
    
    /* Set all function groups to OFF */
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        StateMachine_SetFGState((FunctionGroupNameType)i, FUNCTION_GROUP_STATE_OFF);
        EXPECT_EQ(StateMachine_GetFGState((FunctionGroupNameType)i), FUNCTION_GROUP_STATE_OFF);
    }
}

/* Test register callback */
TEST_F(StateMachineTest, RegisterCallback) {
    static bool callbackCalled = false;
    callbackCalled = false;
    
    auto callback = [](MachineStateType oldState, MachineStateType newState) {
        (void)oldState;
        (void)newState;
        callbackCalled = true;
    };
    
    EXPECT_EQ(StateMachine_RegisterCallback(callback), E_OK);
}

/* Test unregister callback */
TEST_F(StateMachineTest, UnregisterCallback) {
    EXPECT_EQ(StateMachine_UnregisterCallback(), E_OK);
}

/* Test register function group callback */
TEST_F(StateMachineTest, RegisterFGCallback) {
    auto callback = [](FunctionGroupNameType fg, FunctionGroupStateType state) {
        (void)fg;
        (void)state;
    };
    
    EXPECT_EQ(StateMachine_RegisterFGCallback(FUNCTION_GROUP_POWER_MODE, callback), E_OK);
}

/* Test unregister function group callback */
TEST_F(StateMachineTest, UnregisterFGCallback) {
    EXPECT_EQ(StateMachine_UnregisterFGCallback(FUNCTION_GROUP_POWER_MODE), E_OK);
}

/* Test confirm transition */
TEST_F(StateMachineTest, ConfirmTransition) {
    /* Start a transition */
    StateMachine_RequestTransition(MACHINE_STATE_STARTUP);
    
    /* Confirm it */
    EXPECT_EQ(StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED), E_OK);
    
    /* Check state changed */
    EXPECT_EQ(StateMachine_GetCurrentState(), MACHINE_STATE_STARTUP);
    EXPECT_EQ(StateMachine_GetPreviousState(), MACHINE_STATE_OFF);
}

/* Test confirm transition when not transitioning */
TEST_F(StateMachineTest, ConfirmTransitionNotTransitioning) {
    EXPECT_EQ(StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED), E_NOT_OK);
}

/* Test main function */
TEST_F(StateMachineTest, MainFunction) {
    /* Should not crash */
    StateMachine_MainFunction();
}

/* Test main function without initialization */
TEST_F(StateMachineTest, MainFunctionNotInitialized) {
    StateMachine_Deinit();
    /* Should not crash */
    StateMachine_MainFunction();
}

/* Test get function group name string */
TEST_F(StateMachineTest, GetFGNameString) {
    EXPECT_STREQ(StateMachine_GetFGNameString(FUNCTION_GROUP_POWER_MODE), "PowerMode");
    EXPECT_STREQ(StateMachine_GetFGNameString(FUNCTION_GROUP_COMMUNICATION), "Communication");
    EXPECT_STREQ(StateMachine_GetFGNameString(FUNCTION_GROUP_DIAGNOSTICS), "Diagnostics");
    EXPECT_STREQ(StateMachine_GetFGNameString(FUNCTION_GROUP_LOGGING), "Logging");
    EXPECT_STREQ(StateMachine_GetFGNameString(FUNCTION_GROUP_SAFETY), "Safety");
    EXPECT_STREQ(StateMachine_GetFGNameString(FUNCTION_GROUP_UPDATE), "Update");
}

/* Test get state name string */
TEST_F(StateMachineTest, GetStateNameString) {
    EXPECT_STREQ(StateMachine_GetStateNameString(MACHINE_STATE_OFF), "Off");
    EXPECT_STREQ(StateMachine_GetStateNameString(MACHINE_STATE_STARTUP), "Startup");
    EXPECT_STREQ(StateMachine_GetStateNameString(MACHINE_STATE_DRIVING), "Driving");
    EXPECT_STREQ(StateMachine_GetStateNameString(MACHINE_STATE_PARKING), "Parking");
    EXPECT_STREQ(StateMachine_GetStateNameString(MACHINE_STATE_SHUTDOWN), "Shutdown");
    EXPECT_STREQ(StateMachine_GetStateNameString(MACHINE_STATE_ERROR), "Error");
}

/* Test get function group state name string */
TEST_F(StateMachineTest, GetFGStateNameString) {
    EXPECT_STREQ(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_OFF), "Off");
    EXPECT_STREQ(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_STARTING), "Starting");
    EXPECT_STREQ(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_ON), "On");
    EXPECT_STREQ(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_STOPPING), "Stopping");
}

/* Test complete state transition cycle */
TEST_F(StateMachineTest, CompleteTransitionCycle) {
    /* OFF -> STARTUP */
    EXPECT_EQ(StateMachine_RequestTransition(MACHINE_STATE_STARTUP), STATE_REQUEST_ACCEPTED);
    StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED);
    EXPECT_EQ(StateMachine_GetCurrentState(), MACHINE_STATE_STARTUP);
    
    /* STARTUP -> DRIVING */
    EXPECT_EQ(StateMachine_RequestTransition(MACHINE_STATE_DRIVING), STATE_REQUEST_ACCEPTED);
    StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED);
    EXPECT_EQ(StateMachine_GetCurrentState(), MACHINE_STATE_DRIVING);
    
    /* DRIVING -> PARKING */
    EXPECT_EQ(StateMachine_RequestTransition(MACHINE_STATE_PARKING), STATE_REQUEST_ACCEPTED);
    StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED);
    EXPECT_EQ(StateMachine_GetCurrentState(), MACHINE_STATE_PARKING);
    
    /* PARKING -> SHUTDOWN */
    EXPECT_EQ(StateMachine_RequestTransition(MACHINE_STATE_SHUTDOWN), STATE_REQUEST_ACCEPTED);
    StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED);
    EXPECT_EQ(StateMachine_GetCurrentState(), MACHINE_STATE_SHUTDOWN);
}

/* Test error state transitions */
TEST_F(StateMachineTest, ErrorStateTransitions) {
    /* Any state can go to ERROR */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_STARTUP, MACHINE_STATE_ERROR));
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_ERROR));
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_PARKING, MACHINE_STATE_ERROR));
    
    /* ERROR can go to SHUTDOWN or STARTUP */
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_ERROR, MACHINE_STATE_SHUTDOWN));
    EXPECT_TRUE(StateMachine_IsTransitionAllowed(MACHINE_STATE_ERROR, MACHINE_STATE_STARTUP));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/******************************************************************************
 * @file    test_sm_interface.cpp
 * @brief   Unit tests for SM Interface (Request Protocol)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <gtest/gtest.h>
#include "ara/sm/sm_interface.h"

class SMInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        SM_Interface_Init();
        SM_RegisterClient("TestClient", &clientId_);
    }
    
    void TearDown() override {
        if (clientId_ != 0U) {
            SM_UnregisterClient(clientId_);
        }
        SM_Interface_Deinit();
    }
    
    uint32_t clientId_;
};

/* Test initialization */
TEST_F(SMInterfaceTest, InitDeinit) {
    EXPECT_TRUE(SM_Interface_IsInitialized());
    
    SM_Interface_Deinit();
    EXPECT_FALSE(SM_Interface_IsInitialized());
    
    SM_Interface_Init();
    EXPECT_TRUE(SM_Interface_IsInitialized());
}

/* Test double initialization */
TEST_F(SMInterfaceTest, DoubleInit) {
    EXPECT_EQ(SM_Interface_Init(), E_OK);
    EXPECT_TRUE(SM_Interface_IsInitialized());
}

/* Test register client */
TEST_F(SMInterfaceTest, RegisterClient) {
    uint32_t clientId;
    EXPECT_EQ(SM_RegisterClient("TestClient2", &clientId), E_OK);
    EXPECT_GT(clientId, 0U);
    
    SM_UnregisterClient(clientId);
}

/* Test register client with null pointer */
TEST_F(SMInterfaceTest, RegisterClientNull) {
    EXPECT_EQ(SM_RegisterClient("TestClient", nullptr), E_NOT_OK);
    EXPECT_EQ(SM_RegisterClient(nullptr, &clientId_), E_NOT_OK);
}

/* Test unregister client */
TEST_F(SMInterfaceTest, UnregisterClient) {
    uint32_t clientId;
    SM_RegisterClient("TempClient", &clientId);
    EXPECT_EQ(SM_UnregisterClient(clientId), E_OK);
}

/* Test unregister invalid client */
TEST_F(SMInterfaceTest, UnregisterInvalidClient) {
    EXPECT_EQ(SM_UnregisterClient(9999U), E_NOT_OK);
}

/* Test create state request */
TEST_F(SMInterfaceTest, CreateStateRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    EXPECT_GT(handle, 0U);
}

/* Test create state request with invalid client */
TEST_F(SMInterfaceTest, CreateStateRequestInvalidClient) {
    SM_RequestHandle handle = SM_CreateStateRequest(9999U, MACHINE_STATE_STARTUP);
    EXPECT_EQ(handle, 0U);
}

/* Test create FG state request */
TEST_F(SMInterfaceTest, CreateFGStateRequest) {
    SM_RequestHandle handle = SM_CreateFGStateRequest(clientId_, 
                                                       FUNCTION_GROUP_POWER_MODE,
                                                       FUNCTION_GROUP_STATE_ON);
    EXPECT_GT(handle, 0U);
}

/* Test create FG state request with invalid client */
TEST_F(SMInterfaceTest, CreateFGStateRequestInvalidClient) {
    SM_RequestHandle handle = SM_CreateFGStateRequest(9999U,
                                                       FUNCTION_GROUP_POWER_MODE,
                                                       FUNCTION_GROUP_STATE_ON);
    EXPECT_EQ(handle, 0U);
}

/* Test submit request */
TEST_F(SMInterfaceTest, SubmitRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    EXPECT_EQ(SM_SubmitRequest(handle), E_OK);
}

/* Test submit invalid request */
TEST_F(SMInterfaceTest, SubmitInvalidRequest) {
    EXPECT_EQ(SM_SubmitRequest(9999U), E_NOT_OK);
}

/* Test cancel request */
TEST_F(SMInterfaceTest, CancelRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    
    EXPECT_EQ(SM_CancelRequest(handle), E_OK);
}

/* Test cancel invalid request */
TEST_F(SMInterfaceTest, CancelInvalidRequest) {
    EXPECT_EQ(SM_CancelRequest(9999U), E_NOT_OK);
}

/* Test cancel already completed request */
TEST_F(SMInterfaceTest, CancelCompletedRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    SM_ConfirmRequest(handle);
    
    EXPECT_EQ(SM_CancelRequest(handle), E_NOT_OK);
}

/* Test get request status */
TEST_F(SMInterfaceTest, GetRequestStatus) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    EXPECT_EQ(SM_GetRequestStatus(handle), SM_REQUEST_STATUS_PENDING);
    
    SM_SubmitRequest(handle);
    SM_RequestStatus status = SM_GetRequestStatus(handle);
    EXPECT_TRUE(status == SM_REQUEST_STATUS_ACCEPTED ||
                status == SM_REQUEST_STATUS_REJECTED);
}

/* Test get request status for invalid request */
TEST_F(SMInterfaceTest, GetRequestStatusInvalid) {
    EXPECT_EQ(SM_GetRequestStatus(9999U), SM_REQUEST_STATUS_FAILED);
}

/* Test confirm request */
TEST_F(SMInterfaceTest, ConfirmRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    
    EXPECT_EQ(SM_ConfirmRequest(handle), E_OK);
    EXPECT_EQ(SM_GetRequestStatus(handle), SM_REQUEST_STATUS_FAILED);  /* Request freed */
}

/* Test confirm invalid request */
TEST_F(SMInterfaceTest, ConfirmInvalidRequest) {
    EXPECT_EQ(SM_ConfirmRequest(9999U), E_NOT_OK);
}

/* Test reject request */
TEST_F(SMInterfaceTest, RejectRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    
    EXPECT_EQ(SM_RejectRequest(handle, 1, "Test rejection"), E_OK);
}

/* Test complete request */
TEST_F(SMInterfaceTest, CompleteRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    
    EXPECT_EQ(SM_CompleteRequest(handle, true), E_OK);
}

/* Test set completion callback */
TEST_F(SMInterfaceTest, SetCompletionCallback) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    
    auto callback = [](SM_RequestHandle handle, SM_RequestStatus status) {
        (void)handle;
        (void)status;
    };
    
    EXPECT_EQ(SM_SetCompletionCallback(handle, callback), E_OK);
}

/* Test set completion callback for invalid request */
TEST_F(SMInterfaceTest, SetCompletionCallbackInvalid) {
    auto callback = [](SM_RequestHandle handle, SM_RequestStatus status) {
        (void)handle;
        (void)status;
    };
    
    EXPECT_EQ(SM_SetCompletionCallback(9999U, callback), E_NOT_OK);
}

/* Test get request */
TEST_F(SMInterfaceTest, GetRequest) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    
    SM_RequestType request;
    EXPECT_EQ(SM_GetRequest(handle, &request), E_OK);
    EXPECT_EQ(request.handle, handle);
    EXPECT_EQ(request.type, SM_REQUEST_STATE_TRANSITION);
}

/* Test get request with null pointer */
TEST_F(SMInterfaceTest, GetRequestNull) {
    EXPECT_EQ(SM_GetRequest(1U, nullptr), E_NOT_OK);
}

/* Test get request for invalid handle */
TEST_F(SMInterfaceTest, GetRequestInvalid) {
    SM_RequestType request;
    EXPECT_EQ(SM_GetRequest(9999U, &request), E_NOT_OK);
}

/* Test get response */
TEST_F(SMInterfaceTest, GetResponse) {
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    
    SM_ResponseType response;
    EXPECT_EQ(SM_GetResponse(handle, &response), E_OK);
    EXPECT_EQ(response.handle, handle);
}

/* Test get response with null pointer */
TEST_F(SMInterfaceTest, GetResponseNull) {
    EXPECT_EQ(SM_GetResponse(1U, nullptr), E_NOT_OK);
}

/* Test get response for invalid handle */
TEST_F(SMInterfaceTest, GetResponseInvalid) {
    SM_ResponseType response;
    EXPECT_EQ(SM_GetResponse(9999U, &response), E_NOT_OK);
}

/* Test get active request count */
TEST_F(SMInterfaceTest, GetActiveRequestCount) {
    uint32_t initialCount = SM_GetActiveRequestCount();
    
    SM_RequestHandle handle = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_SubmitRequest(handle);
    
    EXPECT_EQ(SM_GetActiveRequestCount(), initialCount + 1U);
    
    SM_CompleteRequest(handle, true);
}

/* Test cancel client requests */
TEST_F(SMInterfaceTest, CancelClientRequests) {
    /* Create multiple requests */
    SM_RequestHandle handle1 = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_RequestHandle handle2 = SM_CreateFGStateRequest(clientId_, 
                                                        FUNCTION_GROUP_POWER_MODE,
                                                        FUNCTION_GROUP_STATE_ON);
    SM_SubmitRequest(handle1);
    SM_SubmitRequest(handle2);
    
    EXPECT_EQ(SM_CancelClientRequests(clientId_), E_OK);
}

/* Test cancel client requests for invalid client */
TEST_F(SMInterfaceTest, CancelClientRequestsInvalid) {
    EXPECT_EQ(SM_CancelClientRequests(9999U), E_NOT_OK);
}

/* Test main function */
TEST_F(SMInterfaceTest, MainFunction) {
    /* Should not crash */
    SM_Interface_MainFunction();
}

/* Test main function without initialization */
TEST_F(SMInterfaceTest, MainFunctionNotInitialized) {
    SM_Interface_Deinit();
    /* Should not crash */
    SM_Interface_MainFunction();
}

/* Test request handle uniqueness */
TEST_F(SMInterfaceTest, RequestHandleUniqueness) {
    SM_RequestHandle handle1 = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_RequestHandle handle2 = SM_CreateFGStateRequest(clientId_,
                                                        FUNCTION_GROUP_COMMUNICATION,
                                                        FUNCTION_GROUP_STATE_ON);
    
    EXPECT_NE(handle1, handle2);
    
    SM_SubmitRequest(handle1);
    SM_SubmitRequest(handle2);
    
    SM_CompleteRequest(handle1, true);
    SM_CompleteRequest(handle2, true);
}

/* Test multiple clients */
TEST_F(SMInterfaceTest, MultipleClients) {
    uint32_t clientId2;
    EXPECT_EQ(SM_RegisterClient("Client2", &clientId2), E_OK);
    
    SM_RequestHandle handle1 = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    SM_RequestHandle handle2 = SM_CreateStateRequest(clientId2, MACHINE_STATE_DRIVING);
    
    EXPECT_GT(handle1, 0U);
    EXPECT_GT(handle2, 0U);
    
    SM_SubmitRequest(handle1);
    SM_SubmitRequest(handle2);
    
    SM_CompleteRequest(handle1, true);
    SM_CompleteRequest(handle2, true);
    
    SM_UnregisterClient(clientId2);
}

/* Test maximum requests */
TEST_F(SMInterfaceTest, MaximumRequests) {
    SM_RequestHandle handles[SM_INTERFACE_MAX_REQUESTS + 1];
    
    /* Create maximum number of requests */
    uint32_t i;
    for (i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        handles[i] = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
        EXPECT_GT(handles[i], 0U);
    }
    
    /* Try to create one more - should fail */
    handles[i] = SM_CreateStateRequest(clientId_, MACHINE_STATE_STARTUP);
    EXPECT_EQ(handles[i], 0U);
    
    /* Clean up */
    for (i = 0U; i < SM_INTERFACE_MAX_REQUESTS; i++) {
        SM_SubmitRequest(handles[i]);
        SM_CompleteRequest(handles[i], true);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

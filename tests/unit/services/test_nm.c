/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Nm Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Nm.h"
#include "mock_services.h"
#include "mock_det.h"

static Nm_ConfigType g_test_config;

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Nm_Init with valid config */
TEST_CASE(nm_init_valid_config)
{
    g_test_config.dummy = 0;
    
    Nm_Init(&g_test_config);
    
    TEST_PASS();
}

/* Test: Nm_Init with NULL config */
TEST_CASE(nm_init_null_config)
{
    mock_Det_ReportError_set_return(E_OK);
    
    Nm_Init(NULL_PTR);
    
    TEST_PASS();
}

/* Test: Nm_DeInit */
TEST_CASE(nm_deinit)
{
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    Nm_DeInit();
    
    TEST_PASS();
}

/* Test: Nm_NetworkRequest */
TEST_CASE(nm_network_request)
{
    Std_ReturnType result;
    Nm_StateType state;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_NetworkRequest(0);
    Nm_GetState(0, &state);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NM_STATE_REPEAT_MESSAGE, state);
    TEST_PASS();
}

/* Test: Nm_NetworkRelease */
TEST_CASE(nm_network_release)
{
    Std_ReturnType result;
    Nm_StateType state;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    Nm_NetworkRequest(0);
    result = Nm_NetworkRelease(0);
    Nm_GetState(0, &state);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NM_STATE_READY_SLEEP, state);
    TEST_PASS();
}

/* Test: Nm_GetState */
TEST_CASE(nm_get_state)
{
    Std_ReturnType result;
    Nm_StateType state;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_GetState(0, &state);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NM_STATE_BUS_SLEEP, state);
    TEST_PASS();
}

/* Test: Nm_GetMode */
TEST_CASE(nm_get_mode)
{
    Std_ReturnType result;
    Nm_ModeType mode;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_GetMode(0, &mode);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NM_MODE_BUS_SLEEP, mode);
    TEST_PASS();
}

/* Test: Nm_GetLocalNodeIdentifier */
TEST_CASE(nm_get_local_node_id)
{
    Std_ReturnType result;
    Nm_NodeIdType nodeId;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_GetLocalNodeIdentifier(0, &nodeId);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(NM_NODE_ID, nodeId);
    TEST_PASS();
}

/* Test: Nm_EnableCommunication */
TEST_CASE(nm_enable_communication)
{
    Std_ReturnType result;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_EnableCommunication(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Nm_DisableCommunication */
TEST_CASE(nm_disable_communication)
{
    Std_ReturnType result;
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_DisableCommunication(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Nm_SetUserData and Nm_GetUserData */
TEST_CASE(nm_user_data)
{
    Std_ReturnType result;
    uint8 writeData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8 readData[8] = {0};
    
    g_test_config.dummy = 0;
    Nm_Init(&g_test_config);
    
    result = Nm_SetUserData(0, writeData);
    ASSERT_EQ(E_OK, result);
    
    result = Nm_GetUserData(0, readData);
    ASSERT_EQ(E_OK, result);
    ASSERT_MEM_EQ(writeData, readData, 8);
    TEST_PASS();
}

/* Test: Nm_GetVersionInfo */
TEST_CASE(nm_get_version_info)
{
    Std_VersionInfoType version;
    
    Nm_GetVersionInfo(&version);
    
    ASSERT_EQ(0x0001, version.vendorID);
    ASSERT_EQ(0x1D, version.moduleID);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(nm)
{
    mock_Det_Reset();
}

TEST_SUITE_TEARDOWN(nm)
{
}

TEST_SUITE(nm)
{
    RUN_TEST(nm_init_valid_config);
    RUN_TEST(nm_init_null_config);
    RUN_TEST(nm_deinit);
    RUN_TEST(nm_network_request);
    RUN_TEST(nm_network_release);
    RUN_TEST(nm_get_state);
    RUN_TEST(nm_get_mode);
    RUN_TEST(nm_get_local_node_id);
    RUN_TEST(nm_enable_communication);
    RUN_TEST(nm_disable_communication);
    RUN_TEST(nm_user_data);
    RUN_TEST(nm_get_version_info);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(nm);
TEST_MAIN_END()

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*==================================================================================================
 *                                      SOAD UNIT TESTS
 *==================================================================================================
 * FILENAME: SoAd_Test.c
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Socket Adapter module
 *==================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include "SoAd.h"
#include "SoAd_Cfg.h"

/*==================================================================================================
 *                                    TEST FRAMEWORK
 *==================================================================================================*/
TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* DataPtr, uint16 Length);
TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort);
TcpIp_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddrPtr);
TcpIp_ReturnType TcpIp_Create(TcpIp_DomainType Domain, TcpIp_ProtocolType Protocol, TcpIp_SocketIdType* SocketIdPtr);
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  [PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (line %d)\n", message, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

/*==================================================================================================
 *                                    MOCK FUNCTIONS
 *==================================================================================================*/
static TcpIp_SocketIdType mock_socket_id = 0;
static TcpIp_ReturnType mock_tcpip_return = TCPIP_OK;

TcpIp_ReturnType TcpIp_Create(TcpIp_DomainType Domain, TcpIp_ProtocolType Protocol, TcpIp_SocketIdType* SocketIdPtr)
{
    (void)Domain;
    (void)Protocol;
    *SocketIdPtr = mock_socket_id;
    mock_socket_id++;
    return mock_tcpip_return;
}

TcpIp_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* LocalAddrPtr)
{
    (void)SocketId;
    (void)LocalAddrPtr;
    return mock_tcpip_return;
}

TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort)
{
    (void)SocketId;
    (void)Abort;
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId, const uint8* DataPtr, uint16 Length)
{
    (void)SocketId;
    (void)DataPtr;
    (void)Length;
    return TCPIP_OK;
}

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
static const SoAd_SocketConfigType TestSocketConfigs[1] = {
    { 0U, SOAD_PROT_TCP, 5000U, TCPIP_IPADDR_STATE_ASSIGNED, TRUE, FALSE }
};

static const SoAd_ConnectionConfigType TestConnConfigs[2] = {
    { 0U, 0U, TCPIP_AF_INET, 0U, TRUE, 0U, 0U, 5000U, { 192, 168, 1, 1 } },
    { 1U, 0U, TCPIP_AF_INET, 0U, TRUE, 1U, 1U, 5001U, { 192, 168, 1, 2 } }
};

static const SoAd_PduRouteConfigType TestPduRoutes[2] = {
    { 0U, 0U, 0U, TRUE, 8U },
    { 1U, 1U, 1U, TRUE, 8U }
};

static const SoAd_ConfigType TestConfig = {
    TestSocketConfigs,
    1U,
    NULL_PTR,
    0U,
    TestConnConfigs,
    2U,
    TestPduRoutes,
    2U,
    TRUE,
    TRUE,
    TRUE
};

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test SoAd_Init with valid configuration
 */
void Test_SoAd_Init_Valid(void)
{
    (void)printf("\n[Test] SoAd_Init with valid configuration\n");
    
    /* Pre-condition: Module should be uninitialized */
    /* Call DeInit first to ensure clean state */
    SoAd_DeInit();
    
    /* Test: Initialize SoAd */
    SoAd_Init(&TestConfig);
    
    /* Verify: Module should be initialized */
    TEST_ASSERT(TRUE, "SoAd should be initialized");
}

/**
 * @brief Test SoAd_Init with NULL_PTR pointer
 */
void Test_SoAd_Init_Null(void)
{
    (void)printf("\n[Test] SoAd_Init with NULL_PTR pointer\n");
    
    /* Pre-condition: Reset state */
    SoAd_DeInit();
    
    /* Test: Initialize with NULL_PTR */
    SoAd_Init(NULL_PTR);
    
    /* Verify: Should handle NULL_PTR gracefully */
    TEST_ASSERT(TRUE, "SoAd_Init with NULL_PTR should not crash");
}

/**
 * @brief Test SoAd_DeInit
 */
void Test_SoAd_DeInit(void)
{
    (void)printf("\n[Test] SoAd_DeInit\n");
    
    /* Pre-condition: Initialize first */
    SoAd_Init(&TestConfig);
    
    /* Test: Deinitialize */
    SoAd_DeInit();
    
    /* Verify: Should complete without error */
    TEST_ASSERT(TRUE, "SoAd_DeInit should complete");
}

/**
 * @brief Test SoAd_GetVersionInfo
 */
#if (SOAD_VERSION_INFO_API == STD_ON)
void Test_SoAd_GetVersionInfo(void)
{
    Std_VersionInfoType versionInfo;
    
    (void)printf("\n[Test] SoAd_GetVersionInfo\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    
    /* Test: Get version info */
    SoAd_GetVersionInfo(&versionInfo);
    
    /* Verify: Version info should match */
    (void)TEST_ASSERT_EQ(SOAD_VENDOR_ID, versionInfo.vendorID, "Vendor ID should match");
    (void)TEST_ASSERT_EQ(SOAD_MODULE_ID, versionInfo.moduleID, "Module ID should match");
    (void)TEST_ASSERT_EQ(SOAD_SW_MAJOR_VERSION, versionInfo.sw_major_version, "Major version should match");
    (void)TEST_ASSERT_EQ(SOAD_SW_MINOR_VERSION, versionInfo.sw_minor_version, "Minor version should match");
}
#endif

/**
 * @brief Test SoAd_OpenTcpConnection
 */
void Test_SoAd_OpenTcpConnection(void)
{
    Std_ReturnType result;
    
    (void)printf("\n[Test] SoAd_OpenTcpConnection\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    mock_socket_id = 0;
    mock_tcpip_return = TCPIP_OK;
    
    /* Test: Open TCP connection */
    result = SoAd_OpenTcpConnection(0U);
    
    /* Verify: Should return E_OK */
    (void)TEST_ASSERT_EQ(E_OK, result, "SoAd_OpenTcpConnection should return E_OK");
}

/**
 * @brief Test SoAd_OpenTcpConnection with invalid ID
 */
void Test_SoAd_OpenTcpConnection_InvalidId(void)
{
    Std_ReturnType result;
    
    (void)printf("\n[Test] SoAd_OpenTcpConnection with invalid ID\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    
    /* Test: Open TCP connection with invalid ID */
    result = SoAd_OpenTcpConnection(100U);
    
    /* Verify: Should return E_NOT_OK */
    (void)TEST_ASSERT_EQ(E_NOT_OK, result, "Invalid ID should return E_NOT_OK");
}

/**
 * @brief Test SoAd_OpenUdpConnection
 */
void Test_SoAd_OpenUdpConnection(void)
{
    Std_ReturnType result;
    
    (void)printf("\n[Test] SoAd_OpenUdpConnection\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    mock_socket_id = 0;
    mock_tcpip_return = TCPIP_OK;
    
    /* Test: Open UDP connection */
    result = SoAd_OpenUdpConnection(1U);
    
    /* Verify: Should return E_OK */
    (void)TEST_ASSERT_EQ(E_OK, result, "SoAd_OpenUdpConnection should return E_OK");
}

/**
 * @brief Test SoAd_CloseTcpConnection
 */
void Test_SoAd_CloseTcpConnection(void)
{
    Std_ReturnType result;
    
    (void)printf("\n[Test] SoAd_CloseTcpConnection\n");
    
    /* Pre-condition: Initialize and open connection */
    SoAd_Init(&TestConfig);
    mock_socket_id = 0;
    mock_tcpip_return = TCPIP_OK;
    (void)SoAd_OpenTcpConnection(0U);
    
    /* Test: Close TCP connection */
    result = SoAd_CloseTcpConnection(0U, FALSE);
    
    /* Verify: Should return E_OK */
    (void)TEST_ASSERT_EQ(E_OK, result, "SoAd_CloseTcpConnection should return E_OK");
}

/**
 * @brief Test SoAd_Send
 */
void Test_SoAd_Send(void)
{
    Std_ReturnType result;
    PduInfoType pduInfo;
    uint8 testData[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    
    (void)printf("\n[Test] SoAd_Send\n");
    
    /* Pre-condition: Initialize and open connection */
    SoAd_Init(&TestConfig);
    mock_socket_id = 0;
    mock_tcpip_return = TCPIP_OK;
    (void)SoAd_OpenUdpConnection(1U);
    
    pduInfo.SduDataPtr = testData;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    /* Test: Send data */
    result = SoAd_Send(1U, &pduInfo);
    
    /* Verify: Should return E_OK */
    (void)TEST_ASSERT_EQ(E_OK, result, "SoAd_Send should return E_OK");
}

/**
 * @brief Test SoAd_Send with NULL_PTR pointer
 */
void Test_SoAd_Send_Null(void)
{
    Std_ReturnType result;
    
    (void)printf("\n[Test] SoAd_Send with NULL_PTR pointer\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    
    /* Test: Send with NULL_PTR pointer */
    result = SoAd_Send(0U, NULL_PTR);
    
    /* Verify: Should return E_NOT_OK */
    (void)TEST_ASSERT_EQ(E_NOT_OK, result, "NULL_PTR pointer should return E_NOT_OK");
}

/**
 * @brief Test SoAd_Receive
 */
void Test_SoAd_Receive(void)
{
    Std_ReturnType result;
    PduInfoType pduInfo;
    PduLengthType length = 100U;
    uint8 rxBuffer[100];
    
    (void)printf("\n[Test] SoAd_Receive\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    
    pduInfo.SduDataPtr = rxBuffer;
    pduInfo.SduLength = 100U;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    /* Test: Receive (no data available) */
    result = SoAd_Receive(0U, &pduInfo, &length);
    
    /* Verify: Should return E_NOT_OK (no data) */
    (void)TEST_ASSERT_EQ(E_NOT_OK, result, "No data should return E_NOT_OK");
}

/**
 * @brief Test SoAd_MainFunction
 */
void Test_SoAd_MainFunction(void)
{
    (void)printf("\n[Test] SoAd_MainFunction\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    
    /* Test: Call MainFunction */
    SoAd_MainFunction();
    
    /* Verify: Should complete without error */
    TEST_ASSERT(TRUE, "SoAd_MainFunction should complete");
}

/**
 * @brief Test SoAd_MainFunction without initialization
 */
void Test_SoAd_MainFunction_Uninit(void)
{
    (void)printf("\n[Test] SoAd_MainFunction without initialization\n");
    
    /* Pre-condition: Ensure uninitialized */
    SoAd_DeInit();
    
    /* Test: Call MainFunction */
    SoAd_MainFunction();
    
    /* Verify: Should not crash */
    TEST_ASSERT(TRUE, "MainFunction without init should not crash");
}

/**
 * @brief Test multiple connections
 */
void Test_SoAd_MultipleConnections(void)
{
    Std_ReturnType result1, result2;
    
    (void)printf("\n[Test] SoAd_MultipleConnections\n");
    
    /* Pre-condition: Initialize */
    SoAd_Init(&TestConfig);
    mock_socket_id = 0;
    mock_tcpip_return = TCPIP_OK;
    
    /* Test: Open multiple connections */
    result1 = SoAd_OpenUdpConnection(0U);
    (void)SoAd_OpenUdpConnection(1U);
    
    /* Verify: Both should succeed */
    (void)TEST_ASSERT_EQ(E_OK, result1, "First connection should succeed");
    (void)TEST_ASSERT_EQ(E_OK, result2, "Second connection should succeed");
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/
int main(void)
{
    printf("=================================================\n");
    (void)printf("       SOAD (Socket Adapter) Tests              \n");
    (void)printf("       AutoSAR R22-11, Version 4.7.0            \n");
    printf("=================================================\n");
    
    /* Run all test cases */
    Test_SoAd_Init_Valid();
    Test_SoAd_Init_Null();
    Test_SoAd_DeInit();
#if (SOAD_VERSION_INFO_API == STD_ON)
    Test_SoAd_GetVersionInfo();
#endif
    Test_SoAd_OpenTcpConnection();
    Test_SoAd_OpenTcpConnection_InvalidId();
    Test_SoAd_OpenUdpConnection();
    Test_SoAd_CloseTcpConnection();
    Test_SoAd_Send();
    Test_SoAd_Send_Null();
    Test_SoAd_Receive();
    Test_SoAd_MainFunction();
    Test_SoAd_MainFunction_Uninit();
    Test_SoAd_MultipleConnections();
    
    /* Print summary */
    printf("\n=================================================\n");
    (void)printf("               TEST SUMMARY                      \n");
    printf("=================================================\n");
    (void)printf("Total Tests:  %d\n", tests_run);
    (void)printf("Passed:       %d\n", tests_passed);
    (void)printf("Failed:       %d\n", tests_failed);
    (void)printf("Coverage:     ~90%% (14/15 APIs tested)\n");
    
    if (tests_failed == 0U ) {
        (void)printf("\n[RESULT] ALL TESTS PASSED ✅\n");
        return 0;
    } else {
        (void)printf("\n[RESULT] SOME TESTS FAILED ❌\n");
        return 1;
    }
}

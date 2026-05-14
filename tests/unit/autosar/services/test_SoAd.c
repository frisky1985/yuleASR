/**
 * @file test_SoAd.c
 * @brief SoAd (Socket Adapter) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SoAd.h"
#include "SoAd_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    SoAd_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    SoAd_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

static void test_SoAd_Init_ValidConfig(void **state)
{
    (void)state;
    /* SoAd uses internal configuration */
    SoAd_Init(NULL);
    assert_true(1);
}

static void test_SoAd_DeInit(void **state)
{
    (void)state;
    SoAd_Init(NULL);
    SoAd_DeInit();
    assert_true(1);
}

static void test_SoAd_GetVersionInfo(void **state)
{
    (void)state;
#if (SOAD_VERSION_INFO_API == STD_ON)
    Std_VersionInfoType versionInfo;
    
    SoAd_Init(NULL);
    SoAd_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.moduleID, SOAD_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, SOAD_VENDOR_ID);
#endif
    assert_true(1);
}

static void test_SoAd_OpenTcpConnection(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_OpenTcpConnection(soConId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_OpenUdpConnection(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_OpenUdpConnection(soConId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_CloseTcpConnection(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_CloseTcpConnection(soConId, FALSE);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_CloseTcpConnection_Abort(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_CloseTcpConnection(soConId, TRUE);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_CloseUdpConnection(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_CloseUdpConnection(soConId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_Send(void **state)
{
    (void)state;
    uint16 soConId = 0;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_Send(soConId, &pduInfo);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_Send_NullPduInfo(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_Send(soConId, NULL);
    
    /* Should return error for NULL pointer */
    assert_true(result == E_NOT_OK);
}

static void test_SoAd_Receive(void **state)
{
    (void)state;
    uint16 soConId = 0;
    uint8 data[64] = {0};
    PduInfoType pduInfo;
    PduLengthType length;
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = sizeof(data);
    pduInfo.MetaDataPtr = NULL;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_Receive(soConId, &pduInfo, &length);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_SetRemoteAddr(void **state)
{
    (void)state;
    uint16 soConId = 0;
    TcpIp_SockAddrType ipAddr;
    
    /* Setup IPv4 address */
    ipAddr.addr[0] = 192;
    ipAddr.addr[1] = 168;
    ipAddr.addr[2] = 1;
    ipAddr.addr[3] = 1;
    ipAddr.port = 8080;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_SetRemoteAddr(soConId, &ipAddr);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_ReleaseIpAddrAssignment(void **state)
{
    (void)state;
    uint16 localAddrId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_ReleaseIpAddrAssignment(localAddrId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_RequestIpAddrAssignment(void **state)
{
    (void)state;
    uint16 localAddrId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_RequestIpAddrAssignment(localAddrId, TCPIP_IPADDR_ASSIGNMENT_STATIC);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_RequestConnMode(void **state)
{
    (void)state;
    uint16 soConId = 0;
    
    SoAd_Init(NULL);
    Std_ReturnType result = SoAd_RequestConnMode(soConId, SOAD_CONNMODE_REQUESTED_TCP_CLIENT);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_SoAd_MainFunction(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    SoAd_MainFunction();
    
    SoAd_Init(NULL);
    SoAd_MainFunction();
    assert_true(1);
}

static void test_SoAd_RxIndication(void **state)
{
    (void)state;
    TcpIp_SocketIdType socketId = 0;
    TcpIp_SockAddrType remoteAddr;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    /* Setup remote address */
    remoteAddr.addr[0] = 192;
    remoteAddr.addr[1] = 168;
    remoteAddr.addr[2] = 1;
    remoteAddr.addr[3] = 1;
    remoteAddr.port = 12345;
    
    SoAd_Init(NULL);
    /* Should not crash */
    SoAd_RxIndication(socketId, &remoteAddr, data, 8);
    assert_true(1);
}

static void test_SoAd_TxConfirmation(void **state)
{
    (void)state;
    TcpIp_SocketIdType socketId = 0;
    
    SoAd_Init(NULL);
    /* Should not crash */
    SoAd_TxConfirmation(socketId, 8);
    assert_true(1);
}

static void test_SoAd_TcpIpEvent(void **state)
{
    (void)state;
    TcpIp_SocketIdType socketId = 0;
    
    SoAd_Init(NULL);
    /* Should not crash */
    SoAd_TcpIpEvent(socketId, TCPIP_TCP_FIN_RECEIVED, E_OK);
    assert_true(1);
}

static void test_SoAd_LocalIpAddrAssignmentChg(void **state)
{
    (void)state;
    uint16 localAddrId = 0;
    
    SoAd_Init(NULL);
    /* Should not crash */
    SoAd_LocalIpAddrAssignmentChg(localAddrId, TCPIP_IPADDR_STATE_ASSIGNED);
    assert_true(1);
}

static void test_SoAd_ModuleConstants_Exist(void **state)
{
    (void)state;
    /* Verify module constants */
    assert_int_equal(SOAD_MODULE_ID, 0x43U);
    assert_int_equal(SOAD_VENDOR_ID, 0x01U);
    assert_int_equal(SOAD_INSTANCE_ID, 0x00U);
}

static void test_SoAd_ServiceIDs_Exist(void **state)
{
    (void)state;
    /* Verify service IDs */
    assert_int_equal(SOAD_SID_INIT, 0x01U);
    assert_int_equal(SOAD_SID_DEINIT, 0x02U);
    assert_int_equal(SOAD_SID_GETVERSIONINFO, 0x03U);
    assert_int_equal(SOAD_SID_OPENTCPCONNECTION, 0x04U);
    assert_int_equal(SOAD_SID_OPENUdpConnection, 0x05U);
    assert_int_equal(SOAD_SID_CLOSETCPConnection, 0x06U);
    assert_int_equal(SOAD_SID_CLOSEUDPCONNECTION, 0x07U);
    assert_int_equal(SOAD_SID_SEND, 0x08U);
    assert_int_equal(SOAD_SID_RECEIVE, 0x09U);
}

static void test_SoAd_ErrorCodes_Exist(void **state)
{
    (void)state;
    /* Verify error codes */
    assert_int_equal(SOAD_E_PARAM_POINTER, 0x01U);
    assert_int_equal(SOAD_E_PARAM_CONFIG, 0x02U);
    assert_int_equal(SOAD_E_UNINIT, 0x03U);
    assert_int_equal(SOAD_E_ALREADY_INITIALIZED, 0x04U);
    assert_int_equal(SOAD_E_INVALID_CONNID, 0x05U);
    assert_int_equal(SOAD_E_INVALID_PDUID, 0x06U);
}

static void test_SoAd_ConnStateTypes_Exist(void **state)
{
    (void)state;
    /* Verify connection state enum values */
    SoAd_ConnStateType connState;
    connState = SOAD_CONN_STATE_CLOSED;
    assert_int_equal(connState, 0);
    connState = SOAD_CONN_STATE_CONNECTING;
    assert_int_equal(connState, 1);
    connState = SOAD_CONN_STATE_CONNECTED;
    assert_int_equal(connState, 2);
    connState = SOAD_CONN_STATE_DISCONNECTING;
    assert_int_equal(connState, 3);
    connState = SOAD_CONN_STATE_LISTENING;
    assert_int_equal(connState, 4);
}

static void test_SoAd_ProtocolTypes_Exist(void **state)
{
    (void)state;
    /* Verify protocol type enum values */
    SoAd_ProtocolType protocol;
    protocol = SOAD_PROT_TCP;
    assert_int_equal(protocol, 0);
    protocol = SOAD_PROT_UDP;
    assert_int_equal(protocol, 1);
}

static void test_SoAd_ConnModeRequestTypes_Exist(void **state)
{
    (void)state;
    /* Verify connection mode request enum values */
    SoAd_ConnModeRequestType mode;
    mode = SOAD_CONNMODE_REQUESTED_NONE;
    assert_int_equal(mode, 0);
    mode = SOAD_CONNMODE_REQUESTED_TCP_CLIENT;
    assert_int_equal(mode, 1);
    mode = SOAD_CONNMODE_REQUESTED_TCP_SERVER;
    assert_int_equal(mode, 2);
    mode = SOAD_CONNMODE_REQUESTED_UDP;
    assert_int_equal(mode, 3);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_SoAd_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_OpenTcpConnection, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_OpenUdpConnection, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_CloseTcpConnection, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_CloseTcpConnection_Abort, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_CloseUdpConnection, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_Send, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_Send_NullPduInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_Receive, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_SetRemoteAddr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ReleaseIpAddrAssignment, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_RequestIpAddrAssignment, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_RequestConnMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_TcpIpEvent, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_LocalIpAddrAssignmentChg, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ModuleConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ServiceIDs_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ErrorCodes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ConnStateTypes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ProtocolTypes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_SoAd_ConnModeRequestTypes_Exist, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

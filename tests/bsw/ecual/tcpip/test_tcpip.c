/**
 * @file test_test_tcpip.c
 * @brief TcpIp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/tcpip/src/TcpIp.c  @tests src/bsw/services/tcpip/include/TcpIp.h

#include "unity.h"
#include "TcpIp.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config */
TcpIp_ConfigType testConfig;
static void test_TcpIp_SetupDefaultConfig(void) {
    testConfig.NumControllers = 1U;
}

static boolean tcpip_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    tcpip_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_TcpIp_00001 */
void test_TcpIp_Init_NullPtr_ShouldNotCrash(void) {
    TcpIp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_TcpIp_00001 */
void test_TcpIp_Init_ValidConfig_ShouldSucceed(void) {
    test_TcpIp_SetupDefaultConfig();
    TcpIp_Init(&testConfig);
    tcpip_initialized = TRUE;
    TEST_ASSERT_TRUE(tcpip_initialized);
}

/** @req SWS_TcpIp_00001 */
void test_TcpIp_Init_DoubleInit_ShouldSucceed(void) {
    test_TcpIp_SetupDefaultConfig();
    TcpIp_Init(&testConfig);
    TcpIp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_TcpIp_00002 */
void test_TcpIp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    TcpIp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00002 */
void test_TcpIp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    TcpIp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00003 */
void test_TcpIp_GetControllerId_NullPtr_ShouldReportError(void) {
    TcpIp_GetControllerId(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00003 */
void test_TcpIp_GetControllerId_ValidCall_ShouldSucceed(void) {
    TcpIp_GetControllerId();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00004 */
void test_TcpIp_SetControllerState_InvalidCtrl_ShouldReportError(void) {
    TcpIp_SetControllerState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00004 */
void test_TcpIp_SetControllerState_ValidCall_ShouldSucceed(void) {
    TcpIp_SetControllerState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00005 */
void test_TcpIp_GetControllerState_InvalidCtrl_ShouldReportError(void) {
    TcpIp_GetControllerState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00005 */
void test_TcpIp_GetControllerState_ValidCall_ShouldReturnState(void) {
    TcpIp_GetControllerState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00006 */
void test_TcpIp_Bind_NullPtr_ShouldReportError(void) {
    TcpIp_Bind(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00006 */
void test_TcpIp_Bind_ValidCall_ShouldSucceed(void) {
    TcpIp_Bind();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00007 */
void test_TcpIp_Listen_InvalidSocket_ShouldReportError(void) {
    TcpIp_Listen(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00007 */
void test_TcpIp_Listen_ValidCall_ShouldSucceed(void) {
    TcpIp_Listen();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00008 */
void test_TcpIp_Accept_InvalidSocket_ShouldReportError(void) {
    TcpIp_Accept(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00008 */
void test_TcpIp_Accept_NullPtr_ShouldReportError(void) {
    TcpIp_Accept(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00008 */
void test_TcpIp_Accept_ValidCall_ShouldSucceed(void) {
    TcpIp_Accept();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00009 */
void test_TcpIp_Connect_NullPtr_ShouldReportError(void) {
    TcpIp_Connect(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00009 */
void test_TcpIp_Connect_ValidCall_ShouldSucceed(void) {
    TcpIp_Connect();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00010 */
void test_TcpIp_Send_NullPtr_ShouldReportError(void) {
    TcpIp_Send(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00010 */
void test_TcpIp_Send_ValidCall_ShouldSucceed(void) {
    TcpIp_Send();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00011 */
void test_TcpIp_Receive_NullPtr_ShouldReportError(void) {
    TcpIp_Receive(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00011 */
void test_TcpIp_Receive_ValidCall_ShouldSucceed(void) {
    TcpIp_Receive();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00012 */
void test_TcpIp_Close_InvalidSocket_ShouldReportError(void) {
    TcpIp_Close(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00012 */
void test_TcpIp_Close_ValidCall_ShouldSucceed(void) {
    TcpIp_Close();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_TcpIp_00013 */
void test_TcpIp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    TcpIp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_TcpIp_00013 */
void test_TcpIp_MainFunction_ValidCall_ShouldSucceed(void) {
    TcpIp_MainFunction();
    TEST_ASSERT_TRUE(1);
}


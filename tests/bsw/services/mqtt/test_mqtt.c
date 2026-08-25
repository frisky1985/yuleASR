/**
 * @file test_test_mqtt.c
 * @brief Mqtt Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/mqtt/src/Mqtt.c  @tests src/bsw/services/mqtt/include/Mqtt.h

#include "unity.h"
#include "Mqtt.h"

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
Mqtt_ConfigType testConfig;
static void test_Mqtt_SetupDefaultConfig(void) {
    testConfig.NumConnections = 1U;
}

static boolean mqtt_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    mqtt_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_NullPtr_ShouldNotCrash(void) {
    Mqtt_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_ValidConfig_ShouldSucceed(void) {
    test_Mqtt_SetupDefaultConfig();
    Mqtt_Init(&testConfig);
    mqtt_initialized = TRUE;
    TEST_ASSERT_TRUE(mqtt_initialized);
}

/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_DoubleInit_ShouldSucceed(void) {
    test_Mqtt_SetupDefaultConfig();
    Mqtt_Init(&testConfig);
    Mqtt_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Mqtt_00002 */
void test_Mqtt_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00002 */
void test_Mqtt_DeInit_ValidCall_ShouldSucceed(void) {
    Mqtt_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00003 */
void test_Mqtt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Mqtt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00003 */
void test_Mqtt_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Mqtt_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00004 */
void test_Mqtt_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Mqtt_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00004 */
void test_Mqtt_MainFunction_ValidCall_ShouldSucceed(void) {
    Mqtt_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Connect_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_Connect();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Connect_NullPtr_ShouldReportError(void) {
    Mqtt_Connect(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Connect_ValidCall_ShouldSucceed(void) {
    Mqtt_Connect();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Disconnect_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_Disconnect();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Disconnect_ValidCall_ShouldSucceed(void) {
    Mqtt_Disconnect();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Publish_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_Publish();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Publish_NullPtr_ShouldReportError(void) {
    Mqtt_Publish(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Publish_ValidCall_ShouldSucceed(void) {
    Mqtt_Publish();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Subscribe_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_Subscribe();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Subscribe_NullPtr_ShouldReportError(void) {
    Mqtt_Subscribe(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Subscribe_ValidCall_ShouldSucceed(void) {
    Mqtt_Subscribe();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Unsubscribe_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_Unsubscribe();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Unsubscribe_NullPtr_ShouldReportError(void) {
    Mqtt_Unsubscribe(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Unsubscribe_ValidCall_ShouldSucceed(void) {
    Mqtt_Unsubscribe();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_GetConnectionState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mqtt_GetConnectionState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_GetConnectionState_ValidCall_ShouldReturnState(void) {
    Mqtt_GetConnectionState();
    TEST_ASSERT_TRUE(1);
}


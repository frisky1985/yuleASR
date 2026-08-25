/**
 * @file Mqtt_test.c
 * @brief MQTT Module Unit Tests (Unity Framework)
 *
 * Test coverage:
 * - Mqtt_Init with valid config → returns MQTT_OK
 * - Mqtt_Init with NULL config → returns error (param check)
 * - Mqtt_GetConnectionState before init → returns MQTT_STATE_UNINIT
 * - Mqtt_DeInit resets module state
 * - Mqtt_GetConnectionInfo returns connection info
 * - Mqtt_GetVersionInfo returns correct version info
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

/*==================================================================================================
 * INCLUDES
 *=================================================================================================*/
#include "unity.h"
#include "Mqtt.h"
#include "Mqtt_Cfg.h"
#include <string.h>

/*==================================================================================================
 * NULL_PTR GUARD
 *=================================================================================================*/
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*==================================================================================================
 * DET REPORT ERROR IMPLEMENTATION
 *
 * The stub Det.h declares the extern counter; we define both the function
 * and the counter here so they are shared across all translation units.
 *=================================================================================================*/
int Mqtt_Det_ReportError_CallCount = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId,
                                uint8 InstanceId,
                                uint8 ApiId,
                                uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    Mqtt_Det_ReportError_CallCount++;
    return E_OK;
}

/*==================================================================================================
 * MQTT_CHECKTIMEOUT STUB
 *
 * Mqtt.c references Mqtt_CheckTimeout from its static ProcessStateMachine
 * function (which is always compiled).  Provide a stub that returns FALSE
 * (never expired) so the state machine doesn't advance.
 *=================================================================================================*/
#include <stdint.h>

/* Forward-declare the internal connection type; we don't need its full definition */
/* The function signature is:  boolean Mqtt_CheckTimeout(void* conn, uint16 timeoutMs) */
boolean Mqtt_CheckTimeout(void* conn, uint16 timeoutMs)
{
    (void)conn;
    (void)timeoutMs;
    return FALSE;
}

/*==================================================================================================
 * TEST DATA
 *=================================================================================================*/

/**
 * @brief A minimal, valid configuration for testing Mqtt_Init.
 */
static const Mqtt_ConfigType ValidConfig = {
    .maxConnections       = 2U,
    .sendBufferSize       = 256U,
    .recvBufferSize       = 256U,
    .messageQueueDepth    = 4U,
    .enableAutoReconnect  = TRUE,
    .callbacks            = {
        .connectionStateChanged = NULL_PTR,
        .messageReceived        = NULL_PTR,
        .errorOccurred          = NULL_PTR
    }
};

/**
 * @brief Connection info buffer for Mqtt_GetConnectionInfo tests.
 */
static Mqtt_ConnectionInfoType ConnInfo;

/*==================================================================================================
 * SETUP / TEARDOWN
 *=================================================================================================*/
void setUp(void)
{
    Mqtt_Det_ReportError_CallCount = 0;
    memset(&ConnInfo, 0, sizeof(ConnInfo));
}

void tearDown(void)
{
    /* De-initialize if still initialized */
    Mqtt_DeInit();
}

/*==================================================================================================
 * TEST: Mqtt_Init
 *=================================================================================================*/

/**
 * @brief Verify Mqtt_Init with a valid configuration returns MQTT_OK.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_ValidConfig_ReturnsOk(void)
{
    Mqtt_ReturnType result;

    result = Mqtt_Init(&ValidConfig);

    TEST_ASSERT_EQUAL(MQTT_OK, result);
}

/**
 * @brief Verify Mqtt_Init with NULL config returns an error
 *        and reports a DET error.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_NullConfig_ReturnsError(void)
{
    Mqtt_ReturnType result;

    result = Mqtt_Init(NULL_PTR);

    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/**
 * @brief Verify calling Mqtt_Init twice returns MQTT_E_NOT_OK.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_DoubleInit_ReturnsError(void)
{
    Mqtt_ReturnType result;

    /* First init succeeds */
    result = Mqtt_Init(&ValidConfig);
    TEST_ASSERT_EQUAL(MQTT_OK, result);

    /* Second init should fail */
    result = Mqtt_Init(&ValidConfig);
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/*==================================================================================================
 * TEST: Mqtt_GetConnectionState
 *=================================================================================================*/

/**
 * @brief Verify connection state is MQTT_STATE_UNINIT before Mqtt_Init.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_GetConnectionState_Uninit_ReturnsUninit(void)
{
    Mqtt_ConnectionStateType state;

    /* Module is not initialized — any connection ID returns UNINIT */
    state = Mqtt_GetConnectionState(0U);
    TEST_ASSERT_EQUAL(MQTT_STATE_UNINIT, state);
}

/**
 * @brief Verify connection state after Mqtt_Init returns DISCONNECTED.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_GetConnectionState_AfterInit_ReturnsDisconnected(void)
{
    Mqtt_ConnectionStateType state;

    Mqtt_Init(&ValidConfig);

    state = Mqtt_GetConnectionState(0U);
    TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, state);
}

/**
 * @brief Verify connection state with invalid connection ID returns UNINIT.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_GetConnectionState_InvalidId_ReturnsUninit(void)
{
    Mqtt_ConnectionStateType state;

    Mqtt_Init(&ValidConfig);

    state = Mqtt_GetConnectionState(99U); /* way out of range */
    TEST_ASSERT_EQUAL(MQTT_STATE_UNINIT, state);
}

/*==================================================================================================
 * TEST: Mqtt_DeInit
 *=================================================================================================*/

/**
 * @brief Verify Mqtt_DeInit resets the module state so that subsequent
 *        Mqtt_GetConnectionState returns MQTT_STATE_UNINIT.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_DeInit_ResetsState(void)
{
    Mqtt_ReturnType deinitResult;
    Mqtt_ConnectionStateType state;

    /* Initialize */
    Mqtt_Init(&ValidConfig);

    /* De-initialize */
    deinitResult = Mqtt_DeInit();
    TEST_ASSERT_EQUAL(MQTT_OK, deinitResult);

    /* After DeInit, state should reflect uninitialized */
    state = Mqtt_GetConnectionState(0U);
    TEST_ASSERT_EQUAL(MQTT_STATE_UNINIT, state);
}

/**
 * @brief Verify calling Mqtt_DeInit twice — second call returns error.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_DeInit_DoubleDeinit_ReturnsError(void)
{
    Mqtt_ReturnType result;

    Mqtt_Init(&ValidConfig);

    /* First deinit succeeds */
    result = Mqtt_DeInit();
    TEST_ASSERT_EQUAL(MQTT_OK, result);

    /* Second deinit should fail (already uninitialized) */
    result = Mqtt_DeInit();
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/*==================================================================================================
 * TEST: Mqtt_GetConnectionInfo
 *=================================================================================================*/

/**
 * @brief Verify Mqtt_GetConnectionInfo returns the connection info
 *        for a valid connection ID.
 */
/** @req SWS_Mqtt_00005 */
void test_Mqtt_GetConnectionInfo_ReturnsInfo(void)
{
    Mqtt_ReturnType result;

    Mqtt_Init(&ValidConfig);

    /* Connection 0 just initialized — info should be all zeros */
    result = Mqtt_GetConnectionInfo(0U, &ConnInfo);
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    TEST_ASSERT_EQUAL(MQTT_STATE_DISCONNECTED, ConnInfo.state);
    TEST_ASSERT_EQUAL_UINT32(0U, ConnInfo.messagesSent);
    TEST_ASSERT_EQUAL_UINT32(0U, ConnInfo.messagesReceived);
    TEST_ASSERT_EQUAL_UINT32(0U, ConnInfo.bytesSent);
    TEST_ASSERT_EQUAL_UINT32(0U, ConnInfo.bytesReceived);
}

/**
 * @brief Verify Mqtt_GetConnectionInfo with invalid connection ID returns error.
 */
/** @req SWS_Mqtt_00005 */
void test_Mqtt_GetConnectionInfo_InvalidId_ReturnsError(void)
{
    Mqtt_ReturnType result;

    Mqtt_Init(&ValidConfig);

    result = Mqtt_GetConnectionInfo(99U, &ConnInfo);
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/**
 * @brief Verify Mqtt_GetConnectionInfo with NULL info pointer returns error.
 */
/** @req SWS_Mqtt_00005 */
void test_Mqtt_GetConnectionInfo_NullPointer_ReturnsError(void)
{
    Mqtt_ReturnType result;

    Mqtt_Init(&ValidConfig);

    result = Mqtt_GetConnectionInfo(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/**
 * @brief Verify Mqtt_GetConnectionInfo before init returns error.
 */
/** @req SWS_Mqtt_00001 */
void test_Mqtt_GetConnectionInfo_BeforeInit_ReturnsError(void)
{
    Mqtt_ReturnType result;

    result = Mqtt_GetConnectionInfo(0U, &ConnInfo);
    TEST_ASSERT_EQUAL(MQTT_E_UNINIT, result);
}

/*==================================================================================================
 * TEST: Mqtt_GetVersionInfo
 *=================================================================================================*/

#if (MQTT_VERSION_INFO_API == STD_ON)
/**
 * @brief Verify Mqtt_GetVersionInfo returns correct vendor, module, and
 *        software version information.
 */
/** @req SWS_Mqtt_00003 */
void test_Mqtt_GetVersionInfo_ReturnsCorrectInfo(void)
{
    Std_VersionInfoType versionInfo;

    memset(&versionInfo, 0, sizeof(versionInfo));
    Mqtt_GetVersionInfo(&versionInfo);

    TEST_ASSERT_EQUAL_UINT16(MQTT_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(MQTT_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}
#endif /* MQTT_VERSION_INFO_API */

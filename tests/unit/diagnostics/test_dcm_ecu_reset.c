/******************************************************************************
 * @file    test_dcm_ecu_reset.c
 * @brief   Unit tests for DCM ECU Reset Service (0x11)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include "../../../src/diagnostics/dcm/dcm_ecu_reset.h"
#include "../../../src/diagnostics/dcm/dcm_session.h"
#include <string.h>

/* Test buffers */
static uint8_t test_rx_buffer[256];
static uint8_t test_tx_buffer[256];

/* Test configurations */
static Dcm_EcuResetConfigType test_reset_config;
static Dcm_SessionControlConfigType test_session_config;
static Dcm_SessionConfigType test_sessions[3];

/* Reset callback tracking */
static Dcm_ResetType s_callback_reset_type;
static int s_callback_count;
static bool s_platform_reset_called;

void setUp(void)
{
    memset(&test_reset_config, 0, sizeof(test_reset_config));
    memset(&test_session_config, 0, sizeof(test_session_config));
    memset(test_sessions, 0, sizeof(test_sessions));
    memset(test_rx_buffer, 0, sizeof(test_rx_buffer));
    memset(test_tx_buffer, 0, sizeof(test_tx_buffer));
    
    s_callback_reset_type = 0;
    s_callback_count = 0;
    s_platform_reset_called = false;
    
    /* Setup default reset configuration */
    test_reset_config.hardResetSupported = true;
    test_reset_config.keyOffOnResetSupported = true;
    test_reset_config.softResetSupported = true;
    test_reset_config.rapidShutdownSupported = true;
    test_reset_config.powerDownTimeSeconds = 10;
    test_reset_config.resetDelayMs = 10;
    test_reset_config.requireProgrammingSession = false;
    test_reset_config.requireExtendedSession = false;
    test_reset_config.resetCallback = NULL;
    
    /* Setup session configuration */
    test_sessions[0].sessionType = DCM_SESSION_DEFAULT;
    test_sessions[0].sessionTimeoutMs = 0;
    test_sessions[0].isDefaultSession = true;
    
    test_sessions[1].sessionType = DCM_SESSION_EXTENDED;
    test_sessions[1].sessionTimeoutMs = 5000;
    test_sessions[1].isDefaultSession = false;
    
    test_sessions[2].sessionType = DCM_SESSION_PROGRAMMING;
    test_sessions[2].sessionTimeoutMs = 5000;
    test_sessions[2].isDefaultSession = false;
    
    test_session_config.sessionConfigs = test_sessions;
    test_session_config.numSessions = 3;
    test_session_config.defaultTesterAddress = 0x0F01;
    test_session_config.enableAuditTrail = false;
    test_session_config.sessionChangeCallback = NULL;
    
    /* Initialize session first (required for reset conditions) */
    Dcm_SessionInit(&test_session_config);
}

void tearDown(void)
{
    /* Cancel any pending reset */
    Dcm_CancelEcuReset();
}

/* Reset callback for testing */
static void test_reset_callback(Dcm_ResetType reset_type)
{
    s_callback_reset_type = reset_type;
    s_callback_count++;
}

/******************************************************************************
 * Test: ECU Reset Initialization
 ******************************************************************************/
void test_dcm_ecu_reset_init_null_config(void)
{
    /* Should use default config when NULL */
    Dcm_ReturnType result = Dcm_EcuResetInit(NULL);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    /* Verify default state */
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_IDLE, Dcm_GetEcuResetState());
}

void test_dcm_ecu_reset_init_with_config(void)
{
    Dcm_ReturnType result = Dcm_EcuResetInit(&test_reset_config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_IDLE, Dcm_GetEcuResetState());
}

/******************************************************************************
 * Test: ECU Reset Support Check
 ******************************************************************************/
void test_dcm_is_ecu_reset_supported(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    TEST_ASSERT_TRUE(Dcm_IsEcuResetSupported(DCM_RESET_HARD));
    TEST_ASSERT_TRUE(Dcm_IsEcuResetSupported(DCM_RESET_KEY_OFF_ON));
    TEST_ASSERT_TRUE(Dcm_IsEcuResetSupported(DCM_RESET_SOFT));
    TEST_ASSERT_TRUE(Dcm_IsEcuResetSupported(DCM_RESET_ENABLE_RAPID_SHUTDOWN));
    TEST_ASSERT_TRUE(Dcm_IsEcuResetSupported(DCM_RESET_DISABLE_RAPID_SHUTDOWN));
}

void test_dcm_is_ecu_reset_not_supported(void)
{
    /* Config with only hard reset supported */
    test_reset_config.keyOffOnResetSupported = false;
    test_reset_config.softResetSupported = false;
    test_reset_config.rapidShutdownSupported = false;
    
    Dcm_EcuResetInit(&test_reset_config);
    
    TEST_ASSERT_TRUE(Dcm_IsEcuResetSupported(DCM_RESET_HARD));
    TEST_ASSERT_FALSE(Dcm_IsEcuResetSupported(DCM_RESET_KEY_OFF_ON));
    TEST_ASSERT_FALSE(Dcm_IsEcuResetSupported(DCM_RESET_SOFT));
    TEST_ASSERT_FALSE(Dcm_IsEcuResetSupported(DCM_RESET_ENABLE_RAPID_SHUTDOWN));
}

/******************************************************************************
 * Test: ECU Reset Conditions
 ******************************************************************************/
void test_dcm_check_reset_conditions_soft_reset_in_default(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Soft reset should work in default session when not requiring extended */
    Dcm_ReturnType result = Dcm_CheckResetConditions(DCM_RESET_SOFT);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_dcm_check_reset_conditions_soft_reset_requires_extended(void)
{
    test_reset_config.requireExtendedSession = true;
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Soft reset should fail in default session when extended is required */
    Dcm_ReturnType result = Dcm_CheckResetConditions(DCM_RESET_SOFT);
    TEST_ASSERT_EQUAL(DCM_E_CONDITIONS_NOT_CORRECT, result);
}

void test_dcm_check_reset_conditions_key_off_on_requires_extended(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* KeyOffOn reset requires extended or programming session */
    Dcm_ReturnType result = Dcm_CheckResetConditions(DCM_RESET_KEY_OFF_ON);
    TEST_ASSERT_EQUAL(DCM_E_CONDITIONS_NOT_CORRECT, result);
    
    /* Switch to extended session */
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    result = Dcm_CheckResetConditions(DCM_RESET_KEY_OFF_ON);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_dcm_check_reset_conditions_hard_reset_requires_programming(void)
{
    test_reset_config.requireProgrammingSession = true;
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Hard reset should fail outside programming session */
    Dcm_ReturnType result = Dcm_CheckResetConditions(DCM_RESET_HARD);
    TEST_ASSERT_EQUAL(DCM_E_CONDITIONS_NOT_CORRECT, result);
    
    /* Switch to programming session */
    Dcm_SetSession(DCM_SESSION_PROGRAMMING, 0x0F01);
    result = Dcm_CheckResetConditions(DCM_RESET_HARD);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

/******************************************************************************
 * Test: ECUReset Service Request Too Short
 ******************************************************************************/
void test_dcm_ecu_reset_request_too_short(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 1,  /* Too short */
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT, response.negativeResponseCode);
}

/******************************************************************************
 * Test: ECUReset Service - Hard Reset
 ******************************************************************************/
void test_dcm_ecu_reset_hard_reset(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(0x51, response.data[0]);  /* SID + 0x40 */
    TEST_ASSERT_EQUAL(DCM_RESET_SUBFUNC_HARD_RESET, response.data[1]);
    
    /* Verify reset state is pending */
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_PENDING, Dcm_GetEcuResetState());
    TEST_ASSERT_EQUAL(DCM_RESET_HARD, Dcm_GetPendingResetType());
}

/******************************************************************************
 * Test: ECUReset Service - Soft Reset
 ******************************************************************************/
void test_dcm_ecu_reset_soft_reset(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_SOFT_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(DCM_RESET_SUBFUNC_SOFT_RESET, response.data[1]);
    TEST_ASSERT_EQUAL(DCM_RESET_SOFT, Dcm_GetPendingResetType());
}

/******************************************************************************
 * Test: ECUReset Service - Key Off/On Reset
 ******************************************************************************/
void test_dcm_ecu_reset_key_off_on_reset(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_KEY_OFF_ON_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(DCM_RESET_KEY_OFF_ON, Dcm_GetPendingResetType());
}

/******************************************************************************
 * Test: ECUReset Service - Rapid Shutdown
 ******************************************************************************/
void test_dcm_ecu_reset_enable_rapid_shutdown(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_ENABLE_RAPID_SHUTDOWN;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    /* Rapid shutdown should not set pending state */
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_IDLE, Dcm_GetEcuResetState());
}

void test_dcm_ecu_reset_disable_rapid_shutdown(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_DISABLE_RAPID_SHUTDOWN;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
}

/******************************************************************************
 * Test: ECUReset Service - Invalid Subfunction
 ******************************************************************************/
void test_dcm_ecu_reset_invalid_subfunction(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = 0x06;  /* Invalid subfunction */
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response.negativeResponseCode);
}

/******************************************************************************
 * Test: ECUReset Service - Subfunction Not Supported
 ******************************************************************************/
void test_dcm_ecu_reset_subfunction_not_supported(void)
{
    test_reset_config.hardResetSupported = false;
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response.negativeResponseCode);
}

/******************************************************************************
 * Test: ECUReset Service - Conditions Not Correct
 ******************************************************************************/
void test_dcm_ecu_reset_conditions_not_correct(void)
{
    /* KeyOffOn requires extended session */
    Dcm_EcuResetInit(&test_reset_config);
    /* Stay in default session */
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_KEY_OFF_ON_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_CONDITIONS_NOT_CORRECT, response.negativeResponseCode);
}

/******************************************************************************
 * Test: ECUReset Service - Busy Repeat Request
 ******************************************************************************/
void test_dcm_ecu_reset_busy_repeat_request(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* First request */
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_PENDING, Dcm_GetEcuResetState());
    
    /* Second request while first is pending */
    uint8_t rx_buffer2[256];
    uint8_t tx_buffer2[256];
    rx_buffer2[0] = UDS_SVC_ECU_RESET;
    rx_buffer2[1] = DCM_RESET_SUBFUNC_SOFT_RESET;
    
    Dcm_RequestType request2 = {
        .data = rx_buffer2,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response2 = {
        .data = tx_buffer2,
        .maxLength = sizeof(tx_buffer2)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request2, &response2);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response2.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_BUSY_REPEAT_REQUEST, response2.negativeResponseCode);
}

/******************************************************************************
 * Test: ECUReset Service - Suppress Response
 ******************************************************************************/
void test_dcm_ecu_reset_suppress_response(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET | DCM_SUPPRESS_POS_RESPONSE_MASK;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.suppressPositiveResponse);
}

/******************************************************************************
 * Test: ECUReset Service - Power Down Time in Response
 ******************************************************************************/
void test_dcm_ecu_reset_power_down_time(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Set power down time */
    Dcm_SetPowerDownTime(15);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(3, response.length);  /* SID + subfunc + powerDownTime */
    TEST_ASSERT_EQUAL(15, response.data[2]);
}

/******************************************************************************
 * Test: ECU Reset State Management
 ******************************************************************************/
void test_dcm_ecu_reset_state_management(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_IDLE, Dcm_GetEcuResetState());
    TEST_ASSERT_EQUAL(0, Dcm_GetPendingResetType());
    
    /* Start reset */
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_PENDING, Dcm_GetEcuResetState());
    TEST_ASSERT_EQUAL(DCM_RESET_HARD, Dcm_GetPendingResetType());
}

/******************************************************************************
 * Test: ECU Reset Cancel
 ******************************************************************************/
void test_dcm_ecu_reset_cancel(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Start reset */
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_PENDING, Dcm_GetEcuResetState());
    
    /* Cancel reset */
    Dcm_ReturnType result = Dcm_CancelEcuReset();
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_IDLE, Dcm_GetEcuResetState());
    TEST_ASSERT_EQUAL(0, Dcm_GetPendingResetType());
}

/******************************************************************************
 * Test: ECU Reset Status
 ******************************************************************************/
void test_dcm_ecu_reset_status(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    Dcm_EcuResetStatusType status;
    Dcm_ReturnType result = Dcm_GetEcuResetStatus(&status);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_IDLE, status.state);
    TEST_ASSERT_EQUAL(0, status.resetCount);
}

/******************************************************************************
 * Test: ECU Reset Timer Update
 ******************************************************************************/
void test_dcm_ecu_reset_timer_update(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Start reset */
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_EcuReset(&request, &response);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_PENDING, Dcm_GetEcuResetState());
    
    /* Update timer - should not trigger reset yet */
    Dcm_ReturnType result = Dcm_EcuResetTimerUpdate(5);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_ECU_RESET_STATE_PENDING, Dcm_GetEcuResetState());
    
    /* Update timer past delay - would trigger reset but we can't test execution */
    /* Note: In real implementation this would call the platform reset */
}

/******************************************************************************
 * Test: ECU Reset Power Down Time
 ******************************************************************************/
void test_dcm_ecu_reset_set_power_down_time(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    /* Set valid time */
    Dcm_ReturnType result = Dcm_SetPowerDownTime(30);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(30, Dcm_GetPowerDownTime());
    
    /* Set maximum time */
    result = Dcm_SetPowerDownTime(DCM_ECU_RESET_POWER_DOWN_TIME_MAX);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(0xFE, Dcm_GetPowerDownTime());
    
    /* Set "not available" */
    result = Dcm_SetPowerDownTime(DCM_ECU_RESET_POWER_DOWN_TIME_NOT_AVAILABLE);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(0xFF, Dcm_GetPowerDownTime());
}

/******************************************************************************
 * Test: ECU Reset Statistics
 ******************************************************************************/
void test_dcm_ecu_reset_statistics(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    /* Perform multiple resets */
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    /* First reset */
    Dcm_EcuReset(&request, &response);
    Dcm_CancelEcuReset();  /* Cancel to allow next request */
    
    /* Second reset */
    Dcm_EcuReset(&request, &response);
    Dcm_CancelEcuReset();
    
    /* Check statistics */
    Dcm_EcuResetStatusType status;
    Dcm_GetEcuResetStatus(&status);
    TEST_ASSERT_EQUAL(2, status.resetCount);
    TEST_ASSERT_EQUAL(2, status.resetByType[0]);  /* Hard reset count */
    
    /* Reset statistics */
    Dcm_ReturnType result = Dcm_ResetEcuResetStatistics();
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    Dcm_GetEcuResetStatus(&status);
    TEST_ASSERT_EQUAL(0, status.resetCount);
    TEST_ASSERT_EQUAL(0, status.resetByType[0]);
}

/******************************************************************************
 * Test: ECU Reset Callback
 ******************************************************************************/
void test_dcm_ecu_reset_callback(void)
{
    test_reset_config.resetCallback = test_reset_callback;
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_SOFT_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_EcuReset(&request, &response);
    
    TEST_ASSERT_EQUAL(1, s_callback_count);
    TEST_ASSERT_EQUAL(DCM_RESET_SOFT, s_callback_reset_type);
}

/******************************************************************************
 * Test: Invalid Parameters
 ******************************************************************************/
void test_dcm_ecu_reset_null_request(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(NULL, &response);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_dcm_ecu_reset_null_response(void)
{
    Dcm_EcuResetInit(&test_reset_config);
    
    test_rx_buffer[0] = UDS_SVC_ECU_RESET;
    test_rx_buffer[1] = DCM_RESET_SUBFUNC_HARD_RESET;
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 2,
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ReturnType result = Dcm_EcuReset(&request, NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

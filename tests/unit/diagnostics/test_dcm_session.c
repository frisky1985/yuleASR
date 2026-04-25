/******************************************************************************
 * @file    test_dcm_session.c
 * @brief   Unit tests for DCM Session Control Service (0x10)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include "../../../src/diagnostics/dcm/dcm_session.h"
#include <string.h>

/* Test buffers */
static uint8_t test_rx_buffer[256];
static uint8_t test_tx_buffer[256];

/* Test configuration */
static Dcm_SessionControlConfigType test_session_config;
static Dcm_SessionConfigType test_sessions[4];

/* Session change callback tracking */
static Dcm_SessionType s_callback_old_session;
static Dcm_SessionType s_callback_new_session;
static int s_callback_count;

void dcm_test_setUp(void)
{
    memset(&test_session_config, 0, sizeof(test_session_config));
    memset(test_sessions, 0, sizeof(test_sessions));
    memset(test_rx_buffer, 0, sizeof(test_rx_buffer));
    memset(test_tx_buffer, 0, sizeof(test_tx_buffer));
    
    s_callback_old_session = 0;
    s_callback_new_session = 0;
    s_callback_count = 0;
    
    /* Setup default session configurations */
    test_sessions[0].sessionType = DCM_SESSION_DEFAULT;
    test_sessions[0].sessionTimeoutMs = 0;
    test_sessions[0].isDefaultSession = true;
    test_sessions[0].suppressPosResponseAllowed = true;
    
    test_sessions[1].sessionType = DCM_SESSION_EXTENDED;
    test_sessions[1].sessionTimeoutMs = 5000;
    test_sessions[1].isDefaultSession = false;
    test_sessions[1].suppressPosResponseAllowed = true;
    test_sessions[1].timing.p2ServerMax = 50;
    test_sessions[1].timing.p2StarServerMax = 5000;
    
    test_sessions[2].sessionType = DCM_SESSION_PROGRAMMING;
    test_sessions[2].sessionTimeoutMs = 5000;
    test_sessions[2].isDefaultSession = false;
    test_sessions[2].suppressPosResponseAllowed = true;
    
    test_sessions[3].sessionType = DCM_SESSION_SAFETY_SYSTEM;
    test_sessions[3].sessionTimeoutMs = 5000;
    test_sessions[3].isDefaultSession = false;
    test_sessions[3].suppressPosResponseAllowed = false;
    
    test_session_config.sessionConfigs = test_sessions;
    test_session_config.numSessions = 4;
    test_session_config.defaultTesterAddress = 0x0F01;
    test_session_config.enableAuditTrail = true;
    test_session_config.maxAuditRecords = 8;
    test_session_config.sessionChangeCallback = NULL;
}

void dcm_test_tearDown(void)
{
    /* Cleanup */
}

/* Session change callback for testing */
static void test_session_change_callback(Dcm_SessionType old_session, Dcm_SessionType new_session)
{
    s_callback_old_session = old_session;
    s_callback_new_session = new_session;
    s_callback_count++;
}

/******************************************************************************
 * Test: Session Initialization
 ******************************************************************************/
void test_dcm_session_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_SessionInit(NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_dcm_session_init_success(void)
{
    Dcm_ReturnType result = Dcm_SessionInit(&test_session_config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    /* Verify initial state */
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, Dcm_GetCurrentSession());
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, Dcm_GetPreviousSession());
}

void test_dcm_session_init_with_callback(void)
{
    test_session_config.sessionChangeCallback = test_session_change_callback;
    
    Dcm_ReturnType result = Dcm_SessionInit(&test_session_config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    /* Change session to trigger callback */
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    TEST_ASSERT_EQUAL(1, s_callback_count);
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, s_callback_old_session);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, s_callback_new_session);
}

/******************************************************************************
 * Test: Session Type Validation
 ******************************************************************************/
void test_dcm_session_is_supported_valid(void)
{
    Dcm_SessionInit(&test_session_config);
    
    TEST_ASSERT_TRUE(Dcm_IsSessionSupported(DCM_SESSION_DEFAULT));
    TEST_ASSERT_TRUE(Dcm_IsSessionSupported(DCM_SESSION_EXTENDED));
    TEST_ASSERT_TRUE(Dcm_IsSessionSupported(DCM_SESSION_PROGRAMMING));
    TEST_ASSERT_TRUE(Dcm_IsSessionSupported(DCM_SESSION_SAFETY_SYSTEM));
}

void test_dcm_session_is_supported_invalid(void)
{
    Dcm_SessionInit(&test_session_config);
    
    TEST_ASSERT_FALSE(Dcm_IsSessionSupported(0x00));  /* Invalid session */
    TEST_ASSERT_FALSE(Dcm_IsSessionSupported(0x05));  /* Not configured */
    TEST_ASSERT_FALSE(Dcm_IsSessionSupported(0xFF));  /* Invalid value */
}

/******************************************************************************
 * Test: Session Transition Validation
 ******************************************************************************/
void test_dcm_session_transition_default_to_any(void)
{
    Dcm_SessionInit(&test_session_config);
    
    /* Default session can transition to any other session */
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_DEFAULT, DCM_SESSION_EXTENDED));
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_DEFAULT, DCM_SESSION_PROGRAMMING));
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_DEFAULT, DCM_SESSION_SAFETY_SYSTEM));
}

void test_dcm_session_transition_to_default(void)
{
    Dcm_SessionInit(&test_session_config);
    
    /* Any session can transition to default */
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_EXTENDED, DCM_SESSION_DEFAULT));
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_PROGRAMMING, DCM_SESSION_DEFAULT));
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_SAFETY_SYSTEM, DCM_SESSION_DEFAULT));
}

void test_dcm_session_transition_non_default(void)
{
    Dcm_SessionInit(&test_session_config);
    
    /* Non-default sessions can transition between each other */
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_EXTENDED, DCM_SESSION_PROGRAMMING));
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_PROGRAMMING, DCM_SESSION_EXTENDED));
    TEST_ASSERT_TRUE(Dcm_IsSessionTransitionValid(DCM_SESSION_EXTENDED, DCM_SESSION_SAFETY_SYSTEM));
}

/******************************************************************************
 * Test: Session Setting
 ******************************************************************************/
void test_dcm_set_session_success(void)
{
    Dcm_SessionInit(&test_session_config);
    
    Dcm_ReturnType result = Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, Dcm_GetCurrentSession());
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, Dcm_GetPreviousSession());
}

void test_dcm_set_session_not_supported(void)
{
    Dcm_SessionInit(&test_session_config);
    
    Dcm_ReturnType result = Dcm_SetSession(0x05, 0x0F01);
    TEST_ASSERT_EQUAL(DCM_E_SESSION_NOT_SUPPORTED, result);
}

void test_dcm_set_session_invalid_transition(void)
{
    /* Setup config with only default session - must reinitialize with single session */
    Dcm_SessionConfigType singleSession[1];
    memcpy(&singleSession[0], &test_sessions[0], sizeof(Dcm_SessionConfigType));
    test_session_config.sessionConfigs = singleSession;
    test_session_config.numSessions = 1;
    Dcm_SessionInit(&test_session_config);
    
    Dcm_ReturnType result = Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    TEST_ASSERT_EQUAL(DCM_E_SESSION_NOT_SUPPORTED, result);
}

/******************************************************************************
 * Test: DiagnosticSessionControl Service
 ******************************************************************************/
void test_dcm_session_control_request_too_short(void)
{
    Dcm_SessionInit(&test_session_config);
    
    Dcm_RequestType request = {
        .data = test_rx_buffer,
        .length = 1,  /* Too short - need at least SID + subfunction */
        .sourceAddress = 0x0F01,
        .addrMode = DCM_ADDR_PHYSICAL
    };
    
    Dcm_ResponseType response = {
        .data = test_tx_buffer,
        .maxLength = sizeof(test_tx_buffer)
    };
    
    Dcm_ReturnType result = Dcm_DiagnosticSessionControl(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT, response.negativeResponseCode);
}

void test_dcm_session_control_extended_session(void)
{
    Dcm_SessionInit(&test_session_config);
    
    test_rx_buffer[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL;
    test_rx_buffer[1] = DCM_SUBFUNC_EXTENDED_SESSION;
    
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
    
    Dcm_ReturnType result = Dcm_DiagnosticSessionControl(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(0x50, response.data[0]);  /* SID + 0x40 */
    TEST_ASSERT_EQUAL(DCM_SUBFUNC_EXTENDED_SESSION, response.data[1]);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, Dcm_GetCurrentSession());
}

void test_dcm_session_control_programming_session(void)
{
    Dcm_SessionInit(&test_session_config);
    
    test_rx_buffer[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL;
    test_rx_buffer[1] = DCM_SUBFUNC_PROGRAMMING_SESSION;
    
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
    
    Dcm_ReturnType result = Dcm_DiagnosticSessionControl(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(DCM_SESSION_PROGRAMMING, Dcm_GetCurrentSession());
}

void test_dcm_session_control_default_session(void)
{
    /* First set to extended */
    Dcm_SessionInit(&test_session_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, Dcm_GetCurrentSession());
    
    /* Then request default */
    test_rx_buffer[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL;
    test_rx_buffer[1] = DCM_SUBFUNC_DEFAULT_SESSION;
    
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
    
    Dcm_ReturnType result = Dcm_DiagnosticSessionControl(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, Dcm_GetCurrentSession());
}

void test_dcm_session_control_invalid_subfunction(void)
{
    Dcm_SessionInit(&test_session_config);
    
    test_rx_buffer[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL;
    test_rx_buffer[1] = 0x05;  /* Invalid subfunction */
    
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
    
    Dcm_ReturnType result = Dcm_DiagnosticSessionControl(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response.negativeResponseCode);
}

void test_dcm_session_control_suppress_response(void)
{
    Dcm_SessionInit(&test_session_config);
    
    test_rx_buffer[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL;
    test_rx_buffer[1] = DCM_SUBFUNC_EXTENDED_SESSION | DCM_SUPPRESS_POS_RESPONSE_MASK;
    
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
    
    Dcm_ReturnType result = Dcm_DiagnosticSessionControl(&request, &response);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(response.suppressPositiveResponse);
}

/******************************************************************************
 * Test: Session Timing Parameters
 ******************************************************************************/
void test_dcm_session_timing_parameters(void)
{
    Dcm_SessionInit(&test_session_config);
    
    uint16_t p2, p2star;
    Dcm_ReturnType result = Dcm_GetSessionTimingParameters(&p2, &p2star);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_DEFAULT_P2_SERVER_MAX, p2);
    TEST_ASSERT_EQUAL(DCM_DEFAULT_P2STAR_SERVER_MAX, p2star);
}

void test_dcm_session_set_timing_parameters(void)
{
    Dcm_SessionInit(&test_session_config);
    
    Dcm_ReturnType result = Dcm_SetSessionTimingParameters(100, 10000);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    uint16_t p2, p2star;
    Dcm_GetSessionTimingParameters(&p2, &p2star);
    TEST_ASSERT_EQUAL(100, p2);
    TEST_ASSERT_EQUAL(10000, p2star);
}

void test_dcm_session_set_timing_parameters_zero(void)
{
    Dcm_SessionInit(&test_session_config);
    
    Dcm_ReturnType result = Dcm_SetSessionTimingParameters(0, 100);
    TEST_ASSERT_EQUAL(DCM_E_REQUEST_OUT_OF_RANGE, result);
}

/******************************************************************************
 * Test: Session Timer Management
 ******************************************************************************/
void test_dcm_session_timer_update(void)
{
    Dcm_SessionInit(&test_session_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    /* Get initial timeout */
    uint32_t initial_timeout = Dcm_GetSessionTimeoutRemaining();
    TEST_ASSERT_TRUE(initial_timeout > 0);
    
    /* Update timer */
    Dcm_SessionTimerUpdate(1000);
    
    /* Check timeout decreased */
    uint32_t remaining = Dcm_GetSessionTimeoutRemaining();
    TEST_ASSERT_EQUAL(initial_timeout - 1000, remaining);
}

void test_dcm_session_timer_reset(void)
{
    Dcm_SessionInit(&test_session_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    /* Consume some time */
    Dcm_SessionTimerUpdate(1000);
    uint32_t reduced = Dcm_GetSessionTimeoutRemaining();
    TEST_ASSERT_TRUE(reduced < 5000);
    
    /* Reset timer */
    Dcm_ResetSessionTimer();
    
    uint32_t reset = Dcm_GetSessionTimeoutRemaining();
    TEST_ASSERT_EQUAL(5000, reset);
}

void test_dcm_session_stop(void)
{
    Dcm_SessionInit(&test_session_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, Dcm_GetCurrentSession());
    
    Dcm_StopSession();
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, Dcm_GetCurrentSession());
}

/******************************************************************************
 * Test: Session Configuration Retrieval
 ******************************************************************************/
void test_dcm_get_current_session_config(void)
{
    Dcm_SessionInit(&test_session_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    const Dcm_SessionConfigType *config = NULL;
    Dcm_ReturnType result = Dcm_GetCurrentSessionConfig(&config);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, config->sessionType);
    TEST_ASSERT_EQUAL(5000, config->sessionTimeoutMs);
}

void test_dcm_get_session_config(void)
{
    Dcm_SessionInit(&test_session_config);
    
    const Dcm_SessionConfigType *config = NULL;
    Dcm_ReturnType result = Dcm_GetSessionConfig(DCM_SESSION_PROGRAMMING, &config);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(DCM_SESSION_PROGRAMMING, config->sessionType);
}

void test_dcm_get_session_config_invalid(void)
{
    Dcm_SessionInit(&test_session_config);
    
    const Dcm_SessionConfigType *config = (void*)1;  /* Non-null initial value */
    Dcm_ReturnType result = Dcm_GetSessionConfig(0x05, &config);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_NULL(config);
}

/******************************************************************************
 * Test: Session Statistics
 ******************************************************************************/
void test_dcm_session_statistics(void)
{
    Dcm_SessionInit(&test_session_config);
    
    Dcm_SessionStatisticsType stats;
    Dcm_ReturnType result = Dcm_GetSessionStatistics(&stats);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(0, stats.sessionTimeoutCount);
}

/******************************************************************************
 * Test: Session Transition Records (Audit Trail)
 ******************************************************************************/
void test_dcm_session_transition_record(void)
{
    test_session_config.sessionChangeCallback = NULL;
    Dcm_SessionInit(&test_session_config);
    
    /* Perform a transition */
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    /* Get the record */
    Dcm_SessionTransitionRecordType record;
    Dcm_ReturnType result = Dcm_GetSessionTransitionRecord(0, &record);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_SESSION_DEFAULT, record.fromSession);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, record.toSession);
    TEST_ASSERT_TRUE(record.transitionSuccessful);
}

/******************************************************************************
 * Test: S3 Timer
 ******************************************************************************/
void test_dcm_s3_timer_get(void)
{
    Dcm_SessionInit(&test_session_config);
    
    uint32_t s3_time = Dcm_GetS3ServerTime();
    TEST_ASSERT_EQUAL(DCM_DEFAULT_S3_SERVER, s3_time);
}

void test_dcm_s3_timer_reset(void)
{
    Dcm_SessionInit(&test_session_config);
    Dcm_SetSession(DCM_SESSION_EXTENDED, 0x0F01);
    
    /* S3 timer gets consumed */
    Dcm_SessionTimerUpdate(1000);
    uint32_t s3_after = Dcm_GetS3ServerTime();
    TEST_ASSERT_EQUAL(DCM_DEFAULT_S3_SERVER - 1000, s3_after);
    
    /* Reset S3 timer */
    Dcm_ResetS3Timer();
    uint32_t s3_reset = Dcm_GetS3ServerTime();
    TEST_ASSERT_EQUAL(DCM_DEFAULT_S3_SERVER, s3_reset);
}

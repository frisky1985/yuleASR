/**
 * @file test_diagnostic_manager.c
 * @brief Swc_DiagnosticManager host unit tests (main + assert style)
 *
 * Covers session/security services, DTC services, tester present, security
 * gating, timeout handling and the public getter APIs. RTE port APIs and Det
 * are mocked in this file.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_DiagnosticManager.h"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* UDS constants used by assertions */
#define NRC_INCORRECT_LENGTH   0x13U
#define NRC_SUBFUNC_UNSUPPORTED 0x12U
#define NRC_OUT_OF_RANGE       0x31U
#define NRC_SECURITY_DENIED    0x33U
#define NRC_SERVICE_UNSUPPORTED 0x11U

/* ============================================================================
 * RTE / DET mocks
 * ============================================================================ */
static Swc_DiagnosticRequestType mockRequest;
static Std_ReturnType mockReadResult;
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_DiagnosticResponseType lastResponse;
static int responseWrites;
static Swc_DiagnosticSessionType lastWrittenSession;
static Swc_SecurityLevelType lastWrittenSecurity;
static int sessionWrites;
static int securityWrites;
static int detCalls;

Std_ReturnType Rte_Read_SWC_DIAGNOSTICMANAGER_PORT_DIAG_REQUEST_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(Swc_DiagnosticRequestType*)data = mockRequest;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SESSION_P(const void* data)
{
    lastWrittenSession = *(const Swc_DiagnosticSessionType*)data;
    sessionWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SECURITY_P(const void* data)
{
    lastWrittenSecurity = *(const Swc_SecurityLevelType*)data;
    securityWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DIAG_RESPONSE_P(const void* data)
{
    lastResponse = *(const Swc_DiagnosticResponseType*)data;
    responseWrites++;
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DTC_STATUS_P(const void* data)
{
    (void)data;
    return mockWriteResult;
}

uint32 Rte_GetTime(void)
{
    return mockTime;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                               uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    detCalls++;
    return E_OK;
}

/* ============================================================================
 * Helpers
 * ============================================================================ */
static void fresh_state(void)
{
    Swc_DiagnosticManager_Deinit();
    Swc_DiagnosticManager_Init();
}

static void mock_reset(void)
{
    memset(&mockRequest, 0, sizeof(mockRequest));
    memset(&lastResponse, 0, sizeof(lastResponse));
    mockReadResult = E_OK;
    mockWriteResult = E_OK;
    mockTime = 100U;
    responseWrites = 0;
    sessionWrites = 0;
    securityWrites = 0;
    lastWrittenSession = DIAG_SESSION_DEFAULT;
    lastWrittenSecurity = SECURITY_LOCKED;
    detCalls = 0;
}

static void make_request(uint8 serviceId, uint8 subFunction, uint8 dataLength)
{
    memset(&mockRequest, 0, sizeof(mockRequest));
    mockRequest.serviceId = serviceId;
    mockRequest.subFunction = subFunction;
    mockRequest.dataLength = dataLength;
    mockRequest.data[0] = subFunction;
}

/* Prime the pending-request latch: ChangeSession relies on it after Init */
static void prime_pending(void)
{
    Swc_DiagnosticRequestType request;
    Swc_DiagnosticResponseType response;

    memset(&request, 0, sizeof(request));
    request.serviceId = 0x3EU;
    request.dataLength = 1U;
    request.data[0] = 0x00U;
    (void)Swc_DiagnosticManager_ProcessDiagnosticRequest(&request, &response);
}

/* Send a request through the public API and return the number of responses */
static int send_request(uint8 serviceId, uint8 subFunction, uint8 dataLength)
{
    Swc_DiagnosticResponseType response;

    make_request(serviceId, subFunction, dataLength);
    responseWrites = 0;
    (void)Swc_DiagnosticManager_ProcessDiagnosticRequest(&mockRequest, &response);
    return responseWrites;
}

static int unlock_level1(void)
{
    static const uint8 key[4] = { 0x01, 0x02, 0x03, 0x04 };
    return (Swc_DiagnosticManager_UnlockSecurity(SECURITY_LEVEL_1, key) == RTE_E_OK) ? 0 : -1;
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_null_and_invalid_arguments(void)
{
    Swc_DiagnosticSessionType session;
    Swc_SecurityLevelType level;
    Swc_DtcStatusType dtcStatus;
    Swc_DiagnosticManagerStatusType status;
    Swc_DiagnosticRequestType request;
    Swc_DiagnosticResponseType response;
    static const uint8 key[4] = { 0x01, 0x02, 0x03, 0x04 };

    mock_reset();
    fresh_state();

    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetDtcStatus(0x123456U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetStatus(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_UnlockSecurity(SECURITY_LEVEL_1, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_UnlockSecurity((Swc_SecurityLevelType)7, key), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ProcessDiagnosticRequest(NULL_PTR, &response), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ProcessDiagnosticRequest(&request, NULL_PTR), RTE_E_INVALID);

    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_DEFAULT);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(&level), RTE_E_OK);
    TEST_ASSERT_EQ(level, SECURITY_LOCKED);

    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT_EQ(status.currentSession, DIAG_SESSION_DEFAULT);
    TEST_ASSERT_EQ(status.securityLevel, SECURITY_LOCKED);
    TEST_ASSERT(status.communicationEnabled != 0U);
    TEST_ASSERT_EQ(status.sessionTimeout, SWC_DIAG_SESSION_TIMEOUT_DEFAULT);

    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetDtcStatus(0x123456U, &dtcStatus), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ClearDtc(0x123456U), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ClearDtc(0xFFFFFFU), RTE_E_OK);
    return 0;
}

static int test_session_control(void)
{
    Swc_DiagnosticSessionType session = DIAG_SESSION_DEFAULT;

    mock_reset();
    fresh_state();
    prime_pending();

    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_EXTENDED), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_EXTENDED);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x50U);
    TEST_ASSERT_EQ(lastResponse.dataLength, 5U);
    TEST_ASSERT_EQ(lastResponse.data[0], DIAG_SESSION_EXTENDED);

    /* Programming session requires security level 2 */
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_PROGRAMMING), RTE_E_OK);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_OUT_OF_RANGE);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_EXTENDED);

    /* Unsupported session id */
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession((Swc_DiagnosticSessionType)0x55), RTE_E_OK);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_SUBFUNC_UNSUPPORTED);

    /* Back to default */
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_DEFAULT), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_DEFAULT);
    return 0;
}

static int test_session_control_zero_length(void)
{
    mock_reset();
    fresh_state();

    /* dataLength == 0 -> incorrect message length */
    TEST_ASSERT_EQ(send_request(0x10U, 0x03U, 0U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_INCORRECT_LENGTH);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x7FU);
    return 0;
}

static int test_security_access_seed(void)
{
    mock_reset();
    fresh_state();
    mockTime = 0x11223344U;

    TEST_ASSERT_EQ(send_request(0x27U, 0x01U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x67U);
    TEST_ASSERT_EQ(lastResponse.dataLength, 5U);
    TEST_ASSERT_EQ(lastResponse.data[0], 0x01U);
    TEST_ASSERT_EQ(lastResponse.data[1], 0x44U);
    TEST_ASSERT_EQ(lastResponse.data[2], 0x33U);
    TEST_ASSERT_EQ(lastResponse.data[3], 0x22U);
    TEST_ASSERT_EQ(lastResponse.data[4], 0x11U);

    /* Subfunction 0x05 requests seed for level 3 (accepted) */
    TEST_ASSERT_EQ(send_request(0x27U, 0x05U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x67U);

    /* Subfunction 0x07 maps to level 4 -> not supported */
    TEST_ASSERT_EQ(send_request(0x27U, 0x07U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_SUBFUNC_UNSUPPORTED);
    return 0;
}

static int test_security_access_key(void)
{
    Swc_SecurityLevelType level = SECURITY_LOCKED;
    Swc_DiagnosticRequestType request;
    Swc_DiagnosticResponseType response;

    mock_reset();
    fresh_state();

    /* Correct key for level 1 through the public request path */
    memset(&request, 0, sizeof(request));
    request.serviceId = 0x27U;
    request.dataLength = 5U;
    request.data[0] = 0x02U;
    request.data[1] = 0x01U;
    request.data[2] = 0x02U;
    request.data[3] = 0x03U;
    request.data[4] = 0x04U;
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ProcessDiagnosticRequest(&request, &response), RTE_E_OK);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x67U);
    TEST_ASSERT_EQ(lastResponse.dataLength, 1U);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(&level), RTE_E_OK);
    TEST_ASSERT_EQ(level, SECURITY_LEVEL_1);

    /* Wrong key */
    fresh_state();
    request.data[1] = 0xFFU;
    (void)Swc_DiagnosticManager_ProcessDiagnosticRequest(&request, &response);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_SECURITY_DENIED);

    /* Key too short */
    request.dataLength = 4U;
    (void)Swc_DiagnosticManager_ProcessDiagnosticRequest(&request, &response);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_INCORRECT_LENGTH);

    /* UnlockSecurity public API for level 2 / level 3 */
    {
        static const uint8 key2[4] = { 0x05, 0x06, 0x07, 0x08 };
        static const uint8 key3[4] = { 0x00, 0x00, 0x00, 0x00 };
        TEST_ASSERT_EQ(Swc_DiagnosticManager_UnlockSecurity(SECURITY_LEVEL_2, key2), RTE_E_OK);
        TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(&level), RTE_E_OK);
        TEST_ASSERT_EQ(level, SECURITY_LEVEL_2);
        TEST_ASSERT_EQ(Swc_DiagnosticManager_UnlockSecurity(SECURITY_LEVEL_3, key3), RTE_E_INVALID);
    }
    return 0;
}

static int test_programming_session_after_unlock(void)
{
    Swc_DiagnosticSessionType session = DIAG_SESSION_DEFAULT;
    static const uint8 key2[4] = { 0x05, 0x06, 0x07, 0x08 };

    mock_reset();
    fresh_state();
    prime_pending();

    TEST_ASSERT_EQ(Swc_DiagnosticManager_UnlockSecurity(SECURITY_LEVEL_2, key2), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_PROGRAMMING), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_PROGRAMMING);

    /* Safety system session needs level 3 -> rejected, session unchanged */
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_SAFETY_SYSTEM), RTE_E_OK);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_OUT_OF_RANGE);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_PROGRAMMING);
    return 0;
}

static int test_tester_present(void)
{
    mock_reset();
    fresh_state();
    mockTime = 1234U;

    TEST_ASSERT_EQ(send_request(0x3EU, 0x00U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x7EU);
    TEST_ASSERT_EQ(lastResponse.dataLength, 1U);
    TEST_ASSERT_EQ(lastResponse.data[0], 0x00U);

    /* Zero length is rejected */
    TEST_ASSERT_EQ(send_request(0x3EU, 0x00U, 0U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_INCORRECT_LENGTH);
    return 0;
}

static int test_security_gating(void)
{
    mock_reset();
    fresh_state();

    /* WriteDataByIdentifier requires level 1 */
    TEST_ASSERT_EQ(send_request(0x2EU, 0x00U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_OUT_OF_RANGE);

    /* ECU reset requires level 2 */
    TEST_ASSERT_EQ(unlock_level1(), 0);
    TEST_ASSERT_EQ(send_request(0x11U, 0x01U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_OUT_OF_RANGE);

    /* With level 1, WriteDataByIdentifier passes gating but is unimplemented */
    TEST_ASSERT_EQ(send_request(0x2EU, 0x00U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_SERVICE_UNSUPPORTED);

    /* Unsupported service id */
    TEST_ASSERT_EQ(send_request(0x99U, 0x00U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_SERVICE_UNSUPPORTED);
    return 0;
}

static int test_read_dtc(void)
{
    mock_reset();
    fresh_state();

    /* No DTCs stored -> positive response with count 0 */
    TEST_ASSERT_EQ(send_request(0x19U, 0x02U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x59U);
    TEST_ASSERT_EQ(lastResponse.dataLength, 2U);
    TEST_ASSERT_EQ(lastResponse.data[0], 0x02U);
    TEST_ASSERT_EQ(lastResponse.data[1], 0U);

    /* Extended data with no DTC -> out of range */
    TEST_ASSERT_EQ(send_request(0x19U, 0x06U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_OUT_OF_RANGE);

    /* Unsupported subfunction */
    TEST_ASSERT_EQ(send_request(0x19U, 0x01U, 1U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_SUBFUNC_UNSUPPORTED);

    /* Missing subfunction */
    TEST_ASSERT_EQ(send_request(0x19U, 0x02U, 0U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_INCORRECT_LENGTH);
    return 0;
}

static int test_clear_dtc_service(void)
{
    mock_reset();
    fresh_state();

    /* Too short */
    TEST_ASSERT_EQ(send_request(0x14U, 0x00U, 2U), 1);
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_INCORRECT_LENGTH);

    /* Unknown specific DTC */
    mockRequest.data[0] = 0x12U;
    mockRequest.data[1] = 0x34U;
    mockRequest.data[2] = 0x56U;
    mockRequest.dataLength = 3U;
    responseWrites = 0;
    {
        Swc_DiagnosticResponseType response;
        (void)Swc_DiagnosticManager_ProcessDiagnosticRequest(&mockRequest, &response);
    }
    TEST_ASSERT_EQ(lastResponse.negativeResponseCode, NRC_OUT_OF_RANGE);

    /* Clear all DTCs -> positive response */
    mockRequest.data[0] = 0xFFU;
    mockRequest.data[1] = 0xFFU;
    mockRequest.data[2] = 0xFFU;
    {
        Swc_DiagnosticResponseType response;
        (void)Swc_DiagnosticManager_ProcessDiagnosticRequest(&mockRequest, &response);
    }
    TEST_ASSERT_EQ(lastResponse.responseId, 0x54U);
    TEST_ASSERT_EQ(lastResponse.dataLength, 0U);
    return 0;
}

static int test_timeout_handling(void)
{
    Swc_DiagnosticSessionType session = DIAG_SESSION_DEFAULT;
    Swc_SecurityLevelType level = SECURITY_LOCKED;

    mock_reset();
    fresh_state();
    prime_pending();

    mockTime = 1000U;
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_EXTENDED), RTE_E_OK);
    TEST_ASSERT_EQ(unlock_level1(), 0);

    /* Refresh tester present, then run 50ms inside the timeout window */
    TEST_ASSERT_EQ(send_request(0x3EU, 0x00U, 1U), 1);
    mockTime = 3000U;
    Swc_DiagnosticManager_50ms();
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_EXTENDED);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(&level), RTE_E_OK);
    TEST_ASSERT_EQ(level, SECURITY_LEVEL_1);
    TEST_ASSERT_EQ(sessionWrites, 1);
    TEST_ASSERT_EQ(securityWrites, 1);
    TEST_ASSERT_EQ(lastWrittenSession, DIAG_SESSION_EXTENDED);
    TEST_ASSERT_EQ(lastWrittenSecurity, SECURITY_LEVEL_1);

    /* Beyond 5000 ms both session and security time out */
    mockTime = 9000U;
    Swc_DiagnosticManager_50ms();
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_DEFAULT);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(&level), RTE_E_OK);
    TEST_ASSERT_EQ(level, SECURITY_LOCKED);
    return 0;
}

static int test_process_request_rte_paths(void)
{
    mock_reset();
    fresh_state();

    /* Read fails -> no response */
    mockReadResult = E_NOT_OK;
    responseWrites = 0;
    Swc_DiagnosticManager_ProcessRequest();
    TEST_ASSERT_EQ(responseWrites, 0);

    /* Read succeeds -> request processed */
    mockReadResult = E_OK;
    make_request(0x3EU, 0x00U, 1U);
    Swc_DiagnosticManager_ProcessRequest();
    TEST_ASSERT_EQ(responseWrites, 1);
    TEST_ASSERT_EQ(lastResponse.responseId, 0x7EU);
    return 0;
}

static int test_process_diagnostic_request_api(void)
{
    Swc_DiagnosticRequestType request;
    Swc_DiagnosticResponseType response;

    mock_reset();
    fresh_state();

    memset(&request, 0, sizeof(request));
    memset(&response, 0xAA, sizeof(response));
    request.serviceId = 0x3EU;
    request.dataLength = 1U;
    request.data[0] = 0x42U; /* responseId is copied from data[0] (simplified impl) */

    TEST_ASSERT_EQ(Swc_DiagnosticManager_ProcessDiagnosticRequest(&request, &response), RTE_E_OK);
    TEST_ASSERT_EQ(response.responseId, 0x42U);
    TEST_ASSERT_EQ(response.dataLength, 0U);
    TEST_ASSERT_EQ(response.negativeResponseCode, 0U);
    return 0;
}

static int test_mainfunction_and_deinit(void)
{
    Swc_DiagnosticSessionType session = DIAG_SESSION_EXTENDED;
    Swc_SecurityLevelType level = SECURITY_LEVEL_1;
    Swc_DiagnosticManagerStatusType status;

    mock_reset();
    fresh_state();
    prime_pending();
    TEST_ASSERT_EQ(Swc_DiagnosticManager_ChangeSession(DIAG_SESSION_EXTENDED), RTE_E_OK);
    TEST_ASSERT_EQ(unlock_level1(), 0);

    sessionWrites = 0;
    securityWrites = 0;
    Swc_DiagnosticManager_MainFunction();
    TEST_ASSERT_EQ(sessionWrites, 1);
    TEST_ASSERT_EQ(securityWrites, 1);

    Swc_DiagnosticManager_Deinit();
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSession(&session), RTE_E_OK);
    TEST_ASSERT_EQ(session, DIAG_SESSION_DEFAULT);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetSecurityLevel(&level), RTE_E_OK);
    TEST_ASSERT_EQ(level, SECURITY_LOCKED);
    TEST_ASSERT_EQ(Swc_DiagnosticManager_GetStatus(&status), RTE_E_OK);
    TEST_ASSERT(status.communicationEnabled != 0U);
    return 0;
}

/* ============================================================================
 * Runner
 * ============================================================================ */
typedef struct {
    const char* name;
    int (*func)(void);
} TestCaseType;

static const TestCaseType testCases[] = {
    { "null_invalid_args", test_null_and_invalid_arguments },
    { "session_control", test_session_control },
    { "session_zero_length", test_session_control_zero_length },
    { "security_seed", test_security_access_seed },
    { "security_key", test_security_access_key },
    { "programming_session", test_programming_session_after_unlock },
    { "tester_present", test_tester_present },
    { "security_gating", test_security_gating },
    { "read_dtc", test_read_dtc },
    { "clear_dtc", test_clear_dtc_service },
    { "timeout_handling", test_timeout_handling },
    { "process_request_rte", test_process_request_rte_paths },
    { "process_diag_request_api", test_process_diagnostic_request_api },
    { "mainfunction_deinit", test_mainfunction_and_deinit }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== DiagnosticManager unit tests ===\n");
    for (i = 0; i < numTests; i++) {
        int result;
        tests_run++;
        printf("[%zu/%zu] %s ... ", i + 1, numTests, testCases[i].name);
        result = testCases[i].func();
        if (result == 0) {
            tests_passed++;
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }
    }
    printf("=== %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

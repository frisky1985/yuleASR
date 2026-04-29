/**
 * @file test_dcm_dem_integration.c
 * @brief DCM-DEM Integration Test Suite
 *
 * Integration tests for DCM-DEM integration layer including:
 * - 0x14 ClearDiagnosticInformation service
 * - 0x19 ReadDTCInformation service
 * - DEM event status callbacks
 * - IsoTp-PduR integration
 *
 * @copyright Copyright (c) 2024
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/diagnostics/dcm/dcm.h"
#include "../src/diagnostics/dem/dem.h"
#include "../src/diagnostics/dcm_dem_integration.h"
#include "../src/diagnostics/isotp/isotp_core.h"
#include "../src/diagnostics/isotp/isotp_pdur.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/
/* Test DTC codes */
#define TEST_DTC_1                      0x010101U  /* Powertrain DTC */
#define TEST_DTC_2                      0x020202U  /* Chassis DTC */
#define TEST_DTC_3                      0x030303U  /* Body DTC */

/* Test Event IDs */
#define TEST_EVENT_ID_1                 0x0001U
#define TEST_EVENT_ID_2                 0x0002U
#define TEST_EVENT_ID_3                 0x0003U

/******************************************************************************
 * Test Statistics
 ******************************************************************************/
typedef struct {
    uint32_t testsRun;
    uint32_t testsPassed;
    uint32_t testsFailed;
    uint32_t callbackCount;
} TestStatisticsType;

static TestStatisticsType s_testStats = {0};

/******************************************************************************
 * Test Callbacks
 ******************************************************************************/
static uint32_t s_lastDtcNotified = 0U;
static uint8_t s_lastOldStatus = 0U;
static uint8_t s_lastNewStatus = 0U;

static void testDtcStatusCallbackFunc(
    uint32_t dtc,
    uint8_t statusOld,
    uint8_t statusNew)
{
    s_lastDtcNotified = dtc;
    s_lastOldStatus = statusOld;
    s_lastNewStatus = statusNew;
    s_testStats.callbackCount++;
}

static void testEventStatusCallbackFunc(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatusOld,
    Dem_EventStatusType EventStatusNew)
{
    (void)EventId;
    (void)EventStatusOld;
    (void)EventStatusNew;
    s_testStats.callbackCount++;
}

/******************************************************************************
 * Test Helper Functions
 ******************************************************************************/
static void printTestResult(const char* testName, int passed)
{
    s_testStats.testsRun++;
    if (passed) {
        s_testStats.testsPassed++;
        printf("  [PASS] %s\n", testName);
    } else {
        s_testStats.testsFailed++;
        printf("  [FAIL] %s\n", testName);
    }
}

static void setupTestDtc(Dem_EventIdType eventId)
{
    /* Set event status to create DTC */
    Dem_SetEventStatus(eventId, DEM_EVENT_STATUS_FAILED);
}

static void clearTestDtc(Dem_EventIdType eventId)
{
    /* Reset event status */
    Dem_ResetEventStatus(eventId);
}

/******************************************************************************
 * Test Cases
 ******************************************************************************/

/**
 * @brief Test DCM-DEM integration initialization
 */
static void testIntegrationInit(void)
{
    Dcm_ReturnType result;
    Dcm_DemIntegrationConfigType config = {
        .maxNumberOfDTCs = 10U,
        .maxResponseLength = 4096U,
        .supportDtcSnapshot = 1,
        .supportDtcExtendedData = 1,
        .defaultDtcStatusMask = 0xFFU,
        .defaultDtcSeverityMask = 0x00U
    };

    /* Test initialization */
    result = Dcm_DemIntegration_Init(&config);
    printTestResult("Integration Init", result == DCM_E_OK);

    /* Test double initialization */
    result = Dcm_DemIntegration_Init(&config);
    printTestResult("Integration Double Init", result == DCM_E_OK);

    /* Test deinitialization */
    result = Dcm_DemIntegration_DeInit();
    printTestResult("Integration DeInit", result == DCM_E_OK);

    /* Re-initialize for other tests */
    result = Dcm_DemIntegration_Init(&config);
    printTestResult("Integration Re-Init", result == DCM_E_OK);
}

/**
 * @brief Test ClearDiagnosticInformation service (0x14)
 */
static void testClearDiagnosticInformation(void)
{
    Dcm_ReturnType result;
    uint8_t requestData[16];
    uint8_t responseData[64];
    Dcm_RequestType request;
    Dcm_ResponseType response;

    /* Setup request and response structures */
    request.data = requestData;
    request.length = 0U;
    request.sourceAddress = 0x00U;
    request.addrMode = DCM_ADDR_PHYSICAL;
    request.protocol = DCM_PROTOCOL_UDS_ON_CAN;
    request.timestamp = 0U;

    response.data = responseData;
    response.length = 0U;
    response.maxLength = sizeof(responseData);
    response.isNegativeResponse = 0;
    response.negativeResponseCode = 0U;
    response.suppressPositiveResponse = 0;

    /* Test 1: Clear specific DTC - invalid length */
    request.data[0] = UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION;
    request.length = 2U;  /* Too short */
    result = Dcm_DemIntegration_ClearDiagnosticInformation(&request, &response);
    printTestResult("Clear DTC - Invalid Length", 
        (result == DCM_E_OK) && response.isNegativeResponse &&
        (response.negativeResponseCode == UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT));

    /* Test 2: Clear specific DTC - invalid DTC */
    request.data[0] = UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION;
    request.data[1] = 0x00U;
    request.data[2] = 0x00U;
    request.data[3] = 0x00U;  /* DTC = 0x000000 (invalid) */
    request.length = 4U;
    result = Dcm_DemIntegration_ClearDiagnosticInformation(&request, &response);
    printTestResult("Clear DTC - Invalid DTC (0x000000)", 
        (result == DCM_E_OK) && response.isNegativeResponse &&
        (response.negativeResponseCode == UDS_NRC_REQUEST_OUT_OF_RANGE));

    /* Test 3: Clear specific DTC - valid DTC */
    setupTestDtc(TEST_EVENT_ID_1);
    request.data[0] = UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION;
    request.data[1] = (uint8_t)((TEST_DTC_1 >> 16) & 0xFFU);
    request.data[2] = (uint8_t)((TEST_DTC_1 >> 8) & 0xFFU);
    request.data[3] = (uint8_t)(TEST_DTC_1 & 0xFFU);
    request.length = 4U;
    response.isNegativeResponse = 0;
    result = Dcm_DemIntegration_ClearDiagnosticInformation(&request, &response);
    printTestResult("Clear DTC - Valid DTC", 
        (result == DCM_E_OK) && !response.isNegativeResponse &&
        (response.data[0] == (UDS_SVC_CLEAR_DIAGNOSTIC_INFORMATION + 0x40U)));

    /* Clean up */
    clearTestDtc(TEST_EVENT_ID_1);
}

/**
 * @brief Test ReadDTCInformation service - reportNumberOfDTCByStatusMask
 */
static void testReadDTCNumberByStatusMask(void)
{
    Dcm_ReturnType result;
    uint8_t requestData[16];
    uint8_t responseData[64];
    Dcm_RequestType request;
    Dcm_ResponseType response;

    /* Setup structures */
    request.data = requestData;
    request.length = 0U;
    response.data = responseData;
    response.maxLength = sizeof(responseData);

    /* Setup test DTCs */
    setupTestDtc(TEST_EVENT_ID_1);
    setupTestDtc(TEST_EVENT_ID_2);

    /* Test: Report number of DTCs with status mask 0xFF (all) */
    request.data[0] = UDS_SVC_READ_DTC_INFORMATION;
    request.data[1] = DCM_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS_MASK;
    request.data[2] = 0xFFU;  /* Status mask - all DTCs */
    request.length = 3U;

    result = Dcm_DemIntegration_ReadDTCInformation(&request, &response);

    printTestResult("Read DTC Number By Status Mask", 
        (result == DCM_E_OK) && 
        !response.isNegativeResponse &&
        (response.length == 6U) &&
        (response.data[0] == (UDS_SVC_READ_DTC_INFORMATION + 0x40U)) &&
        (response.data[1] == DCM_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS_MASK) &&
        (response.data[2] == DCM_DTC_FORMAT_ISO_14229_1) &&
        (response.data[3] == 0xFFU));  /* Availability mask */

    /* Clean up */
    clearTestDtc(TEST_EVENT_ID_1);
    clearTestDtc(TEST_EVENT_ID_2);
}

/**
 * @brief Test ReadDTCInformation service - reportDTCByStatusMask
 */
static void testReadDTCByStatusMask(void)
{
    Dcm_ReturnType result;
    uint8_t requestData[64];
    uint8_t responseData[128];
    Dcm_RequestType request;
    Dcm_ResponseType response;

    /* Setup structures */
    request.data = requestData;
    request.length = 0U;
    response.data = responseData;
    response.maxLength = sizeof(responseData);

    /* Setup test DTC */
    setupTestDtc(TEST_EVENT_ID_1);

    /* Test: Report DTCs with status mask */
    request.data[0] = UDS_SVC_READ_DTC_INFORMATION;
    request.data[1] = DCM_SUBFUNC_REPORT_DTC_BY_STATUS_MASK;
    request.data[2] = 0xFFU;  /* Status mask */
    request.length = 3U;

    result = Dcm_DemIntegration_ReadDTCInformation(&request, &response);

    printTestResult("Read DTC By Status Mask", 
        (result == DCM_E_OK) && 
        !response.isNegativeResponse &&
        (response.data[0] == (UDS_SVC_READ_DTC_INFORMATION + 0x40U)) &&
        (response.data[1] == DCM_SUBFUNC_REPORT_DTC_BY_STATUS_MASK) &&
        (response.data[2] == DCM_DTC_FORMAT_ISO_14229_1));

    /* Clean up */
    clearTestDtc(TEST_EVENT_ID_1);
}

/**
 * @brief Test ReadDTCInformation service - reportSupportedDTC
 */
static void testReadSupportedDTC(void)
{
    Dcm_ReturnType result;
    uint8_t requestData[16];
    uint8_t responseData[128];
    Dcm_RequestType request;
    Dcm_ResponseType response;

    /* Setup structures */
    request.data = requestData;
    request.length = 0U;
    response.data = responseData;
    response.maxLength = sizeof(responseData);

    /* Setup test DTC */
    setupTestDtc(TEST_EVENT_ID_1);

    /* Test: Report supported DTCs */
    request.data[0] = UDS_SVC_READ_DTC_INFORMATION;
    request.data[1] = DCM_SUBFUNC_REPORT_SUPPORTED_DTC;
    request.length = 2U;

    result = Dcm_DemIntegration_ReadDTCInformation(&request, &response);

    printTestResult("Read Supported DTC", 
        (result == DCM_E_OK) && 
        !response.isNegativeResponse &&
        (response.data[0] == (UDS_SVC_READ_DTC_INFORMATION + 0x40U)) &&
        (response.data[1] == DCM_SUBFUNC_REPORT_SUPPORTED_DTC) &&
        (response.data[2] == DCM_DTC_FORMAT_ISO_14229_1));

    /* Clean up */
    clearTestDtc(TEST_EVENT_ID_1);
}

/**
 * @brief Test DTC status callback registration and notification
 */
static void testDtcStatusCallback(void)
{
    Dcm_ReturnType result;

    /* Reset callback counters */
    s_testStats.callbackCount = 0U;
    s_lastDtcNotified = 0U;

    /* Register callback */
    result = Dcm_DemIntegration_RegisterDtcStatusCallback(testDtcStatusCallbackFunc);
    printTestResult("Register DTC Status Callback", result == DCM_E_OK);

    /* Setup test DTC to trigger callback */
    setupTestDtc(TEST_EVENT_ID_1);

    /* Verify callback was called */
    printTestResult("DTC Status Callback Called", 
        s_testStats.callbackCount > 0U && s_lastDtcNotified != 0U);

    /* Unregister callback */
    result = Dcm_DemIntegration_RegisterDtcStatusCallback(NULL);
    printTestResult("Unregister DTC Status Callback", result == DCM_E_OK);

    /* Clean up */
    clearTestDtc(TEST_EVENT_ID_1);
}

/**
 * @brief Test event status callback registration
 */
static void testEventStatusCallback(void)
{
    Dcm_ReturnType result;

    /* Reset callback counters */
    s_testStats.callbackCount = 0U;

    /* Register callback */
    result = Dcm_DemIntegration_RegisterEventStatusCallback(testEventStatusCallbackFunc);
    printTestResult("Register Event Status Callback", result == DCM_E_OK);

    /* Setup test event to trigger callback */
    setupTestDtc(TEST_EVENT_ID_2);

    /* Verify callback was called */
    printTestResult("Event Status Callback Called", s_testStats.callbackCount > 0U);

    /* Unregister callback */
    result = Dcm_DemIntegration_RegisterEventStatusCallback(NULL);
    printTestResult("Unregister Event Status Callback", result == DCM_E_OK);

    /* Clean up */
    clearTestDtc(TEST_EVENT_ID_2);
}

/**
 * @brief Test IsoTp-PduR integration initialization
 */
static void testIsoTpPduRIntegration(void)
{
    Isotp_ReturnType result;
    Isotp_PduRChannelConfigType channelConfig = {
        .channelId = 0U,
        .txPduId = ISOTP_PDUR_DCM_TX_PDU_ID,
        .rxPduId = ISOTP_PDUR_DCM_RX_PDU_ID,
        .bufferSize = 4096U,
        .upperLayerModule = PDUR_MODULE_DCM
    };
    Isotp_PduRConfigType config = {
        .channelConfigs = &channelConfig,
        .numChannels = 1U
    };

    /* Test initialization */
    result = Isotp_PduR_Init(&config);
    printTestResult("IsoTp-PduR Integration Init", result == ISOTP_E_OK);

    /* Test initialization check */
    printTestResult("IsoTp-PduR IsInitialized", Isotp_PduR_IsInitialized() == 1);

    /* Test deinitialization */
    result = Isotp_PduR_DeInit();
    printTestResult("IsoTp-PduR Integration DeInit", result == ISOTP_E_OK);
}

/**
 * @brief Test DCM integration with full module initialization
 */
static void testFullIntegration(void)
{
    Std_ReturnType demResult;

    /* Initialize DEM */
    demResult = Dem_Init(NULL);
    printTestResult("DEM Init", demResult == E_OK);

    /* Test DEM-DCM callback registration */
    demResult = Dem_RegisterEventStatusChangedCallback(testEventStatusCallbackFunc);
    printTestResult("DEM Register Event Callback", demResult == E_OK);

    /* Clean up */
    Dem_Shutdown();
}

/******************************************************************************
 * Main Test Function
 ******************************************************************************/
int main(void)
{
    printf("=================================================\n");
    printf("DCM-DEM Integration Test Suite\n");
    printf("=================================================\n\n");

    /* Run test suites */
    printf("--- Test: Integration Initialization ---\n");
    testIntegrationInit();
    printf("\n");

    printf("--- Test: ClearDiagnosticInformation (0x14) ---\n");
    testClearDiagnosticInformation();
    printf("\n");

    printf("--- Test: ReadDTCInformation - Number By Status (0x19 01) ---\n");
    testReadDTCNumberByStatusMask();
    printf("\n");

    printf("--- Test: ReadDTCInformation - DTC By Status (0x19 02) ---\n");
    testReadDTCByStatusMask();
    printf("\n");

    printf("--- Test: ReadDTCInformation - Supported DTC (0x19 0A) ---\n");
    testReadSupportedDTC();
    printf("\n");

    printf("--- Test: DTC Status Callback ---\n");
    testDtcStatusCallback();
    printf("\n");

    printf("--- Test: Event Status Callback ---\n");
    testEventStatusCallback();
    printf("\n");

    printf("--- Test: IsoTp-PduR Integration ---\n");
    testIsoTpPduRIntegration();
    printf("\n");

    printf("--- Test: Full Integration ---\n");
    testFullIntegration();
    printf("\n");

    /* Print test summary */
    printf("=================================================\n");
    printf("Test Summary:\n");
    printf("  Total Tests:  %u\n", s_testStats.testsRun);
    printf("  Passed:       %u\n", s_testStats.testsPassed);
    printf("  Failed:       %u\n", s_testStats.testsFailed);
    printf("  Callbacks:    %u\n", s_testStats.callbackCount);
    printf("=================================================\n");

    return (s_testStats.testsFailed == 0U) ? 0 : 1;
}

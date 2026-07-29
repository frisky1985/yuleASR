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

/******************************************************************************
 * @file    test_write_data_by_identifier.c
 * @brief   Test cases for Write Data By Identifier (0x2E) Service
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant (Section 10.6)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_did.h"
#include "dcm_session.h"
#include "dcm_security.h"
#include <stdio.h>
#include <string.h>

/******************************************************************************
 * Test Data
 ******************************************************************************/
static uint8_t testVinData[17] = {0};
static uint8_t testConfigData[4] = {0};

/******************************************************************************
 * Test Write Callback for VIN (0xF190)
 ******************************************************************************/
static Dcm_ReturnType testWriteVIN(
    uint16_t did,
    const uint8_t *data,
    uint16_t dataLength)
{
    (void)did;
    
    if (dataLength != 17U) {
        return DCM_E_NOT_OK;
    }
    
    (void)memcpy(testVinData, data, 17U);
    printf("VIN written successfully\n");
    return DCM_E_OK;
}

/******************************************************************************
 * Test Write Callback for Configuration Data
 ******************************************************************************/
static Dcm_ReturnType testWriteConfig(
    uint16_t did,
    const uint8_t *data,
    uint16_t dataLength)
{
    (void)did;
    
    if (dataLength != 4U) {
        return DCM_E_NOT_OK;
    }
    
    (void)memcpy(testConfigData, data, 4U);
    printf("Configuration data written successfully\n");
    return DCM_E_OK;
}

/******************************************************************************
 * Test DID Info Structures
 ******************************************************************************/
static const Dcm_DidInfoType vinDidInfo = {
    .did = 0xF190U,
    .dataLength = 17U,
    .maxDataLength = 17U,
    .requiredSecurityLevel = 1U,  /* Requires unlocked state */
    .supportedSessions = 0x06,    /* Extended and Programming sessions */
    .readEnabled = true,
    .writeEnabled = true,
    .controlEnabled = false,
    .readCallback = NULL,
    .writeCallback = testWriteVIN,
    .controlCallback = NULL
};

static const Dcm_DidInfoType configDidInfo = {
    .did = 0x0100U,
    .dataLength = 4U,
    .maxDataLength = 4U,
    .requiredSecurityLevel = 0U,  /* No security required */
    .supportedSessions = 0xFF,    /* All sessions */
    .readEnabled = true,
    .writeEnabled = true,
    .controlEnabled = false,
    .readCallback = NULL,
    .writeCallback = testWriteConfig,
    .controlCallback = NULL
};

/******************************************************************************
 * Test Cases
 ******************************************************************************/

/**
 * @brief Test 1: Successful write to writable DID
 */
static int test_successful_write(void)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    uint8_t requestData[32] = {0};
    uint8_t responseData[32] = {0};
    Dcm_ReturnType result;
    
    printf("Test 1: Successful write to writable DID\n");
    
    /* Setup request: 0x2E 0x01 0x00 + 4 bytes data */
    requestData[0] = 0x2E;  /* SID */
    requestData[1] = 0x01;  /* DID High */
    requestData[2] = 0x00;  /* DID Low */
    requestData[3] = 0x11;
    requestData[4] = 0x22;
    requestData[5] = 0x33;
    requestData[6] = 0x44;
    
    request.data = requestData;
    request.length = 7U;
    
    response.data = responseData;
    response.maxLength = 32U;
    
    result = Dcm_WriteDataByIdentifier(&request, &response);
    
    if (result == DCM_E_OK && 
        response.length == 3U &&
        responseData[0] == 0x6E &&  /* Positive response SID */
        responseData[1] == 0x01 &&  /* DID High */
        responseData[2] == 0x00) {  /* DID Low */
        printf("  PASSED\n");
        return 0;
    }
    
    printf("  FAILED\n");
    return 1;
}

/**
 * @brief Test 2: Request out of range - DID not registered
 */
static int test_request_out_of_range(void)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    uint8_t requestData[32] = {0};
    uint8_t responseData[32] = {0};
    Dcm_ReturnType result;
    
    printf("Test 2: Request out of range - DID not registered\n");
    
    /* Setup request with unregistered DID */
    requestData[0] = 0x2E;  /* SID */
    requestData[1] = 0x99;  /* DID High - unregistered */
    requestData[2] = 0x99;  /* DID Low */
    requestData[3] = 0x11;
    
    request.data = requestData;
    request.length = 4U;
    
    response.data = responseData;
    response.maxLength = 32U;
    
    result = Dcm_WriteDataByIdentifier(&request, &response);
    
    if (result != DCM_E_OK && 
        response.isNegativeResponse &&
        response.negativeResponseCode == UDS_NRC_REQUEST_OUT_OF_RANGE) {
        printf("  PASSED (NRC 0x31)\n");
        return 0;
    }
    
    printf("  FAILED\n");
    return 1;
}

/**
 * @brief Test 3: Incorrect message length
 */
static int test_incorrect_length(void)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    uint8_t requestData[32] = {0};
    uint8_t responseData[32] = {0};
    Dcm_ReturnType result;
    
    printf("Test 3: Incorrect message length\n");
    
    /* Setup request with only SID + DID (no data) */
    requestData[0] = 0x2E;  /* SID */
    requestData[1] = 0x01;  /* DID High */
    requestData[2] = 0x00;  /* DID Low */
    /* Missing data */
    
    request.data = requestData;
    request.length = 3U;  /* Too short - minimum is 3 with at least 1 byte data */
    
    response.data = responseData;
    response.maxLength = 32U;
    
    result = Dcm_WriteDataByIdentifier(&request, &response);
    
    if (result != DCM_E_OK && 
        response.isNegativeResponse &&
        response.negativeResponseCode == UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT) {
        printf("  PASSED (NRC 0x13)\n");
        return 0;
    }
    
    printf("  FAILED\n");
    return 1;
}

/**
 * @brief Test 4: Security access denied
 */
static int test_security_access_denied(void)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    uint8_t requestData[32] = {0};
    uint8_t responseData[32] = {0};
    Dcm_ReturnType result;
    
    printf("Test 4: Security access denied\n");
    
    /* Setup request to VIN which requires security level 1 */
    requestData[0] = 0x2E;  /* SID */
    requestData[1] = 0xF1;  /* DID High */
    requestData[2] = 0x90;  /* DID Low */
    
    /* Fill VIN data (17 bytes) */
    for (int i = 0; i < 17; i++) {
        requestData[3 + i] = (uint8_t)('A' + i);
    }
    
    request.data = requestData;
    request.length = 20U;
    
    response.data = responseData;
    response.maxLength = 32U;
    
    result = Dcm_WriteDataByIdentifier(&request, &response);
    
    /* Should fail with security access denied (NRC 0x33) */
    if (result != DCM_E_OK && 
        response.isNegativeResponse &&
        response.negativeResponseCode == UDS_NRC_SECURITY_ACCESS_DENIED) {
        printf("  PASSED (NRC 0x33)\n");
        return 0;
    }
    
    printf("  FAILED\n");
    return 1;
}

/**
 * @brief Test 5: Check IsDidWritable function
 */
static int test_is_did_writable(void)
{
    printf("Test 5: Check IsDidWritable function\n");
    
    if (Dcm_IsDidWritable(0x0100U) &&    /* Config DID is writable */
        Dcm_IsDidWritable(0xF190U) &&    /* VIN is writable */
        !Dcm_IsDidWritable(0x9999U)) {   /* Unregistered DID is not writable */
        printf("  PASSED\n");
        return 0;
    }
    
    printf("  FAILED\n");
    return 1;
}

/**
 * @brief Test 6: Write with incorrect data length
 */
static int test_incorrect_data_length(void)
{
    Dcm_RequestType request;
    Dcm_ResponseType response;
    uint8_t requestData[32] = {0};
    uint8_t responseData[32] = {0};
    Dcm_ReturnType result;
    
    printf("Test 6: Write with incorrect data length\n");
    
    /* Setup request with wrong data length (Config DID expects 4 bytes) */
    requestData[0] = 0x2E;  /* SID */
    requestData[1] = 0x01;  /* DID High */
    requestData[2] = 0x00;  /* DID Low */
    requestData[3] = 0x11;
    requestData[4] = 0x22;
    /* Only 2 bytes instead of 4 */
    
    request.data = requestData;
    request.length = 5U;
    
    response.data = responseData;
    response.maxLength = 32U;
    
    result = Dcm_WriteDataByIdentifier(&request, &response);
    
    if (result != DCM_E_OK && 
        response.isNegativeResponse &&
        response.negativeResponseCode == UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT) {
        printf("  PASSED (NRC 0x13)\n");
        return 0;
    }
    
    printf("  FAILED\n");
    return 1;
}

/******************************************************************************
 * Main Test Entry
 ******************************************************************************/
int main(void)
{
    int failures = 0;
    Dcm_DidConfigType didConfig;
    
    printf("=== Write Data By Identifier (0x2E) Service Tests ===\n\n");
    
    /* Initialize DID service */
    (void)memset(&didConfig, 0, sizeof(didConfig));
    Dcm_DidInit(&didConfig);
    
    /* Register test DIDs */
    Dcm_RegisterDid(0x0100U, &configDidInfo);
    Dcm_RegisterDid(0xF190U, &vinDidInfo);
    
    /* Run tests */
    failures += test_successful_write();
    failures += test_request_out_of_range();
    failures += test_incorrect_length();
    failures += test_security_access_denied();
    failures += test_is_did_writable();
    failures += test_incorrect_data_length();
    
    printf("\n=== Test Summary ===\n");
    printf("Failures: %d\n", failures);
    
    return failures;
}

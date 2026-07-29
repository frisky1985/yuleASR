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
 * @file    test_io_control.c
 * @brief   Test file for DCM IO Control Service (0x2F)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "dcm_io_control.h"
#include "dcm_types.h"

/* Mock implementations for dependencies */
Dcm_SessionType Dcm_GetCurrentSession(void);
Dcm_SessionType Dcm_GetCurrentSession(void) {
    return DCM_SESSION_EXTENDED;
}

bool Dcm_IsSecurityLevelUnlocked(uint8_t level) {
    return true;
}

/* Test function prototypes */
void test_io_control_init(void);
void test_io_control_return_to_ecu(void);
void test_io_control_reset_to_default(void);
void test_io_control_freeze_state(void);
void test_io_control_short_term_adjustment(void);
void test_io_control_invalid_did(void);
void test_io_control_invalid_control_type(void);

/* Test utilities */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            tests_passed++; \
            printf("  [PASS] %s\n", msg); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s\n", msg); \
        } \
    } while(0)

/******************************************************************************
 * Test Implementations
 ******************************************************************************/

void test_io_control_init(void) {
    printf("\n=== Test: IO Control Init ===\n");
    
    Dcm_ReturnType result = Dcm_IoControlInit(NULL_PTR, 0);
    TEST_ASSERT(result == DCM_E_OK, "Init with defaults should succeed");
    
    Dcm_IoControlStateType state = Dcm_GetIoControlState();
    TEST_ASSERT(state == DCM_IO_STATE_ECU_CONTROL, "Initial state should be ECU_CONTROL");
    
    TEST_ASSERT(!Dcm_IsIoUnderDiagnosticControl(), "Should not be under diagnostic control initially");
}

void test_io_control_return_to_ecu(void) {
    printf("\n=== Test: Return to ECU (0x00) ===\n");
    
    uint8_t requestData[4] = {0x2F, 0xF1, 0x01, 0x00}; /* SID, DID=0xF101, returnToECU */
    uint8_t responseData[32];
    
    Dcm_RequestType request = {
        .data = requestData,
        .length = 4,
        .sourceAddress = 0x01,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN
    };
    
    Dcm_ResponseType response = {
        .data = responseData,
        .maxLength = 32,
        .isNegativeResponse = false
    };
    
    Dcm_ReturnType result = Dcm_InputOutputControlByIdentifier(&request, &response);
    TEST_ASSERT(result == DCM_E_OK, "Return to ECU should succeed");
    TEST_ASSERT(!response.isNegativeResponse, "Should be positive response");
    TEST_ASSERT(response.data[0] == 0x6F, "Response SID should be 0x6F");
    TEST_ASSERT(response.data[1] == 0xF1, "Response DID high byte");
    TEST_ASSERT(response.data[2] == 0x01, "Response DID low byte");
}

void test_io_control_reset_to_default(void) {
    printf("\n=== Test: Reset to Default (0x01) ===\n");
    
    uint8_t requestData[4] = {0x2F, 0xF1, 0x01, 0x01}; /* SID, DID=0xF101, resetToDefault */
    uint8_t responseData[32];
    
    Dcm_RequestType request = {
        .data = requestData,
        .length = 4,
        .sourceAddress = 0x01,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN
    };
    
    Dcm_ResponseType response = {
        .data = responseData,
        .maxLength = 32,
        .isNegativeResponse = false
    };
    
    Dcm_ReturnType result = Dcm_InputOutputControlByIdentifier(&request, &response);
    TEST_ASSERT(result == DCM_E_OK, "Reset to default should succeed");
    TEST_ASSERT(!response.isNegativeResponse, "Should be positive response");
    
    Dcm_IoControlStateType state = Dcm_GetIoControlState();
    TEST_ASSERT(state == DCM_IO_STATE_DEFAULT, "State should be DEFAULT");
}

void test_io_control_freeze_state(void) {
    printf("\n=== Test: Freeze Current State (0x02) ===\n");
    
    uint8_t requestData[4] = {0x2F, 0xF1, 0x01, 0x02}; /* SID, DID=0xF101, freezeState */
    uint8_t responseData[32];
    
    Dcm_RequestType request = {
        .data = requestData,
        .length = 4,
        .sourceAddress = 0x01,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN
    };
    
    Dcm_ResponseType response = {
        .data = responseData,
        .maxLength = 32,
        .isNegativeResponse = false
    };
    
    Dcm_ReturnType result = Dcm_InputOutputControlByIdentifier(&request, &response);
    TEST_ASSERT(result == DCM_E_OK, "Freeze state should succeed");
    TEST_ASSERT(!response.isNegativeResponse, "Should be positive response");
    
    Dcm_IoControlStateType state = Dcm_GetIoControlState();
    TEST_ASSERT(state == DCM_IO_STATE_FROZEN, "State should be FROZEN");
}

void test_io_control_short_term_adjustment(void) {
    printf("\n=== Test: Short Term Adjustment (0x03) ===\n");
    
    /* SID + DID(2) + controlType + controlState(2) */
    uint8_t requestData[7] = {0x2F, 0xF1, 0x01, 0x03, 0x12, 0x34, 0xFF};
    uint8_t responseData[32];
    
    Dcm_RequestType request = {
        .data = requestData,
        .length = 7,
        .sourceAddress = 0x01,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN
    };
    
    Dcm_ResponseType response = {
        .data = responseData,
        .maxLength = 32,
        .isNegativeResponse = false
    };
    
    Dcm_ReturnType result = Dcm_InputOutputControlByIdentifier(&request, &response);
    TEST_ASSERT(result == DCM_E_OK, "Short term adjustment should succeed");
    TEST_ASSERT(!response.isNegativeResponse, "Should be positive response");
    
    Dcm_IoControlStateType state = Dcm_GetIoControlState();
    TEST_ASSERT(state == DCM_IO_STATE_UNDER_DIAGNOSTIC_CONTROL, "State should be UNDER_DIAGNOSTIC_CONTROL");
}

void test_io_control_invalid_did(void) {
    printf("\n=== Test: Invalid DID (NRC 0x31) ===\n");
    
    uint8_t requestData[4] = {0x2F, 0x99, 0x99, 0x00}; /* SID, invalid DID, returnToECU */
    uint8_t responseData[32];
    
    Dcm_RequestType request = {
        .data = requestData,
        .length = 4,
        .sourceAddress = 0x01,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN
    };
    
    Dcm_ResponseType response = {
        .data = responseData,
        .maxLength = 32,
        .isNegativeResponse = false
    };
    
    Dcm_ReturnType result = Dcm_InputOutputControlByIdentifier(&request, &response);
    TEST_ASSERT(result != DCM_E_OK, "Invalid DID should fail");
    TEST_ASSERT(response.isNegativeResponse, "Should be negative response");
    TEST_ASSERT(response.negativeResponseCode == UDS_NRC_REQUEST_OUT_OF_RANGE, "NRC should be requestOutOfRange (0x31)");
}

void test_io_control_invalid_control_type(void) {
    printf("\n=== Test: Invalid Control Type (NRC 0x31) ===\n");
    
    uint8_t requestData[4] = {0x2F, 0xF1, 0x01, 0x99}; /* SID, DID, invalid control type */
    uint8_t responseData[32];
    
    Dcm_RequestType request = {
        .data = requestData,
        .length = 4,
        .sourceAddress = 0x01,
        .addrMode = DCM_ADDR_PHYSICAL,
        .protocol = DCM_PROTOCOL_UDS_ON_CAN
    };
    
    Dcm_ResponseType response = {
        .data = responseData,
        .maxLength = 32,
        .isNegativeResponse = false
    };
    
    Dcm_ReturnType result = Dcm_InputOutputControlByIdentifier(&request, &response);
    TEST_ASSERT(result != DCM_E_OK, "Invalid control type should fail");
    TEST_ASSERT(response.isNegativeResponse, "Should be negative response");
    TEST_ASSERT(response.negativeResponseCode == UDS_NRC_REQUEST_OUT_OF_RANGE, "NRC should be requestOutOfRange (0x31)");
}

void test_io_control_type_validation(void) {
    printf("\n=== Test: Control Type Validation ===\n");
    
    TEST_ASSERT(Dcm_IsIoControlTypeValid(DCM_IO_CTRL_RETURN_TO_ECU), "0x00 should be valid");
    TEST_ASSERT(Dcm_IsIoControlTypeValid(DCM_IO_CTRL_RESET_TO_DEFAULT), "0x01 should be valid");
    TEST_ASSERT(Dcm_IsIoControlTypeValid(DCM_IO_CTRL_FREEZE_CURRENT_STATE), "0x02 should be valid");
    TEST_ASSERT(Dcm_IsIoControlTypeValid(DCM_IO_CTRL_SHORT_TERM_ADJUSTMENT), "0x03 should be valid");
    TEST_ASSERT(!Dcm_IsIoControlTypeValid(0x04), "0x04 should be invalid");
    TEST_ASSERT(!Dcm_IsIoControlTypeValid(0xFF), "0xFF should be invalid");
}

void test_io_control_support_check(void) {
    printf("\n=== Test: IO Control Support Check ===\n");
    
    TEST_ASSERT(Dcm_IsIoControlSupported(DCM_IO_CTRL_DID_ECU_VOLTAGE), "ECU voltage DID should be supported");
    TEST_ASSERT(Dcm_IsIoControlSupported(DCM_IO_CTRL_DID_ENGINE_RPM), "Engine RPM DID should be supported");
    TEST_ASSERT(!Dcm_IsIoControlSupported(0x9999), "Unknown DID should not be supported");
}

/******************************************************************************
 * Main
 ******************************************************************************/
int main(void) {
    printf("========================================\n");
    printf("DCM IO Control Service (0x2F) Tests\n");
    printf("========================================\n");
    
    test_io_control_init();
    test_io_control_return_to_ecu();
    test_io_control_reset_to_default();
    test_io_control_freeze_state();
    test_io_control_short_term_adjustment();
    test_io_control_invalid_did();
    test_io_control_invalid_control_type();
    test_io_control_type_validation();
    test_io_control_support_check();
    
    printf("\n========================================\n");
    printf("Test Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");
    
    return (tests_failed > 0U ) ? 1 : 0;
}

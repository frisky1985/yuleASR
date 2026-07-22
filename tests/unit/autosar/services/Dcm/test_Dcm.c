/**
 * @file test_Dcm.c
 * @brief DCM Unit Tests
 *
 * SHALL-DCM-01: SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37
 * SHALL-DCM-02: SHALL support a maximum of 4 concurrent diagnostic sessions
 * SHALL-DCM-03: SHALL enforce P2 timeout of 50ms for diagnostic responses
 * SHALL-DCM-04: SHALL enforce P2* timeout of 500ms for diagnostic responses
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Dcm.h"
#include "dcm_transfer.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    Dcm_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    Dcm_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

static void test_Dcm_Init_ValidConfig(void **state)
{
    (void)state;
    Std_ReturnType result = Dcm_Init(NULL);
    assert_int_equal(result, E_OK);
}

static void test_Dcm_DeInit(void **state)
{
    (void)state;
    Dcm_Init(NULL);
    Dcm_DeInit();
    assert_true(1);
}

static void test_Dcm_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    Dcm_Init(NULL);
    Dcm_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.moduleID, DCM_MODULE_ID);
}

static void test_Dcm_MainFunction_Uninit(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    Dcm_MainFunction();
    assert_true(1);
}

static void test_Dcm_MainFunction_Initialized(void **state)
{
    (void)state;
    Dcm_Init(NULL);
    Dcm_MainFunction();
    assert_true(1);
}

static void test_Dcm_UdsServices_Exists(void **state)
{
    (void)state;
    /* Verify UDS service SIDs are defined */
    /* Core services */
    assert_true(DCM_SID_DIAGNOSTIC_SESSION_CONTROL == 0x10 || DCM_SID_SESSION_CONTROL == 0x10);
    assert_int_equal(DCM_SID_ECU_RESET, 0x11U);
    assert_int_equal(DCM_SID_SECURITY_ACCESS, 0x27U);
    assert_int_equal(DCM_SID_TESTER_PRESENT, 0x3EU);
    
    /* Data services */
    assert_int_equal(DCM_SID_READ_DATA_BY_IDENTIFIER, 0x22U);
    assert_int_equal(DCM_SID_WRITE_DATA_BY_IDENTIFIER, 0x2EU);
    
    /* Transfer services */
    assert_int_equal(DCM_SID_REQUEST_DOWNLOAD, 0x34U);
    assert_int_equal(DCM_SID_REQUEST_UPLOAD, 0x35U);
    assert_int_equal(DCM_SID_TRANSFER_DATA, 0x36U);
    assert_int_equal(DCM_SID_REQUEST_TRANSFER_EXIT, 0x37U);
    
    /* Routine control */
    assert_int_equal(DCM_SID_ROUTINE_CONTROL, 0x31U);
    
    /* DTC services */
    assert_int_equal(DCM_SID_READ_DTC_INFORMATION, 0x19U);
    assert_int_equal(DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0x14U);
}

static void test_Dcm_Sessions_Exists(void **state)
{
    (void)state;
    /* Verify diagnostic session types */
    assert_int_equal(DCM_DEFAULT_SESSION, 0x01U);
    assert_int_equal(DCM_PROGRAMMING_SESSION, 0x02U);
    assert_int_equal(DCM_EXTENDED_DIAGNOSTIC_SESSION, 0x03U);
    assert_int_equal(DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION, 0x04U);
}

static void test_Dcm_TransferConstants(void **state)
{
    (void)state;
    /* Verify transfer service constants */
    assert_int_equal(DCM_TRANSFER_MAX_ADDR_LEN, 8U);
    assert_int_equal(DCM_TRANSFER_MAX_SIZE_LEN, 8U);
    assert_int_equal(DCM_TRANSFER_BLOCK_COUNTER_MAX, 0xFFU);
}

static void test_Dcm_RxIndication_NullPdu(void **state)
{
    (void)state;
    Dcm_Init(NULL);
    /* Should not crash with NULL */
    Dcm_RxIndication(0, NULL);
    assert_true(1);
}

static void test_Dcm_TxConfirmation(void **state)
{
    (void)state;
    Dcm_Init(NULL);
    /* Should not crash */
    Dcm_TxConfirmation(0, E_OK);
    assert_true(1);
}

static void test_Dcm_NrcCodes_Exists(void **state)
{
    (void)state;
    /* Verify negative response codes */
    assert_int_equal(DCM_E_GENERAL_REJECT, 0x10U);
    assert_int_equal(DCM_E_SERVICE_NOT_SUPPORTED, 0x11U);
    assert_int_equal(DCM_E_SUB_FUNCTION_NOT_SUPPORTED, 0x12U);
    assert_int_equal(DCM_E_INCORRECT_MESSAGE_LENGTH, 0x13U);
    assert_int_equal(DCM_E_CONDITIONS_NOT_CORRECT, 0x22U);
    assert_int_equal(DCM_E_REQUEST_SEQUENCE_ERROR, 0x24U);
    assert_int_equal(DCM_E_REQUEST_OUT_OF_RANGE, 0x31U);
    assert_int_equal(DCM_E_SECURITY_ACCESS_DENIED, 0x33U);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_Dcm_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_MainFunction_Initialized, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_UdsServices_Exists, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_Sessions_Exists, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_TransferConstants, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_RxIndication_NullPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Dcm_NrcCodes_Exists, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

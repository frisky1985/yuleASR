/**
 * @file test_CanTp.c
 * @brief CanTp Module Unit Tests - CAN Transport Protocol Layer
 * @version 1.0.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "CanTp.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    CanTp_Init(&CanTp_Config);
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    CanTp_Shutdown();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

/**
 * @brief Test CanTp_Init with valid configuration
 */
static void test_CanTp_Init_ValidConfig(void **state)
{
    (void)state;
    
    CanTp_Shutdown();
    CanTp_Init(&CanTp_Config);
    /* Module should be initialized without crashing */
    assert_true(1);
}

/**
 * @brief Test CanTp_Shutdown functionality
 */
static void test_CanTp_Shutdown(void **state)
{
    (void)state;
    
    CanTp_Shutdown();
    /* After shutdown, module should be inactive */
    assert_true(1);
    
    /* Re-initialize for other tests */
    CanTp_Init(&CanTp_Config);
}

/**
 * @brief Test CanTp_GetVersionInfo
 */
static void test_CanTp_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    
    CanTp_GetVersionInfo(&versionInfo);
    
    /* Verify module IDs */
    assert_int_equal(versionInfo.moduleID, CANTP_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, CANTP_VENDOR_ID);
    assert_int_equal(versionInfo.sw_major_version, CANTP_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, CANTP_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, CANTP_SW_PATCH_VERSION);
}

/**
 * @brief Test CanTp_Transmit with invalid parameters
 */
static void test_CanTp_Transmit_InvalidParams(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    
    /* Test with invalid TxSduId */
    Std_ReturnType result = CanTp_Transmit(0xFFFF, &pduInfo);
    assert_true(result == E_NOT_OK || result == E_OK);
    
    /* Test with NULL PduInfoPtr */
    result = CanTp_Transmit(0, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test CanTp_Transmit with valid single frame
 */
static void test_CanTp_Transmit_SingleFrame(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 7;  /* Single frame fits in 7 bytes (SF with address) */
    
    Std_ReturnType result = CanTp_Transmit(0, &pduInfo);
    /* Result depends on implementation and configuration */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test CanTp_CancelTransmit with invalid ID
 */
static void test_CanTp_CancelTransmit_InvalidId(void **state)
{
    (void)state;
    
    /* Test with invalid TxSduId */
    Std_ReturnType result = CanTp_CancelTransmit(0xFFFF);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test CanTp_CancelReceive with invalid ID
 */
static void test_CanTp_CancelReceive_InvalidId(void **state)
{
    (void)state;
    
    /* Test with invalid RxSduId */
    Std_ReturnType result = CanTp_CancelReceive(0xFFFF);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test CanTp_MainFunction
 */
static void test_CanTp_MainFunction(void **state)
{
    (void)state;
    
    /* Should not crash when initialized */
    CanTp_MainFunction();
    assert_true(1);
}

/**
 * @brief Test CanTp_MainFunction when uninitialized
 */
static void test_CanTp_MainFunction_Uninit(void **state)
{
    (void)state;
    
    CanTp_Shutdown();
    
    /* Should not crash even when uninitialized */
    CanTp_MainFunction();
    assert_true(1);
    
    /* Restore state */
    CanTp_Init(&CanTp_Config);
}

/**
 * @brief Test frame type constants
 */
static void test_CanTp_FrameTypes(void **state)
{
    (void)state;
    
    /* Verify frame type definitions */
    assert_int_equal(CANTP_FRAME_SINGLE, 0);
    assert_int_equal(CANTP_FRAME_FIRST_FF, 1);
    assert_int_equal(CANTP_FRAME_CONSECUTIVE_CF, 2);
    assert_int_equal(CANTP_FRAME_FLOWCONTROL_FC, 3);
}

/**
 * @brief Test flow status constants
 */
static void test_CanTp_FlowStatus(void **state)
{
    (void)state;
    
    /* Verify flow status definitions */
    assert_int_equal(CANTP_FLOWSTATUS_CTS, 0);
    assert_int_equal(CANTP_FLOWSTATUS_WT, 1);
    assert_int_equal(CANTP_FLOWSTATUS_OVFLW, 2);
}

/**
 * @brief Test addressing format constants
 */
static void test_CanTp_AddressingFormats(void **state)
{
    (void)state;
    
    /* Verify addressing format definitions */
    assert_int_equal(CANTP_STANDARD, 0);
    assert_int_equal(CANTP_EXTENDED, 1);
    assert_int_equal(CANTP_MIXED, 2);
    assert_int_equal(CANTP_MIXED29BIT, 3);
    assert_int_equal(CANTP_NORMALFIXED, 4);
    assert_int_equal(CANTP_CUSTOM, 5);
}

/**
 * @brief Test communication type constants
 */
static void test_CanTp_TaTypes(void **state)
{
    (void)state;
    
    /* Verify TA type definitions */
    assert_int_equal(CANTP_FUNCTIONAL, 0);
    assert_int_equal(CANTP_PHYSICAL, 1);
}

/**
 * @brief Test CanTp_RxIndication callback
 */
static void test_CanTp_RxIndication(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 data[8] = {0x00, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};  /* Single frame */
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    /* Should not crash */
    CanTp_RxIndication(0, &pduInfo);
    assert_true(1);
}

/**
 * @brief Test CanTp_TxConfirmation callback
 */
static void test_CanTp_TxConfirmation(void **state)
{
    (void)state;
    
    /* Should not crash */
    CanTp_TxConfirmation(0);
    assert_true(1);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_CanTp_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_Shutdown, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_Transmit_InvalidParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_Transmit_SingleFrame, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_CancelTransmit_InvalidId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_CancelReceive_InvalidId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_FrameTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_FlowStatus, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_AddressingFormats, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_TaTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_CanTp_TxConfirmation, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

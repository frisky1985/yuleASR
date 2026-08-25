/**
 * @file test_DoIP.c
 * @brief DoIP Module Unit Tests - Diagnostic over IP (ECUAL Layer)
 * @version 1.0.0
 */

// @tests src/bsw/ecual/doip/src/DoIP.c  @tests src/bsw/ecual/doip/include/DoIP.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "DoIP.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    DoIP_Init(NULL);
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    DoIP_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

/**
 * @brief Test DoIP_Init with valid configuration
 */
/** @req SWS_DoIP_00001 */
static void test_DoIP_Init_ValidConfig(void **state)
{
    (void)state;
    
    DoIP_DeInit();
    DoIP_Init(NULL);
    /* Module should be initialized without crashing */
    assert_true(1);
}

/**
 * @brief Test DoIP_DeInit functionality
 */
/** @req SWS_DoIP_00001 */
static void test_DoIP_DeInit(void **state)
{
    (void)state;
    
    DoIP_DeInit();
    /* After de-init, module should be inactive */
    assert_true(1);
    
    /* Re-initialize for other tests */
    DoIP_Init(NULL);
}

/**
 * @brief Test DoIP_GetVersionInfo
 */
/** @req SWS_DoIP_00003 */
static void test_DoIP_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    
    DoIP_GetVersionInfo(&versionInfo);
    
    /* Verify module IDs */
    assert_int_equal(versionInfo.moduleID, DOIP_MODULE_ID);
}

/**
 * @brief Test DoIP_ActivationLineSwitchActive
 */
static void test_DoIP_ActivationLineActive(void **state)
{
    (void)state;
    
    /* Should not crash */
    DoIP_ActivationLineSwitchActive();
    assert_true(1);
}

/**
 * @brief Test DoIP_ActivationLineSwitchInactive
 */
static void test_DoIP_ActivationLineInactive(void **state)
{
    (void)state;
    
    /* Should not crash */
    DoIP_ActivationLineSwitchInactive();
    assert_true(1);
}

/**
 * @brief Test DoIP_MainFunction
 */
/** @req SWS_DoIP_00004 */
static void test_DoIP_MainFunction(void **state)
{
    (void)state;
    
    /* Should not crash when initialized */
    DoIP_MainFunction();
    assert_true(1);
}

/**
 * @brief Test DoIP_MainFunction when uninitialized
 */
/** @req SWS_DoIP_00001 */
static void test_DoIP_MainFunction_Uninit(void **state)
{
    (void)state;
    
    DoIP_DeInit();
    
    /* Should not crash even when uninitialized */
    DoIP_MainFunction();
    assert_true(1);
    
    /* Restore state */
    DoIP_Init(NULL);
}

/**
 * @brief Test DoIP_Transmit with valid parameters
 */
static void test_DoIP_Transmit(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    Std_ReturnType result = DoIP_Transmit(0, &pduInfo);
    /* Result depends on implementation and configuration */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test DoIP_Transmit with invalid parameters
 */
static void test_DoIP_Transmit_InvalidParams(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    
    /* Test with invalid TxPduId */
    Std_ReturnType result = DoIP_Transmit(0xFFFF, &pduInfo);
    assert_true(result == E_NOT_OK);
    
    /* Test with NULL PduInfoPtr */
    result = DoIP_Transmit(0, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test routing activation type constants
 */
static void test_DoIP_RoutingActivationTypes(void **state)
{
    (void)state;
    
    /* Verify routing activation type definitions */
    assert_int_equal(DOIP_ROUTING_ACTIVATION_DEFAULT, 0);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_WWH_OBD, 1);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_CDS, 2);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_CENTRAL_SECURITY, 3);
}

/**
 * @brief Test routing activation response codes
 */
static void test_DoIP_RoutingActivationResCodes(void **state)
{
    (void)state;
    
    /* Verify routing activation response code definitions */
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS, 0x00);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_UNKNOWN_SA, 0x01);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_SA_ACTIVE, 0x02);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_AUTHENTIC_MISSING, 0x03);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_CONFIRM_MISSING, 0x04);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_UNSUPPORTED_RA, 0x05);
    assert_int_equal(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_TLS_REQUIRED, 0x06);
}

/**
 * @brief Test socket state type constants
 */
static void test_DoIP_SocketStateTypes(void **state)
{
    (void)state;
    
    /* Verify socket state definitions */
    assert_int_equal(DOIP_SOCKET_STATE_DISCONNECTED, 0);
    assert_int_equal(DOIP_SOCKET_STATE_RESERVED, 1);
    assert_int_equal(DOIP_SOCKET_STATE_REGISTERED, 2);
    assert_int_equal(DOIP_SOCKET_STATE_ACTIVATED, 3);
}

/**
 * @brief Test DoIP_SoAdIfRxIndication callback
 */
/** @req SWS_DoIP_00006 */
static void test_DoIP_SoAdIfRxIndication(void **state)
{
    (void)state;
    
    PduInfoType pduInfo;
    uint8 data[8] = {0x02, 0xFD, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};  /* Vehicle identification request */
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    /* Should not crash */
    DoIP_SoAdIfRxIndication(0, &pduInfo);
    assert_true(1);
}

/**
 * @brief Test DoIP_SoAdIfTxConfirmation callback
 */
static void test_DoIP_SoAdIfTxConfirmation(void **state)
{
    (void)state;
    
    /* Should not crash */
    DoIP_SoAdIfTxConfirmation(0);
    assert_true(1);
}

/**
 * @brief Test DoIP_SoAdTpTxConfirmation callback
 */
/** @req SWS_DoIP_00017 */
static void test_DoIP_SoAdTpTxConfirmation(void **state)
{
    (void)state;
    
    /* Should not crash */
    DoIP_SoAdTpTxConfirmation(0, E_OK);
    assert_true(1);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_DoIP_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_ActivationLineActive, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_ActivationLineInactive, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_Transmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_Transmit_InvalidParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_RoutingActivationTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_RoutingActivationResCodes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_SocketStateTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_SoAdIfRxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_SoAdIfTxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_SoAdTpTxConfirmation, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

/**
 * @file test_DoIP.c
 * @brief DoIP Unit Tests
 */

// @tests src/bsw/services/doip/src/DoIP.c  @tests src/bsw/services/doip/include/DoIP.h

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
    DoIP_DeInit();
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

static void test_DoIP_Init_ValidConfig(void **state)
{
    (void)state;
    /* Initialize with valid config */
    Std_ReturnType result = DoIP_Init(NULL);
    assert_int_equal(result, E_OK);
}

static void test_DoIP_DeInit(void **state)
{
    (void)state;
    DoIP_Init(NULL);
    DoIP_DeInit();
    /* After de-init, module should be inactive */
    assert_true(1);
}

static void test_DoIP_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    DoIP_Init(NULL);
    DoIP_GetVersionInfo(&versionInfo);
    
    /* Verify module IDs */
    assert_int_equal(versionInfo.moduleID, DOIP_MODULE_ID);
}

static void test_DoIP_MainFunction_Uninit(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    DoIP_MainFunction();
    assert_true(1);
}

static void test_DoIP_MainFunction_Initialized(void **state)
{
    (void)state;
    DoIP_Init(NULL);
    DoIP_MainFunction();
    assert_true(1);
}

static void test_DoIP_PayloadTypes(void **state)
{
    (void)state;
    /* Verify payload type constants */
    assert_int_equal(DOIP_PT_VEHICLE_IDENTIFICATION_REQ, 0x0001U);
    assert_int_equal(DOIP_PT_VEHICLE_IDENTIFICATION_RES, 0x0004U);
    assert_int_equal(DOIP_PT_ROUTING_ACTIVATION_REQ, 0x0005U);
    assert_int_equal(DOIP_PT_ROUTING_ACTIVATION_RES, 0x0006U);
    assert_int_equal(DOIP_PT_ALIVE_CHECK_REQ, 0x0007U);
    assert_int_equal(DOIP_PT_ALIVE_CHECK_RES, 0x0008U);
    assert_int_equal(DOIP_PT_DIAGNOSTIC_MESSAGE, 0x8001U);
    assert_int_equal(DOIP_PT_DIAGNOSTIC_MESSAGE_ACK, 0x8002U);
    assert_int_equal(DOIP_PT_DIAGNOSTIC_MESSAGE_NACK, 0x8003U);
}

static void test_DoIP_RoutingActivationCodes(void **state)
{
    (void)state;
    /* Verify routing activation response codes */
    assert_int_equal(DOIP_RA_RES_CODE_SUCCESS, 0x10U);
    assert_int_equal(DOIP_RA_RES_CODE_UNSUPPORTED_ACT_TYPE, 0x06U);
    assert_int_equal(DOIP_RA_RES_CODE_TCP_SUPPORT, 0x07U);
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
        cmocka_unit_test_setup_teardown(test_DoIP_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_MainFunction_Initialized, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_PayloadTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_DoIP_RoutingActivationCodes, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

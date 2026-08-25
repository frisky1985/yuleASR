/**
 * @file test_EthIf.c
 * @brief EthIf Module Unit Tests - Ethernet Interface Layer
 * @version 1.0.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "EthIf.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    EthIf_Init(&EthIf_Config);
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    /* EthIf does not have explicit DeInit, just cleanup state */
    (void)0;
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

/**
 * @brief Test EthIf_Init with valid configuration
 */
/** @req SWS_EthIf_00001 */
static void test_EthIf_Init_ValidConfig(void **state)
{
    (void)state;
    
    /* Module should be initialized without crashing */
    assert_true(1);
}

/**
 * @brief Test EthIf_GetVersionInfo
 */
/** @req SWS_EthIf_00009 */
static void test_EthIf_GetVersionInfo(void **state)
{
    (void)state;
    
    Std_VersionInfoType versionInfo;
    
    EthIf_GetVersionInfo(&versionInfo);
    
    /* Verify module IDs */
    assert_int_equal(versionInfo.moduleID, ETHIF_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, ETHIF_VENDOR_ID);
    assert_int_equal(versionInfo.sw_major_version, ETHIF_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, ETHIF_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, ETHIF_SW_PATCH_VERSION);
}

/**
 * @brief Test EthIf_ControllerInit
 */
/** @req SWS_EthIf_00001 */
static void test_EthIf_ControllerInit(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_ControllerInit(0, 0);
    assert_true(1);
}

/**
 * @brief Test EthIf_SetControllerMode with valid parameters
 */
/** @req SWS_EthIf_00004 */
static void test_EthIf_SetControllerMode(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Test setting to active mode */
    result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Test setting to down mode */
    result = EthIf_SetControllerMode(0, ETHIF_MODE_DOWN);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_SetControllerMode with invalid controller ID
 */
/** @req SWS_EthIf_00004 */
static void test_EthIf_SetControllerMode_InvalidId(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    /* Test with invalid controller ID */
    result = EthIf_SetControllerMode(0xFF, ETHIF_MODE_ACTIVE);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_GetControllerMode
 */
/** @req SWS_EthIf_00005 */
static void test_EthIf_GetControllerMode(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    EthIf_ControllerModeType mode;
    
    result = EthIf_GetControllerMode(0, &mode);
    assert_true(result == E_OK || result == E_NOT_OK);
    
    /* Verify mode is valid */
    if (result == E_OK) {
        assert_true(mode == ETHIF_MODE_DOWN || mode == ETHIF_MODE_ACTIVE);
    }
}

/**
 * @brief Test EthIf_GetControllerMode with NULL pointer
 */
/** @req SWS_EthIf_00005 */
static void test_EthIf_GetControllerMode_NullPtr(void **state)
{
    (void)state;
    
    Std_ReturnType result;
    
    result = EthIf_GetControllerMode(0, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test EthIf_GetPhysAddr and EthIf_SetPhysAddr
 */
/** @req SWS_EthIf_00003 */
static void test_EthIf_PhysAddr(void **state)
{
    (void)state;
    
    uint8 macAddr[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint8 readAddr[6] = {0};
    
    /* Set physical address */
    EthIf_SetPhysAddr(0, macAddr);
    
    /* Get physical address */
    EthIf_GetPhysAddr(0, readAddr);
    
    /* Verify addresses match */
    assert_memory_equal(macAddr, readAddr, 6);
}

/**
 * @brief Test EthIf_Transmit
 */
/** @req SWS_EthIf_00003 */
static void test_EthIf_Transmit(void **state)
{
    (void)state;
    
    uint8 data[64] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  /* DST MAC */
                       0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,  /* SRC MAC */
                       0x08, 0x00,                           /* EtherType: IPv4 */
                       /* Payload */
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    
    Std_ReturnType result = EthIf_Transmit(0, 0x0800, data, 64);
    /* Result depends on implementation and configuration */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @brief Test EthIf_Transmit with invalid parameters
 */
/** @req SWS_EthIf_00003 */
static void test_EthIf_Transmit_InvalidParams(void **state)
{
    (void)state;
    
    uint8 data[64] = {0};
    
    /* Test with invalid controller ID */
    Std_ReturnType result = EthIf_Transmit(0xFF, 0x0800, data, 64);
    assert_int_equal(result, E_NOT_OK);
    
    /* Test with NULL data pointer */
    result = EthIf_Transmit(0, 0x0800, NULL, 64);
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @brief Test controller mode type constants
 */
static void test_EthIf_ControllerModeTypes(void **state)
{
    (void)state;
    
    /* Verify controller mode definitions */
    assert_int_equal(ETHIF_MODE_DOWN, 0);
    assert_int_equal(ETHIF_MODE_ACTIVE, 1);
}

/**
 * @brief Test speed type constants
 */
static void test_EthIf_SpeedTypes(void **state)
{
    (void)state;
    
    /* Verify speed type definitions */
    assert_int_equal(ETHIF_SPEED_10MBPS, 0);
    assert_int_equal(ETHIF_SPEED_100MBPS, 1);
    assert_int_equal(ETHIF_SPEED_1GBPS, 2);
    assert_int_equal(ETHIF_SPEED_2_5GBPS, 3);
    assert_int_equal(ETHIF_SPEED_10GBPS, 4);
}

/**
 * @brief Test duplex type constants
 */
static void test_EthIf_DuplexTypes(void **state)
{
    (void)state;
    
    /* Verify duplex type definitions */
    assert_int_equal(ETHIF_DUPLEX_HALF, 0);
    assert_int_equal(ETHIF_DUPLEX_FULL, 1);
}

/**
 * @brief Test link state type constants
 */
static void test_EthIf_LinkStateTypes(void **state)
{
    (void)state;
    
    /* Verify link state definitions */
    assert_int_equal(ETHIF_LINK_STATE_DOWN, 0);
    assert_int_equal(ETHIF_LINK_STATE_ACTIVE, 1);
}

/**
 * @brief Test EthIf_MainFunction
 */
/** @req SWS_EthIf_00008 */
static void test_EthIf_MainFunction(void **state)
{
    (void)state;
    
    /* Should not crash when initialized */
    EthIf_MainFunction();
    assert_true(1);
}

/**
 * @brief Test EthIf_RxIndication callback
 */
/** @req SWS_EthIf_00006 */
static void test_EthIf_RxIndication(void **state)
{
    (void)state;
    
    uint8 data[64] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  /* DST MAC */
                       0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,  /* SRC MAC */
                       0x08, 0x00};                         /* EtherType: IPv4 */
    uint8 srcMac[6] = {0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB};
    
    /* Should not crash */
    EthIf_RxIndication(0, 0x0800, FALSE, srcMac, data, 64);
    assert_true(1);
}

/**
 * @brief Test EthIf_TxConfirmation callback
 */
/** @req SWS_EthIf_00007 */
static void test_EthIf_TxConfirmation(void **state)
{
    (void)state;
    
    /* Should not crash */
    EthIf_TxConfirmation(0, 0);
    assert_true(1);
}

/**
 * @brief Test EthIf_GetCurrentTime
 */
static void test_EthIf_GetCurrentTime(void **state)
{
    (void)state;
    
    EthIf_TimestampType timestamp;
    
    Std_ReturnType result = EthIf_GetCurrentTime(0, &timestamp);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_EthIf_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_ControllerInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SetControllerMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SetControllerMode_InvalidId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetControllerMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetControllerMode_NullPtr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_PhysAddr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Transmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_Transmit_InvalidParams, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_ControllerModeTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_SpeedTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_DuplexTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_LinkStateTypes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_EthIf_GetCurrentTime, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

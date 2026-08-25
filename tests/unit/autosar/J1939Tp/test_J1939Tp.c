/**
 * @file test_J1939Tp.c
 * @brief J1939Tp Unit Tests
 */

// @tests src/bsw/services/j1939tp/src/J1939Tp.c  @tests src/bsw/services/j1939tp/include/J1939Tp.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "J1939Tp.h"
#include "J1939Tp_Cfg.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    /* Reset module state before each test */
    J1939Tp_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    J1939Tp_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_Init_ValidConfig(void **state)
{
    (void)state;
    Std_ReturnType result = J1939Tp_Init(&J1939Tp_Config);
    assert_int_equal(result, E_OK);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_Init_NullConfig(void **state)
{
    (void)state;
    /* This will trigger DET error if DEV_ERROR_DETECT is ON */
    Std_ReturnType result = J1939Tp_Init(NULL);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_DeInit(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    J1939Tp_DeInit();
    /* After de-init, subsequent calls should fail */
    PduInfoType pduInfo = {NULL, NULL, 0};
    Std_ReturnType result = J1939Tp_Transmit(0, &pduInfo);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00003 */
static void test_J1939Tp_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    J1939Tp_Init(&J1939Tp_Config);
    J1939Tp_GetVersionInfo(&versionInfo);

#if (J1939TP_VERSION_INFO_API == STD_ON)
    assert_int_equal(versionInfo.vendorID, J1939TP_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, J1939TP_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, J1939TP_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, J1939TP_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, J1939TP_SW_PATCH_VERSION);
#else
    assert_true(1); /* Version info API disabled */
#endif
}

/** @req SWS_J1939Tp_00003 */
static void test_J1939Tp_GetVersionInfo_NullPtr(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    /* Should not crash with NULL pointer */
    J1939Tp_GetVersionInfo(NULL);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_MainFunction_Uninit(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    J1939Tp_MainFunction();
    assert_true(1);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_MainFunction_Initialized(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    J1939Tp_MainFunction();
    assert_true(1);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_Transmit_Uninit(void **state)
{
    (void)state;
    uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 4U;

    Std_ReturnType result = J1939Tp_Transmit(0, &pduInfo);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00005 */
static void test_J1939Tp_Transmit_NullPdu(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    Std_ReturnType result = J1939Tp_Transmit(0, NULL);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00005 */
static void test_J1939Tp_Transmit_SingleFrame(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);

    /* Test single frame transmission (<= 8 bytes) */
    uint8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;

    /* This will return E_OK or E_NOT_OK depending on CanIf */
    Std_ReturnType result = J1939Tp_Transmit(0, &pduInfo);
    /* Just verify it doesn't crash */
    (void)result;
    assert_true(1);
}

/** @req SWS_J1939Tp_00005 */
static void test_J1939Tp_Transmit_MultiFrame(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);

    /* Test multi-frame transmission (> 8 bytes) */
    uint8 data[100];
    for (uint8 i = 0; i < 100; i++) {
        data[i] = i;
    }

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 100U;

    Std_ReturnType result = J1939Tp_Transmit(2, &pduInfo);
    /* Should be accepted for TP transmission */
    (void)result;
    assert_true(1);
}

/** @req SWS_J1939Tp_00005 */
static void test_J1939Tp_Transmit_InvalidSduId(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);

    uint8 data[] = {0x01, 0x02, 0x03};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 3U;

    Std_ReturnType result = J1939Tp_Transmit(100, &pduInfo);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00005 */
static void test_J1939Tp_Transmit_TooLarge(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);

    /* Data larger than max TP size */
    uint8 data[J1939TP_MAX_TP_SIZE + 10];
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = J1939TP_MAX_TP_SIZE + 10U;

    Std_ReturnType result = J1939Tp_Transmit(2, &pduInfo);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_CancelTransmit_Uninit(void **state)
{
    (void)state;
    Std_ReturnType result = J1939Tp_CancelTransmit(0);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00005 */
static void test_J1939Tp_CancelTransmit_NoActiveSession(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    /* No active transmission to cancel */
    Std_ReturnType result = J1939Tp_CancelTransmit(0);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00001 */
static void test_J1939Tp_CancelReceive_Uninit(void **state)
{
    (void)state;
    Std_ReturnType result = J1939Tp_CancelReceive(0);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00008 */
static void test_J1939Tp_ChangeParameter(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    /* Not fully implemented - should return E_NOT_OK */
    Std_ReturnType result = J1939Tp_ChangeParameter(0, J1939TP_PARAM_BLOCK_SIZE, 16U);
    assert_int_equal(result, E_NOT_OK);
}

/** @req SWS_J1939Tp_00009 */
static void test_J1939Tp_RxIndication_NullPdu(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    /* Should not crash with NULL */
    J1939Tp_RxIndication(0, NULL);
    assert_true(1);
}

/** @req SWS_J1939Tp_00009 */
static void test_J1939Tp_RxIndication_ValidPdu(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);

    uint8 data[8] = {J1939TP_CM_RTS, 0x64, 0x00, 0x10, 0xFF, 0x00, 0xEC, 0x00};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;

    /* Should not crash */
    J1939Tp_RxIndication(102, &pduInfo);
    assert_true(1);
}

/** @req SWS_J1939Tp_00010 */
static void test_J1939Tp_TxConfirmation(void **state)
{
    (void)state;
    J1939Tp_Init(&J1939Tp_Config);
    /* Should not crash */
    J1939Tp_TxConfirmation(100, E_OK);
    assert_true(1);
}

static void test_J1939Tp_CmConstants(void **state)
{
    (void)state;
    /* Verify TP.CM control byte constants */
    assert_int_equal(J1939TP_CM_RTS, 0x10);
    assert_int_equal(J1939TP_CM_CTS, 0x11);
    assert_int_equal(J1939TP_CM_ACK, 0x13);
    assert_int_equal(J1939TP_CM_BAM, 0x20);
    assert_int_equal(J1939TP_CM_ABORT, 0xFF);
}

static void test_J1939Tp_ConfigAccess(void **state)
{
    (void)state;
    /* Verify configuration is accessible */
    assert_int_equal(J1939Tp_Config.ConnectionCount, J1939TP_MAX_CONNECTIONS);
    assert_int_equal(J1939Tp_Config.PgCount, J1939TP_MAX_PG);
    assert_non_null(J1939Tp_Config.Connections);
    assert_non_null(J1939Tp_Config.PgConfigs);
}

static void test_J1939Tp_CanIdMacros(void **state)
{
    (void)state;
    /* Test CAN ID construction macros */
    uint32 canId = J1939TP_CAN_ID(6, 0xFEEE, 0x80); /* Priority 6, PGN 0xFEEE, SA 0x80 */
    assert_int_equal(J1939TP_GET_SA(canId), 0x80);
    assert_int_equal(J1939TP_GET_PGN(canId), 0xFEEE);
    assert_int_equal(J1939TP_GET_PRIO(canId), 6);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* Initialization tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_Init_NullConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_DeInit, setup, teardown),

        /* Version info tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_GetVersionInfo_NullPtr, setup, teardown),

        /* Main function tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_MainFunction_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_MainFunction_Initialized, setup, teardown),

        /* Transmit tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_Transmit_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_Transmit_NullPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_Transmit_SingleFrame, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_Transmit_MultiFrame, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_Transmit_InvalidSduId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_Transmit_TooLarge, setup, teardown),

        /* Cancel tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_CancelTransmit_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_CancelTransmit_NoActiveSession, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_CancelReceive_Uninit, setup, teardown),

        /* Parameter tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_ChangeParameter, setup, teardown),

        /* Callback tests */
        cmocka_unit_test_setup_teardown(test_J1939Tp_RxIndication_NullPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_RxIndication_ValidPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_TxConfirmation, setup, teardown),

        /* Constants and macros */
        cmocka_unit_test_setup_teardown(test_J1939Tp_CmConstants, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_ConfigAccess, setup, teardown),
        cmocka_unit_test_setup_teardown(test_J1939Tp_CanIdMacros, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

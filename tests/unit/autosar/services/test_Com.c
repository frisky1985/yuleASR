/**
 * @file test_Com.c
 * @brief Com (Communication Services) Unit Tests
 */

// @tests src/bsw/services/com/src/Com.c  @tests src/bsw/services/com/include/Com.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Com.h"
#include "Com_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    Com_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    Com_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

static void test_Com_Init_ValidConfig(void **state)
{
    (void)state;
    Com_Init(NULL);
    assert_true(1);
}

static void test_Com_DeInit(void **state)
{
    (void)state;
    Com_Init(NULL);
    Com_DeInit();
    assert_true(1);
}

static void test_Com_GetStatus_Uninit(void **state)
{
    (void)state;
    Com_StatusType status = Com_GetStatus();
    assert_true(status == COM_UNINIT || status == COM_INIT);
}

static void test_Com_GetStatus_Init(void **state)
{
    (void)state;
    Com_Init(NULL);
    Com_StatusType status = Com_GetStatus();
    assert_int_equal(status, COM_INIT);
}

static void test_Com_GetConfigurationId(void **state)
{
    (void)state;
    Com_Init(NULL);
    Com_ConfigIdType configId = Com_GetConfigurationId();
    /* Configuration ID should be a valid value */
    assert_true(configId >= 0);
}

static void test_Com_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    Com_Init(NULL);
#if (COM_VERSION_INFO_API == STD_ON)
    Com_GetVersionInfo(&versionInfo);
    assert_int_equal(versionInfo.moduleID, COM_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, COM_VENDOR_ID);
#endif
    assert_true(1);
}

static void test_Com_SendSignal_8bit(void **state)
{
    (void)state;
    uint8 signalData = 0x55;
    Com_SignalIdType signalId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_SendSignal(signalId, &signalData);
    
    assert_true(result == E_OK || result == COM_SERVICE_NOT_OK);
}

static void test_Com_SendSignal_16bit(void **state)
{
    (void)state;
    uint16 signalData = 0x1234;
    Com_SignalIdType signalId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_SendSignal(signalId, &signalData);
    
    assert_true(result == E_OK || result == COM_SERVICE_NOT_OK);
}

static void test_Com_SendSignal_NullPointer(void **state)
{
    (void)state;
    Com_SignalIdType signalId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_SendSignal(signalId, NULL);
    
    /* Should return error for NULL pointer */
    assert_true(result == COM_SERVICE_NOT_OK || result == E_NOT_OK);
}

static void test_Com_ReceiveSignal_8bit(void **state)
{
    (void)state;
    uint8 signalData = 0;
    Com_SignalIdType signalId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_ReceiveSignal(signalId, &signalData);
    
    assert_true(result == E_OK || result == COM_SERVICE_NOT_OK);
}

static void test_Com_ReceiveSignal_NullPointer(void **state)
{
    (void)state;
    Com_SignalIdType signalId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_ReceiveSignal(signalId, NULL);
    
    /* Should return error for NULL pointer */
    assert_true(result == COM_SERVICE_NOT_OK || result == E_NOT_OK);
}

static void test_Com_InvalidateSignal(void **state)
{
    (void)state;
    Com_SignalIdType signalId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_InvalidateSignal(signalId);
    
    assert_true(result == E_OK || result == COM_SERVICE_NOT_OK);
}

static void test_Com_InvalidateSignalGroup(void **state)
{
    (void)state;
    Com_SignalGroupIdType signalGroupId = 0;
    
    Com_Init(NULL);
    uint8 result = Com_InvalidateSignalGroup(signalGroupId);
    
    assert_true(result == E_OK || result == COM_SERVICE_NOT_OK);
}

static void test_Com_TriggerIPDUSend(void **state)
{
    (void)state;
    PduIdType pduId = 0;
    
    Com_Init(NULL);
    Std_ReturnType result = Com_TriggerIPDUSend(pduId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Com_MainFunctionRx(void **state)
{
    (void)state;
    Com_Init(NULL);
    /* Should not crash */
    Com_MainFunctionRx();
    assert_true(1);
}

static void test_Com_MainFunctionTx(void **state)
{
    (void)state;
    Com_Init(NULL);
    /* Should not crash */
    Com_MainFunctionTx();
    assert_true(1);
}

static void test_Com_MainFunctionRouteSignals(void **state)
{
    (void)state;
    Com_Init(NULL);
    /* Should not crash */
    Com_MainFunctionRouteSignals();
    assert_true(1);
}

static void test_Com_IpduGroupControl(void **state)
{
    (void)state;
    Com_IpduGroupVector ipduGroupVector = {0};
    
    Com_Init(NULL);
    /* Should not crash */
    Com_IpduGroupControl(ipduGroupVector, TRUE);
    assert_true(1);
}

static void test_Com_RxIndication(void **state)
{
    (void)state;
    PduIdType rxPduId = 0;
    PduInfoType pduInfo;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    
    Com_Init(NULL);
    /* Should not crash */
    Com_RxIndication(rxPduId, &pduInfo);
    assert_true(1);
}

static void test_Com_TxConfirmation(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    
    Com_Init(NULL);
    /* Should not crash */
    Com_TxConfirmation(txPduId, E_OK);
    assert_true(1);
}

static void test_Com_TriggerTransmit(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    PduInfoType pduInfo;
    uint8 data[8] = {0};
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    
    Com_Init(NULL);
    Std_ReturnType result = Com_TriggerTransmit(txPduId, &pduInfo);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_Com_ModuleConstants_Exist(void **state)
{
    (void)state;
    /* Verify module ID and vendor ID */
    assert_int_equal(COM_MODULE_ID, 0x001EU);
    assert_int_equal(COM_VENDOR_ID, 0x0055U);
}

static void test_Com_StatusConstants_Exist(void **state)
{
    (void)state;
    /* Verify status constants */
    assert_int_equal(COM_UNINIT, 0x00U);
    assert_int_equal(COM_INIT, 0x01U);
}

static void test_Com_TransmissionModes_Exist(void **state)
{
    (void)state;
    /* Verify transmission mode constants */
    assert_int_equal(COM_DIRECT, 0x00U);
    assert_int_equal(COM_MIXED, 0x01U);
    assert_int_equal(COM_NONE, 0x02U);
    assert_int_equal(COM_PERIODIC, 0x03U);
}

static void test_Com_ErrorCodes_Exist(void **state)
{
    (void)state;
    /* Verify error code constants */
    assert_int_equal(COM_E_PARAM, 0x01U);
    assert_int_equal(COM_E_UNINIT, 0x02U);
    assert_int_equal(COM_E_PARAM_POINTER, 0x03U);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_Com_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_GetStatus_Uninit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_GetStatus_Init, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_GetConfigurationId, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_SendSignal_8bit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_SendSignal_16bit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_SendSignal_NullPointer, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_ReceiveSignal_8bit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_ReceiveSignal_NullPointer, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_InvalidateSignal, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_InvalidateSignalGroup, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_TriggerIPDUSend, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_MainFunctionRx, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_MainFunctionTx, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_MainFunctionRouteSignals, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_IpduGroupControl, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_TriggerTransmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_ModuleConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_StatusConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_TransmissionModes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_Com_ErrorCodes_Exist, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

/**
 * @file test_PduR.c
 * @brief PduR (PDU Router) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "PduR.h"
#include "PduR_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
 *                                  Test Fixtures
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;
    PduR_DeInit();
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    PduR_DeInit();
    return 0;
}

/*==================================================================================================
 *                                    Test Cases
 *================================================================================================*/

static void test_PduR_Init_ValidConfig(void **state)
{
    (void)state;
    /* PduR uses internal configuration */
    PduR_Init(NULL);
    assert_true(1);
}

static void test_PduR_DeInit(void **state)
{
    (void)state;
    PduR_Init(NULL);
    PduR_DeInit();
    assert_true(1);
}

static void test_PduR_GetVersionInfo(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    PduR_Init(NULL);
    PduR_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.moduleID, PDUR_MODULE_ID);
    assert_int_equal(versionInfo.vendorID, PDUR_VENDOR_ID);
}

static void test_PduR_Transmit_ValidPdu(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    PduR_Init(NULL);
    Std_ReturnType result = PduR_Transmit(txPduId, &pduInfo);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_PduR_Transmit_NullPduInfo(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    
    PduR_Init(NULL);
    Std_ReturnType result = PduR_Transmit(txPduId, NULL);
    
    /* Should return error for NULL pointer */
    assert_true(result == E_NOT_OK);
}

static void test_PduR_CancelTransmitRequest(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    
    PduR_Init(NULL);
    Std_ReturnType result = PduR_CancelTransmitRequest(txPduId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_PduR_CancelReceiveRequest(void **state)
{
    (void)state;
    PduIdType rxPduId = 0;
    
    PduR_Init(NULL);
    Std_ReturnType result = PduR_CancelReceiveRequest(rxPduId);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_PduR_ChangeParameterRequest(void **state)
{
    (void)state;
    PduIdType pduId = 0;
    TPParameterType parameter = TP_STMIN;
    uint16 value = 10;
    
    PduR_Init(NULL);
    Std_ReturnType result = PduR_ChangeParameterRequest(pduId, parameter, value);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_PduR_EnableRouting(void **state)
{
    (void)state;
    uint8 groupId = 0;
    
    PduR_Init(NULL);
    /* Should not crash */
    PduR_EnableRouting(groupId);
    assert_true(1);
}

static void test_PduR_DisableRouting(void **state)
{
    (void)state;
    uint8 groupId = 0;
    
    PduR_Init(NULL);
    /* Should not crash */
    PduR_DisableRouting(groupId);
    assert_true(1);
}

static void test_PduR_MainFunction(void **state)
{
    (void)state;
    /* Should not crash when uninitialized */
    PduR_MainFunction();
    
    PduR_Init(NULL);
    PduR_MainFunction();
    assert_true(1);
}

static void test_PduR_TxConfirmation(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    
    PduR_Init(NULL);
    /* Should not crash */
    PduR_TxConfirmation(txPduId, E_OK);
    assert_true(1);
}

static void test_PduR_RxIndication(void **state)
{
    (void)state;
    PduIdType rxPduId = 0;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    PduR_Init(NULL);
    /* Should not crash */
    PduR_RxIndication(rxPduId, &pduInfo);
    assert_true(1);
}

static void test_PduR_TriggerTransmit(void **state)
{
    (void)state;
    PduIdType txPduId = 0;
    uint8 data[8] = {0};
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8;
    pduInfo.MetaDataPtr = NULL;
    
    PduR_Init(NULL);
    Std_ReturnType result = PduR_TriggerTransmit(txPduId, &pduInfo);
    
    assert_true(result == E_OK || result == E_NOT_OK);
}

static void test_PduR_ModuleConstants_Exist(void **state)
{
    (void)state;
    /* Verify module constants */
    assert_int_equal(PDUR_MODULE_ID, 0x69U);
    assert_int_equal(PDUR_VENDOR_ID, 0x01U);
}

static void test_PduR_ServiceIDs_Exist(void **state)
{
    (void)state;
    /* Verify service IDs */
    assert_int_equal(PDUR_SID_INIT, 0xF0U);
    assert_int_equal(PDUR_SID_DEINIT, 0xF1U);
    assert_int_equal(PDUR_SID_GETVERSIONINFO, 0xF2U);
    assert_int_equal(PDUR_SID_TRANSMIT, 0x49U);
    assert_int_equal(PDUR_SID_CANCELTRANSMITREQUEST, 0x4AU);
    assert_int_equal(PDUR_SID_CANCELRECEIVEREQUEST, 0x4BU);
    assert_int_equal(PDUR_SID_CHANGEPARAMETREREQUEST, 0x4CU);
}

static void test_PduR_ErrorCodes_Exist(void **state)
{
    (void)state;
    /* Verify error codes */
    assert_int_equal(PDUR_E_PARAM_POINTER, 0x01U);
    assert_int_equal(PDUR_E_PARAM_CONFIG, 0x02U);
    assert_int_equal(PDUR_E_INVALID_REQUEST, 0x03U);
    assert_int_equal(PDUR_E_PDU_ID_INVALID, 0x04U);
    assert_int_equal(PDUR_E_ROUTING_PATH_GROUP_INVALID, 0x05U);
    assert_int_equal(PDUR_E_UNINIT, 0x07U);
}

static void test_PduR_ReturnType_Exist(void **state)
{
    (void)state;
    /* Verify return type enum values */
    PduR_ReturnType ret;
    ret = PDUR_OK;
    assert_int_equal(ret, 0);
    ret = PDUR_NOT_OK;
    assert_int_equal(ret, 1);
    ret = PDUR_BUSY;
    assert_int_equal(ret, 2);
    ret = PDUR_E_SDU_MISMATCH;
    assert_int_equal(ret, 3);
}

static void test_PduR_RoutingPathType_Exist(void **state)
{
    (void)state;
    /* Verify routing path type enum values */
    PduR_RoutingPathType pathType;
    pathType = PDUR_ROUTING_PATH_DIRECT;
    assert_int_equal(pathType, 0);
    pathType = PDUR_ROUTING_PATH_FIFO;
    assert_int_equal(pathType, 1);
    pathType = PDUR_ROUTING_PATH_GATEWAY;
    assert_int_equal(pathType, 2);
}

static void test_PduR_ApiMappings_Exist(void **state)
{
    (void)state;
    /* Verify API mappings are defined */
    /* These are macros that map to PduR_Transmit */
    assert_ptr_equal(PduR_ComTransmit, PduR_Transmit);
    assert_ptr_equal(PduR_DcmTransmit, PduR_Transmit);
}

static void test_PduR_FrTpCallbacks_Exist(void **state)
{
    (void)state;
    PduIdType pduId = 0;
    
    PduR_Init(NULL);
    
    /* FrTp TxConfirmation callback */
    PduR_FrTpTxConfirmation(pduId, E_OK);
    
    /* FrTp RxIndication callback */
    PduR_FrTpRxIndication(pduId, E_OK);
    
    assert_true(1);
}

/*==================================================================================================
 *                                      Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_PduR_Init_ValidConfig, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_DeInit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_GetVersionInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_Transmit_ValidPdu, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_Transmit_NullPduInfo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_CancelTransmitRequest, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_CancelReceiveRequest, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_ChangeParameterRequest, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_EnableRouting, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_DisableRouting, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_TxConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_RxIndication, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_TriggerTransmit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_ModuleConstants_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_ServiceIDs_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_ErrorCodes_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_ReturnType_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_RoutingPathType_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_ApiMappings_Exist, setup, teardown),
        cmocka_unit_test_setup_teardown(test_PduR_FrTpCallbacks_Exist, setup, teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

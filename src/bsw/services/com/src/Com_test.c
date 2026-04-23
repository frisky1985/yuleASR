/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Com Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-24
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "Com.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static Std_ReturnType mock_pdur_transmit_result = E_OK;
static PduIdType mock_pdur_transmit_id = 0xFFFFU;
static uint8 mock_pdur_transmit_called = 0U;
static uint8 mock_pdur_transmit_data[64U];
static uint8 mock_pdur_transmit_length = 0U;

static uint8 mock_det_report_error_called = 0U;
static uint16 mock_det_module_id = 0xFFFFU;
static uint8 mock_det_instance_id = 0xFFU;
static uint8 mock_det_api_id = 0xFFU;
static uint8 mock_det_error_id = 0xFFU;

/*==================================================================================================
*                                     TEST CONFIGURATION
==================================================================================================*/
static Com_SignalConfigType testSignals[] = {
    /* SignalId=0, BitPos=0, BitSize=16, LE, TRIGGERED, ALWAYS, Mask=0, X=0, IPduRef=0 */
    {0U, 0U, 16U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0U, 0U, 0U},
    /* SignalId=1, BitPos=16, BitSize=8, LE, TRIGGERED, ALWAYS, Mask=0, X=0, IPduRef=0 */
    {1U, 16U, 8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0U, 0U, 0U},
    /* SignalId=2, BitPos=24, BitSize=32, BE, PENDING, ALWAYS, Mask=0, X=0, IPduRef=1 */
    {2U, 24U, 32U, COM_BIG_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 1U}
};

static Com_IPduConfigType testIPdus[] = {
    /* PduId=0, Length=8, NoRepeat, 0 reps, 0 timeBetween, 0 period */
    {0U, 8U, FALSE, 0U, 0U, 0U},
    /* PduId=1, Length=8, NoRepeat, 0 reps, 0 timeBetween, 0 period */
    {1U, 8U, FALSE, 0U, 0U, 0U}
};

static Com_ConfigType testConfig = {
    testSignals,
    3U,
    testIPdus,
    2U
};

/*==================================================================================================
*                                     MOCK FUNCTIONS
==================================================================================================*/

/* PduR mock */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    mock_pdur_transmit_id = TxPduId;
    mock_pdur_transmit_called++;
    if (PduInfoPtr != NULL_PTR)
    {
        mock_pdur_transmit_length = (uint8)PduInfoPtr->SduLength;
        if (PduInfoPtr->SduDataPtr != NULL_PTR)
        {
            (void)memcpy(mock_pdur_transmit_data, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        }
    }
    return mock_pdur_transmit_result;
}

/* Det mock */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    mock_det_module_id = ModuleId;
    mock_det_instance_id = InstanceId;
    mock_det_api_id = ApiId;
    mock_det_error_id = ErrorId;
    mock_det_report_error_called++;
    return E_OK;
}

/*==================================================================================================
*                                  TEST HELPER FUNCTIONS
==================================================================================================*/
static void reset_mocks(void)
{
    mock_pdur_transmit_result = E_OK;
    mock_pdur_transmit_id = 0xFFFFU;
    mock_pdur_transmit_called = 0U;
    mock_pdur_transmit_length = 0U;
    (void)memset(mock_pdur_transmit_data, 0U, sizeof(mock_pdur_transmit_data));

    mock_det_report_error_called = 0U;
    mock_det_module_id = 0xFFFFU;
    mock_det_instance_id = 0xFFU;
    mock_det_api_id = 0xFFU;
    mock_det_error_id = 0xFFU;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/* Test: Com_Init with valid config */
void test_com_init_valid_config(void)
{
    reset_mocks();

    Com_Init(&testConfig);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: Com_Init with NULL config reports DET error */
void test_com_init_null_config(void)
{
    reset_mocks();

    Com_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(COM_SERVICE_ID_INIT, mock_det_api_id);
    ASSERT_EQ(COM_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_DeInit success */
void test_com_deinit_success(void)
{
    reset_mocks();

    Com_Init(&testConfig);
    Com_DeInit();

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: Com_DeInit when not initialized reports DET error */
void test_com_deinit_uninit(void)
{
    reset_mocks();

    /* Ensure uninitialized state by calling DeInit without Init,
       but first init then deinit to safely get to uninit */
    Com_Init(&testConfig);
    Com_DeInit();
    /* Now call DeInit again while uninitialized */
    Com_DeInit();

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(COM_SERVICE_ID_DEINIT, mock_det_api_id);
    ASSERT_EQ(COM_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_SendSignal successful transmission triggers PduR_Transmit */
void test_com_sendsignal_triggers_pdur(void)
{
    uint16 rpmValue = 3500U;
    uint8 result;

    reset_mocks();
    Com_Init(&testConfig);

    result = Com_SendSignal(0U, &rpmValue);

    ASSERT_EQ(COM_SERVICE_OK, result);
    ASSERT_EQ(1U, mock_pdur_transmit_called);
    ASSERT_EQ(0U, mock_pdur_transmit_id); /* IPduRef = 0 */
    TEST_PASS();
}

/* Test: Com_SendSignal with NULL data pointer reports DET error */
void test_com_sendsignal_null_pointer(void)
{
    uint8 result;

    reset_mocks();
    Com_Init(&testConfig);

    result = Com_SendSignal(0U, NULL_PTR);

    ASSERT_EQ(COM_SERVICE_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_SendSignal when not initialized reports DET error */
void test_com_sendsignal_uninit(void)
{
    uint16 rpmValue = 3500U;
    uint8 result;

    reset_mocks();
    /* Ensure uninitialized */
    Com_Init(&testConfig);
    Com_DeInit();

    result = Com_SendSignal(0U, &rpmValue);

    ASSERT_EQ(COM_SERVICE_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_ReceiveSignal successfully unpacks signal data */
void test_com_receivesignal_unpacks_value(void)
{
    uint8 rxBuffer[8] = {0x44U, 0x0DU, 0x55U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U}; /* 0x0D44 = 3396 LE */
    uint16 receivedValue = 0U;
    uint8 result;
    PduInfoType pduInfo;

    reset_mocks();
    Com_Init(&testConfig);

    /* Simulate reception via RxIndication */
    pduInfo.SduDataPtr = rxBuffer;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pduInfo);

    result = Com_ReceiveSignal(0U, &receivedValue);

    ASSERT_EQ(COM_SERVICE_OK, result);
    ASSERT_EQ(3396U, receivedValue);
    TEST_PASS();
}

/* Test: Com_SendSignalGroup triggers PduR_Transmit */
void test_com_sendsignalgroup_triggers_pdur(void)
{
    uint8 result;

    reset_mocks();
    Com_Init(&testConfig);

    result = Com_SendSignalGroup(0U);

    ASSERT_EQ(COM_SERVICE_OK, result);
    ASSERT_EQ(1U, mock_pdur_transmit_called);
    ASSERT_EQ(0U, mock_pdur_transmit_id);
    TEST_PASS();
}

/* Test: Com_InvalidateSignal sets invalid pattern and returns OK */
void test_com_invalidatesignal(void)
{
    uint8 result;

    reset_mocks();
    Com_Init(&testConfig);

    result = Com_InvalidateSignal(0U);

    ASSERT_EQ(COM_SERVICE_OK, result);
    TEST_PASS();
}

/* Test: Com_GetVersionInfo returns correct version */
void test_com_getversioninfo(void)
{
    Std_VersionInfoType versionInfo;

    reset_mocks();

    Com_GetVersionInfo(&versionInfo);

    ASSERT_EQ(COM_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(COM_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(COM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(COM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(COM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_PASS();
}

/* Test: Com_GetVersionInfo with NULL pointer reports DET error */
void test_com_getversioninfo_null(void)
{
    reset_mocks();

    Com_GetVersionInfo(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_TriggerIPDUSend triggers PduR_Transmit */
void test_com_triggeripdusend(void)
{
    Std_ReturnType result;

    reset_mocks();
    Com_Init(&testConfig);

    result = Com_TriggerIPDUSend(0U);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, mock_pdur_transmit_called);
    ASSERT_EQ(0U, mock_pdur_transmit_id);
    TEST_PASS();
}

/* Test: Com_TriggerIPDUSend when not initialized reports DET error */
void test_com_triggeripdusend_uninit(void)
{
    Std_ReturnType result;

    reset_mocks();
    Com_Init(&testConfig);
    Com_DeInit();

    result = Com_TriggerIPDUSend(0U);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_ReceiveSignal with invalid SignalId reports DET error */
void test_com_receivesignal_invalid_id(void)
{
    uint8 value = 0U;
    uint8 result;

    reset_mocks();
    Com_Init(&testConfig);

    result = Com_ReceiveSignal(99U, &value);

    ASSERT_EQ(COM_SERVICE_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(COM_E_INVALID_SIGNAL_ID, mock_det_error_id);
    TEST_PASS();
}

/* Test: Com_SendSignal with pending property does not trigger immediate transmit */
void test_com_sendsignal_pending_notrigger(void)
{
    uint32 bigValue = 0x12345678U;
    uint8 result;

    reset_mocks();
    Com_Init(&testConfig);

    /* Signal 2 is PENDING, 32-bit, in IPdu 1 - should not trigger PduR_Transmit */
    result = Com_SendSignal(2U, &bigValue);

    ASSERT_EQ(COM_SERVICE_OK, result);
    ASSERT_EQ(0U, mock_pdur_transmit_called); /* PENDING does not trigger immediate Tx */
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    printf("\n--- Com Module Tests ---\n");

    RUN_TEST(com_init_valid_config);
    RUN_TEST(com_init_null_config);
    RUN_TEST(com_deinit_success);
    RUN_TEST(com_deinit_uninit);
    RUN_TEST(com_sendsignal_triggers_pdur);
    RUN_TEST(com_sendsignal_null_pointer);
    RUN_TEST(com_sendsignal_uninit);
    RUN_TEST(com_receivesignal_unpacks_value);
    RUN_TEST(com_sendsignalgroup_triggers_pdur);
    RUN_TEST(com_invalidatesignal);
    RUN_TEST(com_getversioninfo);
    RUN_TEST(com_getversioninfo_null);
    RUN_TEST(com_triggeripdusend);
    RUN_TEST(com_triggeripdusend_uninit);
    RUN_TEST(com_receivesignal_invalid_id);
    RUN_TEST(com_sendsignal_pending_notrigger);

TEST_MAIN_END()

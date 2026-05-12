/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DoIp Unit Test
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
#include "DoIp.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static Std_ReturnType mock_soad_transmit_result = E_OK;
static PduIdType mock_soad_transmit_id = 0xFFFFU;
static uint8 mock_soad_transmit_called = 0U;

static PduIdType mock_dcm_rx_indication_id = 0xFFFFU;
static uint8 mock_dcm_rx_indication_called = 0U;
static uint8 mock_dcm_rx_data[256];
static uint16 mock_dcm_rx_length = 0U;

static PduIdType mock_dcm_tx_confirmation_id = 0xFFFFU;
static Std_ReturnType mock_dcm_tx_confirmation_result = E_NOT_OK;
static uint8 mock_dcm_tx_confirmation_called = 0U;

static uint8 mock_det_report_error_called = 0U;
static uint16 mock_det_module_id = 0xFFFFU;
static uint8 mock_det_instance_id = 0xFFU;
static uint8 mock_det_api_id = 0xFFU;
static uint8 mock_det_error_id = 0xFFU;

/*==================================================================================================
*                                     MOCK FUNCTIONS
==================================================================================================*/

/* SoAd mocks */
Std_ReturnType SoAd_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    mock_soad_transmit_id = TxPduId;
    mock_soad_transmit_called++;
    return mock_soad_transmit_result;
}

/* Dcm mocks */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    mock_dcm_rx_indication_id = RxPduId;
    mock_dcm_rx_indication_called++;
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR))
    {
        mock_dcm_rx_length = (PduInfoPtr->SduLength < 256U) ? PduInfoPtr->SduLength : 256U;
        (void)memcpy(mock_dcm_rx_data, PduInfoPtr->SduDataPtr, mock_dcm_rx_length);
    }
}

void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    mock_dcm_tx_confirmation_id = TxPduId;
    mock_dcm_tx_confirmation_result = result;
    mock_dcm_tx_confirmation_called++;
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
    mock_soad_transmit_result = E_OK;
    mock_soad_transmit_id = 0xFFFFU;
    mock_soad_transmit_called = 0U;

    mock_dcm_rx_indication_id = 0xFFFFU;
    mock_dcm_rx_indication_called = 0U;
    mock_dcm_rx_length = 0U;

    mock_dcm_tx_confirmation_id = 0xFFFFU;
    mock_dcm_tx_confirmation_result = E_NOT_OK;
    mock_dcm_tx_confirmation_called = 0U;

    mock_det_report_error_called = 0U;
    mock_det_module_id = 0xFFFFU;
    mock_det_instance_id = 0xFFU;
    mock_det_api_id = 0xFFU;
    mock_det_error_id = 0xFFU;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/* Test: DoIp_Init with valid config */
void test_doip_init_valid_config(void)
{
    reset_mocks();

    DoIp_Init(&DoIp_Config);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: DoIp_Init with NULL config reports DET error */
void test_doip_init_null_config(void)
{
    reset_mocks();

    DoIp_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DOIP_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(DOIP_SID_INIT, mock_det_api_id);
    ASSERT_EQ(DOIP_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: DoIp_ActivateRouting success */
void test_doip_activate_routing_success(void)
{
    Std_ReturnType result;

    reset_mocks();
    DoIp_Init(&DoIp_Config);

    result = DoIp_ActivateRouting(DOIP_LOGICAL_ADDRESS_TESTER, DOIP_LOGICAL_ADDRESS_ECU,
                                  DOIP_ROUTING_ACTIVATION_DEFAULT);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: DoIp_IfTransmit correctly encapsulates DoIP header */
void test_doip_iftransmit_encapsulates_header(void)
{
    uint8 txData[8] = {0x10U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    DoIp_Init(&DoIp_Config);

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = DoIp_IfTransmit(DOIP_DCM_TX_DIAG_REQUEST, &pduInfo);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, mock_soad_transmit_called);
    ASSERT_EQ(DOIP_SOAD_TX_PDU_ID, mock_soad_transmit_id);
    TEST_PASS();
}

/* Test: DoIp_IfRxIndication correctly parses and routes to DCM */
void test_doip_ifrxindication_routes_to_dcm(void)
{
    uint8 rxData[16] = {
        0x02U, 0xFDU,                 /* Protocol version + inverse */
        0x80U, 0x01U,                 /* Payload Type: Diagnostic Message */
        0x00U, 0x00U, 0x00U, 0x04U,  /* Payload Length: 4 */
        0x0EU, 0x00U,                 /* Source Address */
        0x00U, 0x01U,                 /* Target Address */
        0x22U, 0xF1U, 0x90U          /* UDS data: ReadDatabyIdentifier VIN */
    };
    PduInfoType pduInfo;

    reset_mocks();
    DoIp_Init(&DoIp_Config);

    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 15U;
    pduInfo.MetaDataPtr = NULL_PTR;

    DoIp_IfRxIndication(DOIP_SOAD_RX_PDU_ID, &pduInfo);

    ASSERT_EQ(1U, mock_dcm_rx_indication_called);
    ASSERT_EQ(3U, mock_dcm_rx_length);
    ASSERT_EQ(0x22U, mock_dcm_rx_data[0]);
    ASSERT_EQ(0xF1U, mock_dcm_rx_data[1]);
    ASSERT_EQ(0x90U, mock_dcm_rx_data[2]);
    TEST_PASS();
}

/* Test: DoIp_IfTransmit when not initialized reports DET error */
void test_doip_iftransmit_uninit(void)
{
    uint8 txData[8] = {0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    DoIp_Init(&DoIp_Config);
    DoIp_DeInit();

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 1U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = DoIp_IfTransmit(DOIP_DCM_TX_DIAG_REQUEST, &pduInfo);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DOIP_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: DoIp_CloseConnection success */
void test_doip_close_connection_success(void)
{
    Std_ReturnType result;

    reset_mocks();
    DoIp_Init(&DoIp_Config);

    result = DoIp_CloseConnection(DOIP_CONNECTION_0);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: DoIp_SoAdTxConfirmation routes to DCM */
void test_doip_soadtxconfirmation_routes_to_dcm(void)
{
    reset_mocks();
    DoIp_Init(&DoIp_Config);

    DoIp_SoAdTxConfirmation(DOIP_SOAD_TX_PDU_ID, E_OK);

    ASSERT_EQ(1U, mock_dcm_tx_confirmation_called);
    ASSERT_EQ(E_OK, mock_dcm_tx_confirmation_result);
    TEST_PASS();
}

/* Test: DoIp_GetVersionInfo returns correct version */
void test_doip_getversioninfo(void)
{
    Std_VersionInfoType versionInfo;

    reset_mocks();
    DoIp_Init(&DoIp_Config);

    DoIp_GetVersionInfo(&versionInfo);

    ASSERT_EQ(DOIP_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(DOIP_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(DOIP_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(DOIP_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(DOIP_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    RUN_TEST(doip_init_valid_config);
    RUN_TEST(doip_init_null_config);
    RUN_TEST(doip_activate_routing_success);
    RUN_TEST(doip_iftransmit_encapsulates_header);
    RUN_TEST(doip_ifrxindication_routes_to_dcm);
    RUN_TEST(doip_iftransmit_uninit);
    RUN_TEST(doip_close_connection_success);
    RUN_TEST(doip_soadtxconfirmation_routes_to_dcm);
    RUN_TEST(doip_getversioninfo);

TEST_MAIN_END()

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DoCan Unit Test
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
#include "DoCan.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static Std_ReturnType mock_cantp_transmit_result = E_OK;
static PduIdType mock_cantp_transmit_id = 0xFFFFU;
static uint8 mock_cantp_transmit_called = 0U;

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

/* CanTp mocks */
Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr)
{
    (void)CanTpTxInfoPtr;
    mock_cantp_transmit_id = CanTpTxSduId;
    mock_cantp_transmit_called++;
    return mock_cantp_transmit_result;
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
    mock_cantp_transmit_result = E_OK;
    mock_cantp_transmit_id = 0xFFFFU;
    mock_cantp_transmit_called = 0U;

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

/* Test: DoCan_Init with valid config */
void test_docan_init_valid_config(void)
{
    reset_mocks();

    DoCan_Init(&DoCan_Config);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: DoCan_Init with NULL config reports DET error */
void test_docan_init_null_config(void)
{
    reset_mocks();

    DoCan_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DOCAN_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(DOCAN_SID_INIT, mock_det_api_id);
    ASSERT_EQ(DOCAN_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: DoCan_Transmit correctly calls CanTp_Transmit */
void test_docan_transmit_calls_cantp(void)
{
    uint8 txData[8] = {0x10U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    DoCan_Init(&DoCan_Config);

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = DoCan_Transmit(DOCAN_DCM_TX_DIAG_PHYSICAL, &pduInfo);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, mock_cantp_transmit_called);
    ASSERT_EQ(DOCAN_CANTP_TX_DIAG_PHYSICAL, mock_cantp_transmit_id);
    TEST_PASS();
}

/* Test: DoCan_RxIndication correctly routes to DCM */
void test_docan_rxindication_routes_to_dcm(void)
{
    uint8 rxData[8] = {0x50U, 0x01U, 0x00U, 0x32U, 0x01U, 0xF4U, 0x00U, 0x00U};
    PduInfoType pduInfo;

    reset_mocks();
    DoCan_Init(&DoCan_Config);

    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;

    DoCan_RxIndication(DOCAN_CANTP_RX_DIAG_PHYSICAL, &pduInfo);

    ASSERT_EQ(1U, mock_dcm_rx_indication_called);
    ASSERT_EQ(DOCAN_DCM_RX_DIAG_PHYSICAL, mock_dcm_rx_indication_id);
    ASSERT_EQ(8U, mock_dcm_rx_length);
    ASSERT_EQ(0x50U, mock_dcm_rx_data[0]);
    ASSERT_EQ(0x01U, mock_dcm_rx_data[1]);
    TEST_PASS();
}

/* Test: DoCan_TxConfirmation correctly notifies DCM */
void test_docan_txconfirmation_notifies_dcm(void)
{
    reset_mocks();
    DoCan_Init(&DoCan_Config);

    DoCan_TxConfirmation(DOCAN_CANTP_TX_DIAG_PHYSICAL, E_OK);

    ASSERT_EQ(1U, mock_dcm_tx_confirmation_called);
    ASSERT_EQ(DOCAN_DCM_TX_DIAG_PHYSICAL, mock_dcm_tx_confirmation_id);
    ASSERT_EQ(E_OK, mock_dcm_tx_confirmation_result);
    TEST_PASS();
}

/* Test: DoCan_Transmit when not initialized reports DET error */
void test_docan_transmit_uninit(void)
{
    uint8 txData[8] = {0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    DoCan_Init(&DoCan_Config);
    DoCan_DeInit();

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 1U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = DoCan_Transmit(DOCAN_DCM_TX_DIAG_PHYSICAL, &pduInfo);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DOCAN_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: DoCan_GetVersionInfo returns correct version */
void test_docan_getversioninfo(void)
{
    Std_VersionInfoType versionInfo;

    reset_mocks();
    DoCan_Init(&DoCan_Config);

    DoCan_GetVersionInfo(&versionInfo);

    ASSERT_EQ(DOCAN_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(DOCAN_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(DOCAN_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(DOCAN_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(DOCAN_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_PASS();
}

/* Test: DoCan_Transmit with functional addressing */
void test_docan_transmit_functional_addressing(void)
{
    uint8 txData[8] = {0x3EU, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    DoCan_Init(&DoCan_Config);

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 2U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = DoCan_Transmit(DOCAN_DCM_TX_DIAG_FUNCTIONAL, &pduInfo);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, mock_cantp_transmit_called);
    ASSERT_EQ(DOCAN_CANTP_TX_DIAG_FUNCTIONAL, mock_cantp_transmit_id);
    TEST_PASS();
}

/* Test: DoCan_RxIndication when not initialized reports DET error */
void test_docan_rxindication_uninit(void)
{
    uint8 rxData[8] = {0x00U};
    PduInfoType pduInfo;

    reset_mocks();
    DoCan_Init(&DoCan_Config);
    DoCan_DeInit();

    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 1U;
    pduInfo.MetaDataPtr = NULL_PTR;

    DoCan_RxIndication(DOCAN_CANTP_RX_DIAG_PHYSICAL, &pduInfo);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DOCAN_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    RUN_TEST(docan_init_valid_config);
    RUN_TEST(docan_init_null_config);
    RUN_TEST(docan_transmit_calls_cantp);
    RUN_TEST(docan_rxindication_routes_to_dcm);
    RUN_TEST(docan_txconfirmation_notifies_dcm);
    RUN_TEST(docan_transmit_uninit);
    RUN_TEST(docan_getversioninfo);
    RUN_TEST(docan_transmit_functional_addressing);
    RUN_TEST(docan_rxindication_uninit);

TEST_MAIN_END()

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : PduR Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include "test_framework.h"
#include "PduR.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static Std_ReturnType mock_canif_transmit_result = E_OK;
static PduIdType mock_canif_transmit_id = 0xFFFFU;
static uint8 mock_canif_transmit_called = 0U;

static PduIdType mock_com_rx_indication_id = 0xFFFFU;
static uint8 mock_com_rx_indication_called = 0U;

static PduIdType mock_com_tx_confirmation_id = 0xFFFFU;
static Std_ReturnType mock_com_tx_confirmation_result = E_NOT_OK;
static uint8 mock_com_tx_confirmation_called = 0U;

static uint8 mock_det_report_error_called = 0U;
static uint16 mock_det_module_id = 0xFFFFU;
static uint8 mock_det_instance_id = 0xFFU;
static uint8 mock_det_api_id = 0xFFU;
static uint8 mock_det_error_id = 0xFFU;

/*==================================================================================================
*                                     MOCK FUNCTIONS
==================================================================================================*/

/* CanIf mocks */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    mock_canif_transmit_id = TxPduId;
    mock_canif_transmit_called++;
    return mock_canif_transmit_result;
}

Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId)
{
    (void)TxPduId;
    return E_OK;
}

/* Com mocks */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)PduInfoPtr;
    mock_com_rx_indication_id = RxPduId;
    mock_com_rx_indication_called++;
}

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    mock_com_tx_confirmation_id = TxPduId;
    mock_com_tx_confirmation_result = result;
    mock_com_tx_confirmation_called++;
}

Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_OK;
}

/* Dcm mocks */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
}

void Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
}

Std_ReturnType Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_OK;
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
    mock_canif_transmit_result = E_OK;
    mock_canif_transmit_id = 0xFFFFU;
    mock_canif_transmit_called = 0U;

    mock_com_rx_indication_id = 0xFFFFU;
    mock_com_rx_indication_called = 0U;

    mock_com_tx_confirmation_id = 0xFFFFU;
    mock_com_tx_confirmation_result = E_NOT_OK;
    mock_com_tx_confirmation_called = 0U;

    mock_det_report_error_called = 0U;
    mock_det_module_id = 0xFFFFU;
    mock_det_instance_id = 0xFFU;
    mock_det_api_id = 0xFFU;
    mock_det_error_id = 0xFFU;
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/* Test: PduR_Init with valid config */
void test_pdur_init_valid_config(void)
{
    reset_mocks();

    PduR_Init(&PduR_Config);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: PduR_Init with NULL config reports DET error */
void test_pdur_init_null_config(void)
{
    reset_mocks();

    PduR_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(PDUR_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(PDUR_SID_INIT, mock_det_api_id);
    ASSERT_EQ(PDUR_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: PduR_ComTransmit routes to CanIf_Transmit */
void test_pdur_comtransmit_routes_to_canif(void)
{
    uint8 txData[8] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    PduR_Init(&PduR_Config);

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = PduR_ComTransmit(PDUR_COM_TX_ENGINE_STATUS, &pduInfo);

    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(1U, mock_canif_transmit_called);
    ASSERT_EQ(0U, mock_canif_transmit_id); /* CanIf DestPduId for engine status */
    TEST_PASS();
}

/* Test: PduR_ComTransmit returns E_NOT_OK when not initialized */
void test_pdur_comtransmit_uninit(void)
{
    uint8 txData[8] = {0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    /* Note: PduR_DeInit may not be safe if already uninitialized, 
       but we call it to ensure state is UNINIT */
    PduR_Init(&PduR_Config);
    PduR_DeInit();

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 1U;
    pduInfo.MetaDataPtr = NULL_PTR;

    result = PduR_ComTransmit(PDUR_COM_TX_ENGINE_STATUS, &pduInfo);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(PDUR_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: PduR_CanIfRxIndication routes to Com_RxIndication */
void test_pdur_canifrxindication_routes_to_com(void)
{
    uint8 rxData[8] = {0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x11U, 0x22U};
    PduInfoType pduInfo;

    reset_mocks();
    PduR_Init(&PduR_Config);

    pduInfo.SduDataPtr = rxData;
    pduInfo.SduLength = 8U;
    pduInfo.MetaDataPtr = NULL_PTR;

    PduR_CanIfRxIndication(PDUR_COM_RX_ENGINE_CMD, &pduInfo);

    ASSERT_EQ(1U, mock_com_rx_indication_called);
    ASSERT_EQ(PDUR_COM_RX_ENGINE_CMD, mock_com_rx_indication_id);
    TEST_PASS();
}

/* Test: PduR_CanIfRxIndication with NULL pointer reports DET error */
void test_pdur_canifrxindication_null_pointer(void)
{
    reset_mocks();
    PduR_Init(&PduR_Config);

    PduR_CanIfRxIndication(PDUR_COM_RX_ENGINE_CMD, NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(PDUR_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: PduR_CanIfTxConfirmation routes to Com_TxConfirmation */
void test_pdur_caniftxconfirmation_routes_to_com(void)
{
    reset_mocks();
    PduR_Init(&PduR_Config);

    PduR_CanIfTxConfirmation(PDUR_COM_TX_ENGINE_STATUS, E_OK);

    ASSERT_EQ(1U, mock_com_tx_confirmation_called);
    ASSERT_EQ(PDUR_COM_TX_ENGINE_STATUS, mock_com_tx_confirmation_id);
    ASSERT_EQ(E_OK, mock_com_tx_confirmation_result);
    TEST_PASS();
}

/* Test: PduR_CanIfTxConfirmation when not initialized reports DET error */
void test_pdur_caniftxconfirmation_uninit(void)
{
    reset_mocks();
    PduR_Init(&PduR_Config);
    PduR_DeInit();

    PduR_CanIfTxConfirmation(PDUR_COM_TX_ENGINE_STATUS, E_OK);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(PDUR_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: PduR_GetVersionInfo returns correct version */
void test_pdur_getversioninfo(void)
{
    Std_VersionInfoType versionInfo;

    reset_mocks();
    PduR_Init(&PduR_Config);

    PduR_GetVersionInfo(&versionInfo);

    ASSERT_EQ(PDUR_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(PDUR_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(PDUR_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(PDUR_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(PDUR_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_PASS();
}

/* Test: PduR_ComTransmit with unknown PDU ID returns E_NOT_OK */
void test_pdur_comtransmit_unknown_pdu(void)
{
    uint8 txData[8] = {0x00U};
    PduInfoType pduInfo;
    Std_ReturnType result;

    reset_mocks();
    PduR_Init(&PduR_Config);

    pduInfo.SduDataPtr = txData;
    pduInfo.SduLength = 1U;
    pduInfo.MetaDataPtr = NULL_PTR;

    /* Use an invalid PDU ID that is not in the routing table */
    result = PduR_ComTransmit((PduIdType)99U, &pduInfo);

    ASSERT_EQ(E_NOT_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    RUN_TEST(pdur_init_valid_config);
    RUN_TEST(pdur_init_null_config);
    RUN_TEST(pdur_comtransmit_routes_to_canif);
    RUN_TEST(pdur_comtransmit_uninit);
    RUN_TEST(pdur_canifrxindication_routes_to_com);
    RUN_TEST(pdur_canifrxindication_null_pointer);
    RUN_TEST(pdur_caniftxconfirmation_routes_to_com);
    RUN_TEST(pdur_caniftxconfirmation_uninit);
    RUN_TEST(pdur_getversioninfo);
    RUN_TEST(pdur_comtransmit_unknown_pdu);

TEST_MAIN_END()

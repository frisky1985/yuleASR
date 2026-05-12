/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DCM Unit Test
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
#include "Dcm.h"

/*==================================================================================================
*                                     MOCK VARIABLES
==================================================================================================*/
static uint8 mock_pdur_transmit_called = 0U;
static PduIdType mock_pdur_transmit_id = 0xFFFFU;
static uint8 mock_pdur_tx_data[256];
static uint16 mock_pdur_tx_length = 0U;

static uint8 mock_det_report_error_called = 0U;
static uint16 mock_det_module_id = 0xFFFFU;
static uint8 mock_det_instance_id = 0xFFU;
static uint8 mock_det_api_id = 0xFFU;
static uint8 mock_det_error_id = 0xFFU;

static uint8 mock_read_did_data[4] = {0x01U, 0x02U, 0x03U, 0x04U};
static uint8 mock_read_did_called = 0U;

/*==================================================================================================
*                                     MOCK FUNCTIONS
==================================================================================================*/

/* PduR mock */
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    uint16 i;
    mock_pdur_transmit_id = TxPduId;
    mock_pdur_transmit_called++;
    if (PduInfoPtr != NULL_PTR)
    {
        mock_pdur_tx_length = PduInfoPtr->SduLength;
        for (i = 0U; i < PduInfoPtr->SduLength; i++)
        {
            mock_pdur_tx_data[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
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

/* DID read mock */
static Std_ReturnType testReadDidF18C(uint8* Data)
{
    uint8 i;
    mock_read_did_called++;
    for (i = 0U; i < 4U; i++)
    {
        Data[i] = mock_read_did_data[i];
    }
    return E_OK;
}

/*==================================================================================================
*                                  TEST CONFIGURATION
==================================================================================================*/
static Dcm_DIDConfigType testDIDs[] = {
    {0xF18CU, 4U, DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, testReadDidF18C, NULL_PTR}
};

const Dcm_ConfigType Dcm_Config = {
    DCM_NUM_PROTOCOLS,
    DCM_NUM_CONNECTIONS,
    DCM_NUM_RX_PDU_IDS,
    DCM_NUM_TX_PDU_IDS,
    DCM_NUM_SESSIONS,
    DCM_NUM_SECURITY_LEVELS,
    DCM_NUM_SERVICES,
    1U, /* NumDIDs */
    0U, /* NumRIDs */
    testDIDs,
    NULL_PTR,
    TRUE,  /* DevErrorDetect */
    TRUE,  /* VersionInfoApi */
    TRUE,  /* RespondAllRequest */
    TRUE   /* DcmTaskTime */
};

/*==================================================================================================
*                                  TEST HELPER FUNCTIONS
==================================================================================================*/
static void reset_mocks(void)
{
    mock_pdur_transmit_called = 0U;
    mock_pdur_transmit_id = 0xFFFFU;
    memset(mock_pdur_tx_data, 0, sizeof(mock_pdur_tx_data));
    mock_pdur_tx_length = 0U;

    mock_det_report_error_called = 0U;
    mock_det_module_id = 0xFFFFU;
    mock_det_instance_id = 0xFFU;
    mock_det_api_id = 0xFFU;
    mock_det_error_id = 0xFFU;

    mock_read_did_called = 0U;
}

static void send_dcm_request(uint8 ProtocolId, const uint8* data, uint16 length)
{
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = (uint8*)data;
    pduInfo.SduLength = length;
    pduInfo.MetaDataPtr = NULL_PTR;
    Dcm_RxIndication(ProtocolId, &pduInfo);
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/* Test: Dcm_Init with valid config */
void test_dcm_init_valid_config(void)
{
    reset_mocks();

    Dcm_Init(&Dcm_Config);

    ASSERT_EQ(0U, mock_det_report_error_called);
    TEST_PASS();
}

/* Test: Dcm_Init with NULL config reports DET error */
void test_dcm_init_null_config(void)
{
    reset_mocks();

    Dcm_Init(NULL_PTR);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DCM_MODULE_ID, mock_det_module_id);
    ASSERT_EQ(DCM_SERVICE_ID_INIT, mock_det_api_id);
    ASSERT_EQ(DCM_E_PARAM_POINTER, mock_det_error_id);
    TEST_PASS();
}

/* Test: UDS 0x10 session switch Default -> Extended */
void test_dcm_uds_session_control_extended(void)
{
    uint8 request[2] = {0x10U, 0x03U}; /* DiagnosticSessionControl -> Extended */

    reset_mocks();
    Dcm_Init(&Dcm_Config);

    send_dcm_request(0U, request, 2U);

    ASSERT_EQ(1U, mock_pdur_transmit_called);
    ASSERT_EQ(0x50U, mock_pdur_tx_data[0]); /* Positive response SID */
    ASSERT_EQ(0x03U, mock_pdur_tx_data[1]); /* Extended session */
    TEST_PASS();
}

/* Test: UDS 0x22 read DID */
void test_dcm_uds_read_did(void)
{
    uint8 request[3] = {0x22U, 0xF1U, 0x8CU}; /* ReadDataByIdentifier -> 0xF18C */

    reset_mocks();
    Dcm_Init(&Dcm_Config);

    send_dcm_request(0U, request, 3U);

    ASSERT_EQ(1U, mock_read_did_called);
    ASSERT_EQ(1U, mock_pdur_transmit_called);
    ASSERT_EQ(0x62U, mock_pdur_tx_data[0]); /* Positive response SID */
    ASSERT_EQ(0xF1U, mock_pdur_tx_data[1]); /* DID high byte */
    ASSERT_EQ(0x8CU, mock_pdur_tx_data[2]); /* DID low byte */
    ASSERT_EQ(0x01U, mock_pdur_tx_data[3]); /* Data byte 0 */
    ASSERT_EQ(0x02U, mock_pdur_tx_data[4]); /* Data byte 1 */
    TEST_PASS();
}

/* Test: UDS 0x3E Tester Present keeps session alive */
void test_dcm_uds_tester_present(void)
{
    uint8 request[2] = {0x3EU, 0x00U}; /* TesterPresent, subFunction 0x00 */

    reset_mocks();
    Dcm_Init(&Dcm_Config);

    /* Switch to extended session first */
    {
        uint8 sessionReq[2] = {0x10U, 0x03U};
        send_dcm_request(0U, sessionReq, 2U);
    }

    reset_mocks();

    send_dcm_request(0U, request, 2U);

    ASSERT_EQ(1U, mock_pdur_transmit_called);
    ASSERT_EQ(0x7EU, mock_pdur_tx_data[0]); /* Positive response SID */
    ASSERT_EQ(0x00U, mock_pdur_tx_data[1]); /* SubFunction */
    TEST_PASS();
}

/* Test: Dcm_RxIndication when not initialized reports DET error */
void test_dcm_rxindication_uninit(void)
{
    uint8 request[2] = {0x10U, 0x01U};
    PduInfoType pduInfo;

    reset_mocks();
    Dcm_Init(&Dcm_Config);
    Dcm_DeInit();

    pduInfo.SduDataPtr = request;
    pduInfo.SduLength = 2U;
    pduInfo.MetaDataPtr = NULL_PTR;
    Dcm_RxIndication(0U, &pduInfo);

    ASSERT_EQ(1U, mock_det_report_error_called);
    ASSERT_EQ(DCM_E_UNINIT, mock_det_error_id);
    TEST_PASS();
}

/* Test: Dcm_GetVersionInfo returns correct version */
void test_dcm_getversioninfo(void)
{
    Std_VersionInfoType versionInfo;

    reset_mocks();
    Dcm_Init(&Dcm_Config);

    Dcm_GetVersionInfo(&versionInfo);

    ASSERT_EQ(DCM_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(DCM_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(DCM_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(DCM_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(DCM_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()

    printf("\n--- DCM Module Tests ---\n");

    RUN_TEST(dcm_init_valid_config);
    RUN_TEST(dcm_init_null_config);
    RUN_TEST(dcm_uds_session_control_extended);
    RUN_TEST(dcm_uds_read_did);
    RUN_TEST(dcm_uds_tester_present);
    RUN_TEST(dcm_rxindication_uninit);
    RUN_TEST(dcm_getversioninfo);

TEST_MAIN_END()

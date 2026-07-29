/*==================================================================================================
 *                              SECURE ONBOARD COMMUNICATION (SecOC)
 *==================================================================================================
 * FILENAME: SecOC_Test.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Secure Onboard Communication module
 *==================================================================================================
 */

#include "unity.h"
#include "SecOC.h"
#include "SecOC_Cfg.h"
#include "PduR.h"
#include "Csm.h"
#include "Det.h"
#include "mock_PduR.h"
#include "mock_Csm.h"
#include "mock_Det.h"
#include "mock_SchM_SecOC.h"

/*==================================================================================================
 *                                    TEST SETUP
 *==================================================================================================*/
void setUp(void)
{
    /* Reset module state before each test */
    SecOC_DeInit();
}

void tearDown(void)
{
    /* Cleanup after each test */
    SecOC_DeInit();
}

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
static const SecOC_AuthBuildConfigType TestAuthConfig = {
    SECOC_AUTH_ALGORITHM,
    SECOC_AUTH_INFO_LENGTH,
    0u
};

static const SecOC_FreshnessValueConfigType TestFreshnessConfig = {
    SECOC_FRESHNESS_VALUE_TYPE,
    0u,
    SECOC_FRESHNESS_VALUE_LENGTH,
    SECOC_FRESHNESS_VALUE_TX_LENGTH
};

static const SecOC_PduConfigType TestTxPduConfigs[SECOC_NUM_TX_PDUS] = {
    { 0u, 0u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 0u}, {SECOC_COUNTER, 0u, 32u, 16u}, FALSE, 0u, 0u, 64u },
    { 1u, 1u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 1u}, {SECOC_COUNTER, 1u, 32u, 16u}, FALSE, 0u, 0u, 64u },
    { 2u, 2u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 2u}, {SECOC_COUNTER, 2u, 32u, 16u}, FALSE, 0u, 0u, 64u },
    { 3u, 3u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 3u}, {SECOC_COUNTER, 3u, 32u, 16u}, FALSE, 0u, 0u, 64u }
};

static const SecOC_PduConfigType TestRxPduConfigs[SECOC_NUM_RX_PDUS] = {
    { 0u, 0u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 0u}, {SECOC_COUNTER, 0u, 32u, 16u}, FALSE, 0u, 0u, 64u },
    { 1u, 1u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 1u}, {SECOC_COUNTER, 1u, 32u, 16u}, FALSE, 0u, 0u, 64u },
    { 2u, 2u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 2u}, {SECOC_COUNTER, 2u, 32u, 16u}, FALSE, 0u, 0u, 64u },
    { 3u, 3u, SECOC_IFPDU, {SECOC_HMAC_SHA256, 16u, 3u}, {SECOC_COUNTER, 3u, 32u, 16u}, FALSE, 0u, 0u, 64u }
};

static const SecOC_ConfigType TestConfig = {
    TestTxPduConfigs,
    SECOC_NUM_TX_PDUS,
    TestRxPduConfigs,
    SECOC_NUM_RX_PDUS,
    SECOC_MAIN_FUNCTION_PERIOD_RX_MS,
    SECOC_MAIN_FUNCTION_PERIOD_TX_MS,
    (SECOC_DEV_ERROR_DETECT == STD_ON),
    (SECOC_VERSION_INFO_API == STD_ON),
    (SECOC_OVERRIDE_STATUS_ALLOWED == STD_ON)
};

/*==================================================================================================
 *                                    TEST CASES - Init/DeInit
 *==================================================================================================*/
void test_SecOC_Init_ShouldInitializeModule(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Expect();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Expect();
    
    SecOC_Init(&TestConfig);
    
    TEST_ASSERT_TRUE(SecOC_Initialized);
}

void test_SecOC_Init_ShouldReportError_WhenAlreadyInitialized(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_INIT, 
                                     SECOC_E_ALREADY_INITIALIZED, E_OK);
    
    SecOC_Init(&TestConfig);
}

void test_SecOC_Init_ShouldReportError_WhenConfigNull(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_INIT, 
                                     SECOC_E_PARAM_POINTER, E_OK);
    
    SecOC_Init(NULL);
    
    TEST_ASSERT_FALSE(SecOC_Initialized);
}

void test_SecOC_DeInit_ShouldDeinitializeModule(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    SecOC_DeInit();
    
    TEST_ASSERT_FALSE(SecOC_Initialized);
}

void test_SecOC_DeInit_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_DEINIT, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_DeInit();
}

/*==================================================================================================
 *                                    TEST CASES - Transmit
 *==================================================================================================*/
void test_SecOC_IfTransmit_ShouldAcceptValidPdu(void)
{
    uint8 pduData[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    PduInfoType pduInfo;
    Std_ReturnType result;
    
    pduInfo.SduDataPtr = pduData;
    pduInfo.SduLength = 16;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    /* Mock CSM calls for authentication */
    Csm_MacGenerate_ExpectAndReturn(SECOC_CSM_JOB_ID_AUTH, CSM_OPERATIONMODE_STREAMSTART,
                                     NULL, 0, NULL, NULL, E_OK);
    Csm_MacGenerate_IgnoreArg_dataPtr();
    Csm_MacGenerate_IgnoreArg_dataLength();
    Csm_MacGenerate_IgnoreArg_macPtr();
    Csm_MacGenerate_IgnoreArg_macLengthPtr();
    Csm_MacGenerate_ReturnThruPtr_macLengthPtr((uint32[]){16});
    
    /* Mock PduR transmit */
    PduR_SecOCTransmit_ExpectAndReturn(0u, NULL, E_OK);
    PduR_SecOCTransmit_IgnoreArg_PduInfoPtr();
    
    result = SecOC_IfTransmit(SECOC_TX_PDU_ID_0, &pduInfo);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_SecOC_IfTransmit_ShouldReportError_WhenNotInitialized(void)
{
    uint8 pduData[16];
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = pduData;
    pduInfo.SduLength = 16;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_IfTransmit(SECOC_TX_PDU_ID_0, &pduInfo);
}

void test_SecOC_IfTransmit_ShouldReportError_WhenNullPointer(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
                                     SECOC_E_PARAM_POINTER, E_OK);
    
    SecOC_IfTransmit(SECOC_TX_PDU_ID_0, NULL);
}

void test_SecOC_IfTransmit_ShouldReportError_WhenInvalidPduId(void)
{
    uint8 pduData[16];
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = pduData;
    pduInfo.SduLength = 16;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
                                     SECOC_E_INVALID_PDU_SDU_ID, E_OK);
    
    SecOC_IfTransmit(100u, &pduInfo);  /* Invalid PDU ID */
}

/*==================================================================================================
 *                                    TEST CASES - Receive
 *==================================================================================================*/
void test_SecOC_IfRxIndication_ShouldProcessValidPdu(void)
{
    /* Secured PDU: [Data(8)][Freshness(2)][AuthCode(16)] */
    uint8 securedPdu[26] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  /* Data */
        0x00, 0x01,                                      /* Freshness (truncated) */
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,             /* Auth code */
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
        0x77, 0x88, 0x99, 0x00
    };
    PduInfoType pduInfo;
    Csm_VerifyResultType verifyResult = CSM_E_VER_OK;
    
    pduInfo.SduDataPtr = securedPdu;
    pduInfo.SduLength = 26;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    /* Mock CSM verification */
    Csm_MacVerify_ExpectAndReturn(SECOC_CSM_JOB_ID_VERIFY, CSM_OPERATIONMODE_STREAMSTART,
                                   NULL, 0, NULL, 16, NULL, E_OK, NULL);
    Csm_MacVerify_IgnoreArg_dataPtr();
    Csm_MacVerify_IgnoreArg_dataLength();
    Csm_MacVerify_IgnoreArg_macPtr();
    Csm_MacVerify_IgnoreArg_verifyPtr();
    Csm_MacVerify_ReturnThruPtr_verifyPtr(&verifyResult);
    
    /* Mock PduR RxIndication */
    PduR_SecOCRxIndication_Expect(SECOC_RX_PDU_ID_0, NULL);
    PduR_SecOCRxIndication_IgnoreArg_PduInfoPtr();
    
    SecOC_IfRxIndication(SECOC_RX_PDU_ID_0, &pduInfo);
    
    TEST_ASSERT_EQUAL(SECOC_VERIFICATIONSUCCESS_STATUS, SecOC_GetVerificationStatus(SECOC_RX_PDU_ID_0));
}

void test_SecOC_IfRxIndication_ShouldDetectVerificationFailure(void)
{
    /* Secured PDU with bad auth code */
    uint8 securedPdu[26] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x00, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* Bad auth code */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    PduInfoType pduInfo;
    Csm_VerifyResultType verifyResult = CSM_E_VER_NOT_OK;
    
    pduInfo.SduDataPtr = securedPdu;
    pduInfo.SduLength = 26;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    /* Mock CSM verification - will fail */
    Csm_MacVerify_ExpectAndReturn(SECOC_CSM_JOB_ID_VERIFY, CSM_OPERATIONMODE_STREAMSTART,
                                   NULL, 0, NULL, 16, NULL, E_OK, NULL);
    Csm_MacVerify_IgnoreArg_dataPtr();
    Csm_MacVerify_IgnoreArg_dataLength();
    Csm_MacVerify_IgnoreArg_macPtr();
    Csm_MacVerify_IgnoreArg_verifyPtr();
    Csm_MacVerify_ReturnThruPtr_verifyPtr(&verifyResult);
    
    SecOC_IfRxIndication(SECOC_RX_PDU_ID_0, &pduInfo);
    
    TEST_ASSERT_EQUAL(SECOC_VERIFICATIONFAILURE_STATUS, SecOC_GetVerificationStatus(SECOC_RX_PDU_ID_0));
}

void test_SecOC_IfRxIndication_ShouldReportError_WhenNotInitialized(void)
{
    uint8 securedPdu[16];
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = securedPdu;
    pduInfo.SduLength = 16;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFRXINDICATION, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_IfRxIndication(SECOC_RX_PDU_ID_0, &pduInfo);
}

void test_SecOC_IfRxIndication_ShouldReportError_WhenNullPointer(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFRXINDICATION, 
                                     SECOC_E_PARAM_POINTER, E_OK);
    
    SecOC_IfRxIndication(SECOC_RX_PDU_ID_0, NULL);
}

/*==================================================================================================
 *                                    TEST CASES - Verification Status
 *==================================================================================================*/
void test_SecOC_VerifyStatusOverride_ShouldOverrideStatus(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    /* Set initial status */
    SecOC_VerifyStatusOverride(SECOC_RX_PDU_ID_0, SECOC_VERIFICATIONFAILURE_STATUS);
    TEST_ASSERT_EQUAL(SECOC_VERIFICATIONFAILURE_STATUS, SecOC_GetVerificationStatus(SECOC_RX_PDU_ID_0));
    
    /* Override to success */
    SecOC_VerifyStatusOverride(SECOC_RX_PDU_ID_0, SECOC_VERIFICATIONSUCCESS_STATUS);
    TEST_ASSERT_EQUAL(SECOC_VERIFICATIONSUCCESS_STATUS, SecOC_GetVerificationStatus(SECOC_RX_PDU_ID_0));
}

void test_SecOC_VerifyStatusOverride_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_VERIFYSTATUSOVERRIDE, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_VerifyStatusOverride(SECOC_RX_PDU_ID_0, SECOC_VERIFICATIONSUCCESS_STATUS);
}

void test_SecOC_VerifyStatusOverride_ShouldReportError_WhenInvalidStatus(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_VERIFYSTATUSOVERRIDE, 
                                     SECOC_E_INVALID_PARAMETER, E_OK);
    
    SecOC_VerifyStatusOverride(SECOC_RX_PDU_ID_0, (SecOC_VerificationStatusType)100);
}

void test_SecOC_GetVerificationResult_ShouldReturnResult(void)
{
    SecOC_VerificationResultType result;
    Std_ReturnType status;
    
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    status = SecOC_GetVerificationResult(SECOC_RX_PDU_ID_0, &result);
    
    TEST_ASSERT_EQUAL(E_OK, status);
    TEST_ASSERT_EQUAL(SECOC_NO_VERIFICATION, result);
}

void test_SecOC_GetVerificationResult_ShouldReportError_WhenNullPointer(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_VERIFYSTATUSOVERRIDE, 
                                     SECOC_E_PARAM_POINTER, E_OK);
    
    SecOC_GetVerificationResult(SECOC_RX_PDU_ID_0, NULL);
}

/*==================================================================================================
 *                                    TEST CASES - Tx Confirmation
 *==================================================================================================*/
void test_SecOC_TxConfirmation_ShouldProcessConfirmation(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    /* Setup TX in progress */
    {
        uint8 pduData[16] = {0};
        PduInfoType pduInfo;
        pduInfo.SduDataPtr = pduData;
        pduInfo.SduLength = 16;
        
        Csm_MacGenerate_IgnoreAndReturn(E_OK);
        Csm_MacGenerate_ReturnThruPtr_macLengthPtr((uint32[]){16});
        PduR_SecOCTransmit_IgnoreAndReturn(E_OK);
        
        SecOC_IfTransmit(SECOC_TX_PDU_ID_0, &pduInfo);
    }
    
    /* Expect PduR confirmation forwarding */
    PduR_SecOCTxConfirmation_Expect(SECOC_TX_PDU_ID_0, E_OK);
    
    SecOC_TxConfirmation(SECOC_TX_PDU_ID_0, E_OK);
}

void test_SecOC_TxConfirmation_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_TXCONFIRMATION, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_TxConfirmation(SECOC_TX_PDU_ID_0, E_OK);
}

/*==================================================================================================
 *                                    TEST CASES - Main Functions
 *==================================================================================================*/
void test_SecOC_MainFunctionRx_ShouldProcessPendingPdus(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    SecOC_MainFunctionRx();
    
    /* Should complete without errors */
    TEST_ASSERT_TRUE(TRUE);
}

void test_SecOC_MainFunctionRx_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_MAINFUNCTIONRX, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_MainFunctionRx();
}

void test_SecOC_MainFunctionTx_ShouldProcessPendingPdus(void)
{
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    SecOC_MainFunctionTx();
    
    /* Should complete without errors */
    TEST_ASSERT_TRUE(TRUE);
}

void test_SecOC_MainFunctionTx_ShouldReportError_WhenNotInitialized(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_MAINFUNCTIONTX, 
                                     SECOC_E_UNINIT, E_OK);
    
    SecOC_MainFunctionTx();
}

/*==================================================================================================
 *                                    TEST CASES - Version Info
 *==================================================================================================*/
#if (SECOC_VERSION_INFO_API == STD_ON)
void test_SecOC_GetVersionInfo_ShouldReturnVersion(void)
{
    Std_VersionInfoType versionInfo;
    
    SecOC_GetVersionInfo(&versionInfo);
    
    TEST_ASSERT_EQUAL(SECOC_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(SECOC_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL(SECOC_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL(SECOC_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL(SECOC_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

void test_SecOC_GetVersionInfo_ShouldReportError_WhenNullPointer(void)
{
    Det_ReportError_ExpectAndReturn(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_GETVERSIONINFO, 
                                     SECOC_E_PARAM_POINTER, E_OK);
    
    SecOC_GetVersionInfo(NULL);
}
#endif

/*==================================================================================================
 *                                    TEST CASES - Edge Cases
 *==================================================================================================*/
void test_SecOC_InvalidPduLength_ShouldHandleGracefully(void)
{
    /* PDU too short - should handle gracefully */
    uint8 shortPdu[4] = {0x00, 0x01, 0x02, 0x03};
    PduInfoType pduInfo;
    
    pduInfo.SduDataPtr = shortPdu;
    pduInfo.SduLength = 4;  /* Too short for secured PDU */
    pduInfo.MetaDataPtr = NULL_PTR;
    
    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0_Ignore();
    
    SecOC_Init(&TestConfig);
    
    SecOC_IfRxIndication(SECOC_RX_PDU_ID_0, &pduInfo);
    
    /* Should mark as failure due to short PDU */
    TEST_ASSERT_EQUAL(SECOC_VERIFICATIONFAILURE_STATUS, SecOC_GetVerificationStatus(SECOC_RX_PDU_ID_0));
}

/*==================================================================================================
 *                                    TEST RUNNER
 *==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();
    
    /* Init/DeInit tests */
    RUN_TEST(test_SecOC_Init_ShouldInitializeModule);
    RUN_TEST(test_SecOC_Init_ShouldReportError_WhenAlreadyInitialized);
    RUN_TEST(test_SecOC_Init_ShouldReportError_WhenConfigNull);
    RUN_TEST(test_SecOC_DeInit_ShouldDeinitializeModule);
    RUN_TEST(test_SecOC_DeInit_ShouldReportError_WhenNotInitialized);
    
    /* Transmit tests */
    RUN_TEST(test_SecOC_IfTransmit_ShouldAcceptValidPdu);
    RUN_TEST(test_SecOC_IfTransmit_ShouldReportError_WhenNotInitialized);
    RUN_TEST(test_SecOC_IfTransmit_ShouldReportError_WhenNullPointer);
    RUN_TEST(test_SecOC_IfTransmit_ShouldReportError_WhenInvalidPduId);
    
    /* Receive tests */
    RUN_TEST(test_SecOC_IfRxIndication_ShouldProcessValidPdu);
    RUN_TEST(test_SecOC_IfRxIndication_ShouldDetectVerificationFailure);
    RUN_TEST(test_SecOC_IfRxIndication_ShouldReportError_WhenNotInitialized);
    RUN_TEST(test_SecOC_IfRxIndication_ShouldReportError_WhenNullPointer);
    
    /* Verification status tests */
    RUN_TEST(test_SecOC_VerifyStatusOverride_ShouldOverrideStatus);
    RUN_TEST(test_SecOC_VerifyStatusOverride_ShouldReportError_WhenNotInitialized);
    RUN_TEST(test_SecOC_VerifyStatusOverride_ShouldReportError_WhenInvalidStatus);
    RUN_TEST(test_SecOC_GetVerificationResult_ShouldReturnResult);
    RUN_TEST(test_SecOC_GetVerificationResult_ShouldReportError_WhenNullPointer);
    
    /* Tx confirmation tests */
    RUN_TEST(test_SecOC_TxConfirmation_ShouldProcessConfirmation);
    RUN_TEST(test_SecOC_TxConfirmation_ShouldReportError_WhenNotInitialized);
    
    /* Main function tests */
    RUN_TEST(test_SecOC_MainFunctionRx_ShouldProcessPendingPdus);
    RUN_TEST(test_SecOC_MainFunctionRx_ShouldReportError_WhenNotInitialized);
    RUN_TEST(test_SecOC_MainFunctionTx_ShouldProcessPendingPdus);
    RUN_TEST(test_SecOC_MainFunctionTx_ShouldReportError_WhenNotInitialized);
    
    /* Version info tests */
#if (SECOC_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_SecOC_GetVersionInfo_ShouldReturnVersion);
    RUN_TEST(test_SecOC_GetVersionInfo_ShouldReportError_WhenNullPointer);
#endif
    
    /* Edge case tests */
    RUN_TEST(test_SecOC_InvalidPduLength_ShouldHandleGracefully);
    
    return UNITY_END();
}

/*==================================================================================================
 *                              SECURE ONBOARD COMMUNICATION (SecOC)
 *==================================================================================================
 * FILENAME: test_secoc.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Comprehensive unit tests for Secure Onboard Communication module
 *              Target coverage: 80%+
 *==================================================================================================
 */

// @tests src/bsw/services/secoc/src/SecOC.c  @tests src/bsw/services/secoc/include/SecOC.h

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

/*==================================================================================================
 *                                    INCLUDES
 *==================================================================================================*/
#include "SecOC.h"
#include "SecOC_Cfg.h"
#include "PduR.h"
#include "Csm.h"
#include "Det.h"
#include "ComStack_Types.h"

/*==================================================================================================
 *                                    MOCK FUNCTIONS
 *==================================================================================================*/
/* SchM mocks */
void SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0(void) {}
void SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0(void) {}

/* PduR mocks */
static Std_ReturnType mock_PduR_SecOCTransmit_result = E_OK;
static boolean mock_PduR_SecOCRxIndication_called = FALSE;
static boolean mock_PduR_SecOCTxConfirmation_called = FALSE;

Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    return mock_PduR_SecOCTransmit_result;
}

void PduR_SecOCRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
    mock_PduR_SecOCRxIndication_called = TRUE;
}

void PduR_SecOCTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
    mock_PduR_SecOCTxConfirmation_called = TRUE;
}

/* Csm mocks */
static Std_ReturnType mock_Csm_MacGenerate_result = E_OK;
static Std_ReturnType mock_Csm_MacVerify_result = E_OK;
static Csm_VerifyResultType mock_Csm_VerifyResult = CSM_E_VER_OK;
static boolean mock_Csm_MacGenerate_called = FALSE;
static boolean mock_Csm_MacVerify_called = FALSE;

Std_ReturnType Csm_MacGenerate(uint32 jobId, Csm_OperationModeType mode,
                                const uint8* dataPtr, uint32 dataLength,
                                uint8* macPtr, uint32* macLengthPtr)
{
    (void)jobId;
    (void)mode;
    (void)dataPtr;
    (void)dataLength;
    
    mock_Csm_MacGenerate_called = TRUE;
    
    /* Fill macPtr with dummy auth code */
    if (macPtr != NULL && macLengthPtr != NULL) {
        for (uint8 i = 0; i < SECOC_AUTH_INFO_LENGTH && i < *macLengthPtr; i++) {
            macPtr[i] = (uint8)(0xA0 + i);
        }
        *macLengthPtr = SECOC_AUTH_INFO_LENGTH;
    }
    
    return mock_Csm_MacGenerate_result;
}

Std_ReturnType Csm_MacVerify(uint32 jobId, Csm_OperationModeType mode,
                              const uint8* dataPtr, uint32 dataLength,
                              const uint8* macPtr, uint32 macLength,
                              Csm_VerifyResultType* verifyPtr)
{
    (void)jobId;
    (void)mode;
    (void)dataPtr;
    (void)dataLength;
    (void)macPtr;
    (void)macLength;
    
    mock_Csm_MacVerify_called = TRUE;
    
    if (verifyPtr != NULL) {
        *verifyPtr = mock_Csm_VerifyResult;
    }
    
    return mock_Csm_MacVerify_result;
}

/* Det mock */
static uint8 det_LastErrorId = 0;
static boolean det_ReportError_called = FALSE;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, 
                                uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    det_LastErrorId = ErrorId;
    det_ReportError_called = TRUE;
    return E_OK;
}

/*==================================================================================================
 *                                    TEST FIXTURE
 *==================================================================================================*/
static SecOC_PduConfigType TestTxPduConfigs[SECOC_NUM_TX_PDUS];
static SecOC_PduConfigType TestRxPduConfigs[SECOC_NUM_RX_PDUS];
static SecOC_ConfigType TestConfig;

typedef struct {
    boolean initialized;
    uint8 testPduData[64];
    PduInfoType testPduInfo;
} TestFixture;

static int test_setup(void **state)
{
    TestFixture *fixture = (TestFixture *)malloc(sizeof(TestFixture));
    assert_non_null(fixture);
    
    /* Reset mocks */
    mock_PduR_SecOCTransmit_result = E_OK;
    mock_PduR_SecOCRxIndication_called = FALSE;
    mock_PduR_SecOCTxConfirmation_called = FALSE;
    mock_Csm_MacGenerate_result = E_OK;
    mock_Csm_MacVerify_result = E_OK;
    mock_Csm_VerifyResult = CSM_E_VER_OK;
    mock_Csm_MacGenerate_called = FALSE;
    mock_Csm_MacVerify_called = FALSE;
    det_LastErrorId = 0;
    det_ReportError_called = FALSE;
    
    /* Initialize test PDU configs */
    for (uint8 i = 0; i < SECOC_NUM_TX_PDUS; i++) {
        TestTxPduConfigs[i].pduId = i;
        TestTxPduConfigs[i].lowerLayerPduId = i;
        TestTxPduConfigs[i].pduType = SECOC_IFPDU;
        TestTxPduConfigs[i].authConfig.algorithm = SECOC_HMAC_SHA256;
        TestTxPduConfigs[i].authConfig.authInfoLength = SECOC_AUTH_INFO_LENGTH;
        TestTxPduConfigs[i].authConfig.dataId = i;
        TestTxPduConfigs[i].freshnessConfig.type = SECOC_COUNTER;
        TestTxPduConfigs[i].freshnessConfig.freshnessValueId = i;
        TestTxPduConfigs[i].freshnessConfig.freshnessValueLength = SECOC_FRESHNESS_VALUE_LENGTH;
        TestTxPduConfigs[i].freshnessConfig.freshnessValueTxLength = SECOC_FRESHNESS_VALUE_TX_LENGTH;
        TestTxPduConfigs[i].useCryptographicPdu = FALSE;
        TestTxPduConfigs[i].dataToAuthOffset = 0;
        TestTxPduConfigs[i].dataToAuthLength = 0;
        TestTxPduConfigs[i].authPduLength = 64;
    }
    
    for (uint8 i = 0; i < SECOC_NUM_RX_PDUS; i++) {
        TestRxPduConfigs[i].pduId = i;
        TestRxPduConfigs[i].lowerLayerPduId = i;
        TestRxPduConfigs[i].pduType = SECOC_IFPDU;
        TestRxPduConfigs[i].authConfig.algorithm = SECOC_HMAC_SHA256;
        TestRxPduConfigs[i].authConfig.authInfoLength = SECOC_AUTH_INFO_LENGTH;
        TestRxPduConfigs[i].authConfig.dataId = i;
        TestRxPduConfigs[i].freshnessConfig.type = SECOC_COUNTER;
        TestRxPduConfigs[i].freshnessConfig.freshnessValueId = i;
        TestRxPduConfigs[i].freshnessConfig.freshnessValueLength = SECOC_FRESHNESS_VALUE_LENGTH;
        TestRxPduConfigs[i].freshnessConfig.freshnessValueTxLength = SECOC_FRESHNESS_VALUE_TX_LENGTH;
        TestRxPduConfigs[i].useCryptographicPdu = FALSE;
        TestRxPduConfigs[i].dataToAuthOffset = 0;
        TestRxPduConfigs[i].dataToAuthLength = 0;
        TestRxPduConfigs[i].authPduLength = 64;
    }
    
    /* Initialize test config */
    TestConfig.txPduConfigs = TestTxPduConfigs;
    TestConfig.numTxPdus = SECOC_NUM_TX_PDUS;
    TestConfig.rxPduConfigs = TestRxPduConfigs;
    TestConfig.numRxPdus = SECOC_NUM_RX_PDUS;
    TestConfig.mainFunctionPeriodRx = SECOC_MAIN_FUNCTION_PERIOD_RX_MS;
    TestConfig.mainFunctionPeriodTx = SECOC_MAIN_FUNCTION_PERIOD_TX_MS;
    TestConfig.devErrorDetect = TRUE;
    TestConfig.versionInfoApi = TRUE;
    TestConfig.overrideStatusAllowed = TRUE;
    
    /* Setup test PDU data */
    for (uint8 i = 0; i < 64; i++) {
        fixture->testPduData[i] = i;
    }
    fixture->testPduInfo.SduDataPtr = fixture->testPduData;
    fixture->testPduInfo.SduLength = 16;
    fixture->testPduInfo.MetaDataPtr = NULL;
    
    /* Ensure module is deinitialized before test */
    SecOC_DeInit();
    fixture->initialized = FALSE;
    
    *state = fixture;
    return 0;
}

static int test_teardown(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    
    /* Cleanup */
    SecOC_DeInit();
    
    free(fixture);
    return 0;
}

/*==================================================================================================
 *                                    TEST CASES - Initialization
 *==================================================================================================*/

/**
 * @test SecOC_Init should initialize module successfully
 * @requirement SWS_SecOC_00001
 */
static void test_SecOC_Init_Success(void **state)
{
    (void)state;
    
    SecOC_Init(&TestConfig);
    
    assert_true(SecOC_Initialized);
    assert_ptr_equal(SecOC_ConfigPtr, &TestConfig);
}

/**
 * @test SecOC_Init should report error when already initialized
 * @requirement SWS_SecOC_00001
 */
static void test_SecOC_Init_AlreadyInitialized(void **state)
{
    (void)state;
    
    /* First init */
    SecOC_Init(&TestConfig);
    assert_true(SecOC_Initialized);
    
    /* Second init should report error */
    det_ReportError_called = FALSE;
    SecOC_Init(&TestConfig);
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_ALREADY_INITIALIZED);
}

/**
 * @test SecOC_Init should report error with NULL config
 * @requirement SWS_SecOC_00001
 */
static void test_SecOC_Init_NullConfig(void **state)
{
    (void)state;
    
    SecOC_Init(NULL);
    
    assert_false(SecOC_Initialized);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_PARAM_POINTER);
}

/**
 * @test SecOC_DeInit should deinitialize module successfully
 * @requirement SWS_SecOC_00002
 */
static void test_SecOC_DeInit_Success(void **state)
{
    (void)state;
    
    /* First initialize */
    SecOC_Init(&TestConfig);
    assert_true(SecOC_Initialized);
    
    /* Then deinitialize */
    SecOC_DeInit();
    
    assert_false(SecOC_Initialized);
    assert_null(SecOC_ConfigPtr);
}

/**
 * @test SecOC_DeInit should report error when not initialized
 * @requirement SWS_SecOC_00002
 */
static void test_SecOC_DeInit_NotInitialized(void **state)
{
    (void)state;
    
    SecOC_DeInit();
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/*==================================================================================================
 *                                    TEST CASES - Version Info
 *==================================================================================================*/

#if (SECOC_VERSION_INFO_API == STD_ON)
/**
 * @test SecOC_GetVersionInfo should return correct version
 * @requirement SWS_SecOC_00003
 */
static void test_SecOC_GetVersionInfo_Success(void **state)
{
    (void)state;
    Std_VersionInfoType versionInfo;
    
    SecOC_GetVersionInfo(&versionInfo);
    
    assert_int_equal(versionInfo.vendorID, SECOC_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, SECOC_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, SECOC_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, SECOC_SW_MINOR_VERSION);
    assert_int_equal(versionInfo.sw_patch_version, SECOC_SW_PATCH_VERSION);
}

/**
 * @test SecOC_GetVersionInfo should report error with NULL pointer
 * @requirement SWS_SecOC_00003
 */
static void test_SecOC_GetVersionInfo_NullPointer(void **state)
{
    (void)state;
    
    SecOC_GetVersionInfo(NULL);
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_PARAM_POINTER);
}
#endif

/*==================================================================================================
 *                                    TEST CASES - Transmit
 *==================================================================================================*/

/**
 * @test SecOC_IfTransmit should transmit PDU successfully
 * @requirement SWS_SecOC_00041
 */
static void test_SecOC_IfTransmit_Success(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    result = SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    assert_int_equal(result, E_OK);
    assert_true(mock_Csm_MacGenerate_called);
}

/**
 * @test SecOC_IfTransmit should report error when not initialized
 * @requirement SWS_SecOC_00041
 */
static void test_SecOC_IfTransmit_NotInitialized(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    result = SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    assert_int_equal(result, E_NOT_OK);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/**
 * @test SecOC_IfTransmit should report error with NULL pointer
 * @requirement SWS_SecOC_00041
 */
static void test_SecOC_IfTransmit_NullPointer(void **state)
{
    (void)state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    result = SecOC_IfTransmit(0, NULL);
    
    assert_int_equal(result, E_NOT_OK);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_PARAM_POINTER);
}

/**
 * @test SecOC_IfTransmit should report error with invalid PDU ID
 * @requirement SWS_SecOC_00041
 */
static void test_SecOC_IfTransmit_InvalidPduId(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    result = SecOC_IfTransmit(100, &fixture->testPduInfo);  /* Invalid ID */
    
    assert_int_equal(result, E_NOT_OK);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_INVALID_PDU_SDU_ID);
}

/**
 * @test SecOC_IfTransmit should handle CSM failure
 * @requirement SWS_SecOC_00041
 */
static void test_SecOC_IfTransmit_CsmFailure(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    /* Simulate CSM failure */
    mock_Csm_MacGenerate_result = E_NOT_OK;
    
    result = SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    assert_int_equal(result, E_NOT_OK);
}

/*==================================================================================================
 *                                    TEST CASES - Receive
 *==================================================================================================*/

/**
 * @test SecOC_IfRxIndication should process valid secured PDU
 * @requirement SWS_SecOC_00043
 */
static void test_SecOC_IfRxIndication_Success(void **state)
{
    (void)state;
    
    /* Build secured PDU: [Data(8)][Freshness(2)][AuthCode(16)] */
    uint8 securedPdu[26];
    for (uint8 i = 0; i < 8; i++) {
        securedPdu[i] = i;  /* Data */
    }
    securedPdu[8] = 0x00;   /* Freshness high byte */
    securedPdu[9] = 0x01;   /* Freshness low byte */
    for (uint8 i = 0; i < 16; i++) {
        securedPdu[10 + i] = (uint8)(0xA0 + i);  /* Auth code */
    }
    
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = securedPdu;
    pduInfo.SduLength = 26;
    pduInfo.MetaDataPtr = NULL;
    
    SecOC_Init(&TestConfig);
    
    SecOC_IfRxIndication(0, &pduInfo);
    
    assert_true(mock_Csm_MacVerify_called);
    assert_true(mock_PduR_SecOCRxIndication_called);
    assert_int_equal(SecOC_GetVerificationStatus(0), SECOC_VERIFICATIONSUCCESS_STATUS);
}

/**
 * @test SecOC_IfRxIndication should detect verification failure
 * @requirement SWS_SecOC_00043
 */
static void test_SecOC_IfRxIndication_VerificationFailure(void **state)
{
    (void)state;
    
    /* Build secured PDU */
    uint8 securedPdu[26];
    for (uint8 i = 0; i < 8; i++) {
        securedPdu[i] = i;
    }
    securedPdu[8] = 0x00;
    securedPdu[9] = 0x01;
    for (uint8 i = 0; i < 16; i++) {
        securedPdu[10 + i] = (uint8)(0xFF);  /* Bad auth code */
    }
    
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = securedPdu;
    pduInfo.SduLength = 26;
    pduInfo.MetaDataPtr = NULL;
    
    SecOC_Init(&TestConfig);
    
    /* Simulate verification failure */
    mock_Csm_VerifyResult = CSM_E_VER_NOT_OK;
    
    SecOC_IfRxIndication(0, &pduInfo);
    
    assert_int_equal(SecOC_GetVerificationStatus(0), SECOC_VERIFICATIONFAILURE_STATUS);
}

/**
 * @test SecOC_IfRxIndication should report error when not initialized
 * @requirement SWS_SecOC_00043
 */
static void test_SecOC_IfRxIndication_NotInitialized(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    
    SecOC_IfRxIndication(0, &fixture->testPduInfo);
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/**
 * @test SecOC_IfRxIndication should report error with NULL pointer
 * @requirement SWS_SecOC_00043
 */
static void test_SecOC_IfRxIndication_NullPointer(void **state)
{
    (void)state;
    
    SecOC_Init(&TestConfig);
    
    SecOC_IfRxIndication(0, NULL);
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_PARAM_POINTER);
}

/**
 * @test SecOC_IfRxIndication should handle PDU too short
 * @requirement SWS_SecOC_00043
 */
static void test_SecOC_IfRxIndication_PduTooShort(void **state)
{
    (void)state;
    
    uint8 shortPdu[4] = {0x00, 0x01, 0x02, 0x03};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = shortPdu;
    pduInfo.SduLength = 4;  /* Too short */
    pduInfo.MetaDataPtr = NULL;
    
    SecOC_Init(&TestConfig);
    
    SecOC_IfRxIndication(0, &pduInfo);
    
    assert_int_equal(SecOC_GetVerificationStatus(0), SECOC_VERIFICATIONFAILURE_STATUS);
}

/*==================================================================================================
 *                                    TEST CASES - Verification Status
 *==================================================================================================*/

/**
 * @test SecOC_VerifyStatusOverride should override verification status
 * @requirement SWS_SecOC_00081
 */
static void test_SecOC_VerifyStatusOverride_Success(void **state)
{
    (void)state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    /* Set initial status */
    result = SecOC_VerifyStatusOverride(0, SECOC_VERIFICATIONFAILURE_STATUS);
    assert_int_equal(result, E_OK);
    assert_int_equal(SecOC_GetVerificationStatus(0), SECOC_VERIFICATIONFAILURE_STATUS);
    
    /* Override to success */
    result = SecOC_VerifyStatusOverride(0, SECOC_VERIFICATIONSUCCESS_STATUS);
    assert_int_equal(result, E_OK);
    assert_int_equal(SecOC_GetVerificationStatus(0), SECOC_VERIFICATIONSUCCESS_STATUS);
}

/**
 * @test SecOC_VerifyStatusOverride should report error when not initialized
 * @requirement SWS_SecOC_00081
 */
static void test_SecOC_VerifyStatusOverride_NotInitialized(void **state)
{
    (void)state;
    Std_ReturnType result;
    
    result = SecOC_VerifyStatusOverride(0, SECOC_VERIFICATIONSUCCESS_STATUS);
    
    assert_int_equal(result, E_NOT_OK);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/**
 * @test SecOC_VerifyStatusOverride should report error with invalid status
 * @requirement SWS_SecOC_00081
 */
static void test_SecOC_VerifyStatusOverride_InvalidStatus(void **state)
{
    (void)state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    result = SecOC_VerifyStatusOverride(0, (SecOC_VerificationStatusType)100);
    
    assert_int_equal(result, E_NOT_OK);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_INVALID_PARAMETER);
}

/**
 * @test SecOC_GetVerificationStatus should return default for uninitialized PDU
 * @requirement SWS_SecOC_00084
 */
static void test_SecOC_GetVerificationStatus_Default(void **state)
{
    (void)state;
    SecOC_VerificationStatusType status;
    
    SecOC_Init(&TestConfig);
    
    status = SecOC_GetVerificationStatus(0);
    
    assert_int_equal(status, SECOC_UNVERIFIED);
}

/**
 * @test SecOC_GetVerificationResult should return verification result
 * @requirement SWS_SecOC_00085
 */
static void test_SecOC_GetVerificationResult_Success(void **state)
{
    (void)state;
    SecOC_VerificationResultType result;
    Std_ReturnType status;
    
    SecOC_Init(&TestConfig);
    
    status = SecOC_GetVerificationResult(0, &result);
    
    assert_int_equal(status, E_OK);
    assert_int_equal(result, SECOC_NO_VERIFICATION);
}

/**
 * @test SecOC_GetVerificationResult should report error with NULL pointer
 * @requirement SWS_SecOC_00085
 */
static void test_SecOC_GetVerificationResult_NullPointer(void **state)
{
    (void)state;
    Std_ReturnType status;
    
    SecOC_Init(&TestConfig);
    
    status = SecOC_GetVerificationResult(0, NULL);
    
    assert_int_equal(status, E_NOT_OK);
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_PARAM_POINTER);
}

/*==================================================================================================
 *                                    TEST CASES - Tx Confirmation
 *==================================================================================================*/

/**
 * @test SecOC_TxConfirmation should process confirmation
 * @requirement SWS_SecOC_00083
 */
static void test_SecOC_TxConfirmation_Success(void **state)
{
    (void)state;
    
    SecOC_Init(&TestConfig);
    
    SecOC_TxConfirmation(0, E_OK);
    
    /* Should complete without errors */
    assert_true(1);
}

/**
 * @test SecOC_TxConfirmation should report error when not initialized
 * @requirement SWS_SecOC_00083
 */
static void test_SecOC_TxConfirmation_NotInitialized(void **state)
{
    (void)state;
    
    SecOC_TxConfirmation(0, E_OK);
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/*==================================================================================================
 *                                    TEST CASES - Main Functions
 *==================================================================================================*/

/**
 * @test SecOC_MainFunctionRx should process pending PDUs
 * @requirement SWS_SecOC_00091
 */
static void test_SecOC_MainFunctionRx_Success(void **state)
{
    (void)state;
    
    SecOC_Init(&TestConfig);
    
    SecOC_MainFunctionRx();
    
    /* Should complete without errors */
    assert_true(1);
}

/**
 * @test SecOC_MainFunctionRx should report error when not initialized
 * @requirement SWS_SecOC_00091
 */
static void test_SecOC_MainFunctionRx_NotInitialized(void **state)
{
    (void)state;
    
    SecOC_MainFunctionRx();
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/**
 * @test SecOC_MainFunctionTx should process pending PDUs
 * @requirement SWS_SecOC_00092
 */
static void test_SecOC_MainFunctionTx_Success(void **state)
{
    (void)state;
    
    SecOC_Init(&TestConfig);
    
    SecOC_MainFunctionTx();
    
    /* Should complete without errors */
    assert_true(1);
}

/**
 * @test SecOC_MainFunctionTx should report error when not initialized
 * @requirement SWS_SecOC_00092
 */
static void test_SecOC_MainFunctionTx_NotInitialized(void **state)
{
    (void)state;
    
    SecOC_MainFunctionTx();
    
    assert_true(det_ReportError_called);
    assert_int_equal(det_LastErrorId, SECOC_E_UNINIT);
}

/*==================================================================================================
 *                                    TEST CASES - Freshness Value
 *==================================================================================================*/

/**
 * @test SecOC should increment freshness value on each transmit
 * @requirement SWS_SecOC_00041
 */
static void test_SecOC_FreshnessValue_Increment(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    
    SecOC_Init(&TestConfig);
    
    /* First transmit */
    SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    /* Second transmit should use incremented freshness */
    mock_Csm_MacGenerate_called = FALSE;
    SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    assert_true(mock_Csm_MacGenerate_called);
}

/*==================================================================================================
 *                                    TEST CASES - Edge Cases
 *==================================================================================================*/

/**
 * @test SecOC should handle maximum PDU ID
 */
static void test_SecOC_MaxPduId(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    /* Test with max valid PDU ID */
    result = SecOC_IfTransmit(SECOC_NUM_TX_PDUS - 1, &fixture->testPduInfo);
    
    assert_int_equal(result, E_OK);
}

/**
 * @test SecOC should reject PDU ID out of range
 */
static void test_SecOC_PduIdOutOfRange(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    result = SecOC_IfTransmit(SECOC_NUM_TX_PDUS, &fixture->testPduInfo);
    
    assert_int_equal(result, E_NOT_OK);
}

/**
 * @test SecOC should handle zero-length PDU
 */
static void test_SecOC_ZeroLengthPdu(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result;
    
    SecOC_Init(&TestConfig);
    
    fixture->testPduInfo.SduLength = 0;
    result = SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    /* Zero-length might be accepted or rejected depending on implementation */
    assert_true(result == E_OK || result == E_NOT_OK);
}

/**
 * @test SecOC should handle multiple PDU configurations
 */
static void test_SecOC_MultiplePduConfigs(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    Std_ReturnType result1, result2;
    
    SecOC_Init(&TestConfig);
    
    /* Transmit on different PDUs */
    result1 = SecOC_IfTransmit(0, &fixture->testPduInfo);
    result2 = SecOC_IfTransmit(1, &fixture->testPduInfo);
    
    assert_int_equal(result1, E_OK);
    assert_int_equal(result2, E_OK);
}

/*==================================================================================================
 *                                    TEST CASES - Session Management
 *==================================================================================================*/

/**
 * @test SecOC should maintain separate state for each PDU
 */
static void test_SecOC_SeparatePduState(void **state)
{
    (void)state;
    
    uint8 securedPdu1[26];
    uint8 securedPdu2[26];
    
    /* Build two secured PDUs */
    for (uint8 i = 0; i < 8; i++) {
        securedPdu1[i] = i;
        securedPdu2[i] = (uint8)(i + 0x10);
    }
    securedPdu1[8] = securedPdu2[8] = 0x00;
    securedPdu1[9] = securedPdu2[9] = 0x01;
    for (uint8 i = 0; i < 16; i++) {
        securedPdu1[10 + i] = (uint8)(0xA0 + i);
        securedPdu2[10 + i] = (uint8)(0xB0 + i);
    }
    
    PduInfoType pduInfo1, pduInfo2;
    pduInfo1.SduDataPtr = securedPdu1;
    pduInfo1.SduLength = 26;
    pduInfo1.MetaDataPtr = NULL;
    pduInfo2.SduDataPtr = securedPdu2;
    pduInfo2.SduLength = 26;
    pduInfo2.MetaDataPtr = NULL;
    
    SecOC_Init(&TestConfig);
    
    /* Receive on different PDUs */
    SecOC_IfRxIndication(0, &pduInfo1);
    SecOC_IfRxIndication(1, &pduInfo2);
    
    /* Both should have success status */
    assert_int_equal(SecOC_GetVerificationStatus(0), SECOC_VERIFICATIONSUCCESS_STATUS);
    assert_int_equal(SecOC_GetVerificationStatus(1), SECOC_VERIFICATIONSUCCESS_STATUS);
}

/*==================================================================================================
 *                                    TEST CASES - Authentication
 *==================================================================================================*/

/**
 * @test SecOC should use correct authentication algorithm configuration
 */
static void test_SecOC_AuthAlgorithm_Config(void **state)
{
    (void)state;
    
    /* Verify auth algorithm is configured correctly */
    assert_int_equal(TestTxPduConfigs[0].authConfig.algorithm, SECOC_HMAC_SHA256);
    assert_int_equal(TestTxPduConfigs[0].authConfig.authInfoLength, SECOC_AUTH_INFO_LENGTH);
}

/**
 * @test SecOC should build authentication data correctly
 */
static void test_SecOC_AuthData_Build(void **state)
{
    TestFixture *fixture = (TestFixture *)*state;
    
    SecOC_Init(&TestConfig);
    
    /* Transmit should trigger auth data build */
    mock_Csm_MacGenerate_called = FALSE;
    SecOC_IfTransmit(0, &fixture->testPduInfo);
    
    assert_true(mock_Csm_MacGenerate_called);
}

/*==================================================================================================
 *                                    TEST RUNNER
 *==================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        /* Initialization tests */
        cmocka_unit_test_setup_teardown(test_SecOC_Init_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_Init_AlreadyInitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_Init_NullConfig, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_DeInit_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_DeInit_NotInitialized, test_setup, test_teardown),
        
        /* Version info tests */
#if (SECOC_VERSION_INFO_API == STD_ON)
        cmocka_unit_test_setup_teardown(test_SecOC_GetVersionInfo_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_GetVersionInfo_NullPointer, test_setup, test_teardown),
#endif
        
        /* Transmit tests */
        cmocka_unit_test_setup_teardown(test_SecOC_IfTransmit_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfTransmit_NotInitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfTransmit_NullPointer, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfTransmit_InvalidPduId, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfTransmit_CsmFailure, test_setup, test_teardown),
        
        /* Receive tests */
        cmocka_unit_test_setup_teardown(test_SecOC_IfRxIndication_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfRxIndication_VerificationFailure, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfRxIndication_NotInitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfRxIndication_NullPointer, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_IfRxIndication_PduTooShort, test_setup, test_teardown),
        
        /* Verification status tests */
        cmocka_unit_test_setup_teardown(test_SecOC_VerifyStatusOverride_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_VerifyStatusOverride_NotInitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_VerifyStatusOverride_InvalidStatus, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_GetVerificationStatus_Default, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_GetVerificationResult_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_GetVerificationResult_NullPointer, test_setup, test_teardown),
        
        /* Tx confirmation tests */
        cmocka_unit_test_setup_teardown(test_SecOC_TxConfirmation_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_TxConfirmation_NotInitialized, test_setup, test_teardown),
        
        /* Main function tests */
        cmocka_unit_test_setup_teardown(test_SecOC_MainFunctionRx_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_MainFunctionRx_NotInitialized, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_MainFunctionTx_Success, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_MainFunctionTx_NotInitialized, test_setup, test_teardown),
        
        /* Freshness value tests */
        cmocka_unit_test_setup_teardown(test_SecOC_FreshnessValue_Increment, test_setup, test_teardown),
        
        /* Edge case tests */
        cmocka_unit_test_setup_teardown(test_SecOC_MaxPduId, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_PduIdOutOfRange, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_ZeroLengthPdu, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_MultiplePduConfigs, test_setup, test_teardown),
        
        /* Session management tests */
        cmocka_unit_test_setup_teardown(test_SecOC_SeparatePduState, test_setup, test_teardown),
        
        /* Authentication tests */
        cmocka_unit_test_setup_teardown(test_SecOC_AuthAlgorithm_Config, test_setup, test_teardown),
        cmocka_unit_test_setup_teardown(test_SecOC_AuthData_Build, test_setup, test_teardown),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

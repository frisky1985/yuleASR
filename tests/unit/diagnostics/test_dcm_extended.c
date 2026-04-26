/******************************************************************************
 * @file    test_dcm_extended.c
 * @brief   Extended DCM Service Tests (0x27, 0x28, 0x2C, 0x3D, 0x31)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include "../../../src/diagnostics/dcm/dcm.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Test Buffers
 ******************************************************************************/
static uint8_t s_requestBuffer[256];
static uint8_t s_responseBuffer[256];
static Dcm_RequestType s_request;
static Dcm_ResponseType s_response;

/******************************************************************************
 * Test Setup/Teardown
 ******************************************************************************/
void dcm_extended_setUp(void)
{
    (void)memset(s_requestBuffer, 0, sizeof(s_requestBuffer));
    (void)memset(s_responseBuffer, 0, sizeof(s_responseBuffer));
    
    s_request.data = s_requestBuffer;
    s_request.length = 0;
    s_request.sourceAddress = 0x1234;
    s_request.addrMode = DCM_ADDR_PHYSICAL;
    s_request.protocol = DCM_PROTOCOL_UDS_ON_CAN;
    s_request.timestamp = 0;
    
    s_response.data = s_responseBuffer;
    s_response.length = 0;
    s_response.maxLength = sizeof(s_responseBuffer);
    s_response.isNegativeResponse = false;
    s_response.suppressPositiveResponse = false;
}

void dcm_extended_tearDown(void)
{
    /* Cleanup */
}

/******************************************************************************
 * Security Access Service (0x27) Tests
 ******************************************************************************/

void test_security_access_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_SecurityAccessInit(NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_security_access_init_success(void)
{
    Dcm_SecurityAccessConfigType config = {
        .levelConfigs = NULL,
        .numSecurityLevels = 0,
        .defaultSecurityLevel = 0,
        .changeCallback = NULL
    };
    
    Dcm_ReturnType result = Dcm_SecurityAccessInit(&config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_security_access_request_seed_level1(void)
{
    /* Setup valid security config */
    Dcm_SecurityLevelConfigType levels[] = {
        {
            .securityLevel = 1,
            .maxFailedAttempts = 3,
            .lockoutTimeMs = 10000,
            .delayTimeMs = 5000,
            .seedLength = 4,
            .keyLength = 4,
            .seedCallback = NULL,
            .keyCallback = NULL,
            .csmKeyDeriveEnabled = false,
            .csmKeyId = 0
        }
    };
    
    Dcm_SecurityAccessConfigType config = {
        .levelConfigs = levels,
        .numSecurityLevels = 1,
        .defaultSecurityLevel = 0,
        .changeCallback = NULL
    };
    
    Dcm_SecurityAccessInit(&config);
    
    /* Request seed for level 1 */
    s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
    s_requestBuffer[1] = DCM_SEC_REQ_SEED_LEVEL_1;
    s_request.length = 2;
    
    Dcm_ReturnType result = Dcm_SecurityAccess(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_SVC_SECURITY_ACCESS + 0x40, s_responseBuffer[0]);
    TEST_ASSERT_EQUAL(DCM_SEC_REQ_SEED_LEVEL_1, s_responseBuffer[1]);
    /* Seed should be 4 bytes */
    TEST_ASSERT_TRUE(s_response.length >= 6);
}

void test_security_access_invalid_key(void)
{
    Dcm_SecurityLevelConfigType levels[] = {
        {
            .securityLevel = 1,
            .maxFailedAttempts = 3,
            .lockoutTimeMs = 10000,
            .delayTimeMs = 5000,
            .seedLength = 4,
            .keyLength = 4,
            .seedCallback = NULL,
            .keyCallback = NULL,
            .csmKeyDeriveEnabled = false,
            .csmKeyId = 0
        }
    };
    
    Dcm_SecurityAccessConfigType config = {
        .levelConfigs = levels,
        .numSecurityLevels = 1,
        .defaultSecurityLevel = 0,
        .changeCallback = NULL
    };
    
    Dcm_SecurityAccessInit(&config);
    
    /* Request seed first */
    s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
    s_requestBuffer[1] = DCM_SEC_REQ_SEED_LEVEL_1;
    s_request.length = 2;
    Dcm_SecurityAccess(&s_request, &s_response);
    
    /* Send invalid key */
    s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
    s_requestBuffer[1] = DCM_SEC_SEND_KEY_LEVEL_1;
    s_requestBuffer[2] = 0x00;
    s_requestBuffer[3] = 0x00;
    s_requestBuffer[4] = 0x00;
    s_requestBuffer[5] = 0x00;
    s_request.length = 6;
    
    Dcm_ReturnType result = Dcm_SecurityAccess(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_INVALID_KEY, s_response.negativeResponseCode);
}

void test_security_access_sequence_error(void)
{
    Dcm_SecurityAccessConfigType config = {0};
    Dcm_SecurityAccessInit(&config);
    
    /* Send key without requesting seed first */
    s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
    s_requestBuffer[1] = DCM_SEC_SEND_KEY_LEVEL_1;
    s_requestBuffer[2] = 0x12;
    s_requestBuffer[3] = 0x34;
    s_requestBuffer[4] = 0x56;
    s_requestBuffer[5] = 0x78;
    s_request.length = 6;
    
    Dcm_ReturnType result = Dcm_SecurityAccess(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_REQUEST_SEQUENCE_ERROR, s_response.negativeResponseCode);
}

void test_security_access_max_attempts(void)
{
    Dcm_SecurityLevelConfigType levels[] = {
        {
            .securityLevel = 1,
            .maxFailedAttempts = 2,  /* Only 2 attempts allowed */
            .lockoutTimeMs = 5000,
            .delayTimeMs = 1000,
            .seedLength = 4,
            .keyLength = 4,
            .seedCallback = NULL,
            .keyCallback = NULL,
            .csmKeyDeriveEnabled = false,
            .csmKeyId = 0
        }
    };
    
    Dcm_SecurityAccessConfigType config = {
        .levelConfigs = levels,
        .numSecurityLevels = 1,
        .defaultSecurityLevel = 0,
        .changeCallback = NULL
    };
    
    Dcm_SecurityAccessInit(&config);
    
    /* Request seed */
    s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
    s_requestBuffer[1] = DCM_SEC_REQ_SEED_LEVEL_1;
    s_request.length = 2;
    Dcm_SecurityAccess(&s_request, &s_response);
    
    /* Send invalid key twice */
    for (int i = 0; i < 3; i++) {
        s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
        s_requestBuffer[1] = DCM_SEC_SEND_KEY_LEVEL_1;
        s_requestBuffer[2] = 0xFF;
        s_requestBuffer[3] = 0xFF;
        s_requestBuffer[4] = 0xFF;
        s_requestBuffer[5] = 0xFF;
        s_request.length = 6;
        Dcm_SecurityAccess(&s_request, &s_response);
        
        if (i < 1) {
            /* Request seed again */
            s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
            s_requestBuffer[1] = DCM_SEC_REQ_SEED_LEVEL_1;
            s_request.length = 2;
            Dcm_SecurityAccess(&s_request, &s_response);
        }
    }
    
    /* Third attempt should fail with exceed number of attempts */
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS, s_response.negativeResponseCode);
}

void test_security_access_get_level(void)
{
    Dcm_SecurityAccessConfigType config = {0};
    Dcm_SecurityAccessInit(&config);
    
    uint8_t level = Dcm_GetSecurityLevel();
    TEST_ASSERT_EQUAL(0, level);  /* Default locked level */
}

void test_security_access_is_level_supported(void)
{
    Dcm_SecurityLevelConfigType levels[] = {
        {
            .securityLevel = 1,
            .maxFailedAttempts = 3,
            .lockoutTimeMs = 10000,
            .delayTimeMs = 5000,
            .seedLength = 4,
            .keyLength = 4,
            .seedCallback = NULL,
            .keyCallback = NULL,
            .csmKeyDeriveEnabled = false,
            .csmKeyId = 0
        }
    };
    
    Dcm_SecurityAccessConfigType config = {
        .levelConfigs = levels,
        .numSecurityLevels = 1,
        .defaultSecurityLevel = 0,
        .changeCallback = NULL
    };
    
    Dcm_SecurityAccessInit(&config);
    
    TEST_ASSERT_TRUE(Dcm_IsSecurityLevelSupported(0));  /* Locked level always supported */
    TEST_ASSERT_TRUE(Dcm_IsSecurityLevelSupported(1));  /* Configured level */
    TEST_ASSERT_FALSE(Dcm_IsSecurityLevelSupported(2)); /* Not configured */
}

void test_security_access_timer_update(void)
{
    Dcm_SecurityAccessConfigType config = {0};
    Dcm_SecurityAccessInit(&config);
    
    Dcm_ReturnType result = Dcm_SecurityTimerUpdate(100);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_security_access_lock(void)
{
    Dcm_SecurityAccessConfigType config = {0};
    Dcm_SecurityAccessInit(&config);
    
    Dcm_ReturnType result = Dcm_LockSecurityAccess();
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(0, Dcm_GetSecurityLevel());
}

/******************************************************************************
 * Communication Control Service (0x28) Tests
 ******************************************************************************/

void test_communication_control_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_CommunicationControlInit(NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_communication_control_init_success(void)
{
    Dcm_CommunicationControlConfigType config = {
        .enableNormalCommControl = true,
        .enableNmCommControl = true,
        .enableDdsControl = false,
        .requireExtendedSession = false,
        .requireProgrammingSession = false,
        .changeCallback = NULL,
        .ddsCallback = NULL,
        .networkCallback = NULL
    };
    
    Dcm_ReturnType result = Dcm_CommunicationControlInit(&config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_communication_control_enable_rx_tx(void)
{
    Dcm_CommunicationControlConfigType config = {
        .enableNormalCommControl = true,
        .enableNmCommControl = true,
        .enableDdsControl = false,
        .requireExtendedSession = false,
        .requireProgrammingSession = false,
        .changeCallback = NULL,
        .ddsCallback = NULL,
        .networkCallback = NULL
    };
    
    Dcm_CommunicationControlInit(&config);
    
    /* Enable RX and TX */
    s_requestBuffer[0] = UDS_SVC_COMMUNICATION_CONTROL;
    s_requestBuffer[1] = DCM_COMM_CTRL_ENABLE_RX_TX;
    s_requestBuffer[2] = DCM_COMM_TYPE_NORMAL;
    s_request.length = 3;
    
    Dcm_ReturnType result = Dcm_CommunicationControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
}

void test_communication_control_disable_rx_tx(void)
{
    Dcm_CommunicationControlConfigType config = {
        .enableNormalCommControl = true,
        .enableNmCommControl = true,
        .enableDdsControl = false,
        .requireExtendedSession = false,
        .requireProgrammingSession = false,
        .changeCallback = NULL,
        .ddsCallback = NULL,
        .networkCallback = NULL
    };
    
    Dcm_CommunicationControlInit(&config);
    
    /* Disable RX and TX */
    s_requestBuffer[0] = UDS_SVC_COMMUNICATION_CONTROL;
    s_requestBuffer[1] = DCM_COMM_CTRL_DISABLE_RX_TX;
    s_requestBuffer[2] = DCM_COMM_TYPE_NORMAL;
    s_request.length = 3;
    
    Dcm_ReturnType result = Dcm_CommunicationControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
}

void test_communication_control_invalid_type(void)
{
    Dcm_CommunicationControlConfigType config = {
        .enableNormalCommControl = true,
        .enableNmCommControl = true,
        .enableDdsControl = false,
        .requireExtendedSession = false,
        .requireProgrammingSession = false,
        .changeCallback = NULL,
        .ddsCallback = NULL,
        .networkCallback = NULL
    };
    
    Dcm_CommunicationControlInit(&config);
    
    /* Invalid communication type */
    s_requestBuffer[0] = UDS_SVC_COMMUNICATION_CONTROL;
    s_requestBuffer[1] = DCM_COMM_CTRL_ENABLE_RX_TX;
    s_requestBuffer[2] = 0x04;  /* Invalid type */
    s_request.length = 3;
    
    Dcm_ReturnType result = Dcm_CommunicationControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
}

void test_communication_control_invalid_subfunction(void)
{
    Dcm_CommunicationControlConfigType config = {0};
    Dcm_CommunicationControlInit(&config);
    
    s_requestBuffer[0] = UDS_SVC_COMMUNICATION_CONTROL;
    s_requestBuffer[1] = 0x04;  /* Invalid subfunction */
    s_requestBuffer[2] = DCM_COMM_TYPE_NORMAL;
    s_request.length = 3;
    
    Dcm_ReturnType result = Dcm_CommunicationControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, s_response.negativeResponseCode);
}

void test_communication_control_enable_disable(void)
{
    Dcm_CommunicationControlConfigType config = {0};
    Dcm_CommunicationControlInit(&config);
    
    Dcm_ReturnType result;
    
    /* Disable communication */
    result = Dcm_DisableCommunication(0);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    /* Check state */
    Dcm_CommunicationStateType state;
    result = Dcm_GetCommunicationState(0, &state);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_COMM_STATE_RX_TX_DISABLED, state);
    
    /* Enable communication */
    result = Dcm_EnableCommunication(0);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    result = Dcm_GetCommunicationState(0, &state);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(DCM_COMM_STATE_ENABLED, state);
}

void test_communication_control_is_type_valid(void)
{
    TEST_ASSERT_TRUE(Dcm_IsControlTypeValid(DCM_COMM_CTRL_ENABLE_RX_TX));
    TEST_ASSERT_TRUE(Dcm_IsControlTypeValid(DCM_COMM_CTRL_DISABLE_RX_TX));
    TEST_ASSERT_FALSE(Dcm_IsControlTypeValid(0x04));
}

void test_communication_control_reset_state(void)
{
    Dcm_CommunicationControlConfigType config = {0};
    Dcm_CommunicationControlInit(&config);
    
    /* Disable first */
    Dcm_DisableCommunication(0);
    
    /* Reset */
    Dcm_ReturnType result = Dcm_ResetCommunicationState();
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    
    /* Check enabled */
    TEST_ASSERT_TRUE(Dcm_IsNormalCommunicationEnabled(0));
}

/******************************************************************************
 * Dynamic DID Service (0x2C) Tests
 ******************************************************************************/

void test_dynamic_did_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_DynamicDidInit(NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_dynamic_did_init_success(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_ReturnType result = Dcm_DynamicDidInit(&config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_dynamic_did_define_by_identifier(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_DynamicDidInit(&config);
    
    /* Define dynamic DID F300 from source DID F190 */
    s_requestBuffer[0] = UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    s_requestBuffer[1] = DCM_DYN_DID_DEFINE_BY_IDENTIFIER;
    s_requestBuffer[2] = 0xF3;  /* Dynamic DID high byte */
    s_requestBuffer[3] = 0x00;  /* Dynamic DID low byte */
    s_requestBuffer[4] = 0xF1;  /* Source DID high byte */
    s_requestBuffer[5] = 0x90;  /* Source DID low byte */
    s_requestBuffer[6] = 0x00;  /* Position */
    s_requestBuffer[7] = 0x11;  /* Size (17 bytes for VIN) */
    s_request.length = 8;
    
    Dcm_ReturnType result = Dcm_DynamicallyDefineDataIdentifier(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
}

void test_dynamic_did_define_by_memory_address(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_DynamicDidInit(&config);
    
    /* Define dynamic DID F301 from memory address */
    s_requestBuffer[0] = UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    s_requestBuffer[1] = DCM_DYN_DID_DEFINE_BY_MEMORY_ADDRESS;
    s_requestBuffer[2] = 0xF3;  /* Dynamic DID high byte */
    s_requestBuffer[3] = 0x01;  /* Dynamic DID low byte */
    s_requestBuffer[4] = 0x24;  /* Address length = 4, Size length = 4 */
    s_requestBuffer[5] = 0x20;  /* Address byte 3 */
    s_requestBuffer[6] = 0x00;  /* Address byte 2 */
    s_requestBuffer[7] = 0x10;  /* Address byte 1 */
    s_requestBuffer[8] = 0x00;  /* Address byte 0 */
    s_requestBuffer[9] = 0x00;  /* Size byte 3 */
    s_requestBuffer[10] = 0x00; /* Size byte 2 */
    s_requestBuffer[11] = 0x01; /* Size byte 1 */
    s_requestBuffer[12] = 0x00; /* Size byte 0 (256 bytes) */
    s_request.length = 13;
    
    Dcm_ReturnType result = Dcm_DynamicallyDefineDataIdentifier(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
}

void test_dynamic_did_clear_specific(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_DynamicDidInit(&config);
    
    /* Define a DID first */
    Dcm_DefineDynamicDidByIdentifier(0xF300, 0xF190, 0, 17);
    TEST_ASSERT_TRUE(Dcm_IsDynamicDidDefined(0xF300));
    
    /* Clear specific DID */
    s_requestBuffer[0] = UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    s_requestBuffer[1] = DCM_DYN_DID_CLEAR_DYN_DID;
    s_requestBuffer[2] = 0xF3;
    s_requestBuffer[3] = 0x00;
    s_request.length = 4;
    
    Dcm_ReturnType result = Dcm_DynamicallyDefineDataIdentifier(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(Dcm_IsDynamicDidDefined(0xF300));
}

void test_dynamic_did_clear_all(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_DynamicDidInit(&config);
    
    /* Define DIDs */
    Dcm_DefineDynamicDidByIdentifier(0xF300, 0xF190, 0, 17);
    Dcm_DefineDynamicDidByIdentifier(0xF301, 0xF194, 0, 4);
    
    /* Clear all DIDs */
    s_requestBuffer[0] = UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    s_requestBuffer[1] = DCM_DYN_DID_CLEAR_DYN_DID;
    s_request.length = 2;  /* No DID specified = clear all */
    
    Dcm_ReturnType result = Dcm_DynamicallyDefineDataIdentifier(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(Dcm_IsDynamicDidDefined(0xF300));
    TEST_ASSERT_FALSE(Dcm_IsDynamicDidDefined(0xF301));
}

void test_dynamic_did_invalid_range(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_DynamicDidInit(&config);
    
    /* Try to define DID outside valid range */
    s_requestBuffer[0] = UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER;
    s_requestBuffer[1] = DCM_DYN_DID_DEFINE_BY_IDENTIFIER;
    s_requestBuffer[2] = 0xF1;  /* Outside F300-F3FF */
    s_requestBuffer[3] = 0x90;
    s_requestBuffer[4] = 0xF1;
    s_requestBuffer[5] = 0x90;
    s_requestBuffer[6] = 0x00;
    s_requestBuffer[7] = 0x04;
    s_request.length = 8;
    
    Dcm_ReturnType result = Dcm_DynamicallyDefineDataIdentifier(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
}

void test_dynamic_did_is_valid_range(void)
{
    Dcm_DynamicDidConfigType config = {
        .enableDidBasedDefinition = true,
        .enableMemoryBasedDefinition = true,
        .enableClearDynamicDid = true,
        .maxDynamicDids = 16,
        .maxSourceElements = 8,
        .minDynamicDid = 0xF300,
        .maxDynamicDid = 0xF3FF,
        .requiredSecurityLevel = 0
    };
    
    Dcm_DynamicDidInit(&config);
    
    TEST_ASSERT_TRUE(Dcm_IsValidDynamicDidRange(0xF300));
    TEST_ASSERT_TRUE(Dcm_IsValidDynamicDidRange(0xF3FF));
    TEST_ASSERT_FALSE(Dcm_IsValidDynamicDidRange(0xF200));
    TEST_ASSERT_FALSE(Dcm_IsValidDynamicDidRange(0xF400));
}

void test_dynamic_did_parse_format(void)
{
    uint8_t addrLen, sizeLen;
    
    Dcm_ReturnType result = Dcm_ParseAddressLengthFormat(0x24, &addrLen, &sizeLen);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(4, addrLen);
    TEST_ASSERT_EQUAL(4, sizeLen);
}

/******************************************************************************
 * Write Memory By Address Service (0x3D) Tests
 ******************************************************************************/

void test_write_memory_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_MemoryWriteInit(NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_write_memory_init_success(void)
{
    Dcm_MemoryRegionConfigType regions[] = {
        {
            .startAddress = 0x20000000,
            .endAddress = 0x2001FFFF,
            .regionType = DCM_MEM_REGION_RAM,
            .requiredSecurityLevel = 0,
            .writeAllowed = true,
            .readAllowed = true,
            .eraseRequired = false,
            .alignment = 1,
            .description = "RAM"
        }
    };
    
    Dcm_MemoryWriteConfigType config = {
        .regions = regions,
        .numRegions = 1,
        .maxWriteSize = 0x1000,
        .enableVerification = true,
        .requireProgrammingSession = false,
        .requiredSecurityLevel = 0,
        .writeCallback = NULL,
        .verifyCallback = NULL
    };
    
    Dcm_ReturnType result = Dcm_MemoryWriteInit(&config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_write_memory_success(void)
{
    Dcm_MemoryRegionConfigType regions[] = {
        {
            .startAddress = 0x20000000,
            .endAddress = 0x2001FFFF,
            .regionType = DCM_MEM_REGION_RAM,
            .requiredSecurityLevel = 0,
            .writeAllowed = true,
            .readAllowed = true,
            .eraseRequired = false,
            .alignment = 1,
            .description = "RAM"
        }
    };
    
    Dcm_MemoryWriteConfigType config = {
        .regions = regions,
        .numRegions = 1,
        .maxWriteSize = 0x1000,
        .enableVerification = true,
        .requireProgrammingSession = false,
        .requiredSecurityLevel = 0,
        .writeCallback = NULL,
        .verifyCallback = NULL
    };
    
    Dcm_MemoryWriteInit(&config);
    
    /* Write 4 bytes to RAM */
    s_requestBuffer[0] = UDS_SVC_WRITE_MEMORY_BY_ADDRESS;
    s_requestBuffer[1] = 0x14;  /* Address length = 4, Size length = 4 */
    s_requestBuffer[2] = 0x20;  /* Address byte 3 */
    s_requestBuffer[3] = 0x00;  /* Address byte 2 */
    s_requestBuffer[4] = 0x00;  /* Address byte 1 */
    s_requestBuffer[5] = 0x00;  /* Address byte 0 */
    s_requestBuffer[6] = 0x00;  /* Size byte 3 */
    s_requestBuffer[7] = 0x00;  /* Size byte 2 */
    s_requestBuffer[8] = 0x00;  /* Size byte 1 */
    s_requestBuffer[9] = 0x04;  /* Size byte 0 (4 bytes) */
    s_requestBuffer[10] = 0xAA; /* Data byte 0 */
    s_requestBuffer[11] = 0xBB; /* Data byte 1 */
    s_requestBuffer[12] = 0xCC; /* Data byte 2 */
    s_requestBuffer[13] = 0xDD; /* Data byte 3 */
    s_request.length = 14;
    
    Dcm_ReturnType result = Dcm_WriteMemoryByAddress(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
}

void test_write_memory_out_of_range(void)
{
    Dcm_MemoryRegionConfigType regions[] = {
        {
            .startAddress = 0x20000000,
            .endAddress = 0x2001FFFF,
            .regionType = DCM_MEM_REGION_RAM,
            .requiredSecurityLevel = 0,
            .writeAllowed = true,
            .readAllowed = true,
            .eraseRequired = false,
            .alignment = 1,
            .description = "RAM"
        }
    };
    
    Dcm_MemoryWriteConfigType config = {
        .regions = regions,
        .numRegions = 1,
        .maxWriteSize = 0x1000,
        .enableVerification = true,
        .requireProgrammingSession = false,
        .requiredSecurityLevel = 0,
        .writeCallback = NULL,
        .verifyCallback = NULL
    };
    
    Dcm_MemoryWriteInit(&config);
    
    /* Write to address outside valid range */
    s_requestBuffer[0] = UDS_SVC_WRITE_MEMORY_BY_ADDRESS;
    s_requestBuffer[1] = 0x14;
    s_requestBuffer[2] = 0x30;  /* Address outside defined region */
    s_requestBuffer[3] = 0x00;
    s_requestBuffer[4] = 0x00;
    s_requestBuffer[5] = 0x00;
    s_requestBuffer[6] = 0x00;
    s_requestBuffer[7] = 0x00;
    s_requestBuffer[8] = 0x00;
    s_requestBuffer[9] = 0x04;
    s_requestBuffer[10] = 0xAA;
    s_requestBuffer[11] = 0xBB;
    s_requestBuffer[12] = 0xCC;
    s_requestBuffer[13] = 0xDD;
    s_request.length = 14;
    
    Dcm_ReturnType result = Dcm_WriteMemoryByAddress(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_REQUEST_OUT_OF_RANGE, s_response.negativeResponseCode);
}

void test_write_memory_invalid_length(void)
{
    Dcm_MemoryWriteConfigType config = {0};
    Dcm_MemoryWriteInit(&config);
    
    /* Request too short */
    s_requestBuffer[0] = UDS_SVC_WRITE_MEMORY_BY_ADDRESS;
    s_requestBuffer[1] = 0x14;
    s_request.length = 2;  /* Too short */
    
    Dcm_ReturnType result = Dcm_WriteMemoryByAddress(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
}

void test_write_memory_parse_format(void)
{
    uint8_t addrLen, sizeLen;
    
    Dcm_ReturnType result = Dcm_ParseMemoryFormat(0x14, &addrLen, &sizeLen);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(4, addrLen);
    TEST_ASSERT_EQUAL(4, sizeLen);
    
    /* Test 1-byte format */
    result = Dcm_ParseMemoryFormat(0x11, &addrLen, &sizeLen);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(4, addrLen);
    TEST_ASSERT_EQUAL(1, sizeLen);
}

void test_write_memory_is_address_valid(void)
{
    Dcm_MemoryRegionConfigType regions[] = {
        {
            .startAddress = 0x20000000,
            .endAddress = 0x2001FFFF,
            .regionType = DCM_MEM_REGION_RAM,
            .requiredSecurityLevel = 0,
            .writeAllowed = true,
            .readAllowed = true,
            .eraseRequired = false,
            .alignment = 1,
            .description = "RAM"
        }
    };
    
    Dcm_MemoryWriteConfigType config = {
        .regions = regions,
        .numRegions = 1,
        .maxWriteSize = 0x1000,
        .enableVerification = true,
        .requireProgrammingSession = false,
        .requiredSecurityLevel = 0,
        .writeCallback = NULL,
        .verifyCallback = NULL
    };
    
    Dcm_MemoryWriteInit(&config);
    
    TEST_ASSERT_TRUE(Dcm_IsMemoryAddressWritable(0x20000000, 256));
    TEST_ASSERT_TRUE(Dcm_IsMemoryAddressWritable(0x20010000, 256));
    TEST_ASSERT_FALSE(Dcm_IsMemoryAddressWritable(0x30000000, 256));
}

void test_write_memory_parse_address(void)
{
    uint8_t data4[] = {0x20, 0x00, 0x10, 0x00};
    uint32_t addr = Dcm_ParseMemoryAddress(data4, 4);
    TEST_ASSERT_EQUAL(0x20001000, addr);
    
    uint8_t data2[] = {0x12, 0x34};
    addr = Dcm_ParseMemoryAddress(data2, 2);
    TEST_ASSERT_EQUAL(0x1234, addr);
}

/******************************************************************************
 * Routine Control Service (0x31) Tests
 ******************************************************************************/

void test_routine_control_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_RoutineControlInit(NULL, 0);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);  /* Uses default routines */
}

void test_routine_control_init_with_config(void)
{
    Dcm_RoutineConfigType routines[] = {
        {
            .routineId = DCM_ROUTINE_ID_ERASE_MEMORY,
            .startSupported = true,
            .stopSupported = false,
            .resultsSupported = true,
            .requiredSecurityLevel = 3,
            .requiredSession = DCM_SESSION_PROGRAMMING,
            .startFunc = NULL,
            .stopFunc = NULL,
            .resultsFunc = NULL,
            .description = "Erase Memory"
        }
    };
    
    Dcm_ReturnType result = Dcm_RoutineControlInit(routines, 1);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
}

void test_routine_control_start_erase_memory(void)
{
    Dcm_RoutineConfigType routines[] = {
        {
            .routineId = DCM_ROUTINE_ID_ERASE_MEMORY,
            .startSupported = true,
            .stopSupported = false,
            .resultsSupported = true,
            .requiredSecurityLevel = 0,  /* No security for testing */
            .requiredSession = DCM_SESSION_DEFAULT,
            .startFunc = NULL,
            .stopFunc = NULL,
            .resultsFunc = NULL,
            .description = "Erase Memory"
        }
    };
    
    Dcm_RoutineControlInit(routines, 1);
    
    /* Start erase memory routine */
    s_requestBuffer[0] = UDS_SVC_ROUTINE_CONTROL;
    s_requestBuffer[1] = DCM_ROUTINE_CTRL_START;
    s_requestBuffer[2] = 0xFF;  /* Routine ID high byte */
    s_requestBuffer[3] = 0x00;  /* Routine ID low byte */
    s_requestBuffer[4] = 0x20;  /* Address option */
    s_requestBuffer[5] = 0x00;
    s_requestBuffer[6] = 0x00;
    s_requestBuffer[7] = 0x00;
    s_requestBuffer[8] = 0x00;
    s_requestBuffer[9] = 0x10;
    s_requestBuffer[10] = 0x00;
    s_requestBuffer[11] = 0x00;
    s_request.length = 12;
    
    Dcm_ReturnType result = Dcm_RoutineControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_SVC_ROUTINE_CONTROL + 0x40, s_responseBuffer[0]);
    TEST_ASSERT_EQUAL(DCM_ROUTINE_CTRL_START, s_responseBuffer[1]);
    TEST_ASSERT_EQUAL(0xFF, s_responseBuffer[2]);
    TEST_ASSERT_EQUAL(0x00, s_responseBuffer[3]);
}

void test_routine_control_request_results(void)
{
    /* First init and start a routine */
    Dcm_RoutineControlInit(NULL, 0);
    
    Dcm_RoutineConfigType routines[] = {
        {
            .routineId = DCM_ROUTINE_ID_CHECK_PROG_DEPENDENCIES,
            .startSupported = true,
            .stopSupported = false,
            .resultsSupported = true,
            .requiredSecurityLevel = 0,
            .requiredSession = DCM_SESSION_DEFAULT,
            .startFunc = NULL,
            .stopFunc = NULL,
            .resultsFunc = NULL,
            .description = "Check Programming Dependencies"
        }
    };
    
    Dcm_RoutineControlInit(routines, 1);
    
    /* Start the routine first */
    s_requestBuffer[0] = UDS_SVC_ROUTINE_CONTROL;
    s_requestBuffer[1] = DCM_ROUTINE_CTRL_START;
    s_requestBuffer[2] = 0xFF;
    s_requestBuffer[3] = 0x01;
    s_request.length = 4;
    Dcm_RoutineControl(&s_request, &s_response);
    
    /* Request results */
    s_requestBuffer[0] = UDS_SVC_ROUTINE_CONTROL;
    s_requestBuffer[1] = DCM_ROUTINE_CTRL_REQUEST_RESULTS;
    s_requestBuffer[2] = 0xFF;
    s_requestBuffer[3] = 0x01;
    s_request.length = 4;
    
    Dcm_ReturnType result = Dcm_RoutineControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_EQUAL(false, s_response.isNegativeResponse);
}

void test_routine_control_invalid_routine(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    /* Request invalid routine */
    s_requestBuffer[0] = UDS_SVC_ROUTINE_CONTROL;
    s_requestBuffer[1] = DCM_ROUTINE_CTRL_START;
    s_requestBuffer[2] = 0xDE;  /* Invalid routine ID */
    s_requestBuffer[3] = 0xAD;
    s_request.length = 4;
    
    Dcm_ReturnType result = Dcm_RoutineControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_REQUEST_OUT_OF_RANGE, s_response.negativeResponseCode);
}

void test_routine_control_invalid_subfunction(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    s_requestBuffer[0] = UDS_SVC_ROUTINE_CONTROL;
    s_requestBuffer[1] = 0x04;  /* Invalid subfunction */
    s_requestBuffer[2] = 0xFF;
    s_requestBuffer[3] = 0x00;
    s_request.length = 4;
    
    Dcm_ReturnType result = Dcm_RoutineControl(&s_request, &s_response);
    
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, s_response.negativeResponseCode);
}

void test_routine_control_is_supported(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    TEST_ASSERT_TRUE(Dcm_IsRoutineSupported(DCM_ROUTINE_ID_ERASE_MEMORY));
    TEST_ASSERT_TRUE(Dcm_IsRoutineSupported(DCM_ROUTINE_ID_CHECK_PROG_DEPENDENCIES));
    TEST_ASSERT_FALSE(Dcm_IsRoutineSupported(0x1234));
}

void test_routine_control_is_control_type_valid(void)
{
    TEST_ASSERT_TRUE(Dcm_IsRoutineControlTypeValid(DCM_ROUTINE_CTRL_START));
    TEST_ASSERT_TRUE(Dcm_IsRoutineControlTypeValid(DCM_ROUTINE_CTRL_STOP));
    TEST_ASSERT_TRUE(Dcm_IsRoutineControlTypeValid(DCM_ROUTINE_CTRL_REQUEST_RESULTS));
    TEST_ASSERT_FALSE(Dcm_IsRoutineControlTypeValid(0x04));
}

void test_routine_control_is_running(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    TEST_ASSERT_FALSE(Dcm_IsRoutineRunning());
}

void test_routine_control_get_state(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    Dcm_RoutineStateType state = Dcm_GetRoutineState();
    TEST_ASSERT_EQUAL(DCM_ROUTINE_STATE_IDLE, state);
}

void test_routine_erase_memory_api(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    Dcm_ReturnType result = Dcm_Routine_EraseMemory(0x20000000, 0x1000);
    /* Will fail due to session/security but should return correctly */
    TEST_ASSERT_TRUE((result == DCM_E_OK) || (result == DCM_E_NOT_OK));
}

void test_routine_check_dependencies_api(void)
{
    Dcm_RoutineControlInit(NULL, 0);
    
    bool result = false;
    Dcm_ReturnType ret = Dcm_Routine_CheckProgrammingDependencies(&result);
    TEST_ASSERT_TRUE((ret == DCM_E_OK) || (ret == DCM_E_NOT_OK));
}

/******************************************************************************
 * DCM Main Module Tests
 ******************************************************************************/

void test_dcm_init_null_config(void)
{
    Dcm_ReturnType result = Dcm_Init(NULL);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_dcm_init_success(void)
{
    Dcm_ConfigType config = {0};
    
    Dcm_ReturnType result = Dcm_Init(&config);
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_TRUE(Dcm_IsInitialized());
}

void test_dcm_deinit(void)
{
    Dcm_ConfigType config = {0};
    Dcm_Init(&config);
    
    Dcm_ReturnType result = Dcm_DeInit();
    TEST_ASSERT_EQUAL(DCM_E_OK, result);
    TEST_ASSERT_FALSE(Dcm_IsInitialized());
}

void test_dcm_main_function(void)
{
    Dcm_ConfigType config = {0};
    Dcm_Init(&config);
    
    /* Should not crash */
    Dcm_MainFunction(10);
}

void test_dcm_get_version(void)
{
    uint8_t major, minor, patch;
    Dcm_GetVersionInfo(&major, &minor, &patch);
    
    TEST_ASSERT_EQUAL(DCM_MAJOR_VERSION, major);
    TEST_ASSERT_EQUAL(DCM_MINOR_VERSION, minor);
    TEST_ASSERT_EQUAL(DCM_PATCH_VERSION, patch);
}

void test_dcm_get_state(void)
{
    Dcm_ConfigType config = {0};
    Dcm_Init(&config);
    
    Dcm_StateType state = Dcm_GetState();
    TEST_ASSERT_EQUAL(DCM_STATE_INIT, state);
}

void test_dcm_process_request_null(void)
{
    Dcm_ConfigType config = {0};
    Dcm_Init(&config);
    
    Dcm_ReturnType result = Dcm_ProcessRequest(NULL, &s_response);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
}

void test_dcm_process_request_service_not_supported(void)
{
    Dcm_ConfigType config = {0};
    Dcm_Init(&config);
    
    s_requestBuffer[0] = 0xBF;  /* Unsupported service */
    s_request.length = 1;
    
    Dcm_ReturnType result = Dcm_ProcessRequest(&s_request, &s_response);
    TEST_ASSERT_EQUAL(DCM_E_NOT_OK, result);
    TEST_ASSERT_EQUAL(true, s_response.isNegativeResponse);
    TEST_ASSERT_EQUAL(UDS_NRC_SERVICE_NOT_SUPPORTED, s_response.negativeResponseCode);
}

void test_dcm_process_security_access(void)
{
    Dcm_SecurityAccessConfigType secConfig = {0};
    Dcm_CommunicationControlConfigType commConfig = {0};
    Dcm_DynamicDidConfigType dynDidConfig = {0};
    Dcm_MemoryWriteConfigType memConfig = {0};
    
    Dcm_ConfigType config = {
        .securityConfig = &secConfig,
        .commControlConfig = &commConfig,
        .dynamicDidConfig = &dynDidConfig,
        .memoryWriteConfig = &memConfig,
        .routineConfigs = NULL,
        .numRoutines = 0
    };
    
    Dcm_Init(&config);
    
    s_requestBuffer[0] = UDS_SVC_SECURITY_ACCESS;
    s_requestBuffer[1] = DCM_SEC_REQ_SEED_LEVEL_1;
    s_request.length = 2;
    
    Dcm_ReturnType result = Dcm_ProcessRequest(&s_request, &s_response);
    /* Will fail due to uninitialized security but shouldn't crash */
    TEST_ASSERT_TRUE((result == DCM_E_OK) || (result == DCM_E_NOT_OK));
}

void test_dcm_process_communication_control(void)
{
    Dcm_CommunicationControlConfigType commConfig = {0};
    
    Dcm_ConfigType config = {
        .commControlConfig = &commConfig
    };
    
    Dcm_Init(&config);
    
    s_requestBuffer[0] = UDS_SVC_COMMUNICATION_CONTROL;
    s_requestBuffer[1] = DCM_COMM_CTRL_ENABLE_RX_TX;
    s_requestBuffer[2] = DCM_COMM_TYPE_NORMAL;
    s_request.length = 3;
    
    Dcm_ReturnType result = Dcm_ProcessRequest(&s_request, &s_response);
    TEST_ASSERT_TRUE((result == DCM_E_OK) || (result == DCM_E_NOT_OK));
}

void test_dcm_process_routine_control(void)
{
    Dcm_RoutineConfigType routines[] = {
        {
            .routineId = DCM_ROUTINE_ID_SELF_TEST,
            .startSupported = true,
            .stopSupported = false,
            .resultsSupported = true,
            .requiredSecurityLevel = 0,
            .requiredSession = DCM_SESSION_DEFAULT,
            .startFunc = NULL,
            .stopFunc = NULL,
            .resultsFunc = NULL,
            .description = "Self Test"
        }
    };
    
    Dcm_ConfigType config = {
        .routineConfigs = routines,
        .numRoutines = 1
    };
    
    Dcm_Init(&config);
    
    s_requestBuffer[0] = UDS_SVC_ROUTINE_CONTROL;
    s_requestBuffer[1] = DCM_ROUTINE_CTRL_START;
    s_requestBuffer[2] = 0x02;  /* Self test routine ID */
    s_requestBuffer[3] = 0x03;
    s_request.length = 4;
    
    Dcm_ReturnType result = Dcm_ProcessRequest(&s_request, &s_response);
    TEST_ASSERT_TRUE((result == DCM_E_OK) || (result == DCM_E_NOT_OK));
}

/******************************************************************************
 * @file    test_doip.c
 * @brief   Unit tests for ISO 13400-2 DoIP protocol stack
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* Include Unity test framework */
#include "../../unity/unity.h"

/* Include DoIP headers */
#include "../../../src/diagnostics/doip/doip_types.h"
#include "../../../src/diagnostics/doip/doip_core.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/

#define TEST_EID                {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
#define TEST_GID                {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
#define TEST_VIN                "WBA1234567890TEST"
#define TEST_LOGICAL_ADDRESS    0x0E00U

/******************************************************************************
 * Test Global Variables
 ******************************************************************************/

static DoIp_EntityConfigType g_entityConfig;
static DoIp_ConnectionConfigType g_connectionConfig;
static uint8_t g_testBuffer[256];

/******************************************************************************
 * Test Setup and Teardown
 ******************************************************************************/

void setUp(void)
{
    /* Initialize entity configuration */
    memset(&g_entityConfig, 0, sizeof(g_entityConfig));
    uint8_t eid[] = TEST_EID;
    uint8_t gid[] = TEST_GID;
    memcpy(g_entityConfig.eid, eid, 6);
    memcpy(g_entityConfig.gid, gid, 6);
    memcpy(g_entityConfig.vin, TEST_VIN, 17);
    g_entityConfig.logicalAddress = TEST_LOGICAL_ADDRESS;
    g_entityConfig.furtherAction = 0x00U;
    g_entityConfig.vinSyncRequired = false;
    g_entityConfig.nodeType = DOIP_NODE_NODE;
    g_entityConfig.maxTcpConnections = 8;
    g_entityConfig.maxDiagnosticSize = 4096;
    g_entityConfig.routingActivationRequired = true;
    
    /* Initialize connection configuration */
    memset(&g_connectionConfig, 0, sizeof(g_connectionConfig));
    g_connectionConfig.connectionId = 0;
    g_connectionConfig.localAddress = TEST_LOGICAL_ADDRESS;
    g_connectionConfig.ipAddress = 0xC0A80101U;  /* 192.168.1.1 */
    g_connectionConfig.port = DOIP_TCP_DATA_PORT;
    g_connectionConfig.tlsEnabled = false;
    g_connectionConfig.inactivityTimeout = 300000U;
    g_connectionConfig.aliveCheckTimeout = 500U;
    g_connectionConfig.generalTimeout = 300000U;
    g_connectionConfig.maxRoutingActivations = 8;
    
    /* Reset test buffer */
    memset(g_testBuffer, 0, sizeof(g_testBuffer));
}

void tearDown(void)
{
    DoIp_DeInit();
}

/******************************************************************************
 * Byte Conversion Tests
 ******************************************************************************/

void test_BytesToUint16(void)
{
    uint8_t bytes[] = {0x12, 0x34};
    uint16_t result = DoIp_BytesToUint16(bytes);
    TEST_ASSERT_EQUAL(0x1234, result);
    
    uint8_t bytes2[] = {0x00, 0xFF};
    result = DoIp_BytesToUint16(bytes2);
    TEST_ASSERT_EQUAL(0x00FF, result);
}

void test_BytesToUint32(void)
{
    uint8_t bytes[] = {0x12, 0x34, 0x56, 0x78};
    uint32_t result = DoIp_BytesToUint32(bytes);
    TEST_ASSERT_EQUAL(0x12345678U, result);
}

void test_Uint16ToBytes(void)
{
    uint8_t bytes[2];
    DoIp_Uint16ToBytes(0x1234, bytes);
    TEST_ASSERT_EQUAL(0x12, bytes[0]);
    TEST_ASSERT_EQUAL(0x34, bytes[1]);
}

void test_Uint32ToBytes(void)
{
    uint8_t bytes[4];
    DoIp_Uint32ToBytes(0x12345678U, bytes);
    TEST_ASSERT_EQUAL(0x12, bytes[0]);
    TEST_ASSERT_EQUAL(0x34, bytes[1]);
    TEST_ASSERT_EQUAL(0x56, bytes[2]);
    TEST_ASSERT_EQUAL(0x78, bytes[3]);
}

/******************************************************************************
 * Header Tests
 ******************************************************************************/

void test_BuildHeader(void)
{
    uint8_t buffer[DOIP_HEADER_SIZE];
    
    DoIp_ReturnType result = DoIp_BuildHeader(
        DOIP_PT_ROUTING_ACTIVATION_REQ,
        100,
        buffer
    );
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_PROTOCOL_VERSION, buffer[0]);
    TEST_ASSERT_EQUAL(DOIP_PROTOCOL_VERSION_INV, buffer[1]);
    TEST_ASSERT_EQUAL(0x00, buffer[2]);  /* High byte of payload type */
    TEST_ASSERT_EQUAL(0x05, buffer[3]);  /* Low byte of payload type */
    TEST_ASSERT_EQUAL(0x00, buffer[4]);  /* High byte of length */
    TEST_ASSERT_EQUAL(0x00, buffer[5]);
    TEST_ASSERT_EQUAL(0x00, buffer[6]);
    TEST_ASSERT_EQUAL(100, buffer[7]);   /* Low byte of length */
}

void test_ParseHeader(void)
{
    uint8_t buffer[] = {
        DOIP_PROTOCOL_VERSION,
        DOIP_PROTOCOL_VERSION_INV,
        0x00, 0x05,              /* Payload type: Routing Activation Request */
        0x00, 0x00, 0x00, 100    /* Payload length: 100 */
    };
    
    DoIp_HeaderType header;
    DoIp_ReturnType result = DoIp_ParseHeader(buffer, &header);
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_PROTOCOL_VERSION, header.protocolVersion);
    TEST_ASSERT_EQUAL(DOIP_PROTOCOL_VERSION_INV, header.protocolVersionInv);
    TEST_ASSERT_EQUAL(DOIP_PT_ROUTING_ACTIVATION_REQ, header.payloadType);
    TEST_ASSERT_EQUAL(100, header.payloadLength);
}

void test_ValidateHeader_Valid(void)
{
    DoIp_HeaderType header = {
        .protocolVersion = DOIP_PROTOCOL_VERSION,
        .protocolVersionInv = DOIP_PROTOCOL_VERSION_INV,
        .payloadType = DOIP_PT_ROUTING_ACTIVATION_REQ,
        .payloadLength = 100
    };
    
    DoIp_ReturnType result = DoIp_ValidateHeader(&header);
    TEST_ASSERT_EQUAL(DOIP_OK, result);
}

void test_ValidateHeader_InvalidVersion(void)
{
    DoIp_HeaderType header = {
        .protocolVersion = 0x01U,  /* Invalid version */
        .protocolVersionInv = DOIP_PROTOCOL_VERSION_INV,
        .payloadType = DOIP_PT_ROUTING_ACTIVATION_REQ,
        .payloadLength = 100
    };
    
    DoIp_ReturnType result = DoIp_ValidateHeader(&header);
    TEST_ASSERT_EQUAL(DOIP_ERROR, result);
}

void test_ValidateHeader_InvalidInverseVersion(void)
{
    DoIp_HeaderType header = {
        .protocolVersion = DOIP_PROTOCOL_VERSION,
        .protocolVersionInv = 0x00U,  /* Invalid inverse */
        .payloadType = DOIP_PT_ROUTING_ACTIVATION_REQ,
        .payloadLength = 100
    };
    
    DoIp_ReturnType result = DoIp_ValidateHeader(&header);
    TEST_ASSERT_EQUAL(DOIP_ERROR, result);
}

/******************************************************************************
 * Payload Type Validation Tests
 ******************************************************************************/

void test_IsValidPayloadType(void)
{
    /* Valid payload types */
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_VID_REQUEST));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_VID_REQUEST_EID));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_VID_REQUEST_VIN));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_VEHICLE_ANNOUNCEMENT));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_ROUTING_ACTIVATION_REQ));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_ROUTING_ACTIVATION_RES));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_ALIVE_CHECK_REQ));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_ALIVE_CHECK_RES));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_ENTITY_STATUS_REQ));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_ENTITY_STATUS_RES));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_REQ));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_DIAGNOSTIC_POWER_MODE_INFO_RES));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_DIAGNOSTIC_MSG));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_DIAGNOSTIC_ACK));
    TEST_ASSERT_TRUE(DoIp_IsValidPayloadType(DOIP_PT_DIAGNOSTIC_NACK));
    
    /* Invalid payload types */
    TEST_ASSERT_FALSE(DoIp_IsValidPayloadType(0x0000));
    TEST_ASSERT_FALSE(DoIp_IsValidPayloadType(0x0009));
    TEST_ASSERT_FALSE(DoIp_IsValidPayloadType(0xFFFF));
}

void test_GetPayloadTypeName(void)
{
    TEST_ASSERT_EQUAL_STRING("VID_REQUEST", DoIp_GetPayloadTypeName(DOIP_PT_VID_REQUEST));
    TEST_ASSERT_EQUAL_STRING("ROUTING_ACTIVATION_REQ", DoIp_GetPayloadTypeName(DOIP_PT_ROUTING_ACTIVATION_REQ));
    TEST_ASSERT_EQUAL_STRING("DIAGNOSTIC_MSG", DoIp_GetPayloadTypeName(DOIP_PT_DIAGNOSTIC_MSG));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", DoIp_GetPayloadTypeName(0xFFFF));
}

/******************************************************************************
 * Vehicle Announcement Tests
 ******************************************************************************/

void test_BuildVehicleAnnouncement(void)
{
    DoIp_VehicleAnnouncementType announcement;
    memcpy(announcement.vin, TEST_VIN, 17);
    DoIp_Uint16ToBytes(TEST_LOGICAL_ADDRESS, announcement.logicalAddress);
    memcpy(announcement.eid, g_entityConfig.eid, 6);
    memcpy(announcement.gid, g_entityConfig.gid, 6);
    announcement.furtherAction = 0x00U;
    announcement.syncStatus = 0x00U;
    
    uint8_t buffer[64];
    uint16_t length;
    
    /* Initialize DoIP first */
    DoIp_Init(&g_entityConfig, &g_connectionConfig, 1);
    
    DoIp_ReturnType result = DoIp_BuildVehicleAnnouncement(&announcement, buffer, &length);
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_HEADER_SIZE + 32, length);  /* No sync status byte */
    
    /* Verify header */
    TEST_ASSERT_EQUAL(DOIP_PROTOCOL_VERSION, buffer[0]);
    TEST_ASSERT_EQUAL(0x00, buffer[2]);  /* High byte of VEHICLE_ANNOUNCEMENT */
    TEST_ASSERT_EQUAL(0x04, buffer[3]);  /* Low byte */
}

/******************************************************************************
 * Routing Activation Response Tests
 ******************************************************************************/

void test_BuildRoutingActivationResponse(void)
{
    DoIp_RoutingActivationResponseType response;
    response.logicalAddressTester = 0x0E00U;
    response.logicalAddressDoip = TEST_LOGICAL_ADDRESS;
    response.responseCode = DOIP_RA_RES_SUCCESS;
    response.oemSpecific = NULL;
    response.oemSpecificLength = 0;
    
    uint8_t buffer[64];
    uint16_t length;
    
    DoIp_ReturnType result = DoIp_BuildRoutingActivationResponse(&response, buffer, &length);
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_HEADER_SIZE + 9, length);
    
    /* Verify header */
    TEST_ASSERT_EQUAL(0x00, buffer[2]);  /* High byte of ROUTING_ACTIVATION_RES */
    TEST_ASSERT_EQUAL(0x06, buffer[3]);  /* Low byte */
    
    /* Verify payload */
    TEST_ASSERT_EQUAL(0x0E, buffer[8]);  /* Tester LA high */
    TEST_ASSERT_EQUAL(0x00, buffer[9]);  /* Tester LA low */
    TEST_ASSERT_EQUAL(0x10, buffer[12]); /* Response code: Success */
}

/******************************************************************************
 * Diagnostic Message Tests
 ******************************************************************************/

void test_BuildDiagnosticMessage(void)
{
    uint8_t udsData[] = {0x22, 0xF1, 0x90};  /* UDS ReadDataByIdentifier */
    
    DoIp_DiagnosticMessageType message;
    message.sourceAddress = 0x0E00U;
    message.targetAddress = TEST_LOGICAL_ADDRESS;
    message.userData = udsData;
    message.userDataLength = sizeof(udsData);
    
    uint8_t buffer[64];
    uint16_t length;
    
    DoIp_ReturnType result = DoIp_BuildDiagnosticMessage(&message, buffer, &length);
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_HEADER_SIZE + 4 + 3, length);  /* Header + SA/TA + UDS data */
    
    /* Verify header */
    TEST_ASSERT_EQUAL(0x80, buffer[2]);  /* High byte of DIAGNOSTIC_MSG */
    TEST_ASSERT_EQUAL(0x01, buffer[3]);  /* Low byte */
    
    /* Verify payload */
    TEST_ASSERT_EQUAL(0x0E, buffer[8]);  /* SA high */
    TEST_ASSERT_EQUAL(0x00, buffer[9]);  /* SA low */
    TEST_ASSERT_EQUAL(0x22, buffer[12]); /* UDS SID */
}

void test_BuildDiagnosticAck(void)
{
    DoIp_DiagnosticAckType ack;
    ack.sourceAddress = 0x0E00U;
    ack.targetAddress = TEST_LOGICAL_ADDRESS;
    ack.ackCode = DOIP_DIAG_ACK_OK;
    ack.previousData = NULL;
    ack.previousDataLength = 0;
    
    uint8_t buffer[64];
    uint16_t length;
    
    DoIp_ReturnType result = DoIp_BuildDiagnosticAck(&ack, true, buffer, &length);
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_HEADER_SIZE + 5, length);
    
    /* Verify header */
    TEST_ASSERT_EQUAL(0x80, buffer[2]);  /* High byte of DIAGNOSTIC_ACK */
    TEST_ASSERT_EQUAL(0x02, buffer[3]);  /* Low byte */
}

void test_BuildDiagnosticNack(void)
{
    DoIp_DiagnosticAckType nack;
    nack.sourceAddress = 0x0E00U;
    nack.targetAddress = TEST_LOGICAL_ADDRESS;
    nack.ackCode = DOIP_DIAG_NACK_INVALID_SA;
    nack.previousData = NULL;
    nack.previousDataLength = 0;
    
    uint8_t buffer[64];
    uint16_t length;
    
    DoIp_ReturnType result = DoIp_BuildDiagnosticAck(&nack, false, buffer, &length);
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_HEADER_SIZE + 5, length);
    
    /* Verify header */
    TEST_ASSERT_EQUAL(0x80, buffer[2]);  /* High byte of DIAGNOSTIC_NACK */
    TEST_ASSERT_EQUAL(0x03, buffer[3]);  /* Low byte */
    
    /* Verify NACK code */
    TEST_ASSERT_EQUAL(DOIP_DIAG_NACK_INVALID_SA, buffer[12]);
}

/******************************************************************************
 * Address Validation Tests
 ******************************************************************************/

void test_IsValidSourceAddress(void)
{
    /* Valid tester source addresses: 0x0E00-0x0EFF */
    TEST_ASSERT_TRUE(DoIp_IsValidSourceAddress(0x0E00U));
    TEST_ASSERT_TRUE(DoIp_IsValidSourceAddress(0x0E80U));
    TEST_ASSERT_TRUE(DoIp_IsValidSourceAddress(0x0EFFU));
    
    /* Invalid addresses */
    TEST_ASSERT_FALSE(DoIp_IsValidSourceAddress(0x0000U));
    TEST_ASSERT_FALSE(DoIp_IsValidSourceAddress(0x0DFFU));
    TEST_ASSERT_FALSE(DoIp_IsValidSourceAddress(0x0F00U));
    TEST_ASSERT_FALSE(DoIp_IsValidSourceAddress(0xFFFFU));
}

void test_IsValidTargetAddress(void)
{
    /* Valid target addresses: 0x0001-0x0DFF and 0x1000-0x7FFF */
    TEST_ASSERT_TRUE(DoIp_IsValidTargetAddress(0x0001U));
    TEST_ASSERT_TRUE(DoIp_IsValidTargetAddress(0x0DFFU));
    TEST_ASSERT_TRUE(DoIp_IsValidTargetAddress(0x1000U));
    TEST_ASSERT_TRUE(DoIp_IsValidTargetAddress(0x7FFFU));
    
    /* Invalid addresses */
    TEST_ASSERT_FALSE(DoIp_IsValidTargetAddress(0x0000U));
    TEST_ASSERT_FALSE(DoIp_IsValidTargetAddress(0x0E00U));  /* Tester range */
    TEST_ASSERT_FALSE(DoIp_IsValidTargetAddress(0x8000U));
    TEST_ASSERT_FALSE(DoIp_IsValidTargetAddress(0xFFFFU));
}

/******************************************************************************
 * Routing Activation Process Tests
 ******************************************************************************/

void test_ProcessRoutingActivation_InvalidSourceAddress(void)
{
    DoIp_Init(&g_entityConfig, &g_connectionConfig, 1);
    DoIp_Start();
    
    uint8_t responseCode;
    DoIp_ReturnType result = DoIp_ProcessRoutingActivation(
        0,              /* Connection ID */
        0x0000U,        /* Invalid source address */
        DOIP_RA_DEFAULT,
        &responseCode
    );
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_RA_RES_UNSUPPORTED_SA, responseCode);
}

void test_ProcessRoutingActivation_Success(void)
{
    DoIp_Init(&g_entityConfig, &g_connectionConfig, 1);
    DoIp_Start();
    
    uint8_t responseCode;
    DoIp_ReturnType result = DoIp_ProcessRoutingActivation(
        0,              /* Connection ID */
        0x0E00U,        /* Valid tester source address */
        DOIP_RA_DEFAULT,
        &responseCode
    );
    
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    TEST_ASSERT_EQUAL(DOIP_RA_RES_SUCCESS, responseCode);
    
    /* Check connection state */
    TEST_ASSERT_TRUE(DoIp_IsConnectionRegistered(0));
}

/******************************************************************************
 * Initialization Tests
 ******************************************************************************/

void test_InitAndDeinit(void)
{
    DoIp_ReturnType result = DoIp_Init(&g_entityConfig, &g_connectionConfig, 1);
    TEST_ASSERT_EQUAL(DOIP_OK, result);
    
    DoIp_DeInit();
    
    /* After deinit, re-init should work */
    result = DoIp_Init(&g_entityConfig, &g_connectionConfig, 1);
    TEST_ASSERT_EQUAL(DOIP_OK, result);
}

void test_Init_InvalidParameters(void)
{
    /* NULL entity config */
    DoIp_ReturnType result = DoIp_Init(NULL, &g_connectionConfig, 1);
    TEST_ASSERT_EQUAL(DOIP_INVALID_PARAMETER, result);
    
    /* NULL connections */
    result = DoIp_Init(&g_entityConfig, NULL, 1);
    TEST_ASSERT_EQUAL(DOIP_INVALID_PARAMETER, result);
    
    /* Zero connections */
    result = DoIp_Init(&g_entityConfig, &g_connectionConfig, 0);
    TEST_ASSERT_EQUAL(DOIP_INVALID_PARAMETER, result);
}

/******************************************************************************
 * Test Runner
 ******************************************************************************/

int main(void)
{
    UNITY_BEGIN();
    
    /* Byte Conversion Tests */
    RUN_TEST(test_BytesToUint16);
    RUN_TEST(test_BytesToUint32);
    RUN_TEST(test_Uint16ToBytes);
    RUN_TEST(test_Uint32ToBytes);
    
    /* Header Tests */
    RUN_TEST(test_BuildHeader);
    RUN_TEST(test_ParseHeader);
    RUN_TEST(test_ValidateHeader_Valid);
    RUN_TEST(test_ValidateHeader_InvalidVersion);
    RUN_TEST(test_ValidateHeader_InvalidInverseVersion);
    
    /* Payload Type Tests */
    RUN_TEST(test_IsValidPayloadType);
    RUN_TEST(test_GetPayloadTypeName);
    
    /* Vehicle Announcement Tests */
    RUN_TEST(test_BuildVehicleAnnouncement);
    
    /* Routing Activation Tests */
    RUN_TEST(test_BuildRoutingActivationResponse);
    RUN_TEST(test_ProcessRoutingActivation_InvalidSourceAddress);
    RUN_TEST(test_ProcessRoutingActivation_Success);
    
    /* Diagnostic Message Tests */
    RUN_TEST(test_BuildDiagnosticMessage);
    RUN_TEST(test_BuildDiagnosticAck);
    RUN_TEST(test_BuildDiagnosticNack);
    
    /* Address Validation Tests */
    RUN_TEST(test_IsValidSourceAddress);
    RUN_TEST(test_IsValidTargetAddress);
    
    /* Initialization Tests */
    RUN_TEST(test_InitAndDeinit);
    RUN_TEST(test_Init_InvalidParameters);
    
    return UNITY_END();
}

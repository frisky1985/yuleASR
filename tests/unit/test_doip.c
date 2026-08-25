/*
 * @file test_doip.c
 * @brief DoIP (Diagnostic over IP) 单元测试
 * 
 * 测试范围:
 * - Generic Header解析
 * - Vehicle Identification
 * - Routing Activation
 * - Diagnostic Message处理
 * - Alive Check
 * - DoIP 时间参数
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: B
 */

// @tests src/bsw/services/doip/src/DoIP.c  @tests src/bsw/services/doip/include/DoIP.h

#include <unity.h>
#include "DoIP.h"
#include "SoAd.h"
#include "PduR.h"
#include <string.h>

/* ================================ 测试数据 ================================ */

#define TEST_PAYLOAD_MAX  4096

static uint8 txBuffer[TEST_PAYLOAD_MAX];
static uint8 rxBuffer[TEST_PAYLOAD_MAX];
static uint16 txLength;
static uint16 rxLength;
static boolean connectionEstablished;
static boolean routingActivated;

void setUp(void) {
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
    txLength = 0;
    rxLength = 0;
    connectionEstablished = FALSE;
    routingActivated = FALSE;
    
    DoIP_Init(NULL_PTR);
}

void tearDown(void) {
    DoIP_DeInit();
}

/* ================================ Generic Header 测试 ================================ */

/**
 * @brief 测试Generic Header解析
 * @test DOIP_HEADER_001
 */
void test_DoIP_GenericHeader_Parse(void) {
    uint8 header[8];
    
    /* 构建有效的Generic Header */
    header[0] = 0x02;  /* Protocol version */
    header[1] = 0xFD;  /* Inverse protocol version */
    header[2] = 0x80;  /* Payload type: Vehicle Identification Request */
    header[3] = 0x01;
    header[4] = 0x00;  /* Payload length: 0 */
    header[5] = 0x00;
    header[6] = 0x00;
    header[7] = 0x00;
    
    /* 解析Header */
    DoIP_GenericHeaderType parsedHeader;
    Std_ReturnType result = DoIP_ParseGenericHeader(header, &parsedHeader);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x02, parsedHeader.ProtocolVersion);
    TEST_ASSERT_EQUAL(0xFD, parsedHeader.InverseProtocolVersion);
    TEST_ASSERT_EQUAL(0x0001, parsedHeader.PayloadType);
    TEST_ASSERT_EQUAL(0, parsedHeader.PayloadLength);
}

/**
 * @brief 测试无效协议版本
 * @test DOIP_HEADER_002
 */
void test_DoIP_Invalid_Protocol_Version(void) {
    uint8 header[8];
    
    /* 无效的协议版本 */
    header[0] = 0x03;  /* Invalid version */
    header[1] = 0xFC;
    header[2] = 0x80;
    header[3] = 0x01;
    header[4] = 0x00;
    header[5] = 0x00;
    header[6] = 0x00;
    header[7] = 0x00;
    
    DoIP_GenericHeaderType parsedHeader;
    Std_ReturnType result = DoIP_ParseGenericHeader(header, &parsedHeader);
    
    TEST_ASSERT_NOT_EQUAL(E_OK, result);
}

/**
 * @brief 测试逆向版本不匹配
 * @test DOIP_HEADER_003
 */
void test_DoIP_Inverse_Version_Mismatch(void) {
    uint8 header[8];
    
    /* 版本和逆向版本不匹配 */
    header[0] = 0x02;  /* Version */
    header[1] = 0xFE;  /* Wrong inverse (should be 0xFD) */
    header[2] = 0x80;
    header[3] = 0x01;
    header[4] = 0x00;
    header[5] = 0x00;
    header[6] = 0x00;
    header[7] = 0x00;
    
    DoIP_GenericHeaderType parsedHeader;
    Std_ReturnType result = DoIP_ParseGenericHeader(header, &parsedHeader);
    
    TEST_ASSERT_NOT_EQUAL(E_OK, result);
}

/* ================================ Vehicle Identification 测试 ================================ */

/**
 * @brief 测试Vehicle Identification Request
 * @test DOIP_VIN_001
 */
void test_DoIP_VehicleIdentification_Request(void) {
    /* 发送Vehicle Identification Request */
    Std_ReturnType result = DoIP_TcpTransmit(0x0001, NULL_PTR, 0);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试Vehicle Announcement响应
 * @test DOIP_VIN_002
 */
void test_DoIP_VehicleAnnouncement_Response(void) {
    uint8 announcement[33];
    
    /* 模拟收到Vehicle Announcement */
    memset(announcement, 0, sizeof(announcement));
    announcement[0] = 0x00;  /* VIN */
    announcement[17] = 0x00; /* Logical Address */
    announcement[19] = 0x00; /* EID */
    announcement[25] = 0x00; /* GID */
    announcement[31] = 0x00; /* Further Action */
    announcement[32] = 0x00; /* Sync Status */
    
    SoAd_RxIndication(0, announcement, sizeof(announcement));
    
    /* 验证Vehicle被识别 */
    TEST_ASSERT_TRUE(DoIP_IsVehicleIdentified());
}

/* ================================ Routing Activation 测试 ================================ */

/**
 * @brief 测试Routing Activation Request
 * @test DOIP_ROUTING_001
 */
void test_DoIP_RoutingActivation_Request(void) {
    uint8 request[7];
    
    /* 构建Routing Activation Request */
    request[0] = 0x0E;  /* Source Address high */
    request[1] = 0x00;  /* Source Address low */
    request[2] = 0x00;  /* Activation type */
    request[3] = 0x00;  /* Reserved */
    request[4] = 0x00;
    request[5] = 0x00;
    request[6] = 0x00;
    
    Std_ReturnType result = DoIP_RoutingActivationRequest(0x0E00, 0x00);
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试Routing Activation成功响应
 * @test DOIP_ROUTING_002
 */
void test_DoIP_RoutingActivation_Success(void) {
    uint8 response[13];
    
    /* 模拟成功的Routing Activation Response */
    memset(response, 0, sizeof(response));
    response[0] = 0x0E;  /* Logical Address Tester */
    response[1] = 0x00;
    response[2] = 0x00;  /* Logical Address DoIP Entity */
    response[3] = 0x00;
    response[4] = 0x10;  /* Response code: Success */
    response[5] = 0x00;  /* Reserved */
    response[6] = 0x00;
    response[7] = 0x00;
    response[8] = 0x00;
    response[9] = 0x00;
    
    SoAd_RxIndication(0, response, sizeof(response));
    
    /* 验证Routing已激活 */
    TEST_ASSERT_TRUE(DoIP_IsRoutingActivated());
}

/**
 * @brief 测试Routing Activation拒绝
 * @test DOIP_ROUTING_003
 */
void test_DoIP_RoutingActivation_Denied(void) {
    uint8 response[13];
    
    /* 模拟拒绝的Routing Activation Response */
    response[4] = 0x02;  /* Response code: Denied */
    
    SoAd_RxIndication(0, response, sizeof(response));
    
    /* 验证Routing未激活 */
    TEST_ASSERT_FALSE(DoIP_IsRoutingActivated());
}

/* ================================ Diagnostic Message 测试 ================================ */

/**
 * @brief 测试Diagnostic Message发送
 * @test DOIP_DIAG_001
 */
void test_DoIP_DiagnosticMessage_Transmit(void) {
    uint8 diagData[] = {0x10, 0x01};  /* Diagnostic request */
    
    Std_ReturnType result = DoIP_DiagnosticMessage_Transmit(0x0E00, 0xE000, diagData, sizeof(diagData));
    
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief 测试Diagnostic Message接收
 * @test DOIP_DIAG_002
 */
void test_DoIP_DiagnosticMessage_Receive(void) {
    uint8 diagMessage[256];
    
    /* 构建Diagnostic Message */
    diagMessage[0] = 0x00;  /* Source Address */
    diagMessage[1] = 0xE0;
    diagMessage[2] = 0x00;  /* Target Address */
    diagMessage[3] = 0x00;
    diagMessage[4] = 0x50;  /* Positive response */
    diagMessage[5] = 0x01;
    
    SoAd_RxIndication(0, diagMessage, 6);
    
    /* 验证数据已传递到DCM */
    TEST_ASSERT_TRUE(DoIP_IsDiagnosticMessageReceived());
}

/* ================================ 时间参敠测试 ================================ */

/**
 * @brief 测试Initial Vehicle Announcement时间参数
 * @test DOIP_TIMING_001
 */
void test_DoIP_InitialAnnouncement_Timing(void) {
    /* 验证A_DoIP_Initial_Vehicle_Announcement_Time = 500ms */
    TEST_ASSERT_EQUAL(500, DoIP_GetInitialAnnouncementTime());
    
    /* 验证A_DoIP_Vehicle_Announcement_Interval = 500ms */
    TEST_ASSERT_EQUAL(500, DoIP_GetAnnouncementInterval());
}

/**
 * @brief 测试Vehicle Announcement次数
 * @test DOIP_TIMING_002
 */
void test_DoIP_Announcement_Count(void) {
    /* 验证A_DoIP_Vehicle_Announcement_Count = 3 */
    TEST_ASSERT_EQUAL(3, DoIP_GetAnnouncementCount());
}

/**
 * @brief 测试Alive Check Response超时
 * @test DOIP_TIMING_003
 */
void test_DoIP_AliveCheck_Timeout(void) {
    /* 验证A_DoIP_Alive_Check_Response_Timeout = 500ms */
    TEST_ASSERT_EQUAL(500, DoIP_GetAliveCheckTimeout());
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* Generic Header测试 */
    RUN_TEST(test_DoIP_GenericHeader_Parse);
    RUN_TEST(test_DoIP_Invalid_Protocol_Version);
    RUN_TEST(test_DoIP_Inverse_Version_Mismatch);
    
    /* Vehicle Identification测试 */
    RUN_TEST(test_DoIP_VehicleIdentification_Request);
    RUN_TEST(test_DoIP_VehicleAnnouncement_Response);
    
    /* Routing Activation测试 */
    RUN_TEST(test_DoIP_RoutingActivation_Request);
    RUN_TEST(test_DoIP_RoutingActivation_Success);
    RUN_TEST(test_DoIP_RoutingActivation_Denied);
    
    /* Diagnostic Message测试 */
    RUN_TEST(test_DoIP_DiagnosticMessage_Transmit);
    RUN_TEST(test_DoIP_DiagnosticMessage_Receive);
    
    /* 时间参数测试 */
    RUN_TEST(test_DoIP_InitialAnnouncement_Timing);
    RUN_TEST(test_DoIP_Announcement_Count);
    RUN_TEST(test_DoIP_AliveCheck_Timeout);
    
    return UNITY_END();
}

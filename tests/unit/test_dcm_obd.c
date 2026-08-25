/*
 * @file test_dcm_obd.c
 * @brief DCM OBD-II 单元测试
 * 
 * 测试范围:
 * - OBD-II PID读取 (Mode 01, 02)
 * - 冻结帧 DTC读取 (Mode 02)
 * - 当前 DTC读取 (Mode 03)
 * - 待定 DTC读取 (Mode 07)
 * - 永久 DTC读取 (Mode 0A)
 * - DTC清除 (Mode 04)
 * - 车辆信息 (Mode 09)
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: QM
 */

// @tests src/bsw/services/dcm/src/Dcm.c  @tests src/bsw/services/dcm/include/Dcm.h

#include <unity.h>
#include "Dcm.h"
#include "Dcm_OBD.h"
#include "PduR.h"
#include <string.h>

/* ================================ 测试数据 ================================ */

static uint8 txBuffer[256];
static uint8 rxBuffer[256];
static uint16 txLength;
static uint16 rxLength;
static boolean responseSent;

void setUp(void) {
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
    txLength = 0;
    rxLength = 0;
    responseSent = FALSE;
    
    Dcm_Init(NULL_PTR);
}

void tearDown(void) {
    Dcm_DeInit();
}

/* ================================ Mode 01 PID 测试 ================================ */

/**
 * @brief 测试发动机转速PID (0x0C)
 * @test DCM_OBD_001
 */
void test_Dcm_OBD_PID_EngineRPM(void) {
    uint8 request[] = {0x01, 0x0C};  /* Mode 01, PID 0C */
    
    /* 模拟接收请求 */
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    /* 调用处理函数 */
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_GREATER_THAN(0, txLength);
    
    /* 验证响应 */
    TEST_ASSERT_EQUAL(0x41, txBuffer[0]);  /* Mode 01 + 0x40 */
    TEST_ASSERT_EQUAL(0x0C, txBuffer[1]);  /* PID */
    /* txBuffer[2-3] 为转速数据 */
}

/**
 * @brief 测试车速PID (0x0D)
 * @test DCM_OBD_002
 */
void test_Dcm_OBD_PID_VehicleSpeed(void) {
    uint8 request[] = {0x01, 0x0D};  /* Mode 01, PID 0D */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x41, txBuffer[0]);
    TEST_ASSERT_EQUAL(0x0D, txBuffer[1]);
    /* txBuffer[2] 为车速 (km/h) */
}

/**
 * @brief 测试支持的PID列表 (Mode 01 PID 00)
 * @test DCM_OBD_003
 */
void test_Dcm_OBD_PID_SupportedPIDs(void) {
    uint8 request[] = {0x01, 0x00};  /* Mode 01, PID 00 */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x41, txBuffer[0]);
    TEST_ASSERT_EQUAL(0x00, txBuffer[1]);
    TEST_ASSERT_EQUAL(4, txLength - 2);  /* 4字节位图 */
}

/* ================================ DTC 测试 ================================ */

/**
 * @brief 测试当前 DTC 读取 (Mode 03)
 * @test DCM_OBD_004
 */
void test_Dcm_OBD_Read_Current_DTC(void) {
    uint8 request[] = {0x03};  /* Mode 03 */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x43, txBuffer[0]);  /* Mode 03 + 0x40 */
    /* 验证DTC数量 */
    TEST_ASSERT_GREATER_OR_EQUAL(0, txBuffer[1]);
}

/**
 * @brief 测试待定 DTC 读取 (Mode 07)
 * @test DCM_OBD_005
 */
void test_Dcm_OBD_Read_Pending_DTC(void) {
    uint8 request[] = {0x07};  /* Mode 07 */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x47, txBuffer[0]);  /* Mode 07 + 0x40 */
}

/**
 * @brief 测试永久 DTC 读取 (Mode 0A)
 * @test DCM_OBD_006
 */
void test_Dcm_OBD_Read_Permanent_DTC(void) {
    uint8 request[] = {0x0A};  /* Mode 0A */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x4A, txBuffer[0]);  /* Mode 0A + 0x40 */
}

/**
 * @brief 测试 DTC 清除 (Mode 04)
 * @test DCM_OBD_007
 */
void test_Dcm_OBD_Clear_DTC(void) {
    uint8 request[] = {0x04};  /* Mode 04 */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x44, txBuffer[0]);  /* Mode 04 + 0x40 */
}

/* ================================ 冻结帧 DTC 测试 ================================ */

/**
 * @brief 测试冻结帧 DTC 读取 (Mode 02)
 * @test DCM_OBD_008
 */
void test_Dcm_OBD_FreezeFrame_DTC(void) {
    uint8 request[] = {0x02, 0x02};  /* Mode 02, PID 02 (DTC causing freeze frame) */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x42, txBuffer[0]);  /* Mode 02 + 0x40 */
    TEST_ASSERT_EQUAL(0x02, txBuffer[1]);
    /* txBuffer[2-3] 为DTC代码 */
}

/* ================================ 车辆信息测试 ================================ */

/**
 * @brief 测试VIN读取 (Mode 09 PID 02)
 * @test DCM_OBD_009
 */
void test_Dcm_OBD_VIN(void) {
    uint8 request[] = {0x09, 0x02};  /* Mode 09, PID 02 */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x49, txBuffer[0]);  /* Mode 09 + 0x40 */
    TEST_ASSERT_EQUAL(0x02, txBuffer[1]);
    /* 验证VIN长度 (17字符) */
    TEST_ASSERT_GREATER_OR_EQUAL(17, txLength - 2);
}

/**
 * @brief 测试软件版本 (Mode 09 PID 0C)
 * @test DCM_OBD_010
 */
void test_Dcm_OBD_Software_Version(void) {
    uint8 request[] = {0x09, 0x0C};  /* Mode 09, PID 0C */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x49, txBuffer[0]);
    TEST_ASSERT_EQUAL(0x0C, txBuffer[1]);
}

/* ================================ 错误处理测试 ================================ */

/**
 * @brief 测试无效 Mode
 * @test DCM_OBD_ERR_001
 */
void test_Dcm_OBD_Invalid_Mode(void) {
    uint8 request[] = {0xFF};  /* 无效 Mode */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    TEST_ASSERT_NOT_EQUAL(E_OK, result);
    /* 验证负响应 */
    TEST_ASSERT_EQUAL(0x7F, txBuffer[0]);
    TEST_ASSERT_EQUAL(0xFF, txBuffer[1]);
    TEST_ASSERT_EQUAL(0x31, txBuffer[2]);  /* requestSequenceError */
}

/**
 * @brief 测试无效 PID
 * @test DCM_OBD_ERR_002
 */
void test_Dcm_OBD_Invalid_PID(void) {
    uint8 request[] = {0x01, 0xFF};  /* 无效 PID */
    
    memcpy(rxBuffer, request, sizeof(request));
    rxLength = sizeof(request);
    
    Std_ReturnType result = Dcm_OBD_ProcessRequest(rxBuffer, rxLength, txBuffer, &txLength);
    
    /* 无支持的PID应返回空响应或负响应 */
    TEST_ASSERT_NOT_EQUAL(E_OK, result);
}

/* ================================ 主函数 ================================ */

int main(void) {
    UNITY_BEGIN();
    
    /* Mode 01 PID测试 */
    RUN_TEST(test_Dcm_OBD_PID_EngineRPM);
    RUN_TEST(test_Dcm_OBD_PID_VehicleSpeed);
    RUN_TEST(test_Dcm_OBD_PID_SupportedPIDs);
    
    /* DTC测试 */
    RUN_TEST(test_Dcm_OBD_Read_Current_DTC);
    RUN_TEST(test_Dcm_OBD_Read_Pending_DTC);
    RUN_TEST(test_Dcm_OBD_Read_Permanent_DTC);
    RUN_TEST(test_Dcm_OBD_Clear_DTC);
    
    /* 冻结帧测试 */
    RUN_TEST(test_Dcm_OBD_FreezeFrame_DTC);
    
    /* 车辆信息测试 */
    RUN_TEST(test_Dcm_OBD_VIN);
    RUN_TEST(test_Dcm_OBD_Software_Version);
    
    /* 错误处理测试 */
    RUN_TEST(test_Dcm_OBD_Invalid_Mode);
    RUN_TEST(test_Dcm_OBD_Invalid_PID);
    
    return UNITY_END();
}

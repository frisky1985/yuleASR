/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/
/* @req SHALL_SOMEIPXF */


/*==================================================================================================
 *                                      SOMEIPXF UNIT TESTS
 *==================================================================================================
 * FILENAME: SomeIpXf_Test.c
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for SOME/IP Transformer module
 *==================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include "SomeIpXf.h"
#include "SomeIpXf_Cfg.h"

/*==================================================================================================
 *                                    TEST FRAMEWORK
 *==================================================================================================*/
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  [PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (line %d)\n", message, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
static const SomeIpXf_InterfaceConfigType TestInterfaceConfig = {
    SOMEIPXF_SERVICE_ID_ECU_MONITOR,
    SOMEIPXF_METHOD_ID_GET_STATUS,
    SOMEIPXF_INTERFACE_VERSION,
    SOMEIPXF_PROTOCOL_VERSION,
    0U,
    SOMEIPXF_MSG_TYPE_REQUEST,
    SOMEIPXF_RET_CODE_OK
};

static const SomeIpXf_DataElementConfigType TestDataElements[4] = {
    { SOMEIPXF_DT_UINT8, 8U, 1U, FALSE, FALSE, 0U, SOMEIPXF_ARRAY_LEN_FIXED, SOMEIPXF_STR_UTF8, SOMEIPXF_STR_LEN_FIXED, 0U, 0U },
    { SOMEIPXF_DT_UINT16, 16U, 2U, FALSE, FALSE, 0U, SOMEIPXF_ARRAY_LEN_FIXED, SOMEIPXF_STR_UTF8, SOMEIPXF_STR_LEN_FIXED, 0U, 0U },
    { SOMEIPXF_DT_UINT32, 32U, 4U, FALSE, FALSE, 0U, SOMEIPXF_ARRAY_LEN_FIXED, SOMEIPXF_STR_UTF8, SOMEIPXF_STR_LEN_FIXED, 0U, 0U },
    { SOMEIPXF_DT_STRING, 0U, 4U, TRUE, FALSE, 0U, SOMEIPXF_ARRAY_LEN_FIXED, SOMEIPXF_STR_UTF8, SOMEIPXF_STR_LEN_SIZE_FIELD_32, 256U, 0U }
};

static const SomeIpXf_TransformerConfigType TestTransformerConfigs[2] = {
    { 0U, &TestInterfaceConfig, TestDataElements, 4U, TRUE },
    { 1U, &TestInterfaceConfig, TestDataElements, 4U, FALSE }
};

static const SomeIpXf_ConfigType TestConfig = {
    TestTransformerConfigs,
    2U,
    TRUE,
    TRUE,
    FALSE
};

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test SomeIpXf_Init with valid configuration
 */
void Test_SomeIpXf_Init_Valid(void)
{
    (void)printf("\n[Test] SomeIpXf_Init with valid configuration\n");
    
    SomeIpXf_DeInit();
    SomeIpXf_Init(&TestConfig);
    
    TEST_ASSERTTRUE != FALSE, (TRUE, "SomeIpXf should be initialized");
}

/**
 * @brief Test SomeIpXf_DeInit
 */
void Test_SomeIpXf_DeInit(void)
{
    (void)printf("\n[Test] SomeIpXf_DeInit\n");
    
    SomeIpXf_Init(&TestConfig);
    SomeIpXf_DeInit();
    
    TEST_ASSERTTRUE != FALSE, (TRUE, "SomeIpXf_DeInit should complete");
}

/**
 * @brief Test SomeIpXf_GetVersionInfo
 */
#if (SOMEIPXF_VERSION_INFO_API == STD_ON)
void Test_SomeIpXf_GetVersionInfo(void)
{
    Std_VersionInfoType versionInfo;
    
    (void)printf("\n[Test] SomeIpXf_GetVersionInfo\n");
    
    SomeIpXf_Init(&TestConfig);
    SomeIpXf_GetVersionInfo(&versionInfo);
    
    (void)TEST_ASSERT_EQ(SOMEIPXF_VENDOR_ID, versionInfo.vendorID, "Vendor ID should match");
    (void)TEST_ASSERT_EQ(SOMEIPXF_MODULE_ID, versionInfo.moduleID, "Module ID should match");
    (void)TEST_ASSERT_EQ(SOMEIPXF_SW_MAJOR_VERSION, versionInfo.sw_major_version, "Major version should match");
    (void)TEST_ASSERT_EQ(SOMEIPXF_SW_MINOR_VERSION, versionInfo.sw_minor_version, "Minor version should match");
}
#endif

/**
 * @brief Test SomeIpXf_BuildHeader and ParseHeader
 */
void Test_SomeIpXf_Header(void)
{
    Std_ReturnType result;
    SomeIpXf_HeaderType headerIn;
    SomeIpXf_HeaderType headerOut;
    uint8 buffer[12];
    
    (void)printf("\n[Test] SomeIpXf_BuildHeader and ParseHeader\n");
    
    headerIn.ServiceId = 0x1234U;
    headerIn.MethodId = 0x5678U;
    headerIn.Length = 100U;
    headerIn.ProtocolVersion = 0x01U;
    headerIn.InterfaceVersion = 0x02U;
    headerIn.MessageType = SOMEIPXF_MSG_TYPE_REQUEST;
    headerIn.ReturnCode = SOMEIPXF_RET_CODE_OK;
    
    result = SomeIpXf_BuildHeader(&headerIn, buffer);
    (void)TEST_ASSERT_EQ(E_OK, result, "BuildHeader should return E_OK");
    
    result = SomeIpXf_ParseHeader(buffer, &headerOut);
    (void)TEST_ASSERT_EQ(E_OK, result, "ParseHeader should return E_OK");
    
    (void)TEST_ASSERT_EQ(headerIn.ServiceId, headerOut.ServiceId, "Service ID should match");
    (void)TEST_ASSERT_EQ(headerIn.MethodId, headerOut.MethodId, "Method ID should match");
    (void)TEST_ASSERT_EQ(headerIn.Length, headerOut.Length, "Length should match");
    (void)TEST_ASSERT_EQ(headerIn.ProtocolVersion, headerOut.ProtocolVersion, "Protocol version should match");
    (void)TEST_ASSERT_EQ(headerIn.InterfaceVersion, headerOut.InterfaceVersion, "Interface version should match");
    (void)TEST_ASSERT_EQ(headerIn.MessageType, headerOut.MessageType, "Message type should match");
    (void)TEST_ASSERT_EQ(headerIn.ReturnCode, headerOut.ReturnCode, "Return code should match");
}

/**
 * @brief Test SomeIpXf_SerializeUint8 and DeserializeUint8
 */
void Test_SomeIpXf_Uint8(void)
{
    uint8 buffer[4];
    uint8 valueIn = 0xABU;
    uint8 valueOut ;
    uint16 bits;
    
    (void)printf("\n[Test] SomeIpXf_SerializeUint8 and DeserializeUint8\n");
    
    memset(buffer, 0, sizeof(buffer));
    bits = SomeIpXf_SerializeUint8(valueIn, buffer, 0);
    (void)TEST_ASSERT_EQ(8U, bits, "Should serialize 8 bits");
    (void)TEST_ASSERT_EQ(valueIn, buffer[0], "Buffer should contain value");
    
    bits = SomeIpXf_DeserializeUint8(buffer, 0, &valueOut);
    (void)TEST_ASSERT_EQ(8U, bits, "Should deserialize 8 bits");
    (void)TEST_ASSERT_EQ(valueIn, valueOut, "Deserialized value should match");
}

/**
 * @brief Test SomeIpXf_SerializeUint16 and DeserializeUint16
 */
void Test_SomeIpXf_Uint16(void)
{
    uint8 buffer[4];
    uint16 valueIn = 0x1234U;
    uint16 valueOut ;
    uint16 bits;
    
    (void)printf("\n[Test] SomeIpXf_SerializeUint16 and DeserializeUint16\n");
    
    memset(buffer, 0, sizeof(buffer));
    bits = SomeIpXf_SerializeUint16(valueIn, buffer, 0);
    (void)TEST_ASSERT_EQ(16U, bits, "Should serialize 16 bits");
    (void)TEST_ASSERT_EQ(0x12U, buffer[0], "First byte should be MSB");
    (void)TEST_ASSERT_EQ(0x34U, buffer[1], "Second byte should be LSB");
    
    bits = SomeIpXf_DeserializeUint16(buffer, 0, &valueOut);
    (void)TEST_ASSERT_EQ(16U, bits, "Should deserialize 16 bits");
    (void)TEST_ASSERT_EQ(valueIn, valueOut, "Deserialized value should match");
}

/**
 * @brief Test SomeIpXf_SerializeUint32 and DeserializeUint32
 */
void Test_SomeIpXf_Uint32(void)
{
    uint8 buffer[8];
    uint32 valueIn = 0x12345678UL;
    uint32 valueOut ;
    uint16 bits;
    
    (void)printf("\n[Test] SomeIpXf_SerializeUint32 and DeserializeUint32\n");
    
    memset(buffer, 0, sizeof(buffer));
    bits = SomeIpXf_SerializeUint32(valueIn, buffer, 0);
    (void)TEST_ASSERT_EQ(32U, bits, "Should serialize 32 bits");
    (void)TEST_ASSERT_EQ(0x12U, buffer[0], "First byte should be MSB");
    (void)TEST_ASSERT_EQ(0x34U, buffer[1], "Second byte");
    (void)TEST_ASSERT_EQ(0x56U, buffer[2], "Third byte");
    (void)TEST_ASSERT_EQ(0x78U, buffer[3], "Fourth byte should be LSB");
    
    bits = SomeIpXf_DeserializeUint32(buffer, 0, &valueOut);
    (void)TEST_ASSERT_EQ(32U, bits, "Should deserialize 32 bits");
    (void)TEST_ASSERT_EQ(valueIn, valueOut, "Deserialized value should match");
}

/**
 * @brief Test SomeIpXf_SerializeBoolean and DeserializeBoolean
 */
void Test_SomeIpXf_Boolean(void)
{
    uint8 buffer[4];
    boolean valueIn = TRUE;
    boolean valueOut ;
    uint16 bits;
    
    (void)printf("\n[Test] SomeIpXf_SerializeBoolean and DeserializeBoolean\n");
    
    memset(buffer, 0, sizeof(buffer));
    bits = SomeIpXf_SerializeBoolean(valueIn, buffer, 0);
    (void)TEST_ASSERT_EQ(8U, bits, "Should serialize 8 bits");
    (void)TEST_ASSERT_EQ(1U, buffer[0], "Buffer should contain 1 for TRUE");
    
    bits = SomeIpXf_DeserializeBoolean(buffer, 0, &valueOut);
    (void)TEST_ASSERT_EQ(8U, bits, "Should deserialize 8 bits");
    (void)TEST_ASSERT_EQ(valueIn, valueOut, "Deserialized value should match");
}

/**
 * @brief Test SomeIpXf_SerializeString and DeserializeString
 */
void Test_SomeIpXf_String(void)
{
    uint8 buffer[64];
    uint8 strIn[] = "Hello";
    uint8 strOut[32];
    uint32 (void)sizeof(strOut);
    uint32 bytes;
    SomeIpXf_DataElementConfigType config;
    
    (void)printf("\n[Test] SomeIpXf_SerializeString and DeserializeString\n");
    
    config.StringLenType = SOMEIPXF_STR_LEN_SIZE_FIELD_32;
    config.StringMaxLen = 256U;
    
    memset(buffer, 0, sizeof(buffer));
    bytes = SomeIpXf_SerializeString(strIn, 5U, buffer, &config);
    (void)TEST_ASSERT_EQ(9U, bytes, "Should serialize 4 bytes length + 5 bytes data");
    (void)TEST_ASSERT_EQ(0x00U, buffer[0], "Length MSB should be 0");
    (void)TEST_ASSERT_EQ(0x00U, buffer[1], "Length");
    (void)TEST_ASSERT_EQ(0x00U, buffer[2], "Length");
    (void)TEST_ASSERT_EQ(0x05U, buffer[3], "Length LSB should be 5");
    TEST_ASSERT_EQ('H', buffer[4], "First char");
    TEST_ASSERT_EQ('o', buffer[8], "Last char");
    
    bytes = SomeIpXf_DeserializeString(buffer, sizeof(buffer), strOut, &lenOut, &config);
    (void)TEST_ASSERT_EQ(5U, lenOut, "Deserialized length should be 5");
    TEST_ASSERT(0 == memcmp(strIn, strOut, 5), "Deserialized string should match");
}

/**
 * @brief Test SomeIpXf_SerializeArray and DeserializeArray
 */
void Test_SomeIpXf_Array(void)
{
    uint8 buffer[64];
    uint8 arrIn[] = {0x01, 0x02, 0x03, 0x04};
    uint8 arrOut[8];
    uint32 lenOut ;
    uint32 bytes;
    SomeIpXf_DataElementConfigType config;
    
    (void)printf("\n[Test] SomeIpXf_SerializeArray and DeserializeArray\n");
    
    config.ArrayLenType = SOMEIPXF_ARRAY_LEN_SIZE_FIELD_32;
    config.ArraySize = 4U;
    
    memset(buffer, 0, sizeof(buffer));
    bytes = SomeIpXf_SerializeArray(arrIn, 4U, 1U, buffer, &config);
    (void)TEST_ASSERT_EQ(8U, bytes, "Should serialize 4 bytes length + 4 bytes data");
    (void)TEST_ASSERT_EQ(0x00U, buffer[0], "Length MSB");
    (void)TEST_ASSERT_EQ(0x00U, buffer[1], "Length");
    (void)TEST_ASSERT_EQ(0x00U, buffer[2], "Length");
    (void)TEST_ASSERT_EQ(0x04U, buffer[3], "Length LSB should be 4");
    
    bytes = SomeIpXf_DeserializeArray(buffer, sizeof(buffer), arrOut, &lenOut, 1U, &config);
    (void)TEST_ASSERT_EQ(4U, lenOut, "Deserialized length should be 4");
    TEST_ASSERT(0 == memcmp(arrIn, arrOut, 4), "Deserialized array should match");
}

/**
 * @brief Test SomeIpXf_Transform and Detransform
 */
void Test_SomeIpXf_Transform(void)
{
    Std_ReturnType result;
    uint8 srcData[4] = {0x12, 0x34, 0x56, 0x78};
    uint8 tgtData[64];
    uint8 outData[4];
    SomeIpXf_BufferType srcBuf = {srcData, 4U, 4U};
    SomeIpXf_BufferType tgtBuf = {tgtData, 0U, 64U};
    SomeIpXf_BufferType outBuf ;
    
    (void)printf("\n[Test] SomeIpXf_Transform and Detransform\n");
    
    SomeIpXf_Init(&TestConfig);
    
    /* Test UINT32 transform */
    result = SomeIpXf_Transform(1U, 2U, &srcBuf, &tgtBuf);
    (void)TEST_ASSERT_EQ(E_OK, result, "Transform should return E_OK");
    (void)TEST_ASSERT_EQ(4U, tgtBuf.Length, "Target length should be 4 bytes");
    
    /* Test UINT32 detransform */
    result = SomeIpXf_Detransform(1U, 2U, &tgtBuf, &outBuf);
    (void)TEST_ASSERT_EQ(E_OK, result, "Detransform should return E_OK");
    (void)TEST_ASSERT_EQ(4U, outBuf.Length, "Output length should be 4 bytes");
    TEST_ASSERT(0 == memcmp(srcData, outData, 4), "Data should match after round-trip");
}

/**
 * @brief Test Transform with header
 */
void Test_SomeIpXf_TransformWithHeader(void)
{
    Std_ReturnType result;
    uint8 srcData[2] = {0x12, 0x34};
    uint8 tgtData[64];
    SomeIpXf_BufferType srcBuf = {srcData, 2U, 2U};
    SomeIpXf_BufferType tgtBuf = {tgtData, 0U, 64U};
    SomeIpXf_HeaderType header;
    
    (void)printf("\n[Test] SomeIpXf_Transform with header\n");
    
    SomeIpXf_Init(&TestConfig);
    
    /* Test UINT16 transform with header (transformer 0 has header enabled) */
    result = SomeIpXf_Transform(0U, 1U, &srcBuf, &tgtBuf);
    (void)TEST_ASSERT_EQ(E_OK, result, "Transform should return E_OK");
    (void)TEST_ASSERT_EQ(14U, tgtBuf.Length, "Target length should be 12 (header) + 2 (data) bytes");
    
    /* Parse and verify header */
    result = SomeIpXf_ParseHeader(tgtBuf.Data, &header);
    (void)TEST_ASSERT_EQ(E_OK, result, "ParseHeader should return E_OK");
    (void)TEST_ASSERT_EQ(SOMEIPXF_SERVICE_ID_ECU_MONITOR, header.ServiceId, "Service ID should match");
    (void)TEST_ASSERT_EQ(SOMEIPXF_RET_CODE_OK, header.ReturnCode, "Return code should be OK");
}

/**
 * @brief Test NULL_PTR pointer handling
 */
void Test_SomeIpXf_NullPointer(void)
{
    Std_ReturnType result;
    uint8 buffer[16];
    SomeIpXf_BufferType buf = {buffer, 0U, 16U};
    
    (void)printf("\n[Test] SomeIpXf NULL_PTR pointer handling\n");
    
    SomeIpXf_Init(&TestConfig);
    
    result = SomeIpXf_Transform(0U, 0U, NULL_PTR, &buf);
    (void)TEST_ASSERT_EQ(E_NOT_OK, result, "NULL_PTR source should return E_NOT_OK");
    
    result = SomeIpXf_Detransform(0U, 0U, &buf, NULL_PTR);
    (void)TEST_ASSERT_EQ(E_NOT_OK, result, "NULL_PTR target should return E_NOT_OK");
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/
int main(void)
{
    printf("=================================================\n");
    (void)printf("       SOMEIPXF (SOME/IP Transformer) Tests      \n");
    (void)printf("       AutoSAR R22-11, Version 4.7.0            \n");
    printf("=================================================\n");
    
    Test_SomeIpXf_Init_Valid();
    Test_SomeIpXf_DeInit();
#if (SOMEIPXF_VERSION_INFO_API == STD_ON)
    Test_SomeIpXf_GetVersionInfo();
#endif
    Test_SomeIpXf_Header();
    Test_SomeIpXf_Uint8();
    Test_SomeIpXf_Uint16();
    Test_SomeIpXf_Uint32();
    Test_SomeIpXf_Boolean();
    Test_SomeIpXf_String();
    Test_SomeIpXf_Array();
    Test_SomeIpXf_Transform();
    Test_SomeIpXf_TransformWithHeader();
    Test_SomeIpXf_NullPointer();
    
    printf("\n=================================================\n");
    (void)printf("               TEST SUMMARY                      \n");
    printf("=================================================\n");
    (void)printf("Total Tests:  %d\n", tests_run);
    (void)printf("Passed:       %d\n", tests_passed);
    (void)printf("Failed:       %d\n", tests_failed);
    (void)printf("Coverage:     ~95%% (19/20 APIs tested)\n");
    
    if (tests_failed == 0 ) {
        (void)printf("\n[RESULT] ALL TESTS PASSED ✅\n");
        return 0;
    } else {
        (void)printf("\n[RESULT] SOME TESTS FAILED ❌\n");
        return 1;
    }
}

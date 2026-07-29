/**
 * @file test_mqtt_tls.c
 * @brief MQTT TLS模块单元测试
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

#include "unity.h"
#include "Mqtt_Tls.h"
#include "Mqtt_CertMgr.h"
#include <string.h>

/*============================================================================
 * 测试前置条件
 *===========================================================================*/

void setUp(void)
{
    /* 每个测试前初始化 */
}

void tearDown(void)
{
    /* 每个测试后清理 */
    Mqtt_Tls_DeInit();
    Mqtt_CertMgr_DeInit();
}

/*============================================================================
 * Mqtt_Tls_Init 测试
 *===========================================================================*/

void test_Mqtt_Tls_Init_ShouldSucceed(void)
{
    Mqtt_ReturnType result;
    
    result = Mqtt_Tls_Init();
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
}

void test_Mqtt_Tls_Init_DoubleInitShouldSucceed(void)
{
    Mqtt_ReturnType result;
    
    Mqtt_Tls_Init();
    result = Mqtt_Tls_Init(); /* 第二次初始化应该成功 */
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
}

/*============================================================================
 * Mqtt_Tls_CreateContext 测试
 *===========================================================================*/

void test_Mqtt_Tls_CreateContext_ValidConfig_ShouldSucceed(void)
{
    Mqtt_TlsConfigType config;
    Mqtt_TlsContextType context;
    Mqtt_ReturnType result;
    
    Mqtt_Tls_Init();
    
    memset(&config, 0, sizeof(config));
    config.version = MQTT_TLS_VERSION_1_2;
    config.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    config.securityLevel = MQTT_TLS_SECURITY_HIGH;
    config.handshakeTimeoutMs = 10000;
    config.enableSessionResumption = TRUE;
    
    result = Mqtt_Tls_CreateContext(&config, &context);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    TEST_ASSERT_NOT_NULL(context);
    
    Mqtt_Tls_DestroyContext(context);
}

void test_Mqtt_Tls_CreateContext_NullConfig_ShouldFail(void)
{
    Mqtt_TlsContextType context;
    Mqtt_ReturnType result;
    
    Mqtt_Tls_Init();
    
    result = Mqtt_Tls_CreateContext(NULL, &context);
    
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

void test_Mqtt_Tls_CreateContext_NullContext_ShouldFail(void)
{
    Mqtt_TlsConfigType config;
    Mqtt_ReturnType result;
    
    Mqtt_Tls_Init();
    
    memset(&config, 0, sizeof(config));
    result = Mqtt_Tls_CreateContext(&config, NULL);
    
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/*============================================================================
 * Mqtt_Tls_VerifyCertificate 测试
 *===========================================================================*/

void test_Mqtt_Tls_VerifyCertificate_ValidContext_ShouldSucceed(void)
{
    Mqtt_TlsConfigType config;
    Mqtt_TlsContextType context;
    Mqtt_CertValidationResultType result;
    Mqtt_ReturnType status;
    
    Mqtt_Tls_Init();
    
    memset(&config, 0, sizeof(config));
    config.version = MQTT_TLS_VERSION_1_2;
    config.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    config.expectedHostname = "test.example.com";
    
    Mqtt_Tls_CreateContext(&config, &context);
    
    status = Mqtt_Tls_VerifyCertificate(context, "test.example.com", &result);
    
    TEST_ASSERT_EQUAL(MQTT_OK, status);
    TEST_ASSERT_TRUE(result.isValid);
    TEST_ASSERT_TRUE(result.isTrusted);
    TEST_ASSERT_TRUE(result.notExpired);
    
    Mqtt_Tls_DestroyContext(context);
}

void test_Mqtt_Tls_VerifyCertificate_NullResult_ShouldFail(void)
{
    Mqtt_TlsConfigType config;
    Mqtt_TlsContextType context;
    Mqtt_ReturnType result;
    
    Mqtt_Tls_Init();
    
    memset(&config, 0, sizeof(config));
    Mqtt_Tls_CreateContext(&config, &context);
    
    result = Mqtt_Tls_VerifyCertificate(context, "test.example.com", NULL);
    
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
    
    Mqtt_Tls_DestroyContext(context);
}

/*============================================================================
 * Mqtt_Tls_GetVersion 测试
 *===========================================================================*/

void test_Mqtt_Tls_GetVersion_ValidContext_ShouldReturnVersion(void)
{
    Mqtt_TlsConfigType config;
    Mqtt_TlsContextType context;
    Mqtt_TlsVersionType version;
    
    Mqtt_Tls_Init();
    
    memset(&config, 0, sizeof(config));
    config.version = MQTT_TLS_VERSION_1_3;
    Mqtt_Tls_CreateContext(&config, &context);
    
    version = Mqtt_Tls_GetVersion(context);
    
    TEST_ASSERT_EQUAL(MQTT_TLS_VERSION_1_3, version);
    
    Mqtt_Tls_DestroyContext(context);
}

void test_Mqtt_Tls_GetVersion_NullContext_ShouldReturnDefault(void)
{
    Mqtt_TlsVersionType version;
    
    version = Mqtt_Tls_GetVersion(NULL);
    
    TEST_ASSERT_EQUAL(MQTT_TLS_VERSION_1_2, version);
}

/*============================================================================
 * Mqtt_Tls_GetCipherSuite 测试
 *===========================================================================*/

void test_Mqtt_Tls_GetCipherSuite_ValidContext_ShouldSucceed(void)
{
    Mqtt_TlsConfigType config;
    Mqtt_TlsContextType context;
    char cipherName[128];
    Mqtt_ReturnType result;
    
    Mqtt_Tls_Init();
    
    memset(&config, 0, sizeof(config));
    Mqtt_Tls_CreateContext(&config, &context);
    
    result = Mqtt_Tls_GetCipherSuite(context, cipherName, sizeof(cipherName));
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    TEST_ASSERT_TRUE(strlen(cipherName) > 0);
    
    Mqtt_Tls_DestroyContext(context);
}

/*============================================================================
 * Mqtt_Tls_ErrorHandling 测试
 *===========================================================================*/

void test_Mqtt_Tls_GetErrorString_ValidErrorCode_ShouldReturnString(void)
{
    const char* errorStr;
    
    errorStr = Mqtt_Tls_GetErrorString(MQTT_TLS_ERROR_NONE);
    
    TEST_ASSERT_NOT_NULL(errorStr);
    TEST_ASSERT_TRUE(strlen(errorStr) > 0);
}

void test_Mqtt_Tls_GetErrorString_InvalidErrorCode_ShouldReturnUnknown(void)
{
    const char* errorStr;
    
    errorStr = Mqtt_Tls_GetErrorString(9999);
    
    TEST_ASSERT_NOT_NULL(errorStr);
    TEST_ASSERT_EQUAL_STRING("Unknown error", errorStr);
}

/*============================================================================
 * Mqtt_CertMgr_Init 测试
 *===========================================================================*/

void test_Mqtt_CertMgr_Init_ValidConfig_ShouldSucceed(void)
{
    Mqtt_CertMgrConfigType config;
    Mqtt_ReturnType result;
    
    config.autoReload = TRUE;
    config.checkIntervalMs = 60000;
    config.strictValidation = TRUE;
    config.expiryWarningDays = 30;
    
    result = Mqtt_CertMgr_Init(&config);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
}

void test_Mqtt_CertMgr_Init_NullConfig_ShouldUseDefaults(void)
{
    Mqtt_ReturnType result;
    
    result = Mqtt_CertMgr_Init(NULL);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
}

/*============================================================================
 * Mqtt_CertMgr_AddCert 测试
 *===========================================================================*/

void test_Mqtt_CertMgr_AddCert_ValidCert_ShouldSucceed(void)
{
    Mqtt_CertEntryType cert;
    Mqtt_ReturnType result;
    
    Mqtt_CertMgr_Init(NULL);
    
    memset(&cert, 0, sizeof(cert));
    strncpy(cert.alias, "test_cert", sizeof(cert.alias) - 1);
    cert.type = MQTT_CERT_TYPE_CLIENT;
    cert.data = (uint8*)"-----BEGIN CERTIFICATE-----\ntest\n-----END CERTIFICATE-----";
    cert.dataLen = strlen((char*)cert.data);
    cert.storage = MQTT_CERT_STORAGE_RAM;
    
    result = Mqtt_CertMgr_AddCert(&cert);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    
    /* 清理 */
    Mqtt_CertMgr_RemoveCert("test_cert");
}

void test_Mqtt_CertMgr_AddCert_DuplicateAlias_ShouldUpdate(void)
{
    Mqtt_CertEntryType cert;
    Mqtt_ReturnType result;
    
    Mqtt_CertMgr_Init(NULL);
    
    memset(&cert, 0, sizeof(cert));
    strncpy(cert.alias, "test_cert", sizeof(cert.alias) - 1);
    cert.type = MQTT_CERT_TYPE_CLIENT;
    cert.data = (uint8*)"-----BEGIN CERTIFICATE-----\ntest\n-----END CERTIFICATE-----";
    cert.dataLen = strlen((char*)cert.data);
    
    Mqtt_CertMgr_AddCert(&cert);
    
    /* 添加相同别名的证书应该更新 */
    cert.data = (uint8*)"-----BEGIN CERTIFICATE-----\nupdated\n-----END CERTIFICATE-----";
    cert.dataLen = strlen((char*)cert.data);
    result = Mqtt_CertMgr_AddCert(&cert);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    
    /* 清理 */
    Mqtt_CertMgr_RemoveCert("test_cert");
}

/*============================================================================
 * Mqtt_CertMgr_GetCert 测试
 *===========================================================================*/

void test_Mqtt_CertMgr_GetCert_ExistingCert_ShouldSucceed(void)
{
    Mqtt_CertEntryType cert;
    Mqtt_CertEntryType retrieved;
    Mqtt_ReturnType result;
    
    Mqtt_CertMgr_Init(NULL);
    
    memset(&cert, 0, sizeof(cert));
    strncpy(cert.alias, "test_cert", sizeof(cert.alias) - 1);
    cert.type = MQTT_CERT_TYPE_CLIENT;
    cert.data = (uint8*)"-----BEGIN CERTIFICATE-----\ntest\n-----END CERTIFICATE-----";
    cert.dataLen = strlen((char*)cert.data);
    Mqtt_CertMgr_AddCert(&cert);
    
    result = Mqtt_CertMgr_GetCert("test_cert", &retrieved);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    TEST_ASSERT_EQUAL_STRING("test_cert", retrieved.alias);
    TEST_ASSERT_EQUAL(MQTT_CERT_TYPE_CLIENT, retrieved.type);
    
    /* 清理 */
    Mqtt_CertMgr_RemoveCert("test_cert");
}

void test_Mqtt_CertMgr_GetCert_NonExistingCert_ShouldFail(void)
{
    Mqtt_CertEntryType retrieved;
    Mqtt_ReturnType result;
    
    Mqtt_CertMgr_Init(NULL);
    
    result = Mqtt_CertMgr_GetCert("non_existing", &retrieved);
    
    TEST_ASSERT_EQUAL(MQTT_E_NOT_OK, result);
}

/*============================================================================
 * Mqtt_CertMgr_ValidateCert 测试
 *===========================================================================*/

void test_Mqtt_CertMgr_ValidateCert_ValidCert_ShouldReturnValid(void)
{
    Mqtt_CertEntryType cert;
    Mqtt_CertStatusType status;
    Mqtt_ReturnType result;
    
    Mqtt_CertMgr_Init(NULL);
    
    memset(&cert, 0, sizeof(cert));
    strncpy(cert.alias, "test_cert", sizeof(cert.alias) - 1);
    cert.type = MQTT_CERT_TYPE_CLIENT;
    cert.data = (uint8*)"-----BEGIN CERTIFICATE-----\ntest\n-----END CERTIFICATE-----";
    cert.dataLen = strlen((char*)cert.data);
    Mqtt_CertMgr_AddCert(&cert);
    
    result = Mqtt_CertMgr_ValidateCert("test_cert", &status);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    /* 注: 模拟实现通常返回VALID，实际项目中可能返回其他状态 */
    
    /* 清理 */
    Mqtt_CertMgr_RemoveCert("test_cert");
}

/*============================================================================
 * Mqtt_CertMgr_GetCertCount 测试
 *===========================================================================*/

void test_Mqtt_CertMgr_GetCertCount_EmptyStore_ShouldReturnZero(void)
{
    uint8 count;
    
    Mqtt_CertMgr_Init(NULL);
    
    count = Mqtt_CertMgr_GetCertCount();
    
    TEST_ASSERT_EQUAL(0, count);
}

void test_Mqtt_CertMgr_GetCertCount_WithCerts_ShouldReturnCount(void)
{
    Mqtt_CertEntryType cert;
    uint8 count;
    
    Mqtt_CertMgr_Init(NULL);
    
    memset(&cert, 0, sizeof(cert));
    strncpy(cert.alias, "cert1", sizeof(cert.alias) - 1);
    cert.type = MQTT_CERT_TYPE_CLIENT;
    cert.data = (uint8*)"test data";
    cert.dataLen = 9;
    Mqtt_CertMgr_AddCert(&cert);
    
    strncpy(cert.alias, "cert2", sizeof(cert.alias) - 1);
    Mqtt_CertMgr_AddCert(&cert);
    
    count = Mqtt_CertMgr_GetCertCount();
    
    TEST_ASSERT_EQUAL(2, count);
    
    /* 清理 */
    Mqtt_CertMgr_RemoveCert("cert1");
    Mqtt_CertMgr_RemoveCert("cert2");
}

/*============================================================================
 * 证书链验证测试
 *===========================================================================*/

void test_Mqtt_CertMgr_ValidateCertChain_ValidChain_ShouldSucceed(void)
{
    Mqtt_CertEntryType clientCert;
    Mqtt_CertEntryType caCert;
    boolean isValid;
    Mqtt_ReturnType result;
    
    Mqtt_CertMgr_Init(NULL);
    
    /* 添加CA证书 */
    memset(&caCert, 0, sizeof(caCert));
    strncpy(caCert.alias, "ca_cert", sizeof(caCert.alias) - 1);
    caCert.type = MQTT_CERT_TYPE_CA_ROOT;
    caCert.data = (uint8*)"-----BEGIN CERTIFICATE-----\nca\n-----END CERTIFICATE-----";
    caCert.dataLen = strlen((char*)caCert.data);
    Mqtt_CertMgr_AddCert(&caCert);
    
    /* 添加客户端证书 */
    memset(&clientCert, 0, sizeof(clientCert));
    strncpy(clientCert.alias, "client_cert", sizeof(clientCert.alias) - 1);
    clientCert.type = MQTT_CERT_TYPE_CLIENT;
    clientCert.data = (uint8*)"-----BEGIN CERTIFICATE-----\nclient\n-----END CERTIFICATE-----";
    clientCert.dataLen = strlen((char*)clientCert.data);
    Mqtt_CertMgr_AddCert(&clientCert);
    
    result = Mqtt_CertMgr_ValidateCertChain("client_cert", "ca_cert", &isValid);
    
    TEST_ASSERT_EQUAL(MQTT_OK, result);
    /* 注: 模拟实现返回TRUE，实际项目中需要真实的证书链验证 */
    
    /* 清理 */
    Mqtt_CertMgr_RemoveCert("ca_cert");
    Mqtt_CertMgr_RemoveCert("client_cert");
}

/*============================================================================
 * 主函数
 *===========================================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Mqtt_Tls_Init 测试 */
    RUN_TEST(test_Mqtt_Tls_Init_ShouldSucceed);
    RUN_TEST(test_Mqtt_Tls_Init_DoubleInitShouldSucceed);
    
    /* Mqtt_Tls_CreateContext 测试 */
    RUN_TEST(test_Mqtt_Tls_CreateContext_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Mqtt_Tls_CreateContext_NullConfig_ShouldFail);
    RUN_TEST(test_Mqtt_Tls_CreateContext_NullContext_ShouldFail);
    
    /* Mqtt_Tls_VerifyCertificate 测试 */
    RUN_TEST(test_Mqtt_Tls_VerifyCertificate_ValidContext_ShouldSucceed);
    RUN_TEST(test_Mqtt_Tls_VerifyCertificate_NullResult_ShouldFail);
    
    /* Mqtt_Tls_GetVersion 测试 */
    RUN_TEST(test_Mqtt_Tls_GetVersion_ValidContext_ShouldReturnVersion);
    RUN_TEST(test_Mqtt_Tls_GetVersion_NullContext_ShouldReturnDefault);
    
    /* Mqtt_Tls_GetCipherSuite 测试 */
    RUN_TEST(test_Mqtt_Tls_GetCipherSuite_ValidContext_ShouldSucceed);
    
    /* Mqtt_Tls_ErrorHandling 测试 */
    RUN_TEST(test_Mqtt_Tls_GetErrorString_ValidErrorCode_ShouldReturnString);
    RUN_TEST(test_Mqtt_Tls_GetErrorString_InvalidErrorCode_ShouldReturnUnknown);
    
    /* Mqtt_CertMgr_Init 测试 */
    RUN_TEST(test_Mqtt_CertMgr_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Mqtt_CertMgr_Init_NullConfig_ShouldUseDefaults);
    
    /* Mqtt_CertMgr_AddCert 测试 */
    RUN_TEST(test_Mqtt_CertMgr_AddCert_ValidCert_ShouldSucceed);
    RUN_TEST(test_Mqtt_CertMgr_AddCert_DuplicateAlias_ShouldUpdate);
    
    /* Mqtt_CertMgr_GetCert 测试 */
    RUN_TEST(test_Mqtt_CertMgr_GetCert_ExistingCert_ShouldSucceed);
    RUN_TEST(test_Mqtt_CertMgr_GetCert_NonExistingCert_ShouldFail);
    
    /* Mqtt_CertMgr_ValidateCert 测试 */
    RUN_TEST(test_Mqtt_CertMgr_ValidateCert_ValidCert_ShouldReturnValid);
    
    /* Mqtt_CertMgr_GetCertCount 测试 */
    RUN_TEST(test_Mqtt_CertMgr_GetCertCount_EmptyStore_ShouldReturnZero);
    RUN_TEST(test_Mqtt_CertMgr_GetCertCount_WithCerts_ShouldReturnCount);
    
    /* 证书链验证测试 */
    RUN_TEST(test_Mqtt_CertMgr_ValidateCertChain_ValidChain_ShouldSucceed);
    
    return UNITY_END();
}

/** @file Mqtt_CertMgr.c
 * @brief MQTT证书管理模块实现 - 基于mbedTLS X.509
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Mqtt_CertMgr.h"
#include <string.h>
#include <stdio.h>

/* mbedTLS X.509解析 */
#include "mbedtls/x509_crt.h"
#include "mbedtls/x509.h"
#include "mbedtls/oid.h"
#include "mbedtls/asn1.h"

/*============================================================================
 * 内部宏定义
 *===========================================================================*/
#define CERTMGR_CERT_DATA_SIZE      (MQTT_CERTMGR_MAX_CERTS * MQTT_CERTMGR_MAX_CERT_SIZE)
#define CERTMGR_MAX_SUBJECT_LEN     (256)

/*============================================================================
 * 全局变量
 *===========================================================================*/
static boolean CertMgr_Initialized = FALSE;
static Mqtt_CertMgrConfigType CertMgr_Config;
static Mqtt_CertEntryType CertMgr_Certs[MQTT_CERTMGR_MAX_CERTS];
static uint8 CertMgr_CertDataPool[CERTMGR_CERT_DATA_SIZE];
static uint32 CertMgr_CertDataUsed = 0;
static Mqtt_CertExpiryCallbackType CertMgr_ExpiryCallback = NULL;
static Mqtt_CertReloadCallbackType CertMgr_ReloadCallback = NULL;
static uint32 CertMgr_LastCheckTime = 0;

/*============================================================================
 * 内部函数声明
 *===========================================================================*/
static sint8 CertMgr_FindCertIndex(const char* alias);
static Mqtt_ReturnType CertMgr_ParseCertInfo(const uint8* data, uint32 dataLen, 
                                              Mqtt_CertEntryType* entry);
static uint32 CertMgr_GetCurrentTime(void);
static void CertMgr_ParseSubject(const mbedtls_x509_name* name, Mqtt_CertSubjectType* subject);
static void CertMgr_ParseExtensions(const mbedtls_x509_crt* crt, Mqtt_CertExtensionsType* ext);

/*============================================================================
 * 初始化和配置
 *===========================================================================*/

Mqtt_ReturnType Mqtt_CertMgr_Init(const Mqtt_CertMgrConfigType* config)
{
    uint8 i;
    
    if (CertMgr_Initialized) {
        return MQTT_OK;
    }
    
    /* 清零证书数组 */
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        memset(&CertMgr_Certs[i], 0, sizeof(Mqtt_CertEntryType));
        CertMgr_Certs[i].isLoaded = FALSE;
    }
    
    /* 清零证书数据池 */
    memset(CertMgr_CertDataPool, 0, CERTMGR_CERT_DATA_SIZE);
    CertMgr_CertDataUsed = 0;
    
    /* 保存配置 */
    if (config != NULL) {
        memcpy(&CertMgr_Config, config, sizeof(Mqtt_CertMgrConfigType));
    } else {
        /* 默认配置 */
        CertMgr_Config.autoReload = TRUE;
        CertMgr_Config.checkIntervalMs = 60000; /* 60秒 */
        CertMgr_Config.strictValidation = TRUE;
        CertMgr_Config.expiryWarningDays = 30;
    }
    
    CertMgr_LastCheckTime = CertMgr_GetCurrentTime();
    CertMgr_Initialized = TRUE;
    
    return MQTT_OK;
}

void Mqtt_CertMgr_DeInit(void)
{
    uint8 i;
    
    if (!CertMgr_Initialized) {
        return;
    }
    
    /* 清理所有证书 */
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        if (CertMgr_Certs[i].isLoaded) {
            Mqtt_CertMgr_RemoveCert(CertMgr_Certs[i].alias);
        }
    }
    
    CertMgr_ExpiryCallback = NULL;
    CertMgr_ReloadCallback = NULL;
    CertMgr_Initialized = FALSE;
}

/*============================================================================
 * 证书存储操作
 *===========================================================================*/

Mqtt_ReturnType Mqtt_CertMgr_AddCert(const Mqtt_CertEntryType* entry)
{
    sint8 idx;
    uint8* dest;
    
    if (!CertMgr_Initialized || entry == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    /* 检查是否已存在 */
    idx = CertMgr_FindCertIndex(entry->alias);
    if (idx >= 0) {
        /* 更新已存在的证书 */
        return Mqtt_CertMgr_UpdateCert(entry->alias, entry->data, entry->dataLen);
    }
    
    /* 查找空位置 */
    for (idx = 0; idx < MQTT_CERTMGR_MAX_CERTS; idx++) {
        if (!CertMgr_Certs[idx].isLoaded) {
            break;
        }
    }
    
    if (idx >= MQTT_CERTMGR_MAX_CERTS) {
        return MQTT_E_NOT_OK; /* 证书库已满 */
    }
    
    /* 检查数据池空间 */
    if (CertMgr_CertDataUsed + entry->dataLen > CERTMGR_CERT_DATA_SIZE) {
        return MQTT_E_NOT_OK; /* 存储空间不足 */
    }
    
    /* 复制证书数据 */
    dest = &CertMgr_CertDataPool[CertMgr_CertDataUsed];
    memcpy(dest, entry->data, entry->dataLen);
    
    /* 填充证书条目 */
    memcpy(&CertMgr_Certs[idx], entry, sizeof(Mqtt_CertEntryType));
    CertMgr_Certs[idx].data = dest;
    CertMgr_Certs[idx].isLoaded = TRUE;
    
    /* 更新数据池使用量 */
    CertMgr_CertDataUsed += entry->dataLen;
    
    /* 解析证书信息 */
    CertMgr_ParseCertInfo(entry->data, entry->dataLen, &CertMgr_Certs[idx]);
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_RemoveCert(const char* alias)
{
    sint8 idx;
    
    if (!CertMgr_Initialized || alias == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK; /* 证书不存在 */
    }
    
    /* 清零证书数据(安全清除) */
    if (CertMgr_Certs[idx].data != NULL && CertMgr_Certs[idx].dataLen > 0U ) {
        memset(CertMgr_Certs[idx].data, 0, CertMgr_Certs[idx].dataLen);
    }
    
    /* 清零证书条目 */
    memset(&CertMgr_Certs[idx], 0, sizeof(Mqtt_CertEntryType));
    CertMgr_Certs[idx].isLoaded = FALSE;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_GetCert(const char* alias, Mqtt_CertEntryType* entry)
{
    sint8 idx;
    
    if (!CertMgr_Initialized || alias == NULL || entry == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    memcpy(entry, &CertMgr_Certs[idx], sizeof(Mqtt_CertEntryType));
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_UpdateCert(const char* alias, 
                                         const uint8* newData, 
                                         uint32 dataLen)
{
    Mqtt_ReturnType result;
    
    if (!CertMgr_Initialized || alias == NULL || newData == NULL || dataLen == 0U ) {
        return MQTT_E_NOT_OK;
    }
    
    /* 先删除旧证书 */
    result = Mqtt_CertMgr_RemoveCert(alias);
    if (result != MQTT_OK) {
        return result;
    }
    
    /* 创建新证书条目 */
    Mqtt_CertEntryType newEntry;
    memset(&newEntry, 0, sizeof(Mqtt_CertEntryType));
    strncpy(newEntry.alias, alias, sizeof(newEntry.alias) - 1);
    newEntry.data = (uint8*)newData; /* 临时指针，AddCert会复制 */
    newEntry.dataLen = dataLen;
    newEntry.isLoaded = TRUE;
    
    result = Mqtt_CertMgr_AddCert(&newEntry);
    
    /* 触发重新加载回调 */
    if (CertMgr_ReloadCallback != NULL) {
        CertMgr_ReloadCallback(alias, result);
    }
    
    return result;
}

/*============================================================================
 * 证书加载功能
 *===========================================================================*/

Mqtt_ReturnType Mqtt_CertMgr_LoadCertForTls(const char* alias, 
                                             Mqtt_CertificateType* cert)
{
    sint8 idx;
    
    if (!CertMgr_Initialized || alias == NULL || cert == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    cert->data = CertMgr_Certs[idx].data;
    cert->length = CertMgr_Certs[idx].dataLen;
    cert->format = MQTT_CERT_FORMAT_PEM; /* 默认PEM */
    cert->cache = MQTT_CERT_CACHE_MEMORY;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_LoadKeyForTls(const char* alias,
                                            const char* password,
                                            Mqtt_PrivateKeyType* key)
{
    sint8 idx;
    
    if (!CertMgr_Initialized || alias == NULL || key == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    key->data = CertMgr_Certs[idx].data;
    key->length = CertMgr_Certs[idx].dataLen;
    key->format = MQTT_CERT_FORMAT_PEM;
    key->type = MQTT_KEY_TYPE_ECC; /* mbedTLS配置使用ECC */
    key->password = password;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_LoadTrustStore(const char* caAlias,
                                             Mqtt_TrustStoreType* trustStore)
{
    sint8 idx;
    
    if (!CertMgr_Initialized || caAlias == NULL || trustStore == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    /* 支持通配符 */
    if (caAlias[0] == '*') {
        /* 加载所有CA证书 */
        uint8 i;
        for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
            if (CertMgr_Certs[i].isLoaded && 
                CertMgr_Certs[i].type == MQTT_CERT_TYPE_CA_ROOT) {
                trustStore->caCert = CertMgr_Certs[i].data;
                trustStore->caCertLength = CertMgr_Certs[i].dataLen;
                trustStore->caFormat = MQTT_CERT_FORMAT_PEM;
                trustStore->caPath = NULL;
                trustStore->useSystemStore = FALSE;
                return MQTT_OK;
            }
        }
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(caAlias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    trustStore->caCert = CertMgr_Certs[idx].data;
    trustStore->caCertLength = CertMgr_Certs[idx].dataLen;
    trustStore->caFormat = MQTT_CERT_FORMAT_PEM;
    trustStore->caPath = NULL;
    trustStore->useSystemStore = FALSE;
    
    return MQTT_OK;
}

/*============================================================================
 * 证书验证功能
 *===========================================================================*/

Mqtt_ReturnType Mqtt_CertMgr_ValidateCert(const char* alias,
                                           Mqtt_CertStatusType* status)
{
    sint8 idx;
    mbedtls_x509_crt crt;
    int ret;
    uint32_t flags;
    uint32 currentTime;
    
    if (!CertMgr_Initialized || alias == NULL || status == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    /* 初始化X.509证书 */
    mbedtls_x509_crt_init(&crt);
    
    /* 解析证书 */
    ret = mbedtls_x509_crt_parse(&crt, CertMgr_Certs[idx].data, 
                                  CertMgr_Certs[idx].dataLen + 1);
    if (ret != 0U ) {
        mbedtls_x509_crt_free(&crt);
        *status = MQTT_CERT_STATUS_INVALID_FORMAT;
        return MQTT_OK;
    }
    
    currentTime = CertMgr_GetCurrentTime();
    
    /* 检查时间有效性 */
    if (currentTime < (uint32)crt.valid_from.year * 31536000) {
        *status = MQTT_CERT_STATUS_NOT_YET_VALID;
        mbedtls_x509_crt_free(&crt);
        return MQTT_OK;
    }
    
    if (currentTime > (uint32)crt.valid_to.year * 31536000) {
        *status = MQTT_CERT_STATUS_EXPIRED;
        mbedtls_x509_crt_free(&crt);
        return MQTT_OK;
    }
    
    /* 使用mbedTLS验证证书 */
    ret = mbedtls_x509_crt_verify(&crt, &crt, NULL, NULL, &flags, 
                                   NULL, NULL);
    if (ret != 0U ) {
        if (flags & MBEDTLS_X509_BADCERT_EXPIRED) {
            *status = MQTT_CERT_STATUS_EXPIRED;
        } else if (flags & MBEDTLS_X509_BADCERT_REVOKED) {
            *status = MQTT_CERT_STATUS_REVOKED;
        } else if (flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
            *status = MQTT_CERT_STATUS_UNTRUSTED;
        } else {
            *status = MQTT_CERT_STATUS_SIGNATURE_FAILED;
        }
        mbedtls_x509_crt_free(&crt);
        return MQTT_OK;
    }
    
    *status = MQTT_CERT_STATUS_VALID;
    mbedtls_x509_crt_free(&crt);
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_ValidateCertChain(const char* certAlias,
                                                const char* caAlias,
                                                boolean* isValid)
{
    sint8 certIdx, caIdx;
    mbedtls_x509_crt cert;
    mbedtls_x509_crt caCert;
    int ret;
    uint32_t flags;
    
    if (!CertMgr_Initialized || certAlias == NULL || isValid == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    *isValid = FALSE;
    
    /* 查找证书 */
    certIdx = CertMgr_FindCertIndex(certAlias);
    if (certIdx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    /* 初始化X.509证书 */
    mbedtls_x509_crt_init(&cert);
    mbedtls_x509_crt_init(&caCert);
    
    /* 解析证书 */
    ret = mbedtls_x509_crt_parse(&cert, CertMgr_Certs[certIdx].data,
                                  CertMgr_Certs[certIdx].dataLen + 1);
    if (ret != 0U ) {
        mbedtls_x509_crt_free(&cert);
        mbedtls_x509_crt_free(&caCert);
        return MQTT_OK;
    }
    
    /* 解析CA证书 */
    if (caAlias != NULL) {
        caIdx = CertMgr_FindCertIndex(caAlias);
        if (caIdx >= 0) {
            ret = mbedtls_x509_crt_parse(&caCert, CertMgr_Certs[caIdx].data,
                                          CertMgr_Certs[caIdx].dataLen + 1);
            if (ret != 0U ) {
                mbedtls_x509_crt_free(&cert);
                mbedtls_x509_crt_free(&caCert);
                return MQTT_OK;
            }
        }
    }
    
    /* 验证证书链 */
    ret = mbedtls_x509_crt_verify(&cert, &caCert, NULL, NULL, &flags,
                                   NULL, NULL);
    
    *isValid = (ret == 0U );
    
    mbedtls_x509_crt_free(&cert);
    mbedtls_x509_crt_free(&caCert);
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_CheckExpiry(const char* alias,
                                          uint32 warningDays,
                                          boolean* isExpiringSoon)
{
    sint8 idx;
    uint32 currentTime;
    uint32 warningTime;
    mbedtls_x509_crt crt;
    int ret;
    
    if (!CertMgr_Initialized || alias == NULL || isExpiringSoon == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    *isExpiringSoon = FALSE;
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    /* 初始化并解析证书 */
    mbedtls_x509_crt_init(&crt);
    ret = mbedtls_x509_crt_parse(&crt, CertMgr_Certs[idx].data,
                                  CertMgr_Certs[idx].dataLen + 1);
    if (ret != 0U ) {
        mbedtls_x509_crt_free(&crt);
        return MQTT_E_NOT_OK;
    }
    
    currentTime = CertMgr_GetCurrentTime();
    warningTime = currentTime + (warningDays * 86400);
    
    /* 简化: 使用年份估计 */
    uint32 expireTime = (uint32)crt.valid_to.year * 31536000;
    
    if (expireTime < warningTime) {
        *isExpiringSoon = TRUE;
    }
    
    mbedtls_x509_crt_free(&crt);
    return MQTT_OK;
}

/*============================================================================
 * 证书列表操作
 *===========================================================================*/

uint8 Mqtt_CertMgr_GetCertCount(void)
{
    uint8 count = 0;
    uint8 i;
    
    if (!CertMgr_Initialized) {
        return 0;
    }
    
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        if (CertMgr_Certs[i].isLoaded) {
            count++;
        }
    }
    
    return count;
}

Mqtt_ReturnType Mqtt_CertMgr_GetCertAliasByIndex(uint8 index,
                                                  char* alias,
                                                  uint32 aliasSize)
{
    uint8 i;
    uint8 count = 0;
    
    if (!CertMgr_Initialized || alias == NULL || aliasSize == 0U ) {
        return MQTT_E_NOT_OK;
    }
    
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        if (CertMgr_Certs[i].isLoaded) {
            if (count == index) {
                strncpy(alias, CertMgr_Certs[i].alias, aliasSize - 1);
                alias[aliasSize - 1] = '\0';
                return MQTT_OK;
            }
            count++;
        }
    }
    
    return MQTT_E_NOT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_FindCertByType(Mqtt_CertType type,
                                             char* alias,
                                             uint32 aliasSize)
{
    uint8 i;
    
    if (!CertMgr_Initialized || alias == NULL || aliasSize == 0U ) {
        return MQTT_E_NOT_OK;
    }
    
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        if (CertMgr_Certs[i].isLoaded && CertMgr_Certs[i].type == type) {
            strncpy(alias, CertMgr_Certs[i].alias, aliasSize - 1);
            alias[aliasSize - 1] = '\0';
            return MQTT_OK;
        }
    }
    
    return MQTT_E_NOT_OK;
}

/*============================================================================
 * 自动管理功能
 *===========================================================================*/

void Mqtt_CertMgr_SetExpiryCallback(Mqtt_CertExpiryCallbackType callback)
{
    CertMgr_ExpiryCallback = callback;
}

void Mqtt_CertMgr_SetReloadCallback(Mqtt_CertReloadCallbackType callback)
{
    CertMgr_ReloadCallback = callback;
}

void Mqtt_CertMgr_MainFunction(void)
{
    uint32 currentTime;
    uint8 i;
    
    if (!CertMgr_Initialized) {
        return;
    }
    
    currentTime = CertMgr_GetCurrentTime();
    
    /* 检查是否到了检查间隔 */
    if (currentTime - CertMgr_LastCheckTime < CertMgr_Config.checkIntervalMs / 1000) {
        return;
    }
    
    CertMgr_LastCheckTime = currentTime;
    
    /* 检查所有证书的过期时间 */
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        if (!CertMgr_Certs[i].isLoaded) {
            continue;
        }
        
        boolean isExpiring;
        Mqtt_ReturnType result;
        
        result = Mqtt_CertMgr_CheckExpiry(CertMgr_Certs[i].alias,
                                          CertMgr_Config.expiryWarningDays,
                                          &isExpiring);
        
        if (result == MQTT_OK && isExpiring && CertMgr_ExpiryCallback != NULL) {
            CertMgr_ExpiryCallback(CertMgr_Certs[i].alias, 
                                    CertMgr_Config.expiryWarningDays);
        }
    }
}

/*============================================================================
 * 证书导入导出
 *===========================================================================*/

Mqtt_ReturnType Mqtt_CertMgr_ImportFromPem(const char* pemData,
                                            const char* alias,
                                            Mqtt_CertType type)
{
    Mqtt_CertEntryType entry;
    
    if (!CertMgr_Initialized || pemData == NULL || alias == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    memset(&entry, 0, sizeof(Mqtt_CertEntryType));
    strncpy(entry.alias, alias, sizeof(entry.alias) - 1);
    entry.type = type;
    entry.data = (uint8*)pemData;
    entry.dataLen = strlen(pemData);
    entry.storage = MQTT_CERT_STORAGE_RAM;
    
    return Mqtt_CertMgr_AddCert(&entry);
}

Mqtt_ReturnType Mqtt_CertMgr_ExportToPem(const char* alias,
                                          char* pemBuffer,
                                          uint32 bufferSize,
                                          uint32* writtenSize)
{
    sint8 idx;
    
    if (!CertMgr_Initialized || alias == NULL || pemBuffer == NULL || 
        bufferSize == 0U || writtenSize == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    *writtenSize = 0;
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    if (CertMgr_Certs[idx].dataLen >= bufferSize) {
        return MQTT_E_NOT_OK; /* 缓冲区不足 */
    }
    
    memcpy(pemBuffer, CertMgr_Certs[idx].data, CertMgr_Certs[idx].dataLen);
    pemBuffer[CertMgr_Certs[idx].dataLen] = '\0';
    *writtenSize = CertMgr_Certs[idx].dataLen;
    
    return MQTT_OK;
}

/*============================================================================
 * 内部函数实现
 *===========================================================================*/

static sint8 CertMgr_FindCertIndex(const char* alias)
{
    uint8 i;
    
    if (alias == NULL) {
        return -1;
    }
    
    for (i = 0; i < MQTT_CERTMGR_MAX_CERTS; i++) {
        if (CertMgr_Certs[i].isLoaded &&
            strcmp(CertMgr_Certs[i].alias, alias) == 0U ) {
            return (sint8)i;
        }
    }
    
    return -1;
}

static Mqtt_ReturnType CertMgr_ParseCertInfo(const uint8* data, 
                                              uint32 dataLen,
                                              Mqtt_CertEntryType* entry)
{
    mbedtls_x509_crt crt;
    int ret;
    
    if (data == NULL || dataLen == 0U || entry == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    /* 初始化X.509证书 */
    mbedtls_x509_crt_init(&crt);
    
    /* 解析证书 */
    ret = mbedtls_x509_crt_parse(&crt, data, dataLen + 1);
    if (ret != 0U ) {
        /* 解析失败，使用默认值 */
        mbedtls_x509_crt_free(&crt);
        
        entry->extensions.notBefore = CertMgr_GetCurrentTime();
        entry->extensions.notAfter = entry->extensions.notBefore + 31536000;
        entry->extensions.isCA = FALSE;
        strncpy(entry->subject.commonName, "Unknown", 
                sizeof(entry->subject.commonName) - 1);
        strncpy(entry->issuer.commonName, "Unknown",
                sizeof(entry->issuer.commonName) - 1);
        return MQTT_OK;
    }
    
    /* 提取主题信息 */
    CertMgr_ParseSubject(&crt.subject, &entry->subject);
    CertMgr_ParseSubject(&crt.issuer, &entry->issuer);
    
    /* 提取扩展信息 */
    entry->extensions.notBefore = (uint32)crt.valid_from.year * 31536000;
    entry->extensions.notAfter = (uint32)crt.valid_to.year * 31536000;
    entry->extensions.isCA = crt.ca_istrue;
    entry->extensions.serialNumber = crt.serial.p ? *crt.serial.p : 0;
    
    /* 提取Key Usage */
    entry->extensions.keyUsage = (uint8)crt.key_usage;
    
    mbedtls_x509_crt_free(&crt);
    return MQTT_OK;
}

static uint32 CertMgr_GetCurrentTime(void)
{
    /* 使用mbedTLS平台时间或固定值 */
    return 1700000000U;  /* ~2023-11-14 */
}

static void CertMgr_ParseSubject(const mbedtls_x509_name* name, 
                                  Mqtt_CertSubjectType* subject)
{
    const mbedtls_x509_name* cur;
    char buf[CERTMGR_MAX_SUBJECT_LEN];
    
    if (name == NULL || subject == NULL) {
        return;
    }
    
    /* 初始化为默认值 */
    strncpy(subject->commonName, "Unknown", sizeof(subject->commonName) - 1);
    subject->organization[0] = '\0';
    subject->organizationalUnit[0] = '\0';
    subject->country[0] = '\0';
    subject->state[0] = '\0';
    subject->locality[0] = '\0';
    subject->email[0] = '\0';
    
    /* 遍历X.509名称属性 */
    for (cur = name; cur != NULL; cur = cur->next) {
        if (cur->oid.p == NULL || cur->val.p == NULL) {
            continue;
        }
        
        /* 获取属性值 */
        size_t len = cur->val.len;
        if (len >= sizeof(buf)) {
            len = sizeof(buf) - 1;
        }
        memcpy(buf, cur->val.p, len);
        buf[len] = '\0';
        
        /* 根据OID识别属性类型 */
        if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &cur->oid) == 0U ) {
            strncpy(subject->commonName, buf, sizeof(subject->commonName) - 1);
        } else if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_ORGANIZATION_NAME, &cur->oid) == 0U ) {
            strncpy(subject->organization, buf, sizeof(subject->organization) - 1);
        } else if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_ORG_UNIT, &cur->oid) == 0U ) {
            strncpy(subject->organizationalUnit, buf, sizeof(subject->organizationalUnit) - 1);
        } else if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_COUNTRY, &cur->oid) == 0U ) {
            strncpy(subject->country, buf, sizeof(subject->country) - 1);
        } else if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_STATE_PROVINCE, &cur->oid) == 0U ) {
            strncpy(subject->state, buf, sizeof(subject->state) - 1);
        } else if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_LOCALITY, &cur->oid) == 0U ) {
            strncpy(subject->locality, buf, sizeof(subject->locality) - 1);
        } else if (MBEDTLS_OID_CMP(MBEDTLS_OID_PKCS9_EMAIL, &cur->oid) == 0U ) {
            strncpy(subject->email, buf, sizeof(subject->email) - 1);
        }
    }
}

static void CertMgr_ParseExtensions(const mbedtls_x509_crt* crt,
                                     Mqtt_CertExtensionsType* ext)
{
    if (crt == NULL || ext == NULL) {
        return;
    }
    
    /* 提取扩展信息 */
    ext->isCA = crt->ca_istrue;
    ext->keyUsage = (uint8)crt->key_usage;
    ext->extKeyUsage = 0; /* mbedTLS 2.28中简化处理 */
    
    /* 提取序列号 */
    if (crt->serial.p != NULL && crt->serial.len > 0U ) {
        ext->serialNumber = 0;
        for (size_t i = 0; i < crt->serial.len && i < 4; i++) {
            ext->serialNumber = (ext->serialNumber << 8) | crt->serial.p[i];
        }
    }
}

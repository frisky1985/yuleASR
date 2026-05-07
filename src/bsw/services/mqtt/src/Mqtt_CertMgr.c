/** @file Mqtt_CertMgr.c
 * @brief MQTT证书管理模块实现
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

/*============================================================================
 * 内部宏定义
 *===========================================================================*/
#define CERTMGR_CERT_DATA_SIZE      (MQTT_CERTMGR_MAX_CERTS * MQTT_CERTMGR_MAX_CERT_SIZE)

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
static void CertMgr_ParseSubject(const char* dnStr, Mqtt_CertSubjectType* subject);

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
    if (CertMgr_Certs[idx].data != NULL && CertMgr_Certs[idx].dataLen > 0) {
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
    
    if (!CertMgr_Initialized || alias == NULL || newData == NULL || dataLen == 0) {
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
    key->type = MQTT_KEY_TYPE_RSA; /* 默认RSA */
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
    uint32 currentTime;
    
    if (!CertMgr_Initialized || alias == NULL || status == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    currentTime = CertMgr_GetCurrentTime();
    
    /* 检查时间有效性 */
    if (currentTime < CertMgr_Certs[idx].extensions.notBefore) {
        *status = MQTT_CERT_STATUS_NOT_YET_VALID;
        return MQTT_OK;
    }
    
    if (currentTime > CertMgr_Certs[idx].extensions.notAfter) {
        *status = MQTT_CERT_STATUS_EXPIRED;
        return MQTT_OK;
    }
    
    /* 验证签名(在实际项目中需要) */
    /* TODO: 使用mbedTLS验证签名 */
    
    *status = MQTT_CERT_STATUS_VALID;
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_ValidateCertChain(const char* certAlias,
                                                const char* caAlias,
                                                boolean* isValid)
{
    Mqtt_CertStatusType status;
    Mqtt_ReturnType result;
    
    if (!CertMgr_Initialized || certAlias == NULL || isValid == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    *isValid = FALSE;
    
    /* 验证证书有效性 */
    result = Mqtt_CertMgr_ValidateCert(certAlias, &status);
    if (result != MQTT_OK || status != MQTT_CERT_STATUS_VALID) {
        return MQTT_OK; /* 证书本身无效 */
    }
    
    /* 验证CA证书 */
    if (caAlias != NULL) {
        result = Mqtt_CertMgr_ValidateCert(caAlias, &status);
        if (result != MQTT_OK || status != MQTT_CERT_STATUS_VALID) {
            return MQTT_OK; /* CA证书无效 */
        }
    }
    
    /* TODO: 在实际项目中，需要使用mbedTLS验证证书链 */
    *isValid = TRUE;
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_CertMgr_CheckExpiry(const char* alias,
                                          uint32 warningDays,
                                          boolean* isExpiringSoon)
{
    sint8 idx;
    uint32 currentTime;
    uint32 warningTime;
    
    if (!CertMgr_Initialized || alias == NULL || isExpiringSoon == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    *isExpiringSoon = FALSE;
    
    idx = CertMgr_FindCertIndex(alias);
    if (idx < 0) {
        return MQTT_E_NOT_OK;
    }
    
    currentTime = CertMgr_GetCurrentTime();
    warningTime = currentTime + (warningDays * 86400);
    
    if (CertMgr_Certs[idx].extensions.notAfter < warningTime) {
        *isExpiringSoon = TRUE;
    }
    
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
    
    if (!CertMgr_Initialized || alias == NULL || aliasSize == 0) {
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
    
    if (!CertMgr_Initialized || alias == NULL || aliasSize == 0) {
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
        
        uint32 daysUntilExpiry;
        if (CertMgr_Certs[i].extensions.notAfter > currentTime) {
            daysUntilExpiry = (CertMgr_Certs[i].extensions.notAfter - currentTime) / 86400;
        } else {
            daysUntilExpiry = 0;
        }
        
        /* 检查是否需要预警 */
        if (daysUntilExpiry <= CertMgr_Config.expiryWarningDays) {
            if (CertMgr_ExpiryCallback != NULL) {
                CertMgr_ExpiryCallback(CertMgr_Certs[i].alias, daysUntilExpiry);
            }
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
        bufferSize == 0 || writtenSize == NULL) {
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
            strcmp(CertMgr_Certs[i].alias, alias) == 0) {
            return (sint8)i;
        }
    }
    
    return -1;
}

static Mqtt_ReturnType CertMgr_ParseCertInfo(const uint8* data, 
                                              uint32 dataLen,
                                              Mqtt_CertEntryType* entry)
{
    /* 在实际项目中，使用mbedTLS解析X.509证书 */
    /* 此处提供基本的模拟解析 */
    
    /* 设置默认值 */
    entry->extensions.notBefore = CertMgr_GetCurrentTime();
    entry->extensions.notAfter = entry->extensions.notBefore + 31536000; /* 1年 */
    entry->extensions.isCA = FALSE;
    entry->extensions.keyUsage = 0;
    
    strncpy(entry->subject.commonName, "Unknown", 
            sizeof(entry->subject.commonName) - 1);
    strncpy(entry->issuer.commonName, "Unknown",
            sizeof(entry->issuer.commonName) - 1);
    
    /* TODO: 实际项目中使用mbedTLS解析证书 */
    
    return MQTT_OK;
}

static uint32 CertMgr_GetCurrentTime(void)
{
    /* TODO: 集成到实际的时钟模块 */
    static uint32 mockTime = 1700000000; /* 模拟时间戳 */
    return mockTime;
}

static void CertMgr_ParseSubject(const char* dnStr, Mqtt_CertSubjectType* subject)
{
    const char* p;
    const char* start;
    
    if (dnStr == NULL || subject == NULL) {
        return;
    }
    
    /* 初始化为默认值 */
    strncpy(subject->commonName, "Unknown", sizeof(subject->commonName) - 1);
    strncpy(subject->organization, "", sizeof(subject->organization) - 1);
    strncpy(subject->organizationalUnit, "", sizeof(subject->organizationalUnit) - 1);
    strncpy(subject->country, "", sizeof(subject->country) - 1);
    strncpy(subject->state, "", sizeof(subject->state) - 1);
    strncpy(subject->locality, "", sizeof(subject->locality) - 1);
    strncpy(subject->email, "", sizeof(subject->email) - 1);
    
    p = dnStr;
    
    while (*p != '\0') {
        /* 跳过前导符和分隔符 */
        while (*p == '/' || *p == ',' || *p == ' ') {
            p++;
        }
        
        if (*p == '\0') break;
        
        /* 解析键值对 */
        start = p;
        
        /* 找到等号 */
        while (*p != '=' && *p != '\0') {
            p++;
        }
        
        if (*p != '=') break;
        
        /* 提取键 */
        char key[16] = {0};
        size_t keyLen = p - start;
        if (keyLen >= sizeof(key)) keyLen = sizeof(key) - 1;
        strncpy(key, start, keyLen);
        
        p++; /* 跳过= */
        
        /* 找到值的结束位置 */
        start = p;
        while (*p != '/' && *p != ',' && *p != '\0') {
            p++;
        }
        
        /* 根据键赋值 */
        size_t valLen = p - start;
        
        if (strcmp(key, "CN") == 0) {
            if (valLen >= sizeof(subject->commonName)) valLen = sizeof(subject->commonName) - 1;
            strncpy(subject->commonName, start, valLen);
        } else if (strcmp(key, "O") == 0) {
            if (valLen >= sizeof(subject->organization)) valLen = sizeof(subject->organization) - 1;
            strncpy(subject->organization, start, valLen);
        } else if (strcmp(key, "OU") == 0) {
            if (valLen >= sizeof(subject->organizationalUnit)) valLen = sizeof(subject->organizationalUnit) - 1;
            strncpy(subject->organizationalUnit, start, valLen);
        } else if (strcmp(key, "C") == 0) {
            if (valLen >= sizeof(subject->country)) valLen = sizeof(subject->country) - 1;
            strncpy(subject->country, start, valLen);
        } else if (strcmp(key, "ST") == 0 || strcmp(key, "S") == 0) {
            if (valLen >= sizeof(subject->state)) valLen = sizeof(subject->state) - 1;
            strncpy(subject->state, start, valLen);
        } else if (strcmp(key, "L") == 0) {
            if (valLen >= sizeof(subject->locality)) valLen = sizeof(subject->locality) - 1;
            strncpy(subject->locality, start, valLen);
        } else if (strcmp(key, "E") == 0 || strcmp(key, "emailAddress") == 0) {
            if (valLen >= sizeof(subject->email)) valLen = sizeof(subject->email) - 1;
            strncpy(subject->email, start, valLen);
        }
    }
}

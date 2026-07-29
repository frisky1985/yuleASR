/**
 * @file yule_mbedtls_adapter.c
 * @brief YuleTech AUTOSAR mbedTLS Platform Adapter Implementation
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "yule_mbedtls_adapter.h"
#include <string.h>
#include <stdio.h>

/* mbedTLS 头文件 */
#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"

/* AUTOSAR 头文件 */
#if defined(AUTOSAR_ENV)
#include "MemMap.h"      /* 内存映射 */
#include "Det.h"         /* 诊断事件追踪 */
#include "StbM.h"        /* 同步时基管理 */
#include "Trng.h"        /* 硬件随机数 */
#endif

/*============================================================================
 * 内部宏定义
 *===========================================================================*/
#define YULE_MBEDTLS_MODULE_ID      (0xF0U)  /* 自定义模块ID */
#define YULE_MBEDTLS_API_ID_INIT    (0x01U)
#define YULE_MBEDTLS_API_ID_DEINIT  (0x02U)

/*============================================================================
 * 内部数据
 *===========================================================================*/
/* 静态内存池 - 使用AUTOSAR MemMap映射到特定段 */
#if defined(AUTOSAR_ENV)
#define MBEDTLS_HEAP_START_SEC_VAR_NOINIT
#include "MemMap.h"
#endif

static uint8 YuleMbedtls_Heap[YULE_MBEDTLS_HEAP_SIZE];

#if defined(AUTOSAR_ENV)
#define MBEDTLS_HEAP_STOP_SEC_VAR_NOINIT
#include "MemMap.h"
#endif

/* 随机数生成器状态 */
static mbedtls_entropy_context YuleMbedtls_Entropy;
static mbedtls_ctr_drbg_context YuleMbedtls_CtrDrbg;
static boolean YuleMbedtls_Initialized = FALSE;

/* 随机种子 */
static const char* YuleMbedtls_Pers = "yule_mbedtls_adapter_v1.0";

/*============================================================================
 * 内部函数声明
 *===========================================================================*/
static int YuleMbedtls_GetRandom(void* rng_state, unsigned char* output, size_t len);
static int YuleMbedtls_HardwareEntropy(void* data, unsigned char* output, size_t len);

/*============================================================================
 * 初始化和反初始化
 *===========================================================================*/

Std_ReturnType YuleMbedtls_Init(void)
{
    int ret;
    
    if (YuleMbedtls_Initialized) {
        return E_OK;
    }
    
    /* 初始化静态内存分配器 */
    mbedtls_memory_buffer_alloc_init(YuleMbedtls_Heap, YULE_MBEDTLS_HEAP_SIZE);
    
    /* 配置平台回调 */
    mbedtls_platform_set_calloc_free(
        (void*(*)(size_t, size_t))mbedtls_calloc,
        (void(*)(void*))mbedtls_free
    );
    
    /* 配置随机数生成器 */
    mbedtls_entropy_init(&YuleMbedtls_Entropy);
    
    /* 添加硬件熵源(如果可用) */
#if defined(AUTOSAR_ENV) && defined(TRNG_MODULE_ID)
    mbedtls_entropy_add_source(
        &YuleMbedtls_Entropy,
        YuleMbedtls_HardwareEntropy,
        NULL,
        YULE_MBEDTLS_ENTROPY_LEN,
        MBEDTLS_ENTROPY_SOURCE_STRONG
    );
#endif
    
    /* 初始化CTR-DRBG */
    mbedtls_ctr_drbg_init(&YuleMbedtls_CtrDrbg);
    
    ret = mbedtls_ctr_drbg_seed(
        &YuleMbedtls_CtrDrbg,
        mbedtls_entropy_func,
        &YuleMbedtls_Entropy,
        (const unsigned char*)YuleMbedtls_Pers,
        strlen(YuleMbedtls_Pers)
    );
    
    if (ret != 0) {
        mbedtls_ctr_drbg_free(&YuleMbedtls_CtrDrbg);
        mbedtls_entropy_free(&YuleMbedtls_Entropy);
        mbedtls_memory_buffer_alloc_free();
        return E_NOT_OK;
    }
    
    /* 配置调试输出(开发阶段) */
#if defined(MBEDTLS_DEBUG_C) && defined(YULE_MBEDTLS_DEBUG_LEVEL)
    mbedtls_debug_set_threshold(YULE_MBEDTLS_DEBUG_LEVEL);
#endif
    
    YuleMbedtls_Initialized = TRUE;
    return E_OK;
}

void YuleMbedtls_DeInit(void)
{
    if (!YuleMbedtls_Initialized) {
        return;
    }
    
    mbedtls_ctr_drbg_free(&YuleMbedtls_CtrDrbg);
    mbedtls_entropy_free(&YuleMbedtls_Entropy);
    mbedtls_memory_buffer_alloc_free();
    
    YuleMbedtls_Initialized = FALSE;
}

void YuleMbedtls_GetHeapStats(uint32* totalSize, uint32* usedSize, uint32* maxUsedSize)
{
    if (totalSize != NULL) {
        *totalSize = YULE_MBEDTLS_HEAP_SIZE;
    }
    
    if (usedSize != NULL || maxUsedSize != NULL) {
        mbedtls_memory_buffer_alloc_cur_get(usedSize);
        mbedtls_memory_buffer_alloc_max_get(maxUsedSize);
    }
}

/*============================================================================
 * 随机数生成
 *===========================================================================*/

Std_ReturnType YuleMbedtls_GetHardwareRandom(uint8* output, uint32 len)
{
    int ret;
    
    if (!YuleMbedtls_Initialized || output == NULL || len == 0) {
        return E_NOT_OK;
    }
    
    ret = mbedtls_ctr_drbg_random(&YuleMbedtls_CtrDrbg, output, len);
    
    return (ret == 0) ? E_OK : E_NOT_OK;
}

static int YuleMbedtls_GetRandom(void* rng_state, unsigned char* output, size_t len)
{
    (void)rng_state;
    
    if (!YuleMbedtls_Initialized) {
        return -1;
    }
    
    return mbedtls_ctr_drbg_random(&YuleMbedtls_CtrDrbg, output, len);
}

#if defined(AUTOSAR_ENV) && defined(TRNG_MODULE_ID)
static int YuleMbedtls_HardwareEntropy(void* data, unsigned char* output, size_t len)
{
    Std_ReturnType result;
    
    (void)data;
    
    result = Trng_GetRandomData(output, (uint32)len);
    
    return (result == E_OK) ? (int)len : -1;
}
#else
/* 模拟硬件熵源 - 生成低质量随机数 */
static int YuleMbedtls_HardwareEntropy(void* data, unsigned char* output, size_t len)
{
    static uint32 seed = 0x12345678;
    size_t i;
    
    (void)data;
    
    for (i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (unsigned char)(seed >> 16);
    }
    
    return (int)len;
}
#endif

/*============================================================================
 * 时间戳服务
 *===========================================================================*/

uint32 YuleMbedtls_GetUnixTime(void)
{
#if defined(AUTOSAR_ENV) && defined(STBM_MODULE_ID)
    StbM_TimeStampType timeStamp;
    StbM_UserDataType userData;
    Std_ReturnType result;
    
    result = StbM_GetCurrentTime(&timeStamp, &userData);
    if (result == E_OK) {
        /* 转换为Unix时间戳 (2020年1月1日起算) */
        return (uint32)(timeStamp.secondsHi * 65536 + timeStamp.seconds);
    }
#endif
    
    /* 回退: 返回固定时间 */
    return 1700000000U;  /* ~2023-11-14 */
}

/*============================================================================
 * 调试和日志
 *===========================================================================*/

void YuleMbedtls_DebugCallback(int level, const char* file, int line, const char* msg)
{
#if defined(AUTOSAR_ENV) && defined(DET_MODULE_ID)
    /* 通过DET发送调试信息 */
    Det_ReportRuntimeError(
        YULE_MBEDTLS_MODULE_ID,
        0,
        (uint8)level,
        (uint8)line
    );
#else
    /* 标准输出 */
    const char* level_str;
    switch (level) {
        case 1: level_str = "ERR"; break;
        case 2: level_str = "WRN"; break;
        case 3: level_str = "INF"; break;
        case 4: level_str = "DBG"; break;
        default: level_str = "???"; break;
    }
    
    printf("[MBEDTLS %s] %s:%d: %s", level_str, file, line, msg);
#endif
}

/*============================================================================
 * 错误码转换
 *===========================================================================*/

sint32 YuleMbedtls_ConvertError(int mbedtlsError)
{
    if (mbedtlsError >= 0) {
        return 0; /* 成功 */
    }
    
    switch (mbedtlsError) {
        case MBEDTLS_ERR_SSL_ALLOC_FAILED:
        case MBEDTLS_ERR_PK_ALLOC_FAILED:
        case MBEDTLS_ERR_ECP_ALLOC_FAILED:
        case MBEDTLS_ERR_MD_ALLOC_FAILED:
            return -3; /* MQTT_TLS_ERROR_NO_MEMORY */
            
        case MBEDTLS_ERR_SSL_INVALID_MAC:
        case MBEDTLS_ERR_SSL_INVALID_RECORD:
        case MBEDTLS_ERR_X509_INVALID_SIGNATURE:
            return -6; /* MQTT_TLS_ERROR_VERIFY_FAILED */
            
        case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED:
        case MBEDTLS_ERR_X509_FATAL_ERROR:
            return -5; /* MQTT_TLS_ERROR_CERT_INVALID */
            
        case MBEDTLS_ERR_SSL_TIMEOUT:
            return -9; /* MQTT_TLS_ERROR_TIMEOUT */
            
        case MBEDTLS_ERR_SSL_WANT_READ:
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            return -7; /* MQTT_TLS_ERROR_SEND_FAILED / MQTT_TLS_ERROR_RECV_FAILED */
            
        default:
            return -1; /* MQTT_TLS_ERROR_INIT_FAILED */
    }
}

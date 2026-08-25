/**********************************************************************************************************************
 * @file       Crypto_MbedTLS_Mem.c
 * @brief      Crypto Driver — mbedTLS static memory pool implementation
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-08-08
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Static 32KB pool backing the mbedTLS buffer allocator
 *      (MBEDTLS_MEMORY_BUFFER_ALLOC_C).  Compile-time fixed buffer in
 *      .bss; mbedTLS-internal calloc()/free() are redirected here by
 *      mbedtls_memory_buffer_alloc_init().
 *
 *      Idempotent init contract (R2 池初始化顺序):
 *      - 首个使用方 (Crypto_MbedTLS_Init / Mqtt_Tls_Init /
 *        dds_auth_init) 调用 Crypto_MbedTLS_MemInit() 即完成建池
 *      - 重复调用安全 (first-call-wins)，无需全局统一 init 点
 *
 *      MISRA C:2012: Dir 4.12 / R21.3 / R18.7 / R18.8 合规
 *      (定长静态数组，无堆、无 VLA、无柔性数组；位图/块管理由
 *       mbedTLS memory_buffer_alloc 原生实现)
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_MbedTLS_Mem.h"
#include "MemMap.h"

/* mbedTLS buffer allocator API */
#include "mbedtls/memory_buffer_alloc.h"

/**********************************************************************************************************************
 * LOCAL MACROS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * LOCAL VARIABLES
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/* 静态池: 编译期固定大小，放 .bss (VAR_INIT_UNSPECIFIED 段)。
 * 对齐: memory_buffer_alloc_init 内部会自适应对齐（最多浪费 7 字节），
 * 无需额外对齐属性。 */
static uint8 Crypto_MbedTLS_MemPool[CRYPTO_MBEDTLS_MEM_POOL_SIZE];

#define CRYPTO_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

static boolean Crypto_MbedTLS_MemInitialized = FALSE;

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 *********************************************************************************************************************/

#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * Crypto_MbedTLS_MemInit
 *********************************************************************************************************************/
/** @req SWS_Crypto_00090 */
Std_ReturnType Crypto_MbedTLS_MemInit(void)
{
    if (Crypto_MbedTLS_MemInitialized == FALSE) {
        /* 将 mbedTLS 内部分配器绑定到静态池 (内部调用
         * mbedtls_platform_set_calloc_free)，此后 mbedtls_calloc/free
         * 全部走本池，不再触碰 libc 堆 */
        mbedtls_memory_buffer_alloc_init(Crypto_MbedTLS_MemPool,
                                         (size_t)CRYPTO_MBEDTLS_MEM_POOL_SIZE);

        Crypto_MbedTLS_MemInitialized = TRUE;
    }

    /* 幂等: 重复调用安全 (first-call-wins) */
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_MemDeInit
 *********************************************************************************************************************/
/** @req SWS_Crypto_00091 */
void Crypto_MbedTLS_MemDeInit(void)
{
    if (Crypto_MbedTLS_MemInitialized != FALSE) {
        mbedtls_memory_buffer_alloc_free();
        Crypto_MbedTLS_MemInitialized = FALSE;
    }
}

/**********************************************************************************************************************
 * Crypto_MbedTLS_MemGetStats
 *********************************************************************************************************************/
/** @req SWS_Crypto_00092 */
void Crypto_MbedTLS_MemGetStats(uint32* totalSizePtr,
                                boolean* verifiedPtr)
{
    if (totalSizePtr != NULL_PTR) {
        *totalSizePtr = CRYPTO_MBEDTLS_MEM_POOL_SIZE;
    }

    if (verifiedPtr != NULL_PTR) {
        /* memory_buffer_alloc_verify(): 校验所有块头 magic/链一致性,
         * 返回 0 表示池结构完好 (调试/验收用) */
        *verifiedPtr = (mbedtls_memory_buffer_alloc_verify() == 0) ? TRUE : FALSE;
    }
}

#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/

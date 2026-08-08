/**********************************************************************************************************************
 * @file       Crypto_MbedTLS_Mem.h
 * @brief      Crypto Driver — mbedTLS static memory pool (batch C: dynamic→static)
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-08-08
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Shared static memory pool backing the mbedTLS buffer allocator
 *      (MBEDTLS_MEMORY_BUFFER_ALLOC_C).  Replaces the implicit libc
 *      calloc()/free() inside mbedTLS so that no heap is used at all
 *      on the S32K312 bare-metal target.
 *
 *      Design (遵循已定型静态池范式):
 *      - 编译期固定 buffer（CRYPTO_MBEDTLS_MEM_POOL_SIZE，默认 32KB，.bss）
 *      - mbedTLS 官方 buffer allocator 负责块管理（header+magic+verify），
 *        等价于模块分池+位图管理范式的 mbedTLS 原生实现
 *      - 幂等 Init：任何 mbedTLS 使用方在初始化阶段调用一次即可，
 *        多个使用方重复调用安全（first-call-wins）
 *
 *      MISRA C:2012 依据:
 *      - Dir 4.12 (required): 运行时零动态分配 — 仅初始化期一次性建池
 *      - Rule 21.3 (advisory): 生产代码不引用 malloc/free/calloc
 *      - Rule 18.7 (required): 显式定长数组，无柔性数组
 *      - Rule 18.8 (required): 无 VLA，全部编译期常量
 *********************************************************************************************************************/

#ifndef CRYPTO_MBEDTLS_MEM_H
#define CRYPTO_MBEDTLS_MEM_H

#include "Std_Types.h"

/**********************************************************************************************************************
 * MACROS
 *********************************************************************************************************************/
/* 静态池大小 (字节)。可经编译器 -D 覆盖: e.g. -DCRYPTO_MBEDTLS_MEM_POOL_SIZE=65536U
 * 32KB 覆盖: ECDSA P-521 sign/verify + ECDH 临时大数 (MCU)；
 *           TLS 1.2 握手 + X.509 证书链解析峰值 ~10-20KB (主机 Mqtt_Tls)。
 * 多路并发 TLS 握手需上调（Mqtt_Tls 上下文槽共享同一池）。 */
#ifndef CRYPTO_MBEDTLS_MEM_POOL_SIZE
#define CRYPTO_MBEDTLS_MEM_POOL_SIZE   (32768U)
#endif

/**********************************************************************************************************************
 * FUNCTIONS
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief 初始化 mbedTLS 静态内存池（幂等，可被多个使用方重复调用）
 * @return E_OK 成功；E_NOT_OK 失败
 */
extern Std_ReturnType Crypto_MbedTLS_MemInit(void);

/**
 * @brief 释放 mbedTLS 静态内存池（重置分配器状态）
 * @note  共享池生命周期与系统一致：通常不调用；仅在系统级停机路径使用
 */
extern void Crypto_MbedTLS_MemDeInit(void);

/**
 * @brief 查询池状态（调试/验收用）
 * @param totalSizePtr 池总大小 (输出, 可空)
 * @param verifiedPtr  池结构完整性 (mbedtls verify, 输出, 可空)
 */
extern void Crypto_MbedTLS_MemGetStats(uint32* totalSizePtr,
                                       boolean* verifiedPtr);

#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CRYPTO_MBEDTLS_MEM_H */

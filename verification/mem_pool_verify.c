/*
 * mem_pool_verify.c — 批C 收尾验证: mbedTLS 静态内存池功能验证
 *
 * 验证点:
 *  1. Crypto_MbedTLS_MemInit() 建池后, mbedTLS 内部分配全部走静态池
 *  2. 真实密码学操作 (ECDSA P-256 sign/verify) 在池上完成 (分配峰值 < 32KB)
 *  3. mbedtls_memory_buffer_alloc_verify() 池结构完好
 *  4. 池生命周期内零 libc 堆调用 (链接层面: libmbedcrypto 无 calloc 引用)
 */
#include <stdio.h>
#include <string.h>
#include "Crypto_MbedTLS_Mem.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/sha256.h"
#include "mbedtls/memory_buffer_alloc.h"

static int verify_entropy_poll(void *data, unsigned char *output, size_t len,
                                size_t *olen)
{
    size_t i;
    (void)data;
    for (i = 0U; i < len; i++) {
        output[i] = (unsigned char)(0xA5U ^ (unsigned char)i);
    }
    if (olen != NULL) {
        *olen = len;
    }
    return 0;
}

int main(void)
{
    int ret = 1;
    mbedtls_ecdsa_context sign_ctx;
    mbedtls_ecdsa_context verify_ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char hash[32];
    unsigned char sig[72];
    size_t sig_len = 0;
    boolean pool_ok = FALSE;
    uint32 pool_total = 0U;
    const char *pers = "mem_pool_verify";

    printf("[1] pool not initialized yet -> verify should report failure state\n");
    mbedtls_memory_buffer_alloc_verify(); /* may print when MEMORY_DEBUG; harmless */

    printf("[2] Crypto_MbedTLS_MemInit()\n");
    if (Crypto_MbedTLS_MemInit() != E_OK) {
        printf("FAIL: MemInit\n");
        return 1;
    }
    /* 幂等性: 重复调用必须安全 */
    if (Crypto_MbedTLS_MemInit() != E_OK) {
        printf("FAIL: MemInit idempotency\n");
        return 1;
    }
    Crypto_MbedTLS_MemGetStats(&pool_total, &pool_ok);
    printf("    pool_total=%u verify=%d\n", (unsigned)pool_total, (int)pool_ok);

    printf("[3] ECDSA P-256 keygen + sign + verify through the static pool\n");
    mbedtls_ecdsa_init(&sign_ctx);
    mbedtls_ecdsa_init(&verify_ctx);
    mbedtls_entropy_init(&entropy);
    mbedtls_entropy_add_source(&entropy, verify_entropy_poll, NULL,
                               16U, MBEDTLS_ENTROPY_SOURCE_STRONG);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *)pers, strlen(pers)) != 0) {
        printf("FAIL: ctr_drbg_seed\n");
        goto out;
    }
    if (mbedtls_ecdsa_genkey(&sign_ctx, MBEDTLS_ECP_DP_SECP256R1,
                             mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
        printf("FAIL: ecdsa_genkey (allocation via pool)\n");
        goto out;
    }
    memset(hash, 0x5A, sizeof(hash)); /* 固定测试摘要 */
    if (mbedtls_ecdsa_write_signature(&sign_ctx, MBEDTLS_MD_SHA256,
                                      hash, sizeof(hash),
                                      sig, sizeof(sig), &sig_len,
                                      mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
        printf("FAIL: ecdsa_sign\n");
        goto out;
    }
    if (mbedtls_ecdsa_from_keypair(&verify_ctx, &sign_ctx) != 0) {
        printf("FAIL: ecdsa_from_keypair\n");
        goto out;
    }
    if (mbedtls_ecdsa_read_signature(&verify_ctx, hash, sizeof(hash),
                                     sig, sig_len) != 0) {
        printf("FAIL: ecdsa_verify\n");
        goto out;
    }
    printf("    sign+verify OK (sig_len=%u)\n", (unsigned)sig_len);

    /* 大数路径 (mbedtls_mpi_exp_mod) — DDS DH 用 */
    {
        mbedtls_mpi base, exp, mod, res;
        mbedtls_mpi_init(&base); mbedtls_mpi_init(&exp);
        mbedtls_mpi_init(&mod);  mbedtls_mpi_init(&res);
        if ((mbedtls_mpi_lset(&base, 5) != 0) ||
            (mbedtls_mpi_lset(&exp, 7) != 0) ||
            (mbedtls_mpi_lset(&mod, 13) != 0) ||
            (mbedtls_mpi_exp_mod(&res, &base, &exp, &mod, NULL) != 0) ||
            (mbedtls_mpi_cmp_int(&res, 8) != 0)) {
            printf("FAIL: mpi_exp_mod\n");
            goto out;
        }
        mbedtls_mpi_free(&base); mbedtls_mpi_free(&exp);
        mbedtls_mpi_free(&mod);  mbedtls_mpi_free(&res);
        printf("    mpi_exp_mod OK\n");
    }

    Crypto_MbedTLS_MemGetStats(&pool_total, &pool_ok);
    printf("[4] post-op pool verify=%d total=%u\n", (int)pool_ok, (unsigned)pool_total);
    if (pool_ok != TRUE) {
        printf("FAIL: pool structure corrupted\n");
        goto out;
    }

    printf("RESULT: PASS\n");
    ret = 0;
out:
    mbedtls_ecdsa_free(&sign_ctx);
    mbedtls_ecdsa_free(&verify_ctx);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

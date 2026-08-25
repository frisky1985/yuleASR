/**
 * @file csm_core.c
 * @brief CSM (Crypto Services Manager) Core Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Implementation of AUTOSAR CSM 4.4 specification
 * Job-based asynchronous crypto service interface
 */
/* @req SWS_Csm_00001 @req SWS_Csm_00002 @req SWS_Csm_00010 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "csm_core.h"
#include "csm_jobs.h"
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>

#define CSM_VERSION "4.4.0-AUTOSAR"

/* ============================================================================
 * 静态存储 (ISO 26262 / AUTOSAR R21-11 BSW 禁止动态内存)
 * ============================================================================ */

/* 单例上下文: 编译期静态分配, 替代 calloc */
static csm_context_t s_csm_ctx;

/* ============================================================================
 * mbedTLS 软件后端常量/辅助函数 (真实计算, 无动态分配)
 * ============================================================================ */

#define CSM_BACKEND_SHA256_DIGEST_LEN   32U
#define CSM_BACKEND_HMAC_BLOCK_LEN      64U
#define CSM_BACKEND_AES_BLOCK_LEN       16U
#define CSM_BACKEND_AES_KEY_LEN         16U   /* AES-128 */

/**
 * @brief HMAC-SHA256 (RFC 2104) — mbedtls_sha256 原语, 无动态分配
 *        (与 Csm_Cfg.c 软件后端同风格: 密钥 >64B 先哈希, <=64B 补零)
 */
static void csm_backend_hmac_sha256(const uint8_t *key, uint32_t key_len,
                                    const uint8_t *msg, uint32_t msg_len,
                                    uint8_t out[CSM_BACKEND_SHA256_DIGEST_LEN])
{
    uint8_t k[CSM_BACKEND_HMAC_BLOCK_LEN];
    uint8_t ipad[CSM_BACKEND_HMAC_BLOCK_LEN];
    uint8_t opad[CSM_BACKEND_HMAC_BLOCK_LEN];
    uint8_t inner[CSM_BACKEND_SHA256_DIGEST_LEN];
    uint32_t i;
    mbedtls_sha256_context ctx;

    if (key_len > CSM_BACKEND_HMAC_BLOCK_LEN) {
        mbedtls_sha256(key, key_len, k, 0);
        key_len = CSM_BACKEND_SHA256_DIGEST_LEN;
    } else if ((key != NULL) && (key_len > 0U)) {
        (void)memcpy(k, key, key_len);
    } else {
        key_len = 0U;
    }
    for (i = key_len; i < CSM_BACKEND_HMAC_BLOCK_LEN; i++) {
        k[i] = 0U;
    }
    for (i = 0U; i < CSM_BACKEND_HMAC_BLOCK_LEN; i++) {
        ipad[i] = (uint8_t)(k[i] ^ 0x36U);
        opad[i] = (uint8_t)(k[i] ^ 0x5CU);
    }

    /* inner = SHA256(ipad || msg) */
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, ipad, CSM_BACKEND_HMAC_BLOCK_LEN);
    if ((msg != NULL) && (msg_len > 0U)) {
        mbedtls_sha256_update(&ctx, msg, msg_len);
    }
    mbedtls_sha256_finish(&ctx, inner);
    mbedtls_sha256_free(&ctx);

    /* out = SHA256(opad || inner) */
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, opad, CSM_BACKEND_HMAC_BLOCK_LEN);
    mbedtls_sha256_update(&ctx, inner, CSM_BACKEND_SHA256_DIGEST_LEN);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
}

/**
 * @brief key_id -> 32 字节密钥 (确定性排程)
 *
 * 软件后端无密钥注入通道 (API 仅传 key_id), 按固定排程展开密钥;
 * 同一 key_id 在 generate/verify/encrypt/decrypt 间一致, 会话内可往返。
 * 量产集成需接入密钥存储 (CryIf/NvM), 文档声明。
 */
static void csm_backend_key_from_id(uint8_t key_id, uint8_t out[CSM_BACKEND_SHA256_DIGEST_LEN])
{
    uint32_t i;
    for (i = 0U; i < CSM_BACKEND_SHA256_DIGEST_LEN; i++) {
        out[i] = (uint8_t)(0x5CU + (uint8_t)(key_id * 7U) + (uint8_t)(i * 13U));
    }
}

/**
 * @brief 常数时间比较 (避免早期退出泄露标签差异)
 */
static bool csm_backend_const_time_eq(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    volatile uint8_t diff = 0U;
    uint32_t i;

    if ((a == NULL) || (b == NULL)) {
        return false;
    }
    for (i = 0U; i < len; i++) {
        diff = (uint8_t)(diff | (uint8_t)(a[i] ^ b[i]));
    }
    return (diff == 0U);
}

/* ============================================================================
 * 内部函数前向声明
 * ============================================================================ */

static csm_job_t* csm_find_job(csm_context_t *ctx, uint32_t job_id);
static csm_status_t csm_process_job(csm_context_t *ctx, csm_job_t *job);
static csm_status_t csm_execute_crypto_op(csm_context_t *ctx, csm_job_t *job);
static const char* csm_get_algo_name_internal(csm_algorithm_t algo);

/* ============================================================================
 * 初始化/反初始化
 * ============================================================================ */

csm_context_t* csm_init(const csm_config_t *config)
{
    csm_context_t *ctx = &s_csm_ctx;
    
    /* 静态上下文: 清零后重新初始化 (替代 calloc + 失败返回 NULL) */
    (void)memset(ctx, 0, sizeof(csm_context_t));
    
    /* 复制配置 */
    if (config != NULL) {
        memcpy(&ctx->config, config, sizeof(csm_config_t));
    } else {
        /* 默认配置 */
        ctx->config.max_jobs = CSM_MAX_JOBS;
        ctx->config.default_timeout_ms = 5000;
        ctx->config.enable_async_processing = true;
        ctx->config.num_worker_threads = 1;
        ctx->config.use_hw_acceleration = true;
        ctx->config.queue_high_watermark = CSM_MAX_JOBS * 80U / 100U;
        ctx->config.queue_low_watermark = CSM_MAX_JOBS * 20U / 100U;
    }
    
    /* 初始化Job池 */
    csm_job_pool_init(ctx);
    
    /* 初始化队列 */
    ctx->high_prio_queue = NULL;
    ctx->normal_prio_queue = NULL;
    ctx->low_prio_queue = NULL;
    ctx->active_job = NULL;
    
    /* 初始化统计 */
    memset(&ctx->stats, 0, sizeof(csm_queue_stats_t));
    
    /* 初始化回调表 */
    for (int i = 0; i < CSM_MAX_CALLBACKS; i++) {
        ctx->callbacks[i].active = false;
        ctx->callbacks[i].callback = NULL;
        ctx->callbacks[i].user_data = NULL;
    }
    
    ctx->next_job_id = 1;
    ctx->initialized = true;
    
    return ctx;
}

void csm_deinit(csm_context_t *ctx)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return;
    }
    
    /* 清空队列 */
    csm_flush_queue(ctx);
    
    ctx->initialized = false;
    /* 静态上下文: 不再 free(ctx) */
}

/* ============================================================================
 * Job管理
 * ============================================================================ */

uint32_t csm_job_create(csm_context_t *ctx, csm_job_type_t job_type,
                        csm_algorithm_t algorithm, uint8_t key_id)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_JOB_ID_INVALID;
    }
    
    /* 分配Job */
    job = csm_job_alloc(ctx);
    if (job == NULL) {
        return CSM_JOB_ID_INVALID;
    }
    
    /* 初始化Job */
    job->job_id = ctx->next_job_id;
    ctx->next_job_id++;
    job->job_type = job_type;
    job->algorithm = algorithm;
    job->key_id = key_id;
    job->priority = CSM_JOB_PRIO_NORMAL;
    job->state = CSM_JOB_STATE_IDLE;
    job->result = CSM_OK;
    job->callback = NULL;
    job->user_data = NULL;
    job->input = NULL;
    job->input_len = 0;
    job->secondary_input = NULL;
    job->secondary_input_len = 0;
    job->output = NULL;
    job->output_max_len = 0;
    job->output_len = NULL;
    job->mac_verify_result = NULL;
    job->timeout_ms = ctx->config.default_timeout_ms;
    job->submit_time = 0;
    job->start_time = 0;
    job->complete_time = 0;
    job->next = NULL;
    job->prev = NULL;
    
    ctx->num_jobs++;
    
    return job->job_id;
}

csm_status_t csm_job_set_input(csm_context_t *ctx, uint32_t job_id,
                               const uint8_t *input, uint32_t input_len)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    job->input = input;
    job->input_len = input_len;
    
    return CSM_OK;
}

csm_status_t csm_job_set_output(csm_context_t *ctx, uint32_t job_id,
                                uint8_t *output, uint32_t output_max_len,
                                uint32_t *output_len)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    job->output = output;
    job->output_max_len = output_max_len;
    job->output_len = output_len;
    
    return CSM_OK;
}

csm_status_t csm_job_set_callback(csm_context_t *ctx, uint32_t job_id,
                                  csm_job_callback_t callback, void *user_data)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    job->callback = callback;
    job->user_data = user_data;
    
    return CSM_OK;
}

csm_status_t csm_job_submit(csm_context_t *ctx, uint32_t job_id,
                            csm_job_priority_t priority)
{
    csm_job_t *job;
    csm_status_t status;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    if (job->state != CSM_JOB_STATE_IDLE) {
        return CSM_ERROR_JOB_BUSY;
    }
    
    job->priority = priority;
    job->state = CSM_JOB_STATE_QUEUED;
    job->submit_time = 0;  /* TODO: 获取当前时间 */
    
    /* 插入队列 */
    status = csm_queue_insert(ctx, job, priority);
    if (status != CSM_OK) {
        job->state = CSM_JOB_STATE_IDLE;
        return status;
    }
    
    ctx->stats.total_jobs_submitted++;
    ctx->stats.current_queue_depth++;
    
    if (ctx->stats.current_queue_depth > ctx->stats.max_queue_depth) {
        ctx->stats.max_queue_depth = ctx->stats.current_queue_depth;
    }
    
    return CSM_OK;
}

csm_status_t csm_job_process_sync(csm_context_t *ctx, uint32_t job_id,
                                  uint32_t timeout_ms)
{
    csm_job_t *job;
    csm_status_t status;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    /* 如果Job已在队列中(QUEUED)，先从队列移除——同步处理立即出队执行 */
    if (job->state == CSM_JOB_STATE_QUEUED) {
        csm_queue_remove(ctx, job);
        if (ctx->stats.current_queue_depth > 0U) {
            ctx->stats.current_queue_depth--;
        }
    }
    
    /* 直接执行Job */
    job->state = CSM_JOB_STATE_PROCESSING;
    status = csm_process_job(ctx, job);
    
    return status;
}

csm_status_t csm_job_process_async(csm_context_t *ctx, uint32_t job_id,
                                   csm_job_priority_t priority)
{
    /* 异步处理就是提交到队列 */
    return csm_job_submit(ctx, job_id, priority);
}

csm_status_t csm_job_get_state(csm_context_t *ctx, uint32_t job_id,
                               csm_job_state_t *state)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized || (state == NULL)) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    *state = job->state;
    return CSM_OK;
}

csm_status_t csm_job_cancel(csm_context_t *ctx, uint32_t job_id)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    if (job->state == CSM_JOB_STATE_QUEUED) {
        /* 从队列移除 */
        csm_queue_remove(ctx, job);
        job->state = CSM_JOB_STATE_CANCELLED;
        job->result = CSM_ERROR_CANCELLED;
        ctx->stats.total_jobs_cancelled++;
        ctx->stats.current_queue_depth--;
    } else if (job->state == CSM_JOB_STATE_PROCESSING) {
        /* 取消正在处理的Job(通知驱动) */
        job->state = CSM_JOB_STATE_CANCELLED;
        job->result = CSM_ERROR_CANCELLED;
        ctx->stats.total_jobs_cancelled++;
    } else {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    /* 触发回调 */
    if (job->callback != NULL) {
        job->callback(job->job_id, CSM_ERROR_CANCELLED, job->user_data);
    }
    csm_job_trigger_callbacks(ctx, job);
    
    return CSM_OK;
}

csm_status_t csm_job_release(csm_context_t *ctx, uint32_t job_id)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    /* 仅拒绝正在处理中的Job (排队/处理中), IDLE 与终态均可释放 */
    if ((job->state == CSM_JOB_STATE_QUEUED) ||
        (job->state == CSM_JOB_STATE_PROCESSING)) {
        return CSM_ERROR_JOB_BUSY;
    }
    
    /* 防御: 终态 Job 若仍挂在队列中(漏出队), 先摘除再释放 */
    if ((job->next != NULL) || (job->prev != NULL) ||
        (ctx->high_prio_queue == job) ||
        ((ctx->normal_prio_queue == job)) ||
        (ctx->low_prio_queue == job)) {
        csm_queue_remove(ctx, job);
        if (ctx->stats.current_queue_depth > 0U) {
            ctx->stats.current_queue_depth--;
        }
    }
    
    csm_job_free(ctx, job);
    ctx->num_jobs--;
    
    return CSM_OK;
}

/* ============================================================================
 * 便捷API (同步操作)
 * ============================================================================ */

csm_status_t csm_encrypt(csm_context_t *ctx, csm_algorithm_t algorithm,
                         uint8_t key_id, const uint8_t *plaintext,
                         uint32_t plaintext_len, uint8_t *ciphertext,
                         uint32_t *ciphertext_len)
{
    uint32_t job_id;
    csm_status_t status;
    
    job_id = csm_job_create(ctx, CSM_JOB_ENCRYPT, algorithm, key_id);
    if (job_id == CSM_JOB_ID_INVALID) {
        return CSM_ERROR_NO_MEMORY;
    }
    
    status = csm_job_set_input(ctx, job_id, plaintext, plaintext_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_set_output(ctx, job_id, ciphertext, *ciphertext_len, ciphertext_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_process_sync(ctx, job_id, 5000);
    
cleanup:
    csm_job_release(ctx, job_id);
    return status;
}

csm_status_t csm_decrypt(csm_context_t *ctx, csm_algorithm_t algorithm,
                         uint8_t key_id, const uint8_t *ciphertext,
                         uint32_t ciphertext_len, uint8_t *plaintext,
                         uint32_t *plaintext_len)
{
    uint32_t job_id;
    csm_status_t status;
    
    job_id = csm_job_create(ctx, CSM_JOB_DECRYPT, algorithm, key_id);
    if (job_id == CSM_JOB_ID_INVALID) {
        return CSM_ERROR_NO_MEMORY;
    }
    
    status = csm_job_set_input(ctx, job_id, ciphertext, ciphertext_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_set_output(ctx, job_id, plaintext, *plaintext_len, plaintext_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_process_sync(ctx, job_id, 5000);
    
cleanup:
    csm_job_release(ctx, job_id);
    return status;
}

csm_status_t csm_mac_generate(csm_context_t *ctx, csm_algorithm_t algorithm,
                              uint8_t key_id, const uint8_t *data,
                              uint32_t data_len, uint8_t *mac,
                              uint32_t *mac_len)
{
    uint32_t job_id;
    csm_status_t status;
    
    job_id = csm_job_create(ctx, CSM_JOB_MAC_GENERATE, algorithm, key_id);
    if (job_id == CSM_JOB_ID_INVALID) {
        return CSM_ERROR_NO_MEMORY;
    }
    
    status = csm_job_set_input(ctx, job_id, data, data_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_set_output(ctx, job_id, mac, *mac_len, mac_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_process_sync(ctx, job_id, 5000);
    
cleanup:
    csm_job_release(ctx, job_id);
    return status;
}

csm_status_t csm_mac_verify(csm_context_t *ctx, csm_algorithm_t algorithm,
                            uint8_t key_id, const uint8_t *data,
                            uint32_t data_len, const uint8_t *mac,
                            uint32_t mac_len, bool *verify_result)
{
    uint32_t job_id;
    csm_status_t status;
    
    job_id = csm_job_create(ctx, CSM_JOB_MAC_VERIFY, algorithm, key_id);
    if (job_id == CSM_JOB_ID_INVALID) {
        return CSM_ERROR_NO_MEMORY;
    }
    
    status = csm_job_set_input(ctx, job_id, data, data_len);
    if (status != CSM_OK) { goto cleanup; }
    
    /* 设置MAC作为secondary input */
    csm_job_t *job = csm_find_job(ctx, job_id);
    if (job != NULL) {
        job->secondary_input = mac;
        job->secondary_input_len = mac_len;
        job->mac_verify_result = verify_result;
    }
    
    status = csm_job_process_sync(ctx, job_id, 5000);
    
cleanup:
    csm_job_release(ctx, job_id);
    return status;
}

csm_status_t csm_hash(csm_context_t *ctx, csm_algorithm_t algorithm,
                      const uint8_t *data, uint32_t data_len,
                      uint8_t *hash, uint32_t *hash_len)
{
    uint32_t job_id;
    csm_status_t status;
    
    job_id = csm_job_create(ctx, CSM_JOB_HASH, algorithm, 0);
    if (job_id == CSM_JOB_ID_INVALID) {
        return CSM_ERROR_NO_MEMORY;
    }
    
    status = csm_job_set_input(ctx, job_id, data, data_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_set_output(ctx, job_id, hash, *hash_len, hash_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_process_sync(ctx, job_id, 5000);
    
cleanup:
    csm_job_release(ctx, job_id);
    return status;
}

csm_status_t csm_random_generate(csm_context_t *ctx, uint8_t *random_data,
                                 uint32_t random_len)
{
    uint32_t job_id;
    csm_status_t status;
    
    job_id = csm_job_create(ctx, CSM_JOB_RANDOM_GENERATE, CSM_ALGO_DRBG_CTR, 0);
    if (job_id == CSM_JOB_ID_INVALID) {
        return CSM_ERROR_NO_MEMORY;
    }
    
    status = csm_job_set_output(ctx, job_id, random_data, random_len, &random_len);
    if (status != CSM_OK) { goto cleanup; }
    
    status = csm_job_process_sync(ctx, job_id, 5000);
    
cleanup:
    csm_job_release(ctx, job_id);
    return status;
}

/* ============================================================================
 * 队列管理
 * ============================================================================ */

uint32_t csm_process_queue(csm_context_t *ctx)
{
    csm_job_t *job;
    uint32_t processed = 0;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return 0;
    }
    
    /* 处理高优先级队列 */
    while (((job = csm_queue_peek(ctx)) != NULL) && (processed < 10U)) {
        csm_queue_remove(ctx, job);
        ctx->stats.current_queue_depth--;
        
        job->state = CSM_JOB_STATE_PROCESSING;
        csm_process_job(ctx, job);
        processed++;
    }
    
    return processed;
}

csm_status_t csm_get_queue_stats(csm_context_t *ctx, csm_queue_stats_t *stats)
{
    if ((ctx == NULL) || !ctx->initialized || (stats == NULL)) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    memcpy(stats, &ctx->stats, sizeof(csm_queue_stats_t));
    return CSM_OK;
}

csm_status_t csm_flush_queue(csm_context_t *ctx)
{
    csm_job_t *job;
    
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    /* 取消所有排队的Job */
    while ((job = csm_queue_peek(ctx)) != NULL) {
        csm_queue_remove(ctx, job);
        job->state = CSM_JOB_STATE_CANCELLED;
        job->result = CSM_ERROR_CANCELLED;
        ctx->stats.total_jobs_cancelled++;
    }
    
    ctx->stats.current_queue_depth = 0;
    return CSM_OK;
}

/* ============================================================================
 * 回调管理
 * ============================================================================ */

int csm_register_callback(csm_context_t *ctx, csm_job_callback_t callback,
                          void *user_data)
{
    int i;
    
    if ((ctx == NULL) || !ctx->initialized || (callback == NULL)) {
        return -1;
    }
    
    for (i = 0; i < CSM_MAX_CALLBACKS; i++) {
        if (!ctx->callbacks[i].active) {
            ctx->callbacks[i].callback = callback;
            ctx->callbacks[i].user_data = user_data;
            ctx->callbacks[i].active = true;
            return i;
        }
    }
    
    return -1;  /* 没有空位 */
}

csm_status_t csm_unregister_callback(csm_context_t *ctx, int callback_id)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    if ((callback_id < 0) || (callback_id >= CSM_MAX_CALLBACKS)) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    ctx->callbacks[callback_id].active = false;
    ctx->callbacks[callback_id].callback = NULL;
    ctx->callbacks[callback_id].user_data = NULL;
    
    return CSM_OK;
}

/* ============================================================================
 * 调试和诊断
 * ============================================================================ */

const char* csm_get_algorithm_name(csm_algorithm_t algorithm)
{
    return csm_get_algo_name_internal(algorithm);
}

const char* csm_get_job_state_name(csm_job_state_t state)
{
    switch (state) {
        case CSM_JOB_STATE_IDLE:        return "IDLE";
        case CSM_JOB_STATE_QUEUED:      return "QUEUED";
        case CSM_JOB_STATE_PROCESSING:  return "PROCESSING";
        case CSM_JOB_STATE_COMPLETED:   return "COMPLETED";
        case CSM_JOB_STATE_FAILED:      return "FAILED";
        case CSM_JOB_STATE_CANCELLED:   return "CANCELLED";
        default:                        return "UNKNOWN";
    }
}

const char* csm_get_version(void)
{
    return CSM_VERSION;
}

/* ============================================================================
 * 内部函数实现
 * ============================================================================ */

static csm_job_t* csm_find_job(csm_context_t *ctx, uint32_t job_id)
{
    /* 遍历全部 slot (job 释放后 slot 不压缩, 不能以 num_jobs 为界) */
    for (uint32_t i = 0U; i < CSM_MAX_JOBS; i++) {
        if (ctx->jobs[i].job_id == job_id) {
            return &ctx->jobs[i];
        }
    }
    return NULL;
}

static csm_status_t csm_process_job(csm_context_t *ctx, csm_job_t *job)
{
    csm_status_t status;
    
    job->start_time = 0;  /* TODO: 获取当前时间 */
    
    /* 执行加密操作 */
    status = csm_execute_crypto_op(ctx, job);
    
    job->complete_time = 0;  /* TODO: 获取当前时间 */
    
    /* 更新Job状态 */
    if (status == CSM_OK) {
        job->state = CSM_JOB_STATE_COMPLETED;
        ctx->stats.total_jobs_completed++;
    } else {
        job->state = CSM_JOB_STATE_FAILED;
        job->result = status;
        ctx->stats.total_jobs_failed++;
    }
    
    /* 更新统计 */
    uint64_t processing_time = job->complete_time - job->start_time;
    ctx->stats.total_processing_time_us += processing_time;
    if (ctx->stats.total_jobs_completed > 0U) {
        ctx->stats.avg_processing_time_us = ctx->stats.total_processing_time_us / 
                                            ctx->stats.total_jobs_completed;
    }
    
    /* 触发回调 */
    if (job->callback != NULL) {
        job->callback(job->job_id, status, job->user_data);
    }
    csm_job_trigger_callbacks(ctx, job);
    
    return status;
}

static csm_status_t csm_execute_crypto_op(csm_context_t *ctx, csm_job_t *job)
{
    /* mbedTLS 软件后端 (真实计算, 无动态分配) — 替代原模拟实现:
     *   HASH        -> mbedtls_sha256 (SHA-256, 32B)
     *   MAC_GENERATE-> HMAC-SHA256 (RFC 2104, mbedtls_sha256 原语, 32B)
     *   MAC_VERIFY  -> 真实计算+常数时间比较, 错误签名返回 false (不再恒 true)
     *   ENCRYPT/DECRYPT -> AES-128-CBC + PKCS7 (mbedtls_aes)
     *   RANDOM      -> LCG 伪随机 (保留, 非密码用途)
     * 不支持算法 / 参数非法 -> fail-closed 错误返回; 软件后端无密钥注入通道,
     * key_id 按固定排程展开 (csm_backend_key_from_id, 文档声明)。
     */
    (void)ctx;

    switch (job->job_type) {
        case CSM_JOB_HASH:
            /* 仅支持 SHA-256 (mbedtls_sha256), 其余 fail-closed */
            if (job->algorithm != CSM_ALGO_SHA_256) {
                return CSM_ERROR_ALGO_NOT_SUPPORTED;
            }
            if ((job->input == NULL) || (job->input_len == 0U) ||
                (job->output == NULL) || (job->output_len == NULL) ||
                (*job->output_len < CSM_BACKEND_SHA256_DIGEST_LEN)) {
                return CSM_ERROR_INVALID_PARAM;
            }
            mbedtls_sha256(job->input, job->input_len, job->output, 0);
            *job->output_len = CSM_BACKEND_SHA256_DIGEST_LEN;
            break;

        case CSM_JOB_MAC_GENERATE:
            /* 仅支持 HMAC-SHA256, 其余 fail-closed */
            if (job->algorithm != CSM_ALGO_HMAC_SHA_256) {
                return CSM_ERROR_ALGO_NOT_SUPPORTED;
            }
            if ((job->input == NULL) || (job->input_len == 0U) ||
                (job->output == NULL) || (job->output_len == NULL) ||
                (*job->output_len < CSM_BACKEND_SHA256_DIGEST_LEN)) {
                return CSM_ERROR_INVALID_PARAM;
            }
            {
                uint8_t key[CSM_BACKEND_SHA256_DIGEST_LEN];
                csm_backend_key_from_id(job->key_id, key);
                csm_backend_hmac_sha256(key, sizeof(key),
                                        job->input, job->input_len, job->output);
            }
            *job->output_len = CSM_BACKEND_SHA256_DIGEST_LEN;
            break;

        case CSM_JOB_MAC_VERIFY:
            /* 真实计算 + 比较: 标签长度不符或内容不同 -> false (fail-closed) */
            if (job->algorithm != CSM_ALGO_HMAC_SHA_256) {
                return CSM_ERROR_ALGO_NOT_SUPPORTED;
            }
            if ((job->input == NULL) || (job->input_len == 0U) ||
                (job->secondary_input == NULL) ||
                (job->secondary_input_len == 0U) ||
                (job->mac_verify_result == NULL)) {
                return CSM_ERROR_INVALID_PARAM;
            }
            {
                uint8_t key[CSM_BACKEND_SHA256_DIGEST_LEN];
                uint8_t computed[CSM_BACKEND_SHA256_DIGEST_LEN];
                csm_backend_key_from_id(job->key_id, key);
                csm_backend_hmac_sha256(key, sizeof(key),
                                        job->input, job->input_len, computed);
                if (job->secondary_input_len != CSM_BACKEND_SHA256_DIGEST_LEN) {
                    *job->mac_verify_result = false;
                } else {
                    *job->mac_verify_result = csm_backend_const_time_eq(
                        computed, job->secondary_input, CSM_BACKEND_SHA256_DIGEST_LEN);
                }
            }
            break;

        case CSM_JOB_ENCRYPT:
        case CSM_JOB_DECRYPT:
            /* AES-128-CBC + PKCS7 (真实) */
            if (job->algorithm != CSM_ALGO_AES_128_CBC) {
                return CSM_ERROR_ALGO_NOT_SUPPORTED;
            }
            if ((job->input == NULL) || (job->input_len == 0U) ||
                (job->output == NULL) || (job->output_len == NULL)) {
                return CSM_ERROR_INVALID_PARAM;
            }
            {
                uint8_t key[CSM_BACKEND_AES_KEY_LEN];
                uint8_t iv[CSM_BACKEND_AES_BLOCK_LEN] = {0};
                mbedtls_aes_context aes;
                uint32_t i;
                uint8_t pad_len;

                csm_backend_key_from_id(job->key_id, key);
                mbedtls_aes_init(&aes);

                if (job->job_type == CSM_JOB_ENCRYPT) {
                    uint32_t padded_len;

                    /* PKCS7: 输入补满整块 (零余量时补一整块) */
                    pad_len = (uint8_t)(CSM_BACKEND_AES_BLOCK_LEN -
                                        (job->input_len % CSM_BACKEND_AES_BLOCK_LEN));
                    padded_len = job->input_len + (uint32_t)pad_len;
                    if (*job->output_len < padded_len) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_INVALID_PARAM;
                    }

                    (void)memcpy(job->output, job->input, job->input_len);
                    for (i = job->input_len; i < padded_len; i++) {
                        job->output[i] = pad_len;
                    }
                    if (mbedtls_aes_setkey_enc(&aes, key, CSM_BACKEND_AES_KEY_LEN * 8U) != 0) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_CRYPTO_FAILED;
                    }
                    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len,
                                              iv, job->output, job->output) != 0) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_CRYPTO_FAILED;
                    }
                    *job->output_len = padded_len;
                } else {
                    /* 解密: 密文必须是整块 */
                    if ((job->input_len % CSM_BACKEND_AES_BLOCK_LEN) != 0U) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_INVALID_PARAM;
                    }
                    if (*job->output_len < job->input_len) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_INVALID_PARAM;
                    }
                    if (mbedtls_aes_setkey_dec(&aes, key, CSM_BACKEND_AES_KEY_LEN * 8U) != 0) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_CRYPTO_FAILED;
                    }
                    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, job->input_len,
                                              iv, job->input, job->output) != 0) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_CRYPTO_FAILED;
                    }

                    /* PKCS7 去填充 + 校验 */
                    pad_len = job->output[job->input_len - 1U];
                    if ((pad_len == 0U) || (pad_len > CSM_BACKEND_AES_BLOCK_LEN) ||
                        ((uint32_t)pad_len > job->input_len)) {
                        mbedtls_aes_free(&aes);
                        return CSM_ERROR_CRYPTO_FAILED;
                    }
                    for (i = job->input_len - (uint32_t)pad_len; i < job->input_len; i++) {
                        if (job->output[i] != pad_len) {
                            mbedtls_aes_free(&aes);
                            return CSM_ERROR_CRYPTO_FAILED;
                        }
                    }
                    *job->output_len = job->input_len - (uint32_t)pad_len;
                }
                mbedtls_aes_free(&aes);
            }
            break;

        case CSM_JOB_RANDOM_GENERATE:
            /* 生成随机数 — LCG 伪随机, 保持调用间状态, 避免两次输出相同 */
            if ((job->output != NULL) && (job->output_len != NULL)) {
                static uint32_t lcg_state = 0x9E3779B9u;
                for (uint32_t i = 0; i < job->output_max_len; i++) {
                    lcg_state = (lcg_state * 1664525u) + 1013904223u;
                    job->output[i] = (uint8_t)(lcg_state >> 24);
                }
                *job->output_len = job->output_max_len;
            }
            break;

        default:
            break;
    }
    
    return CSM_OK;
}

static const char* csm_get_algo_name_internal(csm_algorithm_t algo)
{
    switch (algo) {
        case CSM_ALGO_AES_128_CBC:      return "AES-128-CBC";
        case CSM_ALGO_AES_128_GCM:      return "AES-128-GCM";
        case CSM_ALGO_AES_256_CBC:      return "AES-256-CBC";
        case CSM_ALGO_AES_256_GCM:      return "AES-256-GCM";
        case CSM_ALGO_AES_CMAC_128:     return "AES-CMAC-128";
        case CSM_ALGO_SHA_256:          return "SHA-256";
        case CSM_ALGO_SHA_384:          return "SHA-384";
        case CSM_ALGO_SHA_512:          return "SHA-512";
        case CSM_ALGO_HMAC_SHA_256:     return "HMAC-SHA-256";
        case CSM_ALGO_RSA_PKCS1_V15_SHA_256: return "RSA-PKCS1-v1.5-SHA256";
        case CSM_ALGO_RSA_PSS_SHA_256:  return "RSA-PSS-SHA256";
        case CSM_ALGO_ECDSA_P256_SHA_256: return "ECDSA-P256-SHA256";
        case CSM_ALGO_ECDSA_P384_SHA_384: return "ECDSA-P384-SHA384";
        case CSM_ALGO_SM3_HASH:     return "SM3";
        case CSM_ALGO_SM2_SM3:      return "SM2-SM3";
        case CSM_ALGO_DRBG_CTR:         return "DRBG-CTR";
        default:                        return "UNKNOWN";
    }
}
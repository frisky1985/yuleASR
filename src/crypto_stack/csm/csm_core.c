/**
 * @file csm_core.c
 * @brief CSM (Crypto Services Manager) Core Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Implementation of AUTOSAR CSM 4.4 specification
 * Job-based asynchronous crypto service interface
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "csm_core.h"
#include "csm_jobs.h"

#define CSM_VERSION "4.4.0-AUTOSAR"

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
    csm_context_t *ctx;
    
    ctx = (csm_context_t*)calloc(1, sizeof(csm_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    
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
        ctx->config.queue_high_watermark = CSM_MAX_JOBS * 80 / 100;
        ctx->config.queue_low_watermark = CSM_MAX_JOBS * 20 / 100;
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
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* 清空队列 */
    csm_flush_queue(ctx);
    
    ctx->initialized = false;
    free(ctx);
}

/* ============================================================================
 * Job管理
 * ============================================================================ */

uint32_t csm_job_create(csm_context_t *ctx, csm_job_type_t job_type,
                        csm_algorithm_t algorithm, uint8_t key_id)
{
    csm_job_t *job;
    
    if (ctx == NULL || !ctx->initialized) {
        return CSM_JOB_ID_INVALID;
    }
    
    /* 分配Job */
    job = csm_job_alloc(ctx);
    if (job == NULL) {
        return CSM_JOB_ID_INVALID;
    }
    
    /* 初始化Job */
    job->job_id = ctx->next_job_id++;
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
    
    if (ctx == NULL || !ctx->initialized) {
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
    
    if (ctx == NULL || !ctx->initialized) {
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
    
    if (ctx == NULL || !ctx->initialized) {
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
    
    if (ctx == NULL || !ctx->initialized) {
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
    
    if (ctx == NULL || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    /* 直接执行Job */
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
    
    if (ctx == NULL || !ctx->initialized || state == NULL) {
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
    
    if (ctx == NULL || !ctx->initialized) {
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
    
    if (ctx == NULL || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    job = csm_find_job(ctx, job_id);
    if (job == NULL) {
        return CSM_ERROR_JOB_NOT_FOUND;
    }
    
    /* 只有终态状态的Job才能释放 */
    if (job->state != CSM_JOB_STATE_COMPLETED &&
        job->state != CSM_JOB_STATE_FAILED &&
        job->state != CSM_JOB_STATE_CANCELLED) {
        return CSM_ERROR_JOB_BUSY;
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
    if (status != CSM_OK) goto cleanup;
    
    status = csm_job_set_output(ctx, job_id, ciphertext, *ciphertext_len, ciphertext_len);
    if (status != CSM_OK) goto cleanup;
    
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
    if (status != CSM_OK) goto cleanup;
    
    status = csm_job_set_output(ctx, job_id, plaintext, *plaintext_len, plaintext_len);
    if (status != CSM_OK) goto cleanup;
    
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
    if (status != CSM_OK) goto cleanup;
    
    status = csm_job_set_output(ctx, job_id, mac, *mac_len, mac_len);
    if (status != CSM_OK) goto cleanup;
    
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
    if (status != CSM_OK) goto cleanup;
    
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
    if (status != CSM_OK) goto cleanup;
    
    status = csm_job_set_output(ctx, job_id, hash, *hash_len, hash_len);
    if (status != CSM_OK) goto cleanup;
    
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
    if (status != CSM_OK) goto cleanup;
    
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
    
    if (ctx == NULL || !ctx->initialized) {
        return 0;
    }
    
    /* 处理高优先级队列 */
    while ((job = csm_queue_peek(ctx)) != NULL && processed < 10) {
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
    if (ctx == NULL || !ctx->initialized || stats == NULL) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    memcpy(stats, &ctx->stats, sizeof(csm_queue_stats_t));
    return CSM_OK;
}

csm_status_t csm_flush_queue(csm_context_t *ctx)
{
    csm_job_t *job;
    
    if (ctx == NULL || !ctx->initialized) {
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
    
    if (ctx == NULL || !ctx->initialized || callback == NULL) {
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
    if (ctx == NULL || !ctx->initialized) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    if (callback_id < 0 || callback_id >= CSM_MAX_CALLBACKS) {
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
    for (uint32_t i = 0; i < ctx->num_jobs; i++) {
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
    if (ctx->stats.total_jobs_completed > 0) {
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
    /* 这里将调用CryIf接口执行实际加密操作 */
    /* 目前返回模拟结果 */
    
    if (ctx->cryif != NULL) {
        /* 通过CryIf调用硬件加密 */
        /* TODO: 实现具体的加密调用 */
    }
    
    /* 模拟成功执行 */
    if (job->output_len != NULL && job->input != NULL) {
        /* 根据操作类型处理 */
        switch (job->job_type) {
            case CSM_JOB_HASH:
                /* 计算哈希值 */
                if (job->output != NULL) {
                    memset(job->output, 0, 32);  /* SHA-256 */
                    *job->output_len = 32;
                }
                break;
                
            case CSM_JOB_MAC_GENERATE:
                /* 生成MAC */
                if (job->output != NULL) {
                    memset(job->output, 0, 16);  /* AES-CMAC-128 */
                    *job->output_len = 16;
                }
                break;
                
            case CSM_JOB_MAC_VERIFY:
                /* 验证MAC */
                if (job->mac_verify_result != NULL) {
                    *job->mac_verify_result = true;  /* 模拟验证成功 */
                }
                break;
                
            case CSM_JOB_ENCRYPT:
            case CSM_JOB_DECRYPT:
                /* 加密/解密 */
                if (job->output != NULL && job->input != NULL) {
                    memcpy(job->output, job->input, job->input_len);
                    *job->output_len = job->input_len;
                }
                break;
                
            case CSM_JOB_RANDOM_GENERATE:
                /* 生成随机数 */
                if (job->output != NULL) {
                    for (uint32_t i = 0; i < job->output_max_len; i++) {
                        job->output[i] = (uint8_t)(i * 17 + 0xAB);
                    }
                    *job->output_len = job->output_max_len;
                }
                break;
                
            default:
                break;
        }
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
        case CSM_ALGO_DRBG_CTR:         return "DRBG-CTR";
        default:                        return "UNKNOWN";
    }
}

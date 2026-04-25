/**
 * @file csm_jobs.h
 * @brief CSM Job管理器内部头文件
 * @version 1.0
 * @date 2026-04-25
 *
 * Job队列管理和处理逻辑
 */

#ifndef CSM_JOBS_H
#define CSM_JOBS_H

#include "csm_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Job队列操作
 * ============================================================================ */

/**
 * @brief 初始化Job池
 * @param ctx CSM上下文
 */
void csm_job_pool_init(csm_context_t *ctx);

/**
 * @brief 从池中分配Job
 * @param ctx CSM上下文
 * @return Job指针，NULL表示失败
 */
csm_job_t* csm_job_alloc(csm_context_t *ctx);

/**
 * @brief 释放Job回池
 * @param ctx CSM上下文
 * @param job Job指针
 */
void csm_job_free(csm_context_t *ctx, csm_job_t *job);

/**
 * @brief 通过ID查找Job
 * @param ctx CSM上下文
 * @param job_id Job ID
 * @return Job指针，NULL表示未找到
 */
csm_job_t* csm_job_find_by_id(csm_context_t *ctx, uint32_t job_id);

/**
 * @brief 将Job插入队列
 * @param ctx CSM上下文
 * @param job Job指针
 * @param priority 优先级
 * @return CSM_OK成功
 */
csm_status_t csm_queue_insert(csm_context_t *ctx, csm_job_t *job,
                              csm_job_priority_t priority);

/**
 * @brief 从队列中移除Job
 * @param ctx CSM上下文
 * @param job Job指针
 * @return CSM_OK成功
 */
csm_status_t csm_queue_remove(csm_context_t *ctx, csm_job_t *job);

/**
 * @brief 获取下一个要处理的Job
 * @param ctx CSM上下文
 * @return Job指针，NULL表示队列为空
 */
csm_job_t* csm_queue_peek(csm_context_t *ctx);

/**
 * @brief 执行单个Job
 * @param ctx CSM上下文
 * @param job Job指针
 * @return CSM_OK成功
 */
csm_status_t csm_job_execute(csm_context_t *ctx, csm_job_t *job);

/**
 * @brief 完成Job处理
 * @param ctx CSM上下文
 * @param job Job指针
 * @param result 执行结果
 */
void csm_job_complete(csm_context_t *ctx, csm_job_t *job, csm_status_t result);

/**
 * @brief 触发Job回调
 * @param ctx CSM上下文
 * @param job Job指针
 */
void csm_job_trigger_callbacks(csm_context_t *ctx, csm_job_t *job);

#ifdef __cplusplus
}
#endif

#endif /* CSM_JOBS_H */

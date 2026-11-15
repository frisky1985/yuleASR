/*******************************************************************************
 * @file dds_log_expose.c
 * @brief 将 dds_log.c 中的 static 内部状态暴露给测试代码
 *
 * 通过 #define static 为空使 g_log_state 等内部符号可见，
 * 并提供 setter/getter 函数让测试安全地操作内部状态。
 ******************************************************************************/

#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define static
#include "../dds_log.c"
#undef static

/* ---- 测试辅助 API ---- */

void dds_log_test_set_audit_mask(uint64_t mask) { g_log_state.audit_mask = mask; }
uint64_t dds_log_test_get_audit_mask(void) { return g_log_state.audit_mask; }

/* 直接调 rotate_log_file — enable_rotation=false 时覆盖行 401 早退 */
void dds_log_test_call_rotate_log_file(void) { (void)rotate_log_file(); }

/* write_to_file fputs EOF — 手动塞已关闭的 FILE* (行 421) */
void dds_log_test_trigger_fputs_eof(void) {
    FILE* closed_fp = fopen("/tmp/yule_closed_fp_test.log", "w");
    if (closed_fp != NULL) { (void)fclose(closed_fp); }
    pthread_mutex_lock(&g_log_state.mutex);
    g_log_state.log_fp = closed_fp;
    pthread_mutex_unlock(&g_log_state.mutex);
    (void)write_to_file("EOF test line\n");
    pthread_mutex_lock(&g_log_state.mutex);
    g_log_state.log_fp = NULL;
    pthread_mutex_unlock(&g_log_state.mutex);
    (void)remove("/tmp/yule_closed_fp_test.log");
}

/*******************************************************************************
 * 填充满静态池 bitmap + 手动调 queue_push, 覆盖 !node return -1 (行 252-253)
 * 要求: queue 已 init. 手动把 s_queue_node_used 全部置 1,
 * 然后 queue_push 里 for 循环找不到 free slot → node=NULL → return -1
 * 注意: 之后 queue_destroy 会尝试回收不存在的节点... 所以调用后手动清 bitmap 或 force_cleanup 处理
 ******************************************************************************/
void dds_log_test_fill_static_pool_and_push(void) {
    /* 把所有 bitmap 置 1 — 模拟静态池完全耗尽 */
    memset(s_queue_node_used, 0xFF, sizeof(s_queue_node_used));

    /* 手动调 queue_push (static 已去掉), queue 应该已有 capacity 检查通过 (capacity > size) */
    dds_log_entry_t entry;
    queue_push(&g_log_state.queue, &entry);  /* for 循环找不到 free slot → !node → return -1 (行 252-253) */

    /* 恢复 bitmap — 否则 force_cleanup 里 queue_destroy 处理节点时会误判 */
    memset(s_queue_node_used, 0, sizeof(s_queue_node_used));
}

/*******************************************************************************
 * 触发 queue_pop timeout.tv_nsec overflow (行 281-283)
 * 直接调 queue_pop (static 已去掉), 让它在空队列 + shutdown=false 上执行:
 *   while (size==0 && !shutdown) {
 *       clock_gettime → timeout.tv_nsec += 100ms → overflow 检查  ← 行 281-283
 *       pthread_cond_timedwait
 *   }
 * 关键: 必须让 while 只循环一次就退出 (否则 queue_pop 会无限 cond_wait).
 * 方案: 开一个小线程, 50ms 后设 shutdown=true + broadcast, 让 queue_pop 跳出.
 ******************************************************************************/
static void* _dds_shutdown_after(void* arg) {
    log_queue_t* q = (log_queue_t*)arg;
    usleep(50 * 1000);  /* 50ms — 足够 queue_pop 进入 while + 执行 overflow 检查 */
    pthread_mutex_lock(&q->mutex);
    q->shutdown = true;
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);
    return NULL;
}

void dds_log_test_trigger_queue_pop_timeout_overflow(void) {
    pthread_mutex_lock(&g_log_state.queue.mutex);
    g_log_state.queue.shutdown = false;
    g_log_state.queue.size = 0;
    g_log_state.queue.head = NULL;
    g_log_state.queue.tail = NULL;
    pthread_mutex_unlock(&g_log_state.queue.mutex);

    /* 启动 watchdog 线程, 50ms 后 shutdown=true + broadcast */
    pthread_t t;
    pthread_create(&t, NULL, _dds_shutdown_after, &g_log_state.queue);

    /* 一次 queue_pop 调用: 进 while → clock_gettime + overflow 检查 → cond_wait →
     * 50ms 后被 watchdog broadcast 唤醒 → shutdown=true → 退出 while → return -1
     * overflow 行 (281-283) 在第一次 clock_gettime 后被执行 */
    dds_log_entry_t entry;
    (void)queue_pop(&g_log_state.queue, &entry);

    pthread_join(t, NULL);
}

/*******************************************************************************
 * 强制 flush_thread 走到主循环 fflush 路径 (行 540-541)
 * 原理: queue_pop 在 queue 空 + shutdown=false 时卡在内部 while 永远不 return,
 * 导致内层 while(queue_pop==0) 永远不退出, 外层 fflush 不可达.
 * 方案: 设 shutdown=true 但 flush_running 保持 true → queue_pop return -1 →
 * 内层 while 退出 → 外层 while 继续 → 走到 fflush(log_fp) → usleep
 * 然后再把 shutdown 恢复为 false (不影响后续操作).
 * 要求: enable_file=true (log_fp 有效), flush_thread 已启动.
 ******************************************************************************/
void dds_log_test_force_flush_thread_fflush(void) {
    if (!g_log_state.initialized || !g_log_state.config.enable_async) { return; }

    pthread_mutex_lock(&g_log_state.queue.mutex);
    g_log_state.queue.shutdown = true;    /* 让 queue_pop 跳出内部 while */
    pthread_cond_broadcast(&g_log_state.queue.cond_not_empty);
    pthread_mutex_unlock(&g_log_state.queue.mutex);

    /* 等 flush_thread 执行 fflush + usleep (flush_interval_ms) */
    usleep(200 * 1000);  /* 200ms — 足够 flush_thread 跑一轮主循环 */

    /* 恢复 shutdown=false, 避免影响后续 force_cleanup */
    pthread_mutex_lock(&g_log_state.queue.mutex);
    g_log_state.queue.shutdown = false;
    pthread_mutex_unlock(&g_log_state.queue.mutex);
}

/*******************************************************************************
 * 停 flush_thread — 通用辅助, SIGALRM 打断 usleep + 标准停线程序列
 ******************************************************************************/
void dds_log_test_stop_flush_thread(void) {
    if (!g_log_state.initialized || !g_log_state.config.enable_async) { return; }

    g_log_state.config.flush_interval_ms = 1U;
    g_log_state.flush_running = false;

    struct sigaction sa = {0};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    (void)raise(SIGALRM);

    pthread_mutex_lock(&g_log_state.queue.mutex);
    g_log_state.queue.shutdown = true;
    pthread_cond_broadcast(&g_log_state.queue.cond_not_empty);
    pthread_mutex_unlock(&g_log_state.queue.mutex);
    pthread_cond_broadcast(&g_log_state.queue.cond_not_full);

    pthread_join(g_log_state.flush_thread, NULL);

    sa.sa_handler = SIG_DFL;
    sigaction(SIGALRM, &sa, NULL);
}

/*******************************************************************************
 * queue_destroy while (行 325-336) — 手动塞静态池节点到 queue head, 直接调 queue_destroy
 ******************************************************************************/
void dds_log_test_queue_prepend_pending_node(void) {
    int idx = -1;
    for (int i = 0; i < DDS_LOG_QUEUE_SIZE; i++) {
        uint32_t word = (uint32_t)(i / 32);
        uint32_t bit  = (uint32_t)(i % 32);
        if ((s_queue_node_used[word] & (1U << bit)) == 0U) {
            s_queue_node_used[word] |= (1U << bit);
            idx = i;
            break;
        }
    }
    if (idx < 0) { return; }
    log_queue_node_t* node = &s_queue_node_pool[idx];
    (void)memset(node, 0, sizeof(*node));
    pthread_mutex_lock(&g_log_state.queue.mutex);
    node->next = g_log_state.queue.head;
    g_log_state.queue.head = node;
    if (g_log_state.queue.tail == NULL) { g_log_state.queue.tail = node; }
    g_log_state.queue.size++;
    pthread_mutex_unlock(&g_log_state.queue.mutex);
}

/*******************************************************************************
 * 通用 cleanup — 停 flush_thread + 完整 deinit 清理, 安全可重入
 ******************************************************************************/
void dds_log_test_force_cleanup(void) {
    if (!g_log_state.initialized) { return; }

    if (g_log_state.config.enable_async) {
        /* queue.mutex 可能已被 destroy (比如之前手动调过 queue_destroy) */
        if (g_log_state.queue.mutex_initialized) {
            dds_log_test_stop_flush_thread();
            queue_destroy(&g_log_state.queue);
        }
        g_log_state.config.enable_async = false;
    }

    pthread_mutex_lock(&g_log_state.mutex);
    if ((g_log_state.log_fp) != 0U) {
        (void)fflush(g_log_state.log_fp);
        (void)fclose(g_log_state.log_fp);
        g_log_state.log_fp = NULL;
    }
    pthread_mutex_unlock(&g_log_state.mutex);

    while ((g_log_state.output_callbacks) != 0U) {
        callback_node_t* node = g_log_state.output_callbacks;
        g_log_state.output_callbacks = node->next;
        uintptr_t base = (uintptr_t)s_cb_node_pool;
        uintptr_t p = (uintptr_t)node;
        if ((p >= base) && (p < (base + (uintptr_t)DDS_LOG_MAX_CALLBACK_NODES * sizeof(callback_node_t)))) {
            uint32_t idx = (uint32_t)((p - base) / (uintptr_t)sizeof(callback_node_t));
            if (idx < DDS_LOG_MAX_CALLBACK_NODES) {
                s_cb_node_used[idx / 32U] &= ~(1U << (idx % 32U));
            }
        }
    }
    while ((g_log_state.audit_callbacks) != 0U) {
        callback_node_t* node = g_log_state.audit_callbacks;
        g_log_state.audit_callbacks = node->next;
        uintptr_t base = (uintptr_t)s_cb_node_pool;
        uintptr_t p = (uintptr_t)node;
        if ((p >= base) && (p < (base + (uintptr_t)DDS_LOG_MAX_CALLBACK_NODES * sizeof(callback_node_t)))) {
            uint32_t idx = (uint32_t)((p - base) / (uintptr_t)sizeof(callback_node_t));
            if (idx < DDS_LOG_MAX_CALLBACK_NODES) {
                s_cb_node_used[idx / 32U] &= ~(1U << (idx % 32U));
            }
        }
    }
    while ((g_log_state.module_levels) != 0U) {
        module_level_node_t* node = g_log_state.module_levels;
        g_log_state.module_levels = node->next;
        uintptr_t base = (uintptr_t)s_module_node_pool;
        uintptr_t p = (uintptr_t)node;
        if ((p >= base) && (p < (base + (uintptr_t)DDS_LOG_MAX_MODULE_NODES * sizeof(module_level_node_t)))) {
            uint32_t idx = (uint32_t)((p - base) / (uintptr_t)sizeof(module_level_node_t));
            if (idx < DDS_LOG_MAX_MODULE_NODES) {
                s_module_node_used[idx / 32U] &= ~(1U << (idx % 32U));
            }
        }
    }

    pthread_mutex_destroy(&g_log_state.mutex);
    pthread_rwlock_destroy(&g_log_state.rwlock);
    pthread_mutex_destroy(&g_log_state.stats_mutex);
    pthread_mutex_destroy(&g_log_state.uds_mutex);

    g_log_state.initialized = false;
}

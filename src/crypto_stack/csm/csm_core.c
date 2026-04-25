/**
 * @file csm_core.c
 * @brief CSM (Crypto Services Manager) - Stub Implementation
 * @version 0.1
 * @date 2026-04-25
 *
 * CSM核心模块存根 - 待完整实现
 */

#include <stdint.h>
#include <stdbool.h>

/* CSM存根头文件，用于编译通过 */
typedef enum {
    CSM_JOB_ENCRYPT = 0,
    CSM_JOB_DECRYPT,
    CSM_JOB_MAC_GENERATE,
    CSM_JOB_MAC_VERIFY,
    CSM_JOB_SIGNATURE_GENERATE,
    CSM_JOB_SIGNATURE_VERIFY,
    CSM_JOB_HASH,
    CSM_JOB_RANDOM_GENERATE
} csm_job_type_t;

/* 存根函数 - 返回成功 */
int csm_init(void) { return 0; }
void csm_deinit(void) {}

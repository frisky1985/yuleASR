/**
 * @file bl_rollback_storage.h
 * @brief 抗回滚存储服务接口 (跨层抽象, RS-OTA-01)
 * @version 1.0
 * @date 2026-08-12
 *
 * 分层约束下的接口抽象 (方案 C):
 * - bsw/boot 层 (Boot_Update) 是独立静态库, 不依赖 bootloader 层
 *   (无 bl_antrollback 引用, 交叉构建时 bootloader 层整体排除)
 * - bootloader 层 (bl_antrollback) 实现本接口 = 抗回滚计数器的
 *   单一 NVM 事实源 (单调递增 + 磨损均衡 + CRC + 延后递增)
 * - 集成层 (SBL main) 在初始化时把 bl_antrollback 实现注入给
 *   Boot_Update (Boot_Update_SetAntiRollbackStorage), 由此:
 *   · Boot_Update 通过注入的回调访问抗回滚存储, 不产生层间编译/链接依赖
 *   · BIB 不再重复存抗回滚计数器 (计数器唯一来源 = bl_antrollback NVM);
 *     BIB 仅做版本管理 (sbl_version/app_version/status/boot_count)
 *
 * 本文件为纯接口头 (仅依赖 stdint/stdbool), 位于共享 include 树
 * (include/autosar), 宿主与交叉构建均可见, 不属于任何具体层。
 *
 * 错误码数值与 bl_antrollback 错误码 1:1 对齐 (见 bl_antrollback.c
 * 编译期静态断言); 适配层负责显式映射, 不依赖数值巧合。
 *
 * UNECE R156 / GB 44496-2024 对齐, ASIL-D Safety Level
 */

#ifndef BL_ROLLBACK_STORAGE_H
#define BL_ROLLBACK_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 错误码定义 (与 bl_antrollback 错误码 1:1 对齐)
 * ============================================================================ */
typedef enum {
    BL_ROLLBACK_STORAGE_OK = 0,                  /* 成功 */
    BL_ROLLBACK_STORAGE_ERROR_INVALID_PARAM = -1,/* 非法参数 */
    BL_ROLLBACK_STORAGE_ERROR_NOT_INITIALIZED = -2, /* 未初始化 */
    BL_ROLLBACK_STORAGE_ERROR_STORAGE = -3,      /* NVM 读写/擦除失败 */
    BL_ROLLBACK_STORAGE_ERROR_DECREMENT_ATTEMPT = -4, /* 回退写入/版本低于地板 */
    BL_ROLLBACK_STORAGE_ERROR_INVALID_RECORD = -5, /* NVM 记录损坏 */
    BL_ROLLBACK_STORAGE_ERROR_COUNTER_FULL = -6  /* 计数器达上限 */
} bl_rollback_storage_error_t;

/* ============================================================================
 * 抗回滚存储服务接口 (函数表)
 * ============================================================================
 * 实现方 (bl_antrollback): Boot_AntiRollback_GetStorageApi() 返回静态表,
 * ctx 为 bl_antrollback_context_t*。消费方 (Boot_Update) 只经此表访问,
 * 不感知具体实现。
 */
typedef struct bl_rollback_storage_api_s {
    /**
     * @brief 读取当前已确认计数器值 (0 = 抗回滚禁用语义)
     * @param ctx 存储上下文 (实现方私有)
     * @param counter 输出计数器值
     */
    bl_rollback_storage_error_t (*read_counter)(void *ctx, uint32_t *counter);

    /**
     * @brief 写入计数器 (单调递增; 低于当前值拒绝)
     * @param ctx 存储上下文
     * @param counter 新计数器值
     */
    bl_rollback_storage_error_t (*write_counter)(void *ctx, uint32_t counter);

    /**
     * @brief 计数器递增 (当前值 + 1)
     * @param ctx 存储上下文
     * @param new_counter 输出递增后的值 (可为 NULL)
     */
    bl_rollback_storage_error_t (*increment)(void *ctx, uint32_t *new_counter);

    /**
     * @brief 设置延后递增阈值 N (0 = 实现默认)
     * @param ctx 存储上下文
     * @param boots 成功启动次数阈值
     */
    bl_rollback_storage_error_t (*set_confirm_boots)(void *ctx, uint32_t boots);

    /**
     * @brief 延后递增-阶段 1: 记录待确认升级版本 (不提升计数器)
     * @details 仅当 version > 当前计数器时接受; 版本 <= 计数器 →
     *          BL_ROLLBACK_STORAGE_ERROR_DECREMENT_ATTEMPT (防回滚攻击)。
     * @param ctx 存储上下文
     * @param version 新固件版本号
     */
    bl_rollback_storage_error_t (*stage)(void *ctx, uint32_t version);

    /**
     * @brief 延后递增-阶段 2: 上报一次成功启动
     * @details 仅当 current_version == 待确认版本时计数; 达到阈值 N 后
     *          提交计数器 (counter = pending) 并清除待确认状态。
     * @param ctx 存储上下文
     * @param current_version 本次成功启动的固件版本
     */
    bl_rollback_storage_error_t (*notify_successful_boot)(void *ctx,
                                                          uint32_t current_version);

    /**
     * @brief 查询待确认升级状态
     * @param ctx 存储上下文
     * @param version 输出待确认版本 (0 = 无)
     * @param count 输出已成功启动次数 (可为 NULL)
     */
    bl_rollback_storage_error_t (*get_pending)(void *ctx,
                                               uint32_t *version,
                                               uint32_t *count);
} bl_rollback_storage_api_t;

#ifdef __cplusplus
}
#endif

#endif /* BL_ROLLBACK_STORAGE_H */

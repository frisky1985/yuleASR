#ifndef BOOT_UPDATE_H
#define BOOT_UPDATE_H

#include "Boot_Types.h"
#include "bl_rollback_storage.h"
#include <stdint.h>

/*==================================================================================================
*                                    USER CONFIRM CONFIGURATION (RS-OTA-02)
*==================================================================================================
* 用户告知/确认流程 (GB 44496-2024 §6.3):
* - BOOT_USER_CONFIRM_REQUIRED: 1=升级前必须用户确认 (默认); 0=关闭确认门控
* - BOOT_USER_CONFIRM_TIMEOUT_MS: 确认等待超时 (默认 30000ms), 超时自动取消升级
* 上述宏可由 Configurator 在 Boot_Cfg.h 中覆盖 (保持 #ifndef 可重定义)。
*/
#ifndef BOOT_USER_CONFIRM_REQUIRED
#define BOOT_USER_CONFIRM_REQUIRED   (1U)
#endif

#ifndef BOOT_USER_CONFIRM_TIMEOUT_MS
#define BOOT_USER_CONFIRM_TIMEOUT_MS (30000U)
#endif

/*==================================================================================================
*                              抗回滚延后递增配置 (RS-OTA-01 / P1-4)
*==================================================================================================
* 延后递增策略: 新版本安装后不立即提升抗回滚计数器, 待该版本成功启动
* BOOT_ROLLBACK_CONFIRM_BOOTS 次后才提交 (counter = 新版本)。
* 确认窗口内允许: 回滚到旧版本 (回滚目标 < 计数器不成立) / 同版本重装。
* N=1 即退化为立即递增 (不推荐)。宏可由 Configurator 在 Boot_Cfg.h 覆盖。
*/
#ifndef BOOT_ROLLBACK_CONFIRM_BOOTS
#define BOOT_ROLLBACK_CONFIRM_BOOTS (3U)
#endif

/* 用户确认状态机 */
typedef enum {
    BOOT_CONFIRM_IDLE     = 0x00U,  /* 无确认请求 */
    BOOT_CONFIRM_PENDING  = 0x01U,  /* 等待用户确认 */
    BOOT_CONFIRM_GRANTED  = 0x02U,  /* 用户已确认 */
    BOOT_CONFIRM_DENIED   = 0x03U,  /* 用户拒绝 */
    BOOT_CONFIRM_TIMEOUT  = 0x04U   /* 确认超时 (自动取消) */
} Boot_ConfirmState;

Boot_Result Boot_Update_Prepare(uint32_t slot_addr, Boot_ImageType image_type);
Boot_Result Boot_Update_WriteBlock(const uint8_t *data,
                                   uint32_t       offset,
                                   uint32_t       length);
Boot_Result Boot_Update_Finalize(Boot_ImageType image_type, uint32_t version);
Boot_Result Boot_Update_Abort(void);
Boot_Result Boot_Update_SwapSlots(void);

/*==================================================================================================
*                                    USER CONFIRM API (RS-OTA-02)
*==================================================================================================*/

/**
 * @brief 升级前请求用户确认 (告知后等待确认)
 * @details 应在 Prepare 之前调用。设置确认状态为 PENDING 并启动超时计时;
 *          超时 (BOOT_USER_CONFIRM_TIMEOUT_MS) 后自动取消, 写入操作返回 BOOT_E_TIMEOUT。
 *          GRANTED/DENIED/TIMEOUT 为终态 (防误覆盖用户决策): 重新发起确认流程
 *          需先调用 Boot_Update_Abort() 复位到 IDLE。
 *          BOOT_USER_CONFIRM_REQUIRED==0 时直接放行 (返回 BOOT_OK, 状态 GRANTED)。
 * @return BOOT_OK 成功
 */
Boot_Result Boot_Update_RequestUserConfirm(void);

/**
 * @brief 用户确认决策回调 (平台/UI 在用户响应后调用)
 * @param confirmed TRUE=同意升级; FALSE=拒绝
 * @return BOOT_OK 决策已接受; BOOT_E_PARAM 当前无待确认请求 (非 PENDING)
 */
Boot_Result Boot_Update_ConfirmUserDecision(boolean confirmed);

/**
 * @brief 获取当前确认状态 (诊断用)
 * @return 当前 Boot_ConfirmState
 */
Boot_ConfirmState Boot_Update_GetConfirmState(void);

/**
 * @brief 注册单调时间源 (ms), 用于确认超时判定
 * @param tick_fn 时间源回调; NULL 表示无时间源 (超时功能禁用, 等待显式确认)
 */
void Boot_Update_SetTimeSource(uint64_t (*tick_fn)(void));

/*==================================================================================================
*                                   抗回滚延后递增 API (RS-OTA-01 / P1-4)
*==================================================================================================*/

/**
 * @brief 上报一次新版本成功启动 (延后递增策略的提交触发器)
 * @details 仅当 current_version == BIB.pending_counter (待确认版本) 时计数;
 *          成功启动 BOOT_ROLLBACK_CONFIRM_BOOTS 次后提交抗回滚计数器
 *          (anti_rollback_counter = pending_counter) 并清除待确认状态。
 *          current_version != pending 时忽略 (不计数不清除, 兼容 A/B 槽位回退)。
 * @param current_version 本次成功启动的固件版本号
 * @return BOOT_OK 成功; BOOT_E_NOT_INIT/存储错误透传
 */
Boot_Result Boot_Update_NotifyBootSuccess(uint32_t current_version);

/**
 * @brief 读取当前已确认的抗回滚计数器 (启动验签/诊断用)
 * @param counter 输出计数器值
 * @return BOOT_OK 成功
 */
Boot_Result Boot_Update_GetRollbackCounter(uint32_t *counter);

/**
 * @brief 设置延后递增阈值 N (运行期覆盖 BOOT_ROLLBACK_CONFIRM_BOOTS, 测试/整定用)
 * @param n 成功启动次数阈值; 0 = 恢复默认 (BOOT_ROLLBACK_CONFIRM_BOOTS)
 */
void Boot_Update_SetRollbackConfirmBoots(uint32_t n);

/**
 * @brief 注册抗回滚存储服务接口 (RS-OTA-01 / 方案 C 接口抽象)
 * @details 集成层 (SBL main) 在初始化时注入 bl_antrollback 实现, 使本模块
 *          经回调访问抗回滚计数器 — 不直接依赖 bootloader 层 (分层解耦)。
 *
 *          注入模式语义 (计数器单一事实源 = 注入的 NVM 存储):
 *          - Finalize / NotifyBootSuccess / GetRollbackCounter 全部走注入接口
 *          - BIB 不再重复存抗回滚计数器 (anti_rollback_counter / pending 字段
 *            不再写入), BIB 仅做版本管理 (sbl_version/app_version/status/boot_count)
 *          - 注: 既有设备若曾用 BIB 计数器, 切换注入模式后以 NVM 存储为准
 *            (计数器从 0 或 NVM 既有值恢复; 迁移策略由集成层决定)
 *
 *          未注入 (api==NULL_PTR) 时保持旧行为: BIB 自带 pending 机制
 *          (兼容既有测试与老集成)。
 * @param api 抗回滚存储服务接口 (NULL = 恢复旧 BIB 行为)
 * @param ctx 存储实现上下文 (由 api 解释, 通常为 bl_antrollback_context_t*)
 */
void Boot_Update_SetAntiRollbackStorage(const bl_rollback_storage_api_t *api, void *ctx);

#endif /* BOOT_UPDATE_H */

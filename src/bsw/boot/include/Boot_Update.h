#ifndef BOOT_UPDATE_H
#define BOOT_UPDATE_H

#include "Boot_Types.h"
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

#endif /* BOOT_UPDATE_H */

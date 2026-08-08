/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file E2E.c
 * @brief End-to-End Protection (AUTOSAR Classic BSW Module)
 * @details 模块级初始化/反初始化实现。
 *          E2E_Init 是 AUTOSAR SWS E2E 标准 API (E2E_Init(const void* ConfigPtr)),
 *          由本 Classic 库导出; host 版 (src/autosar/e2e/e2e_protection.c) 的
 *          同名无参 E2E_Init(void) 已于 2026-08-08 更名为 E2E_Protection_Init,
 *          避免同名不同签名符号同时链接 (P2-4)。
 * @author  AutoSAR Team
 * @version 1.0.0
 */

#include "E2E.h"
#include "E2E_Cfg.h"

/*=============================================================================*
 * Module State
 *=============================================================================*/
static boolean E2E_ModuleInitialized = FALSE;

/*=============================================================================*
 * Function Implementations
 *=============================================================================*/

/**
 * @brief 初始化 E2E 模块 (AUTOSAR 标准 API)
 * @param ConfigPtr E2E 配置指针 (当前实现不依赖模块级配置)
 * @return E_OK 初始化成功; E2E_E_INPUTERR_NULL 配置指针为空
 */
Std_ReturnType E2E_Init(const void* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        return E2E_E_INPUTERR_NULL;
    }

    E2E_ModuleInitialized = TRUE;

    return E_OK;
}

/**
 * @brief 反初始化 E2E 模块
 * @return E_OK 始终成功
 */
Std_ReturnType E2E_DeInit(void)
{
    E2E_ModuleInitialized = FALSE;

    return E_OK;
}

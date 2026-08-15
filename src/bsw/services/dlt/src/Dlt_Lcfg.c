/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
* Module               : DLT (Diagnostic Log and Trace)
* File                 : Dlt_Lcfg.c — 链接期配置（Link-time Configuration）
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
* 说明: 按 AUTOSAR SWS_Dlt 标准三层配置结构，链接期配置表独立成文件。
*       本文件由 services/dlt 原 Dlt.c 内联配置迁移而来（2026-08-15 治理），
*       删除 ecual/dlt 重复模块后归口服务层。
*================================================================================================*/

#include "Dlt.h"
#include "Dlt_Cfg.h"
#include "Dlt_Types.h"

/* ========================================================================== */
/*                          链接期配置数据定义                                 */
/* ========================================================================== */

/**
 * @brief 传输配置
 */
const Dlt_TransportConfigType Dlt_TransportConfig = {
    .protocol = DLT_TRANSPORT_PROTOCOL,
    .port = DLT_SERVER_PORT,
    .bufferSize = DLT_BUFFER_SIZE,
    .maxMessageSize = DLT_MAX_MSG_SIZE
};

/**
 * @brief 默认过滤器配置
 */
static const Dlt_FilterConfigType g_DefaultFilterConfig[] = {
    {
        .appHandle = 0U,
        .messageType = DLT_MSG_TYPE_LOG,
        .minLogLevel = DLT_DEFAULT_LOG_LEVEL,
        .enabled = DLT_DEFAULT_ENABLED
    }
};

/**
 * @brief 过滤器配置数量
 */
const uint16 Dlt_FilterConfigCount = 1U;

/**
 * @brief 过滤器配置表
 */
const Dlt_FilterConfigType* Dlt_FilterConfigTable = g_DefaultFilterConfig;

/**
 * @brief 模块配置
 */
const Dlt_ConfigType Dlt_Config = {
    .transportConfig = &Dlt_TransportConfig,
    .filterConfig = g_DefaultFilterConfig,
    .filterCount = Dlt_FilterConfigCount,
    .queueSize = DLT_QUEUE_SIZE
};

/*==================[end of file]===========================================*/

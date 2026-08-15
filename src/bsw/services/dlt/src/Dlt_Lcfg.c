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
*       2026-08-15 治理:
*         - services/dlt 原 Dlt.c 内联配置迁移至此 (TransportConfig/Config/过滤器表)
*         - ecual/dlt 删除后, 其 context 配置体系 (Dlt_ContextConfig/Dlt_RuntimeContext/
*           Dlt_Buffer/DefaultLogLevels/DefaultTraceStatus/EcuId/ProtocolVersion/
*           ContextGroups/LogFilters/BufferTimeout/MainFunctionPeriod/ComPduId)
*           完整并入本文件, 实现零能力损失。
*       类型兼容说明: 表值采用 services 版 Dlt_Types.h/Dlt_Cfg.h 语义
*         (Dlt_ContextIdType 已加宽 uint32 对齐 ecual 打包 ASCII; DLT_LOG_OFF 为宏扩展)。
*================================================================================================*/

#include "Dlt.h"
#include "Dlt_Cfg.h"
#include "Dlt_Types.h"
#include <string.h>

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

/* ========================================================================== */
/*              Context 配置体系 (由 ecual/dlt 合并, 2026-08-15)              */
/* ========================================================================== */

/**
 * @brief Context 配置表 (32 个预配置 context)
 *
 * @note 外部链接 (非 static): Dlt.c 在 Dlt_Init 时将其复制到 Dlt_RuntimeContext。
 *       appId/contextId 为 4 字节打包 ASCII (如 0x44454641 == "DEFA")。
 */
const Dlt_ContextType Dlt_ContextConfig[DLT_MAX_CONTEXT_COUNT] = {
    /* Context 0: Default/Application Context */
    {
        .appId = 0x44454641,        /* "DEFA" - Default Application */
        .contextId = 0x434D444C,    /* "CMDL" - Command Logger */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "DefaultContext",
        .registered = TRUE
    },
    /* Context 1: ECU Manager */
    {
        .appId = 0x4543554D,        /* "ECUM" - ECU Manager */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_DEBUG,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "EcuM_Main",
        .registered = TRUE
    },
    /* Context 2: OS Integration */
    {
        .appId = 0x4F535F5F,        /* "OS__" - OS */
        .contextId = 0x4B45524E,    /* "KERN" - Kernel */
        .logLevel = DLT_LOG_WARN,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "OS_Kernel",
        .registered = TRUE
    },
    /* Context 3: Diagnostic */
    {
        .appId = 0x44494147,        /* "DIAG" - Diagnostic */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Diag_Main",
        .registered = TRUE
    },
    /* Context 4: Communication */
    {
        .appId = 0x434F4D4D,        /* "COMM" - Communication */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Com_Main",
        .registered = TRUE
    },
    /* Context 5: Network Management */
    {
        .appId = 0x4E4D5F5F,        /* "NM__" - Network Management */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Nm_Main",
        .registered = TRUE
    },
    /* Context 6: Bootloader */
    {
        .appId = 0x424F4F54,        /* "BOOT" - Bootloader */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_DEBUG,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Boot_Main",
        .registered = TRUE
    },
    /* Context 7: Memory Stack */
    {
        .appId = 0x4D454D53,        /* "MEMS" - Memory Stack */
        .contextId = 0x4E564D5F,    /* "NVM_" - NVRAM Manager */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "NvM_Manager",
        .registered = TRUE
    },
    /* Context 8: RTE */
    {
        .appId = 0x5254455F,        /* "RTE_" - RTE */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_DEBUG,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Rte_Main",
        .registered = TRUE
    },
    /* Context 9: BSW Manager */
    {
        .appId = 0x4253574D,        /* "BSWM" - BSW Manager */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "BswM_Main",
        .registered = TRUE
    },
    /* Context 10: XCP */
    {
        .appId = 0x5843505F,        /* "XCP_" - XCP Protocol */
        .contextId = 0x50524F54,    /* "PROT" - Protocol */
        .logLevel = DLT_LOG_DEBUG,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Xcp_Protocol",
        .registered = TRUE
    },
    /* Context 11: CDD 1 */
    {
        .appId = 0x43444431,        /* "CDD1" - Complex Driver 1 */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Cdd1_Main",
        .registered = TRUE
    },
    /* Context 12: CDD 2 */
    {
        .appId = 0x43444432,        /* "CDD2" - Complex Driver 2 */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Cdd2_Main",
        .registered = TRUE
    },
    /* Context 13: Watchdog */
    {
        .appId = 0x5744475F,        /* "WDG_" - Watchdog */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_ERROR,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Wdg_Main",
        .registered = TRUE
    },
    /* Context 14: MCU Driver */
    {
        .appId = 0x4D43555F,        /* "MCU_" - MCU Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Mcu_Main",
        .registered = TRUE
    },
    /* Context 15: PORT Driver */
    {
        .appId = 0x504F5254,        /* "PORT" - PORT Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Port_Main",
        .registered = TRUE
    },
    /* Context 16: DIO Driver */
    {
        .appId = 0x44494F5F,        /* "DIO_" - DIO Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Dio_Main",
        .registered = TRUE
    },
    /* Context 17: PWM Driver */
    {
        .appId = 0x50574D5F,        /* "PWM_" - PWM Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Pwm_Main",
        .registered = TRUE
    },
    /* Context 18: ICU Driver */
    {
        .appId = 0x4943555F,        /* "ICU_" - ICU Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Icu_Main",
        .registered = TRUE
    },
    /* Context 19: ADC Driver */
    {
        .appId = 0x4144435F,        /* "ADC_" - ADC Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Adc_Main",
        .registered = TRUE
    },
    /* Context 20: SPI Driver */
    {
        .appId = 0x5350495F,        /* "SPI_" - SPI Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Spi_Main",
        .registered = TRUE
    },
    /* Context 21: CAN Driver */
    {
        .appId = 0x43414E5F,        /* "CAN_" - CAN Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Can_Main",
        .registered = TRUE
    },
    /* Context 22: LIN Driver */
    {
        .appId = 0x4C494E5F,        /* "LIN_" - LIN Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Lin_Main",
        .registered = TRUE
    },
    /* Context 23: FlexRay Driver */
    {
        .appId = 0x4652525F,        /* "FRR_" - FlexRay Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Fr_Main",
        .registered = TRUE
    },
    /* Context 24: Ethernet Driver */
    {
        .appId = 0x4554485F,        /* "ETH_" - Ethernet Driver */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Eth_Main",
        .registered = TRUE
    },
    /* Context 25: System Services */
    {
        .appId = 0x5359535F,        /* "SYS_" - System Services */
        .contextId = 0x53454355,    /* "SECU" - Security */
        .logLevel = DLT_LOG_WARN,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "System_Security",
        .registered = TRUE
    },
    /* Context 26: Mode Management */
    {
        .appId = 0x4D4F4445,        /* "MODE" - Mode Management */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Mode_Main",
        .registered = TRUE
    },
    /* Context 27: Safety */
    {
        .appId = 0x53414645,        /* "SAFE" - Safety */
        .contextId = 0x4D4F4E49,    /* "MONI" - Monitor */
        .logLevel = DLT_LOG_ERROR,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Safety_Monitor",
        .registered = TRUE
    },
    /* Context 28: Crypto Services */
    {
        .appId = 0x43525950,        /* "CRYP" - Crypto */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Crypto_Main",
        .registered = TRUE
    },
    /* Context 29: Synchronization */
    {
        .appId = 0x53544E43,        /* "STNC" - Synchronization */
        .contextId = 0x54534E43,    /* "TSNC" - Time Sync */
        .logLevel = DLT_LOG_INFO,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "Time_Sync",
        .registered = TRUE
    },
    /* Context 30: User Application 1 */
    {
        .appId = 0x41505031,        /* "APP1" - Application 1 */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_VERBOSE,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "App1_Main",
        .registered = TRUE
    },
    /* Context 31: User Application 2 */
    {
        .appId = 0x41505032,        /* "APP2" - Application 2 */
        .contextId = 0x4D41494E,    /* "MAIN" - Main */
        .logLevel = DLT_LOG_VERBOSE,
        .traceStatus = DLT_TRACE_STATUS_ON,
        .description = "App2_Main",
        .registered = TRUE
    }
};

/**
 * @brief 运行时 Context 表 (Dlt_Init 时由 Dlt_ContextConfig 初始化)
 *
 * @note 外部链接 (非 static): Dlt.c 的 context API 基于此表运行时状态。
 */
Dlt_ContextType Dlt_RuntimeContext[DLT_MAX_CONTEXT_COUNT];

/**
 * @brief 缓冲区配置 (优先级 0~3)
 *
 * @note 保留自 ecual (供缓冲路径/未来接入使用); 当前 services Dlt.c 尚未消费。
 *       外部链接: 与 ecual 版 Dlt.c 的 extern 引用一致, 避免 static/extern 矛盾。
 */
Dlt_BufferType Dlt_Buffer[DLT_BUFFER_COUNT] = {
    /* Buffer 0: High Priority */
    {
        .data = {0},
        .writeIndex = 0,
        .readIndex = 0,
        .count = 0,
        .locked = FALSE
    },
    /* Buffer 1: Normal Priority */
    {
        .data = {0},
        .writeIndex = 0,
        .readIndex = 0,
        .count = 0,
        .locked = FALSE
    },
    /* Buffer 2: Low Priority */
    {
        .data = {0},
        .writeIndex = 0,
        .readIndex = 0,
        .count = 0,
        .locked = FALSE
    },
    /* Buffer 3: Control Messages */
    {
        .data = {0},
        .writeIndex = 0,
        .readIndex = 0,
        .count = 0,
        .locked = FALSE
    }
};

/**
 * @brief 默认日志级别配置表 (按系统状态 0~7)
 *
 * @note 状态 0 (生产模式, 不记录) 用 DLT_LOG_OFF 宏 (services 枚举外扩展,
 *       因 services 版枚举以 DLT_LOG_FATAL=0 为基线, 无 OFF 枚举成员)。
 */
const Dlt_LogLevelType Dlt_DefaultLogLevels[8] = {
    DLT_LOG_OFF,        /* State 0: Production - No logging */
    DLT_LOG_FATAL,      /* State 1: Fatal errors only */
    DLT_LOG_ERROR,      /* State 2: Errors only */
    DLT_LOG_WARN,       /* State 3: Warnings and above */
    DLT_LOG_INFO,       /* State 4: Normal operation info */
    DLT_LOG_DEBUG,      /* State 5: Debug mode */
    DLT_LOG_VERBOSE,    /* State 6: Verbose debug */
    DLT_LOG_VERBOSE     /* State 7: Maximum verbosity */
};

/**
 * @brief 默认跟踪状态配置表 (按系统状态 0~7)
 */
const Dlt_TraceStatusType Dlt_DefaultTraceStatus[8] = {
    DLT_TRACE_STATUS_OFF,   /* State 0: Production - Tracing off */
    DLT_TRACE_STATUS_OFF,   /* State 1: Tracing off */
    DLT_TRACE_STATUS_OFF,   /* State 2: Tracing off */
    DLT_TRACE_STATUS_ON,    /* State 3: Tracing on */
    DLT_TRACE_STATUS_ON,    /* State 4: Tracing on */
    DLT_TRACE_STATUS_ON,    /* State 5: Tracing on */
    DLT_TRACE_STATUS_ON,    /* State 6: Tracing on */
    DLT_TRACE_STATUS_ON     /* State 7: Tracing on */
};

/**
 * @brief ECU 配置 (逐字符初始化, 避免字符串含 NUL 触发数组过长告警)
 */
const uint8 Dlt_EcuId[DLT_ECU_ID_LENGTH] = { DLT_ECU_ID[0], DLT_ECU_ID[1], DLT_ECU_ID[2], DLT_ECU_ID[3] };

/**
 * @brief 会话配置
 */
const uint32 Dlt_DefaultSessionId = DLT_DEFAULT_SESSION_ID;

/**
 * @brief 协议版本配置
 */
const uint8 Dlt_ProtocolVersionMajor = DLT_PROTOCOL_VERSION_MAJOR;
const uint8 Dlt_ProtocolVersionMinor = DLT_PROTOCOL_VERSION_MINOR;

/**
 * @brief 缓冲区大小配置 (每缓冲区字节数)
 */
const uint16 Dlt_BufferSize[DLT_BUFFER_COUNT] = {
    DLT_BUFFER_SIZE,    /* Buffer 0 */
    DLT_BUFFER_SIZE,    /* Buffer 1 */
    DLT_BUFFER_SIZE,    /* Buffer 2 */
    DLT_BUFFER_SIZE     /* Buffer 3 */
};

/**
 * @brief 缓冲区优先级配置
 */
const uint8 Dlt_BufferPriority[DLT_BUFFER_COUNT] = {
    0,  /* Buffer 0: Highest */
    1,  /* Buffer 1: High */
    2,  /* Buffer 2: Normal */
    3   /* Buffer 3: Low (Control messages) */
};

/**
 * @brief Context 分组配置 (按 appId 分组, 用于批量操作)
 */
const Dlt_ContextGroupType Dlt_ContextGroups[] = {
    {0x44454641, 0, 1},     /* DEFA group */
    {0x4543554D, 1, 1},     /* ECUM group */
    {0x4F535F5F, 2, 1},     /* OS__ group */
    {0x44494147, 3, 1},     /* DIAG group */
    {0x434F4D4D, 4, 1},     /* COMM group */
    {0x4E4D5F5F, 5, 1},     /* NM__ group */
    {0x424F4F54, 6, 1},     /* BOOT group */
    {0x4D454D53, 7, 1},     /* MEMS group */
    {0x5254455F, 8, 1},     /* RTE_ group */
    {0x4253574D, 9, 1},     /* BSWM group */
    {0x5843505F, 10, 1},    /* XCP_ group */
    {0x43444431, 11, 1},    /* CDD1 group */
    {0x43444432, 12, 1},    /* CDD2 group */
    {0x5744475F, 13, 1},    /* WDG_ group */
    {0x4D43555F, 14, 1},    /* MCU_ group */
    {0x504F5254, 15, 1},    /* PORT group */
    {0x44494F5F, 16, 1},    /* DIO_ group */
    {0x50574D5F, 17, 1},    /* PWM_ group */
    {0x4943555F, 18, 1},    /* ICU_ group */
    {0x4144435F, 19, 1},    /* ADC_ group */
    {0x5350495F, 20, 1},    /* SPI_ group */
    {0x43414E5F, 21, 1},    /* CAN_ group */
    {0x4C494E5F, 22, 1},    /* LIN_ group */
    {0x4652525F, 23, 1},    /* FRR_ group */
    {0x4554485F, 24, 1},    /* ETH_ group */
    {0x5359535F, 25, 1},    /* SYS_ group */
    {0x4D4F4445, 26, 1},    /* MODE group */
    {0x53414645, 27, 1},    /* SAFE group */
    {0x43525950, 28, 1},    /* CRYP group */
    {0x53544E43, 29, 1},    /* STNC group */
    {0x41505031, 30, 1},    /* APP1 group */
    {0x41505032, 31, 1}     /* APP2 group */
};

/**
 * @brief Context 分组数量
 */
const uint16 Dlt_ContextGroupCount = sizeof(Dlt_ContextGroups) / sizeof(Dlt_ContextGroupType);

/**
 * @brief Com 配置 (DLT 消息经 Com 传输的 PDU ID)
 */
#if (DLT_USE_COM == STD_ON)
const uint16 Dlt_ComPduId = 0;  /* PDU ID for DLT messages via Com */
#endif

/**
 * @brief 日志过滤器表 (按 context 预配置)
 */
typedef struct {
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    Dlt_LogLevelType minLogLevel;
    boolean enabled;
} Dlt_LogFilterType;

Dlt_LogFilterType Dlt_LogFilters[DLT_MAX_CONTEXT_COUNT] = {
    {0x44454641, 0x434D444C, DLT_LOG_INFO, TRUE},
    {0x4543554D, 0x4D41494E, DLT_LOG_DEBUG, TRUE},
    {0x4F535F5F, 0x4B45524E, DLT_LOG_WARN, TRUE},
    {0x44494147, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x434F4D4D, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x4E4D5F5F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x424F4F54, 0x4D41494E, DLT_LOG_DEBUG, TRUE},
    {0x4D454D53, 0x4E564D5F, DLT_LOG_INFO, TRUE},
    {0x5254455F, 0x4D41494E, DLT_LOG_DEBUG, TRUE},
    {0x4253574D, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x5843505F, 0x50524F54, DLT_LOG_DEBUG, TRUE},
    {0x43444431, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x43444432, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x5744475F, 0x4D41494E, DLT_LOG_ERROR, TRUE},
    {0x4D43555F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x504F5254, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x44494F5F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x50574D5F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x4943555F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x4144435F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x5350495F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x43414E5F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x4C494E5F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x4652525F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x4554485F, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x5359535F, 0x53454355, DLT_LOG_WARN, TRUE},
    {0x4D4F4445, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x53414645, 0x4D4F4E49, DLT_LOG_ERROR, TRUE},
    {0x43525950, 0x4D41494E, DLT_LOG_INFO, TRUE},
    {0x53544E43, 0x54534E43, DLT_LOG_INFO, TRUE},
    {0x41505031, 0x4D41494E, DLT_LOG_VERBOSE, TRUE},
    {0x41505032, 0x4D41494E, DLT_LOG_VERBOSE, TRUE}
};

/**
 * @brief 超时与周期配置
 */
const uint32 Dlt_BufferTimeout = DLT_BUFFERING_TIMEOUT;
const uint32 Dlt_MainFunctionPeriod = DLT_MAIN_FUNCTION_PERIOD;

/* ========================================================================== */
/*                    配置表尺寸自检 (编译期, 亦防未使用告警)                  */
/* ========================================================================== */

typedef char Dlt_LcfgCheck_ContextConfig[(sizeof(Dlt_ContextConfig) / sizeof(Dlt_ContextType)) == DLT_MAX_CONTEXT_COUNT ? 1 : -1];
typedef char Dlt_LcfgCheck_Buffer[(sizeof(Dlt_Buffer) / sizeof(Dlt_BufferType)) == DLT_BUFFER_COUNT ? 1 : -1];
typedef char Dlt_LcfgCheck_DefaultLogLevels[(sizeof(Dlt_DefaultLogLevels) / sizeof(Dlt_LogLevelType)) == 8U ? 1 : -1];
typedef char Dlt_LcfgCheck_DefaultTraceStatus[(sizeof(Dlt_DefaultTraceStatus) / sizeof(Dlt_TraceStatusType)) == 8U ? 1 : -1];
typedef char Dlt_LcfgCheck_BufferSize[(sizeof(Dlt_BufferSize) / sizeof(uint16)) == DLT_BUFFER_COUNT ? 1 : -1];
typedef char Dlt_LcfgCheck_BufferPriority[(sizeof(Dlt_BufferPriority) / sizeof(uint8)) == DLT_BUFFER_COUNT ? 1 : -1];
typedef char Dlt_LcfgCheck_ContextGroups[(sizeof(Dlt_ContextGroups) / sizeof(Dlt_ContextGroupType)) == DLT_MAX_CONTEXT_COUNT ? 1 : -1];
typedef char Dlt_LcfgCheck_LogFilters[(sizeof(Dlt_LogFilters) / sizeof(Dlt_LogFilterType)) == DLT_MAX_CONTEXT_COUNT ? 1 : -1];

/*==================[end of file]===========================================*/

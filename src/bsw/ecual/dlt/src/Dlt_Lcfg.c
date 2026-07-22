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

/* Dlt_Lcfg.c - AUTOSAR Diagnostic Log and Trace Link-time Configuration */

#include "Dlt.h"
#include <string.h>

/* Context Configuration Table */
/* Pre-configured contexts for the DLT module */

extern const uint32 Dlt_MainFunctionPeriod;
extern const uint32 Dlt_BufferTimeout;
extern const uint8 Dlt_ProtocolVersionMinor;
extern const uint8 Dlt_ProtocolVersionMajor;
extern const uint32 Dlt_DefaultSessionId;
static const Dlt_ContextType Dlt_ContextConfig[DLT_MAX_CONTEXT_COUNT] = {
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

/* Runtime Context Table (initialized at startup) */
Dlt_ContextType Dlt_RuntimeContext[DLT_MAX_CONTEXT_COUNT];

/* Buffer Configuration */
static Dlt_BufferType Dlt_Buffer[DLT_BUFFER_COUNT] = {
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

/* Log Level Configuration Table */
/* Default log levels for different system states */

static const Dlt_LogLevelType Dlt_DefaultLogLevels[8] = {
    DLT_LOG_OFF,        /* State 0: Production - No logging */
    DLT_LOG_FATAL,      /* State 1: Fatal errors only */
    DLT_LOG_ERROR,      /* State 2: Errors only */
    DLT_LOG_WARN,       /* State 3: Warnings and above */
    DLT_LOG_INFO,       /* State 4: Normal operation info */
    DLT_LOG_DEBUG,      /* State 5: Debug mode */
    DLT_LOG_VERBOSE,    /* State 6: Verbose debug */
    DLT_LOG_VERBOSE     /* State 7: Maximum verbosity */
};

/* Trace Configuration Table */
/* Default trace status for different system states */

static const Dlt_TraceStatusType Dlt_DefaultTraceStatus[8] = {
    DLT_TRACE_STATUS_OFF,   /* State 0: Production - Tracing off */
    DLT_TRACE_STATUS_OFF,   /* State 1: Tracing off */
    DLT_TRACE_STATUS_OFF,   /* State 2: Tracing off */
    DLT_TRACE_STATUS_ON,    /* State 3: Tracing on */
    DLT_TRACE_STATUS_ON,    /* State 4: Tracing on */
    DLT_TRACE_STATUS_ON,    /* State 5: Tracing on */
    DLT_TRACE_STATUS_ON,    /* State 6: Tracing on */
    DLT_TRACE_STATUS_ON     /* State 7: Tracing on */
};

/* ECU Configuration */
const uint8 Dlt_EcuId[DLT_ECU_ID_LENGTH] = DLT_ECU_ID;

/* Session Configuration */
const uint32 Dlt_DefaultSessionId = DLT_DEFAULT_SESSION_ID;

/* Protocol Configuration */
const uint8 Dlt_ProtocolVersionMajor = DLT_PROTOCOL_VERSION_MAJOR;
const uint8 Dlt_ProtocolVersionMinor = DLT_PROTOCOL_VERSION_MINOR;

/* Buffer Size Configuration */
static const uint16 Dlt_BufferSize[DLT_BUFFER_COUNT] = {
    DLT_BUFFER_SIZE,    /* Buffer 0 */
    DLT_BUFFER_SIZE,    /* Buffer 1 */
    DLT_BUFFER_SIZE,    /* Buffer 2 */
    DLT_BUFFER_SIZE     /* Buffer 3 */
};

/* Buffer Priority Configuration */
static const uint8 Dlt_BufferPriority[DLT_BUFFER_COUNT] = {
    0,  /* Buffer 0: Highest */
    1,  /* Buffer 1: High */
    2,  /* Buffer 2: Normal */
    3   /* Buffer 3: Low (Control messages) */
};

/* Context Group Configuration */
/* Group contexts by application for bulk operations */

typedef struct {
    Dlt_ApplicationIdType appId;
    uint16 contextStartIndex;
    uint16 contextCount;
} Dlt_ContextGroupType;

static const Dlt_ContextGroupType Dlt_ContextGroups[] = {
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

const uint16 Dlt_ContextGroupCount = sizeof(Dlt_ContextGroups) / sizeof(Dlt_ContextGroupType);

/* Com Configuration */
#if (DLT_USE_COM == STD_ON)
const uint16 Dlt_ComPduId = 0;  /* PDU ID for DLT messages via Com */
#endif

/* Filter Configuration */
/* Pre-configured filters for log messages */

typedef struct {
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    Dlt_LogLevelType minLogLevel;
    boolean enabled;
} Dlt_LogFilterType;

static Dlt_LogFilterType Dlt_LogFilters[DLT_MAX_CONTEXT_COUNT] = {
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

/* Timeout Configuration */
const uint32 Dlt_BufferTimeout = DLT_BUFFERING_TIMEOUT;
const uint32 Dlt_MainFunctionPeriod = DLT_MAIN_FUNCTION_PERIOD;

/**
 * @file Mcu.h
 * @brief MCU Driver 公共接口头文件
 * @version 1.0.0
 * @date 2026-04-13
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details 提供微控制器初始化、时钟配置、复位管理和低功耗模式控制功能
 */

#ifndef MCU_H
#define MCU_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Compiler.h"
#include "Mcu_Cfg.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
/** @brief MCU 模块供应商 ID */
#define MCU_VENDOR_ID                           0x0055U  /* YuleTech */

/** @brief MCU 模块 ID */
#define MCU_AR_RELEASE_MAJOR_VERSION   4U
#define MCU_AR_RELEASE_MINOR_VERSION   4U
#define MCU_AR_RELEASE_REVISION_VERSION 0U
#define MCU_SW_MAJOR_VERSION           1U
#define MCU_SW_MINOR_VERSION           0U
#define MCU_SW_PATCH_VERSION           0U
#define MCU_MODULE_ID                           0x002BU  /* AUTOSAR standard: 43 */

/** @brief MCU 实例 ID */
#define MCU_INSTANCE_ID                         0x00U

/* API Service IDs */
#define MCU_API_ID_INIT                         0x00U
#define MCU_API_ID_INIT_CLOCK                   0x01U
#define MCU_API_ID_DISTRIBUTE_PLL_CLOCK         0x02U
#define MCU_API_ID_GET_PLL_STATUS               0x03U
#define MCU_API_ID_SET_MODE                     0x04U
#define MCU_API_ID_GET_RESET_REASON             0x05U
#define MCU_API_ID_GET_RESET_RAW_VALUE          0x06U
#define MCU_API_ID_PERFORM_RESET                0x07U
#define MCU_API_ID_GET_VERSION_INFO             0x08U
#define MCU_API_ID_GET_RAM_STATE                0x09U
#define MCU_API_ID_INIT_RAM_SECTION             0x0AU

/* Service ID aliases (for Det_ReportError) */
#define MCU_SID_INIT                    MCU_API_ID_INIT
#define MCU_SID_INIT_CLOCK              MCU_API_ID_INIT_CLOCK
#define MCU_SID_DISTRIBUTE_PLL_CLOCK    MCU_API_ID_DISTRIBUTE_PLL_CLOCK
#define MCU_SID_GET_PLL_STATUS          MCU_API_ID_GET_PLL_STATUS
#define MCU_SID_SET_MODE                MCU_API_ID_SET_MODE
#define MCU_SID_GET_RESET_REASON        MCU_API_ID_GET_RESET_REASON
#define MCU_SID_GET_RESET_RAW_VALUE     MCU_API_ID_GET_RESET_RAW_VALUE
#define MCU_SID_PERFORM_RESET           MCU_API_ID_PERFORM_RESET
#define MCU_SID_GET_VERSION_INFO        MCU_API_ID_GET_VERSION_INFO
#define MCU_SID_GET_RAM_STATE           MCU_API_ID_GET_RAM_STATE
#define MCU_SID_INIT_RAM_SECTION        MCU_API_ID_INIT_RAM_SECTION

/* Error Codes */
#define MCU_E_PARAM_CONFIG                      0x0AU
#define MCU_E_PARAM_CLOCK                       0x0BU
#define MCU_E_PARAM_MODE                        0x0CU
#define MCU_E_PLL_NOT_LOCKED                    0x0DU
#define MCU_E_UNINIT                            0x0EU
#define MCU_E_PARAM_POINTER                     0x0FU
#define MCU_E_INIT_FAILED                       0x10U
#define MCU_E_ALREADY_INITIALIZED               0x11U
#define MCU_E_PARAM_RAMSECTION                  0x12U

/*==================================================================================================
*                                          TYPE DEFINITIONS
==================================================================================================*/
/** @brief MCU 时钟配置类型 */
typedef uint32 Mcu_ClockType;

/** @brief MCU 复位原因原始值类型 */
typedef uint32 Mcu_RawResetType;

/** @brief MCU 模式类型 */
typedef uint8 Mcu_ModeType;

/** @brief MCU RAM 状态类型 */
typedef enum {
    MCU_RAMSTATE_INVALID = 0,         /**< RAM 状态无效 */
    MCU_RAMSTATE_VALID = 1,           /**< RAM 状态有效 */
    MCU_RAMSTATE_INITIALIZED = 2,     /**< RAM 已初始化 */
    MCU_RAMSTATE_UNINITIALIZED = 3    /**< RAM 未初始化 */
} Mcu_RamStateType;

/** @brief MCU 状态类型 */
typedef enum {
    MCU_STATE_UNINIT = 0,                 /**< 未初始化 */
    MCU_STATE_CLOCK_UNINIT,               /**< 时钟未初始化 */
    MCU_STATE_CLOCK_INITIALIZED,          /**< 时钟已初始化 */
    MCU_STATE_NORMAL,                /**< 正常模式 */
    MCU_STATE_SLEEP,                 /**< 睡眠模式 */
    MCU_STATE_DEEP_SLEEP             /**< 深度睡眠模式 */
} Mcu_StateType;

/** @brief PLL 状态类型 */
typedef enum {
    MCU_PLL_STATUS_UNDEFINED = 0,   /**< 未定义 */
    MCU_PLL_STATUS_LOCKED,          /**< 已锁定 */
    MCU_PLL_STATUS_UNLOCKED         /**< 未锁定 */
} Mcu_PllStatusType;

/** @brief 复位原因类型 */
typedef enum {
    MCU_RST_UNDEFINED = 0,        /**< 未定义 */
    MCU_RST_POWER_ON,             /**< 上电复位 */
    MCU_RST_WATCHDOG,             /**< 看门狗复位 */
    MCU_RST_SOFTWARE,             /**< 软件复位 */
    MCU_RST_EXTERNAL,             /**< 外部复位 */
    MCU_RST_BROWN_OUT,            /**< 欠压复位 */
    MCU_RST_LOCKUP                /**< 锁死复位 */
} Mcu_ResetType;

/** @brief PLL 配置类型 */
typedef struct {
    uint32 PllBaseAddr;             /**< PLL 基地址 */
    uint32 Prediv;                  /**< 预分频器 */
    uint32 Multiplier;              /**< 倍频系数 */
    uint32 Postdiv1;                /**< 后分频器 1 */
    uint32 Postdiv2;                /**< 后分频器 2 */
    boolean Enable;                 /**< PLL 使能 */
} Mcu_PllConfigType;

/** @brief RAM 段配置类型 */
typedef struct {
    uint32 RamBaseAddr;             /**< RAM 段基地址 */
    uint32 RamSize;                 /**< RAM 段大小 (bytes) */
    uint8  RamDefaultValue;         /**< RAM 默认填充值 */
} Mcu_RamSectionType;

/** @brief MCU 模式配置类型 */
typedef struct {
    Mcu_ModeType Mode;              /**< 模式值 */
} Mcu_ModeConfigType;

/** @brief 时钟配置类型 */
typedef struct {
    uint32 PllBaseAddr;             /**< PLL 基地址 */
    const Mcu_PllConfigType* PllConfigs;  /**< PLL 配置数组 */
    uint8 NumPllConfigs;            /**< PLL 配置数量 */
    uint8 ClockSource;              /**< 时钟源选择 */
    uint32 ArmDiv;                  /**< ARM 时钟分频系数 */
    uint32 AxiDiv;                  /**< AXI 时钟分频系数 */
    uint32 AhbDiv;                  /**< AHB 时钟分频系数 */
} Mcu_ClockConfigType;

/** @brief MCU 配置类型 */
typedef struct {
    Mcu_ClockType ClockSetting;     /**< 时钟配置设置 */
    uint32 ClockFrequency;          /**< 目标时钟频率 (Hz) */
    uint32 PllMultiplier;           /**< PLL 倍频系数 */
    uint32 PllDivider;              /**< PLL 分频系数 */
    boolean PllEnabled;             /**< PLL 使能标志 */
    const Mcu_RamSectionType* RamSections;   /**< RAM 段配置数组 */
    uint8 NumRamSections;           /**< RAM 段数量 */
    const Mcu_ClockConfigType* ClockConfigs; /**< 时钟配置数组 */
    uint8 NumClockConfigs;          /**< 时钟配置数量 */
    const Mcu_ModeConfigType* ModeConfigs;   /**< 模式配置数组 */
    uint8 NumModes;                 /**< 模式数量 */
} Mcu_ConfigType;

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
#define MCU_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/** @brief 外部配置结构体 */
extern const Mcu_ConfigType Mcu_Config;

#define MCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
#define MCU_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief 初始化 MCU 模块
 *
 * @param[in] ConfigPtr 指向 MCU 配置结构的指针
 * @return Std_ReturnType
 *         - E_OK: 初始化成功
 *         - E_NOT_OK: 初始化失败
 *
 * @pre 模块处于 MCU_STATE_UNINIT 状态
 * @post 模块处于 MCU_STATE_CLOCK_UNINIT 状态
 *
 * @note 必须在其他模块初始化之前调用
 * @note 如果 ConfigPtr 为 NULL_PTR，使用默认配置
 */
Std_ReturnType Mcu_Init(const Mcu_ConfigType* ConfigPtr);

/**
 * @brief 初始化时钟系统
 *
 * @param[in] ClockSetting 时钟配置设置
 * @return Std_ReturnType
 *         - E_OK: 时钟初始化成功
 *         - E_NOT_OK: 时钟初始化失败
 *
 * @pre MCU 模块已初始化
 * @post 时钟系统已配置
 *
 * @note 此函数会等待 PLL 锁定
 */
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting);

/**
 * @brief 分发 PLL 时钟到系统
 *
 * @return Std_ReturnType
 *         - E_OK: 分发成功
 *         - E_NOT_OK: PLL 未锁定
 *
 * @pre PLL 已锁定
 * @post 系统时钟已切换到 PLL
 */
void Mcu_DistributePllClock(void);

/**
 * @brief 获取 PLL 状态
 *
 * @return Mcu_PllStatusType PLL 当前状态
 */
Mcu_PllStatusType Mcu_GetPllStatus(void);

/**
 * @brief 设置 MCU 模式
 *
 * @param[in] McuMode 目标模式
 *
 * @pre 时钟系统已初始化
 * @post MCU 进入指定模式
 *
 * @note 支持正常模式、睡眠模式、深度睡眠模式
 */
void Mcu_SetMode(Mcu_ModeType McuMode);

/**
 * @brief 获取复位原因
 *
 * @return Mcu_ResetType 复位原因
 *
 * @note 读取后复位原因寄存器会被清除
 */
Mcu_ResetType Mcu_GetResetReason(void);

/**
 * @brief 获取复位原因原始值
 *
 * @return Mcu_RawResetType 复位寄存器原始值
 */
Mcu_RawResetType Mcu_GetResetRawValue(void);

/**
 * @brief 执行系统复位
 *
 * @pre 系统处于安全状态
 * @post 系统复位重启
 *
 * @warning 此函数不会返回
 */
void Mcu_PerformReset(void);

/**
 * @brief 获取版本信息
 *
 * @param[out] versioninfo 指向版本信息结构的指针
 *
 * @note 如果 versioninfo 为 NULL_PTR，报告开发错误
 */
void Mcu_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief 获取 RAM 状态
 *
 * @return Mcu_RamStateType RAM 当前状态
 *
 * @pre MCU 模块已初始化
 * @post 返回 RAM 有效/无效/初始化/未初始化状态
 */
Mcu_RamStateType Mcu_GetRamState(void);

/**
 * @brief 初始化 RAM 段
 *
 * @param[in] RamSection RAM 段索引
 * @return Std_ReturnType
 *         - E_OK: 初始化成功
 *         - E_NOT_OK: 初始化失败
 *
 * @pre MCU 模块已初始化
 * @post RAM 段已初始化
 */
Std_ReturnType Mcu_InitRamSection(uint8 RamSection);

#define MCU_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                      INLINE FUNCTIONS
==================================================================================================*/

#endif /* MCU_H */

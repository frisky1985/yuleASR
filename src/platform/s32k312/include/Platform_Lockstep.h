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
 * @file Platform_Lockstep.h
 * @brief S32K312平台Lockstep硬件抽象层
 * 
 * 针对NXP S32K312的硬件特定实现
 * 支持Cortex-M7 Lockstep功能
 * 
 * @ASIL-D Safety Level
 * @hardware S32K312
 */

#ifndef PLATFORM_LOCKSTEP_H
#define PLATFORM_LOCKSTEP_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define PLATFORM_LOCKSTEP_VENDOR_ID             43
#define PLATFORM_LOCKSTEP_AR_RELEASE_MAJOR_VERSION  4
#define PLATFORM_LOCKSTEP_AR_RELEASE_MINOR_VERSION  7
#define PLATFORM_LOCKSTEP_AR_RELEASE_REVISION_VERSION   0
#define PLATFORM_LOCKSTEP_SW_MAJOR_VERSION      1
#define PLATFORM_LOCKSTEP_SW_MINOR_VERSION      0
#define PLATFORM_LOCKSTEP_SW_PATCH_VERSION      0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"
#include "Lockstep.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief S32K312寄存器地址
 */
#define S32K312_MSCM_BASE                       0x40260000UL    /* 内存映射和安全控制器 */
#define S32K312_MC_RGM_BASE                     0x40080000UL    /* 复位和电源管理 */
#define S32K312_FCCU_BASE                       0x40090000UL    /* 故障收集和控制单元 */

/**
 * @brief MSCM寄存器偏移
 */
#define MSCM_CP0CFG_OFFSET                      0x0000U
#define MSCM_CP0CFG1_OFFSET                     0x0004U
#define MSCM_CP0COUNT_OFFSET                    0x0008U
#define MSCM_CP0CST_OFFSET                      0x000CU
#define MSCM_CP0CC_OFFSET                       0x0010U
#define MSCM_LOCKSTEP_CTRL_OFFSET               0x0200U
#define MSCM_LOCKSTEP_STATUS_OFFSET             0x0204U
#define MSCM_BIST_CTRL_OFFSET                   0x0210U
#define MSCM_BIST_STATUS_OFFSET                 0x0214U

/**
 * @brief Lockstep控制位
 */
#define MSCM_LOCKSTEP_CTRL_ENABLE               0x00000001U
#define MSCM_LOCKSTEP_CTRL_MODE_MASK            0x00000006U
#define MSCM_LOCKSTEP_CTRL_MODE_SHIFT           1U
#define MSCM_LOCKSTEP_CTRL_BIST_EN              0x00000100U
#define MSCM_LOCKSTEP_CTRL_EOUT_EN              0x00010000U

/**
 * @brief Lockstep状态位
 */
#define MSCM_LOCKSTEP_STATUS_ACTIVE             0x00000001U
#define MSCM_LOCKSTEP_STATUS_ERROR              0x00000002U
#define MSCM_LOCKSTEP_STATUS_MISMATCH           0x00000004U
#define MSCM_LOCKSTEP_STATUS_BIST_DONE          0x00000100U
#define MSCM_LOCKSTEP_STATUS_BIST_FAIL          0x00000200U

/**
 * @brief BIST状态
 */
#define BIST_STATUS_IDLE                        0U
#define BIST_STATUS_RUNNING                     1U
#define BIST_STATUS_COMPLETE_PASS               2U
#define BIST_STATUS_COMPLETE_FAIL               3U

/**
 * @brief 复位原因
 */
#define MC_RGM_DES_F_SWT                        0x00000001U     /* 软件看门狗复位 */
#define MC_RGM_DES_F_TSR                        0x00000002U     /* 温度敏感复位 */
#define MC_RGM_DES_F_LOCKUP                     0x00000004U     /* 锁定复位 */
#define MC_RGM_DES_F_FCCU_SAFE                  0x00000010U     /* FCCU安全模式复位 */
#define MC_RGM_DES_F_FCCU_HARD                  0x00000020U     /* FCCU硬复位 */
#define MC_RGM_DES_F_FCCU_SOFT                  0x00000040U     /* FCCU软复位 */
#define MC_RGM_DES_F_JTAG                       0x00000100U     /* JTAG复位 */
#define MC_RGM_DES_F_LOCKSTEP                   0x00010000U     /* Lockstep错误复位 */

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief MSCM寄存器结构体
 */
typedef struct
{
    volatile uint32 CP0CFG;                 /* CPU0配置 */
    volatile uint32 CP0CFG1;                /* CPU0配置1 */
    volatile uint32 CP0COUNT;               /* CPU0计数器 */
    volatile uint32 CP0CST;                 /* CPU0状态 */
    volatile uint32 CP0CC;                  /* CPU0循环计数 */
    volatile uint32 RESERVED0[59];          /* 保留 */
    volatile uint32 LOCKSTEP_CTRL;          /* Lockstep控制 */
    volatile uint32 LOCKSTEP_STATUS;        /* Lockstep状态 */
    volatile uint32 RESERVED1[2];           /* 保留 */
    volatile uint32 BIST_CTRL;              /* BIST控制 */
    volatile uint32 BIST_STATUS;            /* BIST状态 */
} Mscm_Type;

/**
 * @brief MC_RGM寄存器结构体 (复位管理)
 */
typedef struct
{
    volatile uint32 DES;                    /* 复位源 */
    volatile uint32 FES;                    /* 故障源 */
    volatile uint32 FERD;                   /* 故障源差异 */
    volatile uint32 FBRE;                   /* 故障源复位使能 */
    volatile uint32 FESS;                   /* 故障源状态 */
    volatile uint32 RESERVED0[3];           /* 保留 */
    volatile uint32 CTRL;                   /* 控制 */
    volatile uint32 ERCTRL;                 /* 延迟控制 */
    volatile uint32 RDSS;                   /* 最近复位源 */
} McRgm_Type;

/**
 * @brief FCCU寄存器结构体 (故障管理)
 */
typedef struct
{
    volatile uint32 CTRL;                   /* 控制 */
    volatile uint32 CTRLK;                  /* 控制密钥 */
    volatile uint32 CFG;                    /* 配置 */
    volatile uint32 NCF_CFG0;               /* 非关键故障配置 */
    volatile uint32 NCF_CFG1;               /* 非关键故障配置 */
    volatile uint32 NCF_CFG2;               /* 非关键故障配置 */
    volatile uint32 NCF_CFG3;               /* 非关键故障配置 */
    volatile uint32 NCF_S0;                 /* 非关键故障状态0 */
    volatile uint32 NCF_S1;                 /* 非关键故障状态1 */
    volatile uint32 NCF_S2;                 /* 非关键故障状态2 */
    volatile uint32 NCF_S3;                 /* 非关键故障状态3 */
    volatile uint32 NCFK;                   /* NCF清除密钥 */
    volatile uint32 NCF_E0;                 /* NCF使能0 */
    volatile uint32 NCF_E1;                 /* NCF使能1 */
    volatile uint32 NCF_E2;                 /* NCF使能2 */
    volatile uint32 NCF_E3;                 /* NCF使能3 */
    volatile uint32 NCF_TOE0;               /* NCF超时使能0 */
    volatile uint32 NCF_TOE1;               /* NCF超时使能1 */
    volatile uint32 NCF_TOE2;               /* NCF超时使能2 */
    volatile uint32 NCF_TOE3;               /* NCF超时使能3 */
    volatile uint32 NCF_TO;                 /* NCF超时 */
    volatile uint32 CFG_TO;                 /* 配置超时 */
    volatile uint32 EINOUT;                 /* 错误输入/输出 */
    volatile uint32 STAT;                   /* 状态 */
    volatile uint32 N2AF_STATUS;            /* 正常到故障状态 */
    volatile uint32 A2F_STATUS;             /* 正常到故障状态 */
    volatile uint32 N2AF_CFG;               /* 正常到故障配置 */
    volatile uint32 A2F_CFG;                /* 正常到故障配置 */
} Fccu_Type;

/**
 * @brief 平台Lockstep配置
 */
typedef struct
{
    boolean enableLockstep;                 /* 使能锁步 */
    boolean enableBist;                     /* 启动BIST */
    boolean enableEout;                     /* 错误输出 */
    uint8 lockstepMode;                     /* 锁步模式 (0=split, 1=lockstep) */
    uint32 bistTimeoutUs;                   /* BIST超时 (微秒) */
} Platform_LockstepConfigType;

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define PLATFORM_LOCKSTEP_START_SEC_CODE
#include "Platform_Lockstep_MemMap.h"

/**
 * @brief 初始化平台Lockstep
 * 
 * @param config Lockstep配置
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_Init(const Lockstep_ConfigType* config);

/**
 * @brief 去初始化平台Lockstep
 */
extern void Platform_Lockstep_DeInit(void);

/**
 * @brief 设置Lockstep模式
 * 
 * @param mode 模式
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_SetMode(Lockstep_ModeType mode);

/**
 * @brief 获取当前Lockstep状态
 * 
 * @param isActive 活跃状态输出
 * @param hasError 错误状态输出
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_GetStatus(boolean* isActive, boolean* hasError);

/**
 * @brief 检查Lockstep状态
 * 
 * @param mismatchDetected 检测到不匹配输出
 * @return E_OK: 正常, E_NOT_OK: 异常
 */
extern Std_ReturnType Platform_Lockstep_CheckStatus(boolean* mismatchDetected);

/**
 * @brief 运行BIST
 * 
 * @param timeoutUs 超时时间 (微秒)
 * @return E_OK: 通过, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_RunBist(uint32 timeoutUs);

/**
 * @brief 获取BIST结果
 * 
 * @param results 结果输出
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_GetBistResult(uint32* results);

/**
 * @brief 清除Lockstep错误状态
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_ClearError(void);

/**
 * @brief 获取复位原因
 * 
 * @param resetReason 复位原因输出
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Lockstep_GetResetReason(uint32* resetReason);

/**
 * @brief 触发系统复位
 * 
 * @param resetType 复位类型
 */
extern void Platform_System_Reset(uint8 resetType);

/**
 * @brief 请求安全状态
 * 
 * @param reason 原因
 */
extern void Platform_SafeState_Request(uint32 reason);

/**
 * @brief 计算CRC32
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @param seed 种子
 * @param result CRC结果
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_Crc_Calculate(
    const uint8* data,
    uint32 length,
    uint32 seed,
    uint32* result
);

#define PLATFORM_LOCKSTEP_STOP_SEC_CODE
#include "Platform_Lockstep_MemMap.h"

/*==================================================================================================
*                                       内联寄存器访问
==================================================================================================*/
#define MSCM    ((Mscm_Type*)S32K312_MSCM_BASE)
#define MC_RGM  ((McRgm_Type*)S32K312_MC_RGM_BASE)
#define FCCU    ((Fccu_Type*)S32K312_FCCU_BASE)

#endif /* PLATFORM_LOCKSTEP_H */

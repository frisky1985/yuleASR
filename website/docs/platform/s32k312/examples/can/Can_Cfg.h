---
title: Can_Cfg.H
description: "* S32K312 FlexCAN CAN-FD 配置"
sidebar_position: 11
---

/**
 * Can模块配置头文件
 * 
 * S32K312 FlexCAN CAN-FD 配置
 */

#ifndef CAN_CFG_H
#define CAN_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define CAN_CFG_VENDOR_ID                       43
#define CAN_CFG_AR_RELEASE_MAJOR_VERSION        4
#define CAN_CFG_AR_RELEASE_MINOR_VERSION        7
#define CAN_CFG_AR_RELEASE_REVISION_VERSION     0
#define CAN_CFG_SW_MAJOR_VERSION                1
#define CAN_CFG_SW_MINOR_VERSION                0
#define CAN_CFG_SW_PATCH_VERSION                0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 开发错误检测使能
 */
#define CAN_DEV_ERROR_DETECT                    (STD_ON)

/**
 * @brief 版本信息API使能
 */
#define CAN_VERSION_INFO_API                    (STD_ON)

/**
 * @brief 中断API使能
 */
#define CAN_API_ENABLE_INTERRUPT                (STD_ON)

/**
 * @brief 消息缓冲区大小
 */
#define CAN_MAX_MESSAGE_OBJECTS                 64U

/**
 * @brief CAN控制器数量
 */
#define CAN_CONTROLLER_COUNT                    1U

/**
 * @brief CAN通道数量 (每个控制器)
 */
#define CAN_CHANNEL_COUNT                       2U

/**
 * @brief CAN控制器ID
 */
#define CAN_CONTROLLER_0                        0U

/**
 * @brief CAN通道ID
 */
#define CAN_CHANNEL_0                           0U
#define CAN_CHANNEL_1                           1U

/**
 * @brief 波特率定义
 */
#define CAN_BAUDRATE_500K                       500000U
#define CAN_BAUDRATE_1M                         1000000U
#define CAN_FD_BAUDRATE_2M                      2000000U
#define CAN_FD_BAUDRATE_5M                      5000000U

/**
 * @brief 帧格式
 */
#define CAN_FRAME_STANDARD                      0U
#define CAN_FRAME_EXTENDED                      1U

/**
 * @brief 帧类型
 */
#define CAN_FRAME_TYPE_DATA                     0U
#define CAN_FRAME_TYPE_REMOTE                   1U

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief CAN控制器状态
 */
typedef enum
{
    CAN_CS_UNINIT = 0,          /* 未初始化 */
    CAN_CS_STARTED,             /* 已启动 */
    CAN_CS_STOPPED,             /* 已停止 */
    CAN_CS_SLEEP                /* 睡眠模式 */
} Can_ControllerStateType;

/**
 * @brief CAN通道状态
 */
typedef enum
{
    CAN_CH_NOT_ACTIVE = 0,      /* 未激活 */
    CAN_CH_TX,                  /* 发送中 */
    CAN_CH_RX                   /* 接收中 */
} Can_ChannelStateType;

/**
 * @brief CAN帧结构体
 */
typedef struct
{
    uint32 id;                  /* 标准ID (11位) 或 扩展ID (29位) */
    uint8 frameType;            /* 标准/扩展帧 */
    uint8 type;                 /* 数据帧/远程帧 */
    uint8 dlc;                  /* 数据长度码 (0-15, CAN FD) */
    uint8 data[64];             /* 数据 (最大64字节) */
    boolean fdBRS;              /* CAN FD 比特率切换 */
    boolean fdESI;              /* CAN FD 错误状态指示 */
} Can_PduType;

/**
 * @brief CAN消息物体配置
 */
typedef struct
{
    uint32 id;                  /* 消息ID */
    uint8 frameType;            /* 标准/扩展 */
    uint8 type;                 /* 发送/接收 */
    uint8 dlc;                  /* 数据长度 */
    uint8 hwObjHandle;          /* 硬件对象句柄 */
} Can_HwObjectType;

/**
 * @brief CAN滤波器配置 (S32K312支据ID过滤)
 */
typedef struct
{
    uint32 filterId;            /* 过滤ID */
    uint32 filterMask;          /* 过滤掩码 */
    boolean isExtended;         /* 扩展帧过滤 */
} Can_FilterConfigType;

/**
 * @brief CAN控制器配置
 */
typedef struct
{
    uint8 controllerId;         /* 控制器ID */
    uint32 baseAddress;         /* 寄存器基地址 */
    uint32 baudrate;            /* 标准CAN波特率 */
    uint32 fdBaudrate;          /* CAN FD数据段波特率 (如果使能FD) */
    boolean fdEnable;           /* CAN FD使能 */
    uint8 numTxObjects;         /* 发送对象数量 */
    uint8 numRxObjects;         /* 接收对象数量 */
    const Can_HwObjectType* hwObjects;  /* 硬件对象配置 */
    const Can_FilterConfigType* filters; /* 过滤器配置 */
    uint8 numFilters;           /* 过滤器数量 */
} Can_ControllerConfigType;

/**
 * @brief CAN初始化配置
 */
typedef struct
{
    uint8 numControllers;       /* 控制器数量 */
    const Can_ControllerConfigType* controllers; /* 控制器配置 */
} Can_ConfigType;

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define CAN_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Can_MemMap.h"

extern const Can_ConfigType Can_Config;

#define CAN_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Can_MemMap.h"

#endif /* CAN_CFG_H */

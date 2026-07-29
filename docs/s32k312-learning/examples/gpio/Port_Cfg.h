/**
 * Port模块配置头文件
 * 
 * S32K312 PTB15 - LED输出配置
 */

#ifndef PORT_CFG_H
#define PORT_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define PORT_CFG_VENDOR_ID                      43
#define PORT_CFG_AR_RELEASE_MAJOR_VERSION       4
#define PORT_CFG_AR_RELEASE_MINOR_VERSION       7
#define PORT_CFG_AR_RELEASE_REVISION_VERSION    0
#define PORT_CFG_SW_MAJOR_VERSION               1
#define PORT_CFG_SW_MINOR_VERSION               0
#define PORT_CFG_SW_PATCH_VERSION               0

/*==================================================================================================
*                                       预处理指令
==================================================================================================*/
#include "Port_CfgDefines.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 开发错误检测使能
 */
#define PORT_DEV_ERROR_DETECT                   (STD_ON)

/**
 * @brief 版本信息API使能
 */
#define PORT_VERSION_INFO_API                   (STD_ON)

/**
 * @brief 引脚模式API使能 (Port_SetPinMode)
 */
#define PORT_SET_PIN_MODE_API                   (STD_ON)

/**
 * @brief 引脚方向API使能 (Port_SetPinDirection)
 */
#define PORT_SET_PIN_DIRECTION_API              (STD_ON)

/**
 * @brief 引脚数量定义
 */
#define PORT_NUMBER_OF_PINS                     1U

/**
 * @brief LED引脚定义
 */
#define PORT_PIN_LED_RED                        79U     /* PTB15 */

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief Port配置结构体
 */
typedef struct
{
    uint16 pinId;                           /* 引脚ID */
    uint8 pinMode;                          /* 引脚模式 (GPIO/ADC/CAN等) */
    uint8 direction;                        /* 方向: PORT_PIN_IN/PORT_PIN_OUT */
    uint8 initialValue;                     /* 初始值 */
    uint8 driveStrength;                    /* 驱动能力 */
    uint8 pullConfig;                       /* 上下拉配置 */
    boolean invertEnable;                   /* 电平反转使能 */
    boolean openDrainEnable;                /* 开漏使能 */
    boolean inputFilterEnable;              /* 输入滤波使能 */
    boolean inputBufferEnable;              /* 输入缓冲使能 */
} Port_PinConfigType;

/**
 * @brief Port初始化配置结构体
 */
typedef struct
{
    uint16 numPins;                         /* 配置的引脚数量 */
    const Port_PinConfigType* pinConfig;    /* 引脚配置数组 */
} Port_ConfigType;

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define PORT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Port_MemMap.h"

/**
 * @brief Port初始化配置数据
 */
extern const Port_ConfigType Port_Config;

#define PORT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Port_MemMap.h"

#endif /* PORT_CFG_H */

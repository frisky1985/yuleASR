/**
 * Port模块配置实现
 * 
 * S32K312 PTB15 - LED输出配置
 */

#include "Port_Cfg.h"
#include "Port_MemMap.h"

/*==================================================================================================
*                                       定义和宏
==================================================================================================*/
/**
 * @brief 引脚模式定义
 */
#define PORT_PIN_MODE_GPIO              0x00U
#define PORT_PIN_MODE_ADC               0x01U
#define PORT_PIN_MODE_CAN               0x02U
#define PORT_PIN_MODE_LPUART            0x03U
#define PORT_PIN_MODE_LPSPI             0x04U
#define PORT_PIN_MODE_LPI2C             0x05U

/**
 * @brief 引脚方向定义
 */
#define PORT_PIN_IN                     0x00U
#define PORT_PIN_OUT                    0x01U

/**
 * @brief 引脚驱动能力
 */
#define PORT_DRIVE_STRENGTH_DISABLE     0x00U
#define PORT_DRIVE_STRENGTH_LOW         0x01U
#define PORT_DRIVE_STRENGTH_MEDIUM      0x02U
#define PORT_DRIVE_STRENGTH_HIGH        0x03U

/**
 * @brief 上下拉配置
 */
#define PORT_INTERNAL_PULL_NOT_ENABLED  0x00U
#define PORT_INTERNAL_PULL_DOWN_ENABLED 0x01U
#define PORT_INTERNAL_PULL_UP_ENABLED   0x02U

/*==================================================================================================
*                                       配置数据
==================================================================================================*/
#define PORT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Port_MemMap.h"

/**
 * @brief 引脚配置数组
 */
static const Port_PinConfigType Port_PinConfigs[PORT_NUMBER_OF_PINS] =
{
    /* LED_RED - PTB15 */
    {
        .pinId = PORT_PIN_LED_RED,              /* Pin ID: 79 (PTB15) */
        .pinMode = PORT_PIN_MODE_GPIO,          /* GPIO模式 */
        .direction = PORT_PIN_OUT,              /* 输出方向 */
        .initialValue = STD_LOW,                /* 初始值低电平 */
        .driveStrength = PORT_DRIVE_STRENGTH_LOW, /* 低驱动能力 */
        .pullConfig = PORT_INTERNAL_PULL_NOT_ENABLED, /* 无上下拉 */
        .invertEnable = FALSE,                  /* 不反转电平 */
        .openDrainEnable = FALSE,               /* 推挷输出 */
        .inputFilterEnable = FALSE,             /* 关闭输入滤波 */
        .inputBufferEnable = FALSE              /* 关闭输入缓冲 */
    }
};

/**
 * @brief Port初始化配置
 */
const Port_ConfigType Port_Config =
{
    .numPins = PORT_NUMBER_OF_PINS,
    .pinConfig = Port_PinConfigs
};

#define PORT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Port_MemMap.h"

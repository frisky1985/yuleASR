/**
 * Can模块配置实现
 * 
 * S32K312 FlexCAN CAN-FD 配置
 */

#include "Can_Cfg.h"
#include "Can_MemMap.h"

/*==================================================================================================
*                                       定义和宏
==================================================================================================*/
/**
 * @brief S32K312 FlexCAN0 基地址
 */
#define CAN0_BASE_ADDRESS                       0x40130000UL

/**
 * @brief 时钟源
 */
#define CAN_CLOCK_SOURCE                        80000000UL  /* 80MHz */

/**
 * @brief 时间段计算参数
 * 标准CAN: 500Kbps, 采样点1位, TSEG1=13, TSEG2=2, SJW=2
 * CAN FD数据段: 2Mbps, TSEG1=7, TSEG2=2, SJW=2
 */
#define CAN_PROPSEG                             6U
#define CAN_PSEG1                               7U
#define CAN_PSEG2                               2U
#define CAN_RJW                                 2U
#define CAN_PRESDIV                             4U          /* 80MHz/(4*(1+6+7+2))=500KHz */

#define CAN_FD_PRESDIV                          4U          /* 80MHz/(4*(1+6+7+2))=2MHz */

/*==================================================================================================
*                                       配置数据
==================================================================================================*/
#define CAN_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Can_MemMap.h"

/**
 * @brief 接收过滤器配置 - 接收ID 0x100-0x1FF
 */
static const Can_FilterConfigType Can_Filters[CAN_CHANNEL_COUNT] =
{
    /* 通道0: 接收标准数据帧 0x100 */
    {
        .filterId = 0x100U,
        .filterMask = 0x700U,       /* 匹配 0x100-0x1FF */
        .isExtended = FALSE
    },
    /* 通道1: 接收扩展数据帧 0x18FF0000 */
    {
        .filterId = 0x18FF0000U,
        .filterMask = 0x1FFF0000U,  /* 匹配 0x18FF0000-0x18FFFFFF */
        .isExtended = TRUE
    }
};

/**
 * @brief 硬件对象配置
 */
static const Can_HwObjectType Can_HwObjects[CAN_CHANNEL_COUNT] =
{
    /* 通道 0 - 发送 */
    {
        .id = 0x200U,
        .frameType = CAN_FRAME_STANDARD,
        .type = CAN_FRAME_TYPE_DATA,
        .dlc = 8U,
        .hwObjHandle = 0U
    },
    /* 通道 1 - 接收 */
    {
        .id = 0x100U,
        .frameType = CAN_FRAME_STANDARD,
        .type = CAN_FRAME_TYPE_DATA,
        .dlc = 8U,
        .hwObjHandle = 1U
    }
};

/**
 * @brief CAN控制器配置
 */
static const Can_ControllerConfigType Can_Controllers[CAN_CONTROLLER_COUNT] =
{
    {
        .controllerId = CAN_CONTROLLER_0,
        .baseAddress = CAN0_BASE_ADDRESS,
        .baudrate = CAN_BAUDRATE_500K,
        .fdBaudrate = CAN_FD_BAUDRATE_2M,
        .fdEnable = TRUE,           /* 使能CAN FD */
        .numTxObjects = 32U,        /* 32个发送缓冲区 */
        .numRxObjects = 32U,        /* 32个接收缓冲区 */
        .hwObjects = Can_HwObjects,
        .filters = Can_Filters,
        .numFilters = CAN_CHANNEL_COUNT
    }
};

/**
 * @brief CAN模块初始化配置
 */
const Can_ConfigType Can_Config =
{
    .numControllers = CAN_CONTROLLER_COUNT,
    .controllers = Can_Controllers
};

#define CAN_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Can_MemMap.h"

/*==================================================================================================
*                                       回调函数
==================================================================================================*/

/**
 * @brief 发送确认回调
 */
void CanIf_TxConfirmation(uint8 channel)
{
    /* 通知上层发送完成 */
}

/**
 * @brief 接收指示回调
 */
void CanIf_RxIndication(uint8 channel, const Can_PduType* pdu)
{
    /* 通知上层接收到数据 */
}

/**
 * @brief 控制器模式改变回调
 */
void CanIf_ControllerModeIndication(uint8 controller, Can_ControllerStateType state)
{
    /* 通知上层控制器状态变化 */
}

/**
 * @file Dio_Cfg.h
 * @brief DIO Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Seat Control Demo — DIO channel definitions:
 *   - Switch inputs  (0x0001-0x000C)
 *   - LED outputs    (0x0101-0x0103)
 *   - Motor relays   (0x0201-0x0204)
 */

#ifndef DIO_CFG_H
#define DIO_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Dio.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define DIO_DEV_ERROR_DETECT            (STD_ON)
#define DIO_VERSION_INFO_API            (STD_ON)
#define DIO_FLIP_CHANNEL_API            (STD_ON)
#define DIO_MASKED_WRITE_PORT_API       (STD_ON)

#define DIO_NUM_PORTS                   (8U)
#define DIO_NUM_CHANNELS_PER_PORT       (32U)
#define DIO_NUM_CHANNEL_GROUPS          (8U)

/*==================================================================================================
 * Port Definitions
 *==================================================================================================*/
#define DIO_PORT_A                      (0U)
#define DIO_PORT_B                      (1U)
#define DIO_PORT_C                      (2U)
#define DIO_PORT_D                      (3U)
#define DIO_PORT_E                      (4U)
#define DIO_PORT_F                      (5U)
#define DIO_PORT_G                      (6U)
#define DIO_PORT_H                      (7U)

/*==================================================================================================
 * Seat Control — Switch Input Channels
 * (Lower byte = pin within port, upper byte = port index)
 *==================================================================================================*/
/* Port A — Motor direction & switch inputs */
#define DioConf_DioChannel_SeatSwitchForward        ((Dio_ChannelType)0x0001U)  /* 前移开关 */
#define DioConf_DioChannel_SeatSwitchBackward       ((Dio_ChannelType)0x0002U)  /* 后移开关 */
#define DioConf_DioChannel_SeatSwitchReclineFwd     ((Dio_ChannelType)0x0003U)  /* 前倾按钮 */
#define DioConf_DioChannel_SeatSwitchReclineBwd     ((Dio_ChannelType)0x0004U)  /* 后仰按钮 */
#define DioConf_DioChannel_SeatSwitchHeightUp       ((Dio_ChannelType)0x0005U)  /* 上升按钮 */
#define DioConf_DioChannel_SeatSwitchHeightDown     ((Dio_ChannelType)0x0006U)  /* 下降按钮 */
#define DioConf_DioChannel_SeatSwitchTiltUp         ((Dio_ChannelType)0x0007U)  /* 倾斜向上 */
#define DioConf_DioChannel_SeatSwitchTiltDown       ((Dio_ChannelType)0x0008U)  /* 倾斜向下 */
#define DioConf_DioChannel_SeatHeatHigh             ((Dio_ChannelType)0x0009U)  /* 加热高档 */
#define DioConf_DioChannel_SeatHeatLow              ((Dio_ChannelType)0x000AU)  /* 加热低档 */
#define DioConf_DioChannel_SeatMemory1              ((Dio_ChannelType)0x000BU)  /* 记忆1 */
#define DioConf_DioChannel_SeatMemory2              ((Dio_ChannelType)0x000CU)  /* 记忆2 */

/* Port F — LED outputs */
#define DioConf_DioChannel_SeatLedStatus            ((Dio_ChannelType)0x0101U)  /* 状态LED */
#define DioConf_DioChannel_SeatLedHeat              ((Dio_ChannelType)0x0102U)  /* 加热指示 */
#define DioConf_DioChannel_SeatLedMemory            ((Dio_ChannelType)0x0103U)  /* 记忆指示 */

/* Port F — Motor relay enable outputs */
#define DioConf_DioChannel_MotorHorizontalEn        ((Dio_ChannelType)0x0201U)  /* 水平电机使能 */
#define DioConf_DioChannel_MotorReclineEn           ((Dio_ChannelType)0x0202U)  /* 倾斜电机使能 */
#define DioConf_DioChannel_MotorHeightEn            ((Dio_ChannelType)0x0203U)  /* 升降电机使能 */
#define DioConf_DioChannel_MotorTiltEn              ((Dio_ChannelType)0x0204U)  /* 倾斜电机使能 */

/*==================================================================================================
 * DIO Channel Configuration Data
 *==================================================================================================*/

typedef enum {
    DIO_PIN_DIR_INPUT = 0,
    DIO_PIN_DIR_OUTPUT
} Dio_PinDirectionType;

typedef struct {
    Dio_ChannelType           channel;
    Dio_PinDirectionType      direction;
    Dio_LevelType             initialLevel;
    boolean                   enablePullUp;
} Dio_ChannelConfigType;

typedef struct {
    Dio_ChannelConfigType*    channels;
    uint16                    numChannels;
} Dio_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Dio_ConfigType Dio_Config;

#endif /* DIO_CFG_H */

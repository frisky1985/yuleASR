/**
 * @file Pwm_Cfg.h
 * @brief PWM Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * PWM channels:
 *   - Horizontal, Recline, Height, Tilt motors (10kHz, 0-100%)
 *   - Seat heater PWM (1kHz, 0-100%)
 */

#ifndef PWM_CFG_H
#define PWM_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"
#include "Pwm.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define PWM_DEV_ERROR_DETECT            (STD_ON)
#define PWM_VERSION_INFO_API            (STD_ON)
#define PWM_POWER_STATE_SUPPORT         (STD_OFF)

/*==================================================================================================
 * PWM Channel IDs
 *==================================================================================================*/
#define PWM_CHANNEL_HORIZONTAL          ((Pwm_ChannelType)0U)   /* 水平电机 PWM */
#define PWM_CHANNEL_RECLINE             ((Pwm_ChannelType)1U)   /* 靠背电机 PWM */
#define PWM_CHANNEL_HEIGHT              ((Pwm_ChannelType)2U)   /* 升降电机 PWM */
#define PWM_CHANNEL_TILT                ((Pwm_ChannelType)3U)   /* 倾斜电机 PWM */
#define PWM_CHANNEL_SEAT_HEATER         ((Pwm_ChannelType)4U)   /* 座椅加热 PWM */

#define PWM_NUM_CHANNELS                (5U)

/*==================================================================================================
 * PWM Polarity
 *==================================================================================================*/
#define PWM_POLARITY_HIGH               (0U)
#define PWM_POLARITY_LOW                (1U)

/*==================================================================================================
 * PWM Channel Configuration Type
 *==================================================================================================*/
typedef enum {
    PWM_MODE_EDGE_ALIGNED = 0,
    PWM_MODE_CENTER_ALIGNED
} Pwm_ChannelModeType;

typedef enum {
    PWM_IDLE_LOW = 0,
    PWM_IDLE_HIGH
} Pwm_IdleStateType;

typedef struct {
    Pwm_ChannelType         channel;
    uint32                  frequency;          /* PWM frequency in Hz */
    uint16                  defaultDutyCycle;   /* Default duty cycle (0-100%) */
    Pwm_ChannelModeType     mode;               /* Edge or center aligned */
    Pwm_IdleStateType       idleState;          /* Output state when stopped */
    Pwm_OutputStateType     polarity;           /* PWM_POLARITY_HIGH or LOW */
} Pwm_ChannelConfigType;

/*==================================================================================================
 * PWM Configuration Type
 *==================================================================================================*/
typedef struct {
    Pwm_ChannelConfigType*  channels;
    uint16                  numChannels;
} Pwm_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Pwm_ConfigType Pwm_Config;

#endif /* PWM_CFG_H */

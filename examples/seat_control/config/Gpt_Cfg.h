/**
 * @file Gpt_Cfg.h
 * @brief GPT Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Timer channels:
 *   - GPT_CHANNEL_1MS:     1ms system tick
 *   - GPT_CHANNEL_10MS:    10ms seat control cycle
 *   - GPT_CHANNEL_100MS:   100ms status update
 */

#ifndef GPT_CFG_H
#define GPT_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define GPT_DEV_ERROR_DETECT            (STD_ON)
#define GPT_VERSION_INFO_API            (STD_ON)

/*==================================================================================================
 * GPT Channel Definitions
 *==================================================================================================*/
#define GPT_CHANNEL_1MS                 ((Gpt_ChannelType)0U)   /* 1ms 系统滴答 */
#define GPT_CHANNEL_10MS                ((Gpt_ChannelType)1U)   /* 10ms 座椅控制周期 */
#define GPT_CHANNEL_100MS               ((Gpt_ChannelType)2U)   /* 100ms 状态更新 */

#define GPT_NUM_CHANNELS                (3U)

/*==================================================================================================
 * GPT Channel Config Type
 *==================================================================================================*/
typedef uint8 Gpt_ChannelType;

typedef enum {
    GPT_MODE_ONE_SHOT = 0,
    GPT_MODE_CONTINUOUS
} Gpt_ModeType;

typedef struct {
    Gpt_ChannelType     channel;
    uint32              tickRateHz;         /* Timer tick rate */
    Gpt_ModeType        mode;
    boolean             enableInterrupt;
    uint32              expectedMaxCount;   /* For 32-bit timers */
} Gpt_ChannelConfigType;

/*==================================================================================================
 * GPT Configuration Type
 *==================================================================================================*/
typedef struct {
    Gpt_ChannelConfigType* channels;
    uint16                 numChannels;
    uint32                 clockRef;           /* Reference clock in Hz */
} Gpt_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Gpt_ConfigType Gpt_Config;

#endif /* GPT_CFG_H */

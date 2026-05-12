/**
 * @file Dio_Cfg.h
 * @brief DIO Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DIO_CFG_H
#define DIO_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define DIO_DEV_ERROR_DETECT            (STD_ON)
#define DIO_VERSION_INFO_API            (STD_ON)
#define DIO_FLIP_CHANNEL_API            (STD_ON)
#define DIO_MASKED_WRITE_PORT_API       (STD_ON)

/*==================================================================================================
*                                    NUMBER OF PORTS
==================================================================================================*/
#define DIO_NUM_PORTS                   (8U)
#define DIO_NUM_CHANNELS_PER_PORT       (32U)

/*==================================================================================================
*                                    PORT DEFINITIONS
==================================================================================================*/
#define DIO_PORT_A                      (0U)
#define DIO_PORT_B                      (1U)
#define DIO_PORT_C                      (2U)
#define DIO_PORT_D                      (3U)
#define DIO_PORT_E                      (4U)
#define DIO_PORT_F                      (5U)
#define DIO_PORT_G                      (6U)
#define DIO_PORT_H                      (7U)

/*==================================================================================================
*                                    CHANNEL GROUP CONFIGURATION
==================================================================================================*/
#define DIO_NUM_CHANNEL_GROUPS          (4U)

/*==================================================================================================
*                                    DIO CHANNEL DEFINITIONS
==================================================================================================*/
/* Port A Channels */
#define DIO_CHANNEL_A0                  ((Dio_ChannelType)0x0000U)
#define DIO_CHANNEL_A1                  ((Dio_ChannelType)0x0001U)
#define DIO_CHANNEL_A2                  ((Dio_ChannelType)0x0002U)
#define DIO_CHANNEL_A3                  ((Dio_ChannelType)0x0003U)
#define DIO_CHANNEL_A4                  ((Dio_ChannelType)0x0004U)
#define DIO_CHANNEL_A5                  ((Dio_ChannelType)0x0005U)
#define DIO_CHANNEL_A6                  ((Dio_ChannelType)0x0006U)
#define DIO_CHANNEL_A7                  ((Dio_ChannelType)0x0007U)

/* Port B Channels */
#define DIO_CHANNEL_B0                  ((Dio_ChannelType)0x0100U)
#define DIO_CHANNEL_B1                  ((Dio_ChannelType)0x0101U)
#define DIO_CHANNEL_B2                  ((Dio_ChannelType)0x0102U)
#define DIO_CHANNEL_B3                  ((Dio_ChannelType)0x0103U)
#define DIO_CHANNEL_B4                  ((Dio_ChannelType)0x0104U)
#define DIO_CHANNEL_B5                  ((Dio_ChannelType)0x0105U)
#define DIO_CHANNEL_B6                  ((Dio_ChannelType)0x0106U)
#define DIO_CHANNEL_B7                  ((Dio_ChannelType)0x0107U)

/* Port C Channels */
#define DIO_CHANNEL_C0                  ((Dio_ChannelType)0x0200U)
#define DIO_CHANNEL_C1                  ((Dio_ChannelType)0x0201U)
#define DIO_CHANNEL_C2                  ((Dio_ChannelType)0x0202U)
#define DIO_CHANNEL_C3                  ((Dio_ChannelType)0x0203U)
#define DIO_CHANNEL_C4                  ((Dio_ChannelType)0x0204U)
#define DIO_CHANNEL_C5                  ((Dio_ChannelType)0x0205U)
#define DIO_CHANNEL_C6                  ((Dio_ChannelType)0x0206U)
#define DIO_CHANNEL_C7                  ((Dio_ChannelType)0x0207U)

/* Port D Channels */
#define DIO_CHANNEL_D0                  ((Dio_ChannelType)0x0300U)
#define DIO_CHANNEL_D1                  ((Dio_ChannelType)0x0301U)
#define DIO_CHANNEL_D2                  ((Dio_ChannelType)0x0302U)
#define DIO_CHANNEL_D3                  ((Dio_ChannelType)0x0303U)
#define DIO_CHANNEL_D4                  ((Dio_ChannelType)0x0304U)
#define DIO_CHANNEL_D5                  ((Dio_ChannelType)0x0305U)
#define DIO_CHANNEL_D6                  ((Dio_ChannelType)0x0306U)
#define DIO_CHANNEL_D7                  ((Dio_ChannelType)0x0307U)

#endif /* DIO_CFG_H */

/**
 * @file Port_Cfg.h
 * @brief PORT Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef PORT_CFG_H
#define PORT_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define PORT_DEV_ERROR_DETECT           (STD_ON)
#define PORT_VERSION_INFO_API           (STD_ON)
#define PORT_SET_PIN_DIRECTION_API      (STD_ON)
#define PORT_SET_PIN_MODE_API           (STD_ON)

/*==================================================================================================
*                                    NUMBER OF PORTS
==================================================================================================*/
#define PORT_NUM_PORTS                  (8U)
#define PORT_NUM_PINS_PER_PORT          (32U)
#define PORT_TOTAL_NUM_PINS             (256U)

/*==================================================================================================
*                                    PORT DEFINITIONS
==================================================================================================*/
#define PORT_A                          (0U)
#define PORT_B                          (1U)
#define PORT_C                          (2U)
#define PORT_D                          (3U)
#define PORT_E                          (4U)
#define PORT_F                          (5U)
#define PORT_G                          (6U)
#define PORT_H                          (7U)

/*==================================================================================================
*                                    PORT MODE DEFINITIONS
==================================================================================================*/
#define PORT_PIN_MODE_GPIO              (0U)
#define PORT_PIN_MODE_CAN               (1U)
#define PORT_PIN_MODE_SPI               (2U)
#define PORT_PIN_MODE_UART              (3U)
#define PORT_PIN_MODE_I2C               (4U)
#define PORT_PIN_MODE_PWM               (5U)
#define PORT_PIN_MODE_ADC               (6U)
#define PORT_PIN_MODE_ETH               (7U)
#define PORT_PIN_MODE_USB               (8U)
#define PORT_PIN_MODE_FLEXIO            (9U)
#define PORT_PIN_MODE_DISABLED          (15U)

/*==================================================================================================
*                                    PORT PIN DEFINITIONS
==================================================================================================*/
/* Port A Pins */
#define PORT_A_PIN_0                    ((Port_PinType)0x0000U)
#define PORT_A_PIN_1                    ((Port_PinType)0x0001U)
#define PORT_A_PIN_2                    ((Port_PinType)0x0002U)
#define PORT_A_PIN_3                    ((Port_PinType)0x0003U)
#define PORT_A_PIN_4                    ((Port_PinType)0x0004U)
#define PORT_A_PIN_5                    ((Port_PinType)0x0005U)
#define PORT_A_PIN_6                    ((Port_PinType)0x0006U)
#define PORT_A_PIN_7                    ((Port_PinType)0x0007U)

/* Port B Pins */
#define PORT_B_PIN_0                    ((Port_PinType)0x0100U)
#define PORT_B_PIN_1                    ((Port_PinType)0x0101U)
#define PORT_B_PIN_2                    ((Port_PinType)0x0102U)
#define PORT_B_PIN_3                    ((Port_PinType)0x0103U)
#define PORT_B_PIN_4                    ((Port_PinType)0x0104U)
#define PORT_B_PIN_5                    ((Port_PinType)0x0105U)
#define PORT_B_PIN_6                    ((Port_PinType)0x0106U)
#define PORT_B_PIN_7                    ((Port_PinType)0x0107U)

/* Port C Pins */
#define PORT_C_PIN_0                    ((Port_PinType)0x0200U)
#define PORT_C_PIN_1                    ((Port_PinType)0x0201U)
#define PORT_C_PIN_2                    ((Port_PinType)0x0202U)
#define PORT_C_PIN_3                    ((Port_PinType)0x0203U)
#define PORT_C_PIN_4                    ((Port_PinType)0x0204U)
#define PORT_C_PIN_5                    ((Port_PinType)0x0205U)
#define PORT_C_PIN_6                    ((Port_PinType)0x0206U)
#define PORT_C_PIN_7                    ((Port_PinType)0x0207U)

/* Port D Pins */
#define PORT_D_PIN_0                    ((Port_PinType)0x0300U)
#define PORT_D_PIN_1                    ((Port_PinType)0x0301U)
#define PORT_D_PIN_2                    ((Port_PinType)0x0302U)
#define PORT_D_PIN_3                    ((Port_PinType)0x0303U)
#define PORT_D_PIN_4                    ((Port_PinType)0x0304U)
#define PORT_D_PIN_5                    ((Port_PinType)0x0305U)
#define PORT_D_PIN_6                    ((Port_PinType)0x0306U)
#define PORT_D_PIN_7                    ((Port_PinType)0x0307U)

/*==================================================================================================
*                                    PORT CONFIGURATION SET
==================================================================================================*/
#define PORT_CONFIGSET_NAME             (Port_ConfigSet)

#endif /* PORT_CFG_H */

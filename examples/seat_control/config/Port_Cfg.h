/**
 * @file Port_Cfg.h
 * @brief PORT Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * S32K312 pin mapping for seat control demo:
 *   - Motor direction:  PTA0-PTA7 (DIO)
 *   - PWM outputs:      PTB0-PTB4 (FLEXIO_PWM)
 *   - ADC inputs:       PTC0-PTC3
 *   - CAN:              PTD0-PTD1 (CAN0)
 *   - SPI:              PTD2-PTD5 (LPSPI)
 *   - LIN:              PTE0-PTE1 (LPI2C alt)
 *   - Limit switches:   PTF0-PTF7
 *   - LED:              PTF8-PTF10
 */

#ifndef PORT_CFG_H
#define PORT_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define PORT_DEV_ERROR_DETECT           (STD_ON)
#define PORT_VERSION_INFO_API           (STD_ON)

#define PORT_NUM_PINS                   (56U)   /* S32K312 has multiple ports */

/*==================================================================================================
 * Port Pin Definitions
 * Pin ID format: (port << 8) | pin
 *==================================================================================================*/
/* Port A — Motor direction & switch inputs (DIO outputs) */
#define PORT_PIN_PTA0                   ((Port_PinType)0x0000U)  /* 水平电机方向A */
#define PORT_PIN_PTA1                   ((Port_PinType)0x0001U)  /* 水平电机方向B */
#define PORT_PIN_PTA2                   ((Port_PinType)0x0002U)  /* 靠背电机方向A */
#define PORT_PIN_PTA3                   ((Port_PinType)0x0003U)  /* 靠背电机方向B */
#define PORT_PIN_PTA4                   ((Port_PinType)0x0004U)  /* 升降电机方向A */
#define PORT_PIN_PTA5                   ((Port_PinType)0x0005U)  /* 升降电机方向B */
#define PORT_PIN_PTA6                   ((Port_PinType)0x0006U)  /* 倾斜电机方向A */
#define PORT_PIN_PTA7                   ((Port_PinType)0x0007U)  /* 倾斜电机方向B */

/* Port B — PWM outputs */
#define PORT_PIN_PTB0                   ((Port_PinType)0x0100U)  /* 水平电机 PWM (FLEXIO) */
#define PORT_PIN_PTB1                   ((Port_PinType)0x0101U)  /* 靠背电机 PWM (FLEXIO) */
#define PORT_PIN_PTB2                   ((Port_PinType)0x0102U)  /* 升降电机 PWM (FLEXIO) */
#define PORT_PIN_PTB3                   ((Port_PinType)0x0103U)  /* 倾斜电机 PWM (FLEXIO) */
#define PORT_PIN_PTB4                   ((Port_PinType)0x0104U)  /* 加热 PWM (FLEXIO) */

/* Port C — ADC inputs */
#define PORT_PIN_PTC0                   ((Port_PinType)0x0200U)  /* 水平位置 ADC */
#define PORT_PIN_PTC1                   ((Port_PinType)0x0201U)  /* 靠背位置 ADC */
#define PORT_PIN_PTC2                   ((Port_PinType)0x0202U)  /* 升降位置 ADC */
#define PORT_PIN_PTC3                   ((Port_PinType)0x0203U)  /* 倾斜位置 ADC */

/* Port D — CAN & SPI */
#define PORT_PIN_PTD0                   ((Port_PinType)0x0300U)  /* CAN0 TX */
#define PORT_PIN_PTD1                   ((Port_PinType)0x0301U)  /* CAN0 RX */
#define PORT_PIN_PTD2                   ((Port_PinType)0x0302U)  /* LPSPI SCK */
#define PORT_PIN_PTD3                   ((Port_PinType)0x0303U)  /* LPSPI SOUT */
#define PORT_PIN_PTD4                   ((Port_PinType)0x0304U)  /* LPSPI SIN */
#define PORT_PIN_PTD5                   ((Port_PinType)0x0305U)  /* LPSPI PCS */

/* Port E — LIN */
#define PORT_PIN_PTE0                   ((Port_PinType)0x0400U)  /* LIN TX */
#define PORT_PIN_PTE1                   ((Port_PinType)0x0401U)  /* LIN RX */

/* Port F — Limit switches & LEDs */
#define PORT_PIN_PTF0                   ((Port_PinType)0x0500U)  /* 限位开关: 水平前 */
#define PORT_PIN_PTF1                   ((Port_PinType)0x0501U)  /* 限位开关: 水平后 */
#define PORT_PIN_PTF2                   ((Port_PinType)0x0502U)  /* 限位开关: 靠背前 */
#define PORT_PIN_PTF3                   ((Port_PinType)0x0503U)  /* 限位开关: 靠背后 */
#define PORT_PIN_PTF4                   ((Port_PinType)0x0504U)  /* 限位开关: 升降上 */
#define PORT_PIN_PTF5                   ((Port_PinType)0x0505U)  /* 限位开关: 升降下 */
#define PORT_PIN_PTF6                   ((Port_PinType)0x0506U)  /* 限位开关: 倾斜上 */
#define PORT_PIN_PTF7                   ((Port_PinType)0x0507U)  /* 限位开关: 倾斜下 */
#define PORT_PIN_PTF8                   ((Port_PinType)0x0508U)  /* 状态 LED */
#define PORT_PIN_PTF9                   ((Port_PinType)0x0509U)  /* 加热指示 LED */
#define PORT_PIN_PTF10                  ((Port_PinType)0x050AU)  /* 记忆指示 LED */

/*==================================================================================================
 * Port Pin Mode (alternate function)
 *==================================================================================================*/
typedef enum {
    PORT_PIN_MODE_DIO = 0,
    PORT_PIN_MODE_ALT1,
    PORT_PIN_MODE_ALT2,
    PORT_PIN_MODE_ALT3,
    PORT_PIN_MODE_ALT4,
    PORT_PIN_MODE_ALT5,
    PORT_PIN_MODE_ALT6,
    PORT_PIN_MODE_ALT7,
    PORT_PIN_MODE_ANALOG
} Port_PinModeType;

typedef enum {
    PORT_PIN_DIR_INPUT = 0,
    PORT_PIN_DIR_OUTPUT
} Port_PinDirectionType;

/*==================================================================================================
 * Port Pin Configuration
 *==================================================================================================*/
typedef struct {
    Port_PinType            pin;
    Port_PinModeType        mode;
    Port_PinDirectionType   direction;
    boolean                 enablePullUp;
    boolean                 enablePullDown;
    boolean                 enableSlewRateControl;
    boolean                 enableInputFilter;
    uint8                   driveStrength;      /* 0=low, 1=high */
} Port_PinConfigType;

typedef struct {
    Port_PinConfigType*     pins;
    uint16                  numPins;
} Port_ConfigType;

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Port_ConfigType Port_Config;

#endif /* PORT_CFG_H */

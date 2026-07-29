/**
 * @file Seat_Cfg.h
 * @brief Seat control application configuration
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Mechanical limits, electrical thresholds, and behavioral
 * parameters for the 6-way power seat demo.
 */

#ifndef SEAT_CFG_H
#define SEAT_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Mechanical Limits
 *==================================================================================================*/
#define SEAT_MAX_HORIZONTAL_MM          (230U)      /* 最大水平行程(mm) */
#define SEAT_MAX_RECLINE_DEG            (60U)       /* 最大靠背角度(度) */
#define SEAT_MAX_HEIGHT_MM              (50U)       /* 最大升降行程(mm) */
#define SEAT_MAX_TILT_DEG               (15U)       /* 最大倾角(度) */

#define SEAT_MIN_HORIZONTAL_MM          (0U)
#define SEAT_MIN_RECLINE_DEG            (0U)
#define SEAT_MIN_HEIGHT_MM              (0U)
#define SEAT_MIN_TILT_DEG               (0U)

/*==================================================================================================
 * Electrical Thresholds
 *==================================================================================================*/
#define SEAT_OVER_CURRENT_MA            (5000U)     /* 过流保护阈值(mA) */
#define SEAT_UNDER_VOLTAGE_MV           (9000U)     /* 欠压阈值(mV) */
#define SEAT_OVER_VOLTAGE_MV            (18000U)    /* 过压阈值(mV) */

/*==================================================================================================
 * Heater Configuration
 *==================================================================================================*/
#define SEAT_HEATER_HIGH_PERCENT        (80U)       /* 加热高档占空比 */
#define SEAT_HEATER_LOW_PERCENT         (40U)       /* 加热低档占空比 */
#define SEAT_HEATER_TIMEOUT_MS          (600000U)   /* 加热超时(10分钟) */
#define SEAT_HEATER_OVER_TEMP_LIMIT     (60U)       /* 过温保护 (°C) */

/*==================================================================================================
 * Memory Configuration
 *==================================================================================================*/
#define SEAT_MEMORY_SLOTS               (2U)        /* 记忆位置数 */
#define SEAT_MEMORY_RECALL_TIMEOUT_MS   (5000U)     /* 记忆调用超时(ms) */
#define SEAT_MEMORY_STORE_RETRIES       (3U)        /* 存储重试次数 */

/*==================================================================================================
 * Motor Configuration
 *==================================================================================================*/
#define SEAT_MOTOR_SPEED_DEFAULT        (60U)       /* 默认电机速度(%) */
#define SEAT_MOTOR_SPEED_MIN            (10U)       /* 最小电机速度(%) */
#define SEAT_MOTOR_SPEED_MAX            (100U)      /* 最大电机速度(%) */
#define SEAT_MOTOR_RAMP_TIME_MS         (200U)      /* 电机软启动时间(ms) */
#define SEAT_MOTOR_STALL_TIMEOUT_MS     (10000U)    /* 堵转保护超时(ms) */

/*==================================================================================================
 * Communication Configuration
 *==================================================================================================*/
#define SEAT_CAN_BASE_ID                (0x500U)    /* CAN报文基ID */
#define SEAT_CAN_STATUS_ID              (0x501U)    /* 状态报文ID */
#define SEAT_CAN_COMMAND_ID             (0x502U)    /* 命令报文ID */
#define SEAT_LIN_SLAVE_ID               (0x01U)     /* LIN从节点ID */
#define SEAT_COMM_TIMEOUT_MS            (100U)      /* 通信超时(ms) */

/*==================================================================================================
 * ADC Mapping (ADC raw → physical units)
 *==================================================================================================*/
#define SEAT_ADC_HORIZONTAL_MM_PER_LSB  (58U)       /* mm per ADC LSB (230mm / 4095 * 1000) ≈ 56, rounded */
#define SEAT_ADC_RECLINE_DEG_PER_LSB    (15U)       /* deg per ADC LSB (60 / 4095 * 1000 ≈ 15) */
#define SEAT_ADC_HEIGHT_MM_PER_LSB      (13U)       /* mm per ADC LSB (50 / 4095 * 1000 ≈ 12, rounded up) */
#define SEAT_ADC_TILT_DEG_PER_LSB       (4U)        /* deg per ADC LSB (15 / 4095 * 1000 ≈ 4) */

/*==================================================================================================
 * Error Codes
 *==================================================================================================*/
#define SEAT_ERR_NONE                   (0x0000U)
#define SEAT_ERR_OVER_CURRENT           (0x0001U)
#define SEAT_ERR_MOTOR_STALL            (0x0002U)
#define SEAT_ERR_LIMIT_SWITCH           (0x0003U)
#define SEAT_ERR_ADC_FAILURE            (0x0004U)
#define SEAT_ERR_CAN_TIMEOUT            (0x0005U)
#define SEAT_ERR_LIN_TIMEOUT            (0x0006U)
#define SEAT_ERR_HEATER_OVERTEMP        (0x0007U)
#define SEAT_ERR_MEMORY_CORRUPT         (0x0008U)
#define SEAT_ERR_FLASH_FAILURE          (0x0009U)
#define SEAT_ERR_UNDER_VOLTAGE          (0x000AU)
#define SEAT_ERR_OVER_VOLTAGE           (0x000BU)
#define SEAT_ERR_INTERNAL               (0x00FFU)

#endif /* SEAT_CFG_H */

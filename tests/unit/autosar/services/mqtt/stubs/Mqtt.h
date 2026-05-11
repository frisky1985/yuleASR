/**
 * @file Mqtt.h
 * @brief MQTT模块测试桩
 */

#ifndef MQTT_H
#define MQTT_H

#include "Std_Types.h"

/*============================================================================
 * 返回类型
 *===========================================================================*/
typedef uint8 Mqtt_ReturnType;

#define MQTT_OK         0x00U
#define MQTT_E_NOT_OK   0x01U
#define MQTT_E_PARAM    0x02U
#define MQTT_E_NOCONN   0x03U
#define MQTT_E_TIMEOUT  0x04U
#define MQTT_E_BUSY     0x05U

/*============================================================================
 * 连接ID类型
 *===========================================================================*/
typedef uint8 Mqtt_ConnectionIdType;
#define MQTT_MAX_CONNECTIONS 4U

/*============================================================================
 * DET错误报告宏
 *===========================================================================*/
#define MQTT_DET_REPORT_ERROR(serviceId, errorId) \
    do { \
        /* 测试中不执行任何操作 */ \
    } while(0)

#endif /* MQTT_H */

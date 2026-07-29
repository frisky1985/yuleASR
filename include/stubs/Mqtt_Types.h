/**
 * @file Mqtt_Types.h
 * @brief MQTT Common Types - shared type definitions to break circular includes
 */
#ifndef MQTT_TYPES_H
#define MQTT_TYPES_H

#include "Std_Types.h"

/* MQTT Return types (forward declaration for Mqtt_Tls.h circular include) */
typedef enum {
    MQTT_OK = 0,
    MQTT_E_NOT_OK,
    MQTT_E_BUSY,
    MQTT_E_TIMEOUT,
    MQTT_E_NOCONN,
    MQTT_E_INVtopic,
    MQTT_E_INVPAYLOAD,
    MQTT_E_BUFFERFULL,
    MQTT_E_DISCONNECTED
} Mqtt_ReturnType;

#endif /* MQTT_TYPES_H */

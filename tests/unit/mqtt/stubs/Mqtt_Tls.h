/**
 * @file Mqtt_Tls.h
 * @brief MQTT TLS module stub for MQTT unit testing
 *
 * Minimal stub so Mqtt.c can compile with TLS support enabled.
 * All TLS operations are stubbed to return success.
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#ifndef MQTT_TLS_STUB_H
#define MQTT_TLS_STUB_H

#include "Std_Types.h"

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/* TLS context type — opaque handle */
typedef void* Mqtt_TlsContextType;

/* TLS protocol version */
typedef enum {
    MQTT_TLS_VERSION_1_2 = 0,
    MQTT_TLS_VERSION_1_3
} Mqtt_TlsVersionType;

/* TLS verification mode */
typedef enum {
    MQTT_TLS_VERIFY_NONE = 0,
    MQTT_TLS_VERIFY_OPTIONAL,
    MQTT_TLS_VERIFY_REQUIRED
} Mqtt_TlsVerifyModeType;

/* TLS security level */
typedef enum {
    MQTT_TLS_SECURITY_LOW = 0,
    MQTT_TLS_SECURITY_MEDIUM,
    MQTT_TLS_SECURITY_HIGH
} Mqtt_TlsSecurityLevelType;

/* TLS configuration structure */
typedef struct {
    Mqtt_TlsVersionType     version;
    Mqtt_TlsVerifyModeType  verifyMode;
    Mqtt_TlsSecurityLevelType securityLevel;
    uint16                  handshakeTimeoutMs;
    boolean                 enableSessionResumption;
    const char*             caCertPath;
    const char*             clientCertPath;
    const char*             clientKeyPath;
} Mqtt_TlsConfigType;

/* MQTT_OK is defined in Mqtt.h — do not redefine here */

/* Stub function declarations */

static inline uint8 Mqtt_Tls_Init(void)
{
    return 0U; /* MQTT_OK */
}

static inline void Mqtt_Tls_DeInit(void)
{
}

static inline uint8 Mqtt_Tls_CreateContext(const Mqtt_TlsConfigType* config,
                                            Mqtt_TlsContextType* context)
{
    (void)config;
    if (context != NULL_PTR)
    {
        *context = (void*)0x1; /* non-NULL dummy handle */
    }
    return 0U; /* MQTT_OK */
}

static inline void Mqtt_Tls_DestroyContext(Mqtt_TlsContextType context)
{
    (void)context;
}

static inline uint8 Mqtt_Tls_PerformHandshake(Mqtt_TlsContextType context,
                                               TcpIp_SocketIdType socketId,
                                               void* timeoutMs)
{
    (void)context;
    (void)socketId;
    (void)timeoutMs;
    return 0U; /* MQTT_OK */
}

static inline uint8 Mqtt_Tls_Send(Mqtt_TlsContextType context,
                                   const uint8* data,
                                   uint32 length,
                                   uint32* sentLength)
{
    (void)context;
    (void)data;
    (void)length;
    if (sentLength != NULL_PTR)
    {
        *sentLength = length;
    }
    return 0U; /* MQTT_OK */
}

static inline uint8 Mqtt_Tls_Receive(Mqtt_TlsContextType context,
                                      uint8* buffer,
                                      uint32 bufferSize,
                                      uint32* receivedLength)
{
    (void)context;
    (void)buffer;
    (void)bufferSize;
    if (receivedLength != NULL_PTR)
    {
        *receivedLength = 0U;
    }
    return 0U; /* MQTT_OK */
}

static inline uint8 Mqtt_Tls_Close(Mqtt_TlsContextType context)
{
    (void)context;
    return 0U; /* MQTT_OK */
}

#endif /* MQTT_TLS_STUB_H */

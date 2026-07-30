#ifndef SOMEIPTP_H
#define SOMEIPTP_H
#include "Std_Types.h"
#include "ComStack_Types.h"

#define SOMEIPTP_VENDOR_ID                  (0x01U)
#define SOMEIPTP_MODULE_ID                  (0x79U)
#define SOMEIPTP_INSTANCE_ID                (0x00U)

typedef uint8 SomeIpTp_ChannelType;
typedef uint8 SomeIpTp_ConnectionType;

typedef struct {
    uint8  ChannelId;
    uint16 MaxMsgLength;
    uint16 MaxBufferLength;
} SomeIpTp_ChannelConfigType;

typedef struct {
    const SomeIpTp_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
    uint8 NumConnections;
} SomeIpTp_ConfigType;

Std_ReturnType SomeIpTp_Init(const SomeIpTp_ConfigType* ConfigPtr);
#endif

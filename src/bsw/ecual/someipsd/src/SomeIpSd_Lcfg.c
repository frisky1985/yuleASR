/**
 * @file SomeIpSd_Lcfg.c
 * @brief SOME/IP Service Discovery Configuration
 */

#include "SomeIpSd.h"
#include "SomeIpSd_Cfg.h"

/* Service Configurations */
static const SomeIpSd_ServiceConfigType SomeIpSd_Services[] = {
    {
        .ServiceId = 0x1234U,      /* Engine Control */
        .InstanceId = 0x0001U,
        .TTL = 3U,
        .IsServer = TRUE,
        .EndpointTcp = 30501U,
        .EndpointUdp = 30502U
    },
    {
        .ServiceId = 0x1235U,      /* Vehicle Status */
        .InstanceId = 0x0001U,
        .TTL = 3U,
        .IsServer = TRUE,
        .EndpointTcp = 0U,
        .EndpointUdp = 30503U
    }
};

const uint16 SomeIpSd_ServiceCount = 2U;

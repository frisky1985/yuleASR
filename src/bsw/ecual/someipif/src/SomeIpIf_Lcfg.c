/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file SomeIpIf_Lcfg.c
 * @brief SOME/IP Interface Configuration Tables
 */

#include "SomeIpIf.h"
#include "SomeIpIf_Cfg.h"

/* Service Endpoint Configurations */
extern const uint16 SomeIpIf_EndpointCount;
extern const uint16 SomeIpIf_ServiceCount;
static const SomeIpIf_EndpointType SomeIpIf_Endpoints[] = {
    {
        .IpAddress = 0xC0A80001U,  /* 192.168.0.1 */
        .Port = 30501U,
        .ConnectionType = SOMEIP_CONNECTION_TCP
    },
    {
        .IpAddress = 0xC0A80002U,  /* 192.168.0.2 */
        .Port = 30502U,
        .ConnectionType = SOMEIP_CONNECTION_UDP
    },
    {
        .IpAddress = 0xC0A8000AU,  /* 192.168.0.10 - Multicast */
        .Port = 30490U,
        .ConnectionType = SOMEIP_CONNECTION_UDP
    }
};

/* Service Configurations */
static const SomeIpIf_ServiceConfigType SomeIpIf_Services[] = {
    {
        .ServiceId = 0x1234U,      /* Engine Control Service */
        .InstanceId = 0x0001U,
        .Endpoint = { 0xC0A80001U, 30501U, 0x00U },
        .IsReliable = TRUE
    },
    {
        .ServiceId = 0x1235U,      /* Vehicle Status Service */
        .InstanceId = 0x0001U,
        .Endpoint = { 0xC0A80001U, 30502U, 0x01U },
        .IsReliable = FALSE
    },
    {
        .ServiceId = 0x1236U,      /* Diagnostic Service */
        .InstanceId = 0x0001U,
        .Endpoint = { 0xC0A80002U, 30503U, 0x00U },
        .IsReliable = TRUE
    }
};

const uint16 SomeIpIf_ServiceCount = 3U;
const uint16 SomeIpIf_EndpointCount = 3U;

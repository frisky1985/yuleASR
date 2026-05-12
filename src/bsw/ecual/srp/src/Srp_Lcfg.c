/**
 * @file Srp_Lcfg.c
 * @brief SRP Configuration Tables
 */

#include "Srp.h"
#include "Srp_Cfg.h"

/* Stream Configurations */
static const Srp_StreamConfigType Srp_Streams[] = {
    {
        .StreamId = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07},
        .StreamVlanId = 100U,
        .Priority = 3U,
        .FrameSize = 1522U,
        .IntervalFrames = 1U,
        .Role = SRP_RESERVE_TALKER
    },
    {
        .StreamId = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08},
        .StreamVlanId = 101U,
        .Priority = 2U,
        .FrameSize = 512U,
        .IntervalFrames = 1U,
        .Role = SRP_RESERVE_LISTENER
    }
};

const uint16 Srp_StreamCount = 2U;

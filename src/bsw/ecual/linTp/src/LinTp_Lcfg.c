/*
 * LinTp_Lcfg.c
 *
 *  Created on: May 5, 2026
 *      Author: YuleTech
 *
 *  AUTOSAR LinTp (LIN Transport Protocol) Link-Time Configuration
 *  Based on ISO 17987-2 Transport Protocol
 *  Following AUTOSAR_SWS_LINTransportProtocol
 */

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
#include "LinTp.h"

/*==================================================================================================
 *                                         CHANNEL CONFIGURATION
 *================================================================================================*/
#define LINTP_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* LinTp Channel 0 Configuration - Master Channel */
static const LinTp_ChannelConfigType LinTp_ChannelConfig_0 =
{
    0U,                         /* ChannelId */
    0U,                         /* LinIfChannelId - Maps to LinIf channel 0 */
    1000U,                      /* N_As - Sender response timeout (1s) */
    1000U,                      /* N_Cs - Sender confirmation timeout (1s) */
    1000U,                      /* N_Cr - Receiver confirmation timeout (1s) */
    8U,                         /* DefaultBs - Block Size (8 frames) */
    20U,                        /* DefaultStMin - Separation time minimum (20ms) */
    TRUE                        /* TransmitCancellation - Enabled */
};

/* LinTp Channel 1 Configuration - Slave Channel */
static const LinTp_ChannelConfigType LinTp_ChannelConfig_1 =
{
    1U,                         /* ChannelId */
    1U,                         /* LinIfChannelId - Maps to LinIf channel 1 */
    1000U,                      /* N_As - Sender response timeout (1s) */
    1000U,                      /* N_Cs - Sender confirmation timeout (1s) */
    1000U,                      /* N_Cr - Receiver confirmation timeout (1s) */
    8U,                         /* DefaultBs - Block Size (8 frames) */
    20U,                        /* DefaultStMin - Separation time minimum (20ms) */
    TRUE                        /* TransmitCancellation - Enabled */
};

/* Channel Configuration Table */
static const LinTp_ChannelConfigType LinTp_ChannelConfig[LINTP_MAX_CHANNEL_COUNT] =
{
    LinTp_ChannelConfig_0,
    LinTp_ChannelConfig_1
};

#define LINTP_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                         NSDU CONFIGURATION
 *================================================================================================*/
#define LINTP_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/* 
 * NSC (Network Service Connection) Configuration
 * 
 * Configuration for Diagnostic Transport Layer connections.
 * Each connection defines a mapping between LinTp N-SDU and PduR N-SDU.
 */

/* Tx N-SDU 0: Master request to slave (Physical addressing) */
static const LinTp_NsduConfigType LinTp_NsduConfig_0 =
{
    0U,                         /* NsduId - LinTp N-SDU ID */
    0U,                         /* ChannelId - Associated channel */
    0U,                         /* PduRNSduId - PduR N-SDU ID */
    0x01U,                      /* Address - Network Source Address (diagnostic master) */
    TRUE                        /* IsTx - Tx connection */
};

/* Rx N-SDU 0: Slave response to master (Physical addressing) */
static const LinTp_NsduConfigType LinTp_NsduConfig_1 =
{
    1U,                         /* NsduId - LinTp N-SDU ID */
    0U,                         /* ChannelId - Associated channel */
    1U,                         /* PduRNSduId - PduR N-SDU ID */
    0x10U,                      /* Address - Network Source Address (diagnostic slave) */
    FALSE                       /* IsTx - Rx connection */
};

/* Tx N-SDU 1: Functional request (broadcast to all slaves) */
static const LinTp_NsduConfigType LinTp_NsduConfig_2 =
{
    2U,                         /* NsduId - LinTp N-SDU ID */
    1U,                         /* ChannelId - Associated channel */
    2U,                         /* PduRNSduId - PduR N-SDU ID */
    0x7EU,                      /* Address - Functional address (broadcast) */
    TRUE                        /* IsTx - Tx connection */
};

/* Rx N-SDU 1: Slave response on second channel */
static const LinTp_NsduConfigType LinTp_NsduConfig_3 =
{
    3U,                         /* NsduId - LinTp N-SDU ID */
    1U,                         /* ChannelId - Associated channel */
    3U,                         /* PduRNSduId - PduR N-SDU ID */
    0x11U,                      /* Address - Network Source Address (diagnostic slave 2) */
    FALSE                       /* IsTx - Rx connection */
};

/* N-SDU Configuration Table */
static const LinTp_NsduConfigType LinTp_NsduConfig[LINTP_NSDU_COUNT] =
{
    LinTp_NsduConfig_0,
    LinTp_NsduConfig_1,
    LinTp_NsduConfig_2,
    LinTp_NsduConfig_3
};

#define LINTP_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                         MODULE CONFIGURATION
 *================================================================================================*/
#define LINTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* Main LinTp Configuration Structure */
const LinTp_ConfigType LinTp_Config =
{
    LinTp_ChannelConfig,        /* ChannelConfig - Channel configuration table */
    LinTp_NsduConfig,           /* NsduConfig - N-SDU configuration table */
    LINTP_MAX_CHANNEL_COUNT,    /* ChannelCount - Number of configured channels */
    LINTP_NSDU_COUNT            /* NsduCount - Number of configured N-SDUs */
};

#define LINTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                         POST-BUILD CONFIGURATION VARIANTS
 *================================================================================================*/
#if defined(LINTP_POSTBUILD_VARIANTS)

/* Variant 1: Standard configuration */
#define LINTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

static const LinTp_ConfigType LinTp_Config_Variant_1 =
{
    LinTp_ChannelConfig,
    LinTp_NsduConfig,
    LINTP_MAX_CHANNEL_COUNT,
    LINTP_NSDU_COUNT
};

#define LINTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* Variant 2: Extended timing configuration */
#define LINTP_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

static const LinTp_ChannelConfigType LinTp_ChannelConfig_Extended_0 =
{
    0U,                         /* ChannelId */
    0U,                         /* LinIfChannelId */
    2000U,                      /* N_As - Extended timeout (2s) */
    2000U,                      /* N_Cs - Extended timeout (2s) */
    2000U,                      /* N_Cr - Extended timeout (2s) */
    16U,                        /* DefaultBs - Larger block size (16 frames) */
    10U,                        /* DefaultStMin - Faster separation (10ms) */
    TRUE                        /* TransmitCancellation */
};

static const LinTp_ChannelConfigType LinTp_ChannelConfig_Extended_1 =
{
    1U,                         /* ChannelId */
    1U,                         /* LinIfChannelId */
    2000U,                      /* N_As - Extended timeout (2s) */
    2000U,                      /* N_Cs - Extended timeout (2s) */
    2000U,                      /* N_Cr - Extended timeout (2s) */
    16U,                        /* DefaultBs - Larger block size (16 frames) */
    10U,                        /* DefaultStMin - Faster separation (10ms) */
    TRUE                        /* TransmitCancellation */
};

static const LinTp_ChannelConfigType LinTp_ChannelConfig_Extended[LINTP_MAX_CHANNEL_COUNT] =
{
    LinTp_ChannelConfig_Extended_0,
    LinTp_ChannelConfig_Extended_1
};

#define LINTP_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

#define LINTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

static const LinTp_ConfigType LinTp_Config_Variant_2 =
{
    LinTp_ChannelConfig_Extended,
    LinTp_NsduConfig,
    LINTP_MAX_CHANNEL_COUNT,
    LINTP_NSDU_COUNT
};

#define LINTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

#endif /* LINTP_POSTBUILD_VARIANTS */

/*==================================================================================================
 *                                         EXPORTED SYMBOLS
 *================================================================================================*/
/* Export configuration pointers for debugging/tooling */
#define LINTP_START_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

static const LinTp_ChannelConfigType* LinTp_GetChannelConfigPtr(void)
{
    return LinTp_ChannelConfig;
}

static const LinTp_NsduConfigType* LinTp_GetNsduConfigPtr(void)
{
    return LinTp_NsduConfig;
}

#define LINTP_STOP_SEC_CONST_UNSPECIFIED
#include "MemMap.h"

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
/* @req SWS_ComM_00001 @req SWS_ComM_00002 @req SWS_ComM_00004 */


/**
 * @file ComM_Lcfg.c
 * @brief AUTOSAR Communication Manager Link-Time Configuration
 * @version 1.0.0
 * 
 * Link-time configuration tables for ComM module.
 * Defines channel configurations, user mappings, and PNC configurations.
 */

#include "ComM.h"
#include "ComM_Cfg.h"

/*=============================================================================
 * Channel Mappings for Users
 *===========================================================================*/

/* DCM User - Diagnostics on CAN0 and ETH0 */
extern const ComM_ConfigType* ComM_ConfigPtr;
extern const ComM_ConfigType ComM_Config;
const ComM_ChannelHandleType ComM_User0_Channels[] = {
    COMM_CHANNEL_CAN0,
    COMM_CHANNEL_ETH0
};

/* DEM User - Error reporting on CAN0 */
const ComM_ChannelHandleType ComM_User1_Channels[] = {
    COMM_CHANNEL_CAN0
};

/* NVM User - Memory operations on CAN0 and LIN0 */
const ComM_ChannelHandleType ComM_User2_Channels[] = {
    COMM_CHANNEL_CAN0,
    COMM_CHANNEL_LIN0
};

/* ECU Manager - System channels */
const ComM_ChannelHandleType ComM_User3_Channels[] = {
    COMM_CHANNEL_CAN0,
    COMM_CHANNEL_CAN1,
    COMM_CHANNEL_ETH0,
    COMM_CHANNEL_LIN0
};

/* SWC0 - Application component on CAN0 and ETH0 */
const ComM_ChannelHandleType ComM_User4_Channels[] = {
    COMM_CHANNEL_CAN0,
    COMM_CHANNEL_ETH0
};

/* SWC1 - Application component on CAN1 and LIN0 */
const ComM_ChannelHandleType ComM_User5_Channels[] = {
    COMM_CHANNEL_CAN1,
    COMM_CHANNEL_LIN0
};

/* DIAG User - Extended diagnostics on CAN0 and CAN1 */
const ComM_ChannelHandleType ComM_User6_Channels[] = {
    COMM_CHANNEL_CAN0,
    COMM_CHANNEL_CAN1
};

/* APPL User - All channels for application */
const ComM_ChannelHandleType ComM_User7_Channels[] = {
    COMM_CHANNEL_CAN0,
    COMM_CHANNEL_CAN1,
    COMM_CHANNEL_ETH0,
    COMM_CHANNEL_LIN0
};

/*=============================================================================
 * PNC Mappings for Users
 *===========================================================================*/

/* DCM User - Associated with PNC_0 */
const ComM_PncHandleType ComM_User0_Pncs[] = {
    COMM_PNC_0
};

/* DEM User - No PNC association */
const ComM_PncHandleType ComM_User1_Pncs[] = {
    /* Empty - DEM not associated with PNCs */
};

/* NVM User - Associated with PNC_1 */
const ComM_PncHandleType ComM_User2_Pncs[] = {
    COMM_PNC_1
};

/* ECU Manager - System-wide, no specific PNC */
const ComM_PncHandleType ComM_User3_Pncs[] = {
    COMM_PNC_0,
    COMM_PNC_1
};

/* SWC0 - Associated with PNC_0 */
const ComM_PncHandleType ComM_User4_Pncs[] = {
    COMM_PNC_0
};

/* SWC1 - Associated with PNC_1 */
const ComM_PncHandleType ComM_User5_Pncs[] = {
    COMM_PNC_1
};

/* DIAG User - Associated with PNC_0 */
const ComM_PncHandleType ComM_User6_Pncs[] = {
    COMM_PNC_0
};

/* APPL User - All PNCs */
const ComM_PncHandleType ComM_User7_Pncs[] = {
    COMM_PNC_0,
    COMM_PNC_1
};

/*=============================================================================
 * PNC Channel Mappings
 *===========================================================================*/

/* PNC_0 - Powertrain Network Cluster
 * Associated with CAN0 (primary) and ETH0 (gateway)
 */
const ComM_PncChannelMappingType ComM_Pnc0_Channels[] = {
    {
        COMM_CHANNEL_CAN0,      /* Primary channel */
        TRUE                    /* Is requester */
    },
    {
        COMM_CHANNEL_ETH0,      /* Gateway channel */
        FALSE                   /* Is not requester (follows PNC) */
    }
};

/* PNC_1 - Body Network Cluster
 * Associated with CAN1 (primary) and LIN0 (slave)
 */
const ComM_PncChannelMappingType ComM_Pnc1_Channels[] = {
    {
        COMM_CHANNEL_CAN1,      /* Primary channel */
        TRUE                    /* Is requester */
    },
    {
        COMM_CHANNEL_LIN0,      /* Slave channel */
        FALSE                   /* Is not requester (follows PNC) */
    }
};

/*=============================================================================
 * Channel Configuration Table
 *===========================================================================*/
const ComM_ChannelConfigType ComM_ChannelConfig[COMM_NUM_CHANNELS] = {
    /* CAN0 - Primary CAN channel for diagnostics and powertrain */
    {
        COMM_CHANNEL_CAN0,              /* ChannelId */
        COMM_BUS_TYPE_CAN,              /* BusType */
        TRUE,                           /* WakeUpSupport */
        TRUE,                           /* DcmSupport */
        TRUE,                           /* PassiveWakeUp */
        1000U,                          /* NoComTimeout (ms) */
        500U,                           /* SilentTimeout (ms) */
        200U,                           /* FullComTimeout (ms) */
        50U                             /* WakeUpDelay (ms) */
    },
    /* CAN1 - Secondary CAN channel for body electronics */
    {
        COMM_CHANNEL_CAN1,              /* ChannelId */
        COMM_BUS_TYPE_CAN,              /* BusType */
        TRUE,                           /* WakeUpSupport */
        FALSE,                          /* DcmSupport */
        TRUE,                           /* PassiveWakeUp */
        1000U,                          /* NoComTimeout (ms) */
        500U,                           /* SilentTimeout (ms) */
        200U,                           /* FullComTimeout (ms) */
        50U                             /* WakeUpDelay (ms) */
    },
    /* ETH0 - Ethernet channel for high-bandwidth communication */
    {
        COMM_CHANNEL_ETH0,              /* ChannelId */
        COMM_BUS_TYPE_ETH,              /* BusType */
        FALSE,                          /* WakeUpSupport - ETH typically no wake-up */
        TRUE,                           /* DcmSupport */
        FALSE,                          /* PassiveWakeUp */
        800U,                           /* NoComTimeout (ms) */
        400U,                           /* SilentTimeout (ms) */
        150U,                           /* FullComTimeout (ms) */
        30U                             /* WakeUpDelay (ms) */
    },
    /* LIN0 - LIN bus for low-cost peripherals */
    {
        COMM_CHANNEL_LIN0,              /* ChannelId */
        COMM_BUS_TYPE_LIN,              /* BusType */
        TRUE,                           /* WakeUpSupport */
        FALSE,                          /* DcmSupport */
        TRUE,                           /* PassiveWakeUp */
        1200U,                          /* NoComTimeout (ms) */
        600U,                           /* SilentTimeout (ms) */
        250U,                           /* FullComTimeout (ms) */
        70U                             /* WakeUpDelay (ms) */
    }
};

/*=============================================================================
 * User Configuration Table
 *===========================================================================*/
const ComM_UserConfigType ComM_UserConfig[COMM_NUM_USERS] = {
    /* COMM_USER_DCM - DCM User for diagnostics */
    {
        COMM_USER_DCM,                  /* UserId */
        ComM_User0_Channels,            /* ChannelMap */
        2U,                             /* NumChannels */
        ComM_User0_Pncs,                /* PncMap */
        1U                              /* NumPncs */
    },
    /* COMM_USER_DEM - DEM User for error management */
    {
        COMM_USER_DEM,                  /* UserId */
        ComM_User1_Channels,            /* ChannelMap */
        1U,                             /* NumChannels */
        ComM_User1_Pncs,                /* PncMap */
        0U                              /* NumPncs */
    },
    /* COMM_USER_NVM - NVM User for memory management */
    {
        COMM_USER_NVM,                  /* UserId */
        ComM_User2_Channels,            /* ChannelMap */
        2U,                             /* NumChannels */
        ComM_User2_Pncs,                /* PncMap */
        1U                              /* NumPncs */
    },
    /* COMM_USER_ECUM - ECU Manager for system control */
    {
        COMM_USER_ECUM,                 /* UserId */
        ComM_User3_Channels,            /* ChannelMap */
        4U,                             /* NumChannels */
        ComM_User3_Pncs,                /* PncMap */
        2U                              /* NumPncs */
    },
    /* COMM_USER_SWC0 - Software Component 0 */
    {
        COMM_USER_SWC0,                 /* UserId */
        ComM_User4_Channels,            /* ChannelMap */
        2U,                             /* NumChannels */
        ComM_User4_Pncs,                /* PncMap */
        1U                              /* NumPncs */
    },
    /* COMM_USER_SWC1 - Software Component 1 */
    {
        COMM_USER_SWC1,                 /* UserId */
        ComM_User5_Channels,            /* ChannelMap */
        2U,                             /* NumChannels */
        ComM_User5_Pncs,                /* PncMap */
        1U                              /* NumPncs */
    },
    /* COMM_USER_DIAG - Extended Diagnostics User */
    {
        COMM_USER_DIAG,                 /* UserId */
        ComM_User6_Channels,            /* ChannelMap */
        2U,                             /* NumChannels */
        ComM_User6_Pncs,                /* PncMap */
        1U                              /* NumPncs */
    },
    /* COMM_USER_APPL - General Application User */
    {
        COMM_USER_APPL,                 /* UserId */
        ComM_User7_Channels,            /* ChannelMap */
        4U,                             /* NumChannels */
        ComM_User7_Pncs,                /* PncMap */
        2U                              /* NumPncs */
    }
};

/*=============================================================================
 * PNC Configuration Table
 *===========================================================================*/
#if (COMM_PNC_SUPPORT == STD_ON)
const ComM_PncConfigType ComM_PncConfig[COMM_NUM_PNCS] = {
    /* COMM_PNC_0 - Powertrain Network Cluster */
    {
        COMM_PNC_0,                     /* PncId */
        ComM_Pnc0_Channels,             /* ChannelMap */
        2U,                             /* NumChannels */
        300U,                           /* PrepareSleepTimeout (ms) */
        100U,                           /* RequestTimeout (ms) */
        TRUE                            /* WakeUpSupport */
    },
    /* COMM_PNC_1 - Body Network Cluster */
    {
        COMM_PNC_1,                     /* PncId */
        ComM_Pnc1_Channels,             /* ChannelMap */
        2U,                             /* NumChannels */
        300U,                           /* PrepareSleepTimeout (ms) */
        100U,                           /* RequestTimeout (ms) */
        TRUE                            /* WakeUpSupport */
    }
};
#endif /* COMM_PNC_SUPPORT */

/*=============================================================================
 * Module Configuration Instance
 *===========================================================================*/
const ComM_ConfigType ComM_Config = {
    ComM_ChannelConfig,                 /* ChannelConfigs */
    ComM_UserConfig,                    /* UserConfigs */
#if (COMM_PNC_SUPPORT == STD_ON)
    ComM_PncConfig,                     /* PncConfigs */
#else
    NULL_PTR,
#endif
    COMM_NUM_CHANNELS,                  /* NumChannels */
    COMM_NUM_USERS,                     /* NumUsers */
    COMM_NUM_PNCS,                      /* NumPncs */
    50U,                                /* BusWakeUpDelay (ms) */
#if (COMM_PNC_SUPPORT == STD_ON)
    TRUE,                               /* PncSupportEnabled */
#else
    FALSE,
#endif
#if (COMM_DCM_SUPPORT == STD_ON)
    TRUE,                               /* DcmSupportEnabled */
#else
    FALSE,
#endif
#if (COMM_ECUM_SUPPORT == STD_ON)
    TRUE                                /* EcuMSupportEnabled */
#else
    FALSE,
#endif
};

/* Pointer to active configuration */
const ComM_ConfigType* ComM_ConfigPtr = &ComM_Config;

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Sd_Cfg.h
 * @brief Service Discovery Pre-Compile Configuration
 * @version 1.0.0
 */

#ifndef SD_CFG_H
#define SD_CFG_H

/*==================================================================================================
 *                                    DEVELOPMENT ERROR DETECT
 *==================================================================================================*/
#define SD_DEV_ERROR_DETECT                     (STD_ON)
#define SD_VERSION_INFO_API                     (STD_ON)

/*==================================================================================================
 *                                    CAPACITY CONFIGURATION
 *==================================================================================================*/
#define SD_MAX_OFFERED_SERVICES                 (16U)
#define SD_MAX_FOUND_SERVICES                   (32U)
#define SD_MAX_SUBSCRIPTIONS                    (16U)
#define SD_MAX_EVENT_GROUPS                     (16U)

/*==================================================================================================
 *                                    TIMING CONFIGURATION
 *==================================================================================================*/
#define SD_OFFER_CYCLE_TIME_MS                  (2000U)   /* 2 s */
#define SD_FIND_CYCLE_TIME_MS                   (3000U)   /* 3 s */
#define SD_TTL_DEFAULT_SEC                      (5U)      /* 5 s  */
#define SD_MAIN_FUNCTION_PERIOD_MS              (10U)
#define SD_INITIAL_DELAY_MS                     (500U)    /* 500 ms initial delay */

/*==================================================================================================
 *                                    SD MESSAGE CONSTANTS
 *==================================================================================================*/
#define SD_SOMEIP_SERVICE_ID                    (0xFFFFU)
#define SD_SOMEIP_METHOD_ID                     (0x8100U)
#define SD_MAX_SD_MSG_LENGTH                    (1400U)

/*==================================================================================================
 *                                    NETWORK CONFIGURATION
 *==================================================================================================*/
#define SD_MULTICAST_IPV4_ADDR                  ((uint32)0xEFFFFFFFU)  /* 239.255.255.255 */
#define SD_MULTICAST_PORT                       (30490U)
#define SD_UNICAST_PORT                         (30491U)

#endif /* SD_CFG_H */

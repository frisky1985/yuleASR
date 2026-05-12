/**
 * @file SomeIpSd_Cfg.h
 * @brief SOME/IP Service Discovery Configuration
 */

#ifndef SOMEIPSD_CFG_H
#define SOMEIPSD_CFG_H

#define SOMEIPSD_DEV_ERROR_DETECT       STD_ON
#define SOMEIPSD_VERSION_INFO_API       STD_ON

/* SD Protocol Constants */
#define SOMEIPSD_PROTOCOL_VERSION       0x01U
#define SOMEIPSD_INTERFACE_VERSION      0x01U

/* SD Header Size */
#define SOMEIPSD_HEADER_SIZE            12U

/* Entry Length */
#define SOMEIPSD_ENTRY_LENGTH           16U

/* Maximum Services */
#define SOMEIPSD_MAX_SERVICES           32U

/* Maximum Subscriptions */
#define SOMEIPSD_MAX_SUBSCRIPTIONS      64U

/* Timing Parameters */
#define SOMEIPSD_INITIAL_DELAY_MIN_MS   100U
#define SOMEIPSD_INITIAL_DELAY_MAX_MS   500U
#define SOMEIPSD_REPETITIONS_MAX        3U
#define SOMEIPSD_CYCLIC_OFFER_DELAY_MS  3000U

/* Multicast Address */
#define SOMEIPSD_MULTICAST_IP           0xEFFF0001U  /* 239.255.0.1 */
#define SOMEIPSD_MULTICAST_PORT         30490U

#endif

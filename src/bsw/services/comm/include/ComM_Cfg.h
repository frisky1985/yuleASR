/**
 * @file ComM_Cfg.h
 * @brief Communication Manager Configuration
 */

#ifndef COMM_CFG_H
#define COMM_CFG_H

#include "ComM.h"

/* Development Error Detection */
#define COMM_DEV_ERROR_DETECT               STD_ON
#define COMM_VERSION_INFO_API               STD_ON

/* Number of Users */
#define COMM_MAX_USERS                      32

/* Number of Channels */
#define COMM_MAX_CHANNELS                   8

/* Main Function Period (ms) */
#define COMM_MAIN_FUNCTION_PERIOD           10

/* Timeouts (in ms) */
#define COMM_NM_TIMEOUT                     1000
#define COMM_T_MIN_FULL_COM_MODE            100
#define COMM_T_MAX_NOMODE                   1000

/* Wake Up Support */
#define COMM_WAKEUP_INHIBITION_ENABLED      STD_ON
#define COMM_MODE_LIMITATION_ENABLED        STD_ON

/* DCM Support */
#define COMM_DCM_SUPPORT                    STD_ON

/* ECU Group Classification */
#define COMM_ECU_GROUP_CLASSIFICATION       0x00

/* Channel Configuration */
#define COMM_CHANNEL_CAN0                   0x00
#define COMM_CHANNEL_CAN1                   0x01
#define COMM_CHANNEL_LIN0                   0x02
#define COMM_CHANNEL_ETH0                   0x03

/* User Configuration */
#define COMM_USER_DCM                       0x00
#define COMM_USER_ECUM                      0x01
#define COMM_USER_BSWM                      0x02

#endif /* COMM_CFG_H */

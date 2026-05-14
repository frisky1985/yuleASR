/**
 * @file Com_Cfg.h
 * @brief COM Configuration
 */

#ifndef COM_CFG_H
#define COM_CFG_H

#define COM_DEV_ERROR_DETECT            STD_ON
#define COM_VERSION_INFO_API            STD_ON

/* Maximum Counts */
#define COM_MAX_SIGNALS                 256U
#define COM_MAX_IPDUS                   64U
#define COM_MAX_GROUPS                  16U

/* Signal Limits */
#define COM_MAX_SIGNAL_LENGTH           64U

/* Transmission Modes */
#define COM_TX_MODE_DIRECT              0x00U
#define COM_TX_MODE_PERIODIC            0x01U
#define COM_TX_MODE_MIXED               0x02U

/* Byte Order */
#define COM_LITTLE_ENDIAN               0x00U
#define COM_BIG_ENDIAN                  0x01U

#endif

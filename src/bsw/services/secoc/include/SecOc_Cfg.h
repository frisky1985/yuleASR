/**
 * @file SecOc_Cfg.h
 * @brief SecOc Configuration
 */

#ifndef SECOC_CFG_H
#define SECOC_CFG_H

#define SECOC_DEV_ERROR_DETECT          STD_ON
#define SECOC_VERSION_INFO_API          STD_ON

/* Security Settings */
#define SECOC_MAX_PDUS                  64U
#define SECOC_MAX_FRESHNESS_VALUES      32U

/* Authenticator Lengths */
#define SECOC_AUTH_LENGTH_4             4U
#define SECOC_AUTH_LENGTH_8             8U
#define SECOC_AUTH_LENGTH_16            16U

/* Freshness Value Settings */
#define SECOC_FRESHNESS_LENGTH_3        3U
#define SECOC_FRESHNESS_LENGTH_4        4U
#define SECOC_FRESHNESS_LENGTH_8        8U

/* Tripple Freshness */
#define SECOC_USE_TRIPPLE_FRESHNESS     STD_ON

#endif

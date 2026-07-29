/** @file WdgIf_Cfg.h
 * @brief Watchdog Interface configuration header
 */

#ifndef WDGIF_CFG_H
#define WDGIF_CFG_H

/*============================================================================
 *  GENERAL CONFIGURATION
 *===========================================================================*/

/** @brief Development error detection enable/disable */
#define WDGIF_DEV_ERROR_DETECT              STD_ON

/** @brief Version info API enable/disable */
#define WDGIF_VERSION_INFO_API              STD_ON

/** @brief Number of watchdog devices */
#define WDGIF_NUMBER_OF_DEVICES             1

/** @brief Default device index */
#define WDGIF_DEFAULT_DEVICE                0

/*============================================================================
 *  TIMEOUT CONFIGURATION (milliseconds)
 *===========================================================================*/

/** @brief Fast mode timeout */
#define WDGIF_FAST_MODE_TIMEOUT             10

/** @brief Slow mode timeout */
#define WDGIF_SLOW_MODE_TIMEOUT             100

/** @brief Maximum trigger interval in fast mode */
#define WDGIF_FAST_MODE_TRIGGER_MS          5

/** @brief Maximum trigger interval in slow mode */
#define WDGIF_SLOW_MODE_TRIGGER_MS          50

/*============================================================================
 *  CALLBACK CONFIGURATION
 *===========================================================================*/

/** @brief Enable/disable mode switch notification */
#define WDGIF_MODE_CHANGE_CB_ENABLED        STD_OFF

/** @brief Enable/disable trigger notification */
#define WDGIF_TRIGGER_CB_ENABLED            STD_OFF

#endif /* WDGIF_CFG_H */

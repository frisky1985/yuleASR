/**
 * @file StbM_Cfg.h
 * @brief Synchronized Time-base Manager configuration header - AutoSAR R22-11
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef STBM_CFG_H
#define STBM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define STBM_DEV_ERROR_DETECT                   (STD_ON)
#define STBM_VERSION_INFO_API                   (STD_ON)

/*==================================================================================================
*                                    TIME BASE CONFIGURATION
==================================================================================================*/
#define STBM_NUMBER_OF_TIMEBASES                (4U)

/*==================================================================================================
*                                    TIME SYNC CONFIGURATION
==================================================================================================*/
#define STBM_ENABLE_GPTP                        (STD_ON)
#define STBM_ENABLE_PTP                         (STD_OFF)
#define STBM_ENABLE_CAN_SYNC                    (STD_OFF)
#define STBM_ENABLE_FR_SYNC                     (STD_OFF)

/*==================================================================================================
*                                    TIME BASE IDs
==================================================================================================*/
#define STBM_TIMEBASE_ID_0                      (0U)
#define STBM_TIMEBASE_ID_1                      (1U)
#define STBM_TIMEBASE_ID_2                      (2U)
#define STBM_TIMEBASE_ID_3                      (3U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/
#define STBM_SYNC_TIMEOUT_MS                    (1000U)
#define STBM_UPDATE_FREQ_MS                     (10U)

/*==================================================================================================
*                                    RATE CORRECTION CONFIGURATION
==================================================================================================*/
#define STBM_ALLOWED_RATE_DEVIATION_PPM         (100000)  /* 100 ppm */
#define STBM_MAX_RATE_CORRECTION_PPM            (500000)  /* 500 ppm */

/*==================================================================================================
*                                    ETHERNET CONFIGURATION
==================================================================================================*/
#define STBM_ETH_CONTROLLER_0                   (0U)
#define STBM_ETH_CONTROLLER_1                   (1U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define STBM_MAIN_FUNCTION_PERIOD_MS            (10U)

#endif /* STBM_CFG_H */

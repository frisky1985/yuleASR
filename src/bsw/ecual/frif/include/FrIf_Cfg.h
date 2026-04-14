/**
 * @file FrIf_Cfg.h
 * @brief FlexRay Interface configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef FRIF_CFG_H
#define FRIF_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define FRIF_DEV_ERROR_DETECT           (STD_ON)
#define FRIF_VERSION_INFO_API           (STD_ON)
#define FRIF_CLST_STARTUP_ACTIVE        (STD_ON)
#define FRIF_CLST_WAKEUP_ACTIVE         (STD_ON)
#define FRIF_GET_WUP_RX_STATUS_SUPPORT  (STD_ON)
#define FRIF_GET_SYNC_FRAME_LIST_SUPPORT (STD_ON)
#define FRIF_GET_CLOCK_CORRECTION_SUPPORT (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CONTROLLERS
==================================================================================================*/
#define FRIF_NUM_CONTROLLERS            (1U)
#define FRIF_NUM_LPDUS                  (64U)
#define FRIF_NUM_CLUSTERS               (1U)

/*==================================================================================================
*                                    CONTROLLER DEFINITIONS
==================================================================================================*/
#define FRIF_CONTROLLER_0               (0U)

/*==================================================================================================
*                                    CLUSTER CONFIGURATION
==================================================================================================*/
#define FRIF_CLUSTER_0                  (0U)
#define FRIF_GD_CYCLE                   (5000U)      /* 5ms cycle */
#define FRIF_GD_MACROTICK               (1U)         /* 1us macrotick */
#define FRIF_GD_STATIC_SLOT             (61U)        /* Static slot */
#define FRIF_GD_NUMBER_OF_STATIC_SLOTS  (94U)
#define FRIF_GD_MINISLOT                (7U)
#define FRIF_GD_NUMBER_OF_MINISLOTS     (129U)

/*==================================================================================================
*                                    LPDU DEFINITIONS
==================================================================================================*/
#define FRIF_LPDU_ENGINE_STATUS         (0U)
#define FRIF_LPDU_VEHICLE_SPEED         (1U)
#define FRIF_LPDU_STEERING_ANGLE        (2U)
#define FRIF_LPDU_BRAKE_STATUS          (3U)

/*==================================================================================================
*                                    SLOT IDs
==================================================================================================*/
#define FRIF_SLOT_ID_1                  (1U)
#define FRIF_SLOT_ID_2                  (2U)
#define FRIF_SLOT_ID_3                  (3U)
#define FRIF_SLOT_ID_4                  (4U)

/*==================================================================================================
*                                    CHANNEL
==================================================================================================*/
#define FRIF_CHANNEL_A                  (0U)
#define FRIF_CHANNEL_B                  (1U)
#define FRIF_CHANNEL_AB                 (2U)

/*==================================================================================================
*                                    PAYLOAD LENGTH
==================================================================================================*/
#define FRIF_PAYLOAD_LENGTH_MAX         (254U)
#define FRIF_PAYLOAD_LENGTH_DEFAULT     (16U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define FRIF_MAIN_FUNCTION_PERIOD_MS    (1U)

/*==================================================================================================
*                                    WAKEUP
==================================================================================================*/
#define FRIF_WAKEUP_SUPPORT             (STD_ON)
#define FRIF_WAKEUP_CHANNEL             (FRIF_CHANNEL_A)

/*==================================================================================================
*                                    COLDSTART
==================================================================================================*/
#define FRIF_COLDSTART_SUPPORT          (STD_ON)
#define FRIF_COLDSTART_NODES            (2U)

#endif /* FRIF_CFG_H */

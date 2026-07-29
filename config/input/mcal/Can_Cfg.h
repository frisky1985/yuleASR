/**
 * @file Can_Cfg.h
 * @brief CAN Driver configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef CAN_CFG_H
#define CAN_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define CAN_DEV_ERROR_DETECT            (STD_ON)
#define CAN_VERSION_INFO_API            (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CAN CONTROLLERS
==================================================================================================*/
#define CAN_NUM_CONTROLLERS             (2U)

/*==================================================================================================
*                                    NUMBER OF HARDWARE OBJECTS
==================================================================================================*/
#define CAN_NUM_HOH                     (16U)

/*==================================================================================================
*                                    BAUDRATE CONFIGURATIONS
==================================================================================================*/
#define CAN_NUM_BAUDRATE_CONFIGS        (3U)

/* Baudrate Indexes */
#define CAN_BAUDRATE_500K               (0U)
#define CAN_BAUDRATE_250K               (1U)
#define CAN_BAUDRATE_125K               (2U)

/*==================================================================================================
*                                    PROCESSING MODES
==================================================================================================*/
#define CAN_PROCESSING_INTERRUPT        (0U)
#define CAN_PROCESSING_POLLING          (1U)

/*==================================================================================================
*                                    CONTROLLER DEFINITIONS
==================================================================================================*/
#define CAN_CONTROLLER_0                (0U)
#define CAN_CONTROLLER_1                (1U)

/*==================================================================================================
*                                    HARDWARE OBJECT HANDLES
==================================================================================================*/
#define CAN_HOH_RX_0                    ((Can_HwHandleType)0x0000U)
#define CAN_HOH_RX_1                    ((Can_HwHandleType)0x0001U)
#define CAN_HOH_RX_2                    ((Can_HwHandleType)0x0002U)
#define CAN_HOH_RX_3                    ((Can_HwHandleType)0x0003U)
#define CAN_HOH_TX_0                    ((Can_HwHandleType)0x0004U)
#define CAN_HOH_TX_1                    ((Can_HwHandleType)0x0005U)
#define CAN_HOH_TX_2                    ((Can_HwHandleType)0x0006U)
#define CAN_HOH_TX_3                    ((Can_HwHandleType)0x0007U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/
#define CAN_TIMEOUT_DURATION            (10000U)  /* 10ms timeout */

/*==================================================================================================
*                                    MAIN FUNCTION PERIODS
==================================================================================================*/
#define CAN_MAIN_FUNCTION_PERIOD_MS     (10U)

#endif /* CAN_CFG_H */

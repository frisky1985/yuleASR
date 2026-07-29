/**
 * @file J1939Nm_Cfg.h
 * @brief J1939 Network Management configuration header
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef J1939NM_CFG_H
#define J1939NM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define J1939NM_DEV_ERROR_DETECT            (STD_ON)
#define J1939NM_VERSION_INFO_API            (STD_ON)
#define J1939NM_NODE_DETECTION_ENABLED      (STD_ON)
#define J1939NM_NODE_MONITORING_ENABLED     (STD_ON)
#define J1939NM_BUS_OFF_RECOVERY_ENABLED    (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define J1939NM_NUMBER_OF_CHANNELS          (1U)
#define J1939NM_NUMBER_OF_NODES             (1U)

/*==================================================================================================
*                                    CHANNEL IDs
==================================================================================================*/
#define J1939NM_CHANNEL_0                   (0U)
#define J1939NM_NODE_0                      (0U)

/*==================================================================================================
*                                    ADDRESS CLAIMING TIMING
==================================================================================================*/
#define J1939NM_AC_DELAY_MIN_MS             (50U)   /*!< Minimum address claim delay in ms */
#define J1939NM_AC_DELAY_MAX_MS             (150U)  /*!< Maximum address claim delay in ms */
#define J1939NM_AC_TIMEOUT_MS               (250U)  /*!< Address claim timeout in ms */
#define J1939NM_BUS_OFF_RECOVERY_TIME_MS    (1000U) /*!< Bus-off recovery time in ms */
#define J1939NM_AC_REPEAT_TIME_MS           (1000U) /*!< Address claim repeat time in ms */

/*==================================================================================================
*                                    ADDRESS DEFINITIONS
==================================================================================================*/
#define J1939NM_NULL_ADDRESS                (254U)  /*!< NULL address */
#define J1939NM_GLOBAL_ADDRESS              (255U)  /*!< Global address (broadcast) */
#define J1939NM_MIN_VALID_ADDRESS           (0U)    /*!< Minimum valid address */
#define J1939NM_MAX_VALID_ADDRESS           (253U)  /*!< Maximum valid address */

/*==================================================================================================
*                                    NAME FIELD MASKS
==================================================================================================*/
#define J1939NM_NAME_IDENTITY_NUMBER_MASK   (0x00000000001FFFFFULL) /*!< Identity Number (21 bits) */
#define J1939NM_NAME_MANUFACTURER_CODE_MASK (0x00000007FFE00000ULL) /*!< Manufacturer Code (11 bits) */
#define J1939NM_NAME_ECU_INSTANCE_MASK      (0x0000003800000000ULL) /*!< ECU Instance (3 bits) */
#define J1939NM_NAME_FUNCTION_INSTANCE_MASK (0x000007C000000000ULL) /*!< Function Instance (5 bits) */
#define J1939NM_NAME_FUNCTION_MASK          (0x0007F80000000000ULL) /*!< Function (8 bits) */
#define J1939NM_NAME_RESERVED_MASK          (0x0008000000000000ULL) /*!< Reserved (1 bit) */
#define J1939NM_NAME_VEHICLE_SYSTEM_MASK    (0x07F0000000000000ULL) /*!< Vehicle System (7 bits) */
#define J1939NM_NAME_VEHICLE_SYSTEM_INSTANCE_MASK (0x7800000000000000ULL) /*!< Vehicle System Instance (4 bits) */
#define J1939NM_NAME_INDUSTRY_GROUP_MASK    (0x8000000000000000ULL) /*!< Industry Group (3 bits) - partially overlaps */
#define J1939NM_NAME_ARBITRARY_ADDRESS_CAPABLE_MASK (0x8000000000000000ULL) /*!< Arbitrary Address Capable (1 bit) */

/*==================================================================================================
*                                    NAME FIELD SHIFTS
==================================================================================================*/
#define J1939NM_NAME_IDENTITY_NUMBER_SHIFT  (0U)
#define J1939NM_NAME_MANUFACTURER_CODE_SHIFT (21U)
#define J1939NM_NAME_ECU_INSTANCE_SHIFT     (32U)
#define J1939NM_NAME_FUNCTION_INSTANCE_SHIFT (35U)
#define J1939NM_NAME_FUNCTION_SHIFT         (40U)
#define J1939NM_NAME_RESERVED_SHIFT         (48U)
#define J1939NM_NAME_VEHICLE_SYSTEM_SHIFT   (49U)
#define J1939NM_NAME_VEHICLE_SYSTEM_INSTANCE_SHIFT (56U)
#define J1939NM_NAME_INDUSTRY_GROUP_SHIFT   (60U)
#define J1939NM_NAME_ARBITRARY_ADDRESS_CAPABLE_SHIFT (63U)

/*==================================================================================================
*                                    DEFAULT NAME CONFIGURATION
==================================================================================================*/
#define J1939NM_DEFAULT_IDENTITY_NUMBER     (0x00001U)      /*!< Identity Number */
#define J1939NM_DEFAULT_MANUFACTURER_CODE   (0x7D1U)        /*!< Manufacturer Code (YuleTech: 2001) */
#define J1939NM_DEFAULT_ECU_INSTANCE        (0x0U)          /*!< ECU Instance */
#define J1939NM_DEFAULT_FUNCTION_INSTANCE   (0x00U)         /*!< Function Instance */
#define J1939NM_DEFAULT_FUNCTION            (0x00U)         /*!< Function (Unspecified) */
#define J1939NM_DEFAULT_VEHICLE_SYSTEM      (0x00U)         /*!< Vehicle System (Unspecified) */
#define J1939NM_DEFAULT_VEHICLE_SYSTEM_INSTANCE (0x0U)      /*!< Vehicle System Instance */
#define J1939NM_DEFAULT_INDUSTRY_GROUP      (0x0U)          /*!< Industry Group (Global) */
#define J1939NM_DEFAULT_ARBITRARY_ADDRESS_CAPABLE (STD_ON)  /*!< Arbitrary Address Capable */

/*==================================================================================================
*                                    DEFAULT ADDRESS
==================================================================================================*/
#define J1939NM_DEFAULT_ADDRESS             (0x80U)         /*!< Default address (128) */
#define J1939NM_DEFAULT_PREFERRED_ADDRESS   (0x80U)         /*!< Preferred address (128) */

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define J1939NM_MAIN_FUNCTION_PERIOD_MS     (10U)           /*!< Main function period in ms */

#endif /* J1939NM_CFG_H */

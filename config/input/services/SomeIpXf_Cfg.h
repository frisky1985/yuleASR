/**
 * @file SomeIpXf_Cfg.h
 * @brief SOME/IP Transformer configuration header - AutoSAR R22-11
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef SOMEIPXF_CFG_H
#define SOMEIPXF_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define SOMEIPXF_DEV_ERROR_DETECT               (STD_ON)
#define SOMEIPXF_VERSION_INFO_API               (STD_ON)

/*==================================================================================================
*                                    TRANSFORMER CONFIGURATION
==================================================================================================*/
#define SOMEIPXF_NUMBER_OF_TRANSFORMERS         (8U)
#define SOMEIPXF_NUMBER_OF_DATA_ELEMENTS        (32U)

/*==================================================================================================
*                                    BUFFER CONFIGURATION
==================================================================================================*/
#define SOMEIPXF_MAX_BUFFER_SIZE                (1400U)
#define SOMEIPXF_MAX_STRING_LENGTH              (256U)
#define SOMEIPXF_MAX_ARRAY_ELEMENTS             (64U)

/*==================================================================================================
*                                    DATA TYPE CONFIGURATION
==================================================================================================*/
#define SOMEIPXF_ENABLE_BOOLEAN                 (STD_ON)
#define SOMEIPXF_ENABLE_UINT8                   (STD_ON)
#define SOMEIPXF_ENABLE_UINT16                  (STD_ON)
#define SOMEIPXF_ENABLE_UINT32                  (STD_ON)
#define SOMEIPXF_ENABLE_UINT64                  (STD_ON)
#define SOMEIPXF_ENABLE_SINT8                   (STD_ON)
#define SOMEIPXF_ENABLE_SINT16                  (STD_ON)
#define SOMEIPXF_ENABLE_SINT32                  (STD_ON)
#define SOMEIPXF_ENABLE_SINT64                  (STD_ON)
#define SOMEIPXF_ENABLE_FLOAT32                 (STD_ON)
#define SOMEIPXF_ENABLE_FLOAT64                 (STD_ON)
#define SOMEIPXF_ENABLE_STRING                  (STD_ON)
#define SOMEIPXF_ENABLE_ARRAY                   (STD_ON)
#define SOMEIPXF_ENABLE_STRUCT                  (STD_ON)
#define SOMEIPXF_ENABLE_UNION                   (STD_OFF)

/*==================================================================================================
*                                    E2E CONFIGURATION
==================================================================================================*/
#define SOMEIPXF_E2E_ENABLED                    (STD_OFF)
#define SOMEIPXF_E2E_PROFILE                    (0U)

/*==================================================================================================
*                                    TRANSFORMER IDs
==================================================================================================*/
#define SOMEIPXF_TRANSFORMER_ID_0               (0U)
#define SOMEIPXF_TRANSFORMER_ID_1               (1U)
#define SOMEIPXF_TRANSFORMER_ID_2               (2U)
#define SOMEIPXF_TRANSFORMER_ID_3               (3U)
#define SOMEIPXF_TRANSFORMER_ID_4               (4U)
#define SOMEIPXF_TRANSFORMER_ID_5               (5U)
#define SOMEIPXF_TRANSFORMER_ID_6               (6U)
#define SOMEIPXF_TRANSFORMER_ID_7               (7U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define SOMEIPXF_SERVICE_ID_ECU_MONITOR         (0x0001U)
#define SOMEIPXF_SERVICE_ID_DIAGNOSTICS         (0x0002U)
#define SOMEIPXF_SERVICE_ID_VEHICLE_DATA        (0x0003U)
#define SOMEIPXF_SERVICE_ID_ENGINE_CTRL         (0x0004U)

/*==================================================================================================
*                                    METHOD IDs
==================================================================================================*/
#define SOMEIPXF_METHOD_ID_GET_STATUS           (0x0001U)
#define SOMEIPXF_METHOD_ID_SET_MODE             (0x0002U)
#define SOMEIPXF_METHOD_ID_NOTIFY_EVENT         (0x8001U)

/*==================================================================================================
*                                    PROTOCOL VERSION
==================================================================================================*/
#define SOMEIPXF_PROTOCOL_VERSION               (0x01U)
#define SOMEIPXF_INTERFACE_VERSION              (0x01U)

/*==================================================================================================
*                                    ENDIANNESS
==================================================================================================*/
#define SOMEIPXF_BIG_ENDIAN                     (STD_ON)
#define SOMEIPXF_LITTLE_ENDIAN                  (STD_OFF)

#endif /* SOMEIPXF_CFG_H */

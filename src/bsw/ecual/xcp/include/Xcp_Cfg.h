/** @file Xcp_Cfg.h
 * @brief AUTOSAR XCP configuration header file
 * @details XCP module configuration parameters
 * @copyright YuleTech AutoSAR BSW Platform
 */

#ifndef XCP_CFG_H
#define XCP_CFG_H

#include "Std_Types.h"

/*============================================================================
 *                          PRE-COMPILE CONFIGURATION
 *===========================================================================*/

/* XCP protocol layer version */
#define XCP_PROTOCOL_LAYER_VERSION_MAJOR    (1u)
#define XCP_PROTOCOL_LAYER_VERSION_MINOR    (0u)

/* Transport layer selection */
#define XCP_ON_CAN_ENABLED      STD_ON
#define XCP_ON_ETH_ENABLED      STD_OFF
#define XCP_ON_USB_ENABLED      STD_OFF
#define XCP_ON_FLEXRAY_ENABLED  STD_OFF

/* Feature switches */
#define XCP_CALIBRATION_ENABLED     STD_ON
#define XCP_PAGE_SWITCHING_ENABLED  STD_ON
#define XCP_DAQ_ENABLED             STD_ON
#define XCP_STIM_ENABLED            STD_OFF
#define XCP_CHECKSUM_ENABLED        STD_ON
#define XCP_BLOCK_MODE_ENABLED      STD_ON
#define XCP_INTERLEAVED_MODE_ENABLED STD_OFF
#define XCP_MASTER_BLOCK_MODE_ENABLED STD_OFF

/* Resource protection */
#define XCP_RESOURCE_CAL_PAG    (0x01u) /* Calibration/Paging */
#define XCP_RESOURCE_DAQ        (0x04u) /* Data Acquisition */
#define XCP_RESOURCE_STIM       (0x08u) /* Stimulation */
#define XCP_RESOURCE_PGM        (0x10u) /* Programming */

/*============================================================================
 *                          MAXIMUM LIMITS
 *===========================================================================*/

/* CTO packet size (Command Transfer Object) */
#define XCP_CTO_SIZE            (8u)   /* Standard CAN frame size */
#define XCP_DTO_SIZE            (8u)   /* Standard CAN frame size */

/* DAQ configuration */
#define XCP_MAX_DAQ_LISTS       (16u)   /* Maximum DAQ lists */
#define XCP_MAX_DAQ             (16u)   /* Maximum DAQ lists (alias) */
#define XCP_MAX_CTO             (8u)    /* Maximum CTO length */
#define XCP_MAX_DTO             (8u)    /* Maximum DTO length */
#define XCP_MAX_SEED_SIZE       (8u)    /* Maximum seed & key size */
#define XCP_MAX_KEY_SIZE        (8u)
#define XCP_MAX_STIM            (4u)    /* Maximum STIM lists */
#define XCP_MAX_ODT             (8u)    /* Maximum ODTs per DAQ list */
#define XCP_MAX_ODT_ENTRIES     (7u)    /* Maximum entries per ODT */
#define XCP_MAX_EVENT_CHANNELS  (8u)    /* Maximum event channels */

/* Calibration configuration */
#define XCP_MAX_SEGMENTS        (4u)    /* Maximum calibration segments */
#define XCP_MAX_PAGES           (2u)    /* Maximum pages per segment */
#define XCP_PAGE_SIZE           (4096u) /* Page size in bytes */

/* Block transfer configuration */
#define XCP_MAX_BS              (255u)  /* Maximum block size */
#define XCP_MIN_ST              (0u)    /* Minimum separation time */

/* Upload/Download limits */
#define XCP_MAX_UPLOAD_SIZE     (0xFFu) /* Maximum upload size */
#define XCP_MAX_DOWNLOAD_SIZE   (0xFFu) /* Maximum download size */

/*============================================================================
 *                          TIMEOUT CONFIGURATION
 *===========================================================================*/

/* Session timeout in ms (0 = no timeout) */
#define XCP_SESSION_TIMEOUT     (30000u) /* 30 seconds */

/* Response timeout */
#define XCP_RESPONSE_TIMEOUT    (1000u)  /* 1 second */

/*============================================================================
 *                          ADDRESS MAPPING
 *===========================================================================*/

/* Address extension definitions */
#define XCP_ADDR_EXT_RAM        (0x00u) /* Internal RAM */
#define XCP_ADDR_EXT_FLASH      (0x01u) /* Flash memory */
#define XCP_ADDR_EXT_EEPROM     (0x02u) /* EEPROM */
#define XCP_ADDR_EXT_EXT_MEM    (0xFFu) /* External memory */

/*============================================================================
 *                          DAQ TIMING
 *===========================================================================*/

/* DAQ prescaler limits */
#define XCP_DAQ_MIN_PRESCALER   (1u)
#define XCP_DAQ_MAX_PRESCALER   (255u)

/* Timestamp configuration */
#define XCP_TIMESTAMP_ENABLED       STD_ON
#define XCP_TIMESTAMP_SIZE          (2u)  /* 2 bytes timestamp */
#define XCP_TIMESTAMP_UNIT          (0x01u) /* 1ms unit */
#define XCP_TIMESTAMP_TICKS_PER_UNIT (1u)

/*============================================================================
 *                          CHECKSUM CONFIGURATION
 *===========================================================================*/

/* Checksum types */
#define XCP_CHECKSUM_XOR8       (0x01u)
#define XCP_CHECKSUM_CRC16      (0x02u)
#define XCP_CHECKSUM_CRC32      (0x03u)
#define XCP_CHECKSUM_CRC16_CCITT (0x04u)

/* Default checksum type */
#define XCP_CHECKSUM_TYPE       XCP_CHECKSUM_CRC16

/*============================================================================
 *                          SLAVE DEVICE INFO
 *===========================================================================*/

/* XCP slave name */
#define XCP_SLAVE_NAME          "YuleTech_XCP"
#define XCP_SLAVE_NAME_LEN      (12u)

/* XCP slave version */
#define XCP_SLAVE_VERSION_MAJOR (1u)
#define XCP_SLAVE_VERSION_MINOR (0u)

/*============================================================================
 *                          RESOURCE PROTECTION
 *===========================================================================*/

/* Default resource locks (0 = unlocked) */
#define XCP_RESOURCE_DEFAULT_LOCK   (0x00u)

/* Seed/Key algorithm enable */
#define XCP_SEED_KEY_ENABLED        STD_OFF
#define XCP_SEED_LENGTH             (4u)
#define XCP_KEY_LENGTH              (4u)

/*============================================================================
 *                          CALLBACK CONFIGURATION
 *===========================================================================*/

/* User callbacks - enabled if functions provided */
#define XCP_USER_COPY_CAL_PAGE_CALLBACK     STD_OFF
#define XCP_USER_CHECKSUM_CALLBACK          STD_OFF
#define XCP_USER_TIMESTAMP_CALLBACK         STD_ON

/*============================================================================
 *                          ECU CONFIGURATION ID
 *===========================================================================*/
#define XCP_ECU_CONFIGURATION_ID    (0x0001u)

/*============================================================================
 *                          ERROR HANDLING
 *===========================================================================*/

/* Development error detection */
#define XCP_DEV_ERROR_DETECT        STD_ON

/* Error codes for DET */
#define XCP_E_PARAM_POINTER         (0x01u)
#define XCP_E_PARAM_LENGTH          (0x02u)
#define XCP_E_PARAM_ADDRESS         (0x03u)
#define XCP_E_UNINIT                (0x04u)
#define XCP_E_ALREADY_INITIALIZED   (0x05u)
#define XCP_E_INVALID_STATE         (0x06u)
#define XCP_E_INVALID_DAQ_LIST      (0x07u)
#define XCP_E_INVALID_ODT           (0x08u)
#define XCP_E_INVALID_ODT_ENTRY     (0x09u)

/*============================================================================
 *                          POST-BUILD CONFIGURATION
 *===========================================================================*/

/* Configuration variant */
typedef enum {
    XCP_CONFIG_VARIANT_PRE_COMPILE = 0,
    XCP_CONFIG_VARIANT_LINK_TIME,
    XCP_CONFIG_VARIANT_POST_BUILD
} Xcp_ConfigVariantType;

/*============================================================================
 *                          CONFIGURATION STRUCTURES
 *===========================================================================*/

/* Event channel configuration */
typedef struct {
    uint8  eventChannelNumber;
    uint8  eventChannelName[32];
    uint8  eventChannelCycleTime;  /* in ms */
    uint8  eventChannelPriority;
    uint16 eventChannelMaxDaqList;
} Xcp_EventChannelConfigType;

/* Segment configuration */
typedef struct {
    uint8  segmentNumber;
    uint32 segmentStart;
    uint32 segmentSize;
    uint8  numberOfPages;
    uint8  addressExtension;
    uint8  compressionMethod;
    uint8  encryptionMethod;
} Xcp_SegmentConfigType;

/* DAQ list configuration */
typedef struct {
    uint8  daqListNumber;
    uint16 maxOdt;
    uint16 maxOdtEntries;
    boolean fixedEventChannel;
    uint8  eventChannelNumber;
} Xcp_DaqListConfigType;

/* General configuration */
typedef struct {
    uint8  maxCto;
    uint8  maxDto;
    uint8  protocolLayerVersion;
    uint8  transportLayerVersion;
    uint8  byteOrder;           /* 0 = Intel, 1 = Motorola */
    uint8  addressGranularity;  /* 1, 2, or 4 bytes */
    uint8  slaveBlockMode;
} Xcp_GeneralConfigType;

/*============================================================================
 *                          CONFIGURATION EXTERNS
 *===========================================================================*/
extern const Xcp_GeneralConfigType Xcp_GeneralConfig;
extern const Xcp_SegmentConfigType Xcp_SegmentConfig[XCP_MAX_SEGMENTS];
extern const Xcp_EventChannelConfigType Xcp_EventChannelConfig[XCP_MAX_EVENT_CHANNELS];
extern const Xcp_DaqListConfigType Xcp_DaqListConfig[XCP_MAX_DAQ_LISTS];

#endif /* XCP_CFG_H */

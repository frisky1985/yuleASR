/**
 * @file Xcp_Cfg.h
 * @brief XCP Configuration File
 *        following ASAM XCP 1.1 standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef XCP_CFG_H
#define XCP_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define XCP_CFG_VENDOR_ID                   (0x01U)
#define XCP_CFG_MODULE_ID                   (0xD0U)
#define XCP_CFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define XCP_CFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define XCP_CFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define XCP_CFG_SW_MAJOR_VERSION            (0x01U)
#define XCP_CFG_SW_MINOR_VERSION            (0x00U)
#define XCP_CFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/**
 * @brief Development error detection enable
 */
#define XCP_DEV_ERROR_DETECT                (STD_ON)

/**
 * @brief Version info API enable
 */
#define XCP_VERSION_INFO_API                (STD_ON)

/**
 * @brief Number of XCP channels
 */
#define XCP_NUMBER_OF_CHANNELS              (1U)

/**
 * @brief Number of DAQ lists
 */
#define XCP_MAX_DAQ_LISTS                   (4U)

/**
 * @brief Number of ODTs per DAQ list
 */
#define XCP_MAX_ODTS_PER_DAQ                (8U)

/**
 * @brief Number of ODT entries per ODT
 */
#define XCP_MAX_ODT_ENTRIES_PER_ODT         (16U)

/**
 * @brief Maximum CTO (Command Transfer Object) size in bytes
 */
#define XCP_MAX_CTO_SIZE                    (8U)

/**
 * @brief Maximum DTO (Data Transfer Object) size in bytes
 */
#define XCP_MAX_DTO_SIZE                    (8U)

/**
 * @brief Maximum number of event channels
 */
#define XCP_MAX_EVENT_CHANNELS              (4U)

/**
 * @brief Block download support
 */
#define XCP_BLOCK_DOWNLOAD_SUPPORTED        (STD_ON)

/**
 * @brief Block upload support
 */
#define XCP_BLOCK_UPLOAD_SUPPORTED          (STD_ON)

/**
 * @brief Interleaved mode support
 */
#define XCP_INTERLEAVED_MODE_SUPPORTED      (STD_OFF)

/**
 * @brief Checksum calculation method
 * @details 0x01 = XCP_ADD_11, 0x02 = XCP_ADD_12, 0x03 = XCP_ADD_14
 *          0x04 = XCP_ADD_22, 0x05 = XCP_ADD_24, 0x06 = XCP_ADD_44
 *          0x07 = XCP_CRC_16, 0x08 = XCP_CRC_16_CITT, 0x09 = XCP_CRC_32
 */
#define XCP_CHECKSUM_TYPE                   (0x07U)  /* CRC-16 */

/**
 * @brief Time stamp support
 */
#define XCP_TIMESTAMP_SUPPORTED             (STD_ON)

/**
 * @brief Time stamp unit (0=1ns, 1=10ns, 2=100ns, 3=1us, 4=10us, 5=100us, 6=1ms, 7=10ms, 8=100ms, 9=1s)
 */
#define XCP_TIMESTAMP_UNIT                  (6U)  /* 1ms */

/**
 * @brief Time stamp size (0=1byte, 1=2bytes, 2=4bytes)
 */
#define XCP_TIMESTAMP_SIZE                  (2U)  /* 4 bytes */

/**
 * @brief DAQ header type (0=NO_HEADER, 1=ODT, 2=DAQ, 3=DAQ_AND_ODT)
 */
#define XCP_DAQ_HEADER_TYPE                 (2U)  /* DAQ */

/**
 * @brief DAQ identification field type (0=ABSOLUTE, 1=RELATIVE_WORD, 2=RELATIVE_BYTE, 3=RELATIVE_ODT_BYTE)
 */
#define XCP_IDENTIFICATION_FIELD_TYPE       (1U)  /* RELATIVE_WORD */

/**
 * @brief Address granularity (0=BYTE, 1=WORD, 2=DWORD, 3=RESERVED)
 */
#define XCP_ADDRESS_GRANULARITY             (0U)  /* BYTE */

/**
 * @brief Byte order (0=MSB_FIRST, 1=LSB_FIRST)
 */
#define XCP_BYTE_ORDER                      (1U)  /* LSB_FIRST */

/**
 * @brief Slave block mode support
 */
#define XCP_SLAVE_BLOCK_MODE                (STD_ON)

/**
 * @brief Master block mode support
 */
#define XCP_MASTER_BLOCK_MODE               (STD_ON)

/**
 * @brief Interleaved mode support for block transfer
 */
#define XCP_INTERLEAVED_BLOCK_MODE          (STD_OFF)

/**
 * @brief Maximum block size for upload
 */
#define XCP_MAX_BS_UPLOAD                   (255U)

/**
 * @brief Maximum block size for download
 */
#define XCP_MAX_BS_DOWNLOAD                 (255U)

/**
 * @brief Minimum separation time for block transfer (in microseconds)
 */
#define XCP_MIN_ST                          (0U)

/**
 * @brief Queue size for STIM (Stimulation)
 */
#define XCP_STIM_QUEUE_SIZE                 (8U)

/**
 * @brief Number of allowed connections
 */
#define XCP_MAX_CONNECTIONS                 (1U)

/**
 * @brief Maximum number of seeds for resource protection
 */
#define XCP_MAX_SEEDS                       (4U)

/**
 * @brief Seed/Key algorithm enable
 */
#define XCP_SEED_KEY_SUPPORTED              (STD_ON)

/**
 * @brief Calibration page switching support
 */
#define XCP_CAL_PAGE_SWITCHING              (STD_ON)

/**
 * @brief Number of calibration pages
 */
#define XCP_NUMBER_OF_CAL_PAGES             (2U)

/**
 * @brief Programming support
 */
#define XCP_PROGRAMMING_SUPPORTED           (STD_ON)

/**
 * @brief Maximum programming sector size
 */
#define XCP_MAX_PROGRAMMING_SECTOR_SIZE     (1024U)

/**
 * @brief Flash programming support
 */
#define XCP_FLASH_PROGRAMMING_SUPPORTED     (STD_ON)

/**
 * @brief Non-volatile memory programming support
 */
#define XCP_NVM_PROGRAMMING_SUPPORTED       (STD_ON)

/*==================================================================================================
*                                    RESOURCE PROTECTION CONFIGURATION
==================================================================================================*/

/**
 * @brief Calibration/Paging resource protection
 */
#define XCP_RESOURCE_CAL_PAG_PROTECTED      (STD_OFF)

/**
 * @brief DAQ resource protection
 */
#define XCP_RESOURCE_DAQ_PROTECTED          (STD_OFF)

/**
 * @brief STIM resource protection
 */
#define XCP_RESOURCE_STIM_PROTECTED         (STD_OFF)

/**
 * @brief Programming resource protection
 */
#define XCP_RESOURCE_PGM_PROTECTED          (STD_ON)

/*==================================================================================================
*                                    DAQ CONFIGURATION
==================================================================================================*/

/**
 * @brief Dynamic DAQ list allocation support
 */
#define XCP_DAQ_DYNAMIC_ALLOCATION          (STD_ON)

/**
 * @brief Prescaler support for DAQ lists
 */
#define XCP_DAQ_PRESCALER_SUPPORTED         (STD_ON)

/**
 * @brief DAQ timestamp support
 */
#define XCP_DAQ_TIMESTAMP_SUPPORTED         (STD_ON)

/**
 * @brief DAQ PID offset support
 */
#define XCP_DAQ_PID_OFF_SUPPORTED           (STD_ON)

/**
 * @brief DAQ overload indication
 * @details 0=NO_OVERLOAD_INDICATION, 1=OVERLOAD_INDICATION_PID, 2=OVERLOAD_INDICATION_EVENT
 */
#define XCP_DAQ_OVERLOAD_INDICATION         (1U)

/**
 * @brief Event channel timing type
 * @details 0=FIXED_EVENT_LIST, 1=CONFIGURABLE_EVENT_LIST
 */
#define XCP_EVENT_CHANNEL_TIMING_TYPE       (0U)

/**
 * @brief Event channel consistency
 * @details 0=EVENT_CHANNEL_CONSISTENCY, 1=ODT_CONSISTENCY, 2=Daq_LIST_CONSISTENCY
 */
#define XCP_EVENT_CHANNEL_CONSISTENCY       (1U)

/*==================================================================================================
*                                    TRANSPORT LAYER CONFIGURATION
==================================================================================================*/

/**
 * @brief CAN transport layer support
 */
#define XCP_ON_CAN_ENABLED                  (STD_ON)

/**
 * @brief Ethernet (UDP) transport layer support
 */
#define XCP_ON_ETH_UDP_ENABLED              (STD_ON)

/**
 * @brief Ethernet (TCP) transport layer support
 */
#define XCP_ON_ETH_TCP_ENABLED              (STD_OFF)

/**
 * @brief FlexRay transport layer support
 */
#define XCP_ON_FLEXRAY_ENABLED              (STD_ON)

/**
 * @brief USB transport layer support
 */
#define XCP_ON_USB_ENABLED                  (STD_OFF)

/**
 * @brief LIN transport layer support
 */
#define XCP_ON_LIN_ENABLED                  (STD_OFF)

/*==================================================================================================
*                                    DAQ ADDITIONAL CONFIGURATION
==================================================================================================*/
#define XCP_DAQ_SUPPORTED                   (STD_ON)

#define XCP_GRANULARITY_ODT_ENTRY_SIZE_DAQ  (1U)
#define XCP_MAX_ODT_ENTRY_SIZE_DAQ          (XCP_MAX_DTO_SIZE)
#define XCP_GRANULARITY_ODT_ENTRY_SIZE_STIM (1U)
#define XCP_MAX_ODT_ENTRY_SIZE_STIM         (XCP_MAX_DTO_SIZE)
#define XCP_DAQ_KEY_BYTE                    (0x01U)

/*==================================================================================================
*                                    PGM ADDITIONAL CONFIGURATION
==================================================================================================*/
#define XCP_MAX_CTO_PGM                     (XCP_MAX_CTO_SIZE)
#define XCP_MAX_BS_PGM                      (255U)
#define XCP_MIN_ST_PGM                      (0U)
#define XCP_QUEUE_SIZE_PGM                  (8U)

/*==================================================================================================
*                                    INSTANCE ID
==================================================================================================*/
#define XCP_INSTANCE_ID                     (0U)

/*==================================================================================================
*                                    CAN CONFIGURATION
==================================================================================================*/
#if (XCP_ON_CAN_ENABLED == STD_ON)

/**
 * @brief CAN Frame Type (0=STANDARD, 1=EXTENDED)
 */
#define XCP_CAN_FRAME_TYPE                  (0U)

/**
 * @brief CAN ID for CTO (Command Transfer Object)
 */
#define XCP_CAN_CTO_ID                      (0x123U)

/**
 * @brief CAN ID for DTO (Data Transfer Object)
 */
#define XCP_CAN_DTO_ID                      (0x124U)

/**
 * @brief CAN baudrate
 */
#define XCP_CAN_BAUDRATE                    (500U)  /* kbps */

#endif /* XCP_ON_CAN_ENABLED */

/*==================================================================================================
*                                    ETHERNET CONFIGURATION
==================================================================================================*/
#if (XCP_ON_ETH_UDP_ENABLED == STD_ON)

/**
 * @brief XCP on Ethernet port number
 */
#define XCP_ETH_PORT                        (5555U)

/**
 * @brief XCP on Ethernet buffer size
 */
#define XCP_ETH_BUFFER_SIZE                 (1500U)

#endif /* XCP_ON_ETH_UDP_ENABLED */

/*==================================================================================================
*                                    FLEXRAY CONFIGURATION
==================================================================================================*/
#if (XCP_ON_FLEXRAY_ENABLED == STD_ON)

/**
 * @brief XCP on FlexRay slot ID
 */
#define XCP_FLEXRAY_SLOT_ID                 (1U)

/**
 * @brief XCP on FlexRay cycle
 */
#define XCP_FLEXRAY_CYCLE                   (0U)

#endif /* XCP_ON_FLEXRAY_ENABLED */

/*==================================================================================================
*                                    MEMORY RANGES CONFIGURATION
==================================================================================================*/

/**
 * @brief Number of memory ranges
 */
#define XCP_NUMBER_OF_MEMORY_RANGES         (4U)

/**
 * @brief Memory access types
 */
#define XCP_MEMORY_ACCESS_READ              (0x01U)
#define XCP_MEMORY_ACCESS_WRITE             (0x02U)
#define XCP_MEMORY_ACCESS_EXECUTE           (0x04U)
#define XCP_MEMORY_ACCESS_ERASE             (0x08U)

/**
 * @brief Memory sector types
 */
#define XCP_MEMORY_SECTOR_TYPE_RAM          (0x00U)
#define XCP_MEMORY_SECTOR_TYPE_ROM          (0x01U)
#define XCP_MEMORY_SECTOR_TYPE_FLASH        (0x02U)
#define XCP_MEMORY_SECTOR_TYPE_EEPROM       (0x03U)

/*==================================================================================================
*                                    CALLBACK FUNCTIONS
==================================================================================================*/

/**
 * @brief Callback for flash erase
 */
#define XCP_FLASH_ERASE_CALLBACK            Xcp_FlashErase

/**
 * @brief Callback for flash program
 */
#define XCP_FLASH_PROGRAM_CALLBACK          Xcp_FlashProgram

/**
 * @brief Callback for flash read
 */
#define XCP_FLASH_READ_CALLBACK             Xcp_FlashRead

/**
 * @brief Callback for seed generation
 */
#define XCP_GET_SEED_CALLBACK               Xcp_GetSeed

/**
 * @brief Callback for key comparison
 */
#define XCP_COMPARE_KEY_CALLBACK            Xcp_CompareKey

/**
 * @brief Callback for timestamp
 */
#define XCP_GET_TIMESTAMP_CALLBACK          Xcp_GetTimestamp

/*==================================================================================================
*                                    FUNCTION-LIKE MACROS
==================================================================================================*/

/**
 * @brief Convert address granularity to bytes
 */
#define XCP_AG_TO_BYTES(x)                  ((x) << XCP_ADDRESS_GRANULARITY)

/**
 * @brief Convert bytes to address granularity
 */
#define XCP_BYTES_TO_AG(x)                  ((x) >> XCP_ADDRESS_GRANULARITY)

/**
 * @brief Check if resource is protected
 */
#define XCP_IS_RESOURCE_PROTECTED(res, prot)    (((prot) & (res)) != 0U)

/**
 * @brief Set resource protection
 */
#define XCP_SET_RESOURCE_PROTECTION(res, prot)  ((prot) |= (res))

/**
 * @brief Clear resource protection
 */
#define XCP_CLEAR_RESOURCE_PROTECTION(res, prot) ((prot) &= ~(res))

#endif /* XCP_CFG_H */

/**
 * @file Fls_Cfg.h
 * @brief Flash Driver configuration for S32K312 Seat Control Demo
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Flash sector configuration for seat memory storage.
 * 64KB reserved in flash_rsvd2 region for NVM / seat memory data.
 */

#ifndef FLS_CFG_H
#define FLS_CFG_H

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 * Pre-compile Configuration
 *==================================================================================================*/
#define FLS_DEV_ERROR_DETECT            (STD_ON)
#define FLS_VERSION_INFO_API            (STD_ON)

/*==================================================================================================
 * Flash Memory Layout
 *==================================================================================================*/
#define FLS_RESERVED_NVM_OFFSET         (0x001F8000UL)          /* 32KB offset from flash base */
#define FLS_NVM_SIZE                    (0x00010000UL)          /* 64KB for seat position memory */
#define FLS_SECTOR_SIZE                 (0x00000800UL)          /* 2KB per sector */
#define FLS_NUM_SECTORS                 (32U)                   /* 32 sectors x 2KB = 64KB */
#define FLS_PAGE_SIZE                   (0x00000008UL)          /* 8 bytes per page */
#define FLS_NUM_PAGES_PER_SECTOR        (256U)                  /* 256 pages per sector */

/*==================================================================================================
 * Flash Timing Configuration
 *==================================================================================================*/
#define FLS_PROGRAM_TIME_US             (20U)                   /* ~20us per page program */
#define FLS_ERASE_TIME_MS               (20U)                   /* ~20ms per sector erase */
#define FLS_ERASE_RETRIES               (3U)                    /* Max erase retries */

/*==================================================================================================
 * Memory Sector Type
 *==================================================================================================*/
typedef uint8 Fls_SectorType;

typedef enum {
    FLS_MODE_READ = 0,
    FLS_MODE_WRITE,
    FLS_MODE_ERASE
} Fls_OperationModeType;

/*==================================================================================================
 * Flash Sector Configuration
 *==================================================================================================*/
typedef struct {
    Fls_SectorType  sectorId;
    uint32          startAddress;       /* Absolute flash address */
    uint32          sectorSize;         /* Size in bytes */
    uint32          pageSize;           /* Minimum write unit */
} Fls_SectorConfigType;

typedef struct {
    Fls_SectorConfigType* sectors;
    uint8                  numSectors;
    uint32                 baseAddress;        /* NVM base address */
    uint32                 totalSize;          /* Total NVM size in bytes */
    uint8                  maxWriteRetries;
} Fls_ConfigType;

/*==================================================================================================
 * Seat Memory Storage Structure
 *==================================================================================================*/
typedef struct {
    uint16  magic;              /* Magic 0xA55A for valid data */
    uint16  version;            /* Format version */
    uint16  horizontalPos;      /* Horizontal position (mm) */
    uint16  reclinePos;         /* Recline angle (deg) */
    uint16  heightPos;          /* Height position (mm) */
    uint16  tiltPos;            /* Tilt angle (deg) */
    uint8   heaterPref;         /* Heater preference (0/1/2) */
    uint8   reserved[5];        /* Reserved for future use */
    uint16  checksum;           /* XOR-based checksum */
} Fls_SeatMemoryRecordType;

#define FLS_MEMORY_MAGIC                (0xA55AU)
#define FLS_MEMORY_VERSION              (0x0100U)

/*==================================================================================================
 * External Configuration Reference
 *==================================================================================================*/
extern const Fls_ConfigType Fls_Config;

#endif /* FLS_CFG_H */

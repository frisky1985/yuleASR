/**
 * @file MemIf_Types.h
 * @brief Memory Interface Types - stub for compilation
 */
#ifndef MEMIF_TYPES_H
#define MEMIF_TYPES_H

#include "Std_Types.h"

/* Memory device types */
typedef uint8 MemIf_DeviceType;
typedef uint8 MemIf_StatusType;
typedef uint8 MemIf_JobResultType;
typedef uint8 MemIf_ModeType;

/* Device types */
#define MEMIF_FLASH_DEVICE  0x00u
#define MEMIF_EEP_DEVICE    0x01u
#define MEMIF_FEE_DEVICE    0x02u
#define MEMIF_EA_DEVICE     0x03u

/* Status */
#define MEMIF_UNINIT        0x00u
#define MEMIF_IDLE          0x01u
#define MEMIF_BUSY          0x02u
#define MEMIF_BUSY_INTERNAL 0x03u

/* Job results */
#define MEMIF_JOB_OK        0x00u
#define MEMIF_JOB_FAILED    0x01u
#define MEMIF_JOB_PENDING   0x02u
#define MEMIF_JOB_CANCELLED 0x03u

/* Block types */
typedef uint16 MemIf_BlockNumber;
typedef uint16 MemIf_BlockLength;

#endif /* MEMIF_TYPES_H */

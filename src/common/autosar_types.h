/******************************************************************************
 * @file    autosar_types.h
 * @brief   AUTOSAR Standard Types Definition
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef AUTOSAR_TYPES_H
#define AUTOSAR_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Standard AUTOSAR Types
 ******************************************************************************/

/* Boolean types */
#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

/* Standard return type */
typedef uint8_t Std_ReturnType;

/* Standard AUTOSAR types - use stdint.h for portability */
typedef uint8_t   boolean;
typedef int8_t    sint8;
typedef uint8_t   uint8;
typedef int16_t   sint16;
typedef uint16_t  uint16;
typedef int32_t   sint32;
typedef uint32_t  uint32;
typedef int64_t   sint64;
typedef uint64_t  uint64;
typedef float     float32;
typedef double    float64;

/* Physical types */
typedef uint32_t PhysicalTimeType;
typedef uint32_t TickType;
typedef uint32_t TimeInMicrosecondsType;
typedef uint32_t TimeInMillisecondsType;

/* Memory address types */
typedef void* MemoryAddressType;
typedef uint32_t ApplicationDataRefType;

/* AUTOSAR version info */
typedef struct {
    uint16_t vendorID;
    uint16_t moduleID;
    uint8_t  sw_major_version;
    uint8_t  sw_minor_version;
    uint8_t  sw_patch_version;
} Std_VersionInfoType;

/******************************************************************************
 * Safety Related Types
 ******************************************************************************/

/* ASIL levels */
typedef enum {
    ASIL_QM = 0,
    ASIL_A,
    ASIL_B,
    ASIL_C,
    ASIL_D
} ASIL_LevelType;

/* Safety monitoring */
typedef enum {
    SAFETY_STATUS_OK = 0,
    SAFETY_STATUS_ERROR,
    SAFETY_STATUS_WARNING,
    SAFETY_STATUS_FATAL
} SafetyStatusType;

/******************************************************************************
 * Network Types
 ******************************************************************************/

/* Network handle types */
typedef uint8_t NetworkHandleType;
typedef uint16_t PNCHandleType;

/* Communication modes */
typedef enum {
    COMM_NO_COMMUNICATION = 0,
    COMM_SILENT_COMMUNICATION,
    COMM_FULL_COMMUNICATION
} ComM_ModeType;

/******************************************************************************
 * Error Types
 ******************************************************************************/

typedef enum {
    E_OK = 0,
    E_NOT_OK = 1,
    E_UNKNOWN_SERVICE = 2,
    E_SERVICE_ALREADY_ACTIVE = 3,
    E_MUTE_CHANNEL = 4,
    E_PARAMETER_NOT_AVAILABLE = 5,
    E_PARAMETER_OUT_OF_RANGE = 6,
    E_QUEUE_FULL = 7,
    E_WRONG_CONTEXT = 8,
    E_COMMS_ERROR = 9,
    E_TIMEOUT = 10,
    E2E_E_OK = 11,
    E2E_E_WRONG_CRC = 12,
    E2E_E_WRONG_SEQUENCE = 13,
    E2E_E_LOSS = 14,
    E2E_E_DELAY = 15,
    E2E_E_DUPLICATE = 16
} Std_ErrorType;

/******************************************************************************
 * Protection Types
 ******************************************************************************/

typedef enum {
    PROTECTION_NONE = 0,
    PROTECTION_CRC8,
    PROTECTION_CRC16,
    PROTECTION_CRC32,
    PROTECTION_ECC,
    PROTECTION_E2E_P01,
    PROTECTION_E2E_P02,
    PROTECTION_E2E_P04,
    PROTECTION_E2E_P05,
    PROTECTION_E2E_P06,
    PROTECTION_E2E_P07,
    PROTECTION_E2E_P11
} ProtectionType;

/******************************************************************************
 * Hardware Types
 ******************************************************************************/

typedef enum {
    HW_TYPE_UNKNOWN = 0,
    HW_TYPE_MCU,
    HW_TYPE_SOC,
    HW_TYPE_FPGA,
    HW_TYPE_ASIC
} HardwareType;

typedef struct {
    uint32_t cpuFrequencyHz;
    uint32_t ramSizeBytes;
    uint32_t flashSizeBytes;
    uint8_t numCores;
    HardwareType hwType;
} HardwareInfoType;

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAR_TYPES_H */

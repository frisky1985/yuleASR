#ifndef CRC_H
#define CRC_H
#include "Std_Types.h"

/* ── Configuration switches ── */
#ifndef CRC_VERSION_INFO_API
#define CRC_VERSION_INFO_API    STD_ON
#endif
#ifndef CRC_DEV_ERROR_DETECT
#define CRC_DEV_ERROR_DETECT    STD_ON
#endif

/* ── Module constants ── */
#define CRC_VENDOR_ID           0U
#define CRC_MODULE_ID           0x7DU
#define CRC_INSTANCE_ID         0U
#define CRC_SW_MAJOR_VERSION    1U
#define CRC_SW_MINOR_VERSION    0U
#define CRC_SW_PATCH_VERSION    0U

/* ── Error codes ── */
#define CRC_E_PARAM_POINTER     0x10U
#define CRC_E_PARAM_CONFIG      0x11U
#define CRC_E_PARAM_DATA        0x12U
#define CRC_E_UNINIT            0x13U

/* ── Service IDs ── */
#define CRC_SID_INIT            0x00U
#define CRC_SID_CALC_CRC8       0x01U
#define CRC_SID_CALC_CRC16      0x02U
#define CRC_SID_CALC_CRC32      0x03U
#define CRC_SID_CALC_CRC64      0x04U
#define CRC_SID_GET_VERSION     0x05U

/* ── CRC types (only if not pre-defined by Crc_Cfg.h) ── */
#ifndef CRC_8
#define CRC_8   ((uint8)8U)
#endif
#ifndef CRC_16
#define CRC_16  ((uint8)16U)
#endif
#ifndef CRC_32
#define CRC_32  ((uint8)32U)
#endif
#ifndef CRC_64
#define CRC_64  ((uint8)64U)
#endif

/* ── CRC constants for E2E test compatibility ── */
#ifndef CRC8_INITIAL_VALUE
#define CRC8_INITIAL_VALUE              (0xFFU)
#endif
#ifndef CRC8_XOR_VALUE
#define CRC8_XOR_VALUE                  (0xFFU)
#endif
#ifndef CRC16_INITIAL_VALUE
#define CRC16_INITIAL_VALUE             (0xFFFFU)
#endif
#ifndef CRC16_XOR_VALUE
#define CRC16_XOR_VALUE                 (0x0000U)
#endif
#ifndef CRC32_INITIAL_VALUE
#define CRC32_INITIAL_VALUE             (0xFFFFFFFFU)
#endif
#ifndef CRC32_XOR_VALUE
#define CRC32_XOR_VALUE                 (0xFFFFFFFFU)
#endif

/* ── API declarations ── */
uint8  Crc_CalculateCRC8(const uint8* data, uint32 len, uint8 crc, boolean first);
uint16 Crc_CalculateCRC16(const uint8* data, uint32 len, uint16 crc, boolean first);
uint32 Crc_CalculateCRC32(const uint8* data, uint32 len, uint32 crc, boolean first);
uint64 Crc_CalculateCRC64(const uint8* data, uint32 len, uint64 crc, boolean first);
void   Crc_GetVersionInfo(Std_VersionInfoType* ver);
#endif

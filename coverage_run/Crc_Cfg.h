/* Crc_Cfg.h - CRC configuration for coverage testing */
#ifndef CRC_CFG_H
#define CRC_CFG_H

#include "Std_Types.h"

#define CRC_DEV_ERROR_DETECT            STD_OFF
#define CRC_VERSION_INFO_API            STD_ON

/* CRC8 config */
#define CRC_8_MODE                      STD_ON
#define CRC_8_TABLE_MODE                STD_ON
#define CRC_8_POLYNOMIAL                (0x1DU)
#define CRC_8_INITIAL_VALUE             (0xFFU)
#define CRC_8_XOR_OUT                   (0xFFU)
#define CRC_8_TABLE_SIZE                (256U)

/* CRC16 config */
#define CRC_16_MODE                     STD_ON
#define CRC_16_TABLE_MODE               STD_ON
#define CRC_16_POLYNOMIAL               (0x1021U)
#define CRC_16_INITIAL_VALUE            (0xFFFFU)
#define CRC_16_XOR_OUT                  (0x0000U)
#define CRC_16_TABLE_SIZE               (256U)

/* CRC32 config */
#define CRC_32_MODE                     STD_ON
#define CRC_32_TABLE_MODE               STD_ON
#define CRC_32_POLYNOMIAL               (0x04C11DB7U)
#define CRC_32_INITIAL_VALUE            (0xFFFFFFFFU)
#define CRC_32_XOR_OUT                  (0xFFFFFFFFU)
#define CRC_32_TABLE_SIZE               (256U)

/* Crc_ConfigType */
typedef struct {
    boolean Crc8_Enabled;
    boolean Crc16_Enabled;
    boolean Crc32_Enabled;
    uint8 Crc8_Polynomial;
    uint16 Crc16_Polynomial;
    uint32 Crc32_Polynomial;
    uint8 Crc8_InitValue;
    uint16 Crc16_InitValue;
    uint32 Crc32_InitValue;
} Crc_ConfigType;

extern const Crc_ConfigType Crc_Config;

#endif

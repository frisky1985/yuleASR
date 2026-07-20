/*==================================================================================================
 * FILE: Crc_Cfg.h
 * DESCRIPTION: AUTOSAR CRC Services - Configuration Header
 * PROVIDER: 上海予乐电子科技有限公司 (YuleTech)
 * VERSION: 1.0.0
 ==================================================================================================*/

#ifndef CRC_CFG_H
#define CRC_CFG_H

#include "Std_Types.h"

/*==================================================================================================
 * PRECOMPILE-TIME CONFIGURATION
 ==================================================================================================*/

/**
 * @brief Development error detection switch
 * STD_ON: Development error detection enabled
 * STD_OFF: Development error detection disabled
 */
#define CRC_DEV_ERROR_DETECT            STD_OFF

/**
 * @brief Version info API switch
 * STD_ON: Version info API enabled
 * STD_OFF: Version info API disabled
 */
#define CRC_VERSION_INFO_API            STD_ON

/*==================================================================================================
 * CRC8 CONFIGURATION (SAE J1850)
 ==================================================================================================*/

/**
 * @brief Enable CRC8 calculation
 * STD_ON: CRC8 enabled
 * STD_OFF: CRC8 disabled
 */
#define CRC_8_MODE                      STD_ON

/**
 * @brief CRC8 polynomial (SAE J1850)
 * x^8 + x^4 + x^3 + x^2 + 1
 */
#define CRC_8_POLYNOMIAL                (0x1DU)

/**
 * @brief CRC8 initial value
 */
#define CRC_8_INITIAL_VALUE             (0xFFU)

/**
 * @brief CRC8 final XOR value
 */
#define CRC_8_XOR_OUT                   (0xFFU)

/**
 * @brief Enable CRC8 lookup table for fast calculation
 * STD_ON: Use lookup table (faster, uses more ROM)
 * STD_OFF: Use runtime calculation (slower, less ROM)
 */
#define CRC_8_TABLE_MODE                STD_ON

/**
 * @brief CRC8 table size (always 256 for byte-based lookup)
 */
#define CRC_8_TABLE_SIZE                (256U)

/*==================================================================================================
 * CRC16 CONFIGURATION (CCITT-FALSE)
 ==================================================================================================*/

/**
 * @brief Enable CRC16 calculation
 * STD_ON: CRC16 enabled
 * STD_OFF: CRC16 disabled
 */
#define CRC_16_MODE                     STD_ON

/**
 * @brief CRC16 polynomial (CCITT-FALSE)
 * x^16 + x^12 + x^5 + 1
 */
#define CRC_16_POLYNOMIAL               (0x1021U)

/**
 * @brief CRC16 initial value
 */
#define CRC_16_INITIAL_VALUE            (0xFFFFU)

/**
 * @brief CRC16 final XOR value
 */
#define CRC_16_XOR_OUT                  (0x0000U)

/**
 * @brief Enable CRC16 lookup table for fast calculation
 * STD_ON: Use lookup table (faster, uses more ROM)
 * STD_OFF: Use runtime calculation (slower, less ROM)
 */
#define CRC_16_TABLE_MODE               STD_ON

/**
 * @brief CRC16 table size (always 256 for byte-based lookup)
 */
#define CRC_16_TABLE_SIZE               (256U)

/*==================================================================================================
 * CRC32 CONFIGURATION (IEEE 802.3)
 ==================================================================================================*/

/**
 * @brief Enable CRC32 calculation
 * STD_ON: CRC32 enabled
 * STD_OFF: CRC32 disabled
 */
#define CRC_32_MODE                     STD_ON

/**
 * @brief CRC32 polynomial (IEEE 802.3)
 * x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + 
 * x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
 */
#define CRC_32_POLYNOMIAL               (0x04C11DB7U)

/**
 * @brief CRC32 initial value
 */
#define CRC_32_INITIAL_VALUE            (0xFFFFFFFFU)

/**
 * @brief CRC32 final XOR value
 */
#define CRC_32_XOR_OUT                  (0xFFFFFFFFU)

/**
 * @brief Enable CRC32 lookup table for fast calculation
 * STD_ON: Use lookup table (faster, uses more ROM)
 * STD_OFF: Use runtime calculation (slower, less ROM)
 */
#define CRC_32_TABLE_MODE               STD_ON

/**
 * @brief CRC32 table size (always 256 for byte-based lookup)
 */
#define CRC_32_TABLE_SIZE               (256U)

/*==================================================================================================
 * POST-BUILD CONFIGURATION (EXAMPLE - Can be extended)
 ==================================================================================================*/
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

/*==================================================================================================
 * EXTERN CONFIGURATION
 ==================================================================================================*/
extern const Crc_ConfigType Crc_Config;

#endif /* CRC_CFG_H */
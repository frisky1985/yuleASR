/**
 * @file Crc.h
 * @brief AUTOSAR CRC Library Header
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * This header defines the interface for all CRC algorithms
 * as specified in the AUTOSAR CRC Library specification.
 * 
 * @see AUTOSAR_SWS_CRCLibrary
 */

#ifndef CRC_H
#define CRC_H

#include "Std_Types.h"

/*=============================================================================*
 * Function Prototypes
 *=============================================================================*/

/**
 * @brief Calculates 8-bit CRC (SAE J1850 polynomial 0x1D)
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue8 Initial CRC value (ignored if Crc_IsFirstCall is TRUE)
 * @param Crc_IsFirstCall TRUE for first call, FALSE for subsequent calls
 * @return Calculated 8-bit CRC
 */
uint8 Crc_CalculateCRC8(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint8 Crc_StartValue8,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculates CRC8H2F (polynomial 0x2F) for CAN FD
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue8H2F Initial CRC value
 * @param Crc_IsFirstCall TRUE for first call
 * @return Calculated 8-bit CRC
 */
uint8 Crc_CalculateCRC8H2F(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint8 Crc_StartValue8H2F,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculates 16-bit CRC (CCITT-FALSE polynomial 0x1021)
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue16 Initial CRC value
 * @param Crc_IsFirstCall TRUE for first call
 * @return Calculated 16-bit CRC
 */
uint16 Crc_CalculateCRC16(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint16 Crc_StartValue16,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculates 16-bit ARC CRC
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue16 Initial CRC value
 * @param Crc_IsFirstCall TRUE for first call
 * @return Calculated 16-bit CRC
 */
uint16 Crc_CalculateCRC16ARC(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint16 Crc_StartValue16,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculates 32-bit CRC (IEEE 802.3 polynomial 0x04C11DB7)
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue32 Initial CRC value
 * @param Crc_IsFirstCall TRUE for first call
 * @return Calculated 32-bit CRC
 */
uint32 Crc_CalculateCRC32(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint32 Crc_StartValue32,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculates CRC32P4 (polynomial 0xF4ACFB13) for FlexRay
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue32 Initial CRC value
 * @param Crc_IsFirstCall TRUE for first call
 * @return Calculated 32-bit CRC
 */
uint32 Crc_CalculateCRC32P4(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint32 Crc_StartValue32,
    boolean Crc_IsFirstCall
);

/**
 * @brief Calculates 64-bit CRC (polynomial 0x42F0E1EBA9EA3693)
 * @param Crc_DataPtr Pointer to data buffer
 * @param Crc_Length Length of data in bytes
 * @param Crc_StartValue64 Initial CRC value
 * @param Crc_IsFirstCall TRUE for first call
 * @return Calculated 64-bit CRC
 */
uint64 Crc_CalculateCRC64(
    const uint8* Crc_DataPtr,
    uint32 Crc_Length,
    uint64 Crc_StartValue64,
    boolean Crc_IsFirstCall
);

#endif /* CRC_H */

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : portable (any C99)
* Dependencies         : none (stdint.h only)
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Lib_Crc.c
 * @brief   Independent CRC algorithm library — implementation
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Bitwise (runtime) implementations, portable and dependency-free.
 *   Check values (data = "123456789"):
 *     CRC-8/SAE-J1850       0x4B
 *     CRC-8/AUTOSAR         0xDF
 *     CRC-16/CCITT-FALSE    0x29B1
 *     CRC-16/XMODEM         0x31C3
 *     CRC-32/ISO-HDLC       0xCBF43926
 *   Streaming contract: Update() takes the raw register state; the final
 *   XOR equals the INIT value for all variants, so a stream is finished
 *   with  result = state ^ INIT.
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Lib_Crc.h"

/*==================================================================================================
 *                                         LOCAL HELPERS
 *==================================================================================================*/

/**
 * @brief Compute the MSB-first CRC of a buffer
 * @param crc       [in] Running register state
 * @param data      [in] Data buffer
 * @param len       [in] Data length
 * @param poly      [in] Polynomial (e.g. 0x1D / 0x2F / 0x1021)
 * @param topBit    [in] Top bit mask of the effective register (0x80 / 0x8000)
 * @param byteShift [in] Byte feed shift (0 for 8-bit, 8 for 16-bit registers)
 */
static uint32_t CrcUpdateMsbFirst(uint32_t crc, const uint8_t* data, size_t len,
                                  uint32_t poly, uint32_t topBit,
                                  uint32_t byteShift)
{
    size_t i;

    for (i = 0u; i < len; i++)
    {
        uint32_t bit;

        crc ^= ((uint32_t)data[i]) << byteShift;
        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & topBit) != 0u)
            {
                crc = (crc << 1u) ^ poly;
            }
            else
            {
                crc <<= 1u;
            }
        }
    }
    return crc;
}

/**
 * @brief Compute the LSB-first (reflected) CRC of a buffer
 * @param crc    [in] Running register state
 * @param data   [in] Data buffer
 * @param len    [in] Data length
 * @param poly   [in] Reflected polynomial (e.g. 0xEDB88320)
 */
static uint32_t CrcUpdateLsbFirst(uint32_t crc, const uint8_t* data, size_t len,
                                  uint32_t poly)
{
    size_t i;

    for (i = 0u; i < len; i++)
    {
        uint32_t bit;

        crc ^= (uint32_t)data[i];
        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x00000001u) != 0u)
            {
                crc = (crc >> 1u) ^ poly;
            }
            else
            {
                crc >>= 1u;
            }
        }
    }
    return crc;
}

/*==================================================================================================
 *                                         CRC-8 / SAE-J1850
 *==================================================================================================*/
uint8_t Lib_Crc8SaeJ1850(const uint8_t* data, size_t len)
{
    uint32_t crc;

    crc = CrcUpdateMsbFirst((uint32_t)LIB_CRC8_SAE_J1850_INIT, data, len,
                            0x1Du, 0x80u, 0u);
    return (uint8_t)(crc ^ (uint32_t)LIB_CRC8_SAE_J1850_INIT);
}

uint8_t Lib_Crc8SaeJ1850Update(uint8_t crc, const uint8_t* data, size_t len)
{
    return (uint8_t)CrcUpdateMsbFirst((uint32_t)crc, data, len, 0x1Du, 0x80u, 0u);
}

/*==================================================================================================
 *                                         CRC-8 / AUTOSAR (H2F)
 *==================================================================================================*/
uint8_t Lib_Crc8Autosar(const uint8_t* data, size_t len)
{
    uint32_t crc;

    crc = CrcUpdateMsbFirst((uint32_t)LIB_CRC8_AUTOSAR_INIT, data, len,
                            0x2Fu, 0x80u, 0u);
    return (uint8_t)(crc ^ (uint32_t)LIB_CRC8_AUTOSAR_INIT);
}

uint8_t Lib_Crc8AutosarUpdate(uint8_t crc, const uint8_t* data, size_t len)
{
    return (uint8_t)CrcUpdateMsbFirst((uint32_t)crc, data, len, 0x2Fu, 0x80u, 0u);
}

/*==================================================================================================
 *                                         CRC-16 / CCITT-FALSE
 *==================================================================================================*/
uint16_t Lib_Crc16CcittFalse(const uint8_t* data, size_t len)
{
    uint32_t crc;

    crc = CrcUpdateMsbFirst((uint32_t)LIB_CRC16_CCITT_FALSE_INIT, data, len,
                            0x1021u, 0x8000u, 8u);
    /* final XOR = 0x0000 */
    return (uint16_t)crc;
}

uint16_t Lib_Crc16CcittFalseUpdate(uint16_t crc, const uint8_t* data, size_t len)
{
    return (uint16_t)CrcUpdateMsbFirst((uint32_t)crc, data, len, 0x1021u, 0x8000u, 8u);
}

/*==================================================================================================
 *                                         CRC-16 / XMODEM
 *==================================================================================================*/
uint16_t Lib_Crc16Xmodem(const uint8_t* data, size_t len)
{
    uint32_t crc;

    crc = CrcUpdateMsbFirst((uint32_t)LIB_CRC16_XMODEM_INIT, data, len,
                            0x1021u, 0x8000u, 8u);
    /* final XOR = 0x0000 */
    return (uint16_t)crc;
}

uint16_t Lib_Crc16XmodemUpdate(uint16_t crc, const uint8_t* data, size_t len)
{
    return (uint16_t)CrcUpdateMsbFirst((uint32_t)crc, data, len, 0x1021u, 0x8000u, 8u);
}

/*==================================================================================================
 *                                         CRC-32 / ISO-HDLC
 *==================================================================================================*/
uint32_t Lib_Crc32IsoHdlc(const uint8_t* data, size_t len)
{
    uint32_t crc;

    crc = CrcUpdateLsbFirst(LIB_CRC32_ISO_HDLC_INIT, data, len, 0xEDB88320u);
    return (crc ^ LIB_CRC32_ISO_HDLC_INIT);
}

uint32_t Lib_Crc32IsoHdlcUpdate(uint32_t crc, const uint8_t* data, size_t len)
{
    return CrcUpdateLsbFirst(crc, data, len, 0xEDB88320u);
}

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
 * @file    Lib_Crc.h
 * @brief   Independent CRC algorithm library (CRC-8/16/32)
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Decoupled, reusable CRC algorithms (XMEN Libraries/ alignment).
 *   Pure C99 + stdint.h — no AUTOSAR dependencies.
 *
 *   Algorithms:
 *     - CRC-8/SAE-J1850      poly 0x1D, init/xorout 0xFF (CAN 经典校验)
 *     - CRC-8/AUTOSAR (H2F)  poly 0x2F, init/xorout 0xFF (E2E/SecOC 常用)
 *     - CRC-16/CCITT-FALSE   poly 0x1021, init 0xFFFF, xorout 0x0000
 *     - CRC-16/XMODEM        poly 0x1021, init/xorout 0x0000
 *     - CRC-32/ISO-HDLC      poly 0x04C11DB7 (reflected 0xEDB88320),
 *                            init/xorout 0xFFFFFFFF (IEEE 802.3)
 *
 *   Streaming contract (all variants share the same shape):
 *     state = LIB_CRCxx_INIT;
 *     state = Lib_XxxUpdate(state, chunk1, len1);
 *     state = Lib_XxxUpdate(state, chunk2, len2);
 *     result = state ^ LIB_CRCxx_INIT;   // final XOR (== init value)
 *   The one-shot Lib_Xxx(data, len) is equivalent to the streamed form and
 *   matches the standard catalogue check values.
 */

#ifndef LIB_CRC_H
#define LIB_CRC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INIT VALUES
 *==================================================================================================*/

/** @brief CRC-8/SAE-J1850 initial register value (= final XOR) */
#define LIB_CRC8_SAE_J1850_INIT          (0xFFu)

/** @brief CRC-8/AUTOSAR initial register value (= final XOR) */
#define LIB_CRC8_AUTOSAR_INIT            (0xFFu)

/** @brief CRC-16/CCITT-FALSE initial register value */
#define LIB_CRC16_CCITT_FALSE_INIT       (0xFFFFu)

/** @brief CRC-16/XMODEM initial register value */
#define LIB_CRC16_XMODEM_INIT            (0x0000u)

/** @brief CRC-32/ISO-HDLC initial register value (= final XOR) */
#define LIB_CRC32_ISO_HDLC_INIT          (0xFFFFFFFFu)

/*==================================================================================================
 *                                         CRC-8 / SAE-J1850
 *==================================================================================================*/

/**
 * @brief   One-shot CRC-8/SAE-J1850 ("123456789" -> 0x4B)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  CRC-8 value
 */
uint8_t Lib_Crc8SaeJ1850(const uint8_t* data, size_t len);

/**
 * @brief   Streaming update of the CRC-8/SAE-J1850 register
 * @param   crc  [in] Current register state (start with LIB_CRC8_SAE_J1850_INIT)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  Updated register state (apply final XOR ^ LIB_CRC8_SAE_J1850_INIT)
 */
uint8_t Lib_Crc8SaeJ1850Update(uint8_t crc, const uint8_t* data, size_t len);

/*==================================================================================================
 *                                         CRC-8 / AUTOSAR (H2F)
 *==================================================================================================*/

/**
 * @brief   One-shot CRC-8/AUTOSAR ("123456789" -> 0xDF)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  CRC-8 value
 */
uint8_t Lib_Crc8Autosar(const uint8_t* data, size_t len);

/**
 * @brief   Streaming update of the CRC-8/AUTOSAR register
 * @param   crc  [in] Current register state (start with LIB_CRC8_AUTOSAR_INIT)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  Updated register state (apply final XOR ^ LIB_CRC8_AUTOSAR_INIT)
 */
uint8_t Lib_Crc8AutosarUpdate(uint8_t crc, const uint8_t* data, size_t len);

/*==================================================================================================
 *                                         CRC-16 / CCITT-FALSE
 *==================================================================================================*/

/**
 * @brief   One-shot CRC-16/CCITT-FALSE ("123456789" -> 0x29B1)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  CRC-16 value
 */
uint16_t Lib_Crc16CcittFalse(const uint8_t* data, size_t len);

/**
 * @brief   Streaming update of the CRC-16/CCITT-FALSE register
 * @param   crc  [in] Current register state (start with LIB_CRC16_CCITT_FALSE_INIT)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  Updated register state (final XOR is 0x0000)
 */
uint16_t Lib_Crc16CcittFalseUpdate(uint16_t crc, const uint8_t* data, size_t len);

/*==================================================================================================
 *                                         CRC-16 / XMODEM
 *==================================================================================================*/

/**
 * @brief   One-shot CRC-16/XMODEM ("123456789" -> 0x31C3)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  CRC-16 value
 */
uint16_t Lib_Crc16Xmodem(const uint8_t* data, size_t len);

/**
 * @brief   Streaming update of the CRC-16/XMODEM register
 * @param   crc  [in] Current register state (start with LIB_CRC16_XMODEM_INIT)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  Updated register state (final XOR is 0x0000)
 */
uint16_t Lib_Crc16XmodemUpdate(uint16_t crc, const uint8_t* data, size_t len);

/*==================================================================================================
 *                                         CRC-32 / ISO-HDLC
 *==================================================================================================*/

/**
 * @brief   One-shot CRC-32/ISO-HDLC, IEEE 802.3 ("123456789" -> 0xCBF43926)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  CRC-32 value
 */
uint32_t Lib_Crc32IsoHdlc(const uint8_t* data, size_t len);

/**
 * @brief   Streaming update of the CRC-32/ISO-HDLC register
 * @param   crc  [in] Current register state (start with LIB_CRC32_ISO_HDLC_INIT)
 * @param   data [in] Data buffer (may be NULL only when len == 0)
 * @param   len  [in] Data length in bytes
 * @return  Updated register state (apply final XOR ^ LIB_CRC32_ISO_HDLC_INIT)
 */
uint32_t Lib_Crc32IsoHdlcUpdate(uint32_t crc, const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CRC_H */

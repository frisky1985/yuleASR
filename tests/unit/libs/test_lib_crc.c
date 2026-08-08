/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Lib_Crc (independent CRC library) Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* B3-2 (2026-08-09): catalogue check values (data = "123456789") +
* streaming equivalence (chunked == one-shot) for all five algorithms.
*
* Use: cmake -DBUILD_TESTING=ON .. && make LibCrc_UnitTest && ctest -R LibCrc
==================================================================================================*/

#include "../test_framework.h"
#include "Lib_Crc.h"
#include <string.h>

/*==================================================================================================
*                                      FIXTURE
*==================================================================================================*/
static const uint8_t g_checkData[] = "123456789";   /* 9 bytes, no NUL */

/* Two chunks used for the streaming equivalence checks */
static const uint8_t g_chunkA[] = { 0x01u, 0x02u, 0x03u, 0x04u, 0x05u };
static const uint8_t g_chunkB[] = { 0x10u, 0x20u, 0x30u, 0x40u, 0x50u, 0x60u };

void setUp(void)
{
    /* nothing to set up */
}

void tearDown(void)
{
    /* nothing to tear down */
}

/*==================================================================================================
*                                      CRC-8 / SAE-J1850
*==================================================================================================*/
void test_crc8_sae_j1850_check_value(void)
{
    /* Catalogue check value: CRC-8/SAE-J1850("123456789") = 0x4B */
    TEST_ASSERT_EQUAL_HEX8(0x4Bu, Lib_Crc8SaeJ1850(g_checkData, 9u));
}

void test_crc8_sae_j1850_empty(void)
{
    /* Empty input: init ^ final xor = 0xFF ^ 0xFF = 0x00 */
    TEST_ASSERT_EQUAL_HEX8(0x00u, Lib_Crc8SaeJ1850(NULL, 0u));
}

void test_crc8_sae_j1850_streaming_two_parts(void)
{
    uint8_t combined[sizeof(g_chunkA) + sizeof(g_chunkB)];
    uint8_t state;

    (void)memcpy(combined, g_chunkA, sizeof(g_chunkA));
    (void)memcpy(combined + sizeof(g_chunkA), g_chunkB, sizeof(g_chunkB));

    state = LIB_CRC8_SAE_J1850_INIT;
    state = Lib_Crc8SaeJ1850Update(state, g_chunkA, sizeof(g_chunkA));
    state = Lib_Crc8SaeJ1850Update(state, g_chunkB, sizeof(g_chunkB));

    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(state ^ LIB_CRC8_SAE_J1850_INIT),
        Lib_Crc8SaeJ1850(combined, sizeof(combined)));
}

/*==================================================================================================
*                                      CRC-8 / AUTOSAR (H2F)
*==================================================================================================*/
void test_crc8_autosar_check_value(void)
{
    /* Catalogue check value: CRC-8/AUTOSAR("123456789") = 0xDF */
    TEST_ASSERT_EQUAL_HEX8(0xDFu, Lib_Crc8Autosar(g_checkData, 9u));
}

void test_crc8_autosar_streaming_two_parts(void)
{
    uint8_t combined[sizeof(g_chunkA) + sizeof(g_chunkB)];
    uint8_t state;

    (void)memcpy(combined, g_chunkA, sizeof(g_chunkA));
    (void)memcpy(combined + sizeof(g_chunkA), g_chunkB, sizeof(g_chunkB));

    state = LIB_CRC8_AUTOSAR_INIT;
    state = Lib_Crc8AutosarUpdate(state, g_chunkA, sizeof(g_chunkA));
    state = Lib_Crc8AutosarUpdate(state, g_chunkB, sizeof(g_chunkB));

    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(state ^ LIB_CRC8_AUTOSAR_INIT),
        Lib_Crc8Autosar(combined, sizeof(combined)));
}

/*==================================================================================================
*                                      CRC-16 / CCITT-FALSE
*==================================================================================================*/
void test_crc16_ccitt_false_check_value(void)
{
    /* Catalogue check value: CRC-16/CCITT-FALSE("123456789") = 0x29B1 */
    TEST_ASSERT_EQUAL_HEX16(0x29B1u, Lib_Crc16CcittFalse(g_checkData, 9u));
}

void test_crc16_ccitt_false_streaming_two_parts(void)
{
    uint8_t combined[sizeof(g_chunkA) + sizeof(g_chunkB)];
    uint16_t state;

    (void)memcpy(combined, g_chunkA, sizeof(g_chunkA));
    (void)memcpy(combined + sizeof(g_chunkA), g_chunkB, sizeof(g_chunkB));

    state = LIB_CRC16_CCITT_FALSE_INIT;
    state = Lib_Crc16CcittFalseUpdate(state, g_chunkA, sizeof(g_chunkA));
    state = Lib_Crc16CcittFalseUpdate(state, g_chunkB, sizeof(g_chunkB));

    /* final XOR = 0x0000 */
    TEST_ASSERT_EQUAL_HEX16(state, Lib_Crc16CcittFalse(combined, sizeof(combined)));
}

/*==================================================================================================
*                                      CRC-16 / XMODEM
*==================================================================================================*/
void test_crc16_xmodem_check_value(void)
{
    /* Catalogue check value: CRC-16/XMODEM("123456789") = 0x31C3 */
    TEST_ASSERT_EQUAL_HEX16(0x31C3u, Lib_Crc16Xmodem(g_checkData, 9u));
}

void test_crc16_xmodem_streaming_two_parts(void)
{
    uint8_t combined[sizeof(g_chunkA) + sizeof(g_chunkB)];
    uint16_t state;

    (void)memcpy(combined, g_chunkA, sizeof(g_chunkA));
    (void)memcpy(combined + sizeof(g_chunkA), g_chunkB, sizeof(g_chunkB));

    state = LIB_CRC16_XMODEM_INIT;
    state = Lib_Crc16XmodemUpdate(state, g_chunkA, sizeof(g_chunkA));
    state = Lib_Crc16XmodemUpdate(state, g_chunkB, sizeof(g_chunkB));

    /* final XOR = 0x0000 */
    TEST_ASSERT_EQUAL_HEX16(state, Lib_Crc16Xmodem(combined, sizeof(combined)));
}

/*==================================================================================================
*                                      CRC-32 / ISO-HDLC
*==================================================================================================*/
void test_crc32_iso_hdlc_check_value(void)
{
    /* Catalogue check value: CRC-32/IEEE("123456789") = 0xCBF43926 */
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, Lib_Crc32IsoHdlc(g_checkData, 9u));
}

void test_crc32_iso_hdlc_streaming_two_parts(void)
{
    uint8_t combined[sizeof(g_chunkA) + sizeof(g_chunkB)];
    uint32_t state;

    (void)memcpy(combined, g_chunkA, sizeof(g_chunkA));
    (void)memcpy(combined + sizeof(g_chunkA), g_chunkB, sizeof(g_chunkB));

    state = LIB_CRC32_ISO_HDLC_INIT;
    state = Lib_Crc32IsoHdlcUpdate(state, g_chunkA, sizeof(g_chunkA));
    state = Lib_Crc32IsoHdlcUpdate(state, g_chunkB, sizeof(g_chunkB));

    TEST_ASSERT_EQUAL_HEX32(
        (state ^ LIB_CRC32_ISO_HDLC_INIT),
        Lib_Crc32IsoHdlc(combined, sizeof(combined)));
}

void test_crc32_iso_hdlc_bit_flip_detects_change(void)
{
    uint8_t data[] = { 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u };
    uint32_t crc1;
    uint32_t crc2;

    crc1 = Lib_Crc32IsoHdlc(data, sizeof(data));
    data[4] = (uint8_t)(data[4] ^ 0x40u);   /* single bit flip */
    crc2 = Lib_Crc32IsoHdlc(data, sizeof(data));

    TEST_ASSERT_TRUE(crc1 != crc2);
}

/*==================================================================================================
*                                      RUNNER
*==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_crc8_sae_j1850_check_value);
    RUN_TEST(test_crc8_sae_j1850_empty);
    RUN_TEST(test_crc8_sae_j1850_streaming_two_parts);

    RUN_TEST(test_crc8_autosar_check_value);
    RUN_TEST(test_crc8_autosar_streaming_two_parts);

    RUN_TEST(test_crc16_ccitt_false_check_value);
    RUN_TEST(test_crc16_ccitt_false_streaming_two_parts);

    RUN_TEST(test_crc16_xmodem_check_value);
    RUN_TEST(test_crc16_xmodem_streaming_two_parts);

    RUN_TEST(test_crc32_iso_hdlc_check_value);
    RUN_TEST(test_crc32_iso_hdlc_streaming_two_parts);
    RUN_TEST(test_crc32_iso_hdlc_bit_flip_detects_change);

    return UNITY_END();
}

/******************************************************************************
 * @file    test_isotp.c
 * @brief   Unit tests for ISO 15765-2 IsoTp protocol stack
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* Include Unity test framework */
#include "../../unity/unity.h"

/* Include IsoTp headers */
#include "../../../src/diagnostics/isotp/isotp_types.h"
#include "../../../src/diagnostics/isotp/isotp_core.h"

/******************************************************************************
 * Test Configuration
 ******************************************************************************/

#define TEST_CHANNEL_ID         0U
#define TEST_CAN_ID_TX          0x7E0U
#define TEST_CAN_ID_RX          0x7E8U

/******************************************************************************
 * Test Global Variables
 ******************************************************************************/

static Isotp_ChannelConfigType g_testConfig;
static uint8_t g_txBuffer[256];
static uint8_t g_rxBuffer[256];
static uint8_t g_canFrame[64];
static uint8_t g_frameLen;
static bool g_txCompleteCalled;
static bool g_rxIndicationCalled;
static Isotp_ReturnType g_lastTxResult;

/******************************************************************************
 * Mock Functions
 ******************************************************************************/

static Isotp_ReturnType MockCanTransmit(
    uint32_t canId,
    uint8_t *data,
    uint8_t length,
    bool isFd,
    void *userData
)
{
    (void)canId;
    (void)isFd;
    (void)userData;
    
    memcpy(g_canFrame, data, length);
    g_frameLen = length;
    
    return ISOTP_E_OK;
}

static void TxConfirmationCallback(
    uint8_t channelId,
    Isotp_ReturnType result,
    void *userData
)
{
    (void)channelId;
    (void)userData;
    
    g_txCompleteCalled = true;
    g_lastTxResult = result;
}

static void RxIndicationCallback(
    uint8_t channelId,
    uint8_t *data,
    uint16_t length,
    void *userData
)
{
    (void)channelId;
    (void)userData;
    
    g_rxIndicationCalled = true;
    memcpy(g_rxBuffer, data, length);
}

/******************************************************************************
 * Test Setup and Teardown
 ******************************************************************************/

void setUp(void)
{
    /* Initialize test configuration */
    memset(&g_testConfig, 0, sizeof(g_testConfig));
    g_testConfig.channelId = TEST_CHANNEL_ID;
    g_testConfig.addressing.addrMode = ISOTP_NORMAL_ADDRESSING;
    g_testConfig.addressing.txCanId = TEST_CAN_ID_TX;
    g_testConfig.addressing.rxCanId = TEST_CAN_ID_RX;
    g_testConfig.timing.nAs = 1000U;
    g_testConfig.timing.nBs = 1000U;
    g_testConfig.timing.nCs = 1000U;
    g_testConfig.timing.nCr = 1000U;
    g_testConfig.canFrameType = ISOTP_CAN_20;
    g_testConfig.rxBufferSize = 256U;
    g_testConfig.txBufferSize = 256U;
    
    /* Reset test variables */
    g_txCompleteCalled = false;
    g_rxIndicationCalled = false;
    g_lastTxResult = ISOTP_E_NOT_OK;
    memset(g_txBuffer, 0, sizeof(g_txBuffer));
    memset(g_rxBuffer, 0, sizeof(g_rxBuffer));
    memset(g_canFrame, 0, sizeof(g_canFrame));
    g_frameLen = 0;
}

void tearDown(void)
{
    IsoTp_DeInit();
}

/******************************************************************************
 * N_PCI Parsing Tests
 ******************************************************************************/

void test_ParseNpci_SingleFrame(void)
{
    /* SF with 3 bytes payload (normal addressing) */
    uint8_t frame[] = {0x03, 0x22, 0xF1, 0x90, 0xAA, 0xAA, 0xAA, 0xAA};
    Isotp_NpciType npci;
    
    Isotp_ReturnType result = IsoTp_ParseNpci(frame, 8, &npci, ISOTP_NORMAL_ADDRESSING);
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_SF, npci.pciType);
    TEST_ASSERT_EQUAL(3, npci.dataLength);
}

void test_ParseNpci_FirstFrame(void)
{
    /* FF with 100 bytes total length */
    uint8_t frame[] = {0x10, 0x64, 0x22, 0xF1, 0x90, 0xAA, 0xAA, 0xAA};
    Isotp_NpciType npci;
    
    Isotp_ReturnType result = IsoTp_ParseNpci(frame, 8, &npci, ISOTP_NORMAL_ADDRESSING);
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_FF, npci.pciType);
    TEST_ASSERT_EQUAL(100, npci.dataLength);
}

void test_ParseNpci_ConsecutiveFrame(void)
{
    /* CF with sequence number 1 */
    uint8_t frame[] = {0x21, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    Isotp_NpciType npci;
    
    Isotp_ReturnType result = IsoTp_ParseNpci(frame, 8, &npci, ISOTP_NORMAL_ADDRESSING);
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_CF, npci.pciType);
    TEST_ASSERT_EQUAL(1, npci.seqNum);
}

void test_ParseNpci_FlowControl(void)
{
    /* FC with CTS, BS=8, STmin=20ms */
    uint8_t frame[] = {0x30, 0x08, 0x14, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    Isotp_NpciType npci;
    
    Isotp_ReturnType result = IsoTp_ParseNpci(frame, 8, &npci, ISOTP_NORMAL_ADDRESSING);
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_FC, npci.pciType);
    TEST_ASSERT_EQUAL(ISOTP_FS_CTS, npci.flowStatus);
    TEST_ASSERT_EQUAL(8, npci.blockSize);
    TEST_ASSERT_EQUAL(20, npci.stMin);
}

/******************************************************************************
 * Frame Building Tests
 ******************************************************************************/

void test_BuildSingleFrame_Normal(void)
{
    uint8_t data[] = {0x22, 0xF1, 0x90};  /* UDS ReadDataByIdentifier */
    uint8_t frame[8];
    uint8_t length;
    
    Isotp_ReturnType result = IsoTp_BuildSingleFrame(
        data, sizeof(data),
        frame, &length,
        ISOTP_NORMAL_ADDRESSING,
        false
    );
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(8, length);  /* CAN 2.0 always 8 bytes */
    TEST_ASSERT_EQUAL(0x03, frame[0]);  /* SF with DL=3 */
    TEST_ASSERT_EQUAL(0x22, frame[1]);
    TEST_ASSERT_EQUAL(0xF1, frame[2]);
    TEST_ASSERT_EQUAL(0x90, frame[3]);
}

void test_BuildSingleFrame_Extended(void)
{
    uint8_t data[] = {0x22, 0xF1, 0x90};
    uint8_t frame[8];
    uint8_t length;
    
    Isotp_ReturnType result = IsoTp_BuildSingleFrame(
        data, sizeof(data),
        frame, &length,
        ISOTP_EXTENDED_ADDRESSING,
        false
    );
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(8, length);
    TEST_ASSERT_EQUAL(0x00, frame[0]);  /* Extended address */
    TEST_ASSERT_EQUAL(0x03, frame[1]);  /* SF with DL=3 */
}

void test_BuildFirstFrame(void)
{
    uint8_t data[6] = {0x22, 0xF1, 0x90, 0x01, 0x02, 0x03};
    uint8_t frame[8];
    uint8_t length;
    
    Isotp_ReturnType result = IsoTp_BuildFirstFrame(
        100,  /* Total length */
        data,
        frame, &length,
        ISOTP_NORMAL_ADDRESSING,
        false
    );
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(8, length);
    TEST_ASSERT_EQUAL(0x10, frame[0]);  /* FF */
    TEST_ASSERT_EQUAL(0x64, frame[1]);  /* Length = 100 */
    TEST_ASSERT_EQUAL(0x22, frame[2]);  /* First data byte */
}

void test_BuildConsecutiveFrame(void)
{
    uint8_t data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t frame[8];
    uint8_t length;
    
    Isotp_ReturnType result = IsoTp_BuildConsecutiveFrame(
        1,  /* Sequence number 1 */
        data, sizeof(data),
        frame, &length,
        ISOTP_NORMAL_ADDRESSING,
        false
    );
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(8, length);
    TEST_ASSERT_EQUAL(0x21, frame[0]);  /* CF with SN=1 */
    TEST_ASSERT_EQUAL(0x01, frame[1]);
}

void test_BuildFlowControlFrame(void)
{
    uint8_t frame[8];
    uint8_t length;
    
    Isotp_ReturnType result = IsoTp_BuildFlowControlFrame(
        ISOTP_FS_CTS,
        8,      /* Block size */
        20,     /* STmin = 20ms */
        frame, &length,
        ISOTP_NORMAL_ADDRESSING
    );
    
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    TEST_ASSERT_EQUAL(3, length);
    TEST_ASSERT_EQUAL(0x30, frame[0]);  /* FC with FS=CTS */
    TEST_ASSERT_EQUAL(0x08, frame[1]);  /* BS=8 */
    TEST_ASSERT_EQUAL(0x14, frame[2]);  /* STmin=20ms */
}

/******************************************************************************
 * Utility Function Tests
 ******************************************************************************/

void test_IsMultiFrame(void)
{
    /* 7 bytes = single frame for CAN 2.0 normal addressing */
    TEST_ASSERT_FALSE(IsoTp_IsMultiFrame(7, ISOTP_NORMAL_ADDRESSING, false));
    
    /* 8 bytes = multi-frame */
    TEST_ASSERT_TRUE(IsoTp_IsMultiFrame(8, ISOTP_NORMAL_ADDRESSING, false));
    
    /* Extended addressing reduces SF capacity */
    TEST_ASSERT_FALSE(IsoTp_IsMultiFrame(6, ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_TRUE(IsoTp_IsMultiFrame(7, ISOTP_EXTENDED_ADDRESSING, false));
    
    /* CAN FD can handle more in SF */
    TEST_ASSERT_FALSE(IsoTp_IsMultiFrame(62, ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_TRUE(IsoTp_IsMultiFrame(63, ISOTP_NORMAL_ADDRESSING, true));
}

void test_CalculateNumFrames(void)
{
    /* Single frame */
    TEST_ASSERT_EQUAL(1, IsoTp_CalculateNumFrames(7, ISOTP_NORMAL_ADDRESSING, false));
    
    /* Multi-frame: 100 bytes total
     * FF: 6 bytes, remaining: 94 bytes
     * CF: 7 bytes each, need ceil(94/7) = 14 CF frames
     * Total: 1 + 14 = 15 frames
     */
    TEST_ASSERT_EQUAL(15, IsoTp_CalculateNumFrames(100, ISOTP_NORMAL_ADDRESSING, false));
}

void test_StMinToMs(void)
{
    /* 0x00-0x7F: 0-127 ms */
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, IsoTp_StMinToMs(0x00));
    TEST_ASSERT_FLOAT_WITHIN(0.1, 127.0, IsoTp_StMinToMs(0x7F));
    
    /* 0xF1-0xF9: 100-900 us */
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.1, IsoTp_StMinToMs(0xF1));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.9, IsoTp_StMinToMs(0xF9));
    
    /* Reserved: use default */
    TEST_ASSERT_FLOAT_WITHIN(0.1, ISOTP_DEFAULT_STMIN, IsoTp_StMinToMs(0x80));
}

void test_IsValidStMin(void)
{
    /* Valid ms values */
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0x00));
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0x7F));
    
    /* Valid us values */
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0xF1));
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0xF9));
    
    /* Invalid values */
    TEST_ASSERT_FALSE(IsoTp_IsValidStMin(0x80));
    TEST_ASSERT_FALSE(IsoTp_IsValidStMin(0xF0));
    TEST_ASSERT_FALSE(IsoTp_IsValidStMin(0xFA));
}

/******************************************************************************
 * Integration Tests
 ******************************************************************************/

void test_InitAndDeinit(void)
{
    Isotp_ReturnType result = IsoTp_Init(&g_testConfig, 1);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    
    IsoTp_DeInit();
    
    /* After deinit, re-init should work */
    result = IsoTp_Init(&g_testConfig, 1);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
}

void test_TransmitSingleFrame(void)
{
    /* Initialize IsoTp */
    IsoTp_Init(&g_testConfig, 1);
    
    /* Register callbacks */
    IsoTp_RegisterCanTransmitCallback(MockCanTransmit);
    IsoTp_RegisterTxConfirmationCallback(TxConfirmationCallback);
    
    /* Prepare test data */
    uint8_t data[] = {0x22, 0xF1, 0x90};  /* UDS ReadDataByIdentifier */
    
    /* Start transmission */
    Isotp_ReturnType result = IsoTp_Transmit(TEST_CHANNEL_ID, data, sizeof(data), NULL);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, result);
    
    /* Check frame was built correctly */
    TEST_ASSERT_EQUAL(0x03, g_canFrame[0]);  /* SF with DL=3 */
    TEST_ASSERT_EQUAL(0x22, g_canFrame[1]);
    TEST_ASSERT_EQUAL(0xF1, g_canFrame[2]);
    TEST_ASSERT_EQUAL(0x90, g_canFrame[3]);
}

void test_TransmitInvalidParameters(void)
{
    IsoTp_Init(&g_testConfig, 1);
    
    uint8_t data[] = {0x22, 0xF1, 0x90};
    
    /* Invalid channel */
    Isotp_ReturnType result = IsoTp_Transmit(99, data, sizeof(data), NULL);
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, result);
    
    /* NULL data */
    result = IsoTp_Transmit(TEST_CHANNEL_ID, NULL, sizeof(data), NULL);
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, result);
    
    /* Zero length */
    result = IsoTp_Transmit(TEST_CHANNEL_ID, data, 0, NULL);
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, result);
    
    /* Length too large */
    result = IsoTp_Transmit(TEST_CHANNEL_ID, data, 5000, NULL);
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, result);
}

void test_GetMaxSingleFramePayload(void)
{
    /* CAN 2.0 normal addressing */
    TEST_ASSERT_EQUAL(7, IsoTp_GetMaxSingleFramePayload(ISOTP_NORMAL_ADDRESSING, false));
    
    /* CAN 2.0 extended addressing */
    TEST_ASSERT_EQUAL(6, IsoTp_GetMaxSingleFramePayload(ISOTP_EXTENDED_ADDRESSING, false));
    
    /* CAN FD normal addressing */
    TEST_ASSERT_EQUAL(62, IsoTp_GetMaxSingleFramePayload(ISOTP_NORMAL_ADDRESSING, true));
    
    /* CAN FD extended addressing */
    TEST_ASSERT_EQUAL(61, IsoTp_GetMaxSingleFramePayload(ISOTP_EXTENDED_ADDRESSING, true));
}

/******************************************************************************
 * Test Runner
 ******************************************************************************/

int main(void)
{
    UNITY_BEGIN();
    
    /* N_PCI Parsing Tests */
    RUN_TEST(test_ParseNpci_SingleFrame);
    RUN_TEST(test_ParseNpci_FirstFrame);
    RUN_TEST(test_ParseNpci_ConsecutiveFrame);
    RUN_TEST(test_ParseNpci_FlowControl);
    
    /* Frame Building Tests */
    RUN_TEST(test_BuildSingleFrame_Normal);
    RUN_TEST(test_BuildSingleFrame_Extended);
    RUN_TEST(test_BuildFirstFrame);
    RUN_TEST(test_BuildConsecutiveFrame);
    RUN_TEST(test_BuildFlowControlFrame);
    
    /* Utility Function Tests */
    RUN_TEST(test_IsMultiFrame);
    RUN_TEST(test_CalculateNumFrames);
    RUN_TEST(test_StMinToMs);
    RUN_TEST(test_IsValidStMin);
    RUN_TEST(test_GetMaxSingleFramePayload);
    
    /* Integration Tests */
    RUN_TEST(test_InitAndDeinit);
    RUN_TEST(test_TransmitSingleFrame);
    RUN_TEST(test_TransmitInvalidParameters);
    
    return UNITY_END();
}

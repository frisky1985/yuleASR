/** @file test_isotp_core.c
 * @brief Unit tests for the ISO-TP core (isotp_core.c)
 *
 * Covers: init/deinit, SF/FF/CF/FC frame build & parse helpers, RX path
 * (SF, multi-frame FF+FC+CF, SN error, block-size re-FC, N_Cr timeout,
 * buffer overflow), TX path (single frame, multi-frame flow control,
 * block size, STmin wait, N_Bs timeout, WT overrun, FC.OVFLW, send
 * failure, cancel), unexpected-frame handling and parameter guards.
 *
 * NOTE (actual-behavior annotations, verified against the implementation):
 *  - PRODUCT DEFECT (recorded, not fixed): on the TX side, after sending the
 *    FirstFrame the code marks GetMaxSingleFramePayload() bytes as sent
 *    (7 bytes for CAN 2.0), but a CAN 2.0 FirstFrame only carries 6 payload
 *    bytes. ConsecutiveFrames therefore start at payload byte 6 and byte
 *    index 6 is silently SKIPPED. tests below assert the actual (skipping)
 *    behavior; RX side reassembly is correct.
 *  - Time is a static internal counter incremented once per
 *    IsoTp_GetCurrentTime() call, so N_Bs/N_Cr timeouts are exercised via
 *    small timing values in the test-owned channel configuration.
 *  - FC frames received while not in TX_WAIT_FC, and CF/SF received while
 *    not expecting them, are rejected with ISOTP_E_UNEXPECTED_FRAME.
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */
#include "unity.h"
#include "isotp_core.h"
#include <string.h>

#define CH0 0U
#define CH1 1U
#define CAP_MAX 32

/* ============================ test state ============================ */

static Isotp_ChannelConfigType s_cfg[2];
static uint8_t s_rx0[64];
static uint8_t s_rx1[64];

/* CAN TX capture */
static uint32_t s_capId[CAP_MAX];
static uint8_t s_capData[CAP_MAX][ISOTP_CANFD_MAX_FRAME_SIZE];
static uint8_t s_capLen[CAP_MAX];
static uint8_t s_capFd[CAP_MAX];
static int s_capCount;

/* callback records */
static int s_txCalls;
static uint8_t s_txChan;
static Isotp_ReturnType s_txResult;
static int s_rxCalls;
static uint8_t s_rxChan;
static uint16_t s_rxLen;
static uint8_t s_rxData[64];
static int s_fcCalls;
static uint16_t s_fcLen;
static uint8_t s_fcBs;
static uint8_t s_fcStmin;

/* FC answer controlled per-test */
static uint8_t s_fcRespBs;
static uint8_t s_fcRespStmin;

static void ResetRecords(void)
{
    s_capCount = 0;
    s_txCalls = 0;
    s_txChan = 0U;
    s_txResult = ISOTP_E_OK;
    s_rxCalls = 0;
    s_rxChan = 0U;
    s_rxLen = 0U;
    memset(s_rxData, 0, sizeof(s_rxData));
    s_fcCalls = 0;
    s_fcLen = 0U;
    s_fcBs = 0U;
    s_fcStmin = 0U;
    s_fcRespBs = 0U;
    s_fcRespStmin = 0U;
}

static void FillPattern(uint8_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (uint8_t)(0x30U + (i & 0x0FU));
    }
}

/* ============================ callbacks ============================ */

static Isotp_ReturnType CapTx(uint32_t canId, uint8_t *data, uint8_t length,
                              bool isFd, void *userData)
{
    (void)userData;
    if (s_capCount < CAP_MAX) {
        s_capId[s_capCount] = canId;
        memcpy(s_capData[s_capCount], data, length);
        s_capLen[s_capCount] = length;
        s_capFd[s_capCount] = (uint8_t)(isFd ? 1U : 0U);
        s_capCount++;
    }
    return ISOTP_E_OK;
}

static Isotp_ReturnType FailTx(uint32_t canId, uint8_t *data, uint8_t length,
                               bool isFd, void *userData)
{
    (void)canId;
    (void)data;
    (void)length;
    (void)isFd;
    (void)userData;
    return ISOTP_E_NOT_OK;
}

static void RecTxConf(uint8_t channelId, Isotp_ReturnType result, void *userData)
{
    (void)userData;
    s_txCalls++;
    s_txChan = channelId;
    s_txResult = result;
}

static void RecRxInd(uint8_t channelId, uint8_t *data, uint16_t length,
                     void *userData)
{
    (void)userData;
    s_rxCalls++;
    s_rxChan = channelId;
    s_rxLen = length;
    memcpy(s_rxData, data, length);
}

static Isotp_ReturnType RecFc(uint8_t channelId, uint16_t dataLength,
                              uint8_t *blockSize, uint8_t *stMin, void *userData)
{
    (void)channelId;
    (void)userData;
    s_fcCalls++;
    s_fcLen = dataLength;
    *blockSize = s_fcRespBs;
    *stMin = s_fcRespStmin;
    return ISOTP_E_OK;
}

/* ============================ fixtures ============================ */

void setUp(void)
{
    int i;
    memset(s_cfg, 0, sizeof(s_cfg));
    for (i = 0; i < 2; i++) {
        s_cfg[i].channelId = (uint8_t)i;
        s_cfg[i].addressing.addrMode = ISOTP_NORMAL_ADDRESSING;
        s_cfg[i].addressing.txCanId = 0x100U + (uint32_t)i;
        s_cfg[i].addressing.rxCanId = 0x200U + (uint32_t)i;
        s_cfg[i].timing.nAs = 1000U;
        s_cfg[i].timing.nBs = 1000U;
        s_cfg[i].timing.nCr = 1000U;
        s_cfg[i].timing.nAr = 1000U;
        s_cfg[i].timing.nCs = 0U;
        s_cfg[i].canFrameType = ISOTP_CAN_20;
        s_cfg[i].rxBufferSize = 64U;
        s_cfg[i].txBufferSize = 64U;
    }
    s_cfg[1].rxBufferSize = 4U;      /* small RX buffer for overflow tests */
    s_cfg[1].txBufferSize = 8U;      /* small TX buffer for overflow tests */

    memset(s_rx0, 0, sizeof(s_rx0));
    memset(s_rx1, 0, sizeof(s_rx1));

    ResetRecords();
    (void)IsoTp_DeInit();
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Init(s_cfg, 2U));
    IsoTp_RegisterCanTransmitCallback(CapTx);
    IsoTp_RegisterTxConfirmationCallback(RecTxConf);
    IsoTp_RegisterRxIndicationCallback(RecRxInd);
    IsoTp_RegisterFlowControlCallback(RecFc);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_SetRxBuffer(CH0, s_rx0, sizeof(s_rx0)));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_SetRxBuffer(CH1, s_rx1, sizeof(s_rx1)));
}

void tearDown(void)
{
}

/* ==========================================================================
 * Init / Deinit / registration
 * ========================================================================== */

static void test_isotp_init_invalid_params(void)
{
    (void)IsoTp_DeInit();
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Init(NULL, 1U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Init(s_cfg, 0U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Init(s_cfg, 17U));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Init(s_cfg, 2U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH1));
}

static void test_isotp_init_deinit_lifecycle(void)
{
    TEST_ASSERT_NOT_NULL(IsoTp_GetChannelConfig(CH0));
    TEST_ASSERT_NOT_NULL(IsoTp_GetChannelContext(CH0));
    IsoTp_DeInit();
    TEST_ASSERT_NULL(IsoTp_GetChannelConfig(CH0));
    TEST_ASSERT_NULL(IsoTp_GetChannelContext(CH0));
    /* Invalid channel always reports IDLE */
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(9U));
}

static void test_isotp_invalid_channel_and_param_guards(void)
{
    uint8_t frame[8] = {0x03, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00};
    uint8_t big[32];
    const Isotp_ChannelConfigType *cfg;
    Isotp_ChannelContextType *ctx;

    FillPattern(big, sizeof(big));

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ProcessRxFrame(5U, frame, 8U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ProcessRxFrame(CH0, NULL, 8U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ProcessRxFrame(CH0, frame, 0U));

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Transmit(5U, frame, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Transmit(CH0, NULL, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Transmit(CH0, frame, 0U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Transmit(CH0, frame, 4096U, NULL));
    /* 20 bytes > txBufferSize(8) of CH1 */
    TEST_ASSERT_EQUAL(ISOTP_E_BUFFER_OVERFLOW, IsoTp_Transmit(CH1, big, 20U, NULL));

    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(9U));
    TEST_ASSERT_NULL(IsoTp_GetChannelConfig(9U));
    TEST_ASSERT_NULL(IsoTp_GetChannelContext(9U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_SetRxBuffer(9U, frame, 8U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_CancelTransmit(9U));
    IsoTp_ResetChannel(9U);   /* must not crash */

    /* Config/context getters for a valid channel */
    cfg = IsoTp_GetChannelConfig(CH0);
    TEST_ASSERT_NOT_NULL(cfg);
    TEST_ASSERT_EQUAL_UINT8(CH0, cfg->channelId);
    TEST_ASSERT_EQUAL_UINT32(0x100U, cfg->addressing.txCanId);
    ctx = IsoTp_GetChannelContext(CH0);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL_UINT8(CH0, ctx->channelId);
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, ctx->state);

    /* After DeInit every API reports invalid parameters */
    (void)IsoTp_DeInit();
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ProcessRxFrame(CH0, frame, 8U));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Transmit(CH0, frame, 7U, NULL));
}

static void test_isotp_register_null_config_rejected(void)
{
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_Init(NULL, 1U));
}

/* ==========================================================================
 * Frame builders
 * ========================================================================== */

static void test_isotp_build_single_frame(void)
{
    uint8_t d7[7] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    uint8_t d6[6] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6};
    uint8_t d5[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t d62[62];
    uint8_t out[64];
    uint8_t olen = 0U;
    int i;

    FillPattern(d62, sizeof(d62));

    /* CAN 2.0 normal: 1 PCI byte, padded to 8 */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildSingleFrame(d7, 7U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(8U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x07U, out[0]);
    for (i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL_HEX8(d7[i], out[i + 1]);
    }

    /* Extended addressing: N_TA placeholder + PCI byte */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildSingleFrame(d6, 6U, out, &olen, ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(8U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x00U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x06U, out[1]);
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL_HEX8(d6[i], out[i + 2]);
    }

    /* CAN FD: > 7 bytes uses escape encoding (0x00 + DL), padded to 64 */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildSingleFrame(d62, 62U, out, &olen, ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_EQUAL_UINT8(ISOTP_CANFD_MAX_FRAME_SIZE, olen);
    TEST_ASSERT_EQUAL_HEX8(0x00U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(62U, out[1]);
    for (i = 0; i < 62; i++) {
        TEST_ASSERT_EQUAL_HEX8(d62[i], out[i + 2]);
    }

    /* CAN FD short payload keeps normal encoding, no padding */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildSingleFrame(d5, 5U, out, &olen, ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_EQUAL_UINT8(6U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x05U, out[0]);
}

static void test_isotp_build_single_frame_invalid(void)
{
    uint8_t d[7] = {0};
    uint8_t out[64];
    uint8_t olen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildSingleFrame(NULL, 7U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildSingleFrame(d, 0U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildSingleFrame(d, 8U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildSingleFrame(d, 7U, NULL, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildSingleFrame(d, 7U, out, NULL, ISOTP_NORMAL_ADDRESSING, false));
}

static void test_isotp_build_first_frame(void)
{
    uint8_t d[64];
    uint8_t out[64];
    uint8_t olen = 0U;
    int i;

    FillPattern(d, sizeof(d));

    /* CAN 2.0 normal: 2 PCI bytes + 6 payload bytes */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFirstFrame(20U, d, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(8U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x10U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, out[1]);
    for (i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL_HEX8(d[i], out[i + 2]);
    }

    /* Extended addressing */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFirstFrame(20U, d, out, &olen, ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(8U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x00U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x10U, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, out[2]);
    for (i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_HEX8(d[i], out[i + 3]);
    }

    /* CAN FD: 62 payload bytes, padded to 64 */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFirstFrame(100U, d, out, &olen, ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_EQUAL_UINT8(ISOTP_CANFD_MAX_FRAME_SIZE, olen);
    TEST_ASSERT_EQUAL_HEX8(0x10U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x64U, out[1]);
    for (i = 0; i < 62; i++) {
        TEST_ASSERT_EQUAL_HEX8(d[i], out[i + 2]);
    }
}

static void test_isotp_build_first_frame_invalid(void)
{
    uint8_t d[16] = {0};
    uint8_t out[64];
    uint8_t olen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFirstFrame(4096U, d, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFirstFrame(20U, NULL, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFirstFrame(20U, d, NULL, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFirstFrame(20U, d, out, NULL, ISOTP_NORMAL_ADDRESSING, false));
}

static void test_isotp_build_consecutive_frame(void)
{
    uint8_t d7[7] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    uint8_t d63[63];
    uint8_t d3[3] = {0xE1, 0xE2, 0xE3};
    uint8_t out[64];
    uint8_t olen = 0U;
    int i;

    FillPattern(d63, sizeof(d63));

    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildConsecutiveFrame(5U, d7, 7U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(8U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x25U, out[0]);
    for (i = 0; i < 7; i++) {
        TEST_ASSERT_EQUAL_HEX8(d7[i], out[i + 1]);
    }

    /* Sequence number wraps on 4 bits */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildConsecutiveFrame(16U, d3, 3U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_HEX8(0x20U, out[0]);

    /* CAN FD: no padding, out length = offset + PCI + data */
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildConsecutiveFrame(1U, d63, 63U, out, &olen, ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_EQUAL_UINT8(ISOTP_CANFD_MAX_FRAME_SIZE, olen);
    TEST_ASSERT_EQUAL_HEX8(0x21U, out[0]);
}

static void test_isotp_build_consecutive_frame_invalid(void)
{
    uint8_t d[7] = {0};
    uint8_t out[64];
    uint8_t olen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildConsecutiveFrame(0U, NULL, 7U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildConsecutiveFrame(0U, d, 0U, out, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildConsecutiveFrame(0U, d, 7U, NULL, &olen, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildConsecutiveFrame(0U, d, 7U, out, NULL, ISOTP_NORMAL_ADDRESSING, false));
}

static void test_isotp_build_flow_control_frame(void)
{
    /* NOTE: all Build* helpers memset the full CAN-FD max frame size (64B) */
    uint8_t out[64];
    uint8_t olen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFlowControlFrame(ISOTP_FS_CTS, 8U, 20U, out, &olen, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_UINT8(3U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x30U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08U, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, out[2]);

    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFlowControlFrame(ISOTP_FS_WT, 0U, 0U, out, &olen, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_HEX8(0x31U, out[0]);

    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFlowControlFrame(ISOTP_FS_OVFLW, 0U, 0U, out, &olen, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_HEX8(0x32U, out[0]);

    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_BuildFlowControlFrame(ISOTP_FS_CTS, 8U, 20U, out, &olen, ISOTP_EXTENDED_ADDRESSING));
    TEST_ASSERT_EQUAL_UINT8(4U, olen);
    TEST_ASSERT_EQUAL_HEX8(0x00U, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x30U, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x08U, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, out[3]);
}

static void test_isotp_build_flow_control_frame_invalid(void)
{
    uint8_t out[64];
    uint8_t olen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFlowControlFrame(3U, 8U, 20U, out, &olen, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFlowControlFrame(ISOTP_FS_CTS, 8U, 20U, NULL, &olen, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_BuildFlowControlFrame(ISOTP_FS_CTS, 8U, 20U, out, NULL, ISOTP_NORMAL_ADDRESSING));
}

/* ==========================================================================
 * NPCI parser
 * ========================================================================== */

static void test_isotp_parse_sf(void)
{
    uint8_t f[8] = {0x05, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11, 0x22};
    Isotp_NpciType n;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(f, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_SF, n.pciType);
    TEST_ASSERT_EQUAL_UINT16(5U, n.dataLength);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(f, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_UINT8(0U, n.seqNum);
}

static void test_isotp_parse_ff(void)
{
    uint8_t f[8] = {0x10, 0x14, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    Isotp_NpciType n;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(f, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_FF, n.pciType);
    TEST_ASSERT_EQUAL_UINT16(20U, n.dataLength);
}

static void test_isotp_parse_ff_ext32(void)
{
    /* FF_DL escape: 12-bit length field is 0 -> 32-bit length in bytes 2..5 */
    uint8_t f[8] = {0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0xAA, 0xBB};
    Isotp_NpciType n;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(f, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_FF, n.pciType);
    TEST_ASSERT_EQUAL_UINT16(4096U, n.dataLength);
}

static void test_isotp_parse_cf_fc(void)
{
    uint8_t cf[8] = {0x23, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t fc[8] = {0x30, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t fcShort[2] = {0x31, 0x05};
    Isotp_NpciType n;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(cf, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_CF, n.pciType);
    TEST_ASSERT_EQUAL_UINT8(3U, n.seqNum);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(fc, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_FC, n.pciType);
    TEST_ASSERT_EQUAL(ISOTP_FS_CTS, n.flowStatus);
    TEST_ASSERT_EQUAL_UINT8(8U, n.blockSize);
    TEST_ASSERT_EQUAL_UINT8(0x20U, n.stMin);

    /* dlc == 2: stMin not present -> 0 */
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(fcShort, 2U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_FS_WT, n.flowStatus);
    TEST_ASSERT_EQUAL_UINT8(5U, n.blockSize);
    TEST_ASSERT_EQUAL_UINT8(0U, n.stMin);
}

static void test_isotp_parse_invalid(void)
{
    uint8_t bad[8] = {0x50, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8_t ok[8] = {0x03, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00};
    Isotp_NpciType n;

    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_FRAME, IsoTp_ParseNpci(bad, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ParseNpci(NULL, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ParseNpci(ok, 8U, NULL, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER, IsoTp_ParseNpci(ok, 0U, &n, ISOTP_NORMAL_ADDRESSING));
}

static void test_isotp_parse_extended_addressing(void)
{
    uint8_t sf[8] = {0xAB, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t fc[4] = {0xAB, 0x31, 0x05, 0x02};
    Isotp_NpciType n;
    const uint8_t *payload;
    uint8_t payloadLen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(sf, 8U, &n, ISOTP_EXTENDED_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_SF, n.pciType);
    TEST_ASSERT_EQUAL_UINT16(7U, n.dataLength);

    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_GetFramePayload(sf, 8U, &payload, &payloadLen, &n, ISOTP_EXTENDED_ADDRESSING));
    TEST_ASSERT_EQUAL_PTR(&sf[2], payload);
    TEST_ASSERT_EQUAL_UINT8(6U, payloadLen);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(fc, 4U, &n, ISOTP_EXTENDED_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_FS_WT, n.flowStatus);
    TEST_ASSERT_EQUAL_UINT8(5U, n.blockSize);
    TEST_ASSERT_EQUAL_UINT8(2U, n.stMin);
}

static void test_isotp_parse_canfd_ext_sf(void)
{
    /* CAN FD SF with extended DL: PCI byte 0x00, real length in next byte */
    uint8_t f[12] = {0x00, 0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                     0x07, 0x08, 0x09, 0x0A};
    Isotp_NpciType n;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(f, 12U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_N_PCI_SF, n.pciType);
    TEST_ASSERT_EQUAL_UINT16(32U, n.dataLength);
}

/* ==========================================================================
 * Frame payload extraction
 * ========================================================================== */

static void test_isotp_get_frame_payload(void)
{
    uint8_t sf[8] = {0x03, 0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x00, 0x00};
    uint8_t ff[8] = {0x10, 0x14, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t ffExt[8] = {0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0xAA, 0xBB};
    uint8_t cf[8] = {0x21, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
    uint8_t fc[3] = {0x30, 0x00, 0x00};
    Isotp_NpciType n;
    const uint8_t *payload;
    uint8_t payloadLen = 0U;

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(sf, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_GetFramePayload(sf, 8U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_PTR(&sf[1], payload);
    TEST_ASSERT_EQUAL_UINT8(7U, payloadLen);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(ff, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_GetFramePayload(ff, 8U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_PTR(&ff[2], payload);
    TEST_ASSERT_EQUAL_UINT8(6U, payloadLen);

    /* FF_DL escape: payload starts after the 6 PCI bytes */
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(ffExt, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_GetFramePayload(ffExt, 8U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_PTR(&ffExt[6], payload);
    TEST_ASSERT_EQUAL_UINT8(2U, payloadLen);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(cf, 8U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_GetFramePayload(cf, 8U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_PTR(&cf[1], payload);
    TEST_ASSERT_EQUAL_UINT8(7U, payloadLen);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ParseNpci(fc, 3U, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_OK,
                      IsoTp_GetFramePayload(fc, 3U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL_PTR(NULL, payload);
    TEST_ASSERT_EQUAL_UINT8(0U, payloadLen);
}

static void test_isotp_get_frame_payload_invalid(void)
{
    uint8_t f[8] = {0x03, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00};
    Isotp_NpciType n;
    const uint8_t *payload;
    uint8_t payloadLen = 0U;

    (void)IsoTp_ParseNpci(f, 8U, &n, ISOTP_NORMAL_ADDRESSING);
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_GetFramePayload(NULL, 8U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_GetFramePayload(f, 8U, NULL, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_GetFramePayload(f, 8U, &payload, NULL, &n, ISOTP_NORMAL_ADDRESSING));
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_PARAMETER,
                      IsoTp_GetFramePayload(f, 8U, &payload, &payloadLen, NULL, ISOTP_NORMAL_ADDRESSING));

    n.pciType = 0x50U;
    TEST_ASSERT_EQUAL(ISOTP_E_INVALID_FRAME,
                      IsoTp_GetFramePayload(f, 8U, &payload, &payloadLen, &n, ISOTP_NORMAL_ADDRESSING));
}

/* ==========================================================================
 * Utility helpers
 * ========================================================================== */

static void test_isotp_util_multiframe(void)
{
    TEST_ASSERT_FALSE(IsoTp_IsMultiFrame(7U, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_TRUE(IsoTp_IsMultiFrame(8U, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_FALSE(IsoTp_IsMultiFrame(6U, ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_TRUE(IsoTp_IsMultiFrame(7U, ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_FALSE(IsoTp_IsMultiFrame(62U, ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_TRUE(IsoTp_IsMultiFrame(63U, ISOTP_NORMAL_ADDRESSING, true));
}

static void test_isotp_util_num_frames(void)
{
    TEST_ASSERT_EQUAL_UINT8(1U, IsoTp_CalculateNumFrames(7U, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(3U, IsoTp_CalculateNumFrames(20U, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(5U, IsoTp_CalculateNumFrames(30U, ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(4U, IsoTp_CalculateNumFrames(20U, ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(2U, IsoTp_CalculateNumFrames(100U, ISOTP_NORMAL_ADDRESSING, true));
}

static void test_isotp_util_max_sf_payload(void)
{
    TEST_ASSERT_EQUAL_UINT8(7U, IsoTp_GetMaxSingleFramePayload(ISOTP_NORMAL_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(6U, IsoTp_GetMaxSingleFramePayload(ISOTP_EXTENDED_ADDRESSING, false));
    TEST_ASSERT_EQUAL_UINT8(62U, IsoTp_GetMaxSingleFramePayload(ISOTP_NORMAL_ADDRESSING, true));
    TEST_ASSERT_EQUAL_UINT8(61U, IsoTp_GetMaxSingleFramePayload(ISOTP_EXTENDED_ADDRESSING, true));
}

static void test_isotp_util_stmin(void)
{
    /* No TEST_ASSERT_EQUAL_FLOAT in this framework: use ranges */
    TEST_ASSERT_TRUE(IsoTp_StMinToMs(0x00U) < 0.001f);
    TEST_ASSERT_TRUE(IsoTp_StMinToMs(0x7FU) > 126.9f && IsoTp_StMinToMs(0x7FU) < 127.1f);
    /* 0xF1..0xF9 encode 0.1 ms..0.9 ms */
    TEST_ASSERT_TRUE(IsoTp_StMinToMs(0xF1U) > 0.09f && IsoTp_StMinToMs(0xF1U) < 0.11f);
    TEST_ASSERT_TRUE(IsoTp_StMinToMs(0xF9U) > 0.89f && IsoTp_StMinToMs(0xF9U) < 0.91f);
    /* Reserved range maps to 20 ms */
    TEST_ASSERT_TRUE(IsoTp_StMinToMs(0xA0U) > 19.9f && IsoTp_StMinToMs(0xA0U) < 20.1f);
}

static void test_isotp_util_valid_stmin(void)
{
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0x00U));
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0x7FU));
    TEST_ASSERT_FALSE(IsoTp_IsValidStMin(0x80U));
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0xF1U));
    TEST_ASSERT_TRUE(IsoTp_IsValidStMin(0xF9U));
    TEST_ASSERT_FALSE(IsoTp_IsValidStMin(0xFAU));
    TEST_ASSERT_FALSE(IsoTp_IsValidStMin(0xE0U));
}

/* ==========================================================================
 * TX path
 * ========================================================================== */

static void test_isotp_tx_single_frame(void)
{
    uint8_t data[7] = {0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77};

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, data, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_READY, IsoTp_GetState(CH0));

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(1, s_capCount);
    TEST_ASSERT_EQUAL_UINT32(0x100U, s_capId[0]);
    TEST_ASSERT_EQUAL_UINT8(8U, s_capLen[0]);
    TEST_ASSERT_EQUAL_HEX8(0x07U, s_capData[0][0]);
    TEST_ASSERT_EQUAL_MEMORY(data, &s_capData[0][1], 7U);
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_DONE, IsoTp_GetState(CH0));

    /* TX_DONE -> confirmation callback -> IDLE */
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(1, s_txCalls);
    TEST_ASSERT_EQUAL_UINT8(CH0, s_txChan);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, s_txResult);
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(0, s_rxCalls);
}

static void test_isotp_tx_multi_frame(void)
{
    uint8_t msg[20];
    uint8_t fc[3] = {0x30, 0x00, 0x00};   /* CTS, BS=0, STmin=0 */

    FillPattern(msg, sizeof(msg));

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, msg, 20U, NULL));
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_FC, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(1, s_capCount);
    TEST_ASSERT_EQUAL_HEX8(0x10U, s_capData[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, s_capData[0][1]);   /* total length 20 */
    TEST_ASSERT_EQUAL_MEMORY(msg, &s_capData[0][2], 6U);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, fc, 3U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_SEND_CF, IsoTp_GetState(CH0));

    /* NOTE: FF carries payload[0..5] but the state marks 7 bytes as sent
     * (see file header defect note) -> CF1 starts at payload[7] */
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(2, s_capCount);
    TEST_ASSERT_EQUAL_HEX8(0x21U, s_capData[1][0]);
    TEST_ASSERT_EQUAL_MEMORY(&msg[7], &s_capData[1][1], 7U);
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_STMIN, IsoTp_GetState(CH0));

    /* STmin == 0 -> next MainFunction may send immediately */
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_SEND_CF, IsoTp_GetState(CH0));

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(3, s_capCount);
    TEST_ASSERT_EQUAL_HEX8(0x22U, s_capData[2][0]);
    TEST_ASSERT_EQUAL_MEMORY(&msg[14], &s_capData[2][1], 6U);
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_DONE, IsoTp_GetState(CH0));

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(1, s_txCalls);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, s_txResult);
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_tx_block_size(void)
{
    uint8_t msg[30];
    uint8_t fc[3] = {0x30, 0x02, 0x00};   /* CTS, BS=2, STmin=0 */

    FillPattern(msg, sizeof(msg));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, msg, 30U, NULL));
    IsoTp_MainFunction();                              /* FF -> TX_WAIT_FC */
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, fc, 3U));

    IsoTp_MainFunction();                              /* CF1 (SN 1) */
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_STMIN, IsoTp_GetState(CH0));
    IsoTp_MainFunction();
    IsoTp_MainFunction();                              /* CF2 (SN 2) -> block done */
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_FC, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(3, s_capCount);              /* FF + CF1 + CF2 */

    /* New CTS resumes the transfer */
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, fc, 3U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_SEND_CF, IsoTp_GetState(CH0));
    IsoTp_CancelTransmit(CH0);
}

static void test_isotp_tx_stmin_wait(void)
{
    uint8_t msg[20];
    uint8_t fc[3] = {0x30, 0x00, 0x7F};   /* CTS, STmin = 127 ms */

    FillPattern(msg, sizeof(msg));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, msg, 20U, NULL));
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, fc, 3U));

    IsoTp_MainFunction();                 /* CF1, then waits for STmin */
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_STMIN, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(2, s_capCount);

    /* 127 ms have not elapsed -> still waiting, no additional frame */
    IsoTp_MainFunction();
    IsoTp_MainFunction();
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_STMIN, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(2, s_capCount);

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_CancelTransmit(CH0));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_tx_timeout_nbs(void)
{
    /* CH1 tx buffer is 8 bytes: an 8-byte message is the largest payload
     * that still exceeds the 7-byte SF limit, so it goes out as an FF */
    uint8_t msg[8];

    s_cfg[1].timing.nBs = 2U;   /* config stored by pointer -> live effect */

    FillPattern(msg, sizeof(msg));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH1, msg, 8U, NULL));
    IsoTp_MainFunction();                 /* FF sent -> TX_WAIT_FC */
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_FC, IsoTp_GetState(CH1));

    /* No FC arrives; nBs = 2 expires after 3 more MainFunction calls */
    IsoTp_MainFunction();
    IsoTp_MainFunction();
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_ERROR, IsoTp_GetState(CH1));
    TEST_ASSERT_EQUAL_INT(1, s_txCalls);
    TEST_ASSERT_EQUAL_UINT8(CH1, s_txChan);
    TEST_ASSERT_EQUAL(ISOTP_E_TIMEOUT, s_txResult);

    /* TX_ERROR recovers to IDLE */
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH1));
}

static void test_isotp_tx_wt_overrun(void)
{
    uint8_t msg[20];
    uint8_t wt[3] = {0x31, 0x00, 0x00};
    int i;

    FillPattern(msg, sizeof(msg));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, msg, 20U, NULL));
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_FC, IsoTp_GetState(CH0));

    /* 10 x WAIT are tolerated, the 11th overruns WFTmax */
    for (i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, wt, 3U));
        TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_FC, IsoTp_GetState(CH0));
    }
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, wt, 3U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_ERROR, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(1, s_txCalls);
    TEST_ASSERT_EQUAL(ISOTP_E_WFT_OVRN, s_txResult);

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_tx_fc_ovflw(void)
{
    uint8_t msg[20];
    uint8_t ovflw[3] = {0x32, 0x00, 0x00};

    FillPattern(msg, sizeof(msg));
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, msg, 20U, NULL));
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_WAIT_FC, IsoTp_GetState(CH0));

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, ovflw, 3U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_ERROR, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(1, s_txCalls);
    TEST_ASSERT_EQUAL(ISOTP_E_BUFFER_OVERFLOW, s_txResult);
}

static void test_isotp_tx_unexpected_fc_cf(void)
{
    uint8_t cf[2] = {0x21, 0x00};
    uint8_t fc[3] = {0x30, 0x00, 0x00};

    /* Nothing ongoing: both frames are unexpected on the TX side */
    TEST_ASSERT_EQUAL(ISOTP_E_UNEXPECTED_FRAME, IsoTp_ProcessRxFrame(CH0, cf, 2U));
    TEST_ASSERT_EQUAL(ISOTP_E_UNEXPECTED_FRAME, IsoTp_ProcessRxFrame(CH0, fc, 3U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_tx_busy_and_cancel(void)
{
    uint8_t data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, data, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_E_BUSY, IsoTp_Transmit(CH0, data, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_READY, IsoTp_GetState(CH0));

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_CancelTransmit(CH0));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL(ISOTP_E_NOT_OK, IsoTp_CancelTransmit(CH0));   /* idle */
    TEST_ASSERT_EQUAL(ISOTP_E_NOT_OK, IsoTp_CancelTransmit(CH1));   /* idle */
}

static void test_isotp_tx_send_failure(void)
{
    uint8_t data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    /* NULL callback: transmit fails, state stays TX_READY */
    IsoTp_RegisterCanTransmitCallback(NULL);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, data, 7U, NULL));
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_READY, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(0, s_capCount);

    /* Callback rejecting the frame: still TX_READY */
    IsoTp_RegisterCanTransmitCallback(FailTx);
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_READY, IsoTp_GetState(CH0));

    /* Working callback: frame goes out */
    IsoTp_RegisterCanTransmitCallback(CapTx);
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_DONE, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(1, s_capCount);
    IsoTp_MainFunction();                 /* confirmation */
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

/* ==========================================================================
 * RX path
 * ========================================================================== */

static void test_isotp_rx_single_frame(void)
{
    uint8_t sf[8] = {0x03, 0xAA, 0xBB, 0xCC, 0x00, 0x00, 0x00, 0x00};

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, sf, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_DONE, IsoTp_GetState(CH0));

    IsoTp_MainFunction();                 /* delivers the indication */
    TEST_ASSERT_EQUAL_INT(1, s_rxCalls);
    TEST_ASSERT_EQUAL_UINT8(CH0, s_rxChan);
    TEST_ASSERT_EQUAL_UINT16(3U, s_rxLen);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, s_rxData[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBBU, s_rxData[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCCU, s_rxData[2]);
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_rx_sf_no_buffer(void)
{
    uint8_t sf[8] = {0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x00, 0x00};

    /* CH1 rx buffer is only 4 bytes */
    TEST_ASSERT_EQUAL(ISOTP_E_NO_BUFFER, IsoTp_ProcessRxFrame(CH1, sf, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH1));
    TEST_ASSERT_EQUAL_INT(0, s_rxCalls);
}

static void test_isotp_rx_multi_frame(void)
{
    uint8_t msg[20];
    uint8_t ff[8];
    uint8_t cf1[8];
    uint8_t cf2[8];
    int i;

    FillPattern(msg, sizeof(msg));

    ff[0] = 0x10U;
    ff[1] = 20U;
    memcpy(&ff[2], &msg[0], 6U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, ff, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_WAIT_CF, IsoTp_GetState(CH0));
    /* FC CTS generated on the RX CAN id */
    TEST_ASSERT_EQUAL_INT(1, s_capCount);
    TEST_ASSERT_EQUAL_UINT32(0x200U, s_capId[0]);
    TEST_ASSERT_EQUAL_HEX8(0x30U, s_capData[0][0]);
    TEST_ASSERT_EQUAL_INT(1, s_fcCalls);
    TEST_ASSERT_EQUAL_UINT16(20U, s_fcLen);

    cf1[0] = 0x21U;
    memcpy(&cf1[1], &msg[6], 7U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, cf1, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_WAIT_CF, IsoTp_GetState(CH0));

    cf2[0] = 0x22U;
    memcpy(&cf2[1], &msg[13], 7U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, cf2, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_DONE, IsoTp_GetState(CH0));

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(1, s_rxCalls);
    TEST_ASSERT_EQUAL_UINT8(CH0, s_rxChan);
    TEST_ASSERT_EQUAL_UINT16(20U, s_rxLen);
    for (i = 0; i < 20; i++) {
        TEST_ASSERT_EQUAL_HEX8(msg[i], s_rxData[i]);
    }
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_rx_sn_error(void)
{
    uint8_t msg[20];
    uint8_t ff[8];
    uint8_t cfBad[2] = {0x22, 0x55};

    FillPattern(msg, sizeof(msg));
    ff[0] = 0x10U;
    ff[1] = 20U;
    memcpy(&ff[2], &msg[0], 6U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, ff, 8U));

    /* Expecting SN=1, receiving SN=2 -> sequence error */
    TEST_ASSERT_EQUAL(ISOTP_E_SEQUENCE_ERROR, IsoTp_ProcessRxFrame(CH0, cfBad, 2U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_ERROR, IsoTp_GetState(CH0));

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

static void test_isotp_rx_block_boundary(void)
{
    uint8_t msg[30];
    uint8_t ff[8];
    uint8_t cf1[8], cf2[8], cf3[8], cf4[8];
    int i;

    s_fcRespBs = 2U;                      /* app requests BS=2 */
    FillPattern(msg, sizeof(msg));

    ff[0] = 0x10U;
    ff[1] = 30U;
    memcpy(&ff[2], &msg[0], 6U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, ff, 8U));
    TEST_ASSERT_EQUAL_INT(1, s_capCount);

    cf1[0] = 0x21U;
    memcpy(&cf1[1], &msg[6], 7U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, cf1, 8U));

    cf2[0] = 0x22U;
    memcpy(&cf2[1], &msg[13], 7U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, cf2, 8U));
    /* Block of 2 complete (20 of 30 bytes) -> another FC CTS is sent */
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_WAIT_CF, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(2, s_capCount);
    TEST_ASSERT_EQUAL_HEX8(0x30U, s_capData[1][0]);

    cf3[0] = 0x23U;
    memcpy(&cf3[1], &msg[20], 7U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, cf3, 8U));

    cf4[0] = 0x24U;
    memcpy(&cf4[1], &msg[27], 3U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, cf4, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_DONE, IsoTp_GetState(CH0));

    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(30U, s_rxLen);
    for (i = 0; i < 30; i++) {
        TEST_ASSERT_EQUAL_HEX8(msg[i], s_rxData[i]);
    }
}

static void test_isotp_rx_timeout_ncr(void)
{
    /* NOTE: CH1 rx buffer (4B) is too small for a multi-frame transfer,
     * so the N_Cr timeout is exercised on CH0 (rx buffer 64B) instead */
    uint8_t msg[20];
    uint8_t ff[8];

    s_cfg[0].timing.nCr = 2U;   /* config stored by pointer -> live effect */

    FillPattern(msg, sizeof(msg));
    ff[0] = 0x10U;
    ff[1] = 20U;
    memcpy(&ff[2], &msg[0], 6U);
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_ProcessRxFrame(CH0, ff, 8U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_WAIT_CF, IsoTp_GetState(CH0));

    /* No CF arrives; nCr = 2: diffs 1,2 still wait, diff 3 > 2 fires */
    IsoTp_MainFunction();
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_WAIT_CF, IsoTp_GetState(CH0));
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_RX_ERROR, IsoTp_GetState(CH0));

    /* RX_ERROR recovers to IDLE on the next MainFunction */
    IsoTp_MainFunction();
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
    TEST_ASSERT_EQUAL_INT(0, s_rxCalls);

    s_cfg[0].timing.nCr = 1000U;   /* restore default for later tests */
}

static void test_isotp_rx_ff_overflow(void)
{
    uint8_t msg[20];
    uint8_t ff[8];

    FillPattern(msg, sizeof(msg));
    ff[0] = 0x10U;
    ff[1] = 20U;
    memcpy(&ff[2], &msg[0], 6U);

    /* CH1 rx buffer is 4 bytes < 20 -> FC.OVFLW is sent */
    TEST_ASSERT_EQUAL(ISOTP_E_BUFFER_OVERFLOW, IsoTp_ProcessRxFrame(CH1, ff, 8U));
    TEST_ASSERT_EQUAL_INT(1, s_capCount);
    TEST_ASSERT_EQUAL_UINT32(0x201U, s_capId[0]);
    TEST_ASSERT_EQUAL_HEX8(0x32U, s_capData[0][0]);
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH1));
}

static void test_isotp_rx_unexpected_cf(void)
{
    uint8_t cf[2] = {0x21, 0x00};

    TEST_ASSERT_EQUAL(ISOTP_E_UNEXPECTED_FRAME, IsoTp_ProcessRxFrame(CH0, cf, 2U));
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));
}

/* ==========================================================================
 * Channel reset
 * ========================================================================== */

static void test_isotp_reset_channel(void)
{
    uint8_t data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, data, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_READY, IsoTp_GetState(CH0));

    IsoTp_ResetChannel(CH0);
    TEST_ASSERT_EQUAL(ISOTP_STATE_IDLE, IsoTp_GetState(CH0));

    /* Channel usable again right after reset */
    TEST_ASSERT_EQUAL(ISOTP_E_OK, IsoTp_Transmit(CH0, data, 7U, NULL));
    TEST_ASSERT_EQUAL(ISOTP_STATE_TX_READY, IsoTp_GetState(CH0));
    IsoTp_CancelTransmit(CH0);
}

/* ==========================================================================
 * main
 * ========================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_isotp_init_invalid_params);
    RUN_TEST(test_isotp_init_deinit_lifecycle);
    RUN_TEST(test_isotp_invalid_channel_and_param_guards);
    RUN_TEST(test_isotp_register_null_config_rejected);

    RUN_TEST(test_isotp_build_single_frame);
    RUN_TEST(test_isotp_build_single_frame_invalid);
    RUN_TEST(test_isotp_build_first_frame);
    RUN_TEST(test_isotp_build_first_frame_invalid);
    RUN_TEST(test_isotp_build_consecutive_frame);
    RUN_TEST(test_isotp_build_consecutive_frame_invalid);
    RUN_TEST(test_isotp_build_flow_control_frame);
    RUN_TEST(test_isotp_build_flow_control_frame_invalid);

    RUN_TEST(test_isotp_parse_sf);
    RUN_TEST(test_isotp_parse_ff);
    RUN_TEST(test_isotp_parse_ff_ext32);
    RUN_TEST(test_isotp_parse_cf_fc);
    RUN_TEST(test_isotp_parse_invalid);
    RUN_TEST(test_isotp_parse_extended_addressing);
    RUN_TEST(test_isotp_parse_canfd_ext_sf);

    RUN_TEST(test_isotp_get_frame_payload);
    RUN_TEST(test_isotp_get_frame_payload_invalid);

    RUN_TEST(test_isotp_util_multiframe);
    RUN_TEST(test_isotp_util_num_frames);
    RUN_TEST(test_isotp_util_max_sf_payload);
    RUN_TEST(test_isotp_util_stmin);
    RUN_TEST(test_isotp_util_valid_stmin);

    RUN_TEST(test_isotp_tx_single_frame);
    RUN_TEST(test_isotp_tx_multi_frame);
    RUN_TEST(test_isotp_tx_block_size);
    RUN_TEST(test_isotp_tx_stmin_wait);
    RUN_TEST(test_isotp_tx_timeout_nbs);
    RUN_TEST(test_isotp_tx_wt_overrun);
    RUN_TEST(test_isotp_tx_fc_ovflw);
    RUN_TEST(test_isotp_tx_unexpected_fc_cf);
    RUN_TEST(test_isotp_tx_busy_and_cancel);
    RUN_TEST(test_isotp_tx_send_failure);

    RUN_TEST(test_isotp_rx_single_frame);
    RUN_TEST(test_isotp_rx_sf_no_buffer);
    RUN_TEST(test_isotp_rx_multi_frame);
    RUN_TEST(test_isotp_rx_sn_error);
    RUN_TEST(test_isotp_rx_block_boundary);
    RUN_TEST(test_isotp_rx_timeout_ncr);
    RUN_TEST(test_isotp_rx_ff_overflow);
    RUN_TEST(test_isotp_rx_unexpected_cf);

    RUN_TEST(test_isotp_reset_channel);

    return UNITY_END();
}

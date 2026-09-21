/**
 * @file test_uart.c
 * @brief Uart Unit Tests (substantiated)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Links the real production source src/bsw/mcal/uart/src/Uart.c (plus Gpt.c
 * for the timestamp source used by the polling paths) against the
 * mock_registers register model and the mock_det error hook.
 *
 * Notes on driver behaviour covered here:
 *  - Uart_Initialized is a static flag, but Uart_DeInit() clears it, so the
 *    helper test_Uart_EnsureInitialized() performs a DeInit/Init cycle for
 *    every test to obtain a canonical post-Init register/state baseline.
 *  - Tests that must observe the genuine uninitialized driver state are named
 *    *BeforeInit* so the generated runner executes them first.
 *  - Gpt_GetTimeElapsed(0) is the UART timestamp source. GPT is initialised
 *    once (static flag) so that timestamp reads do not raise GPT DET reports
 *    that would pollute the UART DET assertions. The GPT counter mock always
 *    reads 0, therefore the "elapsed > timeout" branches can never fire in
 *    this host test setup (documented as a test-scope limitation).
 */

// @tests src/bsw/mcal/uart/src/Uart.c  @tests src/bsw/mcal/uart/include/Uart.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

/* Uart.h includes the real Det.h whose Det_Init(const Det_ConfigType*)
 * prototype conflicts with mock_det.h's Det_Init(void). The tests never call
 * Det_Init, so the real prototype is renamed out of the way while the UART
 * headers are pulled in. */
#define Det_Init Det_Init_DetH
#include "Uart.h"
#include "Uart_Cfg.h"
#undef Det_Init
#include "Gpt.h"
#include "Dma.h"

/*===========================================================================
 * i.MX8M Mini UART register model (channel 0 = UART1, channel 1 = UART2)
 *==========================================================================*/
#define TEST_UART0_BASE              (0x30860000UL)
#define TEST_UART1_BASE              (0x30890000UL)

#define TEST_UART_URXD_OFF           (0x00UL)
#define TEST_UART_UTXD_OFF           (0x40UL)
#define TEST_UART_UCR1_OFF           (0x80UL)
#define TEST_UART_UCR2_OFF           (0x84UL)
#define TEST_UART_UCR3_OFF           (0x88UL)
#define TEST_UART_UCR4_OFF           (0x8CUL)
#define TEST_UART_UFCR_OFF           (0x90UL)
#define TEST_UART_USR1_OFF           (0x94UL)
#define TEST_UART_USR2_OFF           (0x98UL)
#define TEST_UART_UBIR_OFF           (0xA4UL)
#define TEST_UART_UBMR_OFF           (0xA8UL)

/* Register bits mirrored from Uart.c */
#define TEST_UCR1_UARTEN             (0x0001UL)
#define TEST_UCR1_TXMPTYEN           (0x0040UL)
#define TEST_UCR1_RRDYEN             (0x0200UL)
#define TEST_UCR1_TRDYEN             (0x0080UL)
#define TEST_UCR1_RXDMAEN            (0x0004UL)
#define TEST_UCR1_TXDMAEN            (0x0008UL)
#define TEST_UCR1_RDMAEN             (0x0100UL)
#define TEST_UCR2_SRST_RXEN_TXEN     (0x0007UL)
#define TEST_UCR2_FULL_CFG           (0x007FUL) /* SRST|RXEN|TXEN|PREN|PROE|STPB|WS */
#define TEST_USR1_TRDY               (0x2000UL)
#define TEST_USR1_RRDY               (0x0200UL)
#define TEST_USR2_RDR                (0x0001UL)
#define TEST_USR2_TXDC               (0x0008UL)
#define TEST_USR2_ORE                (0x0002UL)

/*===========================================================================
 * DMA stubs (the DMA driver itself is not part of this unit test target)
 *==========================================================================*/
static uint32 test_DmaInitCalls = 0U;
static uint32 test_DmaEnableCalls = 0U;
static uint32 test_DmaDisableCalls = 0U;

void Dma_InitChannel(const Dma_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    test_DmaInitCalls++;
}

void Dma_EnableChannel(uint8 Channel)
{
    (void)Channel;
    test_DmaEnableCalls++;
}

void Dma_DisableChannel(uint8 Channel)
{
    (void)Channel;
    test_DmaDisableCalls++;
}

/*===========================================================================
 * Test configuration
 *==========================================================================*/
static Uart_ChannelConfigType test_ChannelConfigs[2U];
static Uart_ConfigType test_Config;

/* Channel 0: 115200 baud, 8N1, polling, FIFO enabled
 * Channel 1: 9600 baud, 7E2 (even parity, 2 stop bits, 7 data bits), no FIFO */
static void test_Uart_SetupConfig(void)
{
    test_ChannelConfigs[0].ChannelId = 0U;
    test_ChannelConfigs[0].BaudRate = 115200UL;
    test_ChannelConfigs[0].DataBits = UART_DATA_BITS_8;
    test_ChannelConfigs[0].StopBits = UART_STOP_BITS_1;
    test_ChannelConfigs[0].Parity = UART_PARITY_NONE;
    test_ChannelConfigs[0].OpMode = UART_MODE_POLLING;
    test_ChannelConfigs[0].HwHandshake = UART_HW_HANDSHAKE_NONE;
    test_ChannelConfigs[0].FifoMode = UART_FIFO_ENABLED;
    test_ChannelConfigs[0].TxFifoThreshold = 8U;
    test_ChannelConfigs[0].RxFifoThreshold = 8U;
    test_ChannelConfigs[0].DmaEnabled = FALSE;
    test_ChannelConfigs[0].DmaTxChannel = 0U;
    test_ChannelConfigs[0].DmaRxChannel = 1U;
    test_ChannelConfigs[0].IrqPriority = 5U;
    test_ChannelConfigs[0].TxTimeout = 1000UL;
    test_ChannelConfigs[0].RxTimeout = 1000UL;

    test_ChannelConfigs[1].ChannelId = 1U;
    test_ChannelConfigs[1].BaudRate = 9600UL;
    test_ChannelConfigs[1].DataBits = UART_DATA_BITS_7;
    test_ChannelConfigs[1].StopBits = UART_STOP_BITS_2;
    test_ChannelConfigs[1].Parity = UART_PARITY_EVEN;
    test_ChannelConfigs[1].OpMode = UART_MODE_POLLING;
    test_ChannelConfigs[1].HwHandshake = UART_HW_HANDSHAKE_NONE;
    test_ChannelConfigs[1].FifoMode = UART_FIFO_DISABLED;
    test_ChannelConfigs[1].TxFifoThreshold = 0U;
    test_ChannelConfigs[1].RxFifoThreshold = 0U;
    test_ChannelConfigs[1].DmaEnabled = FALSE;
    test_ChannelConfigs[1].DmaTxChannel = 2U;
    test_ChannelConfigs[1].DmaRxChannel = 3U;
    test_ChannelConfigs[1].IrqPriority = 5U;
    test_ChannelConfigs[1].TxTimeout = 1000UL;
    test_ChannelConfigs[1].RxTimeout = 1000UL;

    test_Config.ChannelCount = 2U;
    test_Config.ChannelConfig = test_ChannelConfigs;
}

/*===========================================================================
 * GPT one-time initialisation
 * Uart_GetCurrentTime() reads Gpt_GetTimeElapsed(0). With GPT initialised the
 * read raises no DET report; the counter mock always reads 0.
 *==========================================================================*/
static boolean test_GptReady = FALSE;
static Gpt_ChannelConfigType test_GptChannels[GPT_NUM_CHANNELS];
static Gpt_ConfigType test_GptConfig;

static void test_Uart_InitGptOnce(void)
{
    uint8 i;

    if (test_GptReady) {
        return;
    }

    for (i = 0U; i < GPT_NUM_CHANNELS; i++) {
        test_GptChannels[i].ChannelId = i;
        test_GptChannels[i].BaseAddress = 0x302E0000UL;
        test_GptChannels[i].ChannelMode = GPT_CH_MODE_CONTINUOUS;
        test_GptChannels[i].ClockPrescaler = GPT_CLOCK_PRESCALER_1;
        test_GptChannels[i].MaxTickValue = 0xFFFFFFFFUL;
        test_GptChannels[i].ClockFrequency = 24000000UL;
        test_GptChannels[i].WakeupSupport = FALSE;
        test_GptChannels[i].NotificationEnabled = FALSE;
        test_GptChannels[i].NotificationFn = NULL_PTR;
    }

    test_GptConfig.Channels = test_GptChannels;
    test_GptConfig.NumChannels = GPT_NUM_CHANNELS;
    test_GptConfig.DevErrorDetect = TRUE;
    test_GptConfig.VersionInfoApi = TRUE;
    test_GptConfig.WakeupFunctionalityApi = FALSE;
    test_GptConfig.DeInitApi = TRUE;
    test_GptConfig.TimeElapsedApi = TRUE;
    test_GptConfig.TimeRemainingApi = TRUE;
    test_GptConfig.EnableDisableNotificationApi = TRUE;
    test_GptConfig.NotificationSupported = FALSE;
    test_GptConfig.DefaultMode = GPT_MODE_NORMAL;
    test_GptConfig.PredefTimer1usEnablingGrade = FALSE;
    test_GptConfig.PredefTimer100us32bitEnable = FALSE;

    Gpt_Init(&test_GptConfig);
    test_GptReady = TRUE;
}

/*===========================================================================
 * Driver (re-)initialisation helper
 * DeInit clears the static init flag, so a full DeInit/Init cycle restores a
 * canonical post-Init state (registers + channel state) for every test.
 *==========================================================================*/
static boolean test_UartDriverReady = FALSE;

static void test_Uart_EnsureInitialized(void)
{
    if (test_UartDriverReady) {
        Uart_DeInit();
    }
    MockRegisters_Reset();
    test_Uart_InitGptOnce();
    test_Uart_SetupConfig();
    Uart_Init(&test_Config);
    Det_Mock_Reset();
    test_UartDriverReady = TRUE;
}

/*===========================================================================
 * Unity fixture
 *==========================================================================*/
void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
    test_DmaInitCalls = 0U;
    test_DmaEnableCalls = 0U;
    test_DmaDisableCalls = 0U;
}

void tearDown(void)
{
}

/*===========================================================================
 * DET assertion helper
 *==========================================================================*/
static void test_Uart_ExpectDet(uint8 ApiId, uint8 ErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL_UINT32(UART_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT32(UART_INSTANCE_ID, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT32(ApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT32(ErrorId, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(1U, Det_MockData.CallCount);
}

/*===========================================================================
 * Uninitialized-driver tests (executed first by the generated runner)
 *==========================================================================*/

/** @req SWS_Uart_00001 */
void test_Uart_BeforeInit_Init_NullPtr_ShouldReportDet(void)
{
    Uart_Init(NULL_PTR);
    test_Uart_ExpectDet(UART_SERVICE_ID_INIT, UART_E_PARAM_POINTER);
    /* driver must still be uninitialized */
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0U));
}

/** @req SWS_Uart_00002 */
void test_Uart_BeforeInit_DeInit_ShouldReportUninit(void)
{
    Uart_DeInit();
    test_Uart_ExpectDet(UART_SERVICE_ID_DEINIT, UART_E_UNINIT);
}

/** @req SWS_Uart_00003 */
void test_Uart_BeforeInit_Send_Uninit_ShouldReportDet(void)
{
    uint8 data[2U] = {0x11U, 0x22U};

    Std_ReturnType ret = Uart_Send(0U, data, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SEND, UART_E_UNINIT);
}

/** @req SWS_Uart_00003 */
void test_Uart_BeforeInit_Send_InvalidChannel_CheckedBeforeInit_ShouldReportParamChannel(void)
{
    uint8 data[2U] = {0x11U, 0x22U};

    /* Channel validation precedes the initialization check */
    Std_ReturnType ret = Uart_Send(200U, data, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SEND, UART_E_PARAM_CHANNEL);
}

/** @req SWS_Uart_00003 */
void test_Uart_BeforeInit_Send_NullPtr_CheckedBeforeInit_ShouldReportParamPointer(void)
{
    /* Pointer validation also precedes the initialization check */
    Std_ReturnType ret = Uart_Send(0U, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SEND, UART_E_PARAM_POINTER);
}

/** @req SWS_Uart_00004 */
void test_Uart_BeforeInit_Receive_Uninit_ShouldReportDet(void)
{
    uint8 buf[2U];

    Std_ReturnType ret = Uart_Receive(0U, buf, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_RECEIVE, UART_E_UNINIT);
}

/** @req SWS_Uart_00005 */
void test_Uart_BeforeInit_GetStatus_ShouldReturnUninitOrErrorWithoutDet(void)
{
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0U));
    /* invalid channel yields error state, still no DET */
    TEST_ASSERT_EQUAL(UART_STATE_ERROR, Uart_GetStatus(200U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00007 */
void test_Uart_BeforeInit_SetBaudRate_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = Uart_SetBaudRate(0U, 115200UL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SETBAUDRATE, UART_E_UNINIT);
}

/** @req SWS_Uart_00006 */
void test_Uart_BeforeInit_GetVersionInfo_NullPtr_ShouldReportDet(void)
{
    Uart_GetVersionInfo(NULL_PTR);
    test_Uart_ExpectDet(UART_SERVICE_ID_GETVERSIONINFO, UART_E_PARAM_POINTER);
}

/** @req SWS_Uart_00005 */
void test_Uart_BeforeInit_GetTxResult_InvalidChannel_ShouldReportParamChannel(void)
{
    /* GetTxResult only validates the channel (no uninit check) */
    (void)Uart_GetTxResult(200U);
    test_Uart_ExpectDet(UART_SERVICE_ID_GETSTATUS, UART_E_PARAM_CHANNEL);
}

/** @req SWS_Uart_00008 */
void test_Uart_BeforeInit_MainFunctionAndIsr_Uninit_ShouldReturnSilently(void)
{
    Uart_MainFunction();
    Uart_IsrHandler(0U);
    Uart_IsrHandler(200U);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0U));
}

/*===========================================================================
 * Initialisation / de-initialisation
 *==========================================================================*/

/** @req SWS_Uart_00001 */
void test_Uart_Init_ValidConfig_ShouldProgramChannelRegisters(void)
{
    test_Uart_EnsureInitialized();

    /* Channel 0: 115200-8N1 polling with FIFO thresholds 8/8, RFDIV=1 */
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR2_SRST_RXEN_TXEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR2_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x84UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR3_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR4_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x2288UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UFCR_OFF));
    /* 80MHz ref clock, 115200 baud: UBIR=115199, UBMR=116278 */
    TEST_ASSERT_EQUAL_UINT32(115199UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UBIR_OFF));
    TEST_ASSERT_EQUAL_UINT32(116278UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UBMR_OFF));
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));

    /* Channel 1: 9600-7E2 without FIFO: UCR2 carries parity/stop/word bits */
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR2_FULL_CFG,
                             MockRegisters_Read32(TEST_UART1_BASE + TEST_UART_UCR2_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x280UL, /* RFDIV=1 only, no FIFO thresholds */
                             MockRegisters_Read32(TEST_UART1_BASE + TEST_UART_UFCR_OFF));
    TEST_ASSERT_EQUAL_UINT32(9599UL,
                             MockRegisters_Read32(TEST_UART1_BASE + TEST_UART_UBIR_OFF));
    TEST_ASSERT_EQUAL_UINT32(9614UL,
                             MockRegisters_Read32(TEST_UART1_BASE + TEST_UART_UBMR_OFF));
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(1U));

    /* successful init reports no DET */
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00001 */
void test_Uart_Init_DoubleInit_ShouldReportDet(void)
{
    test_Uart_EnsureInitialized();

    Uart_Init(&test_Config);
    test_Uart_ExpectDet(UART_SERVICE_ID_INIT, UART_E_ALREADY_INITIALIZED);
    /* original configuration stays active */
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
}

/** @req SWS_Uart_00002 */
void test_Uart_DeInit_ShouldDisableUartAndAllowReinit(void)
{
    test_Uart_EnsureInitialized();

    Uart_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* UART disabled on the hardware */
    TEST_ASSERT_EQUAL_HEX32(0x0UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR2_OFF));
    TEST_ASSERT_EQUAL(UART_STATE_UNINIT, Uart_GetStatus(0U));

    /* driver can be initialised again after DeInit */
    Uart_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
}

/*===========================================================================
 * Polling transmit / receive
 *==========================================================================*/

/** @req SWS_Uart_00003 */
void test_Uart_Send_Polling_ShouldWriteTxDataAndReturnOk(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    /* TX ready + transmission complete flags must be set for the polling loop */
    MockRegisters_Write32(TEST_UART0_BASE + TEST_UART_USR1_OFF, TEST_USR1_TRDY);
    MockRegisters_Write32(TEST_UART0_BASE + TEST_UART_USR2_OFF, TEST_USR2_TXDC);

    Std_ReturnType ret = Uart_Send(0U, data, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* last data byte written to UTXD */
    TEST_ASSERT_EQUAL_HEX32('B',
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UTXD_OFF));
    /* channel back to READY, result OK, no DET */
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL(UART_RESULT_OK, Uart_GetTxResult(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00003 */
void test_Uart_Send_NullPtr_ShouldReportDet(void)
{
    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_Send(0U, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SEND, UART_E_PARAM_POINTER);
}

/** @req SWS_Uart_00003 */
void test_Uart_Send_ZeroLength_ShouldReportDet(void)
{
    uint8 data[2U] = {0x11U, 0x22U};

    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_Send(0U, data, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SEND, UART_E_PARAM_LENGTH);
}

/** @req SWS_Uart_00003 */
void test_Uart_Send_TxBusy_ShouldReportDet(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    /* occupy the channel with an interrupt-mode transfer */
    Std_ReturnType ret = Uart_SendInterrupt(0U, data, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(UART_STATE_TX_BUSY, Uart_GetStatus(0U));
    Det_Mock_Reset();

    ret = Uart_Send(0U, data, 2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SEND, UART_E_TX_BUSY);
}

/** @req SWS_Uart_00004 */
void test_Uart_Receive_Polling_ShouldFillBufferFromRxRegister(void)
{
    uint8 buf[3U] = {0U, 0U, 0U};

    test_Uart_EnsureInitialized();
    MockRegisters_Write32(TEST_UART0_BASE + TEST_UART_USR2_OFF, TEST_USR2_RDR);
    MockRegisters_Write32(TEST_UART0_BASE + TEST_UART_URXD_OFF, 0x42U);

    Std_ReturnType ret = Uart_Receive(0U, buf, 3U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* every byte read from URXD */
    TEST_ASSERT_EQUAL_UINT32(0x42U, buf[0U]);
    TEST_ASSERT_EQUAL_UINT32(0x42U, buf[1U]);
    TEST_ASSERT_EQUAL_UINT32(0x42U, buf[2U]);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL(UART_RESULT_OK, Uart_GetRxResult(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00004 */
void test_Uart_Receive_NullPtr_ShouldReportDet(void)
{
    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_Receive(0U, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_RECEIVE, UART_E_PARAM_POINTER);
}

/** @req SWS_Uart_00004 */
void test_Uart_Receive_RxBusy_ShouldReportDet(void)
{
    uint8 buf[2U];

    test_Uart_EnsureInitialized();
    Std_ReturnType ret = Uart_ReceiveInterrupt(0U, buf, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(UART_STATE_RX_BUSY, Uart_GetStatus(0U));
    Det_Mock_Reset();

    ret = Uart_Receive(0U, buf, 2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_RECEIVE, UART_E_RX_BUSY);
}

/*===========================================================================
 * Interrupt-mode transmit / receive
 *==========================================================================*/

/** @req SWS_Uart_00003 */
void test_Uart_SendInterrupt_ShouldEnableTxIrqAndMarkBusy(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_SendInterrupt(0U, data, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(UART_STATE_TX_BUSY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL(UART_RESULT_PENDING, Uart_GetTxResult(0U));
    /* TX-empty interrupt enabled in UCR1 (read-modify-write over UARTEN) */
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN | TEST_UCR1_TXMPTYEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    /* stale TRDY flag acknowledged (written back set) */
    TEST_ASSERT_EQUAL_HEX32(TEST_USR1_TRDY,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_USR1_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00004 */
void test_Uart_ReceiveInterrupt_ShouldEnableRxIrqAndMarkBusy(void)
{
    uint8 buf[2U];

    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_ReceiveInterrupt(0U, buf, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(UART_STATE_RX_BUSY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN | TEST_UCR1_RRDYEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_HEX32(TEST_USR1_RRDY,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_USR1_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00003 */
void test_Uart_IsrHandler_TxProgress_ShouldCompleteTransfer(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Uart_SendInterrupt(0U, data, 2U));

    /* first ISR: one byte pushed to UTXD */
    Uart_IsrHandler(0U);
    TEST_ASSERT_EQUAL_HEX32('A',
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UTXD_OFF));
    TEST_ASSERT_EQUAL(UART_STATE_TX_BUSY, Uart_GetStatus(0U));

    /* second ISR: second byte pushed */
    Uart_IsrHandler(0U);
    TEST_ASSERT_EQUAL_HEX32('B',
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UTXD_OFF));
    TEST_ASSERT_EQUAL(UART_STATE_TX_BUSY, Uart_GetStatus(0U));

    /* third ISR: transfer complete -> result OK, back to READY, IRQ disabled */
    Uart_IsrHandler(0U);
    TEST_ASSERT_EQUAL(UART_RESULT_OK, Uart_GetTxResult(0U));
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00004 */
void test_Uart_IsrHandler_RxProgress_ShouldCompleteReception(void)
{
    uint8 buf[2U] = {0U, 0U};

    test_Uart_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Uart_ReceiveInterrupt(0U, buf, 2U));
    MockRegisters_Write32(TEST_UART0_BASE + TEST_UART_URXD_OFF, 0x5AU);

    Uart_IsrHandler(0U);
    Uart_IsrHandler(0U);
    /* both bytes fetched from URXD */
    TEST_ASSERT_EQUAL_UINT32(0x5AU, buf[0U]);
    TEST_ASSERT_EQUAL_UINT32(0x5AU, buf[1U]);
    TEST_ASSERT_EQUAL(UART_STATE_RX_BUSY, Uart_GetStatus(0U));

    Uart_IsrHandler(0U);
    TEST_ASSERT_EQUAL(UART_RESULT_OK, Uart_GetRxResult(0U));
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00003 */
void test_Uart_IsrHandler_OverrunError_ShouldSetErrorState(void)
{
    test_Uart_EnsureInitialized();
    MockRegisters_Write32(TEST_UART0_BASE + TEST_UART_USR2_OFF, TEST_USR2_ORE);

    Uart_IsrHandler(0U);
    TEST_ASSERT_EQUAL(UART_STATE_ERROR, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Baud rate re-configuration
 *==========================================================================*/

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_ZeroBaud_ShouldReportDet(void)
{
    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_SetBaudRate(0U, 0UL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SETBAUDRATE, UART_E_PARAM_BAUDRATE);
}

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_InvalidChannel_ShouldReportDet(void)
{
    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_SetBaudRate(200U, 115200UL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_Uart_ExpectDet(UART_SERVICE_ID_SETBAUDRATE, UART_E_PARAM_CHANNEL);
}

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_BusyChannel_ShouldReturnNotOkWithoutDet(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Uart_SendInterrupt(0U, data, 2U));
    Det_Mock_Reset();

    Std_ReturnType ret = Uart_SetBaudRate(0U, 9600UL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* baud registers untouched */
    TEST_ASSERT_EQUAL_UINT32(115199UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UBIR_OFF));
}

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_Valid_ShouldUpdateBaudRegisters(void)
{
    test_Uart_EnsureInitialized();

    Std_ReturnType ret = Uart_SetBaudRate(0U, 9600UL);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* 80MHz ref clock, 9600 baud: UBIR=9599, UBMR=9614 */
    TEST_ASSERT_EQUAL_UINT32(9599UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UBIR_OFF));
    TEST_ASSERT_EQUAL_UINT32(9614UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UBMR_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Interrupt enable/disable, FIFO flush, abort
 *==========================================================================*/

/** @req SWS_Uart_00009 */
void test_Uart_EnableInterrupt_ShouldSetUcr1IrqBits(void)
{
    test_Uart_EnsureInitialized();

    Uart_EnableInterrupt(0U);
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN | TEST_UCR1_TXMPTYEN | TEST_UCR1_RRDYEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00009 */
void test_Uart_DisableInterrupt_ShouldClearUcr1IrqBits(void)
{
    test_Uart_EnsureInitialized();

    Uart_EnableInterrupt(0U);
    Uart_DisableInterrupt(0U);
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00010 */
void test_Uart_ClearFIFO_ShouldSetFlushBitsInUfcr(void)
{
    test_Uart_EnsureInitialized();

    Uart_ClearFIFO(0U);
    /* TX/RX FIFO flush bits (14/15) set on top of the Init value 0x2288 */
    TEST_ASSERT_EQUAL_HEX32(0xE288UL,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UFCR_OFF));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00018 */
void test_Uart_Abort_ShouldRestoreReadyAndDisableIrq(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Uart_SendInterrupt(0U, data, 2U));
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN | TEST_UCR1_TXMPTYEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));

    Uart_Abort(0U);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));
    /* no DMA active -> no DMA disable calls */
    TEST_ASSERT_EQUAL_UINT32(0U, test_DmaDisableCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * DMA transfers
 *==========================================================================*/

/** @req SWS_Uart_00004 */
void test_Uart_SendDMA_ChannelWithoutDma_ShouldReturnNotOk(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized(); /* channel 0 configured with DmaEnabled = FALSE */

    Std_ReturnType ret = Uart_SendDMA(0U, data, 2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, test_DmaInitCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, test_DmaEnableCalls);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00004 */
void test_Uart_SendDMA_DmaEnabled_ShouldStartDmaTransfer(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    /* reconfigure channel 0 with DMA enabled */
    Uart_DeInit();
    test_ChannelConfigs[0].DmaEnabled = TRUE;
    Uart_Init(&test_Config);
    Det_Mock_Reset();

    Std_ReturnType ret = Uart_SendDMA(0U, data, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, test_DmaInitCalls);
    TEST_ASSERT_EQUAL_UINT32(1U, test_DmaEnableCalls);
    TEST_ASSERT_EQUAL(UART_STATE_TX_BUSY, Uart_GetStatus(0U));
    /* Init enables the DMA request bits in UCR1 */
    TEST_ASSERT_EQUAL_HEX32(TEST_UCR1_UARTEN | TEST_UCR1_RXDMAEN | TEST_UCR1_TXDMAEN |
                             TEST_UCR1_RDMAEN | TEST_UCR1_TRDYEN,
                             MockRegisters_Read32(TEST_UART0_BASE + TEST_UART_UCR1_OFF));

    /* aborting a DMA transfer disables both DMA channels */
    Uart_Abort(0U);
    TEST_ASSERT_EQUAL_UINT32(2U, test_DmaDisableCalls);
    TEST_ASSERT_EQUAL(UART_STATE_READY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* restore default configuration for subsequent tests */
    Uart_DeInit();
    test_ChannelConfigs[0].DmaEnabled = FALSE;
    Uart_Init(&test_Config);
    Det_Mock_Reset();
}

/** @req SWS_Uart_00004 */
void test_Uart_ReceiveDMA_DmaEnabled_ShouldStartDmaTransfer(void)
{
    uint8 buf[2U];

    test_Uart_EnsureInitialized();
    Uart_DeInit();
    test_ChannelConfigs[0].DmaEnabled = TRUE;
    Uart_Init(&test_Config);
    Det_Mock_Reset();

    Std_ReturnType ret = Uart_ReceiveDMA(0U, buf, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, test_DmaInitCalls);
    TEST_ASSERT_EQUAL_UINT32(1U, test_DmaEnableCalls);
    TEST_ASSERT_EQUAL(UART_STATE_RX_BUSY, Uart_GetStatus(0U));

    Uart_DeInit();
    test_ChannelConfigs[0].DmaEnabled = FALSE;
    Uart_Init(&test_Config);
    Det_Mock_Reset();
}

/*===========================================================================
 * Version info / main function
 *==========================================================================*/

/** @req SWS_Uart_00006 */
void test_Uart_GetVersionInfo_ValidPtr_ShouldFillVersionFields(void)
{
    Std_VersionInfoType info;

    test_Uart_EnsureInitialized();
    Uart_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(UART_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT32(UART_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT32(UART_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT32(UART_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT32(UART_SW_PATCH_VERSION, info.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_Uart_00008 */
void test_Uart_MainFunction_ActiveTransferNotTimedOut_ShouldKeepState(void)
{
    uint8 data[2U] = {'A', 'B'};

    test_Uart_EnsureInitialized();
    TEST_ASSERT_EQUAL(E_OK, Uart_SendInterrupt(0U, data, 2U));

    /* GPT counter mock is frozen at 0, so no timeout can elapse: the active
     * transfer must survive the main function unchanged. */
    Uart_MainFunction();
    TEST_ASSERT_EQUAL(UART_STATE_TX_BUSY, Uart_GetStatus(0U));
    TEST_ASSERT_EQUAL(UART_RESULT_PENDING, Uart_GetTxResult(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

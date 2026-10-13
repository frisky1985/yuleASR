/**
 * @file test_spi.c
 * @brief Spi (SPI Driver) Unit Tests — Substantiated
 * @req SWS_Spi
 *
 * Substantiation: register-level verification of channel initialization
 * (CONREG/CONFIGREG/DMAREG/INTREG/PERIODREG via mock_registers on the
 * i.MX8M Mini ECSPI bases), synchronous and asynchronous transfer flows
 * (FIFO prefill, interrupt enables, exchange start), DMA configuration
 * delegation, ISR completion, MainFunction timeout handling, and DET
 * parameter validation. Gpt/Dma externals referenced by Spi.c are
 * provided as instrumented stubs.
 */

// @tests src/bsw/mcal/spi/src/Spi.c  @tests src/bsw/mcal/spi/include/Spi.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Spi.h"
#include "Spi_Cfg.h"

/* i.MX8M Mini ECSPI register bases (must match Spi.c) */
#define SPI_TEST_ECSPI1_BASE    (0x30820000UL)
#define SPI_TEST_ECSPI2_BASE    (0x30830000UL)
#define SPI_TEST_ECSPI3_BASE    (0x30840000UL)

/* Register offsets (Spi.c internal defines, mirrored here with _OFF suffix) */
#define ECSPI_RXDATA_OFF        (0x00U)
#define ECSPI_TXDATA_OFF        (0x04U)
#define ECSPI_CONREG_OFF        (0x08U)
#define ECSPI_CONFIGREG_OFF     (0x0CU)
#define ECSPI_INTREG_OFF        (0x10U)
#define ECSPI_DMAREG_OFF        (0x14U)
#define ECSPI_STATREG_OFF       (0x18U)
#define ECSPI_PERIODREG_OFF     (0x1CU)

/* CONREG bits */
#define CONREG_EN               (0x00000001UL)
#define CONREG_MODE             (0x00000002UL)
#define CONREG_XCH              (0x00000004UL)
#define CONREG_CH_SHIFT         (18U)

/* STATREG bits */
#define STATREG_RR              (0x00000008UL)
#define STATREG_TE              (0x00000020UL)
#define STATREG_TF              (0x00000040UL)

/* INTREG bits */
#define INTREG_TEEN             (0x00000020UL)
#define INTREG_TCEN             (0x00000080UL)
#define INTREG_RREN             (0x00000100UL)

/* DMAREG bits */
#define DMAREG_RXDEN            (0x00000001UL)
#define DMAREG_TXDEN            (0x00000002UL)

/* ---- Stubs for external symbols referenced by Spi.c ----
 * Gpt_GetTimeElapsed is controllable: in static mode it returns a constant
 * (elapsed time 0 -> polling loops never time out), in auto-advance mode it
 * steps 200 ms per call so the 1000 ms transfer timeout can be exercised. */
static uint32 gpt_timeMs = 0U;
static boolean gpt_autoAdvance = FALSE;

uint32 Gpt_GetTimeElapsed(uint8 Channel)
{
    (void)Channel;
    if (gpt_autoAdvance != FALSE) {
        gpt_timeMs += 200U;
    }
    return gpt_timeMs;
}

/* Instrumented Dma stubs */
static uint32 dma_configTxCalls = 0U;
static uint32 dma_configRxCalls = 0U;
static uint32 dma_enableCalls = 0U;
static uint32 dma_disableCalls = 0U;
static uint8 dma_lastTxChannel = 0U;
static uint8 dma_lastRxChannel = 0U;
static uint32 dma_lastTxDst = 0U;
static uint32 dma_lastTxLen = 0U;
static uint32 dma_lastRxSrc = 0U;
static uint32 dma_lastRxLen = 0U;

void Dma_ConfigTx(uint8 Channel, uint32 SrcAddr, uint32 DstAddr, uint32 Length)
{
    (void)SrcAddr; /* host pointer truncated to uint32 by the driver; not asserted */
    dma_configTxCalls++;
    dma_lastTxChannel = Channel;
    dma_lastTxDst = DstAddr;
    dma_lastTxLen = Length;
}

void Dma_ConfigRx(uint8 Channel, uint32 SrcAddr, uint32 DstAddr, uint32 Length)
{
    (void)DstAddr;
    dma_configRxCalls++;
    dma_lastRxChannel = Channel;
    dma_lastRxSrc = SrcAddr;
    dma_lastRxLen = Length;
}

void Dma_EnableChannel(uint8 Channel)
{
    (void)Channel;
    dma_enableCalls++;
}

void Dma_DisableChannel(uint8 Channel)
{
    (void)Channel;
    dma_disableCalls++;
}

/* ---- Test configuration ---- */
static Spi_ChannelConfigType testChannels[2];
static Spi_ExternalDeviceType testDevices[2];
static Spi_ConfigType testConfig;

static void test_Spi_SetupConfig(void)
{
    /* Channel 0: ECSPI1, interrupt-driven, 8-bit frames, clock mode 2, 1 Mbit/s */
    testChannels[0].ChannelId = 0U;
    testChannels[0].DataMode = SPI_DATA_MODE_8BIT;
    testChannels[0].ClockMode = SPI_CLOCK_MODE_2;
    testChannels[0].BaudRate = 1000000U;
    testChannels[0].LsbFirst = FALSE;
    testChannels[0].DmaEnabled = FALSE;
    testChannels[0].DmaTxChannel = 0U;
    testChannels[0].DmaRxChannel = 0U;
    testChannels[0].InterruptEnabled = TRUE;
    testChannels[0].InterruptPriority = 4U;

    /* Channel 1: ECSPI2, DMA-driven, 16-bit frames, clock mode 0, 8 Mbit/s */
    testChannels[1].ChannelId = 1U;
    testChannels[1].DataMode = SPI_DATA_MODE_16BIT;
    testChannels[1].ClockMode = SPI_CLOCK_MODE_0;
    testChannels[1].BaudRate = 8000000U;
    testChannels[1].LsbFirst = FALSE;
    testChannels[1].DmaEnabled = TRUE;
    testChannels[1].DmaTxChannel = 8U;
    testChannels[1].DmaRxChannel = 12U;
    testChannels[1].InterruptEnabled = FALSE;
    testChannels[1].InterruptPriority = 0U;

    testDevices[0].DeviceId = 0U;
    testDevices[0].ChannelId = 0U;
    testDevices[0].ChipSelectPin = 10U;
    testDevices[0].ChipSelectActiveLow = TRUE;
    testDevices[0].ChipSelectDelay = 0U;
    testDevices[0].BaudRate = 1000000U;

    testDevices[1].DeviceId = 1U;
    testDevices[1].ChannelId = 1U;
    testDevices[1].ChipSelectPin = 20U;
    testDevices[1].ChipSelectActiveLow = TRUE;
    testDevices[1].ChipSelectDelay = 0U;
    testDevices[1].BaudRate = 8000000U;

    testConfig.ChannelCount = 2U;
    testConfig.ChannelConfig = testChannels;
    testConfig.DeviceConfig = testDevices;
    testConfig.DeviceCount = 2U;
}

/* Spi.c keeps static state, but Spi_DeInit resets it and Spi_Init may be
 * called repeatedly (no double-init DET), so each test initializes fresh. */
static void test_Spi_EnsureInit(void)
{
    test_Spi_SetupConfig();
    Spi_Init(&testConfig);
    Det_Mock_Reset();
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
    gpt_timeMs = 0U;
    gpt_autoAdvance = FALSE;
    dma_configTxCalls = 0U;
    dma_configRxCalls = 0U;
    dma_enableCalls = 0U;
    dma_disableCalls = 0U;
    dma_lastTxChannel = 0U;
    dma_lastRxChannel = 0U;
    dma_lastTxDst = 0U;
    dma_lastTxLen = 0U;
    dma_lastRxSrc = 0U;
    dma_lastRxLen = 0U;
}

void tearDown(void) {}

/* --- Init / state before initialization --- */

/** @req SWS_Spi_00001 */
void test_Spi_Init_NullPtr_ShouldReportDet(void) {
    Spi_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(SPI_SERVICE_ID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(SPI_E_PARAM_POINTER, Det_MockData.ErrorId);
    /* Failed init must leave the driver uninitialized */
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/** @req SWS_Spi_00005 */
void test_Spi_GetStatus_BeforeInit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/** @req SWS_Spi_00006 */
void test_Spi_GetJobResult_BeforeInit_ShouldReturnFailed(void) {
    TEST_ASSERT_EQUAL(SPI_JOB_FAILED, Spi_GetJobResult());
}

/** @req SWS_Spi_00002 */
void test_Spi_DeInit_BeforeInit_ShouldReportUninitDet(void) {
    Std_ReturnType ret = Spi_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(SPI_SERVICE_ID_DEINIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(SPI_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_BeforeInit_ShouldReportUninitDet(void) {
    uint8 tx[1] = {0x00U};
    Std_ReturnType ret = Spi_SyncTransmit(0U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(SPI_SERVICE_ID_SYNCTRANSMIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(SPI_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Spi_00004 */
void test_Spi_AsyncTransmit_BeforeInit_ShouldReportUninitDet(void) {
    uint8 tx[1] = {0x00U};
    Std_ReturnType ret = Spi_AsyncTransmit(0U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(SPI_SERVICE_ID_ASYNC_TRANSMIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(SPI_E_UNINIT, Det_MockData.ErrorId);
}

/* --- Init register programming --- */

/** @req SWS_Spi_00001 */
void test_Spi_Init_ValidConfig_ShouldConfigureChannelRegisters(void) {
    test_Spi_EnsureInit();

    /* CONREG: enable | master mode | channel select field = channel index */
    TEST_ASSERT_EQUAL_HEX32(CONREG_EN | CONREG_MODE | (0UL << CONREG_CH_SHIFT),
                             MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_CONREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(CONREG_EN | CONREG_MODE | (1UL << CONREG_CH_SHIFT),
                             MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_CONREG_OFF));

    /* CONFIGREG: clock mode in low bits; 16-bit frames set bit 4 */
    TEST_ASSERT_EQUAL_HEX32(0x02UL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_CONFIGREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x10UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_CONFIGREG_OFF));

    /* DMAREG only written for the DMA channel: RXDEN|TXDEN with thresholds */
    TEST_ASSERT_EQUAL_HEX32(0x04040003UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_DMAREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_DMAREG_OFF));

    /* INTREG only written for the interrupt channel: TEEN|RREN */
    TEST_ASSERT_EQUAL_HEX32(INTREG_TEEN | INTREG_RREN,
                             MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_INTREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_INTREG_OFF));

    /* PERIODREG baud dividers for 80 MHz ref clock. Golden values: the
     * register carries the exact matched combination 2^preDiv * (postDiv+1)
     * >= refClock/baud. Ch0 1 Mbit/s: tempDiv=80 -> preDiv=3, postDiv=9 ->
     * 3|(9<<4) = 0x93. Ch1 8 Mbit/s: tempDiv=10 -> preDiv=0, postDiv=9 ->
     * 0|(9<<4) = 0x90. */
    TEST_ASSERT_EQUAL_HEX32(0x93UL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_PERIODREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x90UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_PERIODREG_OFF));

    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_OK, Spi_GetJobResult());
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/* --- DeInit --- */

/** @req SWS_Spi_00002 */
void test_Spi_DeInit_AfterInit_ShouldDisableAllHardwareChannels(void) {
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* CONREG cleared on all four hardware channels (ch2/ch3 share ECSPI3) */
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_CONREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_CONREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI3_BASE + ECSPI_CONREG_OFF));
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
    TEST_ASSERT_FALSE(Det_MockData.LastCallValid);
}

/** @req SWS_Spi_00001 */
void test_Spi_Init_ReInitAfterDeInit_ShouldReturnToIdle(void) {
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_DeInit());
    /* Source behavior: Spi_Init has no double-init DET; reconfiguration is
     * silent and always accepted. */
    Spi_Init(&testConfig);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_OK, Spi_GetJobResult());
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/* --- Synchronous transfer --- */

/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_Success_ShouldTransferData(void) {
    uint8 tx[2] = {0x11U, 0x22U};
    uint8 rx[2] = {0x00U, 0x00U};
    test_Spi_EnsureInit();
    /* TX FIFO never full (TF clear) and RX data always ready (RR set) */
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0xABU);

    Std_ReturnType ret = Spi_SyncTransmit(0U, tx, rx, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0xABU, rx[0]);
    TEST_ASSERT_EQUAL_HEX8(0xABU, rx[1]);
    /* Last byte pushed into the TX FIFO */
    TEST_ASSERT_EQUAL_HEX32(0x22U, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_TXDATA_OFF));
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_OK, Spi_GetJobResult());
}

/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_DeviceOnChannel1_ShouldApplyDeviceBaudRate(void) {
    uint8 tx[1] = {0x55U};
    uint8 rx[1] = {0x00U};
    test_Spi_EnsureInit();
    MockRegisters_Write32(SPI_TEST_ECSPI2_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI2_BASE + ECSPI_RXDATA_OFF, 0x99U);

    Std_ReturnType ret = Spi_SyncTransmit(1U, tx, rx, 1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0x99U, rx[0]);
    /* PERIODREG reprogrammed from the device baud (8 Mbit/s): tempDiv=10 ->
     * match preDiv=0/postDiv=9 -> 0|(9<<4) = 0x90. */
    TEST_ASSERT_EQUAL_HEX32(0x90UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_PERIODREG_OFF));
}

/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_InvalidDevice_ShouldFail(void) {
    uint8 tx[1] = {0x00U};
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Spi_SyncTransmit(5U, tx, NULL_PTR, 1U));
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_RxNeverReady_ShouldTimeoutAndFail(void) {
    uint8 tx[1] = {0x77U};
    test_Spi_EnsureInit();
    /* STATREG left at reset: RX ready never asserted. The Gpt stub advances
     * 200 ms per poll so the 1000 ms transfer timeout fires. */
    gpt_autoAdvance = TRUE;
    Std_ReturnType ret = Spi_SyncTransmit(0U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_WhileBusy_ShouldFail(void) {
    uint8 tx[1] = {0x00U};
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(0U, tx, NULL_PTR, 1U));
    TEST_ASSERT_EQUAL(SPI_BUSY, Spi_GetStatus());
    /* A synchronous request while a transfer is in progress is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, Spi_SyncTransmit(0U, tx, NULL_PTR, 1U));
}

/* --- Asynchronous transfer --- */

/** @req SWS_Spi_00004 */
void test_Spi_AsyncTransmit_InterruptMode_ShouldPrefillFifoAndStartExchange(void) {
    uint8 tx[3] = {0xA1U, 0xA2U, 0xA3U};
    uint8 rx[3] = {0x00U, 0x00U, 0x00U};
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(0U, tx, rx, 3U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(SPI_BUSY, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_PENDING, Spi_GetJobResult());
    /* Whole transfer (3 < SPI_FIFO_DEPTH) prefilled; last byte on top */
    TEST_ASSERT_EQUAL_HEX32(0xA3U, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_TXDATA_OFF));
    /* Transfer-complete + RX-ready interrupts enabled on top of Init value */
    TEST_ASSERT_EQUAL_HEX32(INTREG_TEEN | INTREG_RREN | INTREG_TCEN,
                             MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_INTREG_OFF));
    /* Exchange request asserted */
    TEST_ASSERT_EQUAL_HEX32(CONREG_EN | CONREG_MODE | CONREG_XCH,
                             MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_CONREG_OFF));
}

/** @req SWS_Spi_00004 */
void test_Spi_AsyncTransmit_DmaMode_ShouldConfigureDmaAndStartExchange(void) {
    uint8 tx[8] = {0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U};
    uint8 rx[8] = {0U};
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(1U, tx, rx, 8U); /* Length > DMA threshold */
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(SPI_BUSY, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_PENDING, Spi_GetJobResult());
    /* DMA engine delegated: TX streams into ECSPI2 TXDATA, RX drains RXDATA */
    TEST_ASSERT_EQUAL_UINT32(1U, dma_configTxCalls);
    TEST_ASSERT_EQUAL_UINT32(1U, dma_configRxCalls);
    TEST_ASSERT_EQUAL_UINT32(2U, dma_enableCalls);
    TEST_ASSERT_EQUAL_UINT8(8U, dma_lastTxChannel);
    TEST_ASSERT_EQUAL_UINT8(12U, dma_lastRxChannel);
    TEST_ASSERT_EQUAL_HEX32(0x30830004UL, dma_lastTxDst);
    TEST_ASSERT_EQUAL_HEX32(0x30830000UL, dma_lastRxSrc);
    TEST_ASSERT_EQUAL_UINT32(8U, dma_lastTxLen);
    TEST_ASSERT_EQUAL_UINT32(8U, dma_lastRxLen);
    /* Exchange started on channel 1; FIFO not prefilled in DMA mode */
    TEST_ASSERT_EQUAL_HEX32(CONREG_EN | CONREG_MODE | CONREG_XCH | (1UL << CONREG_CH_SHIFT),
                             MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_CONREG_OFF));
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_TXDATA_OFF));
}

/** @req SWS_Spi_00007 */
void test_Spi_AsyncTransmit_IsrCompletion_ShouldFinishJobAndDisableInterrupts(void) {
    uint8 tx[2] = {0x01U, 0x02U};
    uint8 rx[2] = {0x00U, 0x00U};
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(0U, tx, rx, 2U));
    /* RX FIFO ready; drain loop consumes both bytes from RXDATA */
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0xCDU);

    Spi_IsrHandler(0U);
    TEST_ASSERT_EQUAL_HEX8(0xCDU, rx[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCDU, rx[1]);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_OK, Spi_GetJobResult());
    /* Completion clears the interrupt enable register */
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_INTREG_OFF));
}

/** @req SWS_Spi_00008 */
void test_Spi_AsyncTransmit_MainFunctionTimeout_ShouldFailJobWithoutDma(void) {
    uint8 tx[2] = {0x00U, 0x00U};
    uint8 i;
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(0U, tx, NULL_PTR, 2U));
    /* Let virtual time run so the 1000 ms timeout fires */
    gpt_autoAdvance = TRUE;
    for (i = 0U; (i < 20U) && (Spi_GetStatus() == SPI_BUSY); i++) {
        Spi_MainFunction();
    }
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    /* Interrupt-mode transfer must not touch the DMA engine */
    TEST_ASSERT_EQUAL_UINT32(0U, dma_disableCalls);
}

/** @req SWS_Spi_00008 */
void test_Spi_AsyncTransmit_DmaTimeout_ShouldDisableDmaChannels(void) {
    uint8 tx[8] = {0U};
    uint8 rx[8] = {0U};
    uint8 i;
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(1U, tx, rx, 8U));
    gpt_autoAdvance = TRUE;
    for (i = 0U; (i < 20U) && (Spi_GetStatus() == SPI_BUSY); i++) {
        Spi_MainFunction();
    }
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    /* Timeout on a DMA transfer disables both DMA channels */
    TEST_ASSERT_EQUAL_UINT32(2U, dma_disableCalls);
    TEST_ASSERT_EQUAL_HEX32(0x0UL, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_DMAREG_OFF));
}

/* --- GetVersionInfo --- */

/** @req SWS_Spi_00009 */
void test_Spi_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType info;
    Spi_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(SPI_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(SPI_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(SPI_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(SPI_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Spi_00009 */
void test_Spi_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Spi_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(SPI_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(SPI_SERVICE_ID_GETVERSIONINFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(SPI_E_PARAM_POINTER, Det_MockData.ErrorId);
}

/*==================================================================================================
 * Additional Branch Coverage Tests (Target > 90%)
 *==================================================================================================*/

/* --- Init: 32-bit data mode branch (line 176) --- */
void test_Spi_Init_32BitDataMode_ShouldSetConfigreg(void) {
    test_Spi_SetupConfig();
    testChannels[0].DataMode = SPI_DATA_MODE_32BIT;
    testChannels[0].ClockMode = SPI_CLOCK_MODE_0; /* Clear clock mode bits */
    Spi_Init(&testConfig);
    /* 32-bit: cfgreg |= (3u << 4) = 0x30 */
    TEST_ASSERT_EQUAL_HEX32(0x30UL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_CONFIGREG_OFF));
}

/* --- Init: baud rate divider not-found fallback (line 238) --- */
void test_Spi_Init_VeryLowBaudRate_ShouldFallbackToMaxDivider(void) {
    test_Spi_SetupConfig();
    /* Set an extremely low baud rate so no preDiv/postDiv combo in 0..15 range
     * satisfies 2^preDiv*(postDiv+1) >= refClock/baudRate.
     * refClock=80MHz, baud=1Hz -> tempDiv=80000000. Max achievable: 2^15*16=524288 < 80M */
    testChannels[0].BaudRate = 1U;
    Spi_Init(&testConfig);
    /* Fallback: bestPre=15, bestPost=15 -> periodreg = 15 | (15<<4) = 0xFF */
    TEST_ASSERT_EQUAL_HEX32(0xFFUL, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_PERIODREG_OFF));
}

/* --- SyncTransmit: TxData=NULL (line 333 TRUE branch) --- */
void test_Spi_SyncTransmit_NullTxData_ShouldSendFF(void) {
    uint8 rx[2] = {0x00U, 0x00U};
    test_Spi_EnsureInit();
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0x55U);

    Std_ReturnType ret = Spi_SyncTransmit(0U, NULL_PTR, rx, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* When TxData is NULL, driver sends 0xFF */
    TEST_ASSERT_EQUAL_HEX32(0xFFU, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_TXDATA_OFF));
}

/* --- SyncTransmit: RxData=NULL (line 345 FALSE branch) --- */
void test_Spi_SyncTransmit_NullRxData_ShouldDiscardRxData(void) {
    uint8 tx[1] = {0xAAU};
    test_Spi_EnsureInit();
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0xBBU);

    Std_ReturnType ret = Spi_SyncTransmit(0U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/* --- SyncTransmit: TX FIFO full timeout (line 324-325) --- */
void test_Spi_SyncTransmit_TxFifoFullTimeout_ShouldFail(void) {
    uint8 tx[1] = {0x11U};
    test_Spi_EnsureInit();
    /* Set TF (TX FIFO full) bit so the while loop at line 324 spins */
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_TF);
    gpt_autoAdvance = TRUE;

    Std_ReturnType ret = Spi_SyncTransmit(0U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/* --- AsyncTransmit: invalid device (line 370) --- */
void test_Spi_AsyncTransmit_InvalidDevice_ShouldReturnNotOk(void) {
    uint8 tx[1] = {0x00U};
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(5U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- AsyncTransmit: while busy (line 379) --- */
void test_Spi_AsyncTransmit_WhileBusy_ShouldReturnNotOk(void) {
    uint8 tx[1] = {0x00U};
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(0U, tx, NULL_PTR, 1U));
    TEST_ASSERT_EQUAL(SPI_BUSY, Spi_GetStatus());
    Std_ReturnType ret = Spi_AsyncTransmit(0U, tx, NULL_PTR, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- AsyncTransmit: DMA mode with NULL TxData (line 406 FALSE) --- */
void test_Spi_AsyncTransmit_DmaNullTx_ShouldSkipTxConfig(void) {
    uint8 rx[8] = {0U};
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(1U, NULL_PTR, rx, 8U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* TxData=NULL: Dma_ConfigTx should NOT be called */
    TEST_ASSERT_EQUAL_UINT32(0U, dma_configTxCalls);
    /* RxData valid: Dma_ConfigRx should be called */
    TEST_ASSERT_EQUAL_UINT32(1U, dma_configRxCalls);
    /* Both DMA enable calls still happen */
    TEST_ASSERT_EQUAL_UINT32(2U, dma_enableCalls);
}

/* --- AsyncTransmit: DMA mode with NULL RxData (line 412 FALSE) --- */
void test_Spi_AsyncTransmit_DmaNullRx_ShouldSkipRxConfig(void) {
    uint8 tx[8] = {0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U};
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(1U, tx, NULL_PTR, 8U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* TxData valid: Dma_ConfigTx should be called */
    TEST_ASSERT_EQUAL_UINT32(1U, dma_configTxCalls);
    /* RxData=NULL: Dma_ConfigRx should NOT be called */
    TEST_ASSERT_EQUAL_UINT32(0U, dma_configRxCalls);
}

/* --- AsyncTransmit: DMA with both NULL (lines 406 FALSE + 412 FALSE) --- */
void test_Spi_AsyncTransmit_DmaBothNull_ShouldSkipBothDmaConfigs(void) {
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(1U, NULL_PTR, NULL_PTR, 8U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, dma_configTxCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, dma_configRxCalls);
    TEST_ASSERT_EQUAL_UINT32(2U, dma_enableCalls);
}

/* --- AsyncTransmit: interrupt mode with NULL TxData (line 431 ternary) --- */
void test_Spi_AsyncTransmit_InterruptNullTx_ShouldPrefillFF(void) {
    test_Spi_EnsureInit();
    Std_ReturnType ret = Spi_AsyncTransmit(0U, NULL_PTR, NULL_PTR, 3U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* When TxData is NULL, prefill sends 0xFF */
    TEST_ASSERT_EQUAL_HEX32(0xFFU, MockRegisters_Read32(SPI_TEST_ECSPI1_BASE + ECSPI_TXDATA_OFF));
}

/* --- ISR: invalid channel (line 486 TRUE) --- */
void test_Spi_IsrHandler_InvalidChannel_ShouldReturnEarly(void) {
    test_Spi_EnsureInit();
    /* Channel >= SPI_CHANNEL_COUNT should return immediately */
    Spi_IsrHandler(SPI_CHANNEL_COUNT);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/* --- ISR: called before init (line 486 FALSE via initialized==FALSE) --- */
void test_Spi_IsrHandler_BeforeInit_ShouldReturnEarly(void) {
    /* Don't init - just call ISR */
    Spi_IsrHandler(0U);
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/* --- ISR: RX interrupt with NULL RxBuffer (line 497 FALSE) --- */
void test_Spi_IsrHandler_RxInterruptNullRx_ShouldDiscardData(void) {
    uint8 tx[2] = {0x01U, 0x02U};
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(0U, tx, NULL_PTR, 2U));
    /* Set RR to enter the RX interrupt block */
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0xEEU);

    Spi_IsrHandler(0U);
    /* RxBuffer is NULL, so data is discarded (void read) */
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/* --- ISR: TX interrupt path (lines 508-511) --- */
void test_Spi_IsrHandler_TxInterrupt_ShouldFillFifo(void) {
    uint8 tx[2] = {0xAAU, 0xBBU};
    uint8 rx[2] = {0x00U, 0x00U};
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(E_OK, Spi_AsyncTransmit(0U, tx, rx, 2U));
    /* Set TE (TX empty) and RR (RX ready) bits.
     * The RX handler will drain first, then TX handler checks TE. */
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_TE | STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0xCCU);

    Spi_IsrHandler(0U);
    /* After ISR, transferred should be complete */
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    TEST_ASSERT_EQUAL(SPI_JOB_OK, Spi_GetJobResult());
}

/* --- MainFunction: not initialized (line 533 first part) --- */
void test_Spi_MainFunction_NotInitialized_ShouldReturnEarly(void) {
    /* Init then deinit to get clean uninitialized state */
    test_Spi_EnsureInit();
    Spi_DeInit();
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
    /* Now call MainFunction - should return early since not initialized */
    Spi_MainFunction();
    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/* --- MainFunction: not busy (line 533 second part) --- */
void test_Spi_MainFunction_NotBusy_ShouldReturnEarly(void) {
    test_Spi_EnsureInit();
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
    Spi_MainFunction();
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/* --- GetElapsedTime: timer wraparound (line 570 FALSE branch) --- */
void test_Spi_SyncTransmit_TimerWraparound_ShouldHandleCorrectly(void) {
    uint8 tx[1] = {0x55U};
    uint8 rx[1] = {0x00U};
    test_Spi_EnsureInit();
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_STATREG_OFF, STATREG_RR);
    MockRegisters_Write32(SPI_TEST_ECSPI1_BASE + ECSPI_RXDATA_OFF, 0xDDU);

    /* Set time close to max so it wraps during the transfer */
    gpt_timeMs = 0xFFFFFFF0U;
    gpt_autoAdvance = FALSE;

    Std_ReturnType ret = Spi_SyncTransmit(0U, tx, rx, 1U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* --- AsyncTransmit: DMA with Length <= threshold (line 401 FALSE) --- */
void test_Spi_AsyncTransmit_DmaShortLength_ShouldUseInterruptMode(void) {
    uint8 tx[2] = {0x01U, 0x02U};
    uint8 rx[2] = {0x00U, 0x00U};
    test_Spi_EnsureInit();
    /* Channel 1 has DMA enabled, but Length=2 which is <= SPI_DMA_FIFO_THRESHOLD */
    Std_ReturnType ret = Spi_AsyncTransmit(1U, tx, rx, 2U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Should use interrupt mode, not DMA */
    TEST_ASSERT_EQUAL_UINT32(0U, dma_configTxCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, dma_configRxCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, dma_enableCalls);
    /* FIFO should be prefilled (interrupt mode) */
    TEST_ASSERT_NOT_EQUAL(0U, MockRegisters_Read32(SPI_TEST_ECSPI2_BASE + ECSPI_TXDATA_OFF));
}

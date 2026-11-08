/**
 * @file test_i2c_full.c
 * @brief Host unit tests for the I2c MCAL driver.
 *
 * The driver source is included directly (white-box) so that the internal
 * START/STOP/ISR helpers can be exercised on the host.  Register accesses are
 * redirected to the behavioural model in i2c_reg_mock.c through the
 * REG_READ8 / REG_WRITE8 macros, therefore no hardware is required.
 */
#include <stdio.h>

#include "I2c.h"

/* White-box inclusion of the module under test. */
#include "../../../../src/bsw/mcal/i2c/src/I2c.c"

static int I2cTest_Failures;

#define I2C_CHECK(cond)                                                      \
    do {                                                                     \
        if (!(cond)) {                                                       \
            (void)printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            I2cTest_Failures++;                                              \
        }                                                                    \
    } while (0)

/*----------------------------------------------------------------------------
 * Test fixtures
 *---------------------------------------------------------------------------*/
static uint32 I2cTest_TxCount;
static uint32 I2cTest_RxCount;
static uint32 I2cTest_ErrCount;

static void I2cTest_TxNotify(void)
{
    I2cTest_TxCount++;
}

static void I2cTest_RxNotify(void)
{
    I2cTest_RxCount++;
}

static void I2cTest_ErrNotify(void)
{
    I2cTest_ErrCount++;
}

static const I2c_ChannelConfigType I2cTest_Channels[5] = {
    {
        0U, 0U, I2C_MODE_MASTER, I2C_TRANSFER_POLLING,
        { I2C_CLOCK_STANDARD, 0U, FALSE, TRUE },
        { { { 0U, I2C_ADDR_MODE_7BIT, FALSE } }, 0U, FALSE, FALSE },
        { FALSE, FALSE, FALSE, FALSE, 0U },
        { FALSE, FALSE, 0U, 1U, 6U, 6U },
        { FALSE, 5U, 4U },
        I2cTest_TxNotify, I2cTest_RxNotify, I2cTest_ErrNotify
    },
    {
        1U, 1U, I2C_MODE_MASTER, I2C_TRANSFER_POLLING,
        { I2C_CLOCK_FAST, 0U, FALSE, TRUE },
        { { { 0U, I2C_ADDR_MODE_7BIT, FALSE } }, 0U, FALSE, FALSE },
        { FALSE, FALSE, FALSE, FALSE, 0U },
        { FALSE, FALSE, 2U, 3U, 6U, 6U },
        { FALSE, 5U, 4U },
        NULL_PTR, NULL_PTR, NULL_PTR
    },
    {
        2U, 2U, I2C_MODE_MASTER, I2C_TRANSFER_INTERRUPT,
        { I2C_CLOCK_FAST_PLUS, 0U, TRUE, TRUE },
        { { { 0U, I2C_ADDR_MODE_7BIT, FALSE } }, 0U, FALSE, FALSE },
        { FALSE, FALSE, FALSE, FALSE, 0U },
        { TRUE, TRUE, 4U, 5U, 6U, 6U },
        { TRUE, 5U, 4U },
        I2cTest_TxNotify, I2cTest_RxNotify, I2cTest_ErrNotify
    },
    {
        3U, 3U, I2C_MODE_SLAVE, I2C_TRANSFER_POLLING,
        { I2C_CLOCK_HIGH_SPEED, 0U, FALSE, TRUE },
        { { { 0x28U, I2C_ADDR_MODE_7BIT, FALSE } }, 1U, FALSE, FALSE },
        { FALSE, FALSE, FALSE, FALSE, 0U },
        { FALSE, FALSE, 6U, 7U, 6U, 6U },
        { FALSE, 5U, 4U },
        I2cTest_TxNotify, I2cTest_RxNotify, I2cTest_ErrNotify
    },
    {
        4U, 9U, I2C_MODE_MASTER, I2C_TRANSFER_POLLING,
        { (I2c_ClockModeType)99, 0U, FALSE, TRUE },
        { { { 0U, I2C_ADDR_MODE_7BIT, FALSE } }, 0U, FALSE, FALSE },
        { FALSE, FALSE, FALSE, FALSE, 0U },
        { FALSE, FALSE, 0U, 1U, 6U, 6U },
        { FALSE, 5U, 4U },
        NULL_PTR, NULL_PTR, NULL_PTR
    }
};

static const I2c_ConfigType I2cTest_Config = {
    I2cTest_Channels,
    5U,
    TRUE,
    TRUE,
    24000000U
};

static void I2cTest_Setup(void)
{
    uint8 i;

    I2cMock_Reset();
    (void)I2c_DeInit();
    I2c_Init(&I2cTest_Config);

    /* The driver keeps buffer bookkeeping across Init; clear it so each test
     * starts from a known state. */
    for (i = 0U; i < I2C_NUM_CHANNELS; i++) {
        I2c_ChannelInfo[i].TxBuffer.Buffer = NULL_PTR;
        I2c_ChannelInfo[i].TxBuffer.Length = 0U;
        I2c_ChannelInfo[i].TxBuffer.Index = 0U;
        I2c_ChannelInfo[i].RxBuffer.Buffer = NULL_PTR;
        I2c_ChannelInfo[i].RxBuffer.Length = 0U;
        I2c_ChannelInfo[i].RxBuffer.Index = 0U;
        I2c_ChannelInfo[i].Result = I2C_RESULT_OK;
    }

    I2cTest_TxCount = 0U;
    I2cTest_RxCount = 0U;
    I2cTest_ErrCount = 0U;
}

static void I2cTest_Teardown(void)
{
    (void)I2c_DeInit();
    I2cMock_Reset();
}

/*----------------------------------------------------------------------------
 * Init / DeInit
 *---------------------------------------------------------------------------*/
static void Test_I2c_InitDeInit(void)
{
    Std_VersionInfoType version;

    I2cMock_Reset();
    (void)I2c_DeInit();

    I2c_Init(NULL_PTR);
    I2C_CHECK(I2c_GetStatus() == I2C_UNINIT);

    I2c_Init(&I2cTest_Config);
    I2C_CHECK(I2c_GetStatus() == I2C_IDLE);

    /* Second initialisation is rejected by DET. */
    I2c_Init(&I2cTest_Config);
    I2C_CHECK(I2c_GetStatus() == I2C_IDLE);

    I2c_GetVersionInfo(NULL_PTR);
    I2c_GetVersionInfo(&version);
    I2C_CHECK(version.moduleID == I2C_MODULE_ID);

    /* Slave address programmed during init for the slave channel. */
    I2C_CHECK((I2cMock_GetReg(I2C4_BASE_ADDR + I2C_IADR) & 0xFFU) == (uint8)(0x28U << 1));

    I2C_CHECK(I2c_DeInit() == E_OK);
    I2C_CHECK(I2c_GetStatus() == I2C_UNINIT);
    I2C_CHECK(I2c_DeInit() == E_NOT_OK);
}

/*----------------------------------------------------------------------------
 * API behaviour when not initialised
 *---------------------------------------------------------------------------*/
static void Test_I2c_UninitApis(void)
{
    uint8 data = 0U;
    const uint8 tx = 1U;

    I2cMock_Reset();
    (void)I2c_DeInit();

    I2C_CHECK(I2c_WriteBytes(0U, 0x10U, &tx, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_ReadBytes(0U, 0x10U, &data, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteRead(0U, 0x10U, &tx, 1U, &data, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_SetClockMode(0U, I2C_CLOCK_FAST) == E_NOT_OK);
    I2C_CHECK(I2c_EnableInterrupt(0U) == E_NOT_OK);
    I2C_CHECK(I2c_DisableInterrupt(0U) == E_NOT_OK);
    I2C_CHECK(I2c_SetSlaveAddress(3U, 0x30U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_GetBusState(0U) == I2C_BUS_STATE_BUSY);
    I2C_CHECK(I2c_ClearBus(0U) == E_NOT_OK);
    I2C_CHECK(I2c_SoftwareReset(0U) == E_NOT_OK);
    I2C_CHECK(I2c_SetTransferMode(0U, I2C_TRANSFER_POLLING) == E_NOT_OK);
    I2C_CHECK(I2c_CancelTransfer(0U) == E_NOT_OK);
    I2C_CHECK(I2c_PrepareSlaveBuffer(3U, &data, 1U) == E_NOT_OK);
    I2C_CHECK(I2c_SlaveWriteBuffer(3U, &tx, 1U) == E_NOT_OK);
    I2C_CHECK(I2c_SlaveReadBuffer(3U, &data, 1U) == E_NOT_OK);
    I2c_MainFunction();
}

/*----------------------------------------------------------------------------
 * Parameter validation
 *---------------------------------------------------------------------------*/
static void Test_I2c_ParameterChecks(void)
{
    uint8 buffer[4] = { 0U, 0U, 0U, 0U };
    uint8 big[I2C_MAX_BUFFER_SIZE + 1U];

    I2cTest_Setup();

    I2C_CHECK(I2c_WriteBytes(9U, 0x10U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteBytes(0U, 0x10U, NULL_PTR, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteBytes(0U, 0x10U, big, (I2c_LengthType)(I2C_MAX_BUFFER_SIZE + 1U),
                             I2C_ADDR_MODE_7BIT) == E_NOT_OK);

    I2C_CHECK(I2c_ReadBytes(9U, 0x10U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_ReadBytes(0U, 0x10U, NULL_PTR, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_ReadBytes(0U, 0x10U, big, (I2c_LengthType)(I2C_MAX_BUFFER_SIZE + 1U),
                            I2C_ADDR_MODE_7BIT) == E_NOT_OK);

    I2C_CHECK(I2c_WriteRead(9U, 0x10U, buffer, 1U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteRead(0U, 0x10U, NULL_PTR, 1U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteRead(0U, 0x10U, buffer, 1U, NULL_PTR, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteRead(0U, 0x10U, big, (I2c_LengthType)(I2C_MAX_BUFFER_SIZE + 1U),
                            buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteRead(0U, 0x10U, buffer, 1U, big,
                            (I2c_LengthType)(I2C_MAX_BUFFER_SIZE + 1U),
                            I2C_ADDR_MODE_7BIT) == E_NOT_OK);

    /* Busy channel protection. */
    I2c_ChannelInfo[0].State = I2C_STATE_MASTER_TX;
    I2C_CHECK(I2c_WriteBytes(0U, 0x10U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_ReadBytes(0U, 0x10U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_WriteRead(0U, 0x10U, buffer, 1U, buffer, 1U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2c_ChannelInfo[0].State = I2C_STATE_IDLE;

    I2cTest_Teardown();
}

/*----------------------------------------------------------------------------
 * Polling transfers
 *---------------------------------------------------------------------------*/
static void Test_I2c_PollingWrite(void)
{
    uint8 tx[2] = { 0x11U, 0x22U };

    I2cTest_Setup();

    I2C_CHECK(I2c_WriteBytes(0U, 0x50U, tx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(I2c_ChannelInfo[0].TxBuffer.Index == 2U);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_IDLE);

    /* Empty write exercises the address-only path. */
    I2C_CHECK(I2c_WriteBytes(0U, 0x50U, NULL_PTR, 0U, I2C_ADDR_MODE_7BIT) == E_OK);

    I2cTest_Teardown();
}

static void Test_I2c_PollingRead(void)
{
    uint8 rx[2] = { 0U, 0U };

    I2cTest_Setup();
    I2cMock_SetRxByte(0x3CU);

    I2C_CHECK(I2c_ReadBytes(0U, 0x50U, rx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(rx[0] == 0x3CU);
    I2C_CHECK(rx[1] == 0x3CU);
    I2C_CHECK(I2c_ChannelInfo[0].RxBuffer.Index == 2U);

    I2C_CHECK(I2c_ReadBytes(0U, 0x50U, NULL_PTR, 0U, I2C_ADDR_MODE_7BIT) == E_OK);

    I2cTest_Teardown();
}

static void Test_I2c_PollingWriteRead(void)
{
    uint8 tx[2] = { 0x01U, 0x02U };
    uint8 rx[2] = { 0U, 0U };

    I2cTest_Setup();
    I2cMock_SetRxByte(0x77U);

    I2C_CHECK(I2c_WriteRead(0U, 0x50U, tx, 2U, rx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(rx[0] == 0x77U);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_IDLE);

    I2cTest_Teardown();
}

static void Test_I2c_TenBitAddress(void)
{
    uint8 tx = 0x01U;
    uint8 rx = 0U;

    I2cTest_Setup();

    I2C_CHECK(I2c_WriteBytes(0U, 0x1A5U, &tx, 1U, I2C_ADDR_MODE_10BIT) == E_OK);
    I2C_CHECK(I2c_ReadBytes(1U, 0x1A5U, &rx, 1U, I2C_ADDR_MODE_10BIT) == E_OK);

    I2cTest_Teardown();
}

/*----------------------------------------------------------------------------
 * Interrupt and DMA modes
 *---------------------------------------------------------------------------*/
static void Test_I2c_InterruptMode(void)
{
    uint8 tx[2] = { 0x01U, 0x02U };
    uint8 rx[2] = { 0U, 0U };

    I2cTest_Setup();

    I2C_CHECK(I2c_WriteBytes(2U, 0x50U, tx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(I2c_ChannelInfo[2].State == I2C_STATE_MASTER_TX);
    I2C_CHECK(I2c_ChannelInfo[2].Result == I2C_RESULT_PENDING);

    I2c_ChannelInfo[2].State = I2C_STATE_IDLE;
    I2c_ChannelInfo[2].Result = I2C_RESULT_OK;
    I2cMock_Reset();
    I2C_CHECK(I2c_ReadBytes(2U, 0x50U, rx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(I2c_ChannelInfo[2].State == I2C_STATE_MASTER_RX);

    I2c_ChannelInfo[2].State = I2C_STATE_IDLE;
    I2c_ChannelInfo[2].Result = I2C_RESULT_OK;
    I2cMock_Reset();
    I2C_CHECK(I2c_WriteRead(2U, 0x50U, tx, 1U, rx, 1U, I2C_ADDR_MODE_7BIT) == E_OK);

    I2cTest_Teardown();
}

static void Test_I2c_DmaMode(void)
{
    uint8 tx[2] = { 0x01U, 0x02U };
    uint8 rx[2] = { 0U, 0U };

    I2cTest_Setup();

    I2C_CHECK(I2c_SetTransferMode(0U, I2C_TRANSFER_DMA) == E_OK);
    I2C_CHECK(I2c_WriteBytes(0U, 0x50U, tx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(I2c_ChannelInfo[0].Result == I2C_RESULT_PENDING);

    I2C_CHECK(I2c_SetTransferMode(0U, I2C_TRANSFER_DMA) == E_OK);
    I2C_CHECK(I2c_CancelTransfer(0U) == E_OK);
    I2C_CHECK(I2c_ChannelInfo[0].Result == I2C_RESULT_CANCELLED);

    I2C_CHECK(I2c_ReadBytes(0U, 0x50U, rx, 2U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_MASTER_RX);

    I2c_ChannelInfo[0].State = I2C_STATE_IDLE;
    I2c_ChannelInfo[0].Result = I2C_RESULT_OK;
    I2cMock_Reset();
    I2C_CHECK(I2c_WriteRead(0U, 0x50U, tx, 1U, rx, 1U, I2C_ADDR_MODE_7BIT) == E_OK);

    I2cTest_Teardown();
}

/*----------------------------------------------------------------------------
 * Interrupt handler
 *---------------------------------------------------------------------------*/
static void Test_I2c_IsrHandler(void)
{
    uint8 tx[2] = { 0x01U, 0x02U };
    uint8 rx[2] = { 0U, 0U };

    I2cTest_Setup();

    /* Arbitration lost, notification configured. */
    I2cMock_SetArbitrationLost(1U);
    I2c_ChannelInfo[0].State = I2C_STATE_MASTER_TX;
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_ERROR);
    I2C_CHECK(I2c_ChannelInfo[0].Result == I2C_RESULT_FAILED);
    I2C_CHECK((I2c_ChannelInfo[0].ErrorFlags & I2C_E_ARBITRATION_LOST) != 0U);
    I2C_CHECK(I2cTest_ErrCount == 1U);

    /* Arbitration lost, no notification configured. */
    I2c_ChannelInfo[1].State = I2C_STATE_MASTER_TX;
    I2c_IsrHandler(1U);
    I2C_CHECK(I2c_ChannelInfo[1].State == I2C_STATE_ERROR);
    I2C_CHECK(I2cTest_ErrCount == 1U);
    I2cMock_SetArbitrationLost(0U);

    /* NACK during master transmit. */
    I2cMock_SetNack(1U);
    I2c_ChannelInfo[0].State = I2C_STATE_MASTER_TX;
    I2c_IsrHandler(0U);
    I2C_CHECK((I2c_ChannelInfo[0].ErrorFlags & I2C_E_ACK_ERROR) != 0U);
    I2C_CHECK(I2cTest_ErrCount == 2U);
    I2cMock_SetNack(0U);

    /* Transmit one byte. */
    I2c_ChannelInfo[0].State = I2C_STATE_MASTER_TX;
    I2c_ChannelInfo[0].TxBuffer.Buffer = tx;
    I2c_ChannelInfo[0].TxBuffer.Length = 2U;
    I2c_ChannelInfo[0].TxBuffer.Index = 0U;
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].TxBuffer.Index == 1U);

    /* Transmit complete, request STOP. */
    I2c_ChannelInfo[0].TxBuffer.Length = 0U;
    I2c_ChannelInfo[0].TxBuffer.Index = 0U;
    I2c_ChannelInfo[0].RxBuffer.Length = 0U;
    I2c_ChannelInfo[0].StopRequested = TRUE;
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_IDLE);
    I2C_CHECK(I2c_ChannelInfo[0].Result == I2C_RESULT_OK);
    I2C_CHECK(I2cTest_TxCount == 1U);

    /* Transmit complete with pending RX: switch to receive and repeated start. */
    I2c_ChannelInfo[0].State = I2C_STATE_MASTER_TX;
    I2c_ChannelInfo[0].TxBuffer.Length = 0U;
    I2c_ChannelInfo[0].TxBuffer.Index = 0U;
    I2c_ChannelInfo[0].RxBuffer.Buffer = rx;
    I2c_ChannelInfo[0].RxBuffer.Length = 2U;
    I2c_ChannelInfo[0].RxBuffer.Index = 0U;
    I2c_ChannelInfo[0].IsRepeatedStart = FALSE;
    I2c_ChannelInfo[0].CurrentSlaveAddress = 0x50U;
    I2c_ChannelInfo[0].CurrentAddrMode = I2C_ADDR_MODE_7BIT;
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_MASTER_RX);

    /* Receive one byte. */
    I2cMock_SetRxByte(0x42U);
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].RxBuffer.Index == 1U);
    I2C_CHECK(rx[0] == 0x42U);

    /* Last byte fills the buffer. */
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].RxBuffer.Index == 2U);

    /* Receive complete. */
    I2c_ChannelInfo[0].StopRequested = TRUE;
    I2c_IsrHandler(0U);
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_IDLE);
    I2C_CHECK(I2cTest_RxCount == 1U);

    /* Idle state falls through the default branch. */
    I2c_ChannelInfo[0].State = I2C_STATE_IDLE;
    I2c_IsrHandler(0U);

    I2cTest_Teardown();
}

/*----------------------------------------------------------------------------
 * Internal helper error paths
 *---------------------------------------------------------------------------*/
static void Test_I2c_InternalHelpers(void)
{
    uint32 base;
    uint8 data = 0U;

    I2cTest_Setup();
    base = I2c_GetBaseAddr(0U);

    I2C_CHECK(I2c_GetBaseAddr(1U) == I2C2_BASE_ADDR);
    I2C_CHECK(I2c_GetBaseAddr(2U) == I2C3_BASE_ADDR);
    I2C_CHECK(I2c_GetBaseAddr(3U) == I2C4_BASE_ADDR);
    I2C_CHECK(I2c_GetBaseAddr(9U) == 0U);

    /* Exact divider match exits the search early. */
    I2c_SetFrequency(base, 24000000U / 100U);
    I2C_CHECK(I2cMock_GetReg(base + I2C_IFDR) == 35U);

    I2cMock_SetBusyStuck(1U);
    I2C_CHECK(I2c_WaitForBusIdle(base, 1U) == E_NOT_OK);
    I2C_CHECK(I2c_SendStart(base, 0x50U, I2C_ADDR_MODE_7BIT, TRUE) == E_NOT_OK);
    I2C_CHECK(I2c_SendStop(base) == E_NOT_OK);
    I2cMock_SetBusyStuck(0U);

    I2cMock_SetNoInterruptFlag(1U);
    I2C_CHECK(I2c_WaitForTransferComplete(base, 1U) == E_NOT_OK);
    I2C_CHECK(I2c_WriteByte(base, 0x01U, 1U) == E_NOT_OK);
    I2C_CHECK(I2c_ReadByte(base, &data, TRUE, 1U) == E_NOT_OK);
    I2C_CHECK(I2c_SendStart(base, 0x50U, I2C_ADDR_MODE_7BIT, TRUE) == E_NOT_OK);
    I2cMock_SetNoInterruptFlag(0U);

    I2cMock_Reset();
    I2cMock_SetNack(1U);
    I2C_CHECK(I2c_SendStart(base, 0x50U, I2C_ADDR_MODE_7BIT, TRUE) == E_NOT_OK);
    I2cMock_Reset();
    I2cMock_SetNack(1U);
    I2C_CHECK(I2c_SendStart(base, 0x1A5U, I2C_ADDR_MODE_10BIT, TRUE) == E_NOT_OK);
    I2C_CHECK(I2c_WriteByte(base, 0x01U, 1U) == E_NOT_OK);
    I2cMock_SetNack(0U);

    I2cMock_Reset();

    I2C_CHECK(I2c_SendStart(base, 0x50U, I2C_ADDR_MODE_7BIT, TRUE) == E_OK);
    I2cMock_Reset();
    I2C_CHECK(I2c_SendStart(base, 0x1A5U, I2C_ADDR_MODE_10BIT, TRUE) == E_OK);
    I2C_CHECK(I2c_WriteByte(base, 0x11U, 1U) == E_OK);
    I2C_CHECK(I2c_ReadByte(base, &data, TRUE, 1U) == E_OK);
    I2C_CHECK(I2c_ReadByte(base, &data, FALSE, 1U) == E_OK);
    I2C_CHECK(I2c_SendRepeatedStart(base, 0x50U, I2C_ADDR_MODE_7BIT, FALSE) == E_OK);
    I2C_CHECK(I2c_SendStop(base) == E_OK);

    /* Error handler with and without notification. */
    I2c_ErrorHandler(0U, 0x1234U);
    I2C_CHECK(I2cTest_ErrCount == 1U);
    I2c_ErrorHandler(1U, 0x1234U);
    I2C_CHECK(I2cTest_ErrCount == 1U);

    /* DMA helper stubs. */
    I2c_DmaInit(0U, &data, 1U, TRUE);
    I2c_DmaStart(0U);
    I2c_DmaStop(0U);

    /* Direct transfer helpers (bypassing the public wrappers). */
    I2c_ChannelInfo[0].TxBuffer.Buffer = &data;
    I2c_ChannelInfo[0].TxBuffer.Length = 1U;
    I2c_ChannelInfo[0].TxBuffer.Index = 0U;
    I2c_ChannelInfo[0].RxBuffer.Length = 0U;
    I2c_ChannelInfo[0].CurrentSlaveAddress = 0x50U;
    I2c_ChannelInfo[0].CurrentAddrMode = I2C_ADDR_MODE_7BIT;
    I2c_ChannelInfo[0].StopRequested = TRUE;
    I2cMock_Reset();
    I2C_CHECK(I2c_MasterTransferDma(0U) == E_OK);
    I2cMock_Reset();
    I2C_CHECK(I2c_MasterTransferInterrupt(0U) == E_OK);
    I2cMock_Reset();
    I2C_CHECK(I2c_MasterTransferPolling(0U) == E_OK);

    I2c_ChannelInfo[0].TxBuffer.Buffer = NULL_PTR;
    I2c_ChannelInfo[0].TxBuffer.Length = 0U;
    I2c_ChannelInfo[0].RxBuffer.Buffer = &data;
    I2c_ChannelInfo[0].RxBuffer.Length = 1U;
    I2c_ChannelInfo[0].RxBuffer.Index = 0U;
    I2cMock_Reset();
    I2C_CHECK(I2c_MasterTransferDma(0U) == E_OK);

    I2cTest_Teardown();
}

/*----------------------------------------------------------------------------
 * Slave buffers and miscellaneous APIs
 *---------------------------------------------------------------------------*/
static void Test_I2c_SlaveBuffers(void)
{
    uint8 rxbuf[4] = { 0U, 0U, 0U, 0U };
    uint8 out[6];
    uint8 txbuf[4] = { 1U, 2U, 3U, 4U };

    I2cTest_Setup();

    I2C_CHECK(I2c_PrepareSlaveBuffer(3U, rxbuf, 4U) == E_OK);
    I2C_CHECK(I2c_PrepareSlaveBuffer(3U, NULL_PTR, 0U) == E_OK);
    I2C_CHECK(I2c_PrepareSlaveBuffer(3U, NULL_PTR, 2U) == E_NOT_OK);
    I2C_CHECK(I2c_PrepareSlaveBuffer(9U, rxbuf, 4U) == E_NOT_OK);

    I2C_CHECK(I2c_SlaveWriteBuffer(3U, txbuf, 4U) == E_OK);
    I2C_CHECK(I2c_SlaveWriteBuffer(3U, NULL_PTR, 0U) == E_OK);
    I2C_CHECK(I2c_SlaveWriteBuffer(3U, NULL_PTR, 2U) == E_NOT_OK);
    I2C_CHECK(I2c_SlaveWriteBuffer(9U, txbuf, 4U) == E_NOT_OK);

    I2c_ChannelInfo[3].RxBuffer.Buffer = txbuf;
    I2c_ChannelInfo[3].RxBuffer.Length = 4U;
    I2c_ChannelInfo[3].RxBuffer.Index = 3U;

    I2C_CHECK(I2c_SlaveReadBuffer(3U, out, 6U) == E_OK);
    I2C_CHECK(out[0] == 1U);
    I2C_CHECK(out[2] == 3U);
    I2C_CHECK(I2c_SlaveReadBuffer(3U, NULL_PTR, 0U) == E_OK);
    I2C_CHECK(I2c_SlaveReadBuffer(3U, NULL_PTR, 2U) == E_NOT_OK);
    I2C_CHECK(I2c_SlaveReadBuffer(9U, out, 2U) == E_NOT_OK);

    I2cTest_Teardown();
}

static void Test_I2c_MiscApis(void)
{
    I2cTest_Setup();

    I2C_CHECK(I2c_SetClockMode(0U, I2C_CLOCK_STANDARD) == E_OK);
    I2C_CHECK(I2c_SetClockMode(0U, I2C_CLOCK_FAST) == E_OK);
    I2C_CHECK(I2c_SetClockMode(0U, I2C_CLOCK_FAST_PLUS) == E_OK);
    I2C_CHECK(I2c_SetClockMode(0U, I2C_CLOCK_HIGH_SPEED) == E_OK);
    I2C_CHECK(I2c_SetClockMode(0U, (I2c_ClockModeType)9) == E_NOT_OK);
    I2C_CHECK(I2c_SetClockMode(9U, I2C_CLOCK_FAST) == E_NOT_OK);

    I2C_CHECK(I2c_EnableInterrupt(0U) == E_OK);
    I2C_CHECK(I2c_DisableInterrupt(0U) == E_OK);
    I2C_CHECK(I2c_EnableInterrupt(9U) == E_NOT_OK);
    I2C_CHECK(I2c_DisableInterrupt(9U) == E_NOT_OK);

    I2C_CHECK(I2c_SetSlaveAddress(3U, 0x30U, I2C_ADDR_MODE_7BIT) == E_OK);
    I2C_CHECK(I2c_SetSlaveAddress(3U, 0x30U, I2C_ADDR_MODE_10BIT) == E_NOT_OK);
    I2C_CHECK(I2c_SetSlaveAddress(0U, 0x30U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);
    I2C_CHECK(I2c_SetSlaveAddress(9U, 0x30U, I2C_ADDR_MODE_7BIT) == E_NOT_OK);

    I2cMock_SetBusyStuck(1U);
    I2C_CHECK(I2c_GetBusState(0U) == I2C_BUS_STATE_BUSY);
    I2cMock_SetBusyStuck(0U);
    I2C_CHECK(I2c_GetBusState(0U) == I2C_BUS_STATE_IDLE);
    I2C_CHECK(I2c_GetBusState(9U) == I2C_BUS_STATE_BUSY);

    I2C_CHECK(I2c_ClearBus(0U) == E_OK);
    I2C_CHECK(I2c_ClearBus(9U) == E_NOT_OK);
    I2C_CHECK(I2c_SoftwareReset(0U) == E_OK);
    I2C_CHECK(I2c_SoftwareReset(9U) == E_NOT_OK);

    I2C_CHECK(I2c_SetTransferMode(0U, I2C_TRANSFER_POLLING) == E_OK);
    I2C_CHECK(I2c_SetTransferMode(0U, I2C_TRANSFER_INTERRUPT) == E_OK);
    I2C_CHECK(I2c_SetTransferMode(0U, I2C_TRANSFER_DMA) == E_OK);
    I2C_CHECK(I2c_SetTransferMode(0U, (I2c_TransferModeType)7) == E_NOT_OK);
    I2C_CHECK(I2c_SetTransferMode(9U, I2C_TRANSFER_POLLING) == E_NOT_OK);

    I2C_CHECK(I2c_CancelTransfer(0U) == E_OK);
    I2C_CHECK(I2c_CancelTransfer(9U) == E_NOT_OK);

    I2c_ChannelInfo[0].CurrentMode = (I2c_TransferModeType)9;
    {
        uint8 byte = 0x5AU;
        I2C_CHECK(I2c_WriteBytes(0U, 0x50U, &byte, 1U, I2C_ADDR_MODE_7BIT) == E_OK);
        I2C_CHECK(I2c_ReadBytes(0U, 0x50U, &byte, 1U, I2C_ADDR_MODE_7BIT) == E_OK);
        I2C_CHECK(I2c_WriteRead(0U, 0x50U, &byte, 1U, &byte, 1U, I2C_ADDR_MODE_7BIT) == E_OK);
    }

    I2cTest_Teardown();
}

/*----------------------------------------------------------------------------
 * Main function
 *---------------------------------------------------------------------------*/
static void Test_I2c_MainFunction(void)
{
    I2cTest_Setup();

    /* Nothing pending. */
    I2c_MainFunction();
    I2C_CHECK(I2cTest_TxCount == 0U);

    /* TX/RX notifications on the first channel. */
    I2c_ChannelInfo[0].Result = I2C_RESULT_OK;
    I2c_ChannelInfo[0].TxBuffer.Index = 2U;
    I2c_ChannelInfo[0].RxBuffer.Index = 2U;
    I2c_MainFunction();
    I2C_CHECK(I2cTest_TxCount == 1U);
    I2C_CHECK(I2cTest_RxCount == 1U);

    /* Notification pointers are NULL on channel 1. */
    I2c_ChannelInfo[0].TxBuffer.Index = 0U;
    I2c_ChannelInfo[0].RxBuffer.Index = 0U;
    I2c_ChannelInfo[1].Result = I2C_RESULT_OK;
    I2c_ChannelInfo[1].TxBuffer.Index = 1U;
    I2c_ChannelInfo[1].RxBuffer.Index = 1U;
    I2c_MainFunction();
    I2C_CHECK(I2cTest_TxCount == 1U);
    I2C_CHECK(I2cTest_RxCount == 1U);

    /* Error state notification and reset. */
    I2c_ChannelInfo[0].State = I2C_STATE_ERROR;
    I2c_MainFunction();
    I2C_CHECK(I2c_ChannelInfo[0].State == I2C_STATE_IDLE);
    I2C_CHECK(I2cTest_ErrCount == 1U);

    I2c_ChannelInfo[1].State = I2C_STATE_ERROR;
    I2c_MainFunction();
    I2C_CHECK(I2c_ChannelInfo[1].State == I2C_STATE_IDLE);
    I2C_CHECK(I2cTest_ErrCount == 1U);

    I2cTest_Teardown();
}

int main(void)
{
    I2cTest_Failures = 0;

    Test_I2c_InitDeInit();
    Test_I2c_UninitApis();
    Test_I2c_ParameterChecks();
    Test_I2c_PollingWrite();
    Test_I2c_PollingRead();
    Test_I2c_PollingWriteRead();
    Test_I2c_TenBitAddress();
    Test_I2c_InterruptMode();
    Test_I2c_DmaMode();
    Test_I2c_IsrHandler();
    Test_I2c_InternalHelpers();
    Test_I2c_SlaveBuffers();
    Test_I2c_MiscApis();
    Test_I2c_MainFunction();

    if (I2cTest_Failures != 0) {
        (void)printf("%d I2C check(s) failed\n", I2cTest_Failures);
        return 1;
    }

    (void)printf("All I2C tests passed\n");
    return 0;
}

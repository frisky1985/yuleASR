/**
 * @file test_i2c.c
 * @brief I2c Unit Tests (substantiated)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Links the real production source src/bsw/mcal/i2c/src/I2c.c against the
 * mock register model and the mock_det error hook.
 *
 * Notes on driver behaviour covered here:
 *  - The i.MX8M register model is fully static, so I2c_SendStart() can never
 *    succeed: it first waits for IBB == 0 (bus idle) and then for IBB != 0
 *    (bus busy); the mock value cannot change in between. Every master
 *    transfer therefore runs the documented failure path (SendStop +
 *    E_NOT_OK). The channel then stays in MASTER_TX/RX until
 *    I2c_SoftwareReset() or DeInit/Init restores it.
 *  - Because of that, the channels are configured with
 *    I2C_TRANSFER_INTERRUPT. In I2C_TRANSFER_POLLING mode a failed start
 *    leaves State == MASTER_TX and I2c_WriteBytes() enters a busy-wait loop
 *    that never exits (host test would hang) - polling transfers are only
 *    touched indirectly through I2c_SetTransferMode().
 *  - I2c_DeInit() fully resets the module state, so every test re-initialises
 *    the driver through test_I2c_EnsureInitialized() (DeInit + Init cycle).
 *  - Tests that must observe the genuine uninitialized state are named
 *    *BeforeInit* so the generated runner executes them first.
 */

// @tests src/bsw/mcal/i2c/src/I2c.c  @tests src/bsw/mcal/i2c/include/I2c.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "I2c.h"
#include "I2c_Cfg.h"

/*===========================================================================
 * Register model (i.MX8M Mini I2C, 8-bit accesses)
 *==========================================================================*/
#define TEST_I2C1_BASE                  (0x30A20000UL)
#define TEST_I2C2_BASE                  (0x30A30000UL)

#define TEST_I2C_IADR(base)             ((base) + 0x00U)
#define TEST_I2C_IFDR(base)             ((base) + 0x04U)
#define TEST_I2C_I2CR(base)             ((base) + 0x08U)
#define TEST_I2C_I2SR(base)             ((base) + 0x0CU)
#define TEST_I2C_I2DR(base)             ((base) + 0x10U)

#define TEST_I2C_IEN                    (0x80U)
#define TEST_I2C_IIEN                   (0x40U)
#define TEST_I2C_MSTA                   (0x20U)
#define TEST_I2C_MTX                    (0x10U)
#define TEST_I2C_IBB                    (0x20U)

#define TEST_I2C_INSTANCE_ID            (0x00U)
#define TEST_I2C_SLAVE_ADDR             (0x50U)

/*===========================================================================
 * Test configuration
 * Channel 0: master  on I2C1 (HwUnit 0)
 * Channel 1: slave   on I2C2 (HwUnit 1), own address 0x50
 *==========================================================================*/
static I2c_ChannelConfigType test_ChannelConfigs[2U];
static I2c_ConfigType test_Config;

static uint32 test_TxNotifyCount = 0U;
static uint32 test_RxNotifyCount = 0U;
static uint32 test_ErrNotifyCount = 0U;

static void test_I2c_TxNotification(void)  { test_TxNotifyCount++; }
static void test_I2c_RxNotification(void)  { test_RxNotifyCount++; }
static void test_I2c_ErrorNotification(void) { test_ErrNotifyCount++; }

static void test_I2c_SetupConfig(uint8 numChannels, I2c_TransferModeType mode)
{
    uint8 i;
    uint8 s;

    for (i = 0U; i < 2U; i++) {
        test_ChannelConfigs[i].ChannelId = i;
        test_ChannelConfigs[i].HwUnit = i;
        test_ChannelConfigs[i].OpMode = (i == 0U) ? I2C_MODE_MASTER : I2C_MODE_SLAVE;
        test_ChannelConfigs[i].TransferMode = mode;

        test_ChannelConfigs[i].MasterConfig.ClockMode = I2C_CLOCK_FAST;
        test_ChannelConfigs[i].MasterConfig.CustomClockFreq = 0U;
        test_ChannelConfigs[i].MasterConfig.MultiMasterEnabled = FALSE;
        test_ChannelConfigs[i].MasterConfig.ClockStretchingEnabled = FALSE;

        for (s = 0U; s < I2C_MAX_SLAVE_ADDRESSES; s++) {
            test_ChannelConfigs[i].SlaveConfig.SlaveAddresses[s].Address = TEST_I2C_SLAVE_ADDR;
            test_ChannelConfigs[i].SlaveConfig.SlaveAddresses[s].AddrMode = I2C_ADDR_MODE_7BIT;
            test_ChannelConfigs[i].SlaveConfig.SlaveAddresses[s].GeneralCallEnabled = FALSE;
        }
        test_ChannelConfigs[i].SlaveConfig.NumSlaveAddresses = (i == 0U) ? 0U : 1U;
        test_ChannelConfigs[i].SlaveConfig.DualAddressEnabled = FALSE;
        test_ChannelConfigs[i].SlaveConfig.GeneralCallEnabled = FALSE;

        test_ChannelConfigs[i].TxNotification = test_I2c_TxNotification;
        test_ChannelConfigs[i].RxNotification = test_I2c_RxNotification;
        test_ChannelConfigs[i].ErrorNotification = test_I2c_ErrorNotification;
    }

    test_Config.Channels = test_ChannelConfigs;
    test_Config.NumChannels = numChannels;
    test_Config.DevErrorDetect = TRUE;
    test_Config.VersionInfoApi = TRUE;
    test_Config.PeripheralClockFreq = I2C_PERIPHERAL_CLOCK_FREQ;

    test_TxNotifyCount = 0U;
    test_RxNotifyCount = 0U;
    test_ErrNotifyCount = 0U;
}

/*===========================================================================
 * Driver (re-)initialisation helper
 * I2c_DeInit clears the static init flag, so a DeInit/Init cycle restores a
 * canonical post-Init state for every test.
 *==========================================================================*/
static boolean test_I2cDriverReady = FALSE;

static void test_I2c_EnsureInitialized(void)
{
    if (test_I2cDriverReady) {
        (void)I2c_DeInit();
    }
    MockRegisters_Reset();
    test_I2c_SetupConfig(2U, I2C_TRANSFER_INTERRUPT);
    I2c_Init(&test_Config);
    Det_Mock_Reset();
    test_I2cDriverReady = TRUE;
}

/*===========================================================================
 * Unity fixture
 *==========================================================================*/
void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void)
{
}

/*===========================================================================
 * DET assertion helper
 *==========================================================================*/
static void test_I2c_ExpectDet(uint8 ApiId, uint8 ErrorId)
{
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL_UINT32(I2C_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_INSTANCE_ID, Det_MockData.InstanceId);
    TEST_ASSERT_EQUAL_UINT32(ApiId, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL_UINT32(ErrorId, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(1U, Det_MockData.CallCount);
}

/*===========================================================================
 * Uninitialized-driver tests (executed first by the generated runner)
 *==========================================================================*/

/** @req SWS_I2c_00001 */
void test_I2c_BeforeInit_Init_NullConfig_ShouldReportDet(void)
{
    I2c_Init(NULL_PTR);
    test_I2c_ExpectDet(I2C_SID_INIT, I2C_E_PARAM_CONFIG);
}

/** @req SWS_I2c_00002 */
void test_I2c_BeforeInit_DeInit_ShouldReportUninit(void)
{
    Std_ReturnType ret = I2c_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_DEINIT, I2C_E_UNINIT);
}

/** @req SWS_I2c_00006 */
void test_I2c_BeforeInit_GetStatus_ShouldReturnUninitWithoutDet(void)
{
    TEST_ASSERT_EQUAL(I2C_UNINIT, I2c_GetStatus());
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00003 */
void test_I2c_BeforeInit_WriteBytes_Uninit_ShouldReportDet(void)
{
    uint8 buf[2U] = {0x11U, 0x22U};

    Std_ReturnType ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, buf, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEBYTES, I2C_E_UNINIT);
}

/** @req SWS_I2c_00004 */
void test_I2c_BeforeInit_ReadBytes_Uninit_ShouldReportDet(void)
{
    uint8 buf[2U] = {0U};

    Std_ReturnType ret = I2c_ReadBytes(0U, TEST_I2C_SLAVE_ADDR, buf, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_READBYTES, I2C_E_UNINIT);
}

/** @req SWS_I2c_00005 */
void test_I2c_BeforeInit_WriteRead_Uninit_ShouldReportDet(void)
{
    uint8 tx[2U] = {0x11U, 0x22U};
    uint8 rx[2U] = {0U};

    Std_ReturnType ret = I2c_WriteRead(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, rx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEREAD, I2C_E_UNINIT);
}

/** @req SWS_I2c_00007 */
void test_I2c_BeforeInit_GetVersionInfo_NullPtr_ShouldReportDet(void)
{
    I2c_GetVersionInfo(NULL_PTR);
    test_I2c_ExpectDet(I2C_SID_GETVERSIONINFO, I2C_E_PARAM_POINTER);
}

/** @req SWS_I2c_00008 */
void test_I2c_BeforeInit_SetClockMode_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_SetClockMode(0U, I2C_CLOCK_FAST);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETCLOCKMODE, I2C_E_UNINIT);
}

/** @req SWS_I2c_00009 */
void test_I2c_BeforeInit_EnableInterrupt_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_EnableInterrupt(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_ENABLEINTERRUPT, I2C_E_UNINIT);
}

/** @req SWS_I2c_00010 */
void test_I2c_BeforeInit_DisableInterrupt_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_DisableInterrupt(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_DISABLEINTERRUPT, I2C_E_UNINIT);
}

/** @req SWS_I2c_00011 */
void test_I2c_BeforeInit_SetSlaveAddress_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_SetSlaveAddress(0U, TEST_I2C_SLAVE_ADDR, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETSLAVEADDRESS, I2C_E_UNINIT);
}

/** @req SWS_I2c_00012 */
void test_I2c_BeforeInit_GetBusState_Uninit_ShouldReportDetAndReturnBusy(void)
{
    I2c_BusStateType state = I2c_GetBusState(0U);
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_BUSY, state);
    test_I2c_ExpectDet(I2C_SID_GETBUSSTATE, I2C_E_UNINIT);
}

/** @req SWS_I2c_00013 */
void test_I2c_BeforeInit_ClearBus_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_ClearBus(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_CLEARBUS, I2C_E_UNINIT);
}

/** @req SWS_I2c_00014 */
void test_I2c_BeforeInit_SoftwareReset_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_SoftwareReset(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SOFTWARERESET, I2C_E_UNINIT);
}

/** @req SWS_I2c_00015 */
void test_I2c_BeforeInit_SetTransferMode_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_SetTransferMode(0U, I2C_TRANSFER_INTERRUPT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETTRANSFERMODE, I2C_E_UNINIT);
}

/** @req SWS_I2c_00016 */
void test_I2c_BeforeInit_CancelTransfer_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret = I2c_CancelTransfer(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_CANCELTRANSFER, I2C_E_UNINIT);
}

/** @req SWS_I2c_00017 */
/** @req SWS_I2c_00018 */
/** @req SWS_I2c_00019 */
void test_I2c_BeforeInit_SlaveBufferApis_Uninit_ShouldReportDet(void)
{
    Std_ReturnType ret;

    ret = I2c_PrepareSlaveBuffer(0U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_PREPARESLAVEBUFFER, I2C_E_UNINIT);

    Det_Mock_Reset();
    ret = I2c_SlaveWriteBuffer(0U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SLAVEWRITEBUFFER, I2C_E_UNINIT);

    Det_Mock_Reset();
    ret = I2c_SlaveReadBuffer(0U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SLAVEREADBUFFER, I2C_E_UNINIT);
}

/** @req SWS_I2c_00020 */
void test_I2c_BeforeInit_MainFunction_ShouldReturnSilently(void)
{
    I2c_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Initialisation / de-initialisation
 *==========================================================================*/

/** @req SWS_I2c_00001 */
void test_I2c_Init_ShouldProgramChannelRegisters(void)
{
    test_I2c_EnsureInitialized();

    /* master channel on I2C1: I2CR = IEN only, IFDR = 15 (24MHz / 60 = 400kHz
     * exact for I2C_CLOCK_FAST), status cleared, no own address programmed */
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(15U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_I2SR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_IADR(TEST_I2C1_BASE)));

    /* slave channel on I2C2: same clock setup, own address 0x50 << 1 */
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL_UINT32(15U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0xA0U, MockRegisters_Read8(TEST_I2C_IADR(TEST_I2C2_BASE)));

    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_IDLE, I2c_GetBusState(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00001 */
void test_I2c_Init_DoubleInit_ShouldReportDetAndKeepRunning(void)
{
    test_I2c_EnsureInitialized();

    I2c_Init(&test_Config);
    test_I2c_ExpectDet(I2C_SID_INIT, I2C_E_ALREADY_INITIALIZED);

    /* the driver stays initialised */
    Det_Mock_Reset();
    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00001 */
void test_I2c_Init_PartialConfig_UnconfiguredChannelRejected(void)
{
    uint8 buf[2U] = {0x11U, 0x22U};

    test_I2c_EnsureInitialized();
    /* re-init with only channel 0 configured */
    (void)I2c_DeInit();
    test_I2c_SetupConfig(1U, I2C_TRANSFER_INTERRUPT);
    I2c_Init(&test_Config);
    Det_Mock_Reset();

    /* channel 1 is beyond NumChannels */
    Std_ReturnType ret = I2c_SetClockMode(1U, I2C_CLOCK_FAST);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETCLOCKMODE, I2C_E_PARAM_CHANNEL);

    Det_Mock_Reset();
    ret = I2c_WriteBytes(1U, TEST_I2C_SLAVE_ADDR, buf, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEBYTES, I2C_E_PARAM_CHANNEL);

    /* channel 0 still works */
    Det_Mock_Reset();
    TEST_ASSERT_EQUAL(E_OK, I2c_SetClockMode(0U, I2C_CLOCK_FAST));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00002 */
void test_I2c_DeInit_ShouldDisableChannelsAndAllowReinit(void)
{
    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL(I2C_UNINIT, I2c_GetStatus());
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* module is genuinely de-initialised */
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_BUSY, I2c_GetBusState(0U));
    test_I2c_ExpectDet(I2C_SID_GETBUSSTATE, I2C_E_UNINIT);

    /* driver can be initialised again */
    Det_Mock_Reset();
    test_I2c_SetupConfig(2U, I2C_TRANSFER_INTERRUPT);
    I2c_Init(&test_Config);
    TEST_ASSERT_EQUAL(I2C_IDLE, I2c_GetStatus());
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Transfer parameter validation (initialized driver)
 *==========================================================================*/

/** @req SWS_I2c_00003 */
void test_I2c_WriteBytes_ParameterValidation_ShouldReportDet(void)
{
    uint8 buf[4U] = {0x11U, 0x22U, 0x33U, 0x44U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    /* invalid channel */
    ret = I2c_WriteBytes(2U, TEST_I2C_SLAVE_ADDR, buf, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEBYTES, I2C_E_PARAM_CHANNEL);

    /* NULL buffer with non-zero length */
    Det_Mock_Reset();
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, NULL_PTR, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEBYTES, I2C_E_PARAM_POINTER);

    /* NULL pointer check precedes the length check */
    Det_Mock_Reset();
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, NULL_PTR, 300U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEBYTES, I2C_E_PARAM_POINTER);

    /* length beyond I2C_MAX_BUFFER_SIZE */
    Det_Mock_Reset();
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, buf, (I2C_MAX_BUFFER_SIZE + 1U), I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEBYTES, I2C_E_PARAM_LENGTH);
}

/** @req SWS_I2c_00004 */
void test_I2c_ReadBytes_ParameterValidation_ShouldReportDet(void)
{
    uint8 buf[4U] = {0U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    ret = I2c_ReadBytes(2U, TEST_I2C_SLAVE_ADDR, buf, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_READBYTES, I2C_E_PARAM_CHANNEL);

    Det_Mock_Reset();
    ret = I2c_ReadBytes(0U, TEST_I2C_SLAVE_ADDR, NULL_PTR, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_READBYTES, I2C_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = I2c_ReadBytes(0U, TEST_I2C_SLAVE_ADDR, buf, (I2C_MAX_BUFFER_SIZE + 1U), I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_READBYTES, I2C_E_PARAM_LENGTH);
}

/** @req SWS_I2c_00005 */
void test_I2c_WriteRead_ParameterValidation_ShouldReportDet(void)
{
    uint8 tx[4U] = {0x11U, 0x22U, 0x33U, 0x44U};
    uint8 rx[4U] = {0U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    ret = I2c_WriteRead(2U, TEST_I2C_SLAVE_ADDR, tx, 1U, rx, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEREAD, I2C_E_PARAM_CHANNEL);

    /* NULL Tx buffer with non-zero Tx length */
    Det_Mock_Reset();
    ret = I2c_WriteRead(0U, TEST_I2C_SLAVE_ADDR, NULL_PTR, 1U, rx, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEREAD, I2C_E_PARAM_POINTER);

    /* NULL Rx buffer with non-zero Rx length */
    Det_Mock_Reset();
    ret = I2c_WriteRead(0U, TEST_I2C_SLAVE_ADDR, tx, 1U, NULL_PTR, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEREAD, I2C_E_PARAM_POINTER);

    /* length beyond I2C_MAX_BUFFER_SIZE */
    Det_Mock_Reset();
    ret = I2c_WriteRead(0U, TEST_I2C_SLAVE_ADDR, tx, 1U, rx, (I2C_MAX_BUFFER_SIZE + 1U), I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_WRITEREAD, I2C_E_PARAM_LENGTH);
}

/*===========================================================================
 * Master transfers (interrupt mode, documented start failure)
 *==========================================================================*/

/** @req SWS_I2c_00003 */
void test_I2c_WriteBytes_Interrupt_StartFailure_ShouldReturnNotOk(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};

    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    /* SendStart cannot succeed against the static mock: after asserting
     * MSTA|MTX|IEN (0xB0) the "bus busy" poll times out, SendStop clears
     * MSTA again (0x90) and the transfer is reported as failed. */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_MTX),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00003 */
/** @req SWS_I2c_00014 */
void test_I2c_WriteBytes_ChannelBusy_RejectedUntilSoftwareReset(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    /* first transfer fails at START and leaves the channel in MASTER_TX */
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);

    /* a second transfer is rejected by the busy check BEFORE any register
     * access: I2CR keeps the value pre-set here */
    MockRegisters_Write32(TEST_I2C_I2CR(TEST_I2C1_BASE), TEST_I2C_IEN);
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* SoftwareReset restores IDLE state and the registers */
    TEST_ASSERT_EQUAL(E_OK, I2c_SoftwareReset(0U));
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_I2SR(TEST_I2C1_BASE)));

    /* the channel accepts a new transfer attempt (runs the full, failing,
     * START/STOP sequence again -> I2CR = 0x90) */
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_MTX),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00004 */
void test_I2c_ReadBytes_Interrupt_StartFailure_ShouldReturnNotOk(void)
{
    uint8 rx[2U] = {0U};

    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_ReadBytes(0U, TEST_I2C_SLAVE_ADDR, rx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_MTX),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* recover the channel for the following tests */
    TEST_ASSERT_EQUAL(E_OK, I2c_SoftwareReset(0U));
}

/** @req SWS_I2c_00005 */
void test_I2c_WriteRead_Interrupt_StartFailure_ShouldReturnNotOk(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};
    uint8 rx[2U] = {0U};

    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_WriteRead(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, rx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_MTX),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    TEST_ASSERT_EQUAL(E_OK, I2c_SoftwareReset(0U));
}

/** @req SWS_I2c_00016 */
void test_I2c_CancelTransfer_ShouldFreeBusyChannel(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    /* channel gets stuck in MASTER_TX after the failed START */
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);

    /* define a known I2CR value: a busy-rejected transfer must not touch it */
    MockRegisters_Write32(TEST_I2C_I2CR(TEST_I2C1_BASE), TEST_I2C_IEN);
    ret = I2c_CancelTransfer(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* CancelTransfer issues a STOP (clears MSTA, not set here) and resets
     * the channel state; no DET is reported */
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* the freed channel runs the START sequence again (I2CR -> 0x90),
     * proving the busy rejection is gone */
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_MTX),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Clock configuration
 *==========================================================================*/

/** @req SWS_I2c_00008 */
void test_I2c_SetClockMode_AllModes_ShouldProgramDividerRegister(void)
{
    test_I2c_EnsureInitialized();

    /* FAST: 24MHz / 60 = 400kHz exact -> table index 15 */
    TEST_ASSERT_EQUAL(E_OK, I2c_SetClockMode(0U, I2C_CLOCK_FAST));
    TEST_ASSERT_EQUAL_UINT32(15U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));

    /* STANDARD: closest divider 256 -> 93.75kHz -> table index 57 */
    TEST_ASSERT_EQUAL(E_OK, I2c_SetClockMode(0U, I2C_CLOCK_STANDARD));
    TEST_ASSERT_EQUAL_UINT32(57U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));

    /* FAST_PLUS: closest divider 30 -> 800kHz -> table index 0 */
    TEST_ASSERT_EQUAL(E_OK, I2c_SetClockMode(0U, I2C_CLOCK_FAST_PLUS));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));

    /* HIGH_SPEED: closest divider 30 -> 800kHz -> table index 0 */
    TEST_ASSERT_EQUAL(E_OK, I2c_SetClockMode(0U, I2C_CLOCK_HIGH_SPEED));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));

    /* per-channel addressing: channel 1 is on I2C2 */
    TEST_ASSERT_EQUAL(E_OK, I2c_SetClockMode(1U, I2C_CLOCK_STANDARD));
    TEST_ASSERT_EQUAL_UINT32(57U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));

    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00008 */
void test_I2c_SetClockMode_InvalidMode_ShouldReturnNotOkWithoutDet(void)
{
    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_SetClockMode(0U, (I2c_ClockModeType)0x99U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    /* IFDR untouched */
    TEST_ASSERT_EQUAL_UINT32(15U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00008 */
void test_I2c_SetClockMode_InvalidChannel_ShouldReportDet(void)
{
    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_SetClockMode(2U, I2C_CLOCK_FAST);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETCLOCKMODE, I2C_E_PARAM_CHANNEL);
}

/*===========================================================================
 * Interrupt control
 *==========================================================================*/

/** @req SWS_I2c_00009 */
/** @req SWS_I2c_00010 */
void test_I2c_EnableDisableInterrupt_ShouldToggleIIENPerChannel(void)
{
    test_I2c_EnsureInitialized();

    TEST_ASSERT_EQUAL(E_OK, I2c_EnableInterrupt(0U));
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_IIEN),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));

    TEST_ASSERT_EQUAL(E_OK, I2c_DisableInterrupt(0U));
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));

    /* channel 1 is controlled independently through the I2C2 base */
    TEST_ASSERT_EQUAL(E_OK, I2c_EnableInterrupt(1U));
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_IIEN),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));

    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    Det_Mock_Reset();
    TEST_ASSERT_EQUAL(E_NOT_OK, I2c_EnableInterrupt(2U));
    test_I2c_ExpectDet(I2C_SID_ENABLEINTERRUPT, I2C_E_PARAM_CHANNEL);

    Det_Mock_Reset();
    TEST_ASSERT_EQUAL(E_NOT_OK, I2c_DisableInterrupt(2U));
    test_I2c_ExpectDet(I2C_SID_DISABLEINTERRUPT, I2C_E_PARAM_CHANNEL);
}

/*===========================================================================
 * Slave address configuration
 *==========================================================================*/

/** @req SWS_I2c_00011 */
void test_I2c_SetSlaveAddress_MasterChannel_ShouldReturnNotOk(void)
{
    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_SetSlaveAddress(0U, 0x2AU, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    /* no register write on the master channel */
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_IADR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00011 */
void test_I2c_SetSlaveAddress_SlaveChannel7Bit_ShouldProgramIadr(void)
{
    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_SetSlaveAddress(1U, 0x2AU, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((0x2AU << 1), MockRegisters_Read8(TEST_I2C_IADR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* only 7-bit addressing is supported by the API */
    ret = I2c_SetSlaveAddress(1U, 0x1FFU, I2C_ADDR_MODE_10BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((0x2AU << 1), MockRegisters_Read8(TEST_I2C_IADR(TEST_I2C2_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* invalid channel */
    ret = I2c_SetSlaveAddress(2U, 0x2AU, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETSLAVEADDRESS, I2C_E_PARAM_CHANNEL);
}

/*===========================================================================
 * Bus state / bus recovery
 *==========================================================================*/

/** @req SWS_I2c_00012 */
void test_I2c_GetBusState_ShouldReflectIBBBitPerChannel(void)
{
    test_I2c_EnsureInitialized();

    TEST_ASSERT_EQUAL(I2C_BUS_STATE_IDLE, I2c_GetBusState(0U));
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_IDLE, I2c_GetBusState(1U));

    MockRegisters_Write32(TEST_I2C_I2SR(TEST_I2C1_BASE), TEST_I2C_IBB);
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_BUSY, I2c_GetBusState(0U));
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_IDLE, I2c_GetBusState(1U));

    MockRegisters_Write32(TEST_I2C_I2SR(TEST_I2C2_BASE), TEST_I2C_IBB);
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_BUSY, I2c_GetBusState(1U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* invalid channel */
    TEST_ASSERT_EQUAL(I2C_BUS_STATE_BUSY, I2c_GetBusState(2U));
    test_I2c_ExpectDet(I2C_SID_GETBUSSTATE, I2C_E_PARAM_CHANNEL);
}

/** @req SWS_I2c_00013 */
void test_I2c_ClearBus_ShouldToggleIenAndClockDataRegister(void)
{
    test_I2c_EnsureInitialized();

    Std_ReturnType ret = I2c_ClearBus(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* I2C disabled during the 9 clock pulses and re-enabled afterwards */
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    /* the nine 0x00/0xFF pairs end with 0xFF in the data register */
    TEST_ASSERT_EQUAL_UINT32(0xFFU, MockRegisters_Read8(TEST_I2C_I2DR(TEST_I2C1_BASE)));
    /* the clock divider is not touched */
    TEST_ASSERT_EQUAL_UINT32(15U, MockRegisters_Read8(TEST_I2C_IFDR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* invalid channel */
    ret = I2c_ClearBus(2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_CLEARBUS, I2C_E_PARAM_CHANNEL);
}

/** @req SWS_I2c_00014 */
void test_I2c_SoftwareReset_ShouldResetRegistersAndChannelState(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    /* leave the channel busy and the registers dirty */
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    MockRegisters_Write32(TEST_I2C_I2CR(TEST_I2C1_BASE), 0x9FU);
    MockRegisters_Write32(TEST_I2C_I2SR(TEST_I2C1_BASE), 0xFFU);

    ret = I2c_SoftwareReset(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(TEST_I2C_IEN, MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read8(TEST_I2C_I2SR(TEST_I2C1_BASE)));

    /* channel state was reset: a new transfer attempt is dispatched again
     * (it fails at START as before, I2CR -> 0x90) */
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32((TEST_I2C_IEN | TEST_I2C_MTX),
                             MockRegisters_Read8(TEST_I2C_I2CR(TEST_I2C1_BASE)));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* invalid channel */
    ret = I2c_SoftwareReset(2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SOFTWARERESET, I2C_E_PARAM_CHANNEL);
}

/*===========================================================================
 * Transfer mode configuration
 *==========================================================================*/

/** @req SWS_I2c_00015 */
void test_I2c_SetTransferMode_InvalidParameters_ShouldReportDet(void)
{
    test_I2c_EnsureInitialized();

    /* mode beyond I2C_TRANSFER_DMA */
    Std_ReturnType ret = I2c_SetTransferMode(0U, (I2c_TransferModeType)(I2C_TRANSFER_DMA + 1));
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETTRANSFERMODE, I2C_E_PARAM_MODE);

    /* invalid channel */
    Det_Mock_Reset();
    ret = I2c_SetTransferMode(2U, I2C_TRANSFER_INTERRUPT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SETTRANSFERMODE, I2C_E_PARAM_CHANNEL);
}

/** @req SWS_I2c_00015 */
void test_I2c_SetTransferMode_ValidModes_ShouldBeAppliedToChannel(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};

    test_I2c_EnsureInitialized();

    /* re-init with POLLING channels: a master transfer in polling mode would
     * hang the host forever in the internal busy-wait loop */
    (void)I2c_DeInit();
    test_I2c_SetupConfig(2U, I2C_TRANSFER_POLLING);
    I2c_Init(&test_Config);
    Det_Mock_Reset();

    TEST_ASSERT_EQUAL(E_OK, I2c_SetTransferMode(0U, I2C_TRANSFER_INTERRUPT));

    /* this call RETURNS only because the channel now uses the interrupt
     * path; in polling mode the driver would spin on State == MASTER_TX
     * forever (see I2c_WriteBytes polling wait loop) */
    Std_ReturnType ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);

    /* DMA mode is supported (I2C_DMA_SUPPORTED = STD_ON) and also returns */
    TEST_ASSERT_EQUAL(E_OK, I2c_SoftwareReset(0U));
    TEST_ASSERT_EQUAL(E_OK, I2c_SetTransferMode(0U, I2C_TRANSFER_DMA));
    ret = I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 1U, I2C_ADDR_MODE_7BIT);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Slave buffer APIs
 *==========================================================================*/

/** @req SWS_I2c_00017 */
/** @req SWS_I2c_00018 */
/** @req SWS_I2c_00019 */
void test_I2c_SlaveBufferApis_ParameterValidation(void)
{
    uint8 buf[4U] = {0U};
    Std_ReturnType ret;

    test_I2c_EnsureInitialized();

    /* invalid channel */
    ret = I2c_PrepareSlaveBuffer(2U, buf, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_PREPARESLAVEBUFFER, I2C_E_PARAM_CHANNEL);

    Det_Mock_Reset();
    ret = I2c_SlaveWriteBuffer(2U, buf, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SLAVEWRITEBUFFER, I2C_E_PARAM_CHANNEL);

    Det_Mock_Reset();
    ret = I2c_SlaveReadBuffer(2U, buf, 1U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SLAVEREADBUFFER, I2C_E_PARAM_CHANNEL);

    /* NULL buffer with non-zero length */
    Det_Mock_Reset();
    ret = I2c_PrepareSlaveBuffer(1U, NULL_PTR, 2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_PREPARESLAVEBUFFER, I2C_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = I2c_SlaveWriteBuffer(1U, NULL_PTR, 2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SLAVEWRITEBUFFER, I2C_E_PARAM_POINTER);

    Det_Mock_Reset();
    ret = I2c_SlaveReadBuffer(1U, NULL_PTR, 2U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    test_I2c_ExpectDet(I2C_SID_SLAVEREADBUFFER, I2C_E_PARAM_POINTER);

    /* valid calls succeed silently */
    Det_Mock_Reset();
    TEST_ASSERT_EQUAL(E_OK, I2c_PrepareSlaveBuffer(1U, buf, 4U));
    TEST_ASSERT_EQUAL(E_OK, I2c_SlaveWriteBuffer(1U, buf, 4U));
    TEST_ASSERT_EQUAL(E_OK, I2c_SlaveReadBuffer(1U, buf, 4U));
    /* NULL with zero length is explicitly allowed */
    TEST_ASSERT_EQUAL(E_OK, I2c_PrepareSlaveBuffer(1U, NULL_PTR, 0U));
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/*===========================================================================
 * Main function / version info
 *==========================================================================*/

/** @req SWS_I2c_00020 */
void test_I2c_MainFunction_ShouldNotFireNotificationsWithoutTransferredData(void)
{
    uint8 tx[2U] = {0x12U, 0x34U};

    test_I2c_EnsureInitialized();

    I2c_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, test_TxNotifyCount);
    TEST_ASSERT_EQUAL_UINT32(0U, test_RxNotifyCount);
    TEST_ASSERT_EQUAL_UINT32(0U, test_ErrNotifyCount);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* a failed master transfer transferred no data (index 0), so the main
     * function must not call any notification either */
    TEST_ASSERT_EQUAL(E_NOT_OK, I2c_WriteBytes(0U, TEST_I2C_SLAVE_ADDR, tx, 2U, I2C_ADDR_MODE_7BIT));
    I2c_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, test_TxNotifyCount);
    TEST_ASSERT_EQUAL_UINT32(0U, test_RxNotifyCount);
    TEST_ASSERT_EQUAL_UINT32(0U, test_ErrNotifyCount);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

/** @req SWS_I2c_00007 */
void test_I2c_GetVersionInfo_ValidPtr_ShouldFillVersionFields(void)
{
    Std_VersionInfoType info;

    test_I2c_EnsureInitialized();
    I2c_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(I2C_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT32(I2C_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT32(I2C_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT32(I2C_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT32(I2C_SW_PATCH_VERSION, info.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
}

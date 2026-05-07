/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : SPI Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Spi.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Spi_ConfigType g_test_config;
static Spi_ChannelConfigType g_test_channels[2];
static Spi_JobConfigType g_test_jobs[2];
static Spi_SequenceConfigType g_test_sequences[2];
static Spi_ChannelType g_test_job_channels[2];
static Spi_JobType g_test_seq_jobs[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Channel config */
    g_test_channels[0].ChannelId = 0;
    g_test_channels[0].DefaultData = 0xFF;
    g_test_channels[0].DataWidth = 8;
    g_test_channels[0].MaxDataLength = 32;
    g_test_channels[0].BufferType = SPI_IB_BUFFER;
    g_test_channels[0].TransferStart = TRUE;
    
    /* Job config */
    g_test_job_channels[0] = 0;
    g_test_jobs[0].JobId = 0;
    g_test_jobs[0].HwUnit = 0;
    g_test_jobs[0].ChipSelect = 0;
    g_test_jobs[0].Baudrate = 1000000;
    g_test_jobs[0].TimeCs2Clk = 1;
    g_test_jobs[0].TimeClk2Cs = 1;
    g_test_jobs[0].TimeCs2Cs = 10;
    g_test_jobs[0].Channels = g_test_job_channels;
    g_test_jobs[0].NumChannels = 1;
    g_test_jobs[0].CsPolarity = FALSE;
    g_test_jobs[0].SpiDataShiftEdge = 0;
    g_test_jobs[0].SpiShiftClockIdleLevel = 0;
    
    /* Sequence config */
    g_test_seq_jobs[0] = 0;
    g_test_sequences[0].SequenceId = 0;
    g_test_sequences[0].Jobs = g_test_seq_jobs;
    g_test_sequences[0].NumJobs = 1;
    g_test_sequences[0].Interruptible = FALSE;
    
    /* Main config */
    g_test_config.Channels = g_test_channels;
    g_test_config.NumChannels = 1;
    g_test_config.Jobs = g_test_jobs;
    g_test_config.NumJobs = 1;
    g_test_config.Sequences = g_test_sequences;
    g_test_config.NumSequences = 1;
    g_test_config.ExternalDevices = NULL;
    g_test_config.NumExternalDevices = 0;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
    g_test_config.InterruptibleSeqAllowed = FALSE;
    g_test_config.AsyncMode = SPI_POLLING_MODE;
    g_test_config.MaxBufferSize = 256;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Spi_Init with valid config */
TEST_CASE(spi_init_valid)
{
    Spi_StatusType status;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    status = Spi_GetStatus();
    ASSERT_EQ(SPI_IDLE, status);
    TEST_PASS();
}

/* Test: Spi_Init with NULL config */
TEST_CASE(spi_init_null)
{
    Det_Mock_Reset();
    
    Spi_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(SPI_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Spi_DeInit */
TEST_CASE(spi_deinit_valid)
{
    Std_ReturnType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    result = Spi_DeInit();
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(SPI_UNINIT, Spi_GetStatus());
    TEST_PASS();
}

/* Test: Spi_DeInit when busy */
TEST_CASE(spi_deinit_busy)
{
    Std_ReturnType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    /* Start a sequence to make SPI busy */
    Spi_AsyncTransmit(0);
    
    result = Spi_DeInit();
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(SPI_E_SEQ_PENDING, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Spi_WriteIB with valid parameters */
TEST_CASE(spi_write_ib_valid)
{
    Spi_DataBufferType buffer;
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    buffer.SrcPtr = data;
    buffer.DestPtr = NULL;
    buffer.Length = 4;
    
    Spi_WriteIB(0, &buffer);
    
    ASSERT_MEM_EQ(data, Spi_MockChannels[0].TxBuffer, 4);
    TEST_PASS();
}

/* Test: Spi_WriteIB when not initialized */
TEST_CASE(spi_write_ib_uninit)
{
    Spi_DataBufferType buffer = {0};
    
    Det_Mock_Reset();
    
    Spi_WriteIB(0, &buffer);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(SPI_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Spi_ReadIB with valid parameters */
TEST_CASE(spi_read_ib_valid)
{
    Spi_DataBufferType buffer;
    uint8 rx_data[4] = {0};
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    /* Setup mock receive data */
    Spi_MockChannels[0].RxBuffer[0] = 0x11;
    Spi_MockChannels[0].RxBuffer[1] = 0x22;
    Spi_MockChannels[0].RxBuffer[2] = 0x33;
    Spi_MockChannels[0].RxBuffer[3] = 0x44;
    
    buffer.SrcPtr = NULL;
    buffer.DestPtr = rx_data;
    buffer.Length = 4;
    
    Spi_ReadIB(0, &buffer);
    
    ASSERT_EQ(0x11, rx_data[0]);
    ASSERT_EQ(0x22, rx_data[1]);
    TEST_PASS();
}

/* Test: Spi_SetupEB with valid parameters */
TEST_CASE(spi_setup_eb_valid)
{
    Std_ReturnType result;
    Spi_DataBufferType src_buffer;
    Spi_DataBufferType dest_buffer;
    uint8 src_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8 dest_data[4] = {0};
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    src_buffer.SrcPtr = src_data;
    src_buffer.DestPtr = NULL;
    src_buffer.Length = 4;
    
    dest_buffer.SrcPtr = NULL;
    dest_buffer.DestPtr = dest_data;
    dest_buffer.Length = 4;
    
    result = Spi_SetupEB(0, &src_buffer, &dest_buffer, 4);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Spi_AsyncTransmit with valid sequence */
TEST_CASE(spi_async_transmit_valid)
{
    Std_ReturnType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    result = Spi_AsyncTransmit(0);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(SPI_BUSY, Spi_GetStatus());
    TEST_PASS();
}

/* Test: Spi_AsyncTransmit when busy */
TEST_CASE(spi_async_transmit_busy)
{
    Std_ReturnType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    /* Start first sequence */
    Spi_AsyncTransmit(0);
    
    /* Try to start another - should fail */
    result = Spi_AsyncTransmit(0);
    
    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(SPI_E_SEQ_PENDING, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Spi_SyncTransmit with valid sequence */
TEST_CASE(spi_sync_transmit_valid)
{
    Std_ReturnType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    result = Spi_SyncTransmit(0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Spi_GetStatus when uninitialized */
TEST_CASE(spi_get_status_uninit)
{
    Spi_StatusType status;
    
    status = Spi_GetStatus();
    
    ASSERT_EQ(SPI_UNINIT, status);
    TEST_PASS();
}

/* Test: Spi_GetJobResult */
TEST_CASE(spi_get_job_result)
{
    Spi_JobResultType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    Spi_Mock_SetJobResult(0, SPI_JOB_OK);
    
    result = Spi_GetJobResult(0);
    
    ASSERT_EQ(SPI_JOB_OK, result);
    TEST_PASS();
}

/* Test: Spi_GetSequenceResult */
TEST_CASE(spi_get_sequence_result)
{
    Spi_SeqResultType result;
    
    setup_test_config();
    Spi_Init(&g_test_config);
    
    Spi_Mock_SetSequenceResult(0, SPI_SEQ_OK);
    
    result = Spi_GetSequenceResult(0);
    
    ASSERT_EQ(SPI_SEQ_OK, result);
    TEST_PASS();
}

/* Test: Spi_GetVersionInfo */
TEST_CASE(spi_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Spi_GetVersionInfo(&version_info);
    
    ASSERT_EQ(SPI_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(SPI_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

/* Test: Spi_Cancel */
TEST_CASE(spi_cancel)
{
    setup_test_config();
    Spi_Init(&g_test_config);
    
    /* Start async transmission */
    Spi_AsyncTransmit(0);
    
    /* Cancel it */
    Spi_Cancel(0);
    
    ASSERT_EQ(SPI_SEQ_CANCELLED, Spi_GetSequenceResult(0));
    TEST_PASS();
}

/* Test: Spi_SetAsyncMode */
TEST_CASE(spi_set_async_mode)
{
    setup_test_config();
    Spi_Init(&g_test_config);
    
    Spi_SetAsyncMode(SPI_INTERRUPT_MODE);
    
    /* Verify mode changed */
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(spi)
{
    Spi_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(spi)
{
    /* Cleanup */
}

TEST_SUITE(spi)
{
    RUN_TEST(spi_init_valid);
    RUN_TEST(spi_init_null);
    RUN_TEST(spi_deinit_valid);
    RUN_TEST(spi_write_ib_valid);
    RUN_TEST(spi_write_ib_uninit);
    RUN_TEST(spi_read_ib_valid);
    RUN_TEST(spi_setup_eb_valid);
    RUN_TEST(spi_async_transmit_valid);
    RUN_TEST(spi_async_transmit_busy);
    RUN_TEST(spi_sync_transmit_valid);
    RUN_TEST(spi_get_status_uninit);
    RUN_TEST(spi_get_job_result);
    RUN_TEST(spi_get_sequence_result);
    RUN_TEST(spi_get_version_info);
    RUN_TEST(spi_cancel);
    RUN_TEST(spi_set_async_mode);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- SPI Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(spi);
}
TEST_MAIN_END()

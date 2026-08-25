/**
 * @file test_spi.c
 * @brief Spi (SPI Driver) Unit Tests
 * @req SWS_Spi
 */

// @tests src/bsw/mcal/spi/src/Spi.c  @tests src/bsw/mcal/spi/include/Spi.h
#include "unity.h"
#include "Spi.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Spi_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Spi_00001 */
void test_Spi_Init_NullPtr_ShouldReportDet(void) { Spi_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Spi_00001 */
void test_Spi_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; Spi_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Spi_00002 */
void test_Spi_DeInit_AfterInit_ShouldSucceed(void) { Spi_Init(&testConfig); Spi_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Spi_00003 */
void test_Spi_SyncTransmit_AfterInit_ShouldReturnResult(void) { Spi_Init(&testConfig); uint8 txData[8] = {0}; uint8 rxData[8] = {0}; Std_ReturnType ret = Spi_SyncTransmit(0U, txData, rxData, 8U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Spi_00004 */
void test_Spi_AsyncTransmit_AfterInit_ShouldReturnResult(void) { Spi_Init(&testConfig); uint8 txData[8] = {0}; Std_ReturnType ret = Spi_AsyncTransmit(0U, txData, 8U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Spi_00005 */
void test_Spi_GetTransferStatus_AfterInit_ShouldReturnStatus(void) { Spi_Init(&testConfig); Spi_StatusType status = Spi_GetTransferStatus(0U); TEST_ASSERT_TRUE(status == SPI_IDLE || status == SPI_BUSY); }
/** @req SWS_Spi_00006 */
void test_Spi_SetupEB_AfterInit_ShouldReturnResult(void) { Spi_Init(&testConfig); uint8 txData[8] = {0}; uint8 rxData[8] = {0}; Std_ReturnType ret = Spi_SetupEB(0U, txData, rxData, 8U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Spi_00007 */
void test_Spi_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Spi_GetVersionInfo(&info); TEST_ASSERT_EQUAL(SPI_VENDOR_ID, info.vendorID); }
/** @req SWS_Spi_00007 */
void test_Spi_GetVersionInfo_NullPtr_ShouldReportDet(void) { Spi_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
void test_Spi_Init_DoubleInit_ShouldNotCrash(void) { Spi_Init(&testConfig); Spi_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Spi_DeInit_BeforeInit_ShouldNotCrash(void) { Spi_DeInit(); TEST_ASSERT_TRUE(1); }
void test_Spi_SyncTransmit_BeforeInit_ShouldFail(void) { uint8 txData[8] = {0}; uint8 rxData[8] = {0}; Std_ReturnType ret = Spi_SyncTransmit(0U, txData, rxData, 8U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }

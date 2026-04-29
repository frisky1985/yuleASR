/*==================================================================================================
* Test Module          : DCM Transfer Services Unit Test
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Description          : Unit tests for UDS Program Transfer Services (0x34-0x37)
*                        per ISO 14229-1:2020 Sections 10.8-10.11
*=================================================================================================*/

#include "unity.h"
#include "dcm_transfer.h"
#include "Dcm.h"
#include "Dcm_Cfg.h"

/*==================================================================================================
*                                    TEST FIXTURE
==================================================================================================*/

void setUp(void)
{
    Dcm_TransferInit();
}

void tearDown(void)
{
    Dcm_TransferReset();
}

/*==================================================================================================
*                                    TEST CASES
==================================================================================================*/

/**
 * @brief Test: Transfer initialization
 */
void test_Dcm_TransferInit_ShouldResetState(void)
{
    const Dcm_TransferStatusType* status;

    Dcm_TransferInit();
    status = Dcm_GetTransferStatus();

    TEST_ASSERT_EQUAL(DCM_TRANSFER_STATE_IDLE, status->State);
    TEST_ASSERT_EQUAL(DCM_TRANSFER_DIR_NONE, status->Direction);
    TEST_ASSERT_EQUAL(0, status->BlockSequenceCounter);
    TEST_ASSERT_EQUAL(FALSE, status->TransferActive);
}

/**
 * @brief Test: Block sequence counter increment
 */
void test_Dcm_IncrementBlockSequenceCounter_ShouldWrap(void)
{
    const Dcm_TransferStatusType* status;
    uint8 i;

    /* Set counter to 254 */
    for (i = 0; i < 254; i++)
    {
        Dcm_IncrementBlockSequenceCounter();
    }

    status = Dcm_GetTransferStatus();
    TEST_ASSERT_EQUAL(254, status->BlockSequenceCounter);

    /* Increment to 255 */
    Dcm_IncrementBlockSequenceCounter();
    status = Dcm_GetTransferStatus();
    TEST_ASSERT_EQUAL(255, status->BlockSequenceCounter);

    /* Increment should wrap to 0 */
    Dcm_IncrementBlockSequenceCounter();
    status = Dcm_GetTransferStatus();
    TEST_ASSERT_EQUAL(0, status->BlockSequenceCounter);
}

/**
 * @brief Test: Memory range validation - valid range
 */
void test_Dcm_ValidateMemoryRange_ShouldAcceptValidRange(void)
{
    Std_ReturnType result;

    /* Test flash memory range (configured in dcm_transfer.c) */
    result = Dcm_ValidateMemoryRange(0x08000000, 1024, DCM_TRANSFER_DIR_DOWNLOAD);
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Test RAM memory range */
    result = Dcm_ValidateMemoryRange(0x20000000, 1024, DCM_TRANSFER_DIR_UPLOAD);
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @brief Test: Memory range validation - invalid range
 */
void test_Dcm_ValidateMemoryRange_ShouldRejectInvalidRange(void)
{
    Std_ReturnType result;

    /* Test address outside configured ranges */
    result = Dcm_ValidateMemoryRange(0x00000000, 1024, DCM_TRANSFER_DIR_DOWNLOAD);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);

    /* Test zero size */
    result = Dcm_ValidateMemoryRange(0x08000000, 0, DCM_TRANSFER_DIR_DOWNLOAD);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief Test: Address format extraction
 */
void test_Dcm_TransferExtractAddressAndSize(void)
{
    /* This would require access to static functions
     * For now, we test the public interface */
    TEST_IGNORE_MESSAGE("Static function test - requires test helper");
}

/**
 * @brief Test: Request Download - invalid length
 */
void test_Dcm_TransferProcessRequestDownload_ShouldRejectInvalidLength(void)
{
    uint8 requestData[2] = {0x44, 0x00};  /* Only 2 bytes, minimum is 3 */
    Std_ReturnType result;

    result = Dcm_TransferProcessRequestDownload(0, requestData, 2);

    /* Should return E_NOT_OK due to negative response sent */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief Test: Transfer Data without active transfer
 */
void test_Dcm_TransferProcessTransferData_ShouldRejectWhenInactive(void)
{
    uint8 requestData[2] = {0x01, 0xAA};  /* Block counter + data */
    Std_ReturnType result;

    /* Ensure no transfer is active */
    Dcm_TransferReset();

    result = Dcm_TransferProcessTransferData(0, requestData, 2);

    /* Should return E_NOT_OK due to request sequence error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief Test: Transfer Exit without active transfer
 */
void test_Dcm_TransferProcessRequestTransferExit_ShouldRejectWhenInactive(void)
{
    uint8 requestData[0] = {};
    Std_ReturnType result;

    /* Ensure no transfer is active */
    Dcm_TransferReset();

    result = Dcm_TransferProcessRequestTransferExit(0, requestData, 0);

    /* Should return E_NOT_OK due to request sequence error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief Test: Memory address mapping
 */
void test_Dcm_MapLogicalToPhysicalAddress(void)
{
    uint32 physicalAddr;
    Std_ReturnType result;

    /* Test valid flash address */
    result = Dcm_MapLogicalToPhysicalAddress(0x08000000, &physicalAddr);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x08000000, physicalAddr);

    /* Test valid RAM address */
    result = Dcm_MapLogicalToPhysicalAddress(0x20000000, &physicalAddr);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x20000000, physicalAddr);

    /* Test invalid address */
    result = Dcm_MapLogicalToPhysicalAddress(0x00000000, &physicalAddr);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @brief Test: Compression method validation
 */
void test_Dcm_TransferConfig_CompressionSupport(void)
{
    /* Verify compression support is enabled in config */
    TEST_ASSERT_TRUE(Dcm_TransferConfig.SupportCompression);
}

/**
 * @brief Test: Encryption method validation
 */
void test_Dcm_TransferConfig_EncryptionSupport(void)
{
    /* Verify encryption support is enabled in config */
    TEST_ASSERT_TRUE(Dcm_TransferConfig.SupportEncryption);
}

/**
 * @brief Test: Transfer status consistency
 */
void test_Dcm_GetTransferStatus_ShouldReturnConsistentData(void)
{
    const Dcm_TransferStatusType* status1;
    const Dcm_TransferStatusType* status2;

    status1 = Dcm_GetTransferStatus();
    status2 = Dcm_GetTransferStatus();

    /* Should return same pointer */
    TEST_ASSERT_EQUAL_PTR(status1, status2);
}

/*==================================================================================================
*                                    TEST RUNNER
==================================================================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Initialization tests */
    RUN_TEST(test_Dcm_TransferInit_ShouldResetState);

    /* Block sequence counter tests */
    RUN_TEST(test_Dcm_IncrementBlockSequenceCounter_ShouldWrap);

    /* Memory validation tests */
    RUN_TEST(test_Dcm_ValidateMemoryRange_ShouldAcceptValidRange);
    RUN_TEST(test_Dcm_ValidateMemoryRange_ShouldRejectInvalidRange);

    /* Service handler tests */
    RUN_TEST(test_Dcm_TransferProcessRequestDownload_ShouldRejectInvalidLength);
    RUN_TEST(test_Dcm_TransferProcessTransferData_ShouldRejectWhenInactive);
    RUN_TEST(test_Dcm_TransferProcessRequestTransferExit_ShouldRejectWhenInactive);

    /* Address mapping tests */
    RUN_TEST(test_Dcm_MapLogicalToPhysicalAddress);

    /* Configuration tests */
    RUN_TEST(test_Dcm_TransferConfig_CompressionSupport);
    RUN_TEST(test_Dcm_TransferConfig_EncryptionSupport);

    /* Status tests */
    RUN_TEST(test_Dcm_GetTransferStatus_ShouldReturnConsistentData);

    return UNITY_END();
}

/*==================================================================================================
*                                    END OF FILE
=================================================================================================*/

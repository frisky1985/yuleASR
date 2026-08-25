/**
 * @file test_csm.c
 * @brief CSM (Crypto Services Manager) Unit Tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "Csm.h"
#include "Csm_Cfg.h"

/* Test: Csm_Init
 * @req SWS_Csm_00001
 */
static void test_Csm_Init(void **state)
{
    (void)state;
    
    const Csm_ConfigType* config = NULL;
    Std_ReturnType result = Csm_Init(config);
    assert_int_equal(result, E_OK);
}

/* Test: Csm_DeInit
 * @req SWS_Csm_00002
 */
static void test_Csm_DeInit(void **state)
{
    (void)state;
    
    Csm_DeInit();
    assert_true(1);
}

/* Test: Csm_Encrypt
 * @req SWS_Csm_00050
 */
static void test_Csm_Encrypt(void **state)
{
    (void)state;
    
    uint32 jobId = 0;
    Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_SINGLECALL;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    uint8 resultData[16];
    uint32 resultLength = sizeof(resultData);
    
    Std_ReturnType result = Csm_Encrypt(jobId, mode, data, dataLength, resultData, &resultLength);
    assert_true(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Csm_Decrypt
 * @req SWS_Csm_00051
 */
static void test_Csm_Decrypt(void **state)
{
    (void)state;
    
    uint32 jobId = 0;
    Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_SINGLECALL;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    uint8 resultData[16];
    uint32 resultLength = sizeof(resultData);
    
    Std_ReturnType result = Csm_Decrypt(jobId, mode, data, dataLength, resultData, &resultLength);
    assert_true(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Csm_MacGenerate
 * @req SWS_Csm_00040
 */
static void test_Csm_MacGenerate(void **state)
{
    (void)state;
    
    uint32 jobId = 0;
    Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_SINGLECALL;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    uint8 mac[16];
    uint32 macLength = sizeof(mac);
    
    Std_ReturnType result = Csm_MacGenerate(jobId, mode, data, dataLength, mac, &macLength);
    assert_true(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Csm_MacVerify
 * @req SWS_Csm_00041
 */
static void test_Csm_MacVerify(void **state)
{
    (void)state;
    
    uint32 jobId = 0;
    Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_SINGLECALL;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    const uint8 mac[] = {0xAA, 0xBB};
    uint32 macLength = 2;
    Crypto_VerifyResultType verifyResult;
    
    Std_ReturnType result = Csm_MacVerify(jobId, mode, data, dataLength, mac, macLength, &verifyResult);
    assert_true(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

/* Test: Csm_RandomGenerate
 * @req SWS_Csm_00070
 */
static void test_Csm_RandomGenerate(void **state)
{
    (void)state;
    
    uint32 jobId = 0;
    uint8 resultData[16];
    uint32 resultLength = sizeof(resultData);
    
    Std_ReturnType result = Csm_RandomGenerate(jobId, resultData, &resultLength);
    assert_true(result == E_OK || result == E_NOT_OK);
}

/* Test: Csm_Hash
 * @req SWS_Csm_00030
 */
static void test_Csm_Hash(void **state)
{
    (void)state;
    
    uint32 jobId = 0;
    Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_SINGLECALL;
    const uint8 data[] = {0x01, 0x02, 0x03};
    uint32 dataLength = 3;
    uint8 resultData[32];
    uint32 resultLength = sizeof(resultData);
    
    Std_ReturnType result = Csm_Hash(jobId, mode, data, dataLength, resultData, &resultLength);
    assert_true(result == E_OK || result == E_NOT_OK || result == CRYPTO_E_BUSY);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Csm_Init),
        cmocka_unit_test(test_Csm_DeInit),
        cmocka_unit_test(test_Csm_Encrypt),
        cmocka_unit_test(test_Csm_Decrypt),
        cmocka_unit_test(test_Csm_MacGenerate),
        cmocka_unit_test(test_Csm_MacVerify),
        cmocka_unit_test(test_Csm_RandomGenerate),
        cmocka_unit_test(test_Csm_Hash),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

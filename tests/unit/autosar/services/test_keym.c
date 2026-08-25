/*==================================================================================================
 *                                KEY MANAGER (KeyM) - TEST
 *==================================================================================================
 * FILENAME: test_keym.c
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Simple test for Key Manager module
 *==================================================================================================
 */

// @tests src/bsw/services/keym/src/KeyM.c  @tests src/bsw/services/keym/include/KeyM.h

#include <stdio.h>
#include <string.h>
#include "KeyM.h"
#include "KeyM_Cfg.h"

/* Mock configuration */
static KeyM_KeyConfigType testKeyConfigs[KEYM_NUM_KEYS] = {
    {0, KEYM_KEY_TYPE_AES, 32, 4, 1, 0, 0xFF},
    {1, KEYM_KEY_TYPE_AES, 16, 4, 1, 0, 0xFF},
    {2, KEYM_KEY_TYPE_AES, 32, 4, 1, 0, 0xFF},
    {3, KEYM_KEY_TYPE_HMAC, 32, 4, 1, 0, 0xFF},
    {4, KEYM_KEY_TYPE_RSA, 256, 4, 1, 0, 0xFF},
    {5, KEYM_KEY_TYPE_ECC, 32, 4, 1, 0, 0xFF},
    {6, KEYM_KEY_TYPE_GENERIC, 32, 4, 1, 0, 0xFF},
    {7, KEYM_KEY_TYPE_GENERIC, 32, 4, 1, 0, 0xFF}
};

static KeyM_ConfigType testConfig = {
    testKeyConfigs,
    KEYM_NUM_KEYS,
    KEYM_MAX_KEY_ELEMENTS,
    TRUE,  /* asyncOperations */
    TRUE,  /* keyStorageNvM */
    KEYM_MAX_KEY_LIFETIME
};

/* Test data */
static const uint8 testKeyData[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

static void print_test_result(const char* testName, Std_ReturnType result) {
    printf("  %s: %s\n", testName, (result == E_OK) ? "PASSED" : "FAILED");
}

static void run_basic_tests(void) {
    Std_ReturnType result;
    uint8 outputBuffer[KEYM_MAX_KEY_LENGTH];
    uint32 outputLength;
    KeyM_KeyStatusType keyStatus;
    uint32 keyVersion;
    KeyM_KeyInfoType keyInfo;
    
    printf("\n=== KeyM Basic Tests ===\n");
    
    /* Test 1: Initialize */
    printf("\n1. Testing KeyM_Init...\n");
    KeyM_Init(&testConfig);
    printf("  KeyM initialized: %s\n", KeyM_Initialized ? "TRUE" : "FALSE");
    
    /* Test 2: Set Key */
    printf("\n2. Testing KeyM_SetKey...\n");
    result = KeyM_SetKey(KEYM_KEY_ID_AES_128, testKeyData, 16, KEYM_KEY_FORMAT_RAW);
    print_test_result("Set key", result);
    
    /* Test 3: Get Key Status */
    printf("\n3. Testing KeyM_KeyStatusGet...\n");
    result = KeyM_KeyStatusGet(KEYM_KEY_ID_AES_128, &keyStatus);
    print_test_result("Get key status", result);
    printf("  Key status: %d (NEW=%d, UPDATE=%d, VALID=%d, INVALID=%d)\n",
           keyStatus, KEYM_KEY_STATUS_NEW, KEYM_KEY_STATUS_UPDATE, 
           KEYM_KEY_STATUS_VALID, KEYM_KEY_STATUS_INVALID);
    
    /* Test 4: Finalize Key */
    printf("\n4. Testing KeyM_FinalizeKey...\n");
    result = KeyM_FinalizeKey(KEYM_KEY_ID_AES_128);
    print_test_result("Finalize key", result);
    
    /* Check status after finalize */
    KeyM_KeyStatusGet(KEYM_KEY_ID_AES_128, &keyStatus);
    printf("  Key status after finalize: %d\n", keyStatus);
    
    /* Test 5: Get Key */
    printf("\n5. Testing KeyM_GetKey...\n");
    outputLength = sizeof(outputBuffer);
    result = KeyM_GetKey(KEYM_KEY_ID_AES_128, outputBuffer, &outputLength, NULL);
    print_test_result("Get key", result);
    printf("  Key length: %d bytes\n", (int)outputLength);
    printf("  Key data (first 8 bytes): %02X %02X %02X %02X %02X %02X %02X %02X\n",
           outputBuffer[0], outputBuffer[1], outputBuffer[2], outputBuffer[3],
           outputBuffer[4], outputBuffer[5], outputBuffer[6], outputBuffer[7]);
    
    /* Test 6: Get Key Version */
    printf("\n6. Testing KeyM_KeyVersionGet...\n");
    result = KeyM_KeyVersionGet(KEYM_KEY_ID_AES_128, &keyVersion);
    print_test_result("Get key version", result);
    printf("  Key version: %u\n", (unsigned int)keyVersion);
    
    /* Test 7: Get Key Info */
    printf("\n7. Testing KeyM_KeyInfoGet...\n");
    result = KeyM_KeyInfoGet(KEYM_KEY_ID_AES_128, &keyInfo);
    print_test_result("Get key info", result);
    printf("  Key ID: %u\n", (unsigned int)keyInfo.keyId);
    printf("  Key length: %u bits\n", (unsigned int)keyInfo.keyLength);
    printf("  Key version: %u\n", (unsigned int)keyInfo.keyVersion);
    
    /* Test 8: Key Element Set/Get */
    printf("\n8. Testing KeyM_KeyElementSet/Get...\n");
    result = KeyM_KeyElementSet(KEYM_KEY_ID_AES_128, 0, testKeyData, 8);
    print_test_result("Set key element", result);
    
    outputLength = sizeof(outputBuffer);
    result = KeyM_KeyElementGet(KEYM_KEY_ID_AES_128, 0, outputBuffer, &outputLength);
    print_test_result("Get key element", result);
    printf("  Element length: %d bytes\n", (int)outputLength);
    
    /* Test 9: Copy Key */
    printf("\n9. Testing KeyM_CopyKey...\n");
    result = KeyM_CopyKey(KEYM_KEY_ID_AES_128, KEYM_KEY_ID_AES_256);
    print_test_result("Copy key", result);
    
    /* Need to finalize copied key before getting */
    result = KeyM_FinalizeKey(KEYM_KEY_ID_AES_256);
    print_test_result("Finalize copied key", result);
    
    /* Verify copy */
    outputLength = sizeof(outputBuffer);
    result = KeyM_GetKey(KEYM_KEY_ID_AES_256, outputBuffer, &outputLength, NULL);
    print_test_result("Get copied key", result);
    printf("  Copied key length: %d bytes\n", (int)outputLength);
    
    /* Test 10: Update Key */
    printf("\n10. Testing KeyM_UpdateKey...\n");
    uint8 newKeyData[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
                            0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00};
    result = KeyM_UpdateKey(KEYM_KEY_ID_AES_128, newKeyData, 16, KEYM_KEY_FORMAT_RAW);
    print_test_result("Update key", result);
    
    /* Test 11: Parse Key */
    printf("\n11. Testing KeyM_ParseKey...\n");
    result = KeyM_ParseKey(KEYM_KEY_ID_HMAC_SHA256, testKeyData, 32, KEYM_KEY_FORMAT_RAW);
    print_test_result("Parse key", result);
    
    result = KeyM_FinalizeKey(KEYM_KEY_ID_HMAC_SHA256);
    print_test_result("Finalize parsed key", result);
    
    /* Test 12: Convert Key */
    printf("\n12. Testing KeyM_ConvertKey...\n");
    /* Re-finalize the key after update */
    result = KeyM_FinalizeKey(KEYM_KEY_ID_AES_128);
    print_test_result("Finalize key after update", result);
    
    outputLength = sizeof(outputBuffer);
    result = KeyM_ConvertKey(KEYM_KEY_ID_AES_128, outputBuffer, &outputLength, KEYM_KEY_FORMAT_RAW);
    print_test_result("Convert key", result);
    
    /* Test 13: DeInit */
    printf("\n13. Testing KeyM_DeInit...\n");
    KeyM_DeInit();
    printf("  KeyM initialized after DeInit: %s\n", KeyM_Initialized ? "TRUE" : "FALSE");
    
    printf("\n=== All Tests Complete ===\n");
}

static void run_error_tests(void) {
    Std_ReturnType result;
    uint8 buffer[KEYM_MAX_KEY_LENGTH];
    uint32 length;
    KeyM_KeyStatusType status;
    
    printf("\n=== KeyM Error Handling Tests ===\n");
    
    /* Test uninitialized access */
    printf("\n1. Testing uninitialized access...\n");
    result = KeyM_SetKey(KEYM_KEY_ID_AES_128, testKeyData, 16, KEYM_KEY_FORMAT_RAW);
    printf("  SetKey without init: %s (expected: FAILED)\n", 
           (result == E_OK) ? "PASSED" : "FAILED");
    
    /* Re-initialize for remaining tests */
    KeyM_Init(&testConfig);
    
    /* Test invalid key ID */
    printf("\n2. Testing invalid key ID...\n");
    result = KeyM_SetKey(KEYM_NUM_KEYS + 1, testKeyData, 16, KEYM_KEY_FORMAT_RAW);
    printf("  SetKey with invalid ID: %s (expected: FAILED)\n",
           (result == E_OK) ? "PASSED" : "FAILED");
    
    /* Test null pointer */
    printf("\n3. Testing null pointer...\n");
    result = KeyM_SetKey(KEYM_KEY_ID_AES_128, NULL_PTR, 16, KEYM_KEY_FORMAT_RAW);
    printf("  SetKey with NULL pointer: %s (expected: FAILED)\n",
           (result == E_OK) ? "PASSED" : "FAILED");
    
    /* Test getting non-existent key */
    printf("\n4. Testing get non-existent key...\n");
    length = sizeof(buffer);
    result = KeyM_GetKey(KEYM_KEY_ID_MASTER, buffer, &length, NULL);
    printf("  GetKey without setting: %s (expected: FAILED)\n",
           (result == E_OK) ? "PASSED" : "FAILED");
    
    /* Test invalid buffer size */
    printf("\n5. Testing invalid buffer size...\n");
    KeyM_SetKey(KEYM_KEY_ID_AES_128, testKeyData, 32, KEYM_KEY_FORMAT_RAW);
    KeyM_FinalizeKey(KEYM_KEY_ID_AES_128);
    length = 8;  /* Too small */
    result = KeyM_GetKey(KEYM_KEY_ID_AES_128, buffer, &length, NULL);
    printf("  GetKey with small buffer: %s (expected: FAILED)\n",
           (result == E_OK) ? "PASSED" : "FAILED");
    
    KeyM_DeInit();
    printf("\n=== Error Tests Complete ===\n");
}

int main(void) {
    printf("\n========================================\n");
    printf("    KeyM (Key Manager) Module Test    \n");
    printf("========================================\n");
    
    run_basic_tests();
    run_error_tests();
    
    printf("\n========================================\n");
    printf("         All Tests Finished            \n");
    printf("========================================\n\n");
    
    return 0;
}

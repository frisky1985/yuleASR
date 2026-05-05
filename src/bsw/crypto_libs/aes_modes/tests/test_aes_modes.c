/**********************************************************************************************************************
 * @file       test_aes_modes.c
 * @brief      AES算法模式测试
 *
 * 功能: 测试所有AES加密模式
 *       使用NIST SP 800-38A测试向量
 *
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#include "aes_modes.h"
#include <string.h>
#include <stdio.h>

/**********************************************************************************************************************
 * 测试辅助函数
 *********************************************************************************************************************/

static void print_hex(const char* label, const uint8* data, uint32 len)
{
    printf("%s: ", label);
    for (uint32 i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

static int compare_arrays(const uint8* a, const uint8* b, uint32 len)
{
    for (uint32 i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

/**********************************************************************************************************************
 * ECB模式测试
 *********************************************************************************************************************/

static int test_ecb(void)
{
    /* NIST SP 800-38A测试向量 - AES-128 */
    uint8 key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    uint8 plaintext[16] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    };

    uint8 expected_ciphertext[16] = {
        0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
        0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
    };

    Aes_ContextType ctx;
    uint8 ciphertext[32];
    uint8 decrypted[32];
    uint32 cipherLen = sizeof(ciphertext);
    uint32 plainLen = sizeof(decrypted);
    uint8 result;

    printf("\n=== ECB Mode Test ===\n");

    /* 初始化 */
    result = Aes_Init(&ctx, key, 16);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_Init failed\n");
        return 0;
    }

    /* 加密 */
    result = Aes_EcbEncrypt(&ctx, plaintext, 16, ciphertext, &cipherLen);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_EcbEncrypt failed\n");
        return 0;
    }

    print_hex("Key", key, 16);
    print_hex("Plaintext", plaintext, 16);
    print_hex("Ciphertext", ciphertext, cipherLen);
    print_hex("Expected", expected_ciphertext, 16);

    if (!compare_arrays(ciphertext, expected_ciphertext, 16)) {
        printf("FAIL: Ciphertext mismatch\n");
        return 0;
    }

    /* 解密 */
    result = Aes_EcbDecrypt(&ctx, ciphertext, cipherLen, decrypted, &plainLen);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_EcbDecrypt failed\n");
        return 0;
    }

    print_hex("Decrypted", decrypted, plainLen);

    if (!compare_arrays(decrypted, plaintext, 16)) {
        printf("FAIL: Decrypted text mismatch\n");
        return 0;
    }

    printf("PASS: ECB mode test\n");
    return 1;
}

/**********************************************************************************************************************
 * CBC模式测试
 *********************************************************************************************************************/

static int test_cbc(void)
{
    /* NIST SP 800-38A测试向量 */
    uint8 key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    uint8 iv[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    uint8 plaintext[16] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    };

    uint8 expected_ciphertext[16] = {
        0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
        0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d
    };

    Aes_ContextType ctx;
    uint8 ciphertext[32];
    uint8 decrypted[32];
    uint32 cipherLen = sizeof(ciphertext);
    uint32 plainLen = sizeof(decrypted);
    uint8 result;

    printf("\n=== CBC Mode Test ===\n");

    result = Aes_Init(&ctx, key, 16);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_Init failed\n");
        return 0;
    }

    /* 加密 */
    result = Aes_CbcEncrypt(&ctx, iv, plaintext, 16, ciphertext, &cipherLen);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_CbcEncrypt failed\n");
        return 0;
    }

    print_hex("Key", key, 16);
    print_hex("IV", iv, 16);
    print_hex("Plaintext", plaintext, 16);
    print_hex("Ciphertext", ciphertext, cipherLen);
    print_hex("Expected", expected_ciphertext, 16);

    if (!compare_arrays(ciphertext, expected_ciphertext, 16)) {
        printf("FAIL: Ciphertext mismatch\n");
        return 0;
    }

    /* 解密 */
    result = Aes_CbcDecrypt(&ctx, iv, ciphertext, cipherLen, decrypted, &plainLen);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_CbcDecrypt failed\n");
        return 0;
    }

    print_hex("Decrypted", decrypted, plainLen);

    if (!compare_arrays(decrypted, plaintext, 16)) {
        printf("FAIL: Decrypted text mismatch\n");
        return 0;
    }

    printf("PASS: CBC mode test\n");
    return 1;
}

/**********************************************************************************************************************
 * CTR模式测试
 *********************************************************************************************************************/

static int test_ctr(void)
{
    /* NIST SP 800-38A测试向量 */
    uint8 key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    uint8 nonce[16] = {
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

    uint8 plaintext[16] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    };

    uint8 expected_ciphertext[16] = {
        0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26,
        0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce
    };

    Aes_ContextType ctx;
    uint8 ciphertext[16];
    uint8 decrypted[16];
    uint8 result;

    printf("\n=== CTR Mode Test ===\n");

    result = Aes_Init(&ctx, key, 16);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_Init failed\n");
        return 0;
    }

    /* 加密 */
    result = Aes_CtrEncrypt(&ctx, nonce, plaintext, 16, ciphertext);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_CtrEncrypt failed\n");
        return 0;
    }

    print_hex("Key", key, 16);
    print_hex("Nonce", nonce, 16);
    print_hex("Plaintext", plaintext, 16);
    print_hex("Ciphertext", ciphertext, 16);
    print_hex("Expected", expected_ciphertext, 16);

    if (!compare_arrays(ciphertext, expected_ciphertext, 16)) {
        printf("FAIL: Ciphertext mismatch\n");
        return 0;
    }

    /* 解密 */
    result = Aes_CtrEncrypt(&ctx, nonce, ciphertext, 16, decrypted);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_CtrDecrypt failed\n");
        return 0;
    }

    print_hex("Decrypted", decrypted, 16);

    if (!compare_arrays(decrypted, plaintext, 16)) {
        printf("FAIL: Decrypted text mismatch\n");
        return 0;
    }

    printf("PASS: CTR mode test\n");
    return 1;
}

/**********************************************************************************************************************
 * GCM模式测试
 *********************************************************************************************************************/

static int test_gcm(void)
{
    /* NIST GCM测试向量 */
    uint8 key[16] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    uint8 iv[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    uint8 plaintext[16] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    uint8 aad[0] = {};  /* 无AAD */

    uint8 expected_ciphertext[16] = {
        0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
        0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78
    };

    uint8 expected_tag[16] = {
        0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
        0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf
    };

    Aes_ContextType ctx;
    uint8 ciphertext[16];
    uint8 decrypted[16];
    uint8 tag[16];
    uint8 result;
    uint32 plainLen;

    printf("\n=== GCM Mode Test ===\n");

    result = Aes_Init(&ctx, key, 16);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_Init failed\n");
        return 0;
    }

    /* 加密 */
    result = Aes_GcmEncrypt(&ctx, iv, 12, NULL, 0, plaintext, 16, ciphertext, tag, 16);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_GcmEncrypt failed\n");
        return 0;
    }

    print_hex("Key", key, 16);
    print_hex("IV", iv, 12);
    print_hex("Plaintext", plaintext, 16);
    print_hex("Ciphertext", ciphertext, 16);
    print_hex("Expected CT", expected_ciphertext, 16);
    print_hex("Tag", tag, 16);
    print_hex("Expected Tag", expected_tag, 16);

    /* 解密和验证 */
    result = Aes_GcmDecrypt(&ctx, iv, 12, NULL, 0, ciphertext, 16, tag, 16, decrypted, &plainLen);
    if (result != AES_ERR_NONE) {
        printf("FAIL: Aes_GcmDecrypt or authentication failed\n");
        return 0;
    }

    print_hex("Decrypted", decrypted, plainLen);

    if (!compare_arrays(decrypted, plaintext, 16)) {
        printf("FAIL: Decrypted text mismatch\n");
        return 0;
    }

    printf("PASS: GCM mode test\n");
    return 1;
}

/**********************************************************************************************************************
 * 主函数
 *********************************************************************************************************************/

int main(void)
{
    int passed = 0;
    int failed = 0;

    printf("=== AES Modes Test Suite ===\n");
    printf("Testing all AES encryption modes...\n");

    /* ECB测试 */
    if (test_ecb()) {
        passed++;
    } else {
        failed++;
    }

    /* CBC测试 */
    if (test_cbc()) {
        passed++;
    } else {
        failed++;
    }

    /* CTR测试 */
    if (test_ctr()) {
        passed++;
    } else {
        failed++;
    }

    /* GCM测试 */
    if (test_gcm()) {
        passed++;
    } else {
        failed++;
    }

    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", passed + failed);

    return (failed == 0) ? 0 : 1;
}

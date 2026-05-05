/**
 * @file mbedtls_config.h
 * @brief Mbed TLS Configuration for YuleTech AutoSAR - CCC (Car Connectivity Consortium)
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @author YuleTech AutoSAR Team
 * @version 1.0.0
 *
 * This configuration is optimized for UWB/BLE Digital Key (CCC) applications:
 * - ECDH P-256 for key agreement
 * - ECDSA P-256 for digital signatures
 * - AES-128-GCM for authenticated encryption
 * - HKDF for key derivation
 * - Hardware TRNG support
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* ============================================================================
 * System Configuration
 * ============================================================================ */

#define MBEDTLS_CONFIG_FILE "mbedtls_config.h"

/* Platform abstraction */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_PLATFORM_SNPRINTF_ALT

/* Memory allocation */
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C

/* ============================================================================
 * Core Cryptographic Primitives
 * ============================================================================ */

/* Message Digest - SHA256 required for CCC */
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_SMALLER          /* Reduce code size */

/* Symmetric Encryption - AES-128-GCM for CCC */
#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES          /* Use ROM tables to save RAM */
#define MBEDTLS_GCM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CIPHER_MODE_GCM

/* ============================================================================
 * Elliptic Curve Cryptography (ECC)
 * ============================================================================ */

/* ECC core - P-256 (secp256r1) for CCC */
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C

/* Enable only secp256r1 (NIST P-256) for CCC - saves code size */
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED

/* Disable other curves to reduce code size */
#undef MBEDTLS_ECP_DP_SECP192R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP224R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP384R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP521R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP192K1_ENABLED
#undef MBEDTLS_ECP_DP_SECP224K1_ENABLED
#undef MBEDTLS_ECP_DP_SECP256K1_ENABLED
#undef MBEDTLS_ECP_DP_BP256R1_ENABLED
#undef MBEDTLS_ECP_DP_BP384R1_ENABLED
#undef MBEDTLS_ECP_DP_BP512R1_ENABLED
#undef MBEDTLS_ECP_DP_CURVE25519_ENABLED
#undef MBEDTLS_ECP_DP_CURVE448_ENABLED

/* ============================================================================
 * Key Derivation - HKDF for CCC
 * ============================================================================ */

#define MBEDTLS_HKDF_C

/* ============================================================================
 * Random Number Generation
 * ============================================================================ */

#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT    /* Use hardware TRNG */
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_HMAC_DRBG_C

/* ============================================================================
 * PKI and X.509 (Minimal for CCC certificate handling)
 * ============================================================================ */

#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C

/* ============================================================================
 * Error Handling and Debugging
 * ============================================================================ */

#define MBEDTLS_ERROR_C

/* Debug (disable in production) */
// #define MBEDTLS_DEBUG_C

/* ============================================================================
 * Math Configuration
 * ============================================================================ */

/* Use fixed point for ECC operations - faster on embedded */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM

/* Internal elliptic point representation */
#define MBEDTLS_ECP_WINDOW_SIZE         4
#define MBEDTLS_ECP_MAX_BITS            256

/* ============================================================================
 * Size Optimizations - Disable unused features
 * ============================================================================ */

/* Disable deprecated features */
#undef MBEDTLS_SSL_CLI_C
#undef MBEDTLS_SSL_SRV_C
#undef MBEDTLS_SSL_TLS_C
#undef MBEDTLS_SSL_PROTO_TLS1_2

/* Disable unnecessary ciphers and modes */
#undef MBEDTLS_CIPHER_MODE_CBC
#undef MBEDTLS_CIPHER_MODE_CFB
#undef MBEDTLS_CIPHER_MODE_CTR
#undef MBEDTLS_CIPHER_MODE_OFB
#undef MBEDTLS_CIPHER_MODE_XTS
#undef MBEDTLS_CIPHER_PADDING_PKCS7
#undef MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS
#undef MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN
#undef MBEDTLS_CIPHER_PADDING_ZEROS

/* Disable DES - not needed for CCC */
#undef MBEDTLS_DES_C

/* Disable ARC4 - deprecated */
#undef MBEDTLS_ARC4_C

/* Disable MD5 - not needed for CCC */
#undef MBEDTLS_MD5_C

/* Disable SHA-1 - not needed for CCC */
#undef MBEDTLS_SHA1_C

/* Disable SHA-384/512 - not needed for CCC */
#undef MBEDTLS_SHA384_C
#undef MBEDTLS_SHA512_C

/* Disable RIPEMD - not needed */
#undef MBEDTLS_RIPEMD160_C

/* Disable MD2/MD4 - deprecated */
#undef MBEDTLS_MD2_C
#undef MBEDTLS_MD4_C

/* Disable Camellia - not needed */
#undef MBEDTLS_CAMELLIA_C

/* Disable Blowfish - not needed */
#undef MBEDTLS_BLOWFISH_C

/* Disable XTEA - not needed */
#undef MBEDTLS_XTEA_C

/* Disable CCM - use GCM for CCC */
#undef MBEDTLS_CCM_C

/* Disable CMAC - not needed for CCC */
#undef MBEDTLS_CMAC_C

/* Disable HMAC - use with HKDF only */
#define MBEDTLS_MD_C

/* Disable RSA - ECC only for CCC */
#undef MBEDTLS_RSA_C
#undef MBEDTLS_PKCS1_V15
#undef MBEDTLS_PKCS1_V21

/* Disable DSA - use ECDSA for CCC */
#undef MBEDTLS_DSA_C

/* Disable DHM - use ECDH for CCC */
#undef MBEDTLS_DHM_C

/* Disable key exchange algorithms not used in CCC */
#undef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED

/* Disable SSL features */
#undef MBEDTLS_SSL_EXPORT_KEYS
#undef MBEDTLS_SSL_SERVER_NAME_INDICATION
#undef MBEDTLS_SSL_ENCRYPT_THEN_MAC
#undef MBEDTLS_SSL_EXTENDED_MASTER_SECRET

/* ============================================================================
 * AutoSAR Specific Configuration
 * ============================================================================ */

/* Support for AUTOSAR Crypto Services */
#define MBEDTLS_AUTOSAR_CRYPTO_STACK_SIZE   4096
#define MBEDTLS_AUTOSAR_MAX_KEY_LENGTH      64
#define MBEDTLS_AUTOSAR_MAX_IV_LENGTH       16
#define MBEDTLS_AUTOSAR_MAX_AAD_LENGTH      256
#define MBEDTLS_AUTOSAR_MAX_TAG_LENGTH      16

/* Timing resistance for side-channel protection */
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_HAVE_TIME_DATE
#define MBEDTLS_TIMING_C
#define MBEDTLS_TIMING_ALT

/* ============================================================================
 * End of Configuration
 * ============================================================================ */

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */

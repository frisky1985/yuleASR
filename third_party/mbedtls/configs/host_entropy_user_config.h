/* Host-test user config (appended after yuleasr_config.h).
 * Enables MBEDTLS_ENTROPY_HARDWARE_ALT so mbedtls_entropy_func works on
 * macOS/Linux hosts. The hardware poll callback is provided by the test
 * harness (tests/mock/mbedtls_host_entropy.c, software fallback).
 * Production (bare-metal) builds do not set MBEDTLS_USER_CONFIG_FILE. */
#ifndef MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#endif

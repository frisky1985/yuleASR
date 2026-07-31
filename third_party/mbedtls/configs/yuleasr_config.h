/*==================================================================================================
 * yuleasr_config.h - mbedTLS configuration for yuleASR (S32K312 / bare-metal)
 *
 * Based on the mbedTLS 3.6.2 default configuration (full-feature), with
 * platform-specific adjustments for the bare-metal S32K312 target:
 *   - MBEDTLS_NO_PLATFORM_ENTROPY: entropy is provided by the platform TRNG
 *     (Crypto_HwTrng) rather than OS entropy sources
 *   - MBEDTLS_HAVE_TIME_DATE undef: no RTC on the bare-metal target; X.509
 *     validity is evaluated by the application (Mqtt_CertMgr)
 *   - MBEDTLS_TIMING_C undef: no OS timer; TLS I/O timeouts are driven by
 *     the MQTT state machine instead of mbedtls_timing_*
 *   - MBEDTLS_ALLOW_PRIVATE_ACCESS: the crypto/MQTT backends access mbedTLS
 *     internal structures (documented mbedTLS extension for integrators)
 *================================================================================================*/
#ifndef YULEASR_MBEDTLS_CONFIG_H
#define YULEASR_MBEDTLS_CONFIG_H

/* Start from the full-feature default configuration */
#include "mbedtls/mbedtls_config.h"

/* Bare-metal target: entropy comes from the platform TRNG, not the OS */
#ifndef MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_NO_PLATFORM_ENTROPY
#endif

/* No RTC on the target; certificate validity is checked by the application */
#undef MBEDTLS_HAVE_TIME_DATE
#undef MBEDTLS_HAVE_TIME

/* No OS timer; TLS timeouts are handled by the MQTT state machine */
#undef MBEDTLS_TIMING_C

/* Networking is provided by the TcpIp layer, not mbedtls_net_* */
#undef MBEDTLS_NET_C

/* Access to documented internal structures (required by the backends) */
#ifndef MBEDTLS_ALLOW_PRIVATE_ACCESS
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif

#endif /* YULEASR_MBEDTLS_CONFIG_H */

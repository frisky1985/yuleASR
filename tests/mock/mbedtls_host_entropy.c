/**
 * @file mbedtls_host_entropy.c
 * @brief Host (macOS/Linux) mbedTLS hardware entropy poll — test harness only
 *
 * The yuleASR mbedTLS config defines MBEDTLS_NO_PLATFORM_ENTROPY (bare-metal
 * TRNG), so mbedtls_entropy_func has no entropy source on host builds.  For
 * host unit tests the build enables MBEDTLS_ENTROPY_HARDWARE_ALT (see
 * third_party/mbedtls/configs/host_entropy_user_config.h) and links this
 * file, which provides a real OS entropy poll:
 *   - macOS:   arc4random_buf()
 *   - Linux:   getrandom() with /dev/urandom fallback
 *
 * This is TEST-ONLY glue — it is added to the mbedcrypto target only when
 * NOT cross-compiling (host test builds). Production firmware keeps using
 * the platform TRNG (Crypto_HwTrng).
 */

#include "mbedtls/entropy.h"

#if defined(__APPLE__)
#include <stdlib.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/random.h>
#endif

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;

    if ((output == NULL) || (olen == NULL) || (len == 0)) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

#if defined(__APPLE__)
    arc4random_buf(output, len);
    *olen = len;
    return 0;
#else
    /* Linux: getrandom() first, /dev/urandom fallback */
    ssize_t got = getrandom(output, len, 0);
    if (got > 0) {
        *olen = (size_t)got;
        return 0;
    }

    {
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }
        size_t total = 0;
        while (total < len) {
            ssize_t n = read(fd, output + total, len - total);
            if (n <= 0) {
                (void)close(fd);
                return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
            }
            total += (size_t)n;
        }
        (void)close(fd);
        *olen = total;
        return 0;
    }
#endif
}

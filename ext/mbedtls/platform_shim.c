/*
 * mbedtls platform overrides for targets that have no built-in entropy
 * source (or need an application-provided one).
 *
 * Win32 entropy poll adapted from Mbed TLS library/entropy_poll.c (2.x):
 *   Copyright The Mbed TLS Contributors
 *   SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * PSVita entropy and mbedtls_ms_time are original.
 *
 * Provides mbedtls_platform_get_entropy() for:
 *   - Windows XP: CryptGenRandom(), advapi32, works before Vista (bcrypt)
 *   - PSVita: sceKernelGetRandomNumber(), capped at 64 bytes per call
 * and, when MBEDTLS_PLATFORM_MS_TIME_ALT is set, mbedtls_ms_time().
 *
 * The configuration selects this file via MBEDTLS_PSA_DRIVER_GET_ENTROPY
 * instead of MBEDTLS_PSA_BUILTIN_GET_ENTROPY, see crypto_config.h.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "mbedtls/platform.h"
#include "psa/crypto.h"

#if defined(MBEDTLS_PLATFORM_MS_TIME_ALT)

#if defined(_WIN32)
#include <windows.h>

mbedtls_ms_time_t mbedtls_ms_time(void)
{
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);
    return ((mbedtls_ms_time_t) ft.dwLowDateTime +
            ((mbedtls_ms_time_t) ft.dwHighDateTime << 32)) / 10000;
}
#else
#include <time.h>

mbedtls_ms_time_t mbedtls_ms_time(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return (mbedtls_ms_time_t) time(NULL) * 1000;
    }
    return (mbedtls_ms_time_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
#endif

#endif /* MBEDTLS_PLATFORM_MS_TIME_ALT */

#if defined(MBEDTLS_PSA_DRIVER_GET_ENTROPY)

#if defined(_WIN32) && !defined(_WIN64)
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#include <wincrypt.h>

int mbedtls_platform_get_entropy(psa_driver_get_entropy_flags_t flags,
                                 size_t *estimate_bits,
                                 unsigned char *output, size_t output_size)
{
    HCRYPTPROV provider;

    if (flags != PSA_DRIVER_GET_ENTROPY_FLAGS_NONE) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    if (CryptAcquireContextW(&provider, NULL, NULL,
                             PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) == FALSE) {
        return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }

    if (CryptGenRandom(provider, (DWORD) output_size, output) == FALSE) {
        CryptReleaseContext(provider, 0);
        return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }

    CryptReleaseContext(provider, 0);
    *estimate_bits = 8 * output_size;
    return PSA_SUCCESS;
}

#elif defined(__vita__)
#include <psp2/kernel/rng.h>

int mbedtls_platform_get_entropy(psa_driver_get_entropy_flags_t flags,
                                 size_t *estimate_bits,
                                 unsigned char *output, size_t output_size)
{
    size_t done = 0;

    if (flags != PSA_DRIVER_GET_ENTROPY_FLAGS_NONE) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    /* the kernel rng takes at most 64 bytes per call */
    while (done < output_size) {
        size_t chunk = output_size - done;

        if (chunk > 64) {
            chunk = 64;
        }

        if (sceKernelGetRandomNumber(output + done, chunk) != 0) {
            return PSA_ERROR_INSUFFICIENT_ENTROPY;
        }
        done += chunk;
    }

    *estimate_bits = 8 * output_size;
    return PSA_SUCCESS;
}

#else
#error "MBEDTLS_PSA_DRIVER_GET_ENTROPY is enabled, but platform_shim.c has no entropy source for this platform"
#endif

#endif /* MBEDTLS_PSA_DRIVER_GET_ENTROPY */

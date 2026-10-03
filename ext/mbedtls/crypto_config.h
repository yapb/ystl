//
// ystl vendored TF-PSA-Crypto configuration (TLS 1.2-only HTTPS client).
//
// Used via TF_PSA_CRYPTO_CONFIG_FILE, replaces the default psa/crypto_config.h.
// Kept outside the mbedtls/ submodule so bumps don't conflict. Mirrors the old
// mbedtls_config.h crypto section: ECDHE_RSA/ECDHE_ECDSA, AES-GCM and
// ChaCha20-Poly1305, SHA-2, pure-C (no asm accelerators).
//
// SPDX-License-Identifier: Unlicense
//

#pragma once

#define TF_PSA_CRYPTO_CONFIG_VERSION 0x01000000

// hashes: SHA-384 is its own symbol (leaf/intermediate ECDSA chains)
#define PSA_WANT_ALG_SHA_256 1
#define PSA_WANT_ALG_SHA_384 1
#define PSA_WANT_ALG_SHA_512 1

// bulk crypto: AES-GCM + ChaCha20-Poly1305
#define PSA_WANT_KEY_TYPE_AES 1
#define PSA_WANT_ALG_GCM 1
#define PSA_WANT_ALG_ECB_NO_PADDING 1
#define PSA_WANT_KEY_TYPE_CHACHA20 1
#define PSA_WANT_ALG_CHACHA20_POLY1305 1

// key exchange / signatures: ECDHE with RSA/ECDSA peer certs
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE 1
#define PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY 1
#define PSA_WANT_ECC_SECP_R1_256 1
#define PSA_WANT_ECC_SECP_R1_384 1
#define PSA_WANT_ALG_ECDH 1
#define PSA_WANT_ALG_ECDSA 1
#define PSA_WANT_ALG_DETERMINISTIC_ECDSA 1
#define PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY 1
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC 1
#define PSA_WANT_ALG_RSA_PKCS1V15_SIGN 1
#define PSA_WANT_ALG_RSA_PSS 1

// tls 1.2 prf
#define PSA_WANT_ALG_TLS12_PRF 1
#define PSA_WANT_ALG_HMAC 1
#define PSA_WANT_KEY_TYPE_HMAC 1

// x509/pk parsing
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PEM_PARSE_C

// platform
#define MBEDTLS_PLATFORM_C

// psa core + random
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_CTR_DRBG_C

// entropy source: the built-in one (getrandom/bcrypt) where the platform has
// it, otherwise the custom callback from platform_shim.c (winxp, ps vita)
#if defined(YSTL_MBEDTLS_ENTROPY_DRIVER)
  #define MBEDTLS_PSA_DRIVER_GET_ENTROPY
#else
  #define MBEDTLS_PSA_BUILTIN_GET_ENTROPY
#endif

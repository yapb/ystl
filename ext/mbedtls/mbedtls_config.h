//
// ystl vendored mbedtls configuration (TLS 1.2-only HTTPS client).
//
// Used via MBEDTLS_CONFIG_FILE, replaces the default mbedtls_config.h.
// Crypto and platform options live in crypto_config.h next to this file
// (passed as TF_PSA_CRYPTO_CONFIG_FILE); the pinned submodule lives in
// mbedtls/ so submodule bumps don't conflict.
//
// SPDX-License-Identifier: Unlicense
//

#pragma once

#define MBEDTLS_CONFIG_VERSION 0x04000000

// ssl core: client, TLS 1.2, SNI (required by CDNs even with VERIFY_NONE)
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET
#define MBEDTLS_SSL_SESSION_TICKETS

// key exchanges: ephemeral (forward secret) with RSA/ECDSA certs
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

// peer cert parsing (public key needed for ServerKeyExchange check)
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C

// pem ca bundle loading for VERIFY_REQUIRED
#define MBEDTLS_PEM_PARSE_C

// errors
#define MBEDTLS_ERROR_C

// NOTE: TLS 1.3, server side, DTLS, debug, threading and x509 write
// stay off. Crypto mechanisms are configured in crypto_config.h.

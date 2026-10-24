#pragma once
#include <stddef.h>
typedef struct
{
    size_t bytes;
    int corrupt;
} mbedtls_sha256_context;
void mbedtls_sha256_init(mbedtls_sha256_context *);
int mbedtls_sha256_starts(mbedtls_sha256_context *, int);
int mbedtls_sha256_update(mbedtls_sha256_context *, const unsigned char *, size_t);
int mbedtls_sha256_finish(mbedtls_sha256_context *, unsigned char *);
void mbedtls_sha256_free(mbedtls_sha256_context *);

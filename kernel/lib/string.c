/*
 * NaznaOS - Minimal String/Memory Utilities Implementation Copyright (c) 2026 NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "lib/string.h"

size_t nazna_strlen(const char *s)
{
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

void *nazna_memset(void *dst, int c, size_t n)
{
    unsigned char *p = (unsigned char *)dst;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return dst;
}

void *nazna_memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

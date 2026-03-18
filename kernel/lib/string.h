/*
 * NaznaOS - Minimal String/Memory Utilities Interface Copyright (c) 2026 NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNA_LIB_STRING_H
#define NAZNA_LIB_STRING_H

#include "nazna/types.h"

size_t nazna_strlen(const char *s);
void  *nazna_memset(void *dst, int c, size_t n);
void  *nazna_memcpy(void *dst, const void *src, size_t n);

#endif /* NAZNA_LIB_STRING_H */

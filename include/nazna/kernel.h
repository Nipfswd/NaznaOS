/*
 * NaznaOS - Core Kernel Interfaces and Types
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNAOS_KERNEL_H
#define NAZNAOS_KERNEL_H

#include "nazna/types.h"

/* Kernel entry point (called from boot.s) */
void nazna_kernel_main(u32 multiboot_magic, u32 multiboot_info);

/* Kernel panic handler */
void nazna_panic(const char *msg);

#endif /* NAZNAOS_KERNEL_H */

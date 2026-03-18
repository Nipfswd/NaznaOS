/*
 * NaznaOS - Multiboot v1 Structures and Accessors
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNAOS_BOOT_MULTIBOOT_H
#define NAZNAOS_BOOT_MULTIBOOT_H

#include "nazna/types.h"

#define MULTIBOOT_MAGIC_BOOTLOADER 0x2BADB002

/* Multiboot info flags (subset used by NaznaOS) */
#define MULTIBOOT_FLAG_MEMINFO  (1 << 0)
#define MULTIBOOT_FLAG_MMAP     (1 << 6)

/* Multiboot v1 info structure (only fields we care about) */
struct multiboot_info {
    u32 flags;

    /* Valid if MULTIBOOT_FLAG_MEMINFO is set */
    u32 mem_lower;   /* in KiB */
    u32 mem_upper;   /* in KiB */

    u32 boot_device;
    u32 cmdline;
    u32 mods_count;
    u32 mods_addr;

    /* We ignore a.out and ELF section headers for now */
    u32 syms[4];

    /* Valid if MULTIBOOT_FLAG_MMAP is set */
    u32 mmap_length;
    u32 mmap_addr;

    /* Remaining fields are ignored for now */
};

/* Multiboot memory map entry */
struct multiboot_mmap_entry {
    u32 size;
    u64 addr;
    u64 len;
    u32 type;
} __attribute__((packed));

/* Multiboot memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE 1

/* Initialize Multiboot info handling.
 * magic: value from EAX
 * addr:  physical address of multiboot_info structure (from EBX)
 */
void multiboot_init(u32 magic, u32 addr);

/* Get pointer to the Multiboot info structure (or NULL if not initialized). */
const struct multiboot_info *multiboot_get_info(void);

#endif /* NAZNAOS_BOOT_MULTIBOOT_H */

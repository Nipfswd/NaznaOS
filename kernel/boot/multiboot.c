/*
 * NaznaOS - Multiboot v1 Info Handling Implementation
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "boot/multiboot.h"
#include "nazna/kernel.h"
#include "drivers/vga.h"

static const struct multiboot_info *g_multiboot_info = 0;

void multiboot_init(u32 magic, u32 addr)
{
    if (magic != MULTIBOOT_MAGIC_BOOTLOADER) {
        nazna_panic("Multiboot magic mismatch in multiboot_init");
    }

    g_multiboot_info = (const struct multiboot_info *)addr;

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("Multiboot: flags=0x");
    char buf[9];
    u32 flags = g_multiboot_info->flags;
    for (int i = 7; i >= 0; --i) {
        u8 nibble = (flags >> (i * 4)) & 0xF;
        buf[7 - i] = (char)(nibble < 10 ? ('0' + nibble) : ('A' + (nibble - 10)));
    }
    buf[8] = '\0';
    vga_write_string(buf);
    vga_write_string("\n");
}

const struct multiboot_info *multiboot_get_info(void)
{
    return g_multiboot_info;
}

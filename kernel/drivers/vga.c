/*
 * NaznaOS - VGA Text Mode Console Implementation (x86, 80x25) Copyright (c) 2026 NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "drivers/vga.h"
#include "nazna/types.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((u16 *)0xB8000)

static u16 *vga_buffer = VGA_MEMORY;
static u8 vga_color = 0;
static u8 cursor_row = 0;
static u8 cursor_col = 0;

/* Minimal port I/O for VGA cursor control */
static inline void outb(u16 port, u8 value)
{
    __asm__ __volatile__("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline u8 vga_entry_color(vga_color_t fg, vga_color_t bg)
{
    return (u8)(fg | (bg << 4));
}

static inline u16 vga_entry(unsigned char uc, u8 color)
{
    return (u16)uc | ((u16)color << 8);
}

static void vga_update_cursor(void)
{
    u16 pos = (u16)(cursor_row * VGA_WIDTH + cursor_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

static void vga_scroll(void)
{
    if (cursor_row < VGA_HEIGHT)
        return;

    for (u32 y = 1; y < VGA_HEIGHT; ++y) {
        for (u32 x = 0; x < VGA_WIDTH; ++x) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] =
                vga_buffer[y * VGA_WIDTH + x];
        }
    }

    for (u32 x = 0; x < VGA_WIDTH; ++x) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', vga_color);
    }

    cursor_row = VGA_HEIGHT - 1;
}

void vga_init(void)
{
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_color = vga_entry_color(fg, bg);
}

void vga_put_char(char c)
{
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        vga_scroll();
        vga_update_cursor();
        return;
    }

    vga_buffer[cursor_row * VGA_WIDTH + cursor_col] =
        vga_entry((unsigned char)c, vga_color);

    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        vga_scroll();
    }

    vga_update_cursor();
}

void vga_write_string(const char *str)
{
    while (*str) {
        vga_put_char(*str++);
    }
}

void vga_clear(void)
{
    for (u32 y = 0; y < VGA_HEIGHT; ++y) {
        for (u32 x = 0; x < VGA_WIDTH; ++x) {
            vga_buffer[y * VGA_WIDTH + x] =
                vga_entry(' ', vga_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_update_cursor();
}

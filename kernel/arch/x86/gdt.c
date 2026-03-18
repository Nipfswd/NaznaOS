/*
 * NaznaOS - x86 Global Descriptor Table Implementation (32-bit Protected Mode)
 * Copyright (c) 2026 NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "nazna/types.h"
#include "gdt.h"

struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_middle;
    u8  access;
    u8  granularity;
    u8  base_high;
} __attribute__((packed));

struct gdt_ptr {
    u16 limit;
    u32 base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr   gdt_descriptor;

static void gdt_set_entry(int idx, u32 base, u32 limit, u8 access, u8 gran)
{
    gdt[idx].limit_low    = (u16)(limit & 0xFFFF);
    gdt[idx].base_low     = (u16)(base & 0xFFFF);
    gdt[idx].base_middle  = (u8)((base >> 16) & 0xFF);
    gdt[idx].access       = access;
    gdt[idx].granularity  = (u8)((limit >> 16) & 0x0F);
    gdt[idx].granularity |= gran & 0xF0;
    gdt[idx].base_high    = (u8)((base >> 24) & 0xFF);
}

extern void gdt_flush(u32 gdt_ptr);

__asm__ (
".global gdt_flush           \n"
"gdt_flush:                  \n"
"    mov 4(%esp), %eax       \n"
"    lgdt (%eax)             \n"
"    mov $0x10, %ax          \n"
"    mov %ax, %ds            \n"
"    mov %ax, %es            \n"
"    mov %ax, %fs            \n"
"    mov %ax, %gs            \n"
"    mov %ax, %ss            \n"
"    ljmp $0x08, $.flush2    \n"
".flush2:                    \n"
"    ret                     \n"
);

void gdt_init(void)
{
    gdt_descriptor.limit = sizeof(gdt) - 1;
    gdt_descriptor.base  = (u32)&gdt;

    gdt_set_entry(0, 0, 0, 0, 0);                /* Null descriptor */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);    /* Code segment */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);    /* Data segment */

    gdt_flush((u32)&gdt_descriptor);
}

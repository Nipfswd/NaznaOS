/*
 * NaznaOS - x86 Interrupt Descriptor Table Implementation (32-bit Protected Mode)
 * Copyright (c) 2026 NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "nazna/types.h"
#include "idt.h"

struct idt_entry {
    u16 base_low;
    u16 sel;
    u8  always0;
    u8  flags;
    u16 base_high;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u32 base;
} __attribute__((packed));

#define IDT_ENTRIES 256

extern void *isr_stub_table[];

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idt_descriptor;

static void idt_set_entry(u8 num, u32 base, u16 sel, u8 flags)
{
    idt[num].base_low  = (u16)(base & 0xFFFF);
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
    idt[num].base_high = (u16)((base >> 16) & 0xFFFF);
}

static inline void idt_flush(u32 idt_ptr_addr)
{
    __asm__ __volatile__("lidt (%0)" :: "r"(idt_ptr_addr));
}

void idt_init(void)
{
    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base  = (u32)&idt;

    /* Default all entries to a safe, non-present state */
    for (int i = 0; i < IDT_ENTRIES; ++i) {
        idt_set_entry((u8)i, 0, 0x08, 0x00);
    }

    /* CPU exceptions 0–31 */
    for (int i = 0; i < 32; ++i) {
        u32 base = (u32)isr_stub_table[i];
        idt_set_entry((u8)i, base, 0x08, 0x8E);
    }

    /* Hardware IRQs 32–47 (PIC remapped IRQ0–IRQ15) */
    for (int i = 32; i < 48; ++i) {
        u32 base = (u32)isr_stub_table[i];
        idt_set_entry((u8)i, base, 0x08, 0x8E);
    }

    idt_flush((u32)&idt_descriptor);
}

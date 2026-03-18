/*
 * NaznaOS - x86 8259A PIC Implementation (Master/Slave) Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "arch/x86/pic.h"

/* Minimal port I/O helpers (local to this translation unit) */
static inline void outb(u16 port, u8 value)
{
    __asm__ __volatile__("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pic_remap(u8 offset1, u8 offset2)
{
    /* Save masks */
    u8 a1 = inb(PIC1_DATA);
    u8 a2 = inb(PIC2_DATA);

    /* Start initialization sequence (cascade mode) */
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    /* Set vector offsets */
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);

    /* Tell Master PIC that there is a slave at IRQ2 (0000 0100) */
    outb(PIC1_DATA, 0x04);
    /* Tell Slave PIC its cascade identity (0000 0010) */
    outb(PIC2_DATA, 0x02);

    /* Set 8086/88 (MCS-80/85) mode */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* Restore saved masks */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_send_eoi(u8 irq)
{
    if (irq >= 8) {
        /* Slave PIC */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    /* Always notify master */
    outb(PIC1_COMMAND, PIC_EOI);
}

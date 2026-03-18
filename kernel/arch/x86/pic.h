/*
 * NaznaOS - x86 8259A PIC Interface (Master/Slave) Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNAOS_ARCH_X86_PIC_H
#define NAZNAOS_ARCH_X86_PIC_H

#include "nazna/types.h"

/* PIC I/O ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* End-of-interrupt command code */
#define PIC_EOI      0x20

/* Remap the PICs so that:
 *   IRQ0–IRQ7  -> offset1 .. offset1+7
 *   IRQ8–IRQ15 -> offset2 .. offset2+7
 * Typically offset1 = 0x20, offset2 = 0x28.
 */
void pic_remap(u8 offset1, u8 offset2);

/* Send EOI to PIC for a given IRQ number (0–15). */
void pic_send_eoi(u8 irq);

#endif /* NAZNAOS_ARCH_X86_PIC_H */

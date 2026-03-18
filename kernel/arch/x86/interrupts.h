/*
 * NaznaOS - x86 Interrupt Frame and Interface Definitions Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNAOS_ARCH_X86_INTERRUPTS_H
#define NAZNAOS_ARCH_X86_INTERRUPTS_H

#include "nazna/types.h"

/* Base vector for PIC-remapped IRQs */
#define IRQ_BASE 32

struct interrupt_frame {
    u32 ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 int_no, err_code;
    u32 eip, cs, eflags, useresp, ss;
};

void interrupts_init(void);
void interrupts_handle(struct interrupt_frame *frame);

#endif /* NAZNAOS_ARCH_X86_INTERRUPTS_H */

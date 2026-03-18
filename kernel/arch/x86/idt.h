/*
 * NaznaOS - x86 Interrupt Descriptor Table Interface (32-bit Protected Mode)
 * Copyright (c) 2026 NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNA_ARCH_X86_IDT_H
#define NAZNA_ARCH_X86_IDT_H

#include "nazna/types.h"

void idt_init(void);

#endif /* NAZNA_ARCH_X86_IDT_H */

/* NaznaOS - Multiboot v1 Boot Entry (x86 32-bit) */
/* Copyright (c) 2026 NoahBajsToa */
/* SPDX-License-Identifier: MIT */

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/*
 * Multiboot header:
 * - Placed in a dedicated .multiboot section
 * - Marked allocatable so it ends up in a loadable segment
 * - Linker script ensures this section is kept at the start of .text
 */
    .section .multiboot,"a"
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

    .section .text
    .global _start
    .extern nazna_kernel_main

_start:
    cli

    mov $stack_top, %esp

    # Multiboot v1: EAX = magic, EBX = multiboot info pointer
    push %ebx
    push %eax

    call nazna_kernel_main

.hang:
    cli
    hlt
    jmp .hang

    .section .bss
    .align 16
stack_bottom:
    .skip 16384
stack_top:

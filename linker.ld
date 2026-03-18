/* NaznaOS - Linker Script (x86, Multiboot v1 kernel) */
/* Copyright (c) 2026 */
/* NoahBajsToa */
/* SPDX-License-Identifier: MIT */

ENTRY(_start)

SECTIONS
{
    /* Link the kernel to run at 1 MiB in virtual/physical memory */
    . = 1M;

    /* Mark kernel start symbol */
    _kernel_start = .;

    /* Place Multiboot header at the very start of .text so GRUB can find it */
    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot))
        *(.text*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    .data ALIGN(4K) :
    {
        *(.data*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss*)
    }

    /* Mark kernel end symbol */
    _kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}

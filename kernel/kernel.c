/*
 * NaznaOS - Kernel Entry and Initialization
 *
 * Responsibilities:
 *   - Entry point from Multiboot/GRUB.
 *   - Early console initialization and banner.
 *   - Multiboot info validation and basic reporting.
 *   - GDT/IDT/interrupts initialization.
 *   - Paging bootstrap + self-mapping setup.
 *   - PMM initialization from Multiboot memory map.
 *   - Paging self-test using dynamic page tables and PMM.
 *   - Enter idle loop with interrupts enabled.
 *
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "nazna/types.h"
#include "nazna/kernel.h"
#include "drivers/vga.h"
#include "boot/multiboot.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "arch/x86/interrupts.h"

/* Linker-provided symbols for kernel physical range. */
u32 _kernel_phys_start;
u32 _kernel_phys_end;

/* Panic handler: print message and halt the CPU forever.
 *
 * This is the canonical panic symbol used across the tree:
 *   - kernel/boot/multiboot.c
 *   - kernel/arch/x86/interrupts.c
 * expect nazna_panic().
 */
void nazna_panic(const char *msg)
{
    vga_write_string("KERNEL PANIC: ");
    vga_write_string(msg);
    vga_write_string("\n");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

static void kernel_idle_loop(void)
{
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

/* Kernel entry point expected by boot/boot.s:
 *   extern nazna_kernel_main
 */
void nazna_kernel_main(u32 multiboot_magic, u32 multiboot_info_addr)
{
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("NaznaOS Kernel\n");
    vga_write_string("Copyright (c) 2026 NoahBajsToa\n");

    /* Validate Multiboot magic using the constant defined in boot/multiboot.h. */
    if (multiboot_magic != MULTIBOOT_MAGIC_BOOTLOADER) {
        vga_write_string("Error: invalid Multiboot magic.\n");
        nazna_panic("Invalid Multiboot magic");
    }

    const struct multiboot_info *mbi = (const struct multiboot_info *)multiboot_info_addr;

    vga_write_string("Multiboot: flags present.\n");

    if (mbi->flags & MULTIBOOT_FLAG_MMAP) {
        vga_write_string("Multiboot: memory map present.\n");
    } else {
        vga_write_string("Multiboot: no memory map.\n");
    }

    /* Initialize GDT and IDT (exceptions only at this point). */
    gdt_init();
    idt_init();

    /* Initialize paging (bootstrap identity mapping + self-mapping). */
    paging_init();

    /* Initialize PMM from Multiboot memory map. */
    pmm_init(mbi);
    pmm_validate_state();

    /* Run paging self-test (dynamic mapping/unmapping). */
    paging_self_test();

    /* Initialize interrupts (PIC remap + IRQ handlers) and enable interrupts. */
    interrupts_init();

    vga_write_string("Initialization complete. Entering idle loop.\n");

    kernel_idle_loop();
}

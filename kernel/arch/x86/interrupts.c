/*
 * NaznaOS - x86 Interrupt Handling Implementation (CPU Exceptions 0–31, IRQs 32–47)
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#include "nazna/types.h"
#include "interrupts.h"
#include "drivers/vga.h"
#include "nazna/kernel.h"
#include "arch/x86/pic.h"

static const char *exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void interrupts_init(void)
{
    /*
     * Bring up legacy PIC and enable interrupts safely:
     *  - Remap PIC so IRQ0–IRQ15 -> IDT vectors 32–47.
     *  - Optionally unmask only the timer IRQ (IRQ0) for now.
     *  - Enable interrupts globally (sti) once IDT and PIC are ready.
     */

    /* Remap PIC to 0x20–0x2F */
    pic_remap(0x20, 0x28);

    /* Unmask only IRQ0 (timer); keep others masked by default. */
    /* We do this by writing an interrupt mask where only bit 0 is 0. */
    __asm__ __volatile__(
        "movb $0xFE, %%al\n\t"  /* 1111 1110: unmask IRQ0, mask 1–7 */
        "outb %%al, %0\n\t"
        :
        : "N"(PIC1_DATA)
        : "al"
    );

    /* Mask all IRQs on the slave PIC for now. */
    __asm__ __volatile__(
        "movb $0xFF, %%al\n\t"
        "outb %%al, %0\n\t"
        :
        : "N"(PIC2_DATA)
        : "al"
    );

    /* Now that IDT and PIC are configured, enable interrupts. */
    __asm__ __volatile__("sti");
}

void interrupts_handle(struct interrupt_frame *frame)
{
    u32 int_no = frame->int_no;

    if (int_no < 32) {
        /* CPU exception */
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_write_string("CPU Exception: ");
        vga_write_string(exception_messages[int_no]);
        vga_write_string("\n");
        nazna_panic("Unhandled CPU exception");
        return;
    }

    if (int_no >= IRQ_BASE && int_no < IRQ_BASE + 16) {
        /* Hardware IRQ from PIC */
        u8 irq = (u8)(int_no - IRQ_BASE);

        /* For now, do not spam output on timer or other IRQs. */
        /* Future: hook device-specific handlers here. */

        /* Send EOI to PIC so it can deliver further interrupts. */
        pic_send_eoi(irq);
        return;
    }

    /* Any other vector is unexpected but not necessarily fatal. */
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_write_string("Unhandled interrupt vector: ");
    char buf[16];
    u32 num = int_no;
    int idx = 15;
    buf[idx--] = '\0';
    if (num == 0) {
        buf[idx] = '0';
    } else {
        while (num > 0 && idx >= 0) {
            buf[idx--] = (char)('0' + (num % 10));
            num /= 10;
        }
        idx++;
    }
    vga_write_string(&buf[idx]);
    vga_write_string("\n");
}

/* NaznaOS - x86 ISR/IRQ Stubs (32-bit) */
/* Copyright (c) 2026 NoahBajsToa */
/* SPDX-License-Identifier: MIT */

    .section .text
    .intel_syntax noprefix

    .extern interrupts_handle

/*
 * Macros for CPU exceptions:
 * - ISR_NOERR: exceptions without error code
 * - ISR_ERR:   exceptions with error code
 *
 * For no-error-code exceptions, we push a dummy 0 error code and the
 * interrupt number so that the stack layout matches struct interrupt_frame.
 */

    .macro ISR_NOERR n
    .global isr\n
isr\n:
    push 0
    push \n
    jmp isr_common
    .endm

    .macro ISR_ERR n
    .global isr\n
isr\n:
    push \n
    jmp isr_common
    .endm

/* CPU exceptions 0–31 */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

/*
 * IRQ stubs 32–47 (PIC remapped IRQ0–IRQ15).
 * These are all no-error-code interrupts, so we use the same pattern
 * as ISR_NOERR: push dummy error code 0, then the vector number.
 */

    .macro IRQ_STUB n
    .global irq\n
irq\n:
    push 0
    push \n
    jmp isr_common
    .endm

IRQ_STUB 32
IRQ_STUB 33
IRQ_STUB 34
IRQ_STUB 35
IRQ_STUB 36
IRQ_STUB 37
IRQ_STUB 38
IRQ_STUB 39
IRQ_STUB 40
IRQ_STUB 41
IRQ_STUB 42
IRQ_STUB 43
IRQ_STUB 44
IRQ_STUB 45
IRQ_STUB 46
IRQ_STUB 47

/*
 * Common ISR/IRQ handler entry point.
 * Stack on entry (top -> bottom) before pusha:
 *   int_no
 *   err_code
 *   eip
 *   cs
 *   eflags
 *   useresp
 *   ss
 */

isr_common:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push eax
    call interrupts_handle
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    iretd

/*
 * Table of ISR/IRQ stub addresses.
 * First 32 entries: isr0..isr31 (CPU exceptions)
 * Next 16 entries:  irq32..irq47 (hardware IRQs)
 */

    .global isr_stub_table
isr_stub_table:
    .long isr0
    .long isr1
    .long isr2
    .long isr3
    .long isr4
    .long isr5
    .long isr6
    .long isr7
    .long isr8
    .long isr9
    .long isr10
    .long isr11
    .long isr12
    .long isr13
    .long isr14
    .long isr15
    .long isr16
    .long isr17
    .long isr18
    .long isr19
    .long isr20
    .long isr21
    .long isr22
    .long isr23
    .long isr24
    .long isr25
    .long isr26
    .long isr27
    .long isr28
    .long isr29
    .long isr30
    .long isr31

    .long irq32
    .long irq33
    .long irq34
    .long irq35
    .long irq36
    .long irq37
    .long irq38
    .long irq39
    .long irq40
    .long irq41
    .long irq42
    .long irq43
    .long irq44
    .long irq45
    .long irq46
    .long irq47

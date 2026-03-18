# NaznaOS - Top-level Makefile (x86 32-bit, Multiboot v1)
# Copyright (c) 2026
# NoahBajsToa
# SPDX-License-Identifier: MIT

CROSS ?= i686-elf
CC    := $(CROSS)-gcc
AS    := $(CROSS)-gcc
LD    := $(CROSS)-gcc
OBJCOPY := $(CROSS)-objcopy

CFLAGS := -m32 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-stack-protector -fno-pic -fno-pie
CFLAGS += -Iinclude -Ikernel -Ikernel/arch/x86
LDFLAGS := -m32 -nostdlib -T linker.ld

BUILD_DIR := build

KERNEL_ELF := $(BUILD_DIR)/naznaos.bin

OBJS := \
	boot/boot.o \
	kernel/kernel.o \
	kernel/boot/multiboot.o \
	kernel/drivers/vga.o \
	kernel/lib/string.o \
	kernel/mm/paging.o \
	kernel/mm/pmm.o \
	kernel/arch/x86/gdt.o \
	kernel/arch/x86/idt.o \
	kernel/arch/x86/interrupts.o \
	kernel/arch/x86/isr.o \
	kernel/arch/x86/pic.o

.PHONY: all clean iso run

all: $(KERNEL_ELF)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

boot/boot.o: boot/boot.s
	$(AS) $(CFLAGS) -c $< -o $@

kernel/arch/x86/isr.o: kernel/arch/x86/isr.S
	$(AS) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BUILD_DIR) $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

iso: $(KERNEL_ELF)
	mkdir -p iso/boot/grub
	cp $(KERNEL_ELF) iso/boot/naznaos.bin
	printf 'set timeout=0\nset default=0\nmenuentry \"NaznaOS\" {\n  multiboot /boot/naznaos.bin\n  boot\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o naznaos.iso iso

run: iso
	qemu-system-i386 -cdrom naznaos.iso

clean:
	rm -rf $(BUILD_DIR) iso naznaos.iso $(OBJS)

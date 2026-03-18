/* NaznaOS - Multiboot entry and bootstrap paging
 *
 * Responsibilities:
 *   - Provide Multiboot header for GRUB.
 *   - Set up a minimal stack.
 *   - Install a bootstrap page directory and page table:
 *       - identity-map first 4 MiB (0x00000000 - 0x003FFFFF).
 *   - Enable paging with CR3 pointing to the bootstrap page directory.
 *   - Jump to nazna_kernel_main(multiboot_magic, multiboot_info_addr).
 */

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

    .section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

    .section .text
    .global _start
    .extern nazna_kernel_main

/* Bootstrap page directory and first page table.
 * These are placed by the linker in low memory (starting at 1 MiB),
 * and we use them as physical addresses before paging is enabled.
 */
    .extern paging_bootstrap_page_directory_phys

    .align 4096
page_directory:
    .long first_page_table | 0x003    /* Present | RW, identity-mapped PT */
    .rept 1023
    .long 0
    .endr

    .align 4096
first_page_table:
    /* Map 0x00000000 - 0x003FFFFF (first 4 MiB) identity. */
    .set i, 0
1:
    .if i < 1024
        .long (i * 0x1000) | 0x003    /* Present | RW */
        .set i, i + 1
        .goto 1
    .endif

/* Simple stack in .bss */
    .section .bss
    .align 16
stack_bottom:
    .skip 4096
stack_top:

    .section .text
_start:
    /* Set up stack */
    mov $stack_top, %esp

    /* Save Multiboot parameters:
     *  - %eax: multiboot magic
     *  - %ebx: multiboot info address
     */
    push %ebx        /* arg1: multiboot_info_addr */
    push %eax        /* arg0: multiboot_magic */

    /* Load CR3 with physical address of page_directory. */
    mov $page_directory, %eax
    mov %eax, %cr3

    /* Enable paging: set PG (bit 31) and PE (bit 0) in CR0. */
    mov %cr0, %eax
    or  $0x80000001, %eax
    mov %eax, %cr0

    /* At this point, paging is enabled with identity mapping of first 4 MiB.
     * Jump to C kernel entry: nazna_kernel_main(multiboot_magic, multiboot_info_addr).
     */
    call nazna_kernel_main

halt:
    cli
    hlt
    jmp halt

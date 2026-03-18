/* NaznaOS - Linker script
 *
 * Responsibilities:
 *   - Place the kernel at 1 MiB.
 *   - Ensure bootstrap paging structures (page_directory, first_page_table)
 *     reside in low memory (< 4 MiB), so they are identity-mapped at boot.
 */

ENTRY(_start)

SECTIONS
{
    /* Load kernel at 1 MiB physical. */
    . = 1M;

    .text ALIGN(4K) :
    {
        *(.multiboot)
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
        *(.bss*)
        *(COMMON)
    }
}

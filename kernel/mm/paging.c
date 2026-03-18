/*
 * NaznaOS - Paging
 *
 * Responsibilities:
 *   - Keep the bootstrap page directory active.
 *   - Provide self-mapped access to page tables.
 *   - Map/unmap pages dynamically using PMM for page tables.
 *   - Provide translation helpers and a small self-test.
 */

#include "nazna/types.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "nazna/kernel.h"

/* Local memset implementation to avoid relying on a global libc symbol. */
static void *paging_memset(void *s, int c, unsigned long n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

/* Panic handler from kernel core. */
void nazna_panic(const char *msg);

/* Physical address of the bootstrap page directory.
 * Used by PMM to reserve the frame and by paging to know the base.
 * Defined here and referenced via extern in pmm.c.
 */
u32 paging_bootstrap_page_directory_phys = 0;

/* Page directory self-mapping index (last entry). */
#define SELF_MAP_INDEX 1023

/* Read CR3 to discover the current page directory physical address. */
static u32 paging_read_cr3(void)
{
    u32 value;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(value));
    return value;
}

/* Get pointer to the current page directory via self-mapping. */
static u32 *paging_get_page_directory(void)
{
    /* Self-mapped page directory is at:
     *   (SELF_MAP_INDEX << 22) | (SELF_MAP_INDEX << 12)
     */
    return (u32 *)((SELF_MAP_INDEX << 22) | (SELF_MAP_INDEX << 12));
}

/* Get pointer to the page table for a given virtual address.
 * If create != 0 and the table does not exist, allocate a frame from PMM,
 * install it into the page directory, and zero it via the self-mapped view.
 */
static u32 *paging_get_page_table(u32 virt, int create)
{
    u32 *pd = paging_get_page_directory();
    u32 pd_index = virt >> 22;
    u32 pt_entry = pd[pd_index];

    if (!(pt_entry & PAGE_PRESENT)) {
        if (!create) {
            return 0;
        }

        /* Allocate a new page table frame from PMM. */
        u32 pt_phys = pmm_alloc_frame();
        if (!pt_phys) {
            nazna_panic("paging_get_page_table: out of frames");
        }

        /* Install into page directory with RW, present. */
        pd[pd_index] = pt_phys | PAGE_PRESENT | PAGE_RW;

        /* Now that the PDE is installed, the page table is visible via
         * the self-mapped address:
         *   (SELF_MAP_INDEX << 22) | (pd_index << 12)
         */
        u32 *pt = (u32 *)((SELF_MAP_INDEX << 22) | (pd_index << 12));

        /* Zero the new page table via its self-mapped virtual address. */
        paging_memset(pt, 0, PAGE_SIZE);

        return pt;
    }

    /* Existing page table: use its self-mapped address. */
    u32 *pt = (u32 *)((SELF_MAP_INDEX << 22) | (pd_index << 12));
    return pt;
}

void paging_init(void)
{
    /* Discover and store the bootstrap page directory physical address. */
    u32 cr3 = paging_read_cr3();
    paging_bootstrap_page_directory_phys = cr3 & ~0xFFFu;

    /* Install the self-mapping entry in the last PDE.
     * We assume the bootstrap page directory itself resides in low memory
     * and is identity-mapped, so we can access it via its physical address.
     */
    u32 *pd_phys_virt = (u32 *)paging_bootstrap_page_directory_phys;
    pd_phys_virt[SELF_MAP_INDEX] =
        paging_bootstrap_page_directory_phys | PAGE_PRESENT | PAGE_RW;

    /* Reload CR3 to ensure the new self-mapping PDE is visible. */
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(paging_bootstrap_page_directory_phys) : "memory");
}

/* Map a virtual page to a physical frame with flags. */
int paging_map_page(u32 virt, u32 phys, u32 flags)
{
    u32 *pt = paging_get_page_table(virt, 1);
    if (!pt) {
        return -1;
    }

    u32 pt_index = (virt >> 12) & 0x3FF;
    if (pt[pt_index] & PAGE_PRESENT) {
        /* Already mapped; for now, treat as error. */
        return -1;
    }

    pt[pt_index] = (phys & ~0xFFFu) | (flags & 0xFFFu) | PAGE_PRESENT;

    /* Invalidate TLB entry for this virtual address. */
    __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");

    return 0;
}

/* Unmap a virtual page.
 *
 * Ownership rule:
 *   - Always unmaps the virtual address.
 *   - If free_phys != 0, also frees the underlying physical frame via PMM.
 *   - If free_phys == 0, only unmaps it; the caller retains ownership.
 */
int paging_unmap_page(u32 virt, int free_phys)
{
    u32 *pt = paging_get_page_table(virt, 0);
    if (!pt) {
        return -1;
    }

    u32 pt_index = (virt >> 12) & 0x3FF;
    u32 entry = pt[pt_index];

    if (!(entry & PAGE_PRESENT)) {
        return -1;
    }

    u32 phys = entry & ~0xFFFu;

    pt[pt_index] = 0;

    __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");

    if (free_phys && phys != 0) {
        pmm_free_frame(phys);
    }

    return 0;
}

/* Query mapping: return physical address if mapped, 0 otherwise.
 * Optionally report presence via *present.
 */
u32 paging_get_phys(u32 virt, int *present)
{
    u32 *pt = paging_get_page_table(virt, 0);
    if (!pt) {
        if (present) {
            *present = 0;
        }
        return 0;
    }

    u32 pt_index = (virt >> 12) & 0x3FF;
    u32 entry = pt[pt_index];

    if (!(entry & PAGE_PRESENT)) {
        if (present) {
            *present = 0;
        }
        return 0;
    }

    if (present) {
        *present = 1;
    }

    return entry & ~0xFFFu;
}

/* Allocate a fresh frame via PMM and map it at 'virt' with 'flags'.
 * Returns the physical address of the allocated frame on success, 0 on failure.
 */
u32 paging_alloc_map_page(u32 virt, u32 flags)
{
    u32 phys = pmm_alloc_frame();
    if (!phys) {
        return 0;
    }

    if (paging_map_page(virt, phys, flags) != 0) {
        /* Mapping failed; free the frame and report failure. */
        pmm_free_frame(phys);
        return 0;
    }

    return phys;
}

/* Check if a virtual address is mapped. */
int paging_is_mapped(u32 virt)
{
    int present = 0;
    (void)paging_get_phys(virt, &present);
    return present;
}

/* Simple, non-destructive paging self-test.
 *
 * Steps:
 *   1. Choose a test virtual address.
 *   2. Allocate+map one page there.
 *   3. Verify paging_get_phys reports the same physical frame.
 *   4. Touch the memory.
 *   5. Unmap and free the frame.
 */
void paging_self_test(void)
{
    const u32 test_virt = 0x400000; /* 4 MiB, outside low identity region */

    u32 phys = paging_alloc_map_page(test_virt, PAGE_RW);
    if (!phys) {
        nazna_panic("paging_self_test: alloc_map failed");
    }

    int present = 0;
    u32 phys2 = paging_get_phys(test_virt, &present);
    if (!present || phys2 != phys) {
        nazna_panic("paging_self_test: translation mismatch");
    }

    /* Touch the page: write and read back. */
    volatile u32 *ptr = (volatile u32 *)test_virt;
    *ptr = 0xCAFEBABE;
    if (*ptr != 0xCAFEBABE) {
        nazna_panic("paging_self_test: memory access failed");
    }

    /* Unmap and free the frame via paging_unmap_page's ownership semantics. */
    if (paging_unmap_page(test_virt, 1) != 0) {
        nazna_panic("paging_self_test: unmap failed");
    }

    if (paging_is_mapped(test_virt)) {
        nazna_panic("paging_self_test: mapping still present");
    }
}

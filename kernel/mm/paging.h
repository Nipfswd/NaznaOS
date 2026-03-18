/*
 * NaznaOS - Paging Subsystem Interface (x86 32-bit, 4 KiB pages)
 *
 * Responsibilities:
 *   - Provide a minimal but real paging API for the kernel.
 *   - Maintain a single kernel page directory.
 *   - Allow dynamic mapping/unmapping of pages using PMM-backed frames.
 *   - Expose bootstrap paging structures so PMM can reserve them.
 *   - Use page-directory self-mapping so page tables are accessible
 *     regardless of their physical location.
 *
 * Limitations:
 *   - Only a single kernel address space is supported.
 *   - No user/supervisor separation beyond basic flags.
 *   - No page fault recovery; faults are still fatal (diagnostics will
 *     be added in a separate subsystem).
 *
 * Ownership rules:
 *   - Bootstrap paging frames (page directory and initial page tables)
 *     are statically allocated and permanently reserved by PMM.
 *   - Dynamic page table frames are allocated via PMM and owned by the
 *     paging subsystem; they are not reclaimed yet in this stage.
 *   - Data frames mapped via paging_alloc_map_page are owned by the
 *     caller; paging_unmap_page(..., free_phys=1) is only valid when
 *     the caller owns the frame and no other mapping references it.
 *
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNAOS_MM_PAGING_H
#define NAZNAOS_MM_PAGING_H

#include "nazna/types.h"

/* Page size for x86 32-bit paging. */
#define PAGE_SIZE 4096

/* Page table/directory entry flags (subset). */
#define PAGE_PRESENT   0x001
#define PAGE_RW        0x002
#define PAGE_USER      0x004
#define PAGE_PWT       0x008
#define PAGE_PCD       0x010
#define PAGE_ACCESSED  0x020
#define PAGE_DIRTY     0x040
#define PAGE_4MB       0x080
#define PAGE_GLOBAL    0x100

/* Self-mapping constants:
 *   - The page directory is mapped into itself at PDE index 1023.
 *   - The page directory is visible at virtual address 0xFFFFF000.
 *   - Page tables are visible at 0xFFC00000 + (PDE_INDEX * PAGE_SIZE).
 */
#define PAGING_PD_SELF_INDEX   1023
#define PAGING_PD_VIRT         0xFFFFF000U
#define PAGING_PT_BASE_VIRT    0xFFC00000U

/* Initialize basic paging:
 *   - Identity map low memory / kernel region using a bootstrap page directory
 *     and page tables.
 *   - Install self-mapping entry in the page directory.
 *   - Enable paging in CR0.
 *   - Keep the page directory active for later dynamic mappings.
 */
void paging_init(void);

/* Map a virtual page to a specific physical frame with given flags.
 *
 * Parameters:
 *   virt  - virtual address (must be page-aligned)
 *   phys  - physical address (must be page-aligned)
 *   flags - page flags (e.g., PAGE_PRESENT | PAGE_RW)
 *
 * Returns:
 *   0 on success, non-zero on failure (e.g., allocation failure).
 *
 * Notes:
 *   - If the corresponding page table does not exist, it will be allocated
 *     from PMM and zeroed.
 *   - Existing mappings at 'virt' will be overwritten.
 */
int paging_map_page(u32 virt, u32 phys, u32 flags);

/* Allocate a new physical frame via PMM and map it at 'virt' with 'flags'.
 *
 * Parameters:
 *   virt  - virtual address (must be page-aligned)
 *   flags - page flags
 *
 * Returns:
 *   Physical address of the allocated frame on success, 0 on failure.
 *
 * Ownership:
 *   - The caller owns the returned frame and is responsible for unmapping
 *     and freeing it (via paging_unmap_page(..., free_phys=1)).
 */
u32 paging_alloc_map_page(u32 virt, u32 flags);

/* Unmap a virtual page.
 *
 * Parameters:
 *   virt      - virtual address (page-aligned)
 *   free_phys - if non-zero, the backing physical frame will be freed via PMM;
 *               if zero, the frame remains allocated but unmapped.
 *
 * Returns:
 *   0 on success, non-zero if the page was not mapped.
 *
 * Ownership rules:
 *   - Use free_phys = 1 only when the caller owns the frame and no other
 *     mapping references it.
 *   - Paging will never free frames that belong to bootstrap paging
 *     structures or other reserved regions; such requests are ignored
 *     or rejected safely.
 */
int paging_unmap_page(u32 virt, int free_phys);

/* Get the physical address mapped for a given virtual address.
 *
 * Parameters:
 *   virt    - virtual address
 *   present - optional pointer to an int that will be set to 1 if the page
 *             is present, 0 otherwise (may be NULL).
 *
 * Returns:
 *   Physical address of the mapped frame (page-aligned) if present,
 *   or 0 if not mapped or not present.
 */
u32 paging_get_phys(u32 virt, int *present);

/* Check whether a virtual address is mapped and present. */
int paging_is_mapped(u32 virt);

/* Expose bootstrap paging structures so PMM can reserve them.
 * These return physical addresses under the identity-mapping assumption
 * valid during early boot.
 */
u32 paging_get_bootstrap_pd_phys(void);
u32 paging_get_bootstrap_pt_region_start(void);
u32 paging_get_bootstrap_pt_region_end(void);

/* Small, non-destructive paging self-test.
 * Verifies:
 *   - dynamic page table allocation from PMM via self-mapping
 *   - mapping/unmapping of a test virtual address
 *   - translation via paging_get_phys
 *   - basic read/write through the mapping
 *
 * The test uses a virtual address outside the bootstrap identity-mapped
 * region to exercise dynamic page tables.
 */
void paging_self_test(void);

#endif /* NAZNAOS_MM_PAGING_H */

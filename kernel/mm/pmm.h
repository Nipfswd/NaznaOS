/*
 * NaznaOS - Physical Memory Manager (PMM) Interface
 *
 * Responsibilities:
 *   - Track physical 4 KiB frames using a bitmap.
 *   - Distinguish between:
 *       * total managed frames (within PMM range and usable)
 *       * reserved/used frames
 *       * free frames
 *   - Provide allocation/free primitives for paging and other subsystems.
 *   - Reserve critical regions:
 *       * frame 0 / low memory
 *       * kernel image
 *       * bootstrap paging structures
 *       * Multiboot info and memory map region
 *
 * Ownership model:
 *   - Frames outside the managed range are invisible to PMM and not
 *     counted in any statistics.
 *   - Frames marked reserved are never handed out by PMM.
 *   - Frames allocated via pmm_alloc_frame are owned by the caller.
 *   - Paging metadata frames (page tables) are allocated via PMM but
 *     conceptually owned by the paging subsystem.
 *
 * Limitations:
 *   - Single global PMM instance.
 *   - No NUMA or per-node awareness.
 *
 * Copyright (c) 2026
 * NoahBajsToa
 * SPDX-License-Identifier: MIT
 */

#ifndef NAZNAOS_MM_PMM_H
#define NAZNAOS_MM_PMM_H

#include "nazna/types.h"
#include "boot/multiboot.h"

/* Frame size used by PMM and paging. */
#define PMM_FRAME_SIZE 4096

/* Initialize PMM from Multiboot memory map.
 *
 * Parameters:
 *   mbi - pointer to Multiboot info structure provided by GRUB.
 *
 * Effects:
 *   - Builds a bitmap of managed frames from usable memory regions.
 *   - Reserves:
 *       * frame 0 / low memory
 *       * kernel image range
 *       * bootstrap paging structures
 *       * Multiboot info + memory map region
 *   - Computes total/used/free frame counts.
 */
void pmm_init(const struct multiboot_info *mbi);

/* Allocate a single 4 KiB frame.
 *
 * Returns:
 *   Physical address of the allocated frame (page-aligned), or 0 on failure.
 */
u32 pmm_alloc_frame(void);

/* Free a previously allocated 4 KiB frame.
 *
 * Parameters:
 *   phys - physical address of the frame (page-aligned).
 *
 * Notes:
 *   - If 'phys' is outside the managed range or belongs to a reserved
 *     region (e.g., kernel image, bootstrap paging), the free request
 *     is ignored safely.
 */
void pmm_free_frame(u32 phys);

/* Reserve a physical region [start, end) in bytes.
 *
 * Parameters:
 *   start - starting physical address (inclusive)
 *   end   - ending physical address (exclusive)
 *
 * Notes:
 *   - Only frames within the managed range are affected.
 *   - Used internally by pmm_init and may be used by other subsystems
 *     to reserve special regions.
 */
void pmm_reserve_region(u32 start, u32 end);

/* PMM statistics (managed frames only). */
u64 pmm_get_total_managed_frames(void);
u64 pmm_get_used_frames(void);
u64 pmm_get_free_frames(void);

/* Optional validation helper.
 *
 * Verifies:
 *   - used <= total
 *   - free = total - used
 *
 * On invariant violation, the kernel should panic.
 */
void pmm_validate_state(void);

#endif /* NAZNAOS_MM_PMM_H */

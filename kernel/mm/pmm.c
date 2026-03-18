/*
 * NaznaOS - Physical Memory Manager (PMM)
 *
 * Responsibilities:
 *   - Build a bitmap of manageable physical frames from the Multiboot
 *     memory map.
 *   - Distinguish managed vs. unmanaged frames.
 *   - Track reserved/used vs. free frames.
 *   - Provide pmm_alloc_frame / pmm_free_frame.
 *   - Validate internal accounting invariants.
 *
 * Notes:
 *   - Only frames that belong to usable Multiboot regions and fit within
 *     the bitmap capacity are considered "managed".
 *   - Frames outside the managed range are ignored by PMM and never
 *     counted as used/free.
 */

#include "nazna/types.h"
#include "mm/pmm.h"
#include "boot/multiboot.h"
#include "mm/paging.h"
#include "nazna/kernel.h"

/* Local memset implementation to avoid relying on a global libc symbol.
 * This removes the unresolved 'memset' linker dependency.
 */
static void *pmm_memset(void *s, int c, unsigned long n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

/* Panic handler from kernel/kernel.c. */
void nazna_panic(const char *msg);

/* Linker-provided symbols for kernel physical range. */
extern u32 _kernel_phys_start;
extern u32 _kernel_phys_end;

/* Bootstrap paging structures that must remain reserved. */
extern u32 paging_bootstrap_page_directory_phys;

/* PMM configuration.
 *
 * MAX_MANAGED_FRAMES defines the maximum number of frames the bitmap
 * can track. This is a hard limit on "managed" memory, not on total
 * physical memory in the machine.
 *
 * For now, support up to 512 MiB:
 *   512 MiB / 4 KiB = 131072 frames
 */
#define MAX_MANAGED_FRAMES  (131072u)
#define BITMAP_WORD_BITS    32u
#define BITMAP_WORDS        (MAX_MANAGED_FRAMES / BITMAP_WORD_BITS)

/* Bitmap layout:
 *   bit = 1 -> frame is reserved/used
 *   bit = 0 -> frame is free and managed
 */
static u32 pmm_bitmap[BITMAP_WORDS];

/* Accounting:
 *   - total_managed_frames: frames that belong to usable Multiboot
 *     regions and are within bitmap capacity.
 *   - used_managed_frames: frames currently marked as reserved/used.
 *   - free_managed_frames: derived as total - used.
 */
static u32 total_managed_frames = 0;
static u32 used_managed_frames  = 0;

/* Bitmap helpers */

static inline void bitmap_set(u32 frame_idx)
{
    u32 word = frame_idx / BITMAP_WORD_BITS;
    u32 bit  = frame_idx % BITMAP_WORD_BITS;
    pmm_bitmap[word] |= (1u << bit);
}

static inline void bitmap_clear(u32 frame_idx)
{
    u32 word = frame_idx / BITMAP_WORD_BITS;
    u32 bit  = frame_idx % BITMAP_WORD_BITS;
    pmm_bitmap[word] &= ~(1u << bit);
}

static inline int bitmap_test(u32 frame_idx)
{
    u32 word = frame_idx / BITMAP_WORD_BITS;
    u32 bit  = frame_idx % BITMAP_WORD_BITS;
    return (pmm_bitmap[word] & (1u << bit)) != 0;
}

/* Convert physical address to frame index. */
static inline u32 pmm_frame_index(u32 phys_addr)
{
    return phys_addr / PAGE_SIZE;
}

/* Check if a frame index is within the managed bitmap range. */
static inline int pmm_frame_managed(u32 frame_idx)
{
    return frame_idx < MAX_MANAGED_FRAMES;
}

/* Reserve a frame that is already counted as managed. */
static void pmm_reserve_managed_frame(u32 frame_idx)
{
    if (!pmm_frame_managed(frame_idx)) {
        return;
    }
    if (!bitmap_test(frame_idx)) {
        bitmap_set(frame_idx);
        used_managed_frames++;
    }
}

/* Reserve a physical range [start, end) in bytes, assuming frames in
 * this range are already part of the managed set (if within capacity).
 */
static void pmm_reserve_phys_range(u32 start, u32 end)
{
    if (end <= start) {
        return;
    }

    u32 first_frame = pmm_frame_index(start);
    u32 last_frame  = pmm_frame_index(end - 1u);

    for (u32 f = first_frame; f <= last_frame; ++f) {
        if (!pmm_frame_managed(f)) {
            continue;
        }
        pmm_reserve_managed_frame(f);
    }
}

/* Initialize PMM from Multiboot memory map.
 *
 * Steps:
 *   1. Mark all frames as reserved in the bitmap.
 *   2. Walk Multiboot memory map; for each usable region, mark frames
 *      within that region as managed+free (bitmap bit = 0).
 *   3. Reserve:
 *      - frame 0 (and low memory as desired)
 *      - kernel image physical range
 *      - Multiboot info + memory map structures
 *      - bootstrap paging structures (page directory)
 */
void pmm_init(const struct multiboot_info *mbi)
{
    /* Start with all bits set: everything reserved/unavailable. */
    pmm_memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    total_managed_frames = 0;
    used_managed_frames  = 0;

    if (!(mbi->flags & MULTIBOOT_FLAG_MMAP)) {
        nazna_panic("PMM: no Multiboot memory map");
    }

    /* 1. Mark usable frames as managed+free. */
    u32 mmap_addr   = mbi->mmap_addr;
    u32 mmap_length = mbi->mmap_length;
    u32 mmap_end    = mmap_addr + mmap_length;

    for (u32 ptr = mmap_addr; ptr < mmap_end; ) {
        const struct multiboot_mmap_entry *entry =
            (const struct multiboot_mmap_entry *)ptr;

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            u64 base = entry->addr;
            u64 len  = entry->len;
            u64 end  = base + len;

            /* Walk this region in PAGE_SIZE steps. */
            for (u64 addr = base; addr + PAGE_SIZE <= end; addr += PAGE_SIZE) {
                u32 phys = (u32)addr;
                u32 frame_idx = pmm_frame_index(phys);

                if (!pmm_frame_managed(frame_idx)) {
                    continue;
                }

                /* Mark as managed+free: clear bit if it was set. */
                if (bitmap_test(frame_idx)) {
                    bitmap_clear(frame_idx);
                }

                total_managed_frames++;
            }
        }

        ptr += entry->size + sizeof(entry->size);
    }

    /* 2. Reserve low memory (frame 0 at least). */
    if (pmm_frame_managed(0)) {
        pmm_reserve_managed_frame(0);
    }

    /* 3. Reserve kernel image physical range. */
    u32 kernel_start = (u32)&_kernel_phys_start;
    u32 kernel_end   = (u32)&_kernel_phys_end;
    pmm_reserve_phys_range(kernel_start, kernel_end);

    /* 4. Reserve Multiboot info structure and its memory map. */
    pmm_reserve_phys_range((u32)mbi, (u32)mbi + sizeof(*mbi));
    pmm_reserve_phys_range(mmap_addr, mmap_addr + mmap_length);

    /* 5. Reserve bootstrap paging structures (page directory). */
    pmm_reserve_phys_range(paging_bootstrap_page_directory_phys,
                           paging_bootstrap_page_directory_phys + PAGE_SIZE);
}

/* Allocate one managed, free frame.
 *
 * Returns:
 *   physical address of the frame on success, or 0 on failure.
 */
u32 pmm_alloc_frame(void)
{
    for (u32 frame_idx = 0; frame_idx < MAX_MANAGED_FRAMES; ++frame_idx) {
        if (!bitmap_test(frame_idx)) {
            /* Free and managed. */
            bitmap_set(frame_idx);
            used_managed_frames++;
            return frame_idx * PAGE_SIZE;
        }
    }

    return 0; /* Out of managed frames. */
}

/* Free a previously allocated frame.
 *
 * If the frame is outside the managed range or was not marked as used,
 * this function silently ignores the request.
 */
void pmm_free_frame(u32 phys_addr)
{
    u32 frame_idx = pmm_frame_index(phys_addr);

    if (!pmm_frame_managed(frame_idx)) {
        return;
    }

    if (!bitmap_test(frame_idx)) {
        /* Already free or never allocated; ignore. */
        return;
    }

    bitmap_clear(frame_idx);
    if (used_managed_frames > 0) {
        used_managed_frames--;
    }
}

/* Return basic accounting information. */
u32 pmm_total_managed_frames(void)
{
    return total_managed_frames;
}

u32 pmm_used_managed_frames(void)
{
    return used_managed_frames;
}

u32 pmm_free_managed_frames(void)
{
    if (total_managed_frames < used_managed_frames) {
        return 0;
    }
    return total_managed_frames - used_managed_frames;
}

/* Validate PMM invariants.
 *
 * - used_managed_frames must never exceed total_managed_frames.
 * - free_managed_frames must equal total - used.
 *
 * Panic if invariants are violated.
 */
void pmm_validate_state(void)
{
    if (used_managed_frames > total_managed_frames) {
        nazna_panic("PMM: used > total");
    }

    u32 free_calc = total_managed_frames - used_managed_frames;
    if (free_calc != pmm_free_managed_frames()) {
        nazna_panic("PMM: free mismatch");
    }
}

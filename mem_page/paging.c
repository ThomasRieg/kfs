/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   paging.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/01 01:14:15 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/01 02:03:16 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include <stddef.h>
#include "../pmm/pmm.h"
#include "mem_defines.h"
#include "mem_paging.h"
#include "../pmm/multiboot2.h"
#include "../libk/libk.h"
#include "../vga/vga.h"

// Linker symbols
extern char __kernel_start;
extern char __kernel_end;
extern char __kstack_bottom;
extern char __kstack_top;

#define PDE_COUNT 1024
#define PTE_COUNT 1024

static inline uint32_t align_up_u32(uint32_t x, uint32_t a) { return (x + a - 1) & ~(a - 1); }

static void print_debug(void)
{
        printk("[paging] paging enabled\n");

    // Check CR0.PG
    uint32_t cr0_after = read_cr0();
    printk("[paging] CR0 = %p (PG=%u)\n",
           (void *)cr0_after,
           (cr0_after >> 31) & 1);

    // Kernel symbols should still be readable
    printk("[paging] kernel range: %p -> %p\n",
           &__kernel_start, &__kernel_end);

    printk("[paging] stack range: %p -> %p\n",
           &__kstack_bottom, &__kstack_top);

    // PMM bitmap check
    printk("[paging] pmm bitmap: %p -> %p (%u bytes)\n",
           (void *)pmm_bitmap_pa_start(),
           (void *)pmm_bitmap_pa_end(),
           pmm_bitmap_pa_end() - pmm_bitmap_pa_start());

    // Recursive mapping sanity
    uint32_t *pd_va = (uint32_t *)PD_VA;
    printk("[paging] PD_VA[1023] = %p (should point to PD)\n",
           (void *)(pd_va[1023] & 0xFFFFF000));

    printk("[paging] paging init OK\n");
}

// Map a single 4KB page: VA=PA identity mapping helper (paging not yet enabled, so pointers are physical)
static void map_page_identity(uint32_t *pd, uint32_t va, uint32_t pa, uint32_t flags)
{
    uint32_t pdi = (va >> 22) & 0x3FF;
    uint32_t pti = (va >> 12) & 0x3FF;

    if (!(pd[pdi] & PTE_P)) {
        uint32_t pt_phys = pmm_alloc_frame();
        // must have memory for page tables; OOM here should be fatal in a kernel.
        if (!pt_phys) while (1) { asm volatile("cli; hlt"); }

        memset((void *)pt_phys, 0, PAGE_SIZE); // paging off => identity works
        pd[pdi] = (pt_phys & 0xFFFFF000u) | PTE_P | PTE_RW; // supervisor RW
    }

    uint32_t pt_phys = pd[pdi] & 0xFFFFF000u;
    uint32_t *pt = (uint32_t *)pt_phys; // paging off identity
    pt[pti] = (pa & 0xFFFFF000u) | (flags & 0xFFFu);
}

// Identity map [start, end) (end exclusive), page-granular
static void idmap_range(uint32_t *pd, uint32_t start, uint32_t end, uint32_t flags)
{
    start &= 0xFFFFF000u;
    end = align_up_u32(end, PAGE_SIZE);
    for (uint32_t a = start; a < end; a += PAGE_SIZE)
        map_page_identity(pd, a, a, flags);
}

/*
 * paging_init(mbi):
 * - assumes paging currently OFF
 * - builds a new PD/PT structure using PMM frames
 * - identity maps enough memory to keep everything stable right after enabling paging
 * - enables paging
 * - installs recursive mapping PD[1023] so PD_VA/PT_BASE_VA work
 */
void paging_init(void *multiboot2_info)
{
    // 1) Init PMM first (uses physical==virtual right now)
    pmm_init(multiboot2_info);

    // 2) Allocate page directory frame
    uint32_t pd_phys = pmm_alloc_frame();
    if (!pd_phys) while (1) { asm volatile("cli; hlt"); }
    uint32_t *pd = (uint32_t *)pd_phys;
    memset(pd, 0, PAGE_SIZE);

    // 3) Compute what we must identity-map to survive the switch
    uint32_t kernel_start = (uint32_t)(uintptr_t)&__kernel_start;
    uint32_t kernel_end   = (uint32_t)(uintptr_t)&__kernel_end;

    uint32_t stack_bot = (uint32_t)(uintptr_t)&__kstack_bottom;
    uint32_t stack_top = (uint32_t)(uintptr_t)&__kstack_top;

    uint32_t bmp_start = pmm_bitmap_pa_start();
    uint32_t bmp_end   = pmm_bitmap_pa_end();

    // multiboot2 info region
    t_mb2_info *mbi = (t_mb2_info *)multiboot2_info;
    uint32_t mbi_start = (uint32_t)(uintptr_t)mbi;
    uint32_t mbi_end   = mbi_start + mbi->total_size;

    // VGA text buffer physical
    uint32_t vga_start = (uint32_t)(uintptr_t)g_vga_text_buf;
    uint32_t vga_end   = (uint32_t)(uintptr_t)g_vga_text_buf + VGA_SIZE;

    // idmap_end = max of everything you need
    uint32_t idmap_end = kernel_end;
    if (stack_top > idmap_end) idmap_end = stack_top;
    if (bmp_end   > idmap_end) idmap_end = bmp_end;
    if (mbi_end   > idmap_end) idmap_end = mbi_end;
    if (vga_end   > idmap_end) idmap_end = vga_end;

    // Make it page-aligned
    idmap_end = align_up_u32(idmap_end, PAGE_SIZE);

    // 4) Identity-map critical low memory
    idmap_range(pd, 0x00000000u, idmap_end, PTE_P | PTE_RW);

    // 5) Recursive mapping (map page directory physical addresses at the end of virtual memory)
    pd[1023] = (pd_phys & 0xFFFFF000u) | PTE_P | PTE_RW;

    // 6) Load CR3 and enable paging (CR0.PG)
    write_cr3(pd_phys);

    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000u; // PG
    write_cr0(cr0);

    print_debug();
}

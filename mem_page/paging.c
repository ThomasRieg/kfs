/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   paging.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/01 01:14:15 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 02:40:47 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include <stddef.h>
#include "../pmm/pmm.h"
#include "mem_defines.h"
#include "mem_paging.h"
#include "../multiboot2.h"
#include "../libk/libk.h"
#include "../vga/vga.h"

// Linker symbols
extern char __kernel_start;
extern char __kernel_end;
extern char __kstack_bottom;
extern char __kstack_top;

#define PDE_COUNT 1024
#define PTE_COUNT 1024

static void print_paging_debug(void)
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
		   (void *)pmm_bitmap_va_start(),
		   (void *)pmm_bitmap_va_end(),
		   pmm_bitmap_va_end() - pmm_bitmap_va_start());

	// Recursive mapping sanity
	uint32_t *pd_va = (uint32_t *)PD_VA;
	printk("[paging] PD_VA[1023] = %p (should point to PD physical)\n",
		   (void *)(pd_va[1023] & 0xFFFFF000));

	printk("[paging] paging init OK\n");
}

static inline void invlpg(void *va)
{
	__asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
}

static void zero_page_va(uint32_t va)
{
	volatile uint32_t *p = (volatile uint32_t *)va;
	for (uint32_t i = 0; i < PAGE_SIZE / 4; i++)
		p[i] = 0;
}

extern uint32_t g_bootstrap_end_pa;
static uint32_t g_bootstrap_pt_next_pa = 0;

static uint32_t alloc_bootstrap_pt_pa(void)
{
	if (!g_bootstrap_pt_next_pa)
		g_bootstrap_pt_next_pa = g_bootstrap_end_pa;

	uint32_t pa = (g_bootstrap_pt_next_pa + (PAGE_SIZE - 1)) & 0xFFFFF000u;
	uint32_t next = pa + PAGE_SIZE;

	g_bootstrap_pt_next_pa = next;
	return pa;
}

/* Map [pa_start, pa_end) into physmap VA=PA+KERNEL_VIRT_BASE */
void map_range_physmap_runtime(uint32_t pa_start, uint32_t pa_end, uint32_t flags)
{
	pa_start &= 0xFFFFF000u;
	pa_end = (pa_end + PAGE_SIZE - 1) & 0xFFFFF000u;

	volatile uint32_t *pd = (volatile uint32_t *)PD_VA;

	for (uint32_t pa = pa_start; pa < pa_end; pa += PAGE_SIZE)
	{
		uint32_t va = pa + KERNEL_VIRT_BASE;
		uint32_t pdi = (va >> 22) & 0x3FF;

		if (!(pd[pdi] & PTE_P))
		{
			uint32_t pt_pa = alloc_bootstrap_pt_pa();
			if (!pt_pa)
				for (;;)
					__asm__ volatile("hlt"); // or kernel_panic later

			/* Make sure this PT page is mapped so we can zero it.
			   Two ways:
			   - if identity mapping still exists for pt_pa, you can write via (void*)pt_pa
			   - otherwise write via physmap VA: pt_pa + KERNEL_VIRT_BASE
			*/
			zero_page_va(pt_pa + KERNEL_VIRT_BASE);

			pd[pdi] = (pt_pa & 0xFFFFF000u) | (flags & 0xFFFu) | PTE_P | PTE_RW;

			/* Flush PD entry effect */
			invlpg((void *)(PT_BASE_VA + pdi * PAGE_SIZE)); // conservative
		}

		volatile uint32_t *pte = get_pte((virt_ptr)va);
		*pte = (pa & 0xFFFFF000u) | (flags & 0xFFFu) | PTE_P;
		invlpg((void *)va);
	}
}

static inline virt_ptr pt_window_va(phys_ptr pdi)
{
	return (virt_ptr)(PT_BASE_VA + pdi * PAGE_SIZE);
}

// allocate all kernel_space PDEs so processes have valid kernel virtual address space
static void ensure_kernel_pdes(void)
{
	volatile uint32_t *pd = (volatile uint32_t *)PD_VA;
	uint32_t start = (KERNEL_VIRT_BASE >> 22) & 0x3FFu;

	for (uint32_t pdi = start; pdi < 1023; pdi++)
	{
		if (pd[pdi] & PTE_P)
			continue;

		phys_ptr pt_pa = pmm_alloc_frame();
		if (!pt_pa)
			kernel_panic("OOM allocating kernel page table\n", NULL);

		/* Install PDE first */
		pd[pdi] = ((uint32_t)pt_pa & 0xFFFFF000u) | PTE_P | PTE_RW;

		/* Now the PT page is reachable via the recursive PT window */
		invlpg(pt_window_va(pdi));
		memset(pt_window_va(pdi), 0, PAGE_SIZE);
	}
}

static inline uint32_t read_esp(void)
{
	uint32_t v;
	__asm__ volatile("movl %%esp, %0" : "=r"(v));
	return v;
}

static void unmap_low_identity(void)
{
	volatile uint32_t *pd = (volatile uint32_t *)PD_VA;

	uint32_t kstart = (KERNEL_VIRT_BASE >> 22) & 0x3FFu;

	for (uint32_t pdi = 0; pdi < kstart; pdi++)
	{
		/* don't touch recursive entry */
		if (pdi == 1023)
			continue;

		pd[pdi] = 0;
		invlpg((void *)(PT_BASE_VA + pdi * PAGE_SIZE));
	}
	printk("ESP=%p\n", (void *)read_esp());
	reload_cr3();
}

// remove the bootstrap low-va identity map and inits the pmm
void paging_init(void *multiboot2_info)
{
	// 1) Init PMM first (uses physical==virtual right now)
	pmm_init(multiboot2_info);

	// allocate all kernel PDE so processes have valid addresses for all kernel_space
	ensure_kernel_pdes();

	// unmap the low-va identity map
	unmap_low_identity();
	print_paging_debug();
}

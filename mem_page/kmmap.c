/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kmmap.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 13:46:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/08 17:27:48 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include "mem_paging.h"
#include "mem_defines.h"
#include "../common.h"
#include "../pmm/pmm.h"
#include "../libk/libk.h"

static inline uint32_t align_up_u32(uint32_t x, uint32_t a) { return (x + a - 1u) & ~(a - 1u); }

static inline uint32_t *pd_ptr(void) { return (uint32_t *)PD_VA; }

static inline uint32_t *pt_page_va(uint32_t pdi)
{
	return (uint32_t *)(PT_BASE_VA + (pdi << 12));
}

static inline int pde_present(uint32_t pdi)
{
	return (pd_ptr()[pdi] & PTE_P) != 0;
}

static inline int pte_present(uint32_t pdi, uint32_t pti)
{
	// Only valid if PDE is present
	uint32_t *pt = pt_page_va(pdi);
	return (pt[pti] & PTE_P) != 0;
}

static inline int va_page_free(uint32_t va)
{
	uint32_t pdi = PDE_INDEX(va);
	uint32_t pti = PTE_INDEX(va);

	if (!pde_present(pdi))
		return 1; // no page table => definitely unmapped
	return !pte_present(pdi, pti);
}

static int ensure_page_table(uint32_t pdi)
{
	uint32_t *pd = pd_ptr();

	if (pd[pdi] & PTE_P)
		return 1;

	// Allocate a physical frame for the PT
	uint32_t pt_pa = pmm_alloc_frame();
	if (!pt_pa)
		return 0;

	disable_interrupts();
	void *tmp = kmap((phys_ptr)pt_pa);
	if (!tmp)
	{
		pmm_free_frame(pt_pa);
		enable_interrupts();
		return 0;
	}

	uint32_t *pt_words = (uint32_t *)tmp;
	for (uint32_t i = 0; i < 1024; ++i)
		pt_words[i] = 0;

	kunmap();
	enable_interrupts();

	// Install PDE: present + RW, supervisor
	pd[pdi] = (pt_pa & 0xFFFFF000u) | PTE_P | PTE_RW;

	invalidate_cache((virt_ptr)pt_page_va(pdi));

	return 1;
}

// Map one page at VA to PA.
static void map_one(uint32_t va, phys_ptr pa, uint32_t flags)
{
	uint32_t pdi = PDE_INDEX(va);
	uint32_t pti = PTE_INDEX(va);

	// Ensure PT exists
	if (!ensure_page_table(pdi))
	{
		// TODO handle that cleaner
		kernel_panic("fatal error, probably out of memory in map_one\n");
	}

	uint32_t *pt = pt_page_va(pdi);
	uint32_t *pte = &pt[pti];

	// map_page expects a "mapped" pointer to the entry
	map_page(pa, (virt_ptr)pte, flags | PTE_P);

	invalidate_cache((virt_ptr)va);
}

static uint32_t find_free_run(uint32_t start, uint32_t limit, uint32_t nb_map)
{
	if (nb_map == 0)
		return 0;

	uint32_t run = 0;
	uint32_t run_start = 0;

	for (uint32_t va = start; va + nb_map * PAGE_SIZE <= limit; va += PAGE_SIZE)
	{
		if (va_page_free(va))
		{
			if (run == 0)
				run_start = va;
			run++;
			if (run == nb_map)
				return run_start;
		}
		else
		{
			run = 0;
		}
	}
	return 0;
}

virt_ptr kmmap(virt_ptr start, uint32_t nb_map, uint32_t flags)
{
	if (nb_map == 0)
		return NULL;

	uint32_t va_start = (start == NULL) ? KMMAP_BASE : (uint32_t)(uintptr_t)start;
	va_start = align_up_u32(va_start, PAGE_SIZE);

	uint32_t base = find_free_run(va_start, KMMAP_LIMIT, nb_map);
	if (!base)
		return NULL;

	if (flags & MMAP_CONTIG)
	{
		phys_ptr pa_base = pmm_alloc_frames(nb_map);
		if (!pa_base)
			return NULL;

		for (uint32_t i = 0; i < nb_map; ++i)
		{
			uint32_t va = base + i * PAGE_SIZE;
			phys_ptr pa = pa_base + i * PAGE_SIZE;
			map_one(va, pa, flags);
		}
	}
	else
	{
		for (uint32_t i = 0; i < nb_map; ++i)
		{
			phys_ptr pa = pmm_alloc_frame();
			if (!pa)
			{
				// If allocation fails mid-way, roll back what we already mapped.
				for (uint32_t j = 0; j < i; j++)
				{
					virt_ptr curr_page = (virt_ptr)(va_start + (i * PAGE_SIZE));
					pmm_free_frame_from_va(curr_page);
					unmap_page(curr_page, get_pte(curr_page));
				}
				return NULL;
			}
			uint32_t va = base + i * PAGE_SIZE;
			map_one(va, pa, flags);
		}
	}

	return (virt_ptr)(uintptr_t)base;
}

void kmunmap(virt_ptr ptr, uint32_t nb_map)
{
	for (uint32_t i = 0; i < nb_map; i++)
	{
		virt_ptr curr_page = (virt_ptr)(ptr + (i * PAGE_SIZE));
		pmm_free_frame_from_va(curr_page);
		unmap_page(curr_page, get_pte(curr_page));
	}
}
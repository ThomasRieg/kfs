/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 20:24:25 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/21 22:48:13 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include "mem_paging.h"
#include "mem_defines.h"
#include "../common.h"

uint32_t *get_pde(virt_ptr va)
{
	volatile uint32_t *pd = (volatile uint32_t *)(uintptr_t)PD_VA;
	return (uint32_t *)&pd[PDE_INDEX(va)];
}

uint32_t *get_pte(virt_ptr va)
{
	uint32_t *pde = get_pde(va);
	if (!(*pde & PTE_P))
		return NULL;

	// Now PT window page for this PDE is mapped; safe to access
	uint32_t *pt = (uint32_t *)(uintptr_t)(PT_BASE_VA + PDE_INDEX(va) * PAGE_SIZE);
	return &pt[PTE_INDEX(va)];
}

void invalidate_cache(virt_ptr va)
{
	asm volatile("invlpg (%0)" ::"r"(va) : "memory");
}

void map_page(phys_ptr physical_address, virt_ptr pte, uint32_t flags)
{
	*(phys_ptr *)pte = (physical_address & 0xFFFFF000u) | (flags & 0xFFFu);
}

void unmap_page(virt_ptr ptr, virt_ptr pte)
{
	*(uint32_t *)pte &= ~PTE_P;
	invalidate_cache(ptr);
}

virt_ptr page_align_down(virt_ptr virtual_address)
{
	return ((virt_ptr)((uintptr_t)virtual_address - ((uintptr_t)virtual_address % PAGE_SIZE)));
}

virt_ptr page_align_up(virt_ptr virtual_address)
{
	if ((uintptr_t)virtual_address % PAGE_SIZE)
		return (page_align_down((virt_ptr)((uintptr_t)virtual_address + PAGE_SIZE)));
	else
		return (virtual_address);
}

void write_cr3(phys_ptr phys)
{
	asm volatile("mov %0, %%cr3" ::"r"(phys) : "memory");
}

uint32_t read_cr0(void)
{
	uint32_t v;
	asm volatile("mov %%cr0, %0" : "=r"(v));
	return v;
}

void reload_cr3(void)
{
	uint32_t cr3;
	asm volatile("movl %%cr3, %0" : "=r"(cr3));
	asm volatile("movl %0, %%cr3" : : "r"(cr3) : "memory");
}

void write_cr0(uint32_t v)
{
	asm volatile("mov %0, %%cr0" ::"r"(v) : "memory");
}

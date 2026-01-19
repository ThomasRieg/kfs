/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   boot_init.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/16 16:14:58 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/19 15:49:48 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include <stddef.h>
#include "common.h"
#include "mem_page/mem_defines.h"

/* linker symbols are addresses (we treat them as physical values here) */
extern char __kernel_start_phys;
extern char __kernel_end_phys;
extern char __kstack_bottom_phys;
extern char __kstack_top_phys;

/* Your final high-half entry */
extern void kernel_main(void *mbi);

/* --- tiny helpers, no libc --- */
__attribute__((section(".boot"))) static inline uint32_t align_down(uint32_t x) { return x & 0xFFFFF000u; }
__attribute__((section(".boot"))) static inline uint32_t align_up(uint32_t x) { return (x + PAGE_SIZE - 1) & 0xFFFFF000u; }

__attribute__((section(".boot"))) static inline void write_cr3(uint32_t phys)
{
	__asm__ volatile("movl %0, %%cr3" : : "r"(phys) : "memory");
}

__attribute__((section(".boot"))) static inline uint32_t read_cr0(void)
{
	uint32_t v;
	__asm__ volatile("movl %%cr0, %0" : "=r"(v));
	return v;
}

__attribute__((section(".boot"))) static inline void write_cr0(uint32_t v)
{
	__asm__ volatile("movl %0, %%cr0" : : "r"(v) : "memory");
}

__attribute__((section(".boot"))) static void zero_page(uint32_t phys)
{
	/* paging off => physical pointer is usable */
	volatile uint32_t *p = (volatile uint32_t *)phys;
	for (uint32_t i = 0; i < PAGE_SIZE / 4; i++)
		p[i] = 0;
}

/* Map one 4K page: VA -> PA using PD+PT located in physical memory. */
__attribute__((section(".boot"))) static void map_4k(uint32_t *pd_phys_ptr, uint32_t va, uint32_t pa,
													 uint32_t *pt_bump_phys /* in/out bump allocator */)
{
	uint32_t pdi = (va >> 22) & 0x3FFu;
	uint32_t pti = (va >> 12) & 0x3FFu;

	if (!(pd_phys_ptr[pdi] & PTE_P))
	{
		uint32_t pt_phys = *pt_bump_phys;
		*pt_bump_phys = pt_phys + PAGE_SIZE;

		zero_page(pt_phys);
		pd_phys_ptr[pdi] = (pt_phys & 0xFFFFF000u) | PTE_P | PTE_RW;
	}

	uint32_t pt_phys = pd_phys_ptr[pdi] & 0xFFFFF000u;
	uint32_t *pt = (uint32_t *)pt_phys; /* paging off */
	pt[pti] = (pa & 0xFFFFF000u) | PTE_P | PTE_RW;
}

/* Map PA range into high-half physmap: VA = PA + KERNEL_VIRT_BASE */
__attribute__((section(".boot"))) void map_range_physmap(uint32_t *pd, uint32_t pa_start, uint32_t pa_end,
														 uint32_t *pt_bump_phys)
{
	uint32_t a = align_down(pa_start);
	uint32_t end = align_up(pa_end);

	for (; a < end; a += PAGE_SIZE)
	{
		uint32_t va = a + KERNEL_VIRT_BASE;
		map_4k(pd, va, a, pt_bump_phys);
	}
}

/* Map PA range into high-half physmap: VA = PA + KERNEL_VIRT_BASE */
__attribute__((section(".boot"))) static void map_range_identity(uint32_t *pd, uint32_t pa_start, uint32_t pa_end,
																 uint32_t *pt_bump_phys)
{
	uint32_t a = align_down(pa_start);
	uint32_t end = align_up(pa_end);

	for (; a < end; a += PAGE_SIZE)
	{
		uint32_t va = a;
		map_4k(pd, va, a, pt_bump_phys);
	}
}

// ONLY call before paging is enables (or it will page fault)
__attribute__((section(".boot"))) void early_vga_puts(const char *s)
{
	volatile unsigned char *vga = (volatile unsigned char *)0xB8000;
	unsigned int i = 0;
	while (s[i])
	{
		vga[i * 2] = (unsigned char)s[i];
		vga[i * 2 + 1] = 0x0F; // white on black
		i++;
	}
}

__attribute__((section(".boot.data")))
uint32_t g_bootstrap_end_pa = 0;

/*
 * EARLY paging init:
 * - alloc PD + PTs from a scratch region (no PMM)
 * - map kernel + stack + VGA into higher half (PA + KERNEL_VIRT_BASE)
 * - recursive PD[1023]
 * - enable paging
 * - jump to high-half kernel_main(mbi)
 */
__attribute__((section(".boot"))) void boot_main_low(void *mbi)
{
	/* 1) Choose scratch memory after kernel end (physical), aligned */
	uint32_t kernel_start_pa = (uint32_t)(uintptr_t)&__kernel_start_phys;
	uint32_t kernel_end_pa = (uint32_t)(uintptr_t)&__kernel_end_phys;

	uint32_t scratch = align_up(kernel_end_pa);
	uint32_t pd_phys = scratch;
	uint32_t pt_bump = scratch + PAGE_SIZE; /* next free page for PTs */

	/* 2) Create empty page directory */
	zero_page(pd_phys);
	uint32_t *pd = (uint32_t *)pd_phys;

	/* 3) Map what you need into the higher half */
	map_range_physmap(pd, kernel_start_pa, kernel_end_pa, &pt_bump);

	uint32_t stack_bot_pa = (uint32_t)(uintptr_t)&__kstack_bottom_phys;
	uint32_t stack_top_pa = (uint32_t)(uintptr_t)&__kstack_top_phys;
	map_range_physmap(pd, stack_bot_pa, stack_top_pa, &pt_bump);

	/* VGA text buffer physical */
	map_range_physmap(pd, 0x000B8000u, 0x000B8000u + 0x1000u, &pt_bump);

	/* Multiboot info: keep it accessible in high half too (optional but handy) */
	/* If you know mbi size field, map at least first page so you can read total_size */
	map_range_physmap(pd, (uint32_t)(uintptr_t)mbi, (uint32_t)(uintptr_t)mbi + 0x1000u, &pt_bump);

	map_range_identity(pd, 0x00000000u, kernel_end_pa, &pt_bump);

	/* 4) Recursive mapping */
	pd[1023] = (pd_phys & 0xFFFFF000u) | PTE_P | PTE_RW;

	g_bootstrap_end_pa = pt_bump;

	/* 5) Enable paging */
	write_cr3(pd_phys);
	uint32_t cr0 = read_cr0();
	cr0 |= 0x80000000u; /* PG */
	cr0 |= 0x00010000u; /* WP */
	write_cr0(cr0);

	/* 6) Jump to high half and run normal kernel */
	/* Now kernel_main’s address is high-half and valid */
	__attribute__((noreturn)) void boot_jump_high(void *mbi_va);
	boot_jump_high(mbi + KERNEL_VIRT_BASE);
}

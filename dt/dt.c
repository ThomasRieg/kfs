/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gdt.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/24 00:05:21 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/19 19:36:42 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "dt.h"
#include "../common.h"
#include "../mem_page/mem_defines.h"
#include "../libk/libk.h"

static t_tss32 g_tss;

void dt_set_entry(volatile t_dt_entry_32 *dt,
								 int i,
								 unsigned int base,
								 unsigned int limit,
								 unsigned char access,
								 unsigned char flags)
{
	// limit is 20-bit when using 4K granularity (we pass 0xFFFFF for 4GB-1)
	dt[i].limit_low = (unsigned short)(limit & 0xFFFF);
	dt[i].base_low = (unsigned short)(base & 0xFFFF);
	dt[i].base_mid = (unsigned char)((base >> 16) & 0xFF);
	dt[i].access = access;
	dt[i].gran = (unsigned char)(((limit >> 16) & 0x0F) | (flags << 4));
	dt[i].base_high = (unsigned char)((base >> 24) & 0xFF);
}

static inline void gdt_flush(const t_dt_ptr_32 *gp)
{
	// Load GDTR, then reload CS with a far jump, then reload data segments.
	asm volatile(
		"lgdt (%0)\n"
		"ljmp %1, $1f\n"
		"1:\n"
		"mov %2, %%ax\n"
		"mov %%ax, %%ds\n"
		"mov %%ax, %%es\n"
		"mov %%ax, %%fs\n"
		"mov %%ax, %%gs\n"
		"mov %3, %%ax\n"
		"mov %%ax, %%ss\n"
		:
		: "r"(gp),
		  "i"(GDT_SEL_KCODE),
		  "i"(GDT_SEL_KDATA),
		  "i"(GDT_SEL_KSTACK)
		: "ax", "memory");
}

static inline void gdt_set_tss(volatile t_dt_entry_32 *gdt, int i, uint32_t base, uint32_t limit)
{
	// access = 0x89 : present, ring0, system, type=9 (32-bit available TSS)
	// flags  = 0x00 : byte granularity
	dt_set_entry(gdt, i, base, limit, ACCESS_PRESENT | ACCESS_TYPE_32BIT_AVAILABLE_TSS, 0x00);
}

static inline void tss_flush(uint16_t sel)
{
	__asm__ volatile(
		"movw %0, %%ax \n"
		"ltr  %%ax     \n"
		:
		: "rm"(sel)
		: "ax", "memory");
}

void tss_init(void)
{
	memset(&g_tss, 0, sizeof(g_tss));

	// Kernel stack segment selector for ring0 transitions
	g_tss.ss0 = GDT_SEL_KDATA;

	// must be nonzero before entering ring3
	g_tss.esp0 = 0;

	// Disable IO bitmap by pointing past TSS limit
	g_tss.iomap_base = sizeof(g_tss);
}

void tss_set_kernel_stack(uint32_t esp0)
{
	g_tss.esp0 = esp0;
}

// call before removing the boot_init low va identity mapping
void gdt_install_basic(void)
{
	// place the GDT at address 0x800 as subject asked
	volatile t_dt_entry_32 *gdt_phys = (volatile t_dt_entry_32 *)GDT_START;

	// "flat" segments: cover the whole 4 GiB space
	const unsigned int base = 0x00000000;
	const unsigned int limit = 0x000FFFFF;
	const unsigned char flags = FLAG_PAGE_GRANULARITY | FLAG_PROTECTED_32BITS;

	dt_set_entry(gdt_phys, 0, 0, 0, 0, 0);				  // null
	dt_set_entry(gdt_phys, 1, base, limit, ACCESS_PRESENT | ACCESS_CODE_OR_DATA | ACCESS_WRITE, flags); // kernel data  -> 0x08
	dt_set_entry(gdt_phys, 2, base, limit, ACCESS_PRESENT | ACCESS_CODE_OR_DATA | ACCESS_READ | ACCESS_EXEC, flags); // kernel code  -> 0x10
	dt_set_entry(gdt_phys, 3, base, limit, ACCESS_PRESENT | ACCESS_CODE_OR_DATA | ACCESS_WRITE, flags); // kernel stack -> 0x18
	dt_set_entry(gdt_phys, 4, base, limit, ACCESS_PRESENT | ACCESS_RING3 | ACCESS_CODE_OR_DATA | ACCESS_WRITE, flags); // user data    -> 0x20
	dt_set_entry(gdt_phys, 5, base, limit, ACCESS_PRESENT | ACCESS_RING3 | ACCESS_CODE_OR_DATA | ACCESS_READ | ACCESS_EXEC, flags); // user code    -> 0x28
	dt_set_entry(gdt_phys, 6, base, limit, ACCESS_PRESENT | ACCESS_RING3 | ACCESS_CODE_OR_DATA | ACCESS_WRITE, flags); // user stack   -> 0x30
	tss_init();

	// entry 7 = TSS descriptor
	gdt_set_tss(gdt_phys, 7, (uint32_t)&g_tss, sizeof(g_tss) - 1);

	extern void map_range_physmap_runtime(uint32_t pa_start, uint32_t pa_end, uint32_t flags);

	t_dt_ptr_32 gp;
	gp.limit = (unsigned short)(GDT_NB_ENTRY * sizeof(t_dt_entry_32) - 1);
	gp.base = (unsigned int)gdt_phys + KERNEL_VIRT_BASE;
	map_range_physmap_runtime(GDT_START, GDT_END, PTE_P | PTE_RW);
	gdt_flush(&gp);
	tss_flush(GDT_SEL_TSS);
}

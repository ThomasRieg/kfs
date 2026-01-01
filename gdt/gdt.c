/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gdt.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/24 00:05:21 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/01 00:39:41 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "gdt.h"

static inline void gdt_set_entry(volatile t_gdt_entry_32 *gdt,
                                 int i,
                                 unsigned int base,
                                 unsigned int limit,
                                 unsigned char access,
                                 unsigned char flags)
{
	// limit is 20-bit when using 4K granularity (we pass 0xFFFFF for 4GB-1)
	gdt[i].limit_low = (unsigned short)(limit & 0xFFFF);
	gdt[i].base_low  = (unsigned short)(base & 0xFFFF);
	gdt[i].base_mid  = (unsigned char)((base >> 16) & 0xFF);
	gdt[i].access    = access;
	gdt[i].gran      = (unsigned char)(((limit >> 16) & 0x0F) | (flags & 0xF0));
	gdt[i].base_high = (unsigned char)((base >> 24) & 0xFF);
}

static inline void gdt_flush(const t_gdt_ptr_32 *gp)
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
		: "ax", "memory"
	);
}

void gdt_install_basic(void)
{
	// place the GDT at address 0x800 as subject asked
	volatile t_gdt_entry_32 *gdt = (volatile t_gdt_entry_32 *)GDT_START;
	t_gdt_ptr_32 gp;

	// 7 entries: null + (kdata,kcode,kstack, udata,ucode,ustack)
	// Each entry is 8 bytes => 7 * 8 = 56 bytes.
	gp.limit = (unsigned short)(7 * sizeof(t_gdt_entry_32) - 1);
	gp.base  = (unsigned int)gdt;

	// "flat" segments: base=0, limit=0xFFFFF with 4K granularity + 32-bit
	// flags: 0xC0 => G=1 (4K), D=1 (32-bit)
	const unsigned int base = 0x00000000;
	const unsigned int limit = 0x000FFFFF;
	const unsigned char flags = 0xC0;

	// Access bytes:
	// 0x9A = kernel code (present, ring0, code, readable)
	// 0x92 = kernel data (present, ring0, data, writable)
	// 0xFA = user code   (present, ring3, code, readable)
	// 0xF2 = user data   (present, ring3, data, writable)

	gdt_set_entry(gdt, 0, 0, 0, 0, 0);                 // null
	gdt_set_entry(gdt, 1, base, limit, 0x92, flags);   // kernel data  -> 0x08
	gdt_set_entry(gdt, 2, base, limit, 0x9A, flags);   // kernel code  -> 0x10
	gdt_set_entry(gdt, 3, base, limit, 0x92, flags);   // kernel stack -> 0x18
	gdt_set_entry(gdt, 4, base, limit, 0xF2, flags);   // user data    -> 0x20
	gdt_set_entry(gdt, 5, base, limit, 0xFA, flags);   // user code    -> 0x28
	gdt_set_entry(gdt, 6, base, limit, 0xF2, flags);   // user stack   -> 0x30

	gdt_flush(&gp);
}
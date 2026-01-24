/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gdt.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/24 00:04:29 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/19 19:34:19 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GDT_H
#define GDT_H

#define GDT_START 0x800 // physical address, add KERNEL_VIRT_BASE to access it after paging_init
#define GDT_NB_ENTRY 9
#define GDT_END GDT_START + (GDT_NB_ENTRY * sizeof(t_dt_entry_32))

#include "../common.h"

enum dt_flag {
	FLAG_LONG_64BITS = 2,
	FLAG_PROTECTED_32BITS = 4,
	FLAG_PAGE_GRANULARITY = 8 // 4 KiB
};

enum dt_access {
	ACCESS_ACCESSED = 1,
	ACCESS_READ = 2, // code segments
	ACCESS_WRITE = 2, // data segments
	ACCESS_DC = 4,
	ACCESS_EXEC = 8,
	ACCESS_CODE_OR_DATA = 16,
	ACCESS_RING3 = 3 << 5,
	ACCESS_PRESENT = 1 << 7,
	ACCESS_TYPE_LDT = 0x2,
	ACCESS_TYPE_32BIT_AVAILABLE_TSS = 0x9,

};

// Selectors (index * 8):
// 0x00 null
// 0x08 kernel data
// 0x10 kernel code
// 0x18 kernel stack
// 0x20 user data
// 0x28 user code
// 0x30 user stack
// 0x38 tss
enum
{
	GDT_SEL_NULL = 0x00,
	GDT_SEL_KDATA = 0x08,
	GDT_SEL_KCODE = 0x10,
	GDT_SEL_KSTACK = 0x18,
	GDT_SEL_UDATA = 0x20,
	GDT_SEL_UCODE = 0x28,
	GDT_SEL_USTACK = 0x30,
	GDT_SEL_TSS = 0x38,
};

typedef struct s_dt_entry_32
{
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char base_mid;
	unsigned char access;
	unsigned char gran; // high 4 bits = flags, low 4 bits = limit_high
	unsigned char base_high;
} __attribute__((packed)) t_dt_entry_32;

typedef struct s_dt_ptr_32
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed)) t_dt_ptr_32;

typedef struct __attribute__((packed)) s_tss32
{
	uint32_t prev_tss;

	uint32_t esp0;
	uint32_t ss0;

	uint32_t esp1;
	uint32_t ss1;

	uint32_t esp2;
	uint32_t ss2;

	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;

	uint32_t eax, ecx, edx, ebx;
	uint32_t esp, ebp, esi, edi;

	uint32_t es, cs, ss, ds, fs, gs;

	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} t_tss32;

void tss_init(void);
void tss_set_kernel_stack(uint32_t esp0);

void dt_set_entry(volatile t_dt_entry_32 *dt,
					 int i,
					 unsigned int base,
					 unsigned int limit,
					 unsigned char access,
					 unsigned char flags);
void gdt_install_basic(void);

#endif

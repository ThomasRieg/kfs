/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   gdt.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/24 00:04:29 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/24 00:15:02 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GDT_H
#define GDT_H

// Selectors (index * 8):
// 0x00 null
// 0x08 kernel data
// 0x10 kernel code
// 0x18 kernel stack
// 0x20 user data
// 0x28 user code
// 0x30 user stack
enum {
	GDT_SEL_NULL        = 0x00,
	GDT_SEL_KDATA       = 0x08,
	GDT_SEL_KCODE       = 0x10,
	GDT_SEL_KSTACK      = 0x18,
	GDT_SEL_UDATA       = 0x20,
	GDT_SEL_UCODE       = 0x28,
	GDT_SEL_USTACK      = 0x30,
};

typedef struct s_gdt_entry_32 {
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char  base_mid;
	unsigned char  access;
	unsigned char  gran;      // high 4 bits = flags, low 4 bits = limit_high
	unsigned char  base_high;
} __attribute__((packed)) t_gdt_entry_32;

typedef struct s_gdt_ptr_32 {
	unsigned short limit;
	unsigned int   base;
} __attribute__((packed)) t_gdt_ptr_32;

void gdt_install_basic(void);

#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   interrupts.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:10:29 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/12 14:26:41 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../common.h"
#include "s_regs.h"

#define IDT_PRESENT_AND_GATE_32_INT 0x8e

typedef struct s_idt_entry_32
{
	short offset_1;
	short selector;
	unsigned char zero;
	unsigned char type_attributes;
	short offset_2;
} t_idt_entry_32;

// selector 16 = kernel code in GDT
#define DEF_INTERRUPT(handler)                          \
	(t_idt_entry_32)                                    \
	{                                                   \
		.selector = 16,                                 \
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT, \
		.offset_1 = ((unsigned int)&handler) & 0xFFFF,  \
		.offset_2 = ((unsigned int)&handler) >> 16,     \
	}

typedef void (*t_isr_handler)(t_regs *r);

void isr_add_handler(uint8_t int_no, t_isr_handler func);

void setup_interrupts();

#endif
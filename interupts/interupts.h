/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   interupts.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:10:29 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/12 01:42:04 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INTERUPTS_H
#define INTERUPTS_H

#include "../common.h"

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
	(t_idt_entry_32)                               \
	{                                                   \
		.selector = 16,                                 \
		.type_attributes = IDT_PRESENT_AND_GATE_32_INT, \
		.offset_1 = ((unsigned int)&handler) & 0xFFFF,  \
		.offset_2 = ((unsigned int)&handler) >> 16,     \
	}

typedef struct __attribute__((packed)) t_regs
{
    /* segment registers saved by common stub (int_entrypoint.s)*/
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;

    /* general registers saved by pusha */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t orig_esp; /* the esp that existed before pusha */
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* pushed by micro-stub (isr_stubs.s) */
    uint32_t int_no;
    uint32_t err_code;

    /* pushed automatically by CPU */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    /* Only present if coming from lower privilege (user->kernel) (check cs & 3 before using)*/
    uint32_t useresp;
    uint32_t ss;
} t_regs;

typedef void (*t_isr_handler)(t_regs *r);

void isr_add_handler(uint8_t int_no, t_isr_handler func);

void setup_interupts();

#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   setup_interrupts.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:20:35 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/19 02:36:09 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "interrupts.h"
#include "stubs_declaration.h"
#include "handlers/handlers.h"
#include "../common.h"

t_idt_entry_32 idt[256];

// this doesn't enable interrupts, have to call enable_interrupts separately
void setup_interrupts()
{
	idt_install_all_stubs(idt);
	isr_add_handler(INT_BREAKPOINT, breakpoint_handler);
	isr_add_handler(INT_DOUBLE_FAULT, double_fault_handler); // shouldn't ever fire with this design, keeping for legacy code
	isr_add_handler(INT_TIMER, timer_handler_before_scheduler);
	isr_add_handler(INT_KEYBOARD, keyboard_handler);
	isr_add_handler(INT_PAGE_FAULT, page_fault_handler);
	isr_add_handler(INT_SERIAL1, serial1_handler);
	isr_add_handler(INT_NIC, rtl8139_handler);
	isr_add_handler(INT_PRIMARY_ATA, ata_handler);
	isr_add_handler(INT_SECONDARY_ATA, ata_handler);
	isr_add_handler(INT_YIELD, yield_handler);
	isr_add_handler(INT_GENERAL_PROTECTION_FAULT, general_protection_handler);
	struct descriptor_table_pointer
	{
		unsigned short limit;
		unsigned int base;
	} __attribute__((packed)) idt_pointer = {
		.limit = sizeof(idt) - 1,
		.base = (unsigned int)&idt[0],
	};
	asm volatile("lidt %0" : : "m"(idt_pointer));
}

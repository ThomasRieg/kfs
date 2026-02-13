/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:37:11 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 02:11:06 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "interrupts.h"
#include "../libk/libk.h"
#include "../drivers/pic.h"
#include "../tasks/task.h"

static t_isr_handler g_isr_handlers[256];

void isr_dispatch_c(t_interrupt_data *regs)
{
	disable_interrupts(); // should be disabled by default, but let's be safe
	print_trace("servicing interrupt %u\n", regs->int_no);
	if (g_curr_task)
	{
		g_curr_task->k_esp = (uintptr_t)regs;
	}
	if (g_isr_handlers[regs->int_no])
	{
		if (regs->int_no >= PIC_OFFSET && regs->int_no < (PIC_OFFSET + 16))
			pic_eoi((unsigned char)(regs->int_no & 0xFF)); // send before handler in case of a context switch in the handler
		g_isr_handlers[regs->int_no](regs);
	}
	else
	{
		print_err("received unhandled interrupt %u\n", regs->int_no); // TODO add better handling here, just kill process.
		kernel_panic("unhandled interrupt", regs);
	}
	print_trace("end of interrupt, will iret at %p", regs->eip);
	print_trace("\e[31m\ndone servicing interrupt %u\e[0m\n", regs->int_no);
}

void isr_add_handler(uint8_t int_no, t_isr_handler func)
{
	g_isr_handlers[int_no] = func;
}

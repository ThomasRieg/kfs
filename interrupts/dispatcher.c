/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:37:11 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/15 16:38:44 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "interrupts.h"
#include "../libk/libk.h"
#include "../drivers/pic.h"

static t_isr_handler g_isr_handlers[256];

void isr_dispatch_c(t_regs *regs)
{
	if (g_isr_handlers[regs->int_no])
	{
		g_isr_handlers[regs->int_no](regs);
		if (regs->int_no >= PIC_OFFSET && regs->int_no < (PIC_OFFSET + 16))
			pic_eoi((unsigned char)(regs->int_no & 0xFF));
	}
	else
	{
		printk("received unhandled interrupt %u\n", regs->int_no); // TODO add better handling here, idk how to make something robust for when we implement software interrupts and processes, so WIP
		kernel_panic("unhandled interrupt", regs);
	}
}

void isr_add_handler(uint8_t int_no, t_isr_handler func)
{
	g_isr_handlers[int_no] = func;
}

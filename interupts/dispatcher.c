/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dispatcher.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 00:37:11 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/12 01:29:45 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "interupts.h"
#include "../libk/libk.h"
#include "../pic.h"

static t_isr_handler g_isr_handlers[256];

void isr_dispatch_c(t_regs *regs)
{
    if (g_isr_handlers[regs->int_no])
    {
        g_isr_handlers[regs->int_no](regs);
        pic_eoi((unsigned char)(regs->int_no & 0xFF));
    }
    else
    {
        printk("received unhandled interupt %u\n", regs->int_no); //TODO add better handling here, idk how to make something robust for when we implement software interupts and processes, so WIP
    }
}

void isr_add_handler(uint8_t int_no, t_isr_handler func)
{
    g_isr_handlers[int_no] = func;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlers.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:11:02 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/12 14:29:51 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HANDLERS_H
#define HANDLERS_H

#include "../interrupts.h"

// helper function for every handler
void print_interrupt_frame(t_regs *regs);

void double_fault_handler(t_regs *regs);
void page_fault_handler(t_regs *regs);
void breakpoint_handler(t_regs *regs);
void timer_handler(__attribute__((unused)) t_regs *regs);
void serial1_handler(__attribute__((unused)) t_regs *regs);
void keyboard_handler(__attribute__((unused)) t_regs *regs);
void rtl8139_handler(__attribute__((unused)) t_regs *regs); // defined in rtl8139.c

#endif
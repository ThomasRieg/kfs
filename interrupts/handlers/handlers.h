/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlers.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 01:11:02 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/19 02:34:35 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HANDLERS_H
#define HANDLERS_H

#include "../interrupts.h"

// helper function for every handler
void print_interrupt_frame(t_interrupt_data *regs);

void double_fault_handler(t_interrupt_data *regs);
void page_fault_handler(t_interrupt_data *regs);
void breakpoint_handler(t_interrupt_data *regs);
void timer_handler(__attribute__((unused)) t_interrupt_data *regs);
void timer_handler_before_scheduler(__attribute__((unused)) t_interrupt_data *regs);
void serial1_handler(__attribute__((unused)) t_interrupt_data *regs);
void keyboard_handler(__attribute__((unused)) t_interrupt_data *regs);
void rtl8139_handler(__attribute__((unused)) t_interrupt_data *regs); // defined in drivers/rtl8139.c
void ata_handler(__attribute__((unused)) t_interrupt_data *regs);	  // defined in drivers/ide.c
void yield_handler(__attribute__((unused)) t_interrupt_data *regs);
void general_protection_handler(t_interrupt_data *regs);

#endif

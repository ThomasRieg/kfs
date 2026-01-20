/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syscalls.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 16:44:55 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/20 15:24:20 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../common.h"
#include "../interrupts/s_regs.h"

void init_syscalls(void);
typedef uint32_t (*t_syscall_func)(t_interrupt_data *);
void add_syscall(uint32_t syscall_nbr, t_syscall_func func);
uint32_t syscall_call(uint32_t syscall_nbr, ...);

uint32_t syscall_getuid(t_interrupt_data *regs);
uint32_t syscall_wait4(t_interrupt_data *regs);
uint32_t syscall_signal(t_interrupt_data *regs);
uint32_t syscall_kill(t_interrupt_data *regs);
uint32_t syscall_fork(t_interrupt_data *regs);
__attribute__((noreturn)) uint32_t syscall_exit(t_interrupt_data *regs);

#endif
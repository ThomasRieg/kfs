/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syscalls.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 16:44:55 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/14 17:32:16 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../common.h"
#include "../interrupts/s_regs.h"

void init_syscalls(void);
typedef uint32_t (*t_syscall_func)(t_regs *);
void add_syscall(uint32_t syscall_nbr, t_syscall_func func);
uint32_t syscall_call(uint32_t syscall_nbr, ...);

#endif
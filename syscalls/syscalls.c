/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syscalls.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 16:48:13 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 02:25:24 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "syscalls.h"
#include "../interrupts/interrupts.h"
#include "../libk/libk.h"
#include "../errno.h"
#include <stdarg.h>

static t_syscall_func g_syscall_handlers[1000];

void syscall_dispatcher(t_interrupt_data *regs)
{
	print_trace("syscall %u\n", regs->eax);
	if (regs->eax >= (sizeof(g_syscall_handlers) / sizeof(t_syscall_func)))
		regs->eax = -ENOSYS;
	else if (g_syscall_handlers[regs->eax])
		regs->eax = g_syscall_handlers[regs->eax](regs);
	else
	{
		print_err("received unhandled syscall %u\n", regs->eax);
		regs->eax = -ENOSYS;
	}
	print_trace("return value %u\n", regs->eax);
}

void init_syscalls(void)
{
	isr_add_handler(INT_SYSCALLS, syscall_dispatcher);
}

// unsafe showcase way of calling a syscall from the kernel
uint32_t syscall_call(uint32_t syscall_nbr, ...)
{
	va_list ap;
	uint32_t a[5] = {0};

	va_start(ap, syscall_nbr);
	for (int i = 0; i < 5; i++)
		a[i] = va_arg(ap, uint32_t);
	va_end(ap);

	uint32_t ret;

	asm volatile(
		"int $%c[intno]"
		: "=a"(ret)
		: [intno] "i"(INT_SYSCALLS),
		  "a"(syscall_nbr),
		  "b"(a[0]),
		  "c"(a[1]),
		  "d"(a[2]),
		  "S"(a[3]),
		  "D"(a[4])
		: "memory", "cc");

	return ret;
}

void add_syscall(uint32_t syscall_nbr, t_syscall_func func)
{
	if (syscall_nbr < (sizeof(g_syscall_handlers) / sizeof(t_syscall_func)))
		g_syscall_handlers[syscall_nbr] = func;
	else
		kernel_panic("tried to add a syscall with a too high number\n", NULL);
}

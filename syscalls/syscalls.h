/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syscalls.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 16:44:55 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 06:09:25 by thrieg           ###   ########.fr       */
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
uint32_t syscall_getgid(__attribute__((unused)) t_interrupt_data *regs);
uint32_t syscall_geteuid32(__attribute__((unused)) t_interrupt_data *regs);
uint32_t syscall_getegid32(__attribute__((unused)) t_interrupt_data *regs);

uint32_t syscall_getpid(t_interrupt_data *regs);
uint32_t syscall_gettid(t_interrupt_data *regs);
uint32_t syscall_getppid(t_interrupt_data *regs);
uint32_t syscall_brk(t_interrupt_data *regs);
uint32_t syscall_uname(t_interrupt_data *regs);
uint32_t syscall_wait4(t_interrupt_data *regs);
uint32_t syscall_signal(t_interrupt_data *regs);
uint32_t syscall_kill(t_interrupt_data *regs);
uint32_t syscall_fork(t_interrupt_data *regs);
uint32_t syscall_execve(t_interrupt_data *regs);
uint32_t syscall_archprctl(t_interrupt_data *regs);
uint32_t syscall_tkill(t_interrupt_data *regs);
uint32_t syscall_poll(t_interrupt_data *regs);
uint32_t syscall_ioctl(t_interrupt_data *regs);
uint32_t syscall_nanosleep(t_interrupt_data *regs);
__attribute__((noreturn)) uint32_t syscall_exit(t_interrupt_data *regs);

uint32_t syscall_rt_sigprocmask(t_interrupt_data *regs);
uint32_t syscall_rt_sigaction(t_interrupt_data *regs);

uint32_t syscall_get_thread_area(t_interrupt_data *regs);
uint32_t syscall_set_thread_area(t_interrupt_data *regs);
uint32_t syscall_set_tid_address(t_interrupt_data *regs);
uint32_t syscall_writev(t_interrupt_data *regs);
uint32_t syscall_mprotect(__attribute__((unused)) t_interrupt_data *regs);
uint32_t syscall_getpgid(__attribute__((unused)) t_interrupt_data *regs);
uint32_t syscall_setpgid(t_interrupt_data *regs);

uint32_t syscall_mmap2(t_interrupt_data *regs);
uint32_t syscall_munmap(t_interrupt_data *regs);

uint32_t syscall_readlink(t_interrupt_data *regs);
uint32_t syscall_fstatat(t_interrupt_data *regs);
uint32_t syscall_statx(t_interrupt_data *regs);
uint32_t syscall_openat(t_interrupt_data *regs);
uint32_t syscall_open(t_interrupt_data *regs);
uint32_t syscall_dup(t_interrupt_data *regs);
uint32_t syscall_dup2(t_interrupt_data *regs);
uint32_t syscall_pipe(t_interrupt_data *regs);
uint32_t syscall_pipe2(t_interrupt_data *regs);
uint32_t syscall_close(t_interrupt_data *regs);
uint32_t syscall_fcntl64(t_interrupt_data *regs);
uint32_t syscall_read(t_interrupt_data *regs);
uint32_t syscall_write(t_interrupt_data *regs);
uint32_t syscall_chdir(t_interrupt_data *regs);
uint32_t syscall_getdents64(t_interrupt_data *regs);
uint32_t syscall_getcwd(t_interrupt_data *regs);

#endif

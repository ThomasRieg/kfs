/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/15 12:46:15 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/15 20:09:28 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIGNALS_H
#define SIGNALS_H

#include "../common.h"

extern void __sigreturn_trampoline(void);

typedef void (*t_sig_handler)(int);
#define SIGNAL_TRAMPOLINE_PHYS ((phys_ptr)((uintptr_t)&__sigreturn_trampoline - (uintptr_t)KERNEL_VIRT_BASE))
#define SIGNAL_TRAMPOLINE_VA (virt_ptr)(KERNEL_VIRT_BASE - PAGE_SIZE)
#define SIG_IGN ((t_sig_handler)1) // ignore this signal
#define SIG_DFL ((t_sig_handler)0) // default behavior (stop process)

#define SIGCHLD 17u // list here https://man7.org/linux/man-pages/man7/signal.7.html
#define SIGPIPE 13u
#define SIGSEGV 11u
#define SIGKILL 9u

#define NSIG 32

typedef struct s_sigaction_k {
    t_sig_handler handler;
    uint32_t mask;      // signals blocked during handler
    uint32_t flags;     // SA_*
    void *restorer;     // user trampoline (optional, can ignore at first)
} t_sigaction_k;

//struct used by user in syscalls
typedef struct __attribute__((packed)) s_sigaction_u {
    uint32_t handler;     // sa_handler
    uint32_t flags;       // sa_flags
    uint32_t restorer;    // sa_restorer (may be 0)
    uint32_t mask;        // for first pass: 32-bit mask is enough
} t_sigaction_u;

typedef struct __attribute__((packed)) s_sigframe32 {
    uint32_t retaddr;         // trampoline address
    uint32_t signum;
    t_interrupt_data saved_context;        // saved user context
    uint32_t oldmask;         // previous blocked mask
} t_sigframe32;

bool map_signal_trampoline(void);
typedef struct task t_task;
void task_terminate_by_signal(t_task *t, int sig);
void signal_deliver_if_needed(t_interrupt_data *f);
bool enqueue_sig(t_task *task, int sig);

#endif
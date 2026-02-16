/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sys_signals.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/15 13:08:09 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/16 15:50:18 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../common.h"
#include "../../errno.h"
#include "../../tasks/task.h"

static int sig_valid(int sig) { return sig > 0 && sig < NSIG; }

static int sig_forbidden(int sig) { return (sig == 9 /*SIGKILL*/ || sig == 19 /*SIGSTOP*/); }

uint32_t syscall_rt_sigaction(t_interrupt_data *r)
{
    int sig = (int)r->ebx;
    t_sigaction_k *act_u  = (t_sigaction_k *)r->ecx;   // user ptr or 0
    t_sigaction_u *oact_u = (t_sigaction_u *)r->edx;   // user ptr or 0
    uint32_t sigsetsize = r->esi;
    print_trace("rt_sigaction %u %x %x\n", sig, act_u, oact_u);

    if (act_u && !user_range_ok((virt_ptr)act_u, sizeof(*act_u), false, &g_curr_task->proc_memory))
        return (uint32_t)-EFAULT;

    if (oact_u && !user_range_ok((virt_ptr)oact_u, sizeof(*oact_u), true, &g_curr_task->proc_memory))
        return (uint32_t)-EFAULT;

    if (!sig_valid(sig)) return (-EINVAL);
    if (sig_forbidden(sig)) return (-EINVAL);

    if (sigsetsize < 4) return (-EINVAL);

    t_sigaction_k old = g_curr_task->sigact[sig];

    // If user wants old action, write it
    if (oact_u)
    {

        t_sigaction_u *uo = (t_sigaction_u *)(uintptr_t)oact_u;
        uo->handler  = (uint32_t)(uintptr_t)old.handler;
        uo->flags    = old.flags;
        uo->restorer = (uint32_t)(uintptr_t)old.restorer;
        uo->mask     = old.mask;
    }

    // If user provided new action, read it
    if (act_u)
    {
        t_sigaction_u *ua = (t_sigaction_u *)(uintptr_t)act_u;

        t_sigaction_k na;
        na.handler  = (t_sig_handler)(uintptr_t)ua->handler;
        na.flags    = ua->flags;
        na.restorer = (void *)(uintptr_t)ua->restorer;
        na.mask     = ua->mask;

        // Accept SIG_DFL (0), SIG_IGN (1), or a user pointer.
        //too hard to validate handler function is entirely in allocated userspace memory, irrelevant anyway because we'll run it in ring 3 so it will fault adequately

        g_curr_task->sigact[sig] = na;
    }

    return (0);
}

//119
uint32_t syscall_sigreturn(t_interrupt_data *f)
{
    uint32_t usp = f->useresp;
    print_trace("sigreturn from esp %u\n", usp);
    if (!user_range_ok((virt_ptr)usp, sizeof(t_sigframe32), false, &g_curr_task->proc_memory))
        return (-EFAULT);

    t_sigframe32 *uf = (t_sigframe32 *)((uintptr_t)usp - 4);
    print_trace("uf at %p\n", uf);

    // restore mask
    g_curr_task->blocked_signals = uf->oldmask;
    g_curr_task->in_signal = false;

    // restore full user context into interrupt frame
    memcpy(f, &uf->saved_context, sizeof(*f));

    return (0); // interrupt will pop back the modified frame
}

//48
uint32_t syscall_signal(t_interrupt_data *r)
{
    int sig = (int)r->ebx;
    uintptr_t handler = (uintptr_t)r->ecx;
    print_trace("signal %u\n", handler);

    if (!sig_valid(sig)) return (-EINVAL);
    if (sig_forbidden(sig)) return (-EINVAL);

    t_sigaction_k *a = &g_curr_task->sigact[sig];
    t_sig_handler old = a->handler;

    a->handler  = (t_sig_handler)handler;
    a->mask     = 0;
    a->flags    = 0;
    a->restorer = NULL;

    return (uint32_t)(uintptr_t)old;
}

// 37
//  	pid_t pid | int sig
// TODO handle pid = 0 (process group) and pid = -1 (all killeable process)
uint32_t syscall_kill(__attribute__((unused)) t_interrupt_data *regs)
{
    int pid = regs->ebx;
	int sig = regs->ecx;
    print_trace("tkill %d %x\n", pid, sig);
    t_task *target = NULL;
    t_task *curr = g_task_list;
    if (sig > NSIG || sig < 1)
        return (-EINVAL);
    while (curr)
    {
        if ((int)curr->task_id == pid)
        {
            target = curr;
            break;
        }
        curr = curr->next_all_task;
    }
    if (!target)
        return (-ESRCH);
    enqueue_sig(target, sig);
	return (0); // success
}

uint32_t syscall_tkill(t_interrupt_data *regs)
{
	int tid = regs->ebx;
	int sig = regs->ecx;
	print_trace("tkill %d %x\n", tid, sig);
    syscall_kill(regs); //temporary TODO implement
	return (0);
}

uint32_t syscall_rt_sigprocmask(t_interrupt_data *regs)
{
	int how = regs->ebx;
	void *set = (void *)regs->ecx;
	void *oldset = (void *)regs->edx;
	print_trace("rt_sigprocmask %x %x %x\n", how, set, oldset);
	return (0);
}

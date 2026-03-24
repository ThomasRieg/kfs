/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sys_signals.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/15 13:08:09 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/22 00:20:02 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../common.h"
#include "../../errno.h"
#include "../../tasks/task.h"

static int sig_valid(int sig) { return sig > 0 && sig < NSIG; }

static int sig_forbidden(int sig) { return (sig == SIGKILL || sig == SIGSTOP); }

uint32_t syscall_rt_sigaction(t_interrupt_data *r)
{
    int sig = (int)r->ebx;
    t_sigaction_k *act_u  = (t_sigaction_k *)(uintptr_t)r->ecx;   // user ptr or 0
    t_sigaction_u *oact_u = (t_sigaction_u *)(uintptr_t)r->edx;   // user ptr or 0
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
        if (na.restorer)
            print_debug("pid %u sig %d provided a restorer at %p\n", g_curr_task->task_id, sig, na.restorer);
        if (na.flags)
            print_debug("pid %u sig %d provided flags : 0x%x\n", g_curr_task->task_id, sig, na.flags);
        if (na.mask)
            print_debug("pid %u sig %d provided a mask : 0x%x\n", g_curr_task->task_id, sig, na.mask);

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

    // syscall dispatcher sets eax to the return value
    // make it a no-op by returning the eax in the pre-signal saved
    // kernel frame
    return (f->eax); // interrupt epilogue will pop back the modified frame
}

// 173
uint32_t syscall_rt_sigreturn(__attribute__((unused)) t_interrupt_data *f)
{
    print_err("rt_sigreturn called by pid %u\n", g_curr_task->task_id);
    enqueue_sig(g_curr_task, SIGKILL);
    return (0);
}

//48
uint32_t syscall_signal(t_interrupt_data *r)
{
    int sig = (int)r->ebx;
    uintptr_t handler = (uintptr_t)r->ecx;
    print_trace("signal sig %u handler %p\n", sig, handler);

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

bool can_signal(t_task *src, t_task *dst)
{
    if (!src || !dst) return false;
    if (src->euid == 0) return true;
    return (src->uid == dst->uid) || (src->euid == dst->uid) || (src->uid == dst->euid);
}

// 37
//  	pid_t pid | int sig
// TODO handle pid = 0 (process group) and pid = -1 (all killeable process)
uint32_t syscall_kill(__attribute__((unused)) t_interrupt_data *regs)
{
    int pid = regs->ebx;
	int sig = regs->ecx;
    print_trace("kill %d %x\n", pid, sig);
    if (sig > NSIG || sig < 1)
        return (-EINVAL);
    bool found_one = false;
    t_task *curr = g_task_list;
    if (pid > 0)
    {
        t_task *target = NULL;
        while (curr)
        {
            if ((int)curr->task_id == pid)
            {
                target = curr;
                break;
            }
            curr = curr->next_all_task;
            if (curr == g_task_list) break;
        }
        if (!target)
            return (-ESRCH);
        if (!can_signal(g_curr_task, target))
            return (-EPERM);
        found_one = true;
        enqueue_sig(target, sig);
    }
    else if (pid == 0)
    {
        int target_pgid = g_curr_task->pgid;
        while (curr)
        {
            if ((int)curr->pgid == target_pgid && can_signal(g_curr_task, curr))
            {
                enqueue_sig(curr, sig);
                found_one = true;
            }
            curr = curr->next_all_task;
            if (curr == g_task_list) break;
        }
    }
    else if (pid == -1)
    {
        while (curr)
        {
            if (can_signal(g_curr_task, curr))
            {
                if (curr == g_init_task) continue; //don't signal init
                enqueue_sig(curr, sig);
                found_one = true;
            }
            curr = curr->next_all_task;
            if (curr == g_task_list) break;
        }
    }
    else
    {
        int target_pgid = -pid;
        while (curr)
        {
            if ((int)curr->pgid == target_pgid && can_signal(g_curr_task, curr))
            {
                enqueue_sig(curr, sig);
                found_one = true;
            }
            curr = curr->next_all_task;
            if (curr == g_task_list) break;
        }
    }
    if (!found_one)
    {
        return (-ESRCH);
    }
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
	
    int how = (int)regs->ebx;
    uint32_t *set_u   = (uint32_t *)(uintptr_t)regs->ecx;  // can be NULL
    uint32_t *old_u   = (uint32_t *)(uintptr_t)regs->edx;  // can be NULL
    uint32_t sigsetsize = (uint32_t)regs->esi;

    if (old_u && !user_range_ok((virt_ptr)old_u, sizeof(uint32_t), true, &g_curr_task->proc_memory))
        return (uint32_t)(-EFAULT);
    if (set_u && !user_range_ok((virt_ptr)set_u, sizeof(uint32_t), false, &g_curr_task->proc_memory))
        return (uint32_t)(-EFAULT);

    print_trace("rt_sigprocmask new x%x old x%x how %d size %u\n", set_u ? *set_u : 0, old_u ? *old_u : 0, how, sigsetsize);

    // TODO upgrade to 8 bytes masks, because libc uses that.
    if (sigsetsize < sizeof(uint32_t))
        return (uint32_t)(-EINVAL);

    if (old_u)
    {
        // Return old mask
        *(uint32_t *)(uintptr_t)old_u = g_curr_task->blocked_signals;
    }

    if (!set_u)
        return (0); // just asking for old mask

    uint32_t set = *(uint32_t *)(uintptr_t)set_u;

    // Cannot block SIGKILL and SIGSTOP
    set &= ~SIGBIT(SIGKILL);
    set &= ~SIGBIT(SIGSTOP);

    switch (how)
    {
        //SIG_BLOCK
        case 0:
            g_curr_task->blocked_signals |= set;
            break;
        //SIG_UNBLOCK
        case 1:
            g_curr_task->blocked_signals &= ~set;
            break;
        //SIG_SETMASK
        case 2:
            g_curr_task->blocked_signals = set;
            break;
        default:
            return (uint32_t)(-EINVAL);
    }

    return (0); //success
}

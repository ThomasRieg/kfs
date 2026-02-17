/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signals.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/15 12:46:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/17 03:18:55 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "signals.h"
#include "../tasks/task.h"
#include "../mem_page/mem_paging.h"
#include "../pmm/pmm.h"
#include <stdint.h>
#include <string.h>

__attribute__((noreturn)) static inline void iret_from_frame(t_interrupt_data *frame)
{
	__asm__ volatile(
		"movl %0, %%esp \n"
		"jmp isr_common_epilogue \n"
		:
		: "r"(frame)
		: "memory");
	__builtin_unreachable();
}

static int pick_signal(uint32_t pending_unblocked) {
    for (int sig = 1; sig < NSIG; sig++)
        if (pending_unblocked & SIGBIT(sig))
            return sig;
    return 0;
}

bool map_signal_trampoline(void)
{
    uint32_t *pde = get_pde(SIGNAL_TRAMPOLINE_VA);
    uint32_t *pte = get_pte(SIGNAL_TRAMPOLINE_VA);
    if (!(*pde & PTE_P))
		{
			// allocate the page table itself
			print_trace("trying to allocate new page table to map the signal trampoline at %x by pid %u\n", SIGNAL_TRAMPOLINE_VA, g_curr_task->task_id);
			phys_ptr pde_frame = pmm_alloc_frame();
			if (!pde_frame)
			{
				return (false);
			}
			map_page(pde_frame, pde, PTE_US | PTE_RW | PTE_P); // set permissive flags
			uint32_t *page_table = (uint32_t *)(uintptr_t)(PT_BASE_VA + PDE_INDEX(SIGNAL_TRAMPOLINE_VA) * PAGE_SIZE);
			invalidate_cache(page_table);
			memset(page_table, 0, PAGE_SIZE);
			pte = get_pte(SIGNAL_TRAMPOLINE_VA); // pte was null, because no pde, now we need to set it before the rest of the code
			g_curr_task->proc_memory.physical_pages++;
		}
    map_page(PAGE_ALIGN_DOWN(SIGNAL_TRAMPOLINE_PHYS), pte, PTE_P | PTE_US);
    return (true);
}

void task_terminate_by_signal(t_task *t, int sig)
{
    print_debug("pid %u killed by signal: %d\n", t->task_id, sig);
	disable_interrupts(); // doesn't need to reenable because parent's task will override cpu flags

	if (!g_curr_task->parent_task)
		kernel_panic("parentless process got killed, no process left?\n", NULL);
	
    void adopt_children_list(t_task *adopter, t_task *children);
	adopt_children_list(g_init_task, g_curr_task->children);
	g_curr_task->children = NULL;
	g_curr_task->exit_code = sig;
	g_curr_task->status = STATUS_ZOMBIE;
    enqueue_sig(g_curr_task->parent_task, SIGCHLD);

    // wake any wait4 sleepers
    waitq_wake_all(&g_curr_task->parent_task->wait_child);
	cleanup_task(t);
	yield_to(t->parent_task);
	__builtin_unreachable();
}

void signal_deliver_if_needed(t_interrupt_data *f)
{
    t_task *t = g_curr_task;
    if (!t) return;

    // only when returning to usermode
    if ((f->cs & 3) != 3) return;

    if (t->in_signal) return;

    uint32_t pend = t->pending_signals & ~t->blocked_signals;
    if (!pend) return;

    int sig = pick_signal(pend);
    if (!sig) return; //shouldn't happen

    t_sigaction_k *act = &t->sigact[sig];
    t_sig_handler h = act->handler;

    if (h == SIG_IGN /*SIG_IGN*/) {
        t->pending_signals &= ~SIGBIT(sig);
        return;
    }
    if (h ==SIG_DFL /*SIG_DFL*/) {
        task_terminate_by_signal(t, sig);
        __builtin_unreachable();
    }

    // build user sigframe
    uint32_t old_esp = f->useresp;
    uint32_t new_esp = old_esp - (uint32_t)sizeof(t_sigframe32);

    if (!user_range_ok((virt_ptr)new_esp, sizeof(t_sigframe32), true, &t->proc_memory)) {
        // can't deliver
        task_terminate_by_signal(t, SIGSEGV);
        return;
    }

    t_sigframe32 *uf = (t_sigframe32 *)(uintptr_t)new_esp;

    // write frame in user memory (same address space is active here)
    uf->signum  = (uint32_t)sig;
    uf->retaddr = (uint32_t)SIGNAL_TRAMPOLINE_VA;
    uf->oldmask = t->blocked_signals;
    memcpy(&uf->saved_context, f, sizeof(*f));

    // update task signal state
    t->pending_signals &= ~SIGBIT(sig);
    t->blocked_signals |= SIGBIT(sig);      // block itself during handler
    t->blocked_signals |= act->mask;
    t->in_signal = true;

    // redirect execution to handler
    f->useresp = new_esp;
    f->eip = (uint32_t)(uintptr_t)h;
    print_trace("calling signal handler at %x for signal %d for pid %u\n", f->eip, sig, t->task_id);
    print_trace("uf at %p\n", uf);
    iret_from_frame(f);
}

bool enqueue_sig(t_task *task, int sig)
{
    //if (task->blocked_signals &= SIGBIT(sig))
        //return (false);
    task->pending_signals |= SIGBIT(sig);
    waitq_wake(task); //remove sleep status if task sleeping
    return (true);
}

bool has_pending_signals(t_task *task)
{
    return (task->pending_signals & ~task->blocked_signals);
}

uint32_t flags_first_pending_signal(t_task *task)
{
    uint32_t pend = task->pending_signals & ~task->blocked_signals;
    if (!pend) return (0);
    int sig = pick_signal(pend);
    t_sigaction_k *act = &task->sigact[sig];
    return (act->flags);
}

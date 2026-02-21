/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   waitq.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 04:07:52 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/22 00:07:42 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "waitq.h"
#include "../tasks/task.h"
#include "../common.h"

static void waitq_add(t_waitq *q, t_task *t)
{
    // assume interrupts disabled by caller
    t->wq_node.task = t;
    t->wq_node.next = q->head;
    q->head = &t->wq_node;
    t->sleep_q = q;
}

static void waitq_remove(t_task *t)
{
    // assume interrupts disabled by caller
    t_waitq *q = t->sleep_q;
    if (!q) return;

    t_waitq_node **pp = &q->head;
    while (*pp)
    {
        if ((*pp)->task == t)
        {
            *pp = (*pp)->next;
            break;
        }
        pp = &(*pp)->next;
    }

    t->sleep_q = NULL;
    t->wq_node.next = NULL;
}

void waitq_wake_all(t_waitq *q)
{
    // assume interrupts disabled by caller
    t_waitq_node *n = q->head;
    q->head = NULL;

    while (n)
    {
        t_task *t = n->task;
        t_waitq_node *next = n->next;

        t->sleep_q = NULL;
        t->wq_node.next = NULL;

        if (t->status == STATUS_SLEEP)
        {
            t->status = STATUS_RUNNABLE;
            t->wait_reason = WAIT_NONE;
            add_task_to_runq(t);
        }
        n = next;
    }
}

void waitq_wake_one(t_waitq *q)
{
    // assume interrupts disabled by caller
    t_waitq_node *n = q->head;
    if (!n)
        return;

    q->head = n->next;

    t_task *t = n->task;

    t->sleep_q = NULL;
    t->wq_node.next = NULL;

    if (t->status == STATUS_SLEEP)
    {
        t->status = STATUS_RUNNABLE;
        t->wait_reason = WAIT_NONE;
        add_task_to_runq(t);
    }
}

void sleep_on(t_waitq *q, t_wait_reason reason)
{

    // remove if already sleeping somewhere (shouldn't happen)
    print_trace("task pid %u sleep_on because %u\n", g_curr_task->task_id, (uint32_t)reason);
    if (g_curr_task->sleep_q)
    {
        waitq_remove(g_curr_task);
        print_err("ERROR, task was already sleeping on %p when trying to sleep, task pid %u sleep_on because %u\n", g_curr_task->sleep_q, g_curr_task->task_id, (uint32_t)reason);
    }

    g_curr_task->wait_reason = reason;
    g_curr_task->status = STATUS_SLEEP;
    waitq_add(q, g_curr_task);
    unlink_task_from_runq(g_curr_task);

    // Switch away (yield or schedule)
    yield();
}

//careful, wake_at_tick is the g_tick the task will get woken up anyway, not the timeout, we can do (g_tick + timeout) to get it
void sleep_on_timeout(t_waitq *q, t_wait_reason reason, uint32_t wake_at_tick)
{
    // remove if already sleeping somewhere (shouldn't happen)
    if (g_curr_task->sleep_q)
        waitq_remove(g_curr_task);

    g_curr_task->wait_reason = reason;
    g_curr_task->status = STATUS_SLEEP;
    g_curr_task->sleep_until = wake_at_tick;
    extern void insert_in_sleep_queue(t_task *task);
    insert_in_sleep_queue(g_curr_task);
    g_curr_task->in_sleep_queue = true;
    waitq_add(q, g_curr_task);
    unlink_task_from_runq(g_curr_task);

    yield();
    if (g_curr_task->in_sleep_queue)
    {
        remove_from_sleep_queue(g_curr_task); //remove the timeout
        g_curr_task->in_sleep_queue = false;
    }
    if (g_curr_task->sleep_q)
        waitq_remove(g_curr_task);
}

void waitq_wake(t_task *task)
{
    print_trace("wake: waking up %u\n", task->task_id);
    if (task->sleep_q)
        waitq_remove(task);
    if (task->in_sleep_queue)
    {
        print_trace("wake: manually removing pid %u from sleep queue\n", task->task_id);
        remove_from_sleep_queue(task);
        task->in_sleep_queue = false;
    }

    if (task->status == STATUS_SLEEP)
    {
        task->status = STATUS_RUNNABLE;
        task->wait_reason = WAIT_NONE;
        print_trace("wake: adding pid %u back into the run queue\n", task->task_id);
        add_task_to_runq(task);
    }
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   waitq.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 04:07:52 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/12 05:10:22 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "waitq.h"
#include "../tasks/task.h"

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
    }
}

void sleep_on(t_waitq *q, t_wait_reason reason)
{
    disable_interrupts();

    // remove if already sleeping somewhere (shouldn't happen)
    if (g_curr_task->sleep_q)
        waitq_remove(g_curr_task);

    g_curr_task->wait_reason = reason;
    g_curr_task->status = STATUS_SLEEP;
    waitq_add(q, g_curr_task);

    enable_interrupts();

    // Switch away (yield or schedule)
    yield();
}
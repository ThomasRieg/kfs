/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sleep.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/21 02:11:11 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/22 23:46:46 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../tasks/task.h"
#include "waitq.h"
#include "../common.h"

static t_task *g_sleeping_queue = NULL;

static inline uint32_t ticks_left(uint32_t sleep_until)
{
    return (sleep_until - g_tick);
}

void insert_in_sleep_queue(t_task *task)
{
    uint32_t target_ticks_left = ticks_left(task->sleep_until); //overflow_safe
    if (g_sleeping_queue)
    {
        if (target_ticks_left >= ticks_left(g_sleeping_queue->sleep_until))
        {
            t_task *curr = g_sleeping_queue;
            t_task *prev = g_sleeping_queue;
            while (curr && (target_ticks_left > ticks_left(curr->sleep_until)))
            {
                prev = curr;
                curr = curr->next_sleep_task;
            }
            prev->next_sleep_task = task;
            task->next_sleep_task = curr;
        }
        else
        {
            task->next = g_sleeping_queue;
            g_sleeping_queue = task;
        }
    }
    else
        g_sleeping_queue = task;
}

void remove_from_sleep_queue(t_task *task)
{
    if (!task)
        kernel_panic("remove from sleep queue called with task null\n", NULL);
    if (g_sleeping_queue)
    {
        if (task != g_sleeping_queue)
        {
            t_task *curr = g_sleeping_queue;
            t_task *prev = g_sleeping_queue;
            while (curr && (curr != task))
            {
                prev = curr;
                curr = curr->next_sleep_task;
            }
            prev->next_sleep_task = task->next_sleep_task;
        }
        else
        {
            g_sleeping_queue = task->next_sleep_task;
        }
        task->next_sleep_task = NULL;
    }
}

void sleep_until(t_task *task, uint32_t tick)
{
    task->status = STATUS_SLEEP;
    task->wait_reason = WAIT_SLEEP;
    task->sleep_until = tick;
    insert_in_sleep_queue(task);
    task->in_sleep_queue = true;
    unlink_task_from_runq(task);
    yield();
}

bool time_after_eq_u32(uint32_t a, uint32_t b)
{
    // true if a >= b in modulo-32bit time
    return ((int32_t)(a - b) >= 0);
}

void wake_due_sleeping_tasks()
{
    t_task *curr = g_sleeping_queue;
    //has to do == instead of <= to handle overflows
    while (curr && time_after_eq_u32(g_tick, curr->sleep_until))
    {
        t_task *next = curr->next_sleep_task;
        waitq_wake(curr);
        curr = next;
    }
    if (curr != g_sleeping_queue)
        g_sleeping_queue = curr;
}
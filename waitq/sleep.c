/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sleep.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/21 02:11:11 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/26 05:09:22 by thrieg           ###   ########.fr       */
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
	if (task->in_sleep_queue || task->next_sleep_task)
		kernel_panic("double sleep insert", NULL);
	uint32_t target_ticks_left = ticks_left(task->sleep_until); // overflow_safe
	if (g_sleeping_queue)
	{
		if (target_ticks_left > ticks_left(g_sleeping_queue->sleep_until))
		{
			t_task *curr = g_sleeping_queue;
			t_task *prev = g_sleeping_queue;
			while (curr && (target_ticks_left > ticks_left(curr->sleep_until)))
			{
				prev = curr;
				curr = curr->next_sleep_task;
			}
			prev->next_sleep_task = task;
			if (curr == task) kernel_panic("double sleep insert", NULL);
			task->next_sleep_task = curr;
		}
		else
		{
			task->next_sleep_task = g_sleeping_queue;
			g_sleeping_queue = task;
		}
	}
	else
	{
		g_sleeping_queue = task;
		task->next_sleep_task = NULL;
	}
}

void remove_from_sleep_queue(t_task *task)
{
	print_trace("removing %p pid %u from sleep queue, next_sleep_task %p\n", task, task->task_id, task->next_sleep_task);
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
	print_trace("g_sleeping_queue became %p\n", g_sleeping_queue);
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
	// has to do == instead of <= to handle overflows
	while (curr && time_after_eq_u32(g_tick, curr->sleep_until))
	{
		t_task *next = curr->next_sleep_task;
		print_trace("wake_scan: curr=%u(%p) next=%p head=%p tick=%u sleep_until=%u\n",
					curr->task_id, curr, curr->next_sleep_task, g_sleeping_queue,
					g_tick, curr->sleep_until);
		curr->in_sleep_queue = false; // important or waitq-wake will corrupt the sleeping list
		curr->sleep_until = 0;
		curr->next_sleep_task = NULL;

		waitq_wake(curr);
		curr = next;
	}
	if (curr != g_sleeping_queue)
		g_sleeping_queue = curr;
}
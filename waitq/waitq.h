/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   waitq.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 04:00:53 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/26 15:26:22 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WAITQ_H
#define WAITQ_H

#include "../common.h"

typedef enum wait_reason
{
	WAIT_NONE,
	WAIT_CHILD,
	WAIT_PIPE_READ,
	WAIT_PIPE_WRITE,
	WAIT_TTY_READ,
	WAIT_TTY_WRITE,
	WAIT_SLEEP,
	WAIT_SOCK_CONNECT,
	WAIT_SOCK_ACCEPT,
	WAIT_SOCK_READ,
	WAIT_SOCK_WRITE,
} t_wait_reason;

typedef struct task t_task;

typedef struct s_waitq_node
{
	struct s_waitq_node *next;
	t_task *task;
} t_waitq_node;

typedef struct s_waitq
{
	t_waitq_node *head;
} t_waitq;

static inline void waitq_init(t_waitq *q) { q->head = NULL; }

void waitq_wake_all(t_waitq *q);

void waitq_wake_one(t_waitq *q);

void sleep_on(t_waitq *q, t_wait_reason reason);

void sleep_on_timeout(t_waitq *q, t_wait_reason reason, uint32_t wake_at_tick);

void waitq_wake(t_task *task);

void wake_due_sleeping_tasks();

void sleep_until(t_task *task, uint32_t tick);

void remove_from_sleep_queue(t_task *task);

// true is tick a is after tick b, "safe" with overflow if it doesn't overflow by more than 2 billion
bool time_after_eq_u32(uint32_t a, uint32_t b);

#endif
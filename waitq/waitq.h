/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   waitq.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 04:00:53 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/15 20:30:12 by thrieg           ###   ########.fr       */
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

void waitq_wake(t_task *task);

#endif
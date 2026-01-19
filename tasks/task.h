/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:55:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/19 19:50:07 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TASK_H
#define TASK_H

#define TASK_STACK_SIZE 100	  // pages as unit
#define NB_TICKS_PER_TASK 500 // nb of timer irq before we context switch to next task

enum open_file_type
{
	FILE_REGULAR,
	FILE_PIPE
};

enum task_status
{
	STATUS_RUNNABLE,
	STATUS_SLEEP,
	STATUS_ZOMBIE,
};

struct pipe
{
	char buffer[8192];
	unsigned int buffer_size;
};

struct inode
{
	unsigned int filesystem_index;
	unsigned int inode_number;
};

struct open_file
{
	enum open_file_type type;
	union
	{
		// regular
		struct inode inode;
		// pipe
		struct
		{
			struct pipe *pipe;
		};
	};
};

typedef struct task
{
	enum task_status status;
	unsigned int task_id;
	struct task *parent_task;
	unsigned int user_id;
	unsigned int pending_signals;
	struct open_file open_files[10];
	phys_ptr pd;
	uint32_t k_esp;
	uint8_t k_stack[8096]; // stack tss will returns to on interrupt
	struct task *next;	   // circular linked list, TODO do better
						   // TODO struct to keep track of reserved and allocated memory (used to free everything on process end and allocate pmm on page fault)
} t_task;

extern t_task *g_curr_task;

#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:55:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/16 04:44:30 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TASK_H
#define TASK_H

#define TASK_STACK_SIZE 100 // pages as unit

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

struct regs
{
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;
	unsigned int esi;
	unsigned int edi;
	unsigned int ebp;
	unsigned int esp;
	unsigned int eip;
};

typedef struct task
{
	enum task_status status;
	unsigned int task_id;
	struct task *parent_task;
	unsigned int user_id;
	unsigned int pending_signals;
	struct open_file open_files[10];
	struct regs regs;
	phys_ptr pd;
	//TODO struct to keep track of reserved and allocated memory (used to free everything on process end and allocate pmm on page fault)
} t_task;

#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:55:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/20 15:33:57 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TASK_H
#define TASK_H

#define TASK_STACK_SIZE 100	  // pages as unit
#define NB_TICKS_PER_TASK 500 // nb of timer irq before we context switch to next task

typedef void (*t_sig_handler)(int);
#define SIG_IGN 1 // ingore this signal
#define SIG_DFL 0 // default behavior (stop process)

#define SIGCHLD 17 // lsit here https://man7.org/linux/man-pages/man7/signal.7.html

enum open_file_type
{
	FILE_REGULAR,
	FILE_PIPE,
};

enum task_status
{
	STATUS_RUNNABLE,
	STATUS_SLEEP,
	STATUS_ZOMBIE,
};

enum wait_reason
{
	WAIT_NONE,
	WAIT_CHILD,
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

// task objects have to be allocated with vmalloc
typedef struct task
{
	enum task_status status;
	enum wait_reason wait_reason;
	unsigned int task_id;
	struct task *parent_task;  // should NEVER be null, put init if parent dies (except for init)
	struct task *children;	   // head of linked list
	struct task *next_sibling; // link in parent list
	unsigned int uid;
	unsigned int euid;
	unsigned int suid;
	unsigned int exit_code;
	unsigned int pending_signals;
	struct open_file open_files[10];
	phys_ptr pd;
	uint32_t k_esp;
	uint8_t k_stack[8096]; // stack tss will returns to on interrupt
	t_sig_handler sig_handlers[32];
	struct task *next; // circular linked list, TODO do better
					   // TODO struct to keep track of reserved and allocated memory (used to free everything on process end and allocate pmm on page fault)
} t_task;

extern t_task *g_curr_task;

void schedule_next_task();
void context_switch(t_task *next);
void cleanup_task(t_task *task);
void task_reap_zombie(t_task *t);

#endif
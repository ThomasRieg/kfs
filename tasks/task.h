/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:55:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/23 14:54:57 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TASK_H
#define TASK_H

#include "../mem_page/mem_defines.h"
#include "../libk/vec.h"
#include "../dt/dt.h"

#define TASK_STACK_SIZE (100u * PAGE_SIZE)
#define TASK_VA_ENTRYPOINT 0x200000u
#define TASK_STACK_TOP KERNEL_VIRT_BASE - PAGE_SIZE
#define NB_TICKS_PER_TASK 1u // nb of timer irq before we context switch to next task

typedef void (*t_sig_handler)(int);
#define SIG_IGN 1u // ingore this signal
#define SIG_DFL 0u // default behavior (stop process)

#define SIGCHLD 17u // lsit here https://man7.org/linux/man-pages/man7/signal.7.html

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

typedef enum e_vma_backing
{
	VMA_ANON = 0,
	VMA_FILE = 1, // future
	VMA_DEV = 2,  // future (MMIO)
} t_vma_backing;

typedef struct s_vma
{
	virt_ptr start; // inclusive, page-aligned
	virt_ptr end;	// exclusive, page-aligned

	uint32_t prots;	// PROT_READ/WRITE/EXEC
	uint32_t flags; // MAP_PRIVATE/MAP_SHARED/MAP_FIXED/...
	t_vma_backing backing;

	struct s_vma *next;
} t_vma;

typedef struct s_mm
{
	t_vma *vma_list; // sorted linked list
	virt_ptr user_stack_top;
	virt_ptr user_stack_bot;

	virt_ptr heap_current;   // used by brk
	uint32_t reserved_pages; // optional stats/counters
	uint32_t physical_pages; // optional stats/counters
} t_mm;

// task objects have to be allocated with vmalloc (or preferably vcalloc)
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
	unsigned int gid;
	unsigned int egid;
	unsigned int exit_code;
	unsigned int pending_signals;
	struct open_file open_files[10];
	phys_ptr pd;
	uint32_t k_esp;
	uint8_t k_stack[8096]; // stack tss will returns to on interrupt
	t_sig_handler sig_handlers[32];
	struct task *next; // circular linked list, TODO do better
	t_mm proc_memory;
} t_task;

extern t_task *g_curr_task;

void schedule_next_task();
void context_switch(t_task *next);
void cleanup_task(t_task *task);
void task_reap_zombie(t_task *t);
bool setup_process(t_task *task, t_task *parent, uint32_t user_id, struct VecU8 *binary);

#endif

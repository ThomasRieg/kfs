/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */ /*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */ /*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:55:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/04 17:20:25 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TASK_H
#define TASK_H

#include "../mem_page/mem_defines.h"
#include "../libk/vec.h"
#include "../dt/dt.h"
#include "../fd/fd.h"
#include "../waitq/waitq.h"
#include "../signals/signals.h"
#include <stdint.h>

#define TASK_STACK_SIZE (100u * PAGE_SIZE)
#define TASK_VA_ENTRYPOINT 0x200000u
#define TASK_STACK_TOP KERNEL_VIRT_BASE - (PAGE_SIZE * 2) // just below signal trampoline
#define NB_TICKS_PER_TASK 1u							  // nb of timer irq before we context switch to next task
#define MAX_OPEN_FILES 256u
#define TASK_KERNEL_SIZE 16384

enum task_status
{
	STATUS_RUNNABLE,
	STATUS_SLEEP,
	STATUS_ZOMBIE,
	STATUS_STOPPED,
};

/*struct open_file
{
	unsigned int ref_count;
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
};*/

typedef enum e_vma_backing
{
	VMA_ANON = 0,
	VMA_FILE = 1, // future
	VMA_DEV = 2,  // future (MMIO)
} t_vma_backing;

struct shm_page
{
	phys_ptr frame;
	virt_ptr virt_start;
	struct shm_page *next; // sorted by increasing virt_start
};

// backing object for shared anon mem
typedef struct s_shm_anon
{
	uint32_t refcnt;
	// maps page index (within VMA) -> phys frame
	// linked list for now, later a hash/rbtree
	struct shm_page *pages;
} t_shm_anon;

typedef struct s_vma
{
	virt_ptr start; // inclusive, page-aligned
	virt_ptr end;	// exclusive, page-aligned

	uint32_t prots; // PROT_READ/WRITE/EXEC
	uint32_t flags; // MAP_PRIVATE/MAP_SHARED/MAP_FIXED/...
	t_vma_backing backing;
	void *backing_obj;

	struct s_vma *next;
} t_vma;

typedef struct s_mm
{
	t_vma *vma_list; // sorted linked list
	virt_ptr user_stack_top;
	virt_ptr user_stack_bot;

	virt_ptr heap_current;	 // used by brk
	uint32_t reserved_pages; // optional stats/counters
	uint32_t physical_pages; // optional stats/counters
} t_mm;

// task objects have to be allocated with vmalloc (or preferably vcalloc)
typedef struct task
{
	enum task_status status;
	enum wait_reason wait_reason;
	unsigned int task_id;
	unsigned int task_group_id;
	struct task *parent_task;	  // should NEVER be null, put init if parent dies (except for init)
	struct task *children;		  // head of linked list
	struct task *next_sibling;	  // link in parent list
	struct task *next_all_task;	  // link of g_task_list, circular linked list
	struct task *next_sleep_task; // link of g_sleeping_queue, null if not sleeping
	uint32_t sleep_until;		  // g_tick where we need to wake this task
	bool in_sleep_queue;
	unsigned int uid;
	unsigned int euid;
	unsigned int suid;
	unsigned int gid;
	unsigned int egid;
	unsigned int pgid;		 // process group id
	unsigned int exit_code;	 // status returned by wait, signal on last byte, exit number on second to last byte
	unsigned int stopped_by; // 0 if STOP already handled in wait4 or if program not stopped, else, the number of the signal stopping it
	uint32_t pending_signals;
	uint32_t blocked_signals; // mask of blocked signals (can't add them to pending if is this mask)
	bool in_signal;			  // temporary mostly for debug
	t_file *open_files[MAX_OPEN_FILES];
	unsigned int cwd_inode_nr;
	phys_ptr pd;
	struct user_desc user_gdt_segment;
	uint32_t k_esp;
	uint32_t k_context_esp;			   // saved kernel context stack pointer (for yield)
	uint8_t k_stack[TASK_KERNEL_SIZE]; // stack tss will returns to on interrupt
	t_sigaction_k sigact[NSIG];		   // index by sig (0 unused), memset to 0 creates correct behavior (handler SIG_DFL, mask and flag 0)
	struct task *next;				   // circular linked list, NULL means task not in run queue
	t_mm *proc_memory;
	t_waitq_node wq_node; // used when sleeping
	t_waitq *sleep_q;	  // which queue we’re in (for removal)
	t_waitq wait_child;	  // waitq owned by this struct,
} t_task;

struct process_strings
{
	unsigned int string_count;
	struct VecU8 string_data;
};

extern t_task *g_curr_task;
extern t_task *g_init_task; // pid 1
extern uint32_t g_next_pid;
extern uint32_t g_tick;
extern t_task *g_to_schedule;
extern t_task *g_task_list; // head of linked list of every task, no matter their state

void schedule_next_task();
void context_switch(t_task *next);
void cleanup_task(t_task *task);
void task_reap_zombie(t_task *t);
bool setup_process(t_task *task, t_task *parent, uint32_t user_id, struct VecU8 *binary);
void add_child(t_task *parent, t_task *child);
void free_vmas(t_mm *mm);
void free_vma(t_vma *curr);
void yield();
void yield_to(t_task *next_exec);
void unlink_task_from_runq(t_task *task);
void add_task_to_runq(t_task *task);
t_vma *vma_clone(const t_vma *v);
t_task *find_task_by_pid(int pid);
bool pgid_exists(unsigned int pgid);
void task_init_kernel_context(t_task *t);

extern void switch_to(uint32_t *prev_kctx_esp, uint32_t next_kctx_esp);

#endif

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:25:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/03 16:21:32 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mmap.h"
#include "../common.h"
#include "../tasks/task.h"
#include "../vmalloc/vmalloc.h"
#include "../mem_page/mem_paging.h"
#include "../errno.h"

static int vma_overlaps(t_mm *mm, uintptr_t start, uintptr_t end)
{
	for (t_vma *v = mm->vma_list; v; v = v->next)
	{
		uintptr_t vs = (uintptr_t)v->start;
		uintptr_t ve = (uintptr_t)v->end;
		if (start < ve && end > vs) // half-open interval overlap
			return 1;
	}
	return 0;
}

static void find_insert_pos(t_mm *mm, uintptr_t start, t_vma **prev, t_vma **next)
{
	*prev = NULL;
	*next = mm->vma_list;
	while (*next && (uintptr_t)(*next)->start < start)
	{
		*prev = *next;
		*next = (*next)->next;
	}
}

// prev: previous link, or NULL if we become the first reserved zone
// next: next link, or NULL if we become the last reserved zone
// prev and next are undefined when calling the function, the function will fill them
void *find_first_available(void *start, uint32_t len, t_mm *mm, t_vma **prev, t_vma **next)
{
	t_vma *curr = mm->vma_list;
	*prev = NULL;
	*next = NULL;
	while (curr)
	{
		if ((uintptr_t)start + len >= (uintptr_t)start && (uintptr_t)start + len <= (uintptr_t)curr->start)
		{
			*next = curr;
			return (start);
		}
		else
		{
			start = page_align_up(curr->end);
		}
		*prev = curr;
		curr = curr->next;
	}
	if (((uintptr_t)start + (uintptr_t)len) < (uintptr_t)start)
		return NULL; // overflow
	if (((uintptr_t)start + (uintptr_t)len) <= (uintptr_t)KERNEL_VIRT_BASE)
		return start;
	else
		return NULL;
}

// mm is task.mproc_memory (g_curr_task->proc_memory)
void *mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset, t_mm *mm)
{
	void *original_addr = addr;
	if (offset || fd > 0)
		return ((void *)-ENOSYS); // not supported yet, add it when (if) we add memory with fd backing
	if (!!(flags & MAP_PRIVATE) == !!(flags & MAP_SHARED))
		return ((void *)-EINVAL); // both 0 or both 1
	if (!length)
		return ((void *)-EINVAL); // 0 lenght mapping not allowed
	if ((uintptr_t)addr >= KERNEL_VIRT_BASE)
		return ((void *)-EFAULT);
	if (length >= (KERNEL_VIRT_BASE - USERLAND_HEAP_START_VA))
		return ((void *)-ENOMEM); // not enough memory left
	if ((uintptr_t)addr < USERLAND_HEAP_START_VA)
		addr = (virt_ptr)USERLAND_HEAP_START_VA;
	if (length % PAGE_SIZE)
		length = (length / PAGE_SIZE + 1) * PAGE_SIZE; // align up lenght to page_size
	addr = page_align_up(addr);

	t_vma *prev;
	t_vma *next;
	void *available_start;
	// disable_interrupts();
	if (flags & MAP_FIXED)
	{
		uintptr_t start = (uintptr_t)original_addr;
		uintptr_t end = start + length;
		if (end < start)
		{
			// enable_interrupts();
			return ((void *)-ERANGE); // overflow
		}
		if (original_addr != addr)
		{
			// enable_interrupts();
			return ((void *)-EINVAL); // start changed, therefore invalid (not alligned, too low...)
		}

		if (vma_overlaps(mm, start, end))
		{
			// enable_interrupts();
			return ((void *)-EINVAL); // zone already reserved, TODO apparently posix wants us to just override the entry/entries that reserved this zone, idk if it's worth doing
		}

		find_insert_pos(mm, start, &prev, &next);
		available_start = original_addr;
	}
	else
	{
		available_start = find_first_available(addr, length, mm, &prev, &next);
		if (!available_start)
		{
			// enable_interrupts();
			return ((void *)-ENOMEM);
		}
	}
	t_vma *new = vmalloc(sizeof(*new));
	if (!new)
	{
		// enable_interrupts();
		return ((void *)-ENOMEM);
	}
	if (prev)
		prev->next = new;
	else
		mm->vma_list = new;
	new->next = next;
	new->prots = prot;
	new->flags = flags;
	new->backing = VMA_ANON; // TODO handle other backing
	if (new->flags & MAP_SHARED)
	{
		if (new->backing == VMA_ANON)
		{
			new->backing_obj = vmalloc(sizeof(t_shm_anon));
			if (!new->backing_obj)
				return (vfree(new), (void *)-ENOMEM); // enable interrupts
			((t_shm_anon *)(new->backing_obj))->pages = NULL;
			((t_shm_anon *)(new->backing_obj))->refcnt = 1;
		}
	}
	new->start = available_start;
	new->end = (virt_ptr)((uintptr_t)available_start + length);

	mm->reserved_pages += length / PAGE_SIZE;
	// enable_interrupts();
	return (available_start);
}
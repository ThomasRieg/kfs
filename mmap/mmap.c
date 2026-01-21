/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mmap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:25:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/21 01:53:50 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mmap.h"
#include "../common.h"
#include "../tasks/task.h"
#include "../vmalloc/vmalloc.h"
#include "../mem_page/mem_paging.h"

static int vma_overlaps(t_mm *mm, uintptr_t start, uintptr_t end)
{
    for (t_vma *v = mm->vma_list; v; v = v->next)
    {
        uintptr_t vs = (uintptr_t)v->start;
        uintptr_t ve = (uintptr_t)v->end;
        if (start < ve && end > vs)   // half-open interval overlap
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

//prev: previous link, or NULL if we become the first reserved zone
//next: next link, or NULL if we become the last reserved zone
//prev and next are undefined when calling the function, the function will fill them
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
    if ((KERNEL_VIRT_BASE - len > (uintptr_t)start))
        return (start);
    else
        return (NULL);
}

void *mmap(void *addr, uint32_t length, int prot, int flags, int fd, uint32_t offset)
{
    void *original_addr = addr;
    if (offset || fd > 0)
        return (MAP_FAILED); //not supported yet, add it when (if) we add memory with fd backing
    if (!length)
        return (MAP_FAILED); //0 lenght mapping not allowed
    if ((uintptr_t)addr >= KERNEL_VIRT_BASE)
        return (MAP_FAILED); //g_curr_task safe to use with interrupts disabled because everytime this process is called it g_curr_task must be the same
    if (length >= (KERNEL_VIRT_BASE - USERLAND_HEAP_START_VA))
        return (MAP_FAILED); //not enough memory left
    if ((uintptr_t)addr < USERLAND_HEAP_START_VA)
        addr = USERLAND_HEAP_START_VA;
    if (length % PAGE_SIZE)
        length = (length / PAGE_SIZE + 1) * PAGE_SIZE; //align up lenght to page_size
    addr = page_align_up(addr);

    t_vma *prev;
    t_vma *next;
    void *available_start;
    disable_interrupts();
    if (flags & MAP_FIXED)
    {
        uintptr_t start = (uintptr_t)original_addr;
        uintptr_t end = start + length;
        if (end < start)
        {
            enable_interrupts();
            return (MAP_FAILED); //overflow
        }
        if (original_addr != addr)
        {
            enable_interrupts();
            return (MAP_FAILED); //start changed, therefore invalid (not alligned, too low...)
        }

        if (vma_overlaps(&g_curr_task->proc_memory, start, end))
        {
            enable_interrupts();
            return (MAP_FAILED); //zone already reserved, TODO apparently posix wants us to just override the entry/entries that reserved this zone, idk if it's worth doing
        }

        find_insert_pos(&g_curr_task->proc_memory, start, &prev, &next);
        available_start = original_addr;
    }
    else
    {
        available_start = find_first_available(addr, length, &g_curr_task->proc_memory, &prev, &next);
        if (!available_start)
        {
            enable_interrupts();
            return (MAP_FAILED);
        }
    }
    t_vma *new = vmalloc(sizeof(*new));
    if (!new)
    {
        enable_interrupts();
        return (MAP_FAILED);
    }
    if (prev)
        prev->next = new;
    else
        g_curr_task->proc_memory.vma_list = new;
    new->next = next;
    new->prots = prot;
    new->flags = flags;
    new->backing = VMA_ANON;
    new->start = available_start;
    new->end = (virt_ptr)((uintptr_t)available_start + length);

    g_curr_task->proc_memory.reserved_pages += length / PAGE_SIZE;
    enable_interrupts();
    return (available_start);
}
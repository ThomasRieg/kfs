/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:52:50 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/15 19:18:40 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../common.h"
#include "task.h"
#include "../pmm/pmm.h"
#include "../mem_page/mem_paging.h"
#include "../mem_page/mem_defines.h"
#include "../libk/libk.h"

static uint32_t next_pid;

phys_ptr allocate_stack(uint32_t nb_pages)
{
}

phys_ptr copy_current_pd()
{
	phys_ptr pd = pmm_alloc_frame();
	if (!pd)
		return (0);
	disable_interrupts();
	virt_ptr tmp = kmap(pd);
	memcpy(tmp, PD_VA, PAGE_SIZE);
	kunmap();
	enable_interrupts();
	
}

bool setup_process(t_task *task)
{
	task->pending_signals = 0;
	task->task_id = next_pid++;
	task->pd = pmm_alloc_frame();
	if (!task->pd)
		return (false);
	phys_ptr first_pt = pmm_alloc_frame();
	if (!first_pt)
	{
		pmm_free_frame(task->pd);
		return (false);
	}
	disable_interrupts();
	virt_ptr tmp = kmap(task->pd);
	memset(tmp, 0, PAGE_SIZE);
	uint32_t *pd_entry = (uint32_t *)tmp;
	pd_entry[0] = (first_pt & 0xFFFFF000u) | PTE_P | PTE_RW | PTE_US;
	pd_entry[1023] = (task->pd & 0xFFFFF000u) | PTE_P | PTE_RW;
	kunmap();
	enable_interrupts();
	phys_ptr frames[TASK_STACK_SIZE];
	uint32_t nb_frames = 0;
	for (nb_frames; nb_frames < TASK_STACK_SIZE; nb_frames++)
	{
		frames[nb_frames] = pmm_alloc_frame();
		if (!frames[nb_frames])
		{
			for (uint32_t i = 0; i < nb_frames; i++)
			{
				pmm_free_frame(frames[i]);
			}
			pmm_free_frame(task->pd);
			pmm_free_frame(first_pt);
			return (false);
		}
	}
	disable_interrupts();
	tmp = kmap(first_pt);
	memset(tmp, 0, PAGE_SIZE);
	uint32_t *pt_entry = (uint32_t *)tmp;
	for (uint32_t i = 0; i < nb_frames; i++)
	{
		pt_entry[i] = (frames[i] & 0xFFFFF000u) | PTE_P | PTE_RW | PTE_US;
	}
	kunmap();
	enable_interrupts();
}
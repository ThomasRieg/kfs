/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   task.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 17:52:50 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/16 04:41:51 by thrieg           ###   ########.fr       */
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

//will copy the cr3 currently loaded on the cpu
phys_ptr copy_current_pd()
{
	phys_ptr pd = pmm_alloc_frame();
	if (!pd)
		return (0);
	disable_interrupts();
	virt_ptr tmp = kmap(pd);
	memset(tmp, 0, PAGE_SIZE);
	uint32_t *pde = (uint32_t *)tmp;
	for (uint32_t i = 0; i < 1023; i++)
	{
		uint32_t *kernel_pde = (uint32_t *)PD_VA;
		if ((kernel_pde[i] & PTE_P) && !(kernel_pde[i] & PTE_US))
			pde[i] = kernel_pde[i]; //copy only kernel pages to avoid inter-process shared memory
	}
	pd = (pd & 0xFFFFF000u) | PTE_P | PTE_RW;
	pde[1023] = pd; //recursive mapping
	kunmap();
	enable_interrupts();
	return (pd);
}

bool setup_process(t_task *task)
{
	task->pending_signals = 0;
	task->task_id = next_pid++;
	task->pd = copy_current_pd();
}
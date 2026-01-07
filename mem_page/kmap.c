/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kmap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 19:38:31 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 13:19:33 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mem_defines.h"
#include "mem_paging.h"
#include <stdint.h>

__attribute__((aligned(4096), section(".kmap"))) static uint8_t kmap_page[PAGE_SIZE];
void *kmap(phys_ptr physical_address)
{
	virt_ptr va = &kmap_page[0];
	uint32_t *pte = get_pte(va);

	physical_address &= 0xFFFFF000u;
	*pte = physical_address | PTE_P | PTE_RW;

	invalidate_cache(va);
	return (va);
}

void kunmap(void)
{
	virt_ptr va = &kmap_page[0];
	uint32_t *pte = get_pte(va);

	*pte &= 0;
	invalidate_cache(va);
}
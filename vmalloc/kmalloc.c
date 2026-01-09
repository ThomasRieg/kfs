/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kmalloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 16:19:24 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/09 16:32:08 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vmalloc_private.h"

virt_ptr kmalloc(uint32_t size)
{
	if (!size || size % ALLIGN_BYTES)
		size = ((size / ALLIGN_BYTES) + 1) * ALLIGN_BYTES;
	void *ret = NULL;
	if (size > SMALL_SIZE_MAX)
	{
		ret = add_large(size, PTE_RW | MMAP_CONTIG);
	}
	else if (size > TINY_SIZE_MAX)
	{
		ret = add_small(size, PTE_RW | MMAP_CONTIG);
	}
	else
	{
		ret = add_tiny(size, PTE_RW | MMAP_CONTIG);
	}
	return ((virt_ptr)ret);
}

virt_ptr kcalloc(uint32_t nmemb, uint32_t size)
{
	uint32_t real_size = nmemb * size;
	virt_ptr malloc_ret;

	if (real_size % ALLIGN_BYTES)
		real_size = ((real_size / ALLIGN_BYTES) + 1) * ALLIGN_BYTES;
	malloc_ret = kmalloc(real_size);
	if (!malloc_ret)
		return (NULL);
	memset(malloc_ret, 0, real_size);
	return (malloc_ret);
}

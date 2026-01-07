/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vmalloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/17 15:02:55 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 18:00:10 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vmalloc_private.h"

virt_ptr vmalloc(uint32_t size)
{
	if (!size || size % ALLIGN_BYTES)
		size = ((size / ALLIGN_BYTES) + 1) * ALLIGN_BYTES;
	void *ret = NULL;
	if (size > SMALL_SIZE_MAX)
	{
		ret = add_large(size);
	}
	else if (size > TINY_SIZE_MAX)
	{
		ret = add_small(size);
	}
	else
	{
		ret = add_tiny(size);
	}
	return ((virt_ptr)ret);
}

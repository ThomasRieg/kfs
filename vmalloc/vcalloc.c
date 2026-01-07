/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vcalloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 11:47:16 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 18:00:32 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vmalloc_private.h"

virt_ptr vcalloc(uint32_t nmemb, uint32_t size)
{
	uint32_t real_size = nmemb * size;
	virt_ptr malloc_ret;

	if (real_size % ALLIGN_BYTES)
		real_size = ((real_size / ALLIGN_BYTES) + 1) * ALLIGN_BYTES;
	malloc_ret = vmalloc(real_size);
	if (!malloc_ret)
		return (NULL);
	memset(malloc_ret, 0, real_size);
	return (malloc_ret);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vfree.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/19 15:44:05 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 18:37:31 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vmalloc_private.h"

// returns non-0 (1) when the function munmap the zone
int defrag_zone(t_zone *zone)
{
	t_header *header = (t_header *)(zone + 1);
	char *last_adr = ((char *)zone) + zone->size - sizeof(t_header);
	while ((((char *)header) + header->size) < last_adr)
	{
		t_header *next_header = (t_header *)(((char *)header) + sizeof(t_header) + header->size);
		if (!header->occupied && !next_header->occupied)
		{
			header->size = header->size + sizeof(t_header) + next_header->size;
		}
		else
		{
			header = next_header;
		}
	}
	// if only 1 free header left, mmunmap the zone
	if (header == (t_header *)(zone + 1) && !header->occupied)
	{
		if (zone->type == E_SMALL)
		{
			if (g_state.small_zone == zone)
			{
				if (zone->next)
					g_state.small_zone = zone->next;
				else
					return (0); // last small zone, don't deallocate to avoid mmap stress
			}
		}
		else if (zone->type == E_TINY)
		{
			if (g_state.tiny_zone == zone)
			{
				if (zone->next)
					g_state.tiny_zone = zone->next;
				else
					return (0); // last tiny zone, don't deallocate to avoid mmap stress
			}
		}
		if (zone->prev)
			zone->prev->next = zone->next;
		if (zone->next)
			zone->next->prev = zone->prev;
		kmunmap(zone, zone->size / PAGE_SIZE);
		return (1);
	}
	return (0);
}

void vfree(void *ptr)
{
	if (!ptr)
		return;
	t_header *header = ((t_header *)ptr) - 1;
	if (header->zone->type == E_LARGE)
	{
		if (header->zone->prev)
			header->zone->prev->next = header->zone->next;
		if (header->zone->next)
			header->zone->next->prev = header->zone->prev;
		if (header->zone == g_state.large_zone)
		{
			if (g_state.large_zone->next)
				g_state.large_zone = g_state.large_zone->next;
			else
				g_state.large_zone = NULL;
		}
		void *ptr = header->zone;
		size_t len = header->zone->size;
		kmunmap(ptr, len / PAGE_SIZE);
		return;
	}
	else if (header->zone->type == E_SMALL || header->zone->type == E_TINY)
	{
		header->occupied = false;
		if (defrag_zone(header->zone))
			return;
	}
	else
		return;
}

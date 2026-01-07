/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_state.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/18 16:44:13 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 18:38:04 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vmalloc_private.h"

t_state g_state = {.tiny_zone = NULL, .small_zone = NULL, .large_zone = NULL};

// inserts a page in the linked list whilst keeping the zones sorted in ascending address
static void zone_insert_sorted(t_zone **head, t_zone *z)
{
	t_zone *cur;

	z->prev = NULL;
	z->next = NULL;

	if (*head == NULL)
	{
		*head = z;
		return;
	}

	cur = *head;

	// insert before head
	if ((uintptr_t)z < (uintptr_t)cur)
	{
		z->next = cur;
		cur->prev = z;
		*head = z;
		return;
	}

	// walk until the next node is after z (or end)
	while (cur->next && (uintptr_t)cur->next < (uintptr_t)z)
		cur = cur->next;

	// insert after cur
	z->next = cur->next;
	z->prev = cur;
	if (cur->next)
		cur->next->prev = z;
	cur->next = z;
}

// only call this for TINY or SMALL
void *add_page(t_type type)
{
	uint32_t size;
	if (type == E_TINY)
		size = ((((((TINY_SIZE_MAX + sizeof(t_header)) * 100) + sizeof(t_zone))) / PAGE_SIZE) + 1) * PAGE_SIZE;
	else if (type == E_SMALL)
		size = ((((((SMALL_SIZE_MAX + sizeof(t_header)) * 100) + sizeof(t_zone))) / PAGE_SIZE) + 1) * PAGE_SIZE;
	else
		return (NULL); // TODO handle error somehow
	t_zone *ptr = kmmap(NULL, size / PAGE_SIZE, PTE_RW);
	if (ptr == NULL)
		return (NULL);
	ptr->size = size;
	if (type == E_TINY)
	{
		ptr->type = E_TINY;
		ptr->prev = NULL;
		t_header *header = (t_header *)(ptr + 1);
		header->occupied = false;
		header->size = size - sizeof(t_zone) - sizeof(t_header);
		header->zone = ptr;
		zone_insert_sorted(&g_state.tiny_zone, ptr);
	}
	else if (type == E_SMALL)
	{
		ptr->type = E_SMALL;
		ptr->prev = NULL;
		t_header *header = (t_header *)(ptr + 1);
		header->occupied = false;
		header->size = size - sizeof(t_zone) - sizeof(t_header);
		header->zone = ptr;
		zone_insert_sorted(&g_state.small_zone, ptr);
	}
	return (ptr);
}

// returns the start of the buffer (returned by malloc)
void *add_large(uint32_t size)
{
	size = (((size + sizeof(t_header) + sizeof(t_zone)) / PAGE_SIZE) + 1) * PAGE_SIZE;
	t_zone *ptr = kmmap(NULL, size / PAGE_SIZE, PTE_RW);
	if (ptr == NULL)
		return (NULL);
	ptr->size = size;
	ptr->type = E_LARGE;
	ptr->prev = NULL;
	t_header *header = (t_header *)(ptr + 1);
	header->occupied = true;
	header->size = size - sizeof(t_zone) - sizeof(t_header);
	header->zone = ptr;
	zone_insert_sorted(&g_state.large_zone, ptr);
	return ((void *)(header + 1));
}

// returns the start of the buffer (returned by malloc)
// size has to be alligned
void *add_small(uint32_t size)
{
	t_zone *zone;
	t_header *hdr;
	void *result = NULL;

	zone = g_state.small_zone;
	while (zone && !result)
	{
		char *zone_start = (char *)zone;
		char *zone_end = zone_start + zone->size;

		// First header is right after the zone struct
		hdr = (t_header *)(zone + 1);

		while ((char *)hdr + sizeof(t_header) <= zone_end)
		{
			if (!hdr->occupied && hdr->size >= size)
			{
				// Found a free block big enough
				uint32_t remaining = hdr->size - size;

				if (remaining > sizeof(t_header))
				{
					// Split the block: create a new header in the remaining space
					char *new_addr = (char *)(hdr + 1) + size;
					t_header *newhdr = (t_header *)new_addr;

					newhdr->size = remaining - sizeof(t_header);
					newhdr->occupied = false;
					newhdr->zone = zone;

					hdr->size = size;
				}
				// else: not enough room for another header, we just don't touch the size

				hdr->occupied = true;
				result = (void *)(hdr + 1);
				break;
			}

			char *next_addr = (char *)(hdr + 1) + hdr->size;

			// If there's no room for another valid header, stop.
			if (next_addr + sizeof(t_header) > zone_end)
				break;

			hdr = (t_header *)next_addr;
		}

		zone = zone->next;
	}

	if (result)
		return result;

	// not enough room, create a new page
	zone = (t_zone *)add_page(E_SMALL);
	if (!zone)
		return (NULL);

	hdr = (t_header *)(zone + 1);

	if (hdr->size >= size)
	{
		uint32_t remaining = hdr->size - size;

		if (remaining > sizeof(t_header)) // should always be true (enough room for 100 allocs in a page)
		{
			char *new_addr = (char *)(hdr + 1) + size;
			t_header *newhdr = (t_header *)new_addr;

			newhdr->size = remaining - sizeof(t_header);
			newhdr->occupied = false;
			newhdr->zone = zone;

			hdr->size = size;
		}
		hdr->occupied = true;
		result = (void *)(hdr + 1);
	}
	else
	{
		// wtf, not possible, but nice to handle I guess
		result = NULL;
	}

	return (result);
}

// returns the start of the buffer (returned by malloc)
// size has to be alligned
void *add_tiny(uint32_t size)
{
	t_zone *zone;
	t_header *hdr;
	void *result = NULL;

	zone = g_state.tiny_zone;
	while (zone && !result)
	{
		char *zone_start = (char *)zone;
		char *zone_end = zone_start + zone->size;

		// First header is right after the zone struct
		hdr = (t_header *)(zone + 1);

		while ((char *)hdr + sizeof(t_header) <= zone_end)
		{
			if (!hdr->occupied && hdr->size >= size)
			{
				// Found a free block big enough
				uint32_t remaining = hdr->size - size;

				if (remaining > sizeof(t_header))
				{
					// Split the block: create a new header in the remaining space
					char *new_addr = (char *)(hdr + 1) + size;
					t_header *newhdr = (t_header *)new_addr;

					newhdr->size = remaining - sizeof(t_header);
					newhdr->occupied = false;
					newhdr->zone = zone;

					hdr->size = size;
				}
				// else: not enough room for another header, we just

				hdr->occupied = true;
				result = (void *)(hdr + 1);
				break;
			}

			char *next_addr = (char *)(hdr + 1) + hdr->size;

			// If there's no room for another valid header, stop.
			if (next_addr + sizeof(t_header) > zone_end)
				break;

			hdr = (t_header *)next_addr;
		}

		zone = zone->next;
	}

	if (result)
		return result;

	// not enough room, create a new page
	zone = (t_zone *)add_page(E_TINY);
	if (!zone)
		return (NULL);

	hdr = (t_header *)(zone + 1);

	// hdr->size should always be >= size here
	if (hdr->size >= size)
	{
		uint32_t remaining = hdr->size - size;

		if (remaining > sizeof(t_header)) // should always be true (enough room for 100 allocs in a page)
		{
			char *new_addr = (char *)(hdr + 1) + size;
			t_header *newhdr = (t_header *)new_addr;

			newhdr->size = remaining - sizeof(t_header);
			newhdr->occupied = false;
			newhdr->zone = zone;

			hdr->size = size;
		}
		hdr->occupied = true;
		result = (void *)(hdr + 1);
	}
	else
	{
		// wtf, not possible, but nice to handle I guess
		result = NULL;
	}

	return (result);
}

uint32_t vsize(virt_ptr ptr)
{
	t_header *header = ((t_header *)ptr) - 1;
	return (header->size);
}

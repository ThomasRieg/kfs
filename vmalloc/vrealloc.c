/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vrealloc.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 11:47:23 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/13 15:04:45 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "vmalloc_private.h"

// returns non-0 (1) is the zone changes type to a higher type (tiny to small or small to large)
static bool is_upgraded(uint32_t size, t_type current_type)
{
	if (size > SMALL_SIZE_MAX && current_type != E_LARGE)
		return (1);
	else if (size > TINY_SIZE_MAX && current_type != E_SMALL)
		return (1);
	else
		return (0);
}

virt_ptr vrealloc(virt_ptr ptr, uint32_t size)
{
	if (size == 0)
	{
		vfree(ptr);
		return (NULL);
	}
	else if (!ptr)
	{
		return (vmalloc(size));
	}
	uint32_t original_size = size;
	if (size % ALLIGN_BYTES)
		size = ((size / ALLIGN_BYTES) + 1) * ALLIGN_BYTES;
	virt_ptr ret = NULL;
	t_header *header = ((t_header *)ptr) - 1;
	if (header->zone->type == E_LARGE)
	{
		ret = vmalloc(size);
		if (!ret)
			return (NULL);
		if (original_size > header->size)
			memcpy(ret, ptr, header->size); // if grow, copy the entire last buffer
		else
			memcpy(ret, ptr, original_size); // if shrink, copy the new amount of bytes from older buffer
		vfree(ptr);
	}
	else if (is_upgraded(size, header->zone->type))
	{
		ret = vmalloc(size);
		if (!ret)
			return (NULL);
		if (original_size > header->size)
			memcpy(ret, ptr, header->size); // if grow, copy the entire last buffer
		else
			memcpy(ret, ptr, original_size); // if shrink, copy the new amount of bytes from older buffer
		vfree(ptr);
	}
	else if (header->zone->type == E_SMALL || header->zone->type == E_TINY)
	{
		t_header *next_header = (t_header *)(((char *)header) + sizeof(t_header) + header->size);
		if ((char *)next_header > (((char *)header->zone) + header->zone->size - sizeof(t_header)))
			next_header = NULL; // next header out of the zone
		if (size > header->size)
		{
			if (next_header && !next_header->occupied && (header->size + next_header->size + sizeof(t_header)) >= size)
			{
				const uint32_t available_space = (header->size + next_header->size + sizeof(t_header));
				if (available_space - size > sizeof(t_header))
				{
					// Split the block: create a new header in the remaining space
					char *new_addr = (char *)(header + 1) + size;
					t_header *newhdr = (t_header *)new_addr;

					newhdr->size = available_space - size - sizeof(t_header);
					newhdr->occupied = false;
					newhdr->zone = header->zone;

					header->size = size;
				}
				else
				{
					header->size = available_space;
				}
				return (ptr);
			}
			else
			{
				ret = vmalloc(size);
				if (!ret)
					return (NULL);
				if (original_size > header->size)
					memcpy(ret, ptr, header->size); // if grow, copy the entire last buffer
				else
					memcpy(ret, ptr, original_size); // if shrink, copy the new amount of bytes from older buffer
				vfree(ptr);
			}
		}
		else
		{
			ret = ptr;										// in all case we just return the original ptr, rest is just internal logic
			uint32_t available_space = header->size - size; // zones are defragmented so all available contiguous space must be in a single header
			if (next_header && !next_header->occupied)
				available_space += next_header->size + sizeof(t_header);
			if (available_space > sizeof(t_header))
			{
				if (available_space > sizeof(t_header))
				{
					// Split the block: create a new header in the remaining space
					char *new_addr = (char *)(header + 1) + size;
					t_header *newhdr = (t_header *)new_addr;

					newhdr->size = available_space - sizeof(t_header);
					newhdr->occupied = false;
					newhdr->zone = header->zone;

					header->size = size;
				}
				else
				{
					header->size = available_space;
				}
			}
		}
	}
	return (ret);
}

virt_ptr krealloc(virt_ptr ptr, uint32_t size)
{
	if (size == 0)
	{
		kfree(ptr);
		return (NULL);
	}
	else if (!ptr)
	{
		return (kmalloc(size));
	}
	uint32_t original_size = size;
	if (size % ALLIGN_BYTES)
		size = ((size / ALLIGN_BYTES) + 1) * ALLIGN_BYTES;
	virt_ptr ret = NULL;
	t_header *header = ((t_header *)ptr) - 1;
	if (header->zone->type == E_LARGE)
	{
		ret = kmalloc(size);
		if (!ret)
			return (NULL);
		if (original_size > header->size)
			memcpy(ret, ptr, header->size); // if grow, copy the entire last buffer
		else
			memcpy(ret, ptr, original_size); // if shrink, copy the new amount of bytes from older buffer
		kfree(ptr);
	}
	else if (is_upgraded(size, header->zone->type))
	{
		ret = kmalloc(size);
		if (!ret)
			return (NULL);
		if (original_size > header->size)
			memcpy(ret, ptr, header->size); // if grow, copy the entire last buffer
		else
			memcpy(ret, ptr, original_size); // if shrink, copy the new amount of bytes from older buffer
		kfree(ptr);
	}
	else if (header->zone->type == E_SMALL || header->zone->type == E_TINY)
	{
		t_header *next_header = (t_header *)(((char *)header) + sizeof(t_header) + header->size);
		if ((char *)next_header > (((char *)header->zone) + header->zone->size - sizeof(t_header)))
			next_header = NULL; // next header out of the zone
		if (size > header->size)
		{
			if (next_header && !next_header->occupied && (header->size + next_header->size + sizeof(t_header)) >= size)
			{
				const uint32_t available_space = (header->size + next_header->size + sizeof(t_header));
				if (available_space - size > sizeof(t_header))
				{
					// Split the block: create a new header in the remaining space
					char *new_addr = (char *)(header + 1) + size;
					t_header *newhdr = (t_header *)new_addr;

					newhdr->size = available_space - size - sizeof(t_header);
					newhdr->occupied = false;
					newhdr->zone = header->zone;

					header->size = size;
				}
				else
				{
					header->size = available_space;
				}
				return (ptr);
			}
			else
			{
				ret = kmalloc(size);
				if (!ret)
					return (NULL);
				if (original_size > header->size)
					memcpy(ret, ptr, header->size); // if grow, copy the entire last buffer
				else
					memcpy(ret, ptr, original_size); // if shrink, copy the new amount of bytes from older buffer
				kfree(ptr);
			}
		}
		else
		{
			ret = ptr;										// in all case we just return the original ptr, rest is just internal logic
			uint32_t available_space = header->size - size; // zones are defragmented so all available contiguous space must be in a single header
			if (next_header && !next_header->occupied)
				available_space += next_header->size + sizeof(t_header);
			if (available_space > sizeof(t_header))
			{
				if (available_space > sizeof(t_header))
				{
					// Split the block: create a new header in the remaining space
					char *new_addr = (char *)(header + 1) + size;
					t_header *newhdr = (t_header *)new_addr;

					newhdr->size = available_space - sizeof(t_header);
					newhdr->occupied = false;
					newhdr->zone = header->zone;

					header->size = size;
				}
				else
				{
					header->size = available_space;
				}
			}
		}
	}
	return (ret);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_vector.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/21 17:45:12 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/15 15:14:50 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_vector.h"
#include "libk.h"

// creates a vector of initial size size
// returns its address, or NULL if any allocations fails
t_vector *ft_create_vector(size_t size)
{
	t_vector *ret;

	ret = vmalloc(sizeof(t_vector));
	if (!ret)
		return (NULL);
	ret->buffer = vmalloc(sizeof(char) * size);
	if (!ret->buffer)
	{
		vfree(ret);
		return (NULL);
	}
	ret->index = 0;
	ret->size = size;
	ret->finished = 0;
	return (ret);
}

// append c to the vector dest
// returns NULL if the allocation fail and the adress of dest if all goes well
t_vector *ft_vector_pushback(t_vector *dest, char c)
{
	char *buff;

	if (dest->index >= dest->size)
	{
		buff = vmalloc(sizeof(char) * ((dest->size + 1) * 2));
		if (!buff)
			return (NULL);
		memcpy(buff, dest->buffer, dest->index);
		dest->size = (dest->size + 1) * 2;
		vfree(dest->buffer);
		dest->buffer = buff;
	}
	dest->buffer[dest->index] = c;
	dest->index++;
	return (dest);
}

// append size bytes at src to the vector dest
// returns NULL if the allocation fail and the adress of dest if all goes well
// returns NULL if dest or src is NULL without doing anything
t_vector *ft_vector_concat(t_vector *dest, const void *src, size_t size)
{
	char *buff;

	if (!dest || !src)
		return (NULL);
	if ((dest->index + size) >= dest->size)
	{
		buff = vmalloc(sizeof(char) * ((dest->size + size) * 2));
		if (!buff)
			return (NULL);
		memcpy(buff, dest->buffer, dest->index);
		dest->size = (dest->size + size) * 2;
		vfree(dest->buffer);
		dest->buffer = buff;
	}
	memcpy(dest->buffer + dest->index, src, size);
	dest->index += size;
	return (dest);
}

// duplicates the content of a vector into an appropriately sized string
// returns NULL if allocation fails or if vec or vec->buffer is NULL
char *ft_vtoc(t_vector *vec)
{
	char *ret;

	if (!vec)
		return (NULL);
	if (!vec->buffer)
		return (NULL);
	ret = vmalloc(sizeof(char) * (vec->index + 1));
	if (!ret)
		return (NULL);
	memcpy(ret, vec->buffer, vec->index);
	ret[vec->index] = '\0';
	return (ret);
}

// frees vec and all its members, sets vec to NULL and returns NULL
void ft_free_vector(t_vector **vec)
{
	if (!vec)
		return;
	if (!(*vec))
		return;
	if ((*vec)->buffer)
		vfree((*vec)->buffer);
	vfree(*vec);
	*vec = NULL;
}

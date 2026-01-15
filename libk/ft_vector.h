/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_vector.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/21 17:45:45 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/15 15:50:45 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_VECTOR_H
#define FT_VECTOR_H

#include "../common.h"
#include "../vmalloc/vmalloc.h"

typedef struct s_vector
{
	char *buffer;
	size_t index;
	size_t size;
	char finished;
} t_vector;

t_vector *ft_vector_concat(t_vector *dest, const void *src, size_t size);
char *ft_vtoc(t_vector *vec);
t_vector *ft_create_vector(size_t size);
void ft_free_vector(t_vector **vec);
t_vector *ft_vector_pushback(t_vector *dest, char c);

#endif

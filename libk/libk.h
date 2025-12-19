/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libk.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:48:14 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 03:08:28 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIBK_H
#define LIBK_H

void    *memset(void *dest, const char c, unsigned int bytes);
void    *memcpy(void *dest, const void *src, unsigned int bytes);
void    *memmove(void *dest, const void *src, unsigned int bytes);
int     memcmp(const void *s1, const void *s2, unsigned int n);
void	*ft_memchr(const void *s, int c, unsigned int n)

#endif
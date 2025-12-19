/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libk.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:48:14 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 05:21:07 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIBK_H
#define LIBK_H

void    *memset(void *dest, int c, unsigned int bytes);
void    *memcpy(void *dest, const void *src, unsigned int bytes);
void    *memmove(void *dest, const void *src, unsigned int bytes);
int     memcmp(const void *s1, const void *s2, unsigned int n);
void    *memchr(const void *s, int c, unsigned int n);



char	        *strchr(const char *s, int c);
unsigned int	strlen(const char *s);
unsigned int	strnonchr(char *str, char c);


void    int32_str_10(char out[12], int n);
void    uint32_str_10(char out[11], unsigned int n);
void    u32_to_hex(char out[9], unsigned int x, int upper);


int	printk(const char *str, ...);

#endif
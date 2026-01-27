/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   strstuff.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 03:58:49 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/16 15:32:09 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libk.h"

//returns the index of the first character c of the string
// /!\SEGFAULT if str is NULL
//returns ft_strlen(s) if c is \0
char	*strchr(const char *s, int c)
{
	if ((char)c == '\0')
		return ((char *)(s + strlen(s)));
	while (*s)
	{
		if (*s == (char)c)
			return ((char *) s);
		s++;
	}
	return (0);
}

//returns the index of the first character that ISN'T c
// /!\SEGFAULT if str is NULL
//returns 0 if c is \0
unsigned int	strnonchr(char *str, char c)
{
	unsigned int	i;

	i = 0;
	if (c == '\0')
		return (0);
	while (str[i] == c)
		i++;
	return (i);
}

unsigned int	strlen(const char *s)
{
	unsigned int	i;

	i = 0;
	while (s[i])
		i++;
	return (i);
}

int strcmp(const char *a, const char *b)
{
	unsigned int	i;

	i = 0;
	while (a[i] && a[i] == b[i])
		i++;
	return ((unsigned char)a[i] - (unsigned char)b[i]);
}

char invert_caps(char c) {
	if (c >= 'a' && c <= 'z')
		c -= ('a' - 'A');
	else if (c >= 'A' && c <= 'Z')
		c += ('a' - 'A');
	return c;
}

void hex_dump(unsigned char *data, unsigned int size) {
	const char *hex = "0123456789abcdef";
	for (unsigned int i = 0; i < size; i++)
	{
		printk("%c%c", hex[data[i] >> 4], hex[data[i] & 0xF]);
	}
}

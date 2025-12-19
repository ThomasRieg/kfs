/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   strstuff.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 03:58:49 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 04:25:49 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libk.h"

//returns the index of the first character c of the string
// /!\SEGFAULT if str is NULL
//returns ft_strlen(s) if c is \0
char	*strchr(const char *s, int c)
{
	if ((char)c == '\0')
		return ((char *)(s + ft_strlen(s)));
	while (*s)
	{
		if (*s == (char)c)
			return ((char *) s);
		s++;
	}
	return (NULL);
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


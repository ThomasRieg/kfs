/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   memstuff.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:50:39 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 11:59:47 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

void	*memchr(const void *s, int c, unsigned int n)
{
	const char	*str;

	str = (const char *)s;
	while (n--)
	{
		if (*str == (char)c)
			return ((void *)str);
		str++;
	}
	return (0);
}

int	memcmp(const void *s1, const void *s2, unsigned int n)
{
	const unsigned char	*st1;
	const unsigned char	*st2;

	st1 = (const unsigned char *)s1;
	st2 = (const unsigned char *)s2;
	while (n && (*st1 == *st2))
	{
		st1++;
		st2++;
		n--;
	}
	if (n == 0)
		return (0);
	return (*st1 - *st2);
}

void	*memcpy(void *dest, const void *src, unsigned int bytes)
{
	char				*str;
	unsigned long	    *str_packed;
	char				*str_src;
	unsigned long   	*str_src_packed;

	str_packed = (unsigned long *)dest;
	str_src_packed = (unsigned long *)src;
	while (bytes >= sizeof(unsigned long))
	{
		*str_packed++ = *str_src_packed++;
		bytes -= sizeof(unsigned long);
	}
	str = (char *)str_packed;
	str_src = (char *)str_src_packed;
	while (bytes > 0)
	{
		*str++ = *str_src++;
		bytes--;
	}
	return (dest);
}


void	*memset(void *dest, int c, unsigned int bytes)
{
	char				*str;
	unsigned long		*str_packed;
	unsigned long		cccc;
	unsigned long long				i;

	str = (char *)dest;
	while (bytes % sizeof(unsigned long))
	{
		*str++ = (unsigned char)c;
		bytes--;
	}
	str_packed = (unsigned long *)str;
	cccc = 0;
	i = 0;
	while (i < sizeof(unsigned long))
	{
		cccc |= ((unsigned long)(unsigned char)c) << (i * 8);
		i++;
	}
	while (bytes >= sizeof(unsigned long))
	{
		*str_packed++ = cccc;
		bytes -= sizeof(unsigned long);
	}
	return (dest);
}

static void	*ft_optimised_revcpy(void *dest, const void *src, unsigned int n)
{
	char				*str;
	unsigned long   	*str_packed;
	char				*str_src;
	unsigned long   	*str_src_packed;

	str = (char *)dest;
	str_src = (char *)src;
	while (n % sizeof(unsigned long))
	{
		n--;
		str[n] = str_src[n];
	}
	n /= sizeof(unsigned long);
	str_packed = (unsigned long *)dest;
	str_src_packed = (unsigned long *)src;
	while (n > 0)
	{
		n--;
		str_packed[n] = str_src_packed[n];
	}
	return (dest);
}

void	*memmove(void *dest, const void *src, unsigned int bytes)
{
	char		*str;
	const char	*str_src;

	if (!dest && !src)
		return (0);
	str = (char *)dest;
	str_src = (const char *)src;
	if (str > str_src)
	{
		if ((str + sizeof(unsigned long)) > str_src)
			return (ft_optimised_revcpy(dest, src, bytes));
		while (bytes-- > 0)
			str[bytes] = str_src[bytes];
	}
	else
	{
		memcpy(dest, src, bytes);
	}
	return (dest);
}


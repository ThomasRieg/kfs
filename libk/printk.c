/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printk.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 03:53:22 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/19 05:17:51 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../vga/vga.h"
#include "libk.h"
#include <stdarg.h>

static int	ft_case_c(va_list *args)
{
    char c = (char)va_arg(*args, int);
    write(&c, 1);
	return (1);
}

static int	ft_case_percent()
{
	write("%", 1);
	return (1);
}

static int	ft_case_s(va_list *args)
{
	char	*str_to_cat;
    unsigned int ret;

	str_to_cat = va_arg(*args, char *);
	if (!str_to_cat)
	{
        write("(null)", 6);
        ret = 6;
	}
	else
	{
        ret = strlen(str_to_cat);
        write(str_to_cat, ret);
	}
	return ((int)ret);
}

static int	ft_case_p(va_list *args)
{
	char	        str_to_cat[9];
	void	        *arg;
	unsigned int    index;
    unsigned int    ret = 0;

	arg = va_arg(*args, void *);
	if (!arg)
	{
		write("(nil)", 5);
        return (5);
	}
    write("0x", 2);
    ret += 2;
	u32_to_hex(str_to_cat, (unsigned int)arg, 0);
	index = strnonchr(str_to_cat, '0');
    if (!str_to_cat[index] && index > 0) index--; //only 0, print it anyway (shouldn't happen because it would enter the !arg branch)
    ret += strlen(str_to_cat) - index;
    write(str_to_cat + index, strlen(str_to_cat) - index);
	return ((int)ret);
}

static int	ft_case_u(va_list *args)
{
	char	str_to_cat[11];

    uint32_str_10(str_to_cat, va_arg(*args, unsigned int));
    unsigned int ret = strlen(str_to_cat);
    write(str_to_cat, ret);
	return ((int)ret);
}

static int	ft_case_x(va_list *args)
{
	char	str_to_cat[9];

    u32_to_hex(str_to_cat, va_arg(*args, unsigned int), 0);
    unsigned int index = strnonchr(str_to_cat, '0');
    if (!str_to_cat[index] && index > 0) index--; //only 0, print it anyway
    unsigned int ret = strlen(str_to_cat) - index;
    write(str_to_cat + index, strlen(str_to_cat) - index);
	return ((int)ret);
}

int	ft_case_upperx(va_list *args)
{
	char	str_to_cat[9];

    u32_to_hex(str_to_cat, va_arg(*args, unsigned int), 1);
    unsigned int index = strnonchr(str_to_cat, '0');
    if (!str_to_cat[index] && index > 0) index--; //only 0, print it anyway
    unsigned int ret = strlen(str_to_cat) - index;
    write(str_to_cat + index, strlen(str_to_cat) - index);
	return ((int)ret);
}

int	ft_case_d(va_list *args)
{
	char	str_to_cat[12];

    int32_str_10(str_to_cat, va_arg(*args, int));
	unsigned int ret = strlen(str_to_cat);
    write(str_to_cat, ret);
	return ((int)ret);
}

int	ft_case_i(va_list *args)
{
	char	str_to_cat[12];

    int32_str_10(str_to_cat, va_arg(*args, int));
    unsigned int ret = strlen(str_to_cat);
    write(str_to_cat, ret);
	return ((int)ret);
}



static int	handle_percent(const char *str, va_list *args)
{
	if (str[0] == '%' && str[1] == 'c')
		return (ft_case_c(args));
	if (str[0] == '%' && (str[1] == '%' || str[1] == '\0'))
		return (ft_case_percent());
	if (str[0] == '%' && str[1] == 's')
		return (ft_case_s(args));
	if (str[0] == '%' && str[1] == 'p')
		return (ft_case_p(args));
	if (str[0] == '%' && str[1] == 'u')
		return (ft_case_u(args));
	if (str[0] == '%' && str[1] == 'x')
		return (ft_case_x(args));
	if (str[0] == '%' && str[1] == 'X')
		return (ft_case_upperx(args));
	if (str[0] == '%' && (str[1] == 'd'))
		return (ft_case_d(args));
	if (str[0] == '%' && (str[1] == 'i'))
		return (ft_case_i(args));
	write(str, 2);
    return(2);
}


static int	ft_parsing_loop(
const char *str,
va_list *args
)
{
	char	*next_percent;
	int		nb_character;
    unsigned int ret = 0;

	while (*str)
	{
		next_percent = strchr(str, '%');
		if (!next_percent)
		{
			next_percent = strchr(str, '\0');
            ret += (next_percent - str);
            write(str, (next_percent - str));
			return ((int)ret);
		}
        ret += (next_percent - str);
        write(str, (next_percent - str));
		str = next_percent;
		nb_character = handle_percent(str, args);
        ret += nb_character;
        if (str[1])
		    str += 2; //change that if we add support for more than 1 character specifiers or parameter after the %
        else
            str++; //if this percent was the last character of the string
	}
	return ((int)ret);
}

int	printk(const char *str, ...)
{
	va_list		args;

	if (!str)
		return (-1);
	va_start(args, str);
    int ret = ft_parsing_loop(str, &args);
    va_end(args);
	return (ret);
}

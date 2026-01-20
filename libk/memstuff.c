/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   memstuff.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:50:39 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/20 15:25:22 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libk.h"

void *memchr(const void *s, int c, unsigned int n)
{
	const char *str;

	str = (const char *)s;
	while (n--)
	{
		if (*str == (char)c)
			return ((void *)str);
		str++;
	}
	return (0);
}

int memcmp(const void *s1, const void *s2, unsigned int n)
{
	const unsigned char *st1;
	const unsigned char *st2;

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

void *memcpy(void *dest, const void *src, unsigned int bytes)
{
	char *str;
	unsigned long *str_packed;
	char *str_src;
	unsigned long *str_src_packed;

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

void *memset(void *dest, int c, unsigned int bytes)
{
	char *str;
	unsigned long *str_packed;
	unsigned long cccc;
	unsigned long long i;

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

static void *ft_optimised_revcpy(void *dest, const void *src, unsigned int n)
{
	char *str;
	unsigned long *str_packed;
	char *str_src;
	unsigned long *str_src_packed;

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

void *memmove(void *dest, const void *src, unsigned int bytes)
{
	char *str;
	const char *str_src;

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

#include "../mem_page/mem_paging.h"

phys_ptr get_phys_ptr(virt_ptr va)
{
	uint32_t *pte = get_pte(va);
	return ((*pte & 0xFFFFF000) + (((uintptr_t)va) & 0x00000FFF));
}

static inline uint32_t align_down_u32(uint32_t x, uint32_t a)
{
	return x & ~(a - 1u);
}

static inline uint32_t align_up_u32(uint32_t x, uint32_t a)
{
	return (x + a - 1u) & ~(a - 1u);
}

#include "../mem_page/mem_defines.h"
#include "../mem_page/mem_paging.h"

// returns true if process have access to a zone
bool user_range_ok(virt_ptr uaddr, uint32_t size, bool write)
{
	if (size == 0)
		return 1;

	if ((uintptr_t)uaddr >= KERNEL_VIRT_BASE)
		return 0;

	uint32_t end = (uintptr_t)uaddr + size;
	if (end < (uintptr_t)uaddr) // overflow
		return 0;
	if (end > KERNEL_VIRT_BASE) // crosses into kernel
		return 0;

	uint32_t start_page = align_down_u32((uintptr_t)uaddr, PAGE_SIZE);
	uint32_t end_page = align_up_u32(end, PAGE_SIZE);

	for (uint32_t va = start_page; va < end_page; va += PAGE_SIZE)
	{
		uint32_t *pte = get_pte((virt_ptr)va);
		if (!pte)
			return 0;

		uint32_t p = *pte;

		// Must be mapped and user-accessible
		if (!(p & PTE_P))
			return 0;
		if (!(p & PTE_US))
			return 0;

		if (write && !(p & PTE_RW))
			return 0;
	}
	return 1;
}

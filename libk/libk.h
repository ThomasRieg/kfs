/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libk.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/19 02:48:14 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/05 17:33:29 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIBK_H
#define LIBK_H

#include "../common.h"

void *memset(void *dest, int c, unsigned int bytes);
void *memcpy(void *dest, const void *src, unsigned int bytes);
void *memmove(void *dest, const void *src, unsigned int bytes);
int memcmp(const void *s1, const void *s2, unsigned int n);
void *memchr(const void *s, int c, unsigned int n);
phys_ptr get_phys_ptr(virt_ptr va); // will pagefault if va is not mapped
typedef struct s_mm t_mm;
bool user_range_ok(const virt_ptr uaddr, uint32_t size, bool write, t_mm *mm);

char *strchr(const char *s, int c);
unsigned int strlen(const char *s);
unsigned int strnonchr(char *str, char c);
char invert_caps(char c);
int strcmp(const char *a, const char *b);

void int32_str_10(char out[12], int n);
void uint32_str_10(char out[11], unsigned int n);
void u32_to_hex(char out[9], unsigned int x, int upper);

int printk(const char *str, ...);

// print stack
void stack_dump_words(uint32_t bytes);
void stack_trace_ebp(uint32_t max_frames, uint32_t ebp);
void hex_dump(unsigned char *data, unsigned int size);

#include "vec.h"

#endif

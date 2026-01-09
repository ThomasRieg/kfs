/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vmalloc_private.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 16:47:20 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/09 16:15:14 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VMALLOC_PRIVATE_H
#define VMALLOC_PRIVATE_H

#include "../common.h"
#include "../libk/libk.h"
#include "../mem_page/mem_paging.h"
#include "../mem_page/mem_defines.h"
#include "vmalloc.h"

#define TINY_SIZE_MAX 64
#define SMALL_SIZE_MAX 4096
#define ALLIGN_BYTES 8

typedef enum e_type
{
	E_TINY,
	E_SMALL,
	E_LARGE
} t_type;

typedef struct s_zone
{
	t_type type;
	uint32_t size;
	uint32_t flags;
	struct s_zone *next;
	struct s_zone *prev;
} __attribute__((aligned(ALLIGN_BYTES))) t_zone;

typedef struct s_header
{
	uint32_t size;
	bool occupied;
	t_zone *zone;
} __attribute__((aligned(ALLIGN_BYTES))) t_header;

typedef struct s_state
{
	t_zone *tiny_zone;	// lock g_mut
	t_zone *small_zone; // lock g_mut
	t_zone *large_zone; // lock g_mut, 1 zone per alloc for large
} t_state;

extern t_state g_state;

void *add_large(uint32_t size, uint32_t flags);
void *add_small(uint32_t size, uint32_t flags);
void *add_tiny(uint32_t size, uint32_t flags);

#endif
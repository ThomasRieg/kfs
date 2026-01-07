/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vmalloc_private.h                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 16:47:20 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 18:00:54 by thrieg           ###   ########.fr       */
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
#define ALLIGN_BYTES _Alignof(max_align_t)

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
	struct s_zone *next; // lock g_mut
	struct s_zone *prev; // lock g_mut
} t_zone;

typedef struct s_header
{
	uint32_t size;   // lock
	bool occupied; // lock
	t_zone *zone;
	char padding[8];
} t_header;

typedef struct s_state
{
	t_zone *tiny_zone;	// lock g_mut
	t_zone *small_zone; // lock g_mut
	t_zone *large_zone; // lock g_mut, 1 zone per alloc for large
} t_state;

extern t_state g_state;

void *add_large(uint32_t size);
void *add_small(uint32_t size);
void *add_tiny(uint32_t size);
void *add_page(t_type type);

#endif
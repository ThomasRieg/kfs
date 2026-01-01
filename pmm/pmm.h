/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pmm.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 21:33:40 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/01 01:10:55 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

void        pmm_init(void *multiboot2_info);
uint32_t    pmm_alloc_frame(void);          // returns physical address (4KB aligned), 0 on OOM
void        pmm_free_frame(uint32_t pa);    // pa must be 4KB aligned

uint32_t    pmm_total_frames(void);
uint32_t    pmm_free_frames(void);
uint32_t    pmm_bitmap_pa_start(void);
uint32_t    pmm_bitmap_pa_end(void);


#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pmm.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 21:33:40 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 15:46:36 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "../common.h"

void pmm_init(void *multiboot2_info);
phys_ptr pmm_alloc_frame(void);				   // returns physical address (4KB aligned), 0 on OOM
phys_ptr pmm_alloc_frames(uint32_t nb_frames); // returns physical address (4KB aligned), 0 on OOM
void pmm_free_frame(phys_ptr pa);			   // pa must be 4KB aligned
void pmm_free_frame_from_va(virt_ptr va);

uint32_t pmm_total_frames(void);
uint32_t pmm_free_frames(void);
uint32_t pmm_bitmap_pa_start(void);
uint32_t pmm_bitmap_pa_end(void);

#endif
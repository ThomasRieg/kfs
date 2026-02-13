/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   munmap.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/13 05:38:12 by thrieg            #+#    #+#             */
/*   Updated: 2026/02/13 06:07:08 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mmap.h"
#include "../errno.h"
#include "../mem_page/mem_paging.h"
#include "../pmm/pmm.h"

static inline virt_ptr maxv(virt_ptr a, virt_ptr b){ return a > b ? a : b; }
static inline virt_ptr minv(virt_ptr a, virt_ptr b){ return a < b ? a : b; }

//only called if partial removal of a vma
static void zap_range_pages(t_vma *v, virt_ptr s, virt_ptr e)
{
    // s/e page-aligned, within user range
    for (virt_ptr va = s; va < e; va += PAGE_SIZE)
    {
        uint32_t *pte = get_pte(va);
        if (!pte) continue;
        uint32_t entry = *pte;
        if (!(entry & PTE_P)) continue;

        phys_ptr pa = (phys_ptr)(entry & 0xFFFFF000u);

        *pte = 0;
        invalidate_cache(va);

        if ((v->flags & MAP_SHARED) && v->backing == VMA_ANON && v->backing_obj)
        {
            
        }
        else
        {
            // owns the mapping
            pmm_free_frame(pa);
        }
    }
}

uint32_t munmap(t_mm *mm, virt_ptr addr, size_t len)
{
    if (!len) return (-EINVAL);
    if ((uintptr_t)addr & (PAGE_SIZE-1)) return (-EINVAL);

    uintptr_t alen = (uintptr_t)page_align_up((virt_ptr)len);
    if (alen < PAGE_SIZE)
        return (-ERANGE); //overflow
    virt_ptr start = addr;
    virt_ptr end   = addr + alen;
    if (end < start) return (-ERANGE); // overflow
    if (start >= (virt_ptr)KERNEL_VIRT_BASE) return (-EINVAL);
    if (end > (virt_ptr)KERNEL_VIRT_BASE) end = (virt_ptr)KERNEL_VIRT_BASE;

    t_vma *prev = NULL;
    t_vma *v = mm->vma_list;

    while (v)
    {
        if (end <= v->start) break;
        if (start >= v->end) { prev = v; v = v->next; continue; }

        virt_ptr os = maxv(start, v->start);
        virt_ptr oe = minv(end,   v->end);

        //remove
        if (start <= v->start && end >= v->end)
        {
            t_vma *next = v->next;
            free_vma(v);

            if (!prev)
                mm->vma_list = next;
            else
                prev->next = next;
            v = next;
            continue;
        }

        zap_range_pages(v, os, oe);

        if (start <= v->start && end < v->end)
        {
            v->start = end;

            prev = v;
            v = v->next;
            continue;
        }

        if (start > v->start && end >= v->end)
        {
            v->end = start;

            prev = v;
            v = v->next;
            continue;
        }

        // split middle
        // left keeps [v->start, start)
        // right becomes [end, v->end)
        t_vma *right = vma_clone(v);
        if (!right) return (-ENOMEM);

        right->start = end;

        v->end = start;

        right->next = v->next;
        v->next = right;

        // continue after right
        prev = right;
        v = right->next;
    }

    return (0);
}

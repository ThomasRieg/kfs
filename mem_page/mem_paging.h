/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mem_paging.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 19:37:32 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/16 18:29:15 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MEM_PAGING_H
#define MEM_PAGING_H

#include <stdint.h>
#include "../common.h"

#define PDE_INDEX(va) (((uint32_t)(va) >> 22) & 0x3FF)
#define PTE_INDEX(va) (((uint32_t)(va) >> 12) & 0x3FF)

// temporarily maps a PAGE_SIZE bytes of physical memory , RW ring 0 only (not thread safe, disable interrupts or parallelism that would call this function until kunmap returns)
void *kmap(phys_ptr physical_address);
void kunmap(void);

// pte can point to page directory entry instead if we want to map a page table itself, pte is a MAPPED memory that points to the physical address of the page table (for processes, use kmap to initialy temporarily map the process's page directory)
void map_page(phys_ptr physical_address, virt_ptr pte, uint32_t flags);
// pte can point to page directory entry instead if we want to map a page table itself, pte is a MAPPED memory that points to the physical address of the page table (for processes, use kmap to initialy temporarily map the process's page directory)
void unmap_page(virt_ptr ptr, virt_ptr pte);

// after changing a page table entry we must invalidate the cache
void invalidate_cache(virt_ptr va);

// assumes page directory is in page directory's last entry (pd[1023]), assumes va is mapped (or it will page fault) returns the page table entry describing the virtual address va
uint32_t *get_pte(virt_ptr va);

void write_cr3(phys_ptr phys);
void reload_cr3(void);

uint32_t read_cr0(void);

void write_cr0(uint32_t v);

void paging_init(void *multiboot2_info);

virt_ptr kmmap(virt_ptr start, uint32_t nb_map, uint32_t flags);
void kmunmap(virt_ptr ptr, uint32_t nb_map);

// small tester to call after paging is setup, crashes the kernel if memory is unstable
void mem_test_all(void);
void fill_memory(void);

#endif

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mem_defines.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 19:39:37 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 16:05:30 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// https://wiki.osdev.org/Paging

#ifndef MEM_DEFINES_H
#define MEM_DEFINES_H

#define PAGE_SIZE 4096u

#define PTE_P 0x00000001u		// present
#define PTE_RW 0x00000002u		// read/write able
#define PTE_US 0x00000004u		// ring 3 accessible
#define PTE_PWD 0x00000008u		// write through
#define PTE_PCD 0x00000010u		// cache disable
#define PTE_A 0x00000020u		// accessed
#define MMAP_CONTIG 0x00001000u // allocate contiguous physical memory in when calling kmmap (don't overlap with the physical pte flags range)

#define KMMAP_BASE 0x10000000u
#define KMMAP_LIMIT 0xFFFFFFFFu

#define PD_VA 0xFFFFF000u	   // last entry of page directory, maps to the physical address of page directory
#define PT_BASE_VA 0xFFC00000u // start of memory chunk managed by last page directory (last 4MB of virtual memory)

#endif
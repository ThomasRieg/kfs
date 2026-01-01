/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mem_defines.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 19:39:37 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/31 20:52:12 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

//https://wiki.osdev.org/Paging

#ifndef MEM_DEFINES_H
#define MEM_DEFINES_H

#define PAGE_SIZE 4096u

#define PTE_P 0x00000001u //present
#define PTE_RW 0x00000002u //read/write able
#define PTE_US 0x00000004u //ring 3 accessible
#define PTE_PWD 0x00000008u // write through
#define PTE_PCD 0x00000010u //cache disable
#define PTE_A 0x00000020u //accessed

#define PD_VA       0xFFFFF000u //last entry of page directory, maps to the physical address of page directory
#define PT_BASE_VA  0xFFC00000u //start of memory chunk managed by last page directory (last 4MB of virtual memory)

#endif
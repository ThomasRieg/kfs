/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vmalloc.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 17:13:46 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/09 17:13:48 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VMALLOC_H
#define VMALLOC_H

#include <stddef.h>
#include <stdbool.h>

// kmalloc guarantees that the physical memory is contiguous, you can just call get_phys_ptr(virt_ptr va) on the returned ptr to get the physical address
virt_ptr kmalloc(uint32_t size);
virt_ptr kcalloc(uint32_t nmemb, uint32_t size);

virt_ptr vmalloc(uint32_t size);
void vfree(virt_ptr ptr);
virt_ptr vcalloc(uint32_t nmemb, uint32_t size);
virt_ptr vrealloc(virt_ptr ptr, uint32_t size);
uint32_t vsize(virt_ptr ptr);

// these have the exact same behavior and signature, let's not copy paste code for no reason
#define kfree vfree
#define ksize vsize
#define krealloc vrealloc

#endif
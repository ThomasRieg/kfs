/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot2.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 21:35:48 by thrieg            #+#    #+#             */
/*   Updated: 2025/12/31 21:35:49 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

typedef struct s_mb2_info {
    uint32_t total_size;
    uint32_t reserved;
    // followed by tags
} __attribute__((packed)) t_mb2_info;

typedef struct s_mb2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) t_mb2_tag;

#define MB2_TAG_END      0
#define MB2_TAG_MMAP     6

typedef struct s_mb2_tag_mmap {
    uint32_t type;       // = 6
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    // followed by entries
} __attribute__((packed)) t_mb2_tag_mmap;

typedef struct s_mb2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;       // 1 = available
    uint32_t zero;
} __attribute__((packed)) t_mb2_mmap_entry;

#define MB2_MMAP_AVAILABLE 1

#endif
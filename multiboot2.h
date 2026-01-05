/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   multiboot2.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 21:35:48 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/05 17:40:15 by alier            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

struct multiboot2_header {
	int magic;
	int arch;
	int hdr_len;
	int checksum;
	int flags;
};

typedef struct s_mb2_info {
    uint32_t total_size;
    uint32_t reserved;
    // followed by tags
} __attribute__((packed)) t_mb2_info;

typedef struct s_mb2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) t_mb2_tag;

enum mb2_tag_type {
	MB2_TAG_END = 0,
	MB2_TAG_MMAP = 6,
};

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

enum mb2_mmap_entry_type {
	MB2_MMAP_AVAILABLE = 1,
};

#endif

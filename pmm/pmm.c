/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pmm.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 21:35:58 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/07 23:21:21 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "pmm.h"
#include "../multiboot2.h"
#include "../libk/libk.h"
#include "../mem_page/mem_defines.h"
#include "../vga/vga.h"
#include "../gdt/gdt.h"

// defined in linker script:
extern char __kernel_start;
extern char __kernel_end;

static inline uint32_t align_up_u32(uint32_t x, uint32_t a) { return (x + a - 1) & ~(a - 1); }
// static inline uint32_t align_dn_u32(uint32_t x, uint32_t a)   { return x & ~(a - 1); }
static inline uint64_t align_up_u64(uint64_t x, uint64_t a) { return (x + a - 1) & ~(a - 1); }
static inline uint64_t align_dn_u64(uint64_t x, uint64_t a) { return x & ~(a - 1); }

static uint8_t *g_bitmap = 0;
static uint32_t g_bitmap_bytes = 0;
static uint32_t g_total_frames = 0;
static uint32_t g_free_frames = 0;
static uint32_t g_bmp_start = 0;
static uint32_t g_bmp_end = 0;

// bit = 1 => used, bit = 0 => free
static inline void bit_set(uint32_t idx)
{
	g_bitmap[idx >> 3] |= (uint8_t)(1u << (idx & 7));
}
static inline void bit_clear(uint32_t idx)
{
	g_bitmap[idx >> 3] &= (uint8_t)~(1u << (idx & 7));
}
static inline int bit_test(uint32_t idx)
{
	return (g_bitmap[idx >> 3] >> (idx & 7)) & 1;
}

static void mark_used_range(uint64_t start_pa, uint64_t end_pa)
{
	start_pa = align_dn_u64(start_pa, PAGE_SIZE);
	end_pa = align_up_u64(end_pa, PAGE_SIZE);
	if (end_pa <= start_pa)
		return;

	uint32_t first = (uint32_t)(start_pa / PAGE_SIZE);
	uint32_t last = (uint32_t)(end_pa / PAGE_SIZE);

	if (first >= g_total_frames)
		return;
	if (last > g_total_frames)
		last = g_total_frames;

	for (uint32_t i = first; i < last; i++)
	{
		if (!bit_test(i))
		{
			bit_set(i);
			if (g_free_frames)
				g_free_frames--;
		}
		else
		{
			bit_set(i);
		}
	}
}

static void mark_free_range(uint64_t start_pa, uint64_t end_pa)
{
	start_pa = align_up_u64(start_pa, PAGE_SIZE);
	end_pa = align_dn_u64(end_pa, PAGE_SIZE);
	if (end_pa <= start_pa)
		return;

	uint32_t first = (uint32_t)(start_pa / PAGE_SIZE);
	uint32_t last = (uint32_t)(end_pa / PAGE_SIZE);

	if (first >= g_total_frames)
		return;
	if (last > g_total_frames)
		last = g_total_frames;

	for (uint32_t i = first; i < last; i++)
	{
		if (bit_test(i))
		{
			bit_clear(i);
			g_free_frames++;
		}
	}
}

static const t_mb2_tag_mmap *find_mmap_tag(const t_mb2_info *mbi)
{
	const uint8_t *p = (const uint8_t *)mbi + sizeof(t_mb2_info);
	const uint8_t *end = (const uint8_t *)mbi + mbi->total_size;

	while (p + sizeof(t_mb2_tag) <= end)
	{
		const t_mb2_tag *tag = (const t_mb2_tag *)p;
		if (tag->type == MB2_TAG_END)
			break;
		if (tag->type == MB2_TAG_MMAP)
			return (const t_mb2_tag_mmap *)tag;

		// tags are 8-byte aligned
		uint32_t next = (tag->size + 7u) & ~7u;
		p += next;
	}
	return 0;
}

void pmm_init(void *multiboot2_info)
{
	const t_mb2_info *mbi = (const t_mb2_info *)multiboot2_info;
	const t_mb2_tag_mmap *mmap = find_mmap_tag(mbi);

	// 1) Find highest physical address from the mmap (so we size the bitmap)
	uint64_t max_end = 0;
	if (mmap)
	{
		const uint8_t *p = (const uint8_t *)mmap + sizeof(t_mb2_tag_mmap);
		const uint8_t *end = (const uint8_t *)mmap + mmap->size;

		while (p + mmap->entry_size <= end)
		{
			const t_mb2_mmap_entry *e = (const t_mb2_mmap_entry *)p;
			uint64_t e_end = e->addr + e->len;
			if (e_end > max_end)
				max_end = e_end;
			p += mmap->entry_size;
		}
	}

	if (max_end < (uint64_t)(uintptr_t)&__kernel_end)
	{
		max_end = (uint64_t)(uintptr_t)&__kernel_end;
	}

	g_total_frames = (uint32_t)(align_up_u64(max_end, PAGE_SIZE) / PAGE_SIZE);
	g_bitmap_bytes = (g_total_frames + 7u) / 8u;

	// 2) Place bitmap right after kernel end (early boot identity mapping assumed)
	g_bmp_start = align_up_u32((uint32_t)(uintptr_t)&__kernel_end, PAGE_SIZE);
	g_bmp_end = align_up_u32(g_bmp_start + (uint32_t)g_bitmap_bytes, PAGE_SIZE);

	g_bitmap = (uint8_t *)(uintptr_t)g_bmp_start;

	// 3) Mark everything used initially
	memset(g_bitmap, 0xFF, (uint32_t)(g_bmp_end - g_bmp_start));
	g_free_frames = 0;

	// 4) Free all "available" ranges from multiboot mmap
	if (mmap)
	{
		const uint8_t *p = (const uint8_t *)mmap + sizeof(t_mb2_tag_mmap);
		const uint8_t *end = (const uint8_t *)mmap + mmap->size;

		printk("available physical memory:\n");
		while (p + mmap->entry_size <= end)
		{
			const t_mb2_mmap_entry *e = (const t_mb2_mmap_entry *)p;

			if (e->type == MB2_MMAP_AVAILABLE)
			{
				uint64_t end_pa = e->addr + e->len;
				printk("0x%x 0x%x -> 0x%x 0x%x: 0x%x 0x%x bytes\n", (unsigned int)(e->addr >> 32), (unsigned int)(e->addr & 0xFFFFFFFF), (unsigned int)(end_pa >> 32), (unsigned int)(end_pa & 0xFFFFFFFF), (unsigned int)(e->len >> 32), (unsigned int)(e->len & 0xFFFFFFFF));
				mark_free_range(e->addr, end_pa);
			}
			p += mmap->entry_size;
		}
	}

	// 5) Re-reserve critical regions
	// - never allocate frame 0
	mark_used_range(0, PAGE_SIZE);

	// - reserve kernel image itself
	mark_used_range((uint32_t)(uintptr_t)&__kernel_start, (uint32_t)(uintptr_t)&__kernel_end);

	// - reserve the bitmap storage itself
	mark_used_range(g_bmp_start, g_bmp_end);

	// - reserve the VGA screen info
	mark_used_range((uint32_t)(uintptr_t)g_vga_text_buf, (uint32_t)(uintptr_t)g_vga_text_buf + VGA_SIZE);

	// should be withint the first page anyway, but let's be robust
	mark_used_range(GDT_START, GDT_END);

	// - reserve multiboot info structure region
	mark_used_range((uint32_t)(uintptr_t)mbi, (uint32_t)(uintptr_t)mbi + mbi->total_size);
}

static uint32_t g_first_free = 0; // index to start searching for free frames

phys_ptr pmm_alloc_frame(void)
{
	// allign to 4 bytes
	uint32_t i = g_first_free;
	for (; i < g_total_frames && (i % (sizeof(uint32_t) * 8)); i++)
	{
		if (!bit_test(i))
		{
			bit_set(i);
			g_first_free = i;
			if (g_free_frames)
				g_free_frames--;
			return (phys_ptr)(i * PAGE_SIZE);
		}
	}
	// scan word by word
    uint32_t word_index = i >> 5; // goes from bit index to word index by /32
    uint32_t word_count = (g_total_frames + 31u) >> 5;

    for (; word_index < word_count; word_index++)
	{
        uint32_t *wptr = (uint32_t *)(g_bitmap + (word_index << 2));
        uint32_t word = *wptr;

        if (word != 0xFFFFFFFFu) {
            uint32_t free_mask = ~word;
            uint32_t bit = (uint32_t)__builtin_ctz(free_mask);
            uint32_t idx = (word_index << 5) + bit;

            if (idx >= g_total_frames)
                break;

            bit_set(idx);

            g_first_free = idx;
            if (g_free_frames) g_free_frames--;
            return (phys_ptr)(idx * PAGE_SIZE);
        }
    }
	return 0;
}

phys_ptr pmm_alloc_frames(uint32_t nb_frames)
{
	uint32_t max_page = g_total_frames - nb_frames;
	for (uint32_t i = g_first_free; i < max_page; i++)
	{
		if (bit_test(i))
			continue;
		else
		{
			uint32_t start = i;
			bool allfree = true;
			uint32_t nb_free = 0;
			for (; nb_free < nb_frames; nb_free++)
			{
				if (bit_test(i))
				{
					allfree = false;
					break;
				}
			}
			i += nb_free;
			if (allfree)
			{
				for (uint32_t j = start; j < i; j++)
					bit_set(j);
				g_first_free = i;
				if (g_free_frames > nb_frames)
					g_free_frames -= nb_frames;
				else
					g_free_frames = 0;
				return (phys_ptr)(start * PAGE_SIZE);
			}
		}
	}
	return 0;
}

void pmm_free_frame(phys_ptr pa)
{
	if ((pa & (PAGE_SIZE - 1u)) != 0)
		return; // must be aligned
	uint32_t idx = (uint32_t)(pa / PAGE_SIZE);
	if (idx >= g_total_frames)
		return;

	if (bit_test(idx))
	{
		bit_clear(idx);
		g_free_frames++;
		if (idx < g_first_free)
			g_first_free = idx;
	}
}

#include "../mem_page/mem_paging.h"

void pmm_free_frame_from_va(virt_ptr va)
{
	uint32_t *table = get_pte(va);
	phys_ptr pa = (*table) & 0xFFFFF000;
	pmm_free_frame(pa);
}

uint32_t pmm_total_frames(void)
{
	return g_total_frames;
}
uint32_t pmm_free_frames(void) { return g_free_frames; }
uint32_t pmm_bitmap_pa_start(void) { return g_bmp_start; }
uint32_t pmm_bitmap_pa_end(void) { return g_bmp_end; }

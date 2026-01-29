/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pmm.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/31 21:35:58 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/29 20:53:17 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "pmm.h"
#include "../multiboot2.h"
#include "../libk/libk.h"
#include "../mem_page/mem_defines.h"
#include "../vga/vga.h"
#include "../dt/dt.h"
#include "../vmalloc/vmalloc.h"

// defined in linker script:
extern char __kernel_start_phys;
extern char __kernel_end_phys;

static inline uint32_t align_up_u32(uint32_t x, uint32_t a) { return (x + a - 1) & ~(a - 1); }
// static inline uint32_t align_dn_u32(uint32_t x, uint32_t a)   { return x & ~(a - 1); }
static inline uint64_t align_up_u64(uint64_t x, uint64_t a) { return (x + a - 1) & ~(a - 1); }
static inline uint64_t align_dn_u64(uint64_t x, uint64_t a) { return x & ~(a - 1); }

static uint8_t *g_bitmap = 0;
static uint16_t *g_refcount_map = 0; // allocated with vmalloc
static bool g_refcount_init = false;
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

static uint32_t g_usable_frames = 0;

static uint32_t count_usable_frames(const t_mb2_tag_mmap *mmap)
{
	uint64_t sum = 0;
	if (!mmap)
		return 0;

	const uint8_t *p = (const uint8_t *)mmap + sizeof(*mmap);
	const uint8_t *end = (const uint8_t *)mmap + mmap->size;

	while (p + mmap->entry_size <= end)
	{
		const t_mb2_mmap_entry *e = (const t_mb2_mmap_entry *)p;
		if (e->type == MB2_MMAP_AVAILABLE)
		{
			uint64_t start = align_up_u64(e->addr, PAGE_SIZE);
			uint64_t stop = align_dn_u64(e->addr + e->len, PAGE_SIZE);
			if (stop > start)
				sum += (stop - start);
		}
		p += mmap->entry_size;
	}
	return (uint32_t)(sum / PAGE_SIZE);
}

extern uint32_t g_bootstrap_end_pa;

// need to still have the low-va identity map from the bootstrap paging init
void pmm_init(void *multiboot2_info)
{
	const t_mb2_info *mbi = (const t_mb2_info *)multiboot2_info;
	const t_mb2_tag_mmap *mmap = find_mmap_tag(mbi);

	// 1) Find highest physical address from the mmap (so we size the bitmap)
	uint64_t max_end = 0;
	if (mmap)
	{
		g_usable_frames = count_usable_frames(mmap);
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
	if (max_end > (uint64_t)UINT32_MAX)
		max_end = UINT32_MAX;

	if (max_end < (uint64_t)(uintptr_t)&__kernel_end_phys)
	{
		max_end = (uint64_t)(uintptr_t)&__kernel_end_phys;
	}

	g_total_frames = (uint32_t)(align_up_u64(max_end, PAGE_SIZE) / PAGE_SIZE);
	g_bitmap_bytes = (g_total_frames + 7u) / 8u;

	// 2) Place bitmap right after kernel end (early boot identity mapping assumed)
	uint32_t bmp_start_phys = align_up_u32((uint32_t)(uintptr_t)g_bootstrap_end_pa, PAGE_SIZE);
	uint32_t bmp_end_phys = align_up_u32(bmp_start_phys + (uint32_t)g_bitmap_bytes, PAGE_SIZE);

	extern void map_range_physmap_runtime(uint32_t pa_start, uint32_t pa_end, uint32_t flags);
	map_range_physmap_runtime(bmp_start_phys, bmp_end_phys, PTE_P | PTE_RW);
	g_bmp_start = bmp_start_phys + KERNEL_VIRT_BASE;
	g_bmp_end = bmp_end_phys + KERNEL_VIRT_BASE;
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
	mark_used_range((uint32_t)(uintptr_t)&__kernel_start_phys, (uint32_t)(uintptr_t)&__kernel_end_phys);

	// reserve the page directory and page tables allocated by boot_init
	mark_used_range((uint32_t)(uintptr_t)&__kernel_end_phys, g_bootstrap_end_pa);

	// - reserve the bitmap storage itself
	mark_used_range(bmp_start_phys, bmp_end_phys);

	// - reserve the VGA screen info
	mark_used_range((uint32_t)(uintptr_t)g_vga_text_buf, (uint32_t)(uintptr_t)g_vga_text_buf + VGA_SIZE);

	// should be within the first page anyway, but let's be robust
	mark_used_range(GDT_START, GDT_END);

	// - reserve multiboot info structure region
	mark_used_range((uint32_t)(uintptr_t)mbi, (uint32_t)(uintptr_t)mbi + mbi->total_size);
}

// needs memory init to be finished (needs vmalloc)
bool pmm_refcount_init()
{
	g_refcount_map = vcalloc(sizeof(*g_refcount_map) * g_total_frames, 1);
	if (!g_refcount_map)
		return (false);
	g_refcount_init = true;
	for (uint32_t i = 0; i < g_total_frames; i++)
		g_refcount_map[i] = bit_test(i) ? 1 : 0;
	return (true);
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
			if (g_refcount_init)
				g_refcount_map[i] = 1;
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

		if (word != 0xFFFFFFFFu)
		{
			uint32_t free_mask = ~word;
			uint32_t bit = (uint32_t)__builtin_ctz(free_mask);
			uint32_t idx = (word_index << 5) + bit;

			if (idx >= g_total_frames)
				break;

			bit_set(idx);

			g_first_free = idx;
			if (g_free_frames)
				g_free_frames--;
			if (g_refcount_init)
				g_refcount_map[idx] = 1;
			return (phys_ptr)(idx * PAGE_SIZE);
		}
	}
	return 0;
}

phys_ptr pmm_alloc_frames(uint32_t nb_frames)
{
	uint32_t max_page = g_total_frames - nb_frames;
	uint32_t word_count = (g_total_frames + 31u) >> 5;
	bool first_free = 1;
	for (uint32_t i = g_first_free; i < max_page; i++)
	{
		if (bit_test(i))
			continue;
		else
		{
			if (first_free)
			{
				first_free = false;
				g_first_free = i;
			}
			uint32_t start = i;
			bool allfree = true;
			uint32_t nb_free = 0;
			// allign to 32 bits to use words
			for (; nb_free < nb_frames && ((start + nb_free) & 0x0000001F); nb_free++)
			{
				if (i >= max_page)
					return (0);
				if (bit_test(start + nb_free))
				{
					allfree = false;
					break;
				}
			}
			if (!allfree)
			{
				i += nb_free;
				continue;
			}
			// scan word by word
			uint32_t word_index = (start + nb_free) >> 5; // goes from bit index to word index by /32
			for (; (word_index < word_count) && (nb_free < nb_frames) && allfree; word_index++)
			{
				uint32_t *wptr = (uint32_t *)(g_bitmap + (word_index << 2));
				uint32_t word = *wptr;

				if (word != 0x00000000u)
				{
					uint32_t bit = (uint32_t)__builtin_ctz(word); // first used bit
					uint32_t idx = (word_index << 5) + bit;

					nb_free = idx - start;
					if (nb_free < nb_frames)
					{
						allfree = false;
						break;
					}
					if (idx >= g_total_frames)
						return (0);
				}
				else
					nb_free += 32;
			}
			if (nb_free > nb_frames)
				nb_free = nb_frames;
			i += nb_free;
			if (allfree)
			{
				for (uint32_t j = start; j < i; j++)
					bit_set(j);
				if (g_free_frames > nb_frames)
					g_free_frames -= nb_frames;
				else
					g_free_frames = 0;
				if (g_refcount_init)
					for (uint32_t j = 0; j < nb_frames; j++)
						g_refcount_map[start + j] = 1;
				return ((phys_ptr)(start * PAGE_SIZE));
			}
		}
	}
	return 0;
}

// returns false if ref count for this frame is already max ref count
bool pmm_add_ref(phys_ptr frame)
{
	uint32_t index = frame >> 12;
	if (index >= g_total_frames)
		return (false);
	if (g_refcount_map[index] == UINT16_MAX)
		return (false);
	g_refcount_map[index]++;
	return (true);
}

uint32_t pmm_get_refs(phys_ptr frame)
{
	uint32_t index = frame >> 12;
	if (index >= g_total_frames)
		return (0);
	return (g_refcount_map[index]);
}

void pmm_free_frame(phys_ptr pa)
{
	if ((pa & (PAGE_SIZE - 1u)) != 0)
		return; // must be aligned
	uint32_t idx = (uint32_t)(pa / PAGE_SIZE);
	if (idx >= g_total_frames)
		return;
	if (g_refcount_init)
	{
		if (!g_refcount_map[idx])
			return; // frame not allocated
		g_refcount_map[idx]--;
		if (!g_refcount_map[idx])
		{
			if (bit_test(idx))
			{
				bit_clear(idx);
				g_free_frames++;
				if (idx < g_first_free)
					g_first_free = idx;
			}
		}
	}
	else
	{
		if (bit_test(idx))
		{
			bit_clear(idx);
			g_free_frames++;
			if (idx < g_first_free)
				g_first_free = idx;
		}
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

uint32_t pmm_usable_frames(void)
{
	return g_usable_frames;
}

uint32_t pmm_free_frames(void) { return g_free_frames; }
uint32_t pmm_bitmap_va_start(void) { return g_bmp_start; }
uint32_t pmm_bitmap_va_end(void) { return g_bmp_end; }

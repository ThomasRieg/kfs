/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mem_tester.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/08 00:19:00 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/08 17:30:54 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdint.h>
#include "../libk/libk.h"
#include "mem_defines.h"
#include "mem_paging.h"
#include "../vmalloc/vmalloc_private.h" //for macros like ALLIGN_BYTES for alignment check
#include "../tty/tty.h"

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

static void hexdump16(const void *ptr, uint32_t n)
{
	const uint8_t *p = (const uint8_t *)ptr;
	for (uint32_t i = 0; i < n; ++i)
	{
		if ((i & 15u) == 0)
			printk("\n  %p: ", (void *)(p + i));
		uint8_t b = p[i];
		const char *hex = "0123456789abcdef";
		char out[3] = {hex[b >> 4], hex[b & 0xF], 0};
		write(out, 2);
		write(" ", 1);
	}
	write("\n", 1);
}

__attribute__((noreturn)) static void test_panic(const char *msg)
{
	printk("\n[TEST PANIC] %s\n", msg);
	asm volatile("cli");
	for (;;)
		asm volatile("hlt");
}

#define TEST_FAILF(fmt, ...)                         \
	do                                               \
	{                                                \
		printk("\n[FAIL] " fmt "\n", ##__VA_ARGS__); \
		test_panic("allocator test failed");         \
	} while (0)

static inline uint32_t xorshift32(uint32_t *s)
{
	uint32_t x = *s;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*s = x;
	return x;
}

static uint32_t checksum_u32(const uint32_t *p, uint32_t n)
{
	uint32_t h = 0x9E3779B9u;
	for (uint32_t i = 0; i < n; ++i)
		h ^= p[i] + 0x9E3779B9u + (h << 6) + (h >> 2);
	return h;
}

static int is_aligned(void *p, uint32_t a)
{
	return (((uint32_t)(uintptr_t)p) & (a - 1u)) == 0;
}

// Write a pattern and verify it (returns 1 ok, 0 fail)
static int fill_and_check_u32(uint32_t *p, uint32_t words, uint32_t seed)
{
	for (uint32_t i = 0; i < words; ++i)
		p[i] = (seed ^ i) * 2654435761u;

	for (uint32_t i = 0; i < words; ++i)
	{
		uint32_t expect = (seed ^ i) * 2654435761u;
		if (p[i] != expect)
			return 0;
	}
	return 1;
}

static void test_kmmap_stress(uint32_t iters, uint32_t pages_each, uint32_t flags, const char *name, uint32_t *cs_out)
{
	uint32_t cs = 0;
	uint32_t seed = 0xC0FFEE01u;

	const uint32_t words = (pages_each * PAGE_SIZE) / 4;

	for (uint32_t t = 0; t < iters; ++t)
	{
		uint32_t *p = (uint32_t *)kmmap(NULL, pages_each, flags);
		if (!p)
			TEST_FAILF("kmmap returned NULL ( pages_each=%u flags=%u seed=%p)", pages_each, flags, (void *)seed);

		uint32_t s = xorshift32(&seed);
		if (!fill_and_check_u32(p, words, s))
			TEST_FAILF("couldn't read back memory during kmmap stress test ( pages_each=%u flags=%u seed=%p)", pages_each, flags, (void *)seed);

		// Mix checksum so silent corruption shows up
		cs ^= checksum_u32(p, (words > 64) ? 64 : words) ^ (uint32_t)(uintptr_t)p;

		kmunmap((virt_ptr)p, pages_each);
	}

	*cs_out ^= cs;
	printk("[kmmap] %s iters=%u pages=%u cs=%p [OK]\n",
		   name, iters, pages_each, (void *)cs);
}

static void test_kmmap_fragmentation(uint32_t rounds, uint32_t flags_contig, uint32_t flags_noncontig, uint32_t *cs_out)
{
	enum
	{
		HOLD = 8
	};
	uint32_t *hold[HOLD];
	uint32_t hold_pages[HOLD];

	for (uint32_t i = 0; i < HOLD; ++i)
	{
		hold[i] = NULL;
		hold_pages[i] = 0;
	}

	uint32_t cs = 0;
	uint32_t seed = 0xA11CE00Du;

	for (uint32_t r = 0; r < rounds; ++r)
	{
		uint32_t idx = xorshift32(&seed) % HOLD;

		if (hold[idx] && ((xorshift32(&seed) & 3u) == 0))
		{
			kmunmap(hold[idx], hold_pages[idx]);
			hold[idx] = NULL;
			hold_pages[idx] = 0;
		}

		if (!hold[idx])
		{
			uint32_t pages = 1 + (xorshift32(&seed) % 6);
			uint32_t use_contig = (xorshift32(&seed) & 1u);
			uint32_t f = use_contig ? flags_contig : flags_noncontig;

			uint32_t *p = (uint32_t *)kmmap(NULL, pages, f);
			if (!p)
				TEST_FAILF("kmmap returned NULL ( pages=%u f=%u seed=%p)", pages, f, (void *)seed);
			hold[idx] = p;
			hold_pages[idx] = pages;

			uint32_t words = (pages * PAGE_SIZE) / 4;
			uint32_t s = xorshift32(&seed);
			if (!fill_and_check_u32(p, words, s))
				TEST_FAILF("kmmap couldn't read back memory (p=%u words=%u seed=%p)", p, words, (void *)seed);
			cs ^= checksum_u32(p, (words > 32) ? 32 : words);
		}

		uint32_t *tmp = (uint32_t *)kmmap(NULL, 2, flags_noncontig);
		if (tmp)
		{
			if (!fill_and_check_u32(tmp, (2 * PAGE_SIZE) / 4, xorshift32(&seed)))
				TEST_FAILF("kmmap couldn't read back memory #2 (tmp=%u words=%u seed=%p)", tmp, (2 * PAGE_SIZE) / 4, (void *)seed);
			kmunmap(tmp, 2);
		}
		else
		{
			TEST_FAILF("kmmap returned NULL #2 ( pages=%u flags=%u seed=%p)", 2, flags_noncontig, (void *)seed);
		}
	}

	for (uint32_t i = 0; i < HOLD; ++i)
		if (hold[i])
			kmunmap(hold[i], hold_pages[i]);

	*cs_out ^= cs;
	printk("[kmmap] frag rounds=%u cs=%p [OK]\n", rounds, (void *)cs);
}

static void test_vmalloc_fragmentation(uint32_t count)
{
	virt_ptr ptrs[128];
	uint32_t sizes[128];
	uint32_t seed = 0x13579BDFu;

	if (count > 128)
		count = 128;

	for (uint32_t i = 0; i < count; ++i)
	{
		ptrs[i] = NULL;
		sizes[i] = 0;
	}

	for (uint32_t i = 0; i < count; ++i)
	{
		uint32_t sz = 1 + (xorshift32(&seed) % 2048);
		if ((i & 7u) == 0)
			sz = 16;
		if ((i & 7u) == 1)
			sz = 31;
		if ((i & 7u) == 2)
			sz = 32;
		if ((i & 7u) == 3)
			sz = 33;

		virt_ptr p = vmalloc(sz);
		ptrs[i] = p;
		sizes[i] = sz;

		if (!p)
		{
			TEST_FAILF("vmalloc returned NULL (phase=A i=%u sz=%u seed=%p)", i, sz, (void *)seed);
		}

		if (!is_aligned(p, ALLIGN_BYTES))
		{
			TEST_FAILF("bad alignment (phase=A i=%u sz=%u p=%p)", i, sz, p);
		}

		uint32_t vs = vsize(p);
		if (vs < sz)
		{
			TEST_FAILF("vsize < requested (phase=A i=%u p=%p sz=%u vsize=%u)",
					   i, p, sz, vs);
		}

		uint8_t *b = (uint8_t *)p;
		b[0] = 0xA5;
		b[sz - 1] = 0x5A;

		if (b[0] != 0xA5 || b[sz - 1] != 0x5A)
		{
			printk("[dbg] phase=A i=%u p=%p sz=%u\n", i, p, sz);
			hexdump16(p, (sz < 64) ? sz : 64);
			TEST_FAILF("cannot read back memory (phase=A i=%u p=%p sz=%u)", i, p, sz);
		}
	}

	for (uint32_t i = 0; i < count; i += 2)
	{
		if (!ptrs[i])
			TEST_FAILF("unexpected NULL in ptrs (phase=B i=%u)", i);

		uint8_t *b = (uint8_t *)ptrs[i];
		uint32_t sz = sizes[i];
		if (b[0] != 0xA5 || b[sz - 1] != 0x5A)
		{
			printk("[dbg] phase=B pre-free i=%u p=%p sz=%u\n", i, ptrs[i], sz);
			hexdump16(ptrs[i], (sz < 64) ? sz : 64);
			TEST_FAILF("corrupted block before free (phase=B i=%u p=%p sz=%u)", i, ptrs[i], sz);
		}

		vfree(ptrs[i]);
		ptrs[i] = NULL;
	}

	for (uint32_t k = 0; k < count / 2; ++k)
	{
		uint32_t sz = 64 + (xorshift32(&seed) % 4096);

		virt_ptr p = vmalloc(sz);
		if (!p)
		{
			TEST_FAILF("vmalloc returned NULL (phase=C k=%u sz=%u)", k, sz);
		}

		if (!is_aligned(p, ALLIGN_BYTES))
		{
			TEST_FAILF("bad alignment (phase=C k=%u sz=%u p=%p)", k, sz, p);
		}

		uint32_t vs = vsize(p);
		if (vs < sz)
		{
			TEST_FAILF("vsize < requested (phase=C k=%u p=%p sz=%u vsize=%u)", k, p, sz, vs);
		}

		uint8_t *b = (uint8_t *)p;
		b[0] = 0xCC;
		b[sz - 1] = 0x33;

		if (b[0] != 0xCC || b[sz - 1] != 0x33)
		{
			printk("[dbg] phase=C k=%u p=%p sz=%u\n", k, p, sz);
			hexdump16(p, (sz < 64) ? sz : 64);
			TEST_FAILF("cannot read back canaries (phase=C k=%u p=%p sz=%u)", k, p, sz);
		}

		vfree(p);
	}

	for (uint32_t i = 1; i < count; i += 2)
	{
		if (!ptrs[i])
			TEST_FAILF("unexpected NULL in ptrs (phase=D i=%u)", i);

		uint8_t *b = (uint8_t *)ptrs[i];
		uint32_t sz = sizes[i];
		if (b[0] != 0xA5 || b[sz - 1] != 0x5A)
		{
			printk("[dbg] phase=D pre-free i=%u p=%p sz=%u\n", i, ptrs[i], sz);
			hexdump16(ptrs[i], (sz < 64) ? sz : 64);
			TEST_FAILF("corrupted block before free (phase=D i=%u p=%p sz=%u)", i, ptrs[i], sz);
		}

		vfree(ptrs[i]);
		ptrs[i] = NULL;
	}

	printk("[vmalloc] rounds=%u [OK]\n", count);
}

static void test_vrealloc_stress(uint32_t iters, uint32_t *cs_out)
{
	uint32_t cs = 0;
	uint32_t seed = 0xDEADBEEFu;

	for (uint32_t i = 0; i < iters; ++i)
	{
		uint32_t a = 1 + (xorshift32(&seed) % 256);
		uint32_t b = 1 + (xorshift32(&seed) % 8192);

		uint8_t *p = (uint8_t *)vmalloc(a);
		if (!p)
			TEST_FAILF("vmalloc returned NULL (size=%u seed=%p)", a, (void *)seed);

		for (uint32_t k = 0; k < a; ++k)
			p[k] = (uint8_t)(k ^ 0x5A);

		uint8_t *q = (uint8_t *)vrealloc(p, b);
		if (!q)
			TEST_FAILF("vrealloc returned NULL (p=%u b=%u seed=%p)", p, b, (void *)seed);

		// must preserve first min(a,b) bytes
		uint32_t m = (a < b) ? a : b;
		for (uint32_t k = 0; k < m; ++k)
		{
			if (q[k] != (uint8_t)(k ^ 0x5A))
				TEST_FAILF("vrealloc didn't copy the first bytes correctly (p=%u b=%u seed=%p)", p, b, (void *)seed);
		}

		// shrink sometimes
		if ((xorshift32(&seed) & 1u) == 0)
		{
			uint32_t c = 1 + (xorshift32(&seed) % 128);
			uint8_t *r = (uint8_t *)vrealloc(q, c);
			if (!r)
				TEST_FAILF("vrealloc returned NULL (q=%u c=%u seed=%p)", q, c, (void *)seed);
			uint32_t mm = (m < c) ? m : c;
			for (uint32_t k = 0; k < mm; ++k)
			{
				if (r[k] != (uint8_t)(k ^ 0x5A))
					TEST_FAILF("vrealloc didn't copy the first bytes correctly when shrink (q=%u c=%u seed=%p)", q, c, (void *)seed);
			}
			cs ^= vsize(r);
			vfree(r);
		}
		else
		{
			cs ^= vsize(q);
			vfree(q);
		}
	}

	*cs_out ^= cs;
	printk("[vrealloc] iters=%u cs=%p [OK]\n", iters, (void *)cs);
}

void mem_test_all(void)
{
	uint32_t cs = 0;

	uint8_t foreground;
	uint8_t background;
	vga_get_color(&foreground, &background);
	vga_set_color(VGA_GREEN, VGA_BLACK);
	printk("[MEMORY TEST] cs=%p\n", (void *)cs);
	// kmmap: fast repeated
	test_kmmap_stress(8, 2056, PTE_RW | MMAP_CONTIG, "contig", &cs);
	test_kmmap_stress(8, 2056, PTE_RW, "noncontig", &cs);

	// kmmap: fragmentation
	test_kmmap_fragmentation(1024, PTE_RW | MMAP_CONTIG, PTE_RW, &cs);

	// vmalloc/vfree fragmentation
	test_vmalloc_fragmentation(1024);

	// vrealloc stress
	test_vrealloc_stress(1024, &cs);

	printk("[DONE] cs=%p\n", (void *)cs);
	vga_set_color(foreground, background);
}

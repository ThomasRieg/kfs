/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_stack.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thrieg < thrieg@student.42mulhouse.fr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/24 00:19:03 by thrieg            #+#    #+#             */
/*   Updated: 2026/01/08 15:07:36 by thrieg           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stddef.h>
#include "libk.h"
#include "../tty/tty.h"

extern char __kstack_bottom; // defined in link.ld
extern char __kstack_top;	 // defined in link.ld

static inline uint32_t read_esp(void)
{
	uint32_t v;
	asm volatile("mov %%esp, %0" : "=r"(v));
	return v;
}

static inline uint32_t read_ebp(void)
{
	uint32_t v;
	asm volatile("mov %%ebp, %0" : "=r"(v));
	return v;
}

static inline uint32_t read_eip_approx(void)
{
	// We can't directly read EIP in x86, but CALL/POP trick gives current location
	uint32_t eip;
	asm volatile(
		"call 1f\n"
		"1: pop %0\n"
		: "=r"(eip)
		:
		: "memory");
	return eip;
}

static void print_hex(uint8_t byte)
{
	char buf[2];
	if ((byte / 16) < 10)
		buf[0] = (byte / 16) + '0';
	else
		buf[0] = (byte / 16) - 10 + 'a';
	if ((byte % 16) < 10)
		buf[1] = (byte % 16) + '0';
	else
		buf[1] = (byte % 16) - 10 + 'a';
	write(buf, 2);
}

void stack_dump_words(uint32_t bytes)
{
	uint32_t esp = read_esp();
	uint32_t ebp = read_ebp();
	uint32_t eip = read_eip_approx();
	if (bytes == 0)
		bytes = ((uint32_t)&__kstack_top - (uint32_t)esp);

	printk("=== kernel stack dump ===\n");
	printk("eip~=%p  ebp=%p  esp=%p\n", (void *)eip, (void *)ebp, (void *)esp);
	printk("stack top=%p  stack bottom=%p  used=%u bytes (%u%%)\n", (void *)&__kstack_top, (void *)&__kstack_bottom, (uint32_t)&__kstack_top - esp, (((uint32_t)&__kstack_top - esp) * 100) / ((uint32_t)&__kstack_top - (uint32_t)&__kstack_bottom));

	uint8_t *p = (uint8_t *)&__kstack_top - 1;

	// Print 8 bytes per line
	for (uint32_t i = 0; i < bytes; i += 8)
	{
		printk("%p: ", (void *)(p - i)); // substracts because stacks grows from top to bot (careful gank 3:30)
		for (uint32_t j = 0; j < 8 && (i + j) < bytes; ++j)
		{
			print_hex(*(p - i - j));
			if (j < 7)
				write(" ", 1);
		}
		write("\n", 1);
	}
	printk("=========================\n");
}

// Very basic frame-pointer walk (works best if compiled with -fno-omit-frame-pointer)
void stack_trace_ebp(uint32_t max_frames, uint32_t ebp)
{
	printk("=== stack trace (ebp chain) ===\n");
	printk("start ebp=%p\n", (void *)ebp);

	for (uint32_t frame = 0; frame < max_frames; ++frame)
	{
		if (ebp == 0)
			break;

		// Frame layout (typical):
		// [ebp + 0] = prev ebp
		// [ebp + 4] = return eip
		uint32_t *fp = (uint32_t *)ebp;

		// Basic sanity: alignment and “monotonic” stack direction (stack grows down, ebp increases when unwinding)
		if (((uintptr_t)fp & 0x3) != 0)
			break;

		uint32_t prev_ebp = fp[0];
		uint32_t ret_eip = fp[1];

		printk("#%u  ebp=%p  ret=%p  prev=%p\n",
			   frame, (void *)ebp, (void *)ret_eip, (void *)prev_ebp);

		// Stop if it loops or goes weird
		if (prev_ebp <= ebp)
			break;

		ebp = prev_ebp;
	}
	printk("=====================================\n");
}

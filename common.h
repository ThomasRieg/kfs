#ifndef COMMON_H

#define COMMON_H

#include "io.h"

#include <stddef.h>
#include "interrupts/s_regs.h"

#if __STDC_VERSION__ < 202311l

typedef _Bool bool;
#define false 0
#define true 1

#endif

#define KERNEL_PHYS_BASE 0x00200000u // must match linker
#define KERNEL_VIRT_BASE 0xC0000000u // must match linker

typedef void *virt_ptr;
typedef uint32_t phys_ptr;

struct interrupt_stack_frame
{
	void *instruction_pointer;
	unsigned short cs_selector;
	unsigned short _padding;
	unsigned int flags;
} __attribute__((packed));

enum interrupt
{
	INT_BREAKPOINT = 3,
	INT_DOUBLE_FAULT = 8,
	INT_PAGE_FAULT = 14,
	INT_TIMER = PIC_OFFSET,
	INT_KEYBOARD,
	INT_SERIAL1 = PIC_OFFSET + 4,
	INT_NIC = PIC_OFFSET + 11,
	INT_PRIMARY_ATA = PIC_OFFSET + 14,
	INT_SECONDARY_ATA = PIC_OFFSET + 15,
	INT_SYSCALLS = 0x80u,
};

void kernel_panic(const char *message, t_interrupt_data *regs);

static inline void disable_interrupts(void)
{
	asm volatile("cli");
}

static inline void enable_interrupts(void)
{
	asm volatile("sti");
}

#endif

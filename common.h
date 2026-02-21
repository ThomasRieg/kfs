#ifndef COMMON_H

#define COMMON_H

#include "io.h"

#include <stddef.h>
#include <stdint.h>
#include "interrupts/s_regs.h"

#if __STDC_VERSION__ < 202311l

typedef _Bool bool;
#define false 0
#define true 1

#endif

#define KERNEL_PHYS_BASE 0x00200000u // must match linker
#define KERNEL_VIRT_BASE 0xC0000000u // must match linker

#define PAGE_ALIGN_DOWN(x) ((uintptr_t)(x) & ~0xFFFu)

#define TIMER_TICK_PER_SECOND 18 //todo change to the actual value, I just measured how long a sleep 1000 was

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
	INT_GENERAL_PROTECTION_FAULT = 13,
	INT_PAGE_FAULT = 14,
	INT_TIMER = PIC_OFFSET,
	INT_KEYBOARD,
	INT_SERIAL1 = PIC_OFFSET + 4,
	INT_NIC = PIC_OFFSET + 11,
	INT_PRIMARY_ATA = PIC_OFFSET + 14,
	INT_SECONDARY_ATA = PIC_OFFSET + 15,
	INT_SYSCALLS = 0x80u,
	INT_YIELD = 0x81u, // forces a yield, only called from ring 0
};

struct timespec
{
	unsigned int tv_sec;
	unsigned int tv_nsec;
};
struct timespec64
{
	unsigned long long int tv_sec;
	unsigned int tv_nsec;
};
struct timespec rtc_get_time(void);

typedef unsigned long int dev_t;
typedef unsigned long int ino_t;

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	unsigned int st_nlink;
	unsigned int st_mode;
	unsigned int st_uid;
	unsigned int st_gid;
	int __pad0;
	dev_t st_rdev;
	unsigned int st_size;
	unsigned int st_blksize;
	unsigned int st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
};

struct stat64
{
	unsigned long long st_dev;
	unsigned char __pad0[4];
	ino_t __st_ino;
	unsigned int st_nlink;
	unsigned int st_mode;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long long st_rdev;
	unsigned char __pad3[4];
	long long st_size;
	unsigned int st_blksize;
	long long st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	unsigned long long st_ino;
} __attribute__((packed));

struct linux_dirent64
{
	uint64_t d_ino;			 /* 64-bit inode number */
	uint64_t d_off;			 /* 64-bit offset to next structure */
	unsigned short d_reclen; /* Size of this dirent */
	unsigned char d_type;	 /* File type */
	char d_name[];			 /* Filename (null-terminated) */
};

__attribute__((noreturn)) void kernel_panic(const char *message, t_interrupt_data *regs);

static inline void disable_interrupts(void)
{
	asm volatile("cli");
}

static inline void enable_interrupts(void)
{
	asm volatile("sti");
}

enum e_print_level
{
	PRINT_TRACE = 0,
    PRINT_DEBUG = 1,
    PRINT_INFO  = 2,
    PRINT_WARN  = 3,
    PRINT_ERROR = 4,
};

extern enum e_print_level debug_print_level; //set in main.c

#define printk_if(level, fmt, ...)                           \
    do {                                                      \
        if ((level) >= debug_print_level)                    \
            printk(fmt, ##__VA_ARGS__);                       \
    } while (0)

#define print_trace(fmt, ...) printk_if(PRINT_TRACE, fmt, ##__VA_ARGS__)
#define print_debug(fmt, ...) printk_if(PRINT_DEBUG, fmt, ##__VA_ARGS__)
#define print_info(fmt, ...)  printk_if(PRINT_INFO,  fmt, ##__VA_ARGS__)
#define print_warn(fmt, ...)  printk_if(PRINT_WARN,  fmt, ##__VA_ARGS__)
#define print_err(fmt, ...)   printk_if(PRINT_ERROR, fmt, ##__VA_ARGS__)

#endif
